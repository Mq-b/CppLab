# Linux 劫持系统函数

## 起因

为什么会突然想到这个呢？

其实是这样的，最近突然心血来潮，想用 C++ 写一个 ftp 服务器，我这种选手自然不会搓轮子，而是使用了 GitHub 上找到的三方库 [`fineftp-server`](https://github.com/eclipse-ecal/fineftp-server)，接口设计非常简单，也几乎没额外依赖。

我在服务器上很轻松的编译构建，使用 cmake 集成了它，并编写了一个简单的测试程序。

```cpp
#include <fineftp/server.h>
#include <fineftp/permissions.h>
#include <thread>

int main() {
    // 创建 FTP 服务器，监听 2121 端口
    fineftp::FtpServer ftp_server(2121);
    
    // 添加一个具名用户 "user1"，密码 "1234"，只读权限，访问目录为 /home/ecs-user/ftp 目录
    ftp_server.addUser("user1", "1234", "/home/ecs-user/ftp", fineftp::Permission::ReadOnly);

    // 启动 FTP 服务器，线程池大小为 4
    ftp_server.start(4);

    // 防止程序退出
    for (;;) std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

但在使用 `curl` 命令行工具进行 FTP 下载测试时，遇到了一些问题。测试命令如下：

```bash
curl -u user1:1234 ftp://47.100.34.70:2121/test.txt -o downloaded.txt
```

通过分析 FTP 服务端的日志，我发现客户端能够正常连接并完成身份验证，但在下载时却发生了错误。进一步检查后，我发现 FTP 服务端在被动模式（PASV）下**返回了一个内网IP地址**，而客户端自然无法通过内网IP进行下载。

事实上这个服务器确实自己也不知道自己的公网IP地址，`ifconfig` 命令查看到的也是内网IP。

此时就有三种可能的解决方案：

1. 创建一个虚拟环境，这个虚拟环境中我设置了内网ip就是公网ip，获取的自然就是对的公网ip。
2. 直接修改库源代码，返回一个固定的公网IP地址。
3. **劫持系统函数**，让其返回一个固定的公网IP地址。

很显然，我采用了第三种办法。

## 劫持系统函数

> [!IMPORTANT]
> 首先我们稍作分析，发现 [`fineftp-server`](https://github.com/eclipse-ecal/fineftp-server)是依赖于 `Asio` 实现的。然而在 Linux 系统下，它底层依然是对 POSIX 网络 API 的封装。
>
> 因此，服务端在获取套接字本地地址时，最终调用的很可能就是 [`getsockname`](https://pubs.opengroup.org/onlinepubs/007904875/functions/getsockname.html) 函数。该函数用于获取指定套接字当前绑定的本地地址信息（IP 和端口），并将其写入提供的 `sockaddr` 结构体中，同时返回地址的实际长度。
>
> 所以我们可以通过劫持这个函数来返回一个我们固定的公网 IP 地址来解决问题。

> [!NOTE]
> 具体来说，我们将编写一个新的 `getsockname` 函数，强制它返回我们指定的公网 IP 地址，将其编译为`.so`动态库 ，覆盖 `getsockname()` 函数的默认行为。然后通过使用 [**`LD_PRELOAD`**](https://axcheron.github.io/playing-with-ld_preload/) 变量，设置在运行时优先加载这个自定义的动态库，从而实现劫持。
>
> ```shell
> LD_PRELOAD=./libfakeip.so ./Ftp-Server
> ```

下面是一个简单的劫持函数 `getsockname`，我们将在其中替换返回的 IP 地址，确保返回的是我们指定的公网 IP 地址。

```cpp
// fakeip.c
#define _GNU_SOURCE
#include <dlfcn.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stddef.h>

// 自定义的 getsockname 函数
int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    // 获取原始的 getsockname 函数地址
    static int (*real_getsockname)(int, struct sockaddr*, socklen_t*) = NULL;
    if (!real_getsockname)
        // dlsym 获取原始系统库的 getsockname 函数地址
        real_getsockname = dlsym(RTLD_NEXT, "getsockname");

    // 调用原始的 getsockname 函数
    int ret = real_getsockname(sockfd, addr, addrlen);

    // 获取 sockaddr_in 结构体，以便修改其 IP 地址
    struct sockaddr_in* sin = (struct sockaddr_in*)addr;

    // 打印原始获取到的 IP 地址
    char original_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(sin->sin_addr), original_ip, INET_ADDRSTRLEN);
    printf("Original IP: %s\n", original_ip);  // 打印原始 IP 地址

    // 强制返回我们指定的公网 IP 地址
    sin->sin_addr.s_addr = inet_addr("47.100.34.70");  // 公网 IP 地址

    // 返回原始结果
    return ret;
}
```

[`dlsym`](https://pubs.opengroup.org/onlinepubs/9699919799/) 函数类似于 windows 的 [`LoadLibrary`](https://learn.microsoft.com/zh-cn/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibrarya) + [`GetProcAddress`](https://learn.microsoft.com/zh-cn/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress)，它用于获取动态链接库中的函数地址。然后直接调用。

`dlsym` 通常配合 [`dlopen`](https://pubs.opengroup.org/onlinepubs/9699919799/) 使用，但在这里我们使用 `RTLD_NEXT` 来获取下一个匹配的函数地址，这样可以确保我们调用的是系统的原始 `getsockname` 函数。

> [!Tip]
> 简单来说，`RTLD_NEXT` 就是告诉 `dlsym`： 别找我自己写的这个函数，帮我去找系统或别的库里同名的那个原始函数。

直接将其编译为动态库：

```shell
gcc -shared -fPIC -o libfakeip.so fakeip.c -ldl
```

我们写一个使用 `getsockname` 函数获取当前 ip 的程序，然后利用 `LD_PRELOAD` 环境变量优先加载 `libfakeip.so` 看看能否劫持成功。

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        return 1;
    }

    // 远程服务器地址 (例如使用 Google 的 DNS 地址)
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("8.8.8.8");  // 使用 Google 公共 DNS 地址
    server_addr.sin_port = htons(53);  // 使用 DNS 的端口

    // 尝试连接到远程服务器
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        close(sockfd);
        return 1;
    }

    // 获取本地套接字地址
    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    if (getsockname(sockfd, (struct sockaddr*)&local_addr, &addr_len) < 0) {
        perror("getsockname failed");
        close(sockfd);
        return 1;
    }

    // 打印本地 IP 地址
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &local_addr.sin_addr, ip_str, sizeof(ip_str));
    printf("Local IP: %s\n", ip_str);

    close(sockfd);
}
```

```shell
$ gcc test.c -o test
$ LD_PRELOAD=./libfakeip.so ./test
Original IP: 172.16.89.101
Local IP: 47.100.34.70
```

可以看到，函数劫持已经成功生效。
其中，`Original IP` 是在 `libfakeip.so` 中调用原始 `getsockname` 时打印的真实本地 IP；而 `Local IP` 则是 `test` 程序调用 `getsockname` 得到的结果，也就是我们通过劫持强制返回的公网 IP。

## 参考资料
