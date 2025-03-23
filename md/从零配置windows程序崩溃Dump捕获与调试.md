# 从零配置windows程序崩溃Dump捕获与调试

在实际开发中，我们经常可能遇到的问题是自己编写并发布的程序崩溃了，我们如何才能知道程序崩溃了，并且知道崩溃的位置，以及崩溃的堆栈信息呢？

你可能说：“**查看日志文件**”，的确，它是很有作用的，但是通常，我们还会倾向于**调试程序崩溃所产生的 `Dump` 文件**，它有着非常清晰的信息，包括程序崩溃的堆栈（乃至并行堆栈）信息等等。

不管在 windows 还是 Linux 上，都存在这种文件，它是二进制文件，我们需要使用 `Visual Studio` 或者 `GDB` 来解析它。

我们本节来介绍在 windows 上使用  `Visual Studio` 来配置生成以及调试 `Dump` 文件。

## 基本配置

生成 `Dump` 文件的方法在 windows 至少有三种，不过最常用的还是在程序中设置。

```cpp
void CreateDumpFile(EXCEPTION_POINTERS* exceptionInfo) {
    HANDLE hFile = CreateFileW(L"dumpfile.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = exceptionInfo;
        mdei.ClientPointers = FALSE;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithFullMemory, &mdei, NULL, NULL);
        CloseHandle(hFile);
    }
}

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* exceptionInfo) {
    CreateDumpFile(exceptionInfo);
    return EXCEPTION_EXECUTE_HANDLER; // 继续执行默认的崩溃处理
}

int main(){
    SetUnhandledExceptionFilter(ExceptionHandler);
    int* p = nullptr;
    *p = 0;
}
```

你可以在许多的博客看到这段代码，它有用吗？的确，但是事实上只是用户代码上的还远远不够。

首先我们需要链接 `dbghelp` 、`wininet` 这两个库。

并行，我们还需要添加选项用以生成符号文件：

```cmake
add_compile_options(/Zi)
add_link_options(/DEBUG)
```

这样，即使你是 Release 构建出来的程序，也会默认生成一个 `.pdb` 的符号文件，调试 Dump 文件时就需要这个文件进行辅助。

值得注意的是，这代表你需要自行保留程序每一个发行版对应的 `.pdb` 文件，用以调试匹配程序崩溃的堆栈信息。

完整测试代码，以及 `CMakeLists.txt` 文件如下：

```cpp
#include <windows.h>
#include <dbghelp.h>
#include <string>
#include <chrono>
using namespace std::chrono_literals;

inline std::string get_current_date() {
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    // 格式化时间
    std::tm* tm_ptr = std::localtime(&now_time);
    std::stringstream ss;
    ss << std::put_time(tm_ptr, "%Y-%m-%d");
    return ss.str();
}

void CreateDumpFile(EXCEPTION_POINTERS* exceptionInfo) {
    HANDLE hFile = CreateFile((get_current_date() + ".dmp").c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = exceptionInfo;
        mdei.ClientPointers = FALSE;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithFullMemory, &mdei, NULL, NULL);
        CloseHandle(hFile);
    }
}

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* exceptionInfo) {
    CreateDumpFile(exceptionInfo);
    return EXCEPTION_EXECUTE_HANDLER; // 继续执行默认的崩溃处理
}

int main(){
    SetUnhandledExceptionFilter(ExceptionHandler);
    int* p = nullptr;
    *p = 0;
}
```

```cmake
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/bin)

project ("test_dump")

add_compile_options(/Zi)
add_link_options(/DEBUG)

add_executable (${PROJECT_NAME} "main.cpp")

target_link_libraries(${PROJECT_NAME} PRIVATE
    dbghelp
    wininet
)
```
