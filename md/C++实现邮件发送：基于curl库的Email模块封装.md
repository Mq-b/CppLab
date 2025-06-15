# C++实现邮件发送：基于 curl 库的 Email 模块封装

在自动化脚本和各类应用程序中，发送邮件是一个非常常见的需求。我们可以通过使用 [**`curl`**](https://github.com/curl/curl) 库，在C++中高效地实现邮件发送功能，满足多种实际场景的需求。

> [!TIP]
> **`curl`** 是一个功能强大的开源库，广泛用于在各种编程语言中进行网络通信。它不仅支持`HTTP/HTTPS`，还支持`FTP` 、`SMTP`、`POP3`等几乎所有网络协议。由于其稳定性和丰富的功能，**`curl`** 被许多知名项目和企业广泛采用，是网络编程领域的“瑞士军刀”。

## 前言

在介绍如何使用 `curl` 发送邮件之前，我们先了解一下邮件的本质以及相关协议。

我们可以将一封电子邮件看作是具有特定格式的文本数据，其中包含发件人、收件人、主题、正文、附件等信息。
而邮件的发送和接收依赖于`SMTP`（简单邮件传输协议）和`POP3`/`IMAP`等协议。

> [!IMPORTANT]
> 在发送邮件的过程中，我们主要关注的是 `SMTP` 协议，因为它负责邮件的投递。而 `POP3` 和 `IMAP` 协议则用于接收和管理邮件。对于只需要发送邮件的场景（如自动通知、报警等），通常无需关心 `POP3` 或 `IMAP` 协议。只有在需要从服务器读取或管理邮件时，才会涉及到这两个协议。
>
> **我们本节内容只关注邮件的发送部分，所以只关注 `SMTP` 协议。**

邮件发送需要邮件服务器，邮件接收需要邮件客户端。

通常对于普通个人用户，不管是邮件服务器还是邮件客户端，都是由**邮箱服务商**提供的。例如`QQ`邮箱、`Gmail`等。

> [!NOTE]
> 例如我们使用 QQ 邮箱发送邮件，那么 QQ 邮箱就是我们的邮件服务商。它提供了邮件服务器和客户端。我们发送邮件时，实际上是通过 QQ 邮箱的服务器来发送的。而对方收到邮件时，也是通过邮箱的服务器（不一定是 QQ 邮箱的服务器）来接收的。

而一些企业邮箱，则是他们自己搭建的。

无论是使用个人邮箱服务商还是自建的企业邮箱服务器，最终邮件的发送都是通过 `SMTP` 协议完成的。后面我们将介绍如何使用 C++ 和 `curl` 库来实现这一过程。

## 发送邮件的基本流程

发送邮件通常包含以下几个步骤：

1. **连接邮件服务器**：通过 `SMTP` 协议连接到发送方的邮件服务器（如 `smtp.qq.com`）。
2. **身份验证**：大多数邮箱服务商要求使用账号密码或授权码进行身份验证。例如，`QQ` 邮箱 和 `Gmail` 都要求使用**授权码**代替原始密码，以增强安全性。
3. **构建并发送邮件**：准备邮件内容（包括发件人、收件人、主题、正文、附件等），通过 SMTP 协议发送出去。
4. **关闭连接**：发送完成后，关闭与邮件服务器的连接，释放资源。

通常我们会选择一个固定的 SMTP 服务器，例如 `smtp.qq.com`，并将发件人设置为我们的 `QQ` 邮箱账号。而**收件人则可以是任意邮箱地址**，如 `Gmail`、`163` 邮箱等，这不受限制。

这是因为邮件的发送和转发由 SMTP 协议负责，它会将邮件从发送方服务器中继转发到接收方所对应的邮件服务器。只要接收方邮箱地址有效，SMTP 服务就能尝试投递。因此，发送方和接收方可以使用不同的邮箱服务商，互不影响。

## 使用 curl 发送邮件

我们使用 QQ 邮箱作为示例，在编写代码之前，我们需要先做一些准备工作。

1. 开启 `SMTP` 服务与申请授权码。
   1. 打开[QQ邮箱网页端](https://mail.qq.com/)，登录邮箱账号。
   2. 点击右上角头像，选择“**设置**”。
   3. 左侧菜单栏选择“**账号与安全**”。
   4. 选择左侧“**安全设置**”。
   5. 在“**POP3/IMAP/SMTP/Exchange/CardDAV/CalDAV服务**”中，开启“SMTP服务”，并获取授权码。
2. 验证授权码有效，先使用 `curl` 命令行工具测试一下是否可以发送邮件。

端口设置见 [文档](https://service.mail.qq.com/detail/0/427)，通常为 `587`、`465`。

邮件文本 `测试邮件.txt` 内容：

```email
From: Test <Mq-b@qq.com>
To: Mq-b@qq.com
Subject: 测试邮件

这是一封使用curl发送的测试邮件。
```

| 邮件文本字段                             | 说明                                                           |
| ---------------------------------- | ------------------------------------------------------------ |
| `From: Test <Mq-b@qq.com>` | 发件人名字和邮箱地址，显示邮件是谁发的。`Test` 是显示名，`Mq-b@qq.com` 是发件邮箱。 |
| `To: Mq-b@qq.com`                  | 收件人邮箱地址，告诉邮件服务器邮件发给谁。                                        |
| `Subject: 测试邮件`                    | 邮件主题，收件人在邮箱客户端看到的标题。                                         |
| 空行                                 | 邮件头部和正文之间必须有一个空行，用来分隔。                                       |
| `这是一封使用curl发送的测试邮件。`               | 邮件正文内容，邮件的主要文本信息。                                            |

```bash
curl -v --url "smtp://smtp.qq.com:587" --ssl-reqd ^
--mail-from "Mq-b@qq.com" ^
--mail-rcpt "Mq-b@qq.com" ^
--upload-file .\测试邮件.txt ^
--user "Mq-b@qq.com:abcdefg" ^
--login-options "AUTH=LOGIN"
```

| 参数                                      | 说明                                                |
| --------------------------------------- | ------------------------------------------------- |
| `-v`                                    | 详细模式（verbose），打印调试信息，方便观察 SMTP 连接和交互过程            |
| `--url "smtp://smtp.qq.com:587"`        | 指定 SMTP 服务器地址和端口（587 是 SMTP 的 STARTTLS 端口）        |
| `--ssl-reqd`                            | 要求使用 TLS 加密（SMTP STARTTLS），确保传输安全                 |
| `--mail-from "Mq-b@qq.com"`             | 邮件发送者地址，即发件人邮箱                                    |
| `--mail-rcpt "Mq-b@qq.com"`             | 邮件接收者地址，即收件人邮箱（这里发给自己）                            |
| `--upload-file .\测试邮件.txt`              | 指定邮件内容文件，curl 会读取该文件内容作为邮件正文上传                    |
| `--user "Mq-b@qq.com:abcdefg"` | SMTP 登录用户名和授权码（密码）                                |
| `--login-options "AUTH=LOGIN"`          | 强制使用 SMTP `LOGIN` 认证方式，避免 curl 默认用 `PLAIN` 导致编码处理认证失败 |

>[!Tip]
>`curl` 命令行工具在不同平台表现略有差异，以上命令就是在 `windows cmd` 命令提示符中测试执行的。需要注意路径分隔符和编码处理。
>
>Linux 中执行测试：
>
>```bash
>curl --url "smtp://smtp.qq.com:587" --ssl-reqd \
>--mail-from "Mq-b@qq.com" --mail-rcpt "Mq-b@qq.com" \
>  --upload-file ./测试邮件.txt \
>  --user "Mq-b@qq.com:你的授权码" \
>  --login-options "AUTH=LOGIN"
>  ```

邮件文本 `测试邮件.txt` 内容：

```email
From: Test <Mq-b@qq.com>
To: Mq-b@qq.com
Subject: 测试邮件

这是一封使用curl发送的测试邮件。
```

| 邮件文本字段                             | 说明                                                           |
| ---------------------------------- | ------------------------------------------------------------ |
| `From: Test <Mq-b@qq.com>` | 发件人名字和邮箱地址，显示邮件是谁发的。`Test` 是显示名，`Mq-b@qq.com` 是发件邮箱。 |
| `To: Mq-b@qq.com`                  | 收件人邮箱地址，告诉邮件服务器邮件发给谁。                                        |
| `Subject: 测试邮件`                    | 邮件主题，收件人在邮箱客户端看到的标题。                                         |
| 空行                                 | 邮件头部和正文之间必须有一个空行，用来分隔。                                       |
| `这是一封使用curl发送的测试邮件。`               | 邮件正文内容，邮件的主要文本信息。                                            |

---

如果以上内容你都了解了，并且命令测试通过，那么前置也就完成了。下面我们开始编写 C++ 代码进行邮件发送。

## 基于 curl 库编写C++代码发送邮件

我们将使用 `curl` 库来实现邮件发送功能。首先确保你已经安装了 `curl` 库，并且在 C++ 项目中正确链接。

windows 中建议使用 `vcpkg` 安装 `curl` 库：

```bash
vcpkg install openssl:x64-windows # curl 依赖 openssl
vcpkg install curl:x64-windows
```

在 Linux 中，你可以使用包管理器安装 `libcurl`：

```bash
sudo apt-get install libcurl4-openssl-dev
```

cmake 引入 `curl` 库：

```cmake
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    OpenSSL::SSL 
    OpenSSL::Crypto 
    CURL::libcurl
)
```

或者可以单文件，命令行编译：

```bash
g++ ./邮件发送测试.cpp -o mail -lcurl -lssl -lcrypto
```

测试代码：

```cpp
#include <curl/curl.h>
#include <string>
#include <iostream>
#include <cstring>

struct UploadStatus {
    std::size_t bytes_read;
    std::string payload;
};

std::size_t payload_source(char* ptr, std::size_t size, std::size_t nmemb, void* userp) {
    UploadStatus* upload_ctx = reinterpret_cast<UploadStatus*>(userp);
    const std::size_t buffer_size = size * nmemb;
    if (upload_ctx->bytes_read >= upload_ctx->payload.size())
        return 0; // 邮件内容写完了
    std::size_t copy_size = upload_ctx->payload.size() - upload_ctx->bytes_read;
    if (copy_size > buffer_size)
        copy_size = buffer_size;
    memcpy(ptr, upload_ctx->payload.c_str() + upload_ctx->bytes_read, copy_size);
    upload_ctx->bytes_read += copy_size;
    return copy_size;
}

int main() {
    const char* smtp_url = "smtp://smtp.qq.com:587"; // SMTP服务器地址和端口
    const char* username = "Mq-b@qq.com";            // 登录用的邮箱账号
    const char* password = "abcdefg";                // 邮箱的授权码
    const char* from = "Mq-b@qq.com";                // 发件人邮箱
    const char* to = "Mq-b@qq.com";                  // 收件人邮箱

    // 邮件内容必须头部和正文之间有一行空行，并以回车换行结尾
    std::string payload =
        "To: Mq-b@qq.com\r\n"
        "From: Mq-b@qq.com\r\n"
        "Subject: Test Email from libcurl\r\n"
        "\r\n"
        "Hello,\r\n"
        "This is a test email sent using libcurl in C++.\r\n";

    CURL* curl = curl_easy_init();
    CURLcode res = CURLE_OK;
    curl_slist* recipients = nullptr;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, smtp_url);               // 设置SMTP服务器地址
        curl_easy_setopt(curl, CURLOPT_PORT, 587L);                  // 设置SMTP端口
        curl_easy_setopt(curl, CURLOPT_USERNAME, username);          // 设置SMTP用户名
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password);          // 设置SMTP密码（授权码）
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from);             // 设置发件人邮箱
        recipients = curl_slist_append(recipients, to);              // 构造收件人邮箱列表
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);       // 设置收件人邮箱
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);     // 使用 SSL/TLS 加密连接
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);                 // 开启调试信息输出
        curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN"); // 指定 LOGIN 认证方式
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);          // 跳过SSL证书校验
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);          // 跳过SSL主机名校验

        UploadStatus upload_ctx = { 0, payload };
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source); // 设置回调函数，用于提供邮件内容
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);        // 设置回调函数的上下文数据，传递upload_ctx
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);                   // 设置为上传模式（即发邮件）
        res = curl_easy_perform(curl);                                // 正式执行SMTP流程（连接、认证、发信）。libcurl会自动完成所有SMTP细节

        if (res != CURLE_OK)
            std::cerr << "curl_easy_perform() failed ❌: " << curl_easy_strerror(res) << std::endl;
        else
            std::cout << "Email sent successfully! ✅" << std::endl;

        curl_slist_free_all(recipients);                               // 释放收件人列表
        curl_easy_cleanup(curl);                                       // 清理CURL句柄
    }
}
```

> [!Tip]
> 测试了 `windows msvc` 和` Linux g++` 编译器，均可以正常编译代码并执行发送邮件功能。
> 如果你在编译时遇到问题，请确保 `curl` 库和相关依赖（如 `OpenSSL`）已正确安装并链接。

> [!WARNING]
> 注意：在实际使用中，请确保你的授权码是正确的，并且 SMTP 服务已开启。
> 如果你使用的是其他邮箱服务商，请根据其提供的 SMTP 服务器地址和端口进行相应修改。

如果你成功运行了以上代码，并且收到了邮件，那么恭喜你，你已经成功实现了基于 `curl` 库的邮件发送功能。

下面我们来分析一下代码，后面则会再对代码进行封装，提供一个更易用的邮件发送模块。

### 代码分析

代码的主要逻辑是使用 `curl` 库的 API 来设置 SMTP 连接、身份验证和邮件内容。以下是代码的关键部分：

1. **初始化 `curl`**：使用 `curl_easy_init()` 初始化一个 `CURL` 句柄。
2. **设置 SMTP 连接**：使用 `curl_easy_setopt()` 设置 SMTP 连接的选项，包括服务器地址、端口号、身份验证方式、用户名和密码等。
3. **设置邮件内容**：使用 `curl_easy_setopt()` 设置邮件内容的选项，包括发件人、收件人、主题和正文等。
4. **上传邮件内容**：通过 `curl_easy_setopt()` 设置一个回调函数 `payload_source`，该函数负责提供邮件内容。`payload_source` 函数会在需要时被调用，以读取邮件内容并返回给 `curl`。
5. **执行发送**：使用 `curl_easy_perform()` 执行邮件发送操作。 如果发送成功，返回值为 `CURLE_OK`，否则会输出错误信息。
6. **清理资源**：使用 `curl_slist_free_all()` 释放收件人列表，使用 `curl_easy_cleanup()` 清理 `CURL` 句柄。
7. **输出结果**：根据发送结果输出相应的提示信息。

> [!NOTE]
> `payload_source` 函数是一个回调函数，用于提供邮件内容。它会在 `curl` 需要读取邮件内容时被调用。我们使用 `UploadStatus` 结构体来跟踪已读取的字节数和邮件内容。
> 通过 `memcpy` 将邮件内容复制到 `ptr` 指向的缓冲区中，并更新已读取的字节数。
> `curl` 需要知道邮件内容的长度，因此我们返回邮件内容的长度。
> 如果邮件内容已全部读取，则返回 0，表示没有更多内容可供读取。
