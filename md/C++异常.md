# C++ 异常

## 前言

C++ 的异常复杂吗？复杂。但真的复杂吗？又不复杂。这取决于你到底是否在正确学习使用 C++ 异常。互联网上关于C++异常的争议非常之多，以至于扰乱了许多开发者对异常的认知；他们可能能说出很多C++异常性能问题、不符合[零开销原则](https://zh.cppreference.com/w/cpp/language/Zero-overhead_principle)，什么有名项目都不用异常......

许多人对异常避之不及，却认为自己了解异常处理，但实际上，他们连如何继承 C++ 标准异常类来自定义异常类型、基本的异常处理方式都不清楚，更不了解栈回溯，也不理解 [RAII](https://zh.cppreference.com/w/cpp/language/raii)。

尽管围绕 C++ 异常的争议颇多，但它依然被广泛应用于各类项目，尤其是在标准库、GUI 框架、服务器端开发、金融计算等领域，许多高质量的 C++ 代码都依赖异常来管理错误状态和资源清理。异常在绝大多数情况下都不会成为性能瓶颈，C++标准库也是将异常视作事实标准（提供禁止异常的选项（如 `-fno-exceptions`）是对语言特性的阉割，也并非真的没有异常），在 [《CppCoreGuidelines》](https://github.com/isocpp/CppCoreGuidelines) 中也是赞成使用异常进行错误处理。

我们本节不会侧重点在于异常本身，而在于：**使用C++异常**。

C++异常可以用一句话概括：**与其它编程语言并无多少不同，错误处理罢了**。

## 选择异常而非错误码

除非处于极端性能敏感场景，否则大多数情况下，错误处理应优先使用异常。

异常能够自动传播错误，并**进行跨函数的控制流跳转**，大大简化了错误处理逻辑，使代码更清晰、更易维护，而无需**在每个函数中反复检查返回值并手动传递错误状态**。

```cpp
// 底层函数：可能抛出异常
void loadConfigFile(const std::string& path) {
    if (path.empty()) {
        throw std::invalid_argument("配置文件路径不能为空");
    }

    // 模拟文件打开失败
    if (path != "valid.conf") {
        throw std::runtime_error("无法打开配置文件: " + path);
    }
}

void initializeSystem() {
    loadConfigFile("invalid.conf"); // 这里可能会抛出异常
    std::cout << "系统初始化成功" << std::endl;
}

void startApplication() {
    initializeSystem();
    std::cout << "应用程序启动" << std::endl;
}

int main() {
    try {
        startApplication();
    }
    catch (const std::exception& e) {
        std::cerr << "程序异常: " << e.what() << std::endl;
        return 1;
    }
}
```

```cpp
main()
└─ startApplication()
   └─ initializeSystem()
      └─ loadConfigFile("invalid.conf") 
         → 抛出异常 → 直达 main() 的 catch 块
```

实际开发中基本远远不止三层，甚至是更多。得益于异常我们可以直接在最上层的调用中处理，如果是错误码呢？

```cpp
enum class ErrorCode {
    Success,
    InvalidArgument,
    FileNotFound,
    SystemInitFailed
};

// 底层函数：返回错误码
ErrorCode loadConfigFile(const std::string& path) {
    if (path.empty()) {
        return ErrorCode::InvalidArgument;
    }

    // 模拟文件打开失败
    if (path != "valid.conf") {
        return ErrorCode::FileNotFound;
    }

    return ErrorCode::Success;
}

// 中层函数：必须检查并传递错误码
ErrorCode initializeSystem() {
    // 需要手动检查错误码
    if (auto err = loadConfigFile("invalid.conf");
        err != ErrorCode::Success) {
        return err; // 手动传递错误码
    }

    std::cout << "系统初始化成功" << std::endl;
    return ErrorCode::Success;
}

// 顶层函数：必须检查并传递错误码
ErrorCode startApplication() {
    // 需要再次检查错误码
    if (auto err = initializeSystem();
        err != ErrorCode::Success) {
        return ErrorCode::SystemInitFailed; // 转换错误类型
    }

    std::cout << "应用程序启动" << std::endl;
    return ErrorCode::Success;
}

int main() {
    // 最终错误检查
    if (auto err = startApplication();
        err != ErrorCode::Success) {

        // 需要手动解析错误类型
        switch (err) {
        case ErrorCode::InvalidArgument:
            std::cerr << "错误：配置文件路径不能为空" << std::endl;
            break;
        case ErrorCode::FileNotFound:
            std::cerr << "错误：无法打开配置文件" << std::endl;
            break;
        case ErrorCode::SystemInitFailed:
            std::cerr << "错误：系统初始化失败" << std::endl;
            break;
        }
        return 1;
    }
}
```

```cpp
main()
└─ startApplication()
   └─ initializeSystem()
      └─ loadConfigFile("invalid.conf") 
         → 返回错误码 → 逐层上传 → main() 处理
```

错误码无法自动穿透调用栈，你必须编写代码每一层都检查并保存错误码，一直繁琐的往下传递，甚至还需要在传递的过程中进行转换。

并且**错误码无法携带更多的完整信息**，使用异常，抛出异常对象，可以有非常之多的信息。举个例子：

```cpp
bool AuthManager::isDeviceRegistered(const std::string& device_id)try {
    auto res = dbManager.SqlQuery(std::format(checkDeviceRegisteredQuery, device_id));
    if(res->rowsCount() == 1) {
        return true;
    }
    return false;
}catch(sql::SQLException& e){
    spdlog::error("AuthManager::isDeviceRegistered Error: {}", e.what());
    spdlog::error("AuthManager::isDeviceRegistered Error code: {}", e.getErrorCode());
    spdlog::error("AuthManager::isDeviceRegistered SQLState: {}", e.getSQLState());
    return false;
}
```

它可以携带清晰的日志，包括错误码，包括状态码。稍后在自定义异常类中我们会提到如何自己定义这些。

使用异常要远比错误码方便，这是一个无需讨论的共识。实际情况，异常错误处理可能还需要做更多的事情：

```cpp
#include <iostream>
#include <stdexcept>
// 底层函数：可能抛出异常
void loadConfigFile(const std::string& path) {
    if (path.empty()) {
        throw std::invalid_argument("配置文件路径不能为空");
    }

    // 模拟文件打开失败
    if (path != "valid.conf") {
        throw std::runtime_error("无法打开配置文件: " + path);
    }
}

void initializeSystem() {
    try{
        loadConfigFile("invalid.conf"); // 这里可能会抛出异常
    }catch (const std::exception& e){   // 捕获并进行重抛
        throw std::system_error(std::make_error_code(std::errc::invalid_argument), e.what());
    }
    
    std::cout << "系统初始化成功" << std::endl;
}

void startApplication() {
    initializeSystem();
    std::cout << "应用程序启动" << std::endl;
}

int main() {
    try {
        startApplication();
    }
    catch(const std::system_error& e){
        std::cerr << "system 异常：" << e.what() << " 错误码：" << e.code() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "程序异常: " << e.what() << std::endl;
        return 1;
    }
}
```

**调用栈中的函数极有可能捕获了异常之后进行一些处理再次进行重抛**，如果你注意阅读文档，你会发现许多C++标准库能够规定某些接口只抛出特定类型的异常，那无非是先接取异常处理错误后，再重新抛出，这很常见。

运行结果：

```txt
system 异常：无法打开配置文件: invalid.conf: invalid argument 错误码：generic:22
```

## 自定义异常类

C++抛出异常允许是任何类型，可以是 int、double、char 等等，这是 C++ 设计的失误，在实际开发中我们不应该这样做。

```cpp
throw 1;
throw 3.14;
throw "a.txt 文件打开失败"
```

具体来说：

```cpp
void f(){
    throw 1;
}

int main()
{
    try{
        f();
    }
    catch (std::exception& e){
        std::cerr << "std::exception& " << e.what() << '\n';
    }
    catch(int a){ // 会进入这个 catch 块
        std::cerr << "int " << a << '\n';
    }
}
```

这并没有什么意义，C++ 异常可以让控制流跳转，从某种意义来说可以通过这种做法跨函数直接传递错误码（抽象）。

在实际开发中我们都应该使用**标准的异常对象**。如果想要用户定义异常类型，通常需要继承标准异常类，如 [`std::exception`](https://zh.cppreference.com/w/cpp/error/exception)，它是一个多态类型，如果只是简单需求，可以单纯的重写 [`what`](https://zh.cppreference.com/w/cpp/error/exception/what) 虚函数。

```cpp
struct FileOpenException : std::exception {
    const char* what() const noexcept override{
        return "Failed to open file.";
    }
};
```

因为 `FileOpenException` 继承了 `std::exception` ，所以 FileOpenException 类型的对象可以初始化 `std::exception`，也就是：

```cpp
const std::exception& e = FileOpenException{};
std::cout << e.what() << '\n';
```

这是C++的基本规则，父类的指针或引用可以指向子类对象，在 `catch` 块中的形参同样应用此规则，所以在实际情况下，我们接取异常只需要写做：

```cpp
void f(){
    throw FileOpenException{};
}

int main(){
    try{
        f();
    }
    catch (std::exception& e){
        std::cout << e.what() << '\n';
    }
}
```

异常类型基本要求就是必须为 `std::exception` 子类，但是却并不要求必须继承它，也可以继承它的子类，如 `std::runtime_error`：

```cpp
class FileOpenException : public std::runtime_error {
public:
    // 调用父类构造函数
    explicit FileOpenException(const std::string& path): std::runtime_error("Failed to open file: " + path) {}

};

int main() {
    try {
        throw FileOpenException{ "test.txt" };
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
```

重写 `what` 虚函数也不是必须的，唯一几乎一定的是 catch 块中需要是引用，如果是普通类型，则有**对象切片**的问题，在当前例子不会有问题，但是如果多层就会很明显：

```cpp
// 基类
struct BaseException : std::exception {
    const char* what() const noexcept override {
        return "Base Exception";
    }
};

// 派生类
struct DerivedException : BaseException {
    const char* what() const noexcept override {
        return "Derived Exception";
    }
};

int main(){
    try {
        throw DerivedException{}; // 正确抛出派生类对象
    }
    catch (BaseException e) {     // 按值捕获 → 触发拷贝构造 → 切片
        std::cout << e.what();    // 输出 "Base Exception"（丢失派生类信息）
    }
}
```



## RAII

## `noexcept`

## 异常安全
