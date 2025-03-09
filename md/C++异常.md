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

虽然异常类型的基本要求是必须成为 `std::exception` 的子类，但这并不意味着必须直接继承它。通过继承 `std::exception` 的派生类（例如 `std::runtime_error`）同样满足要求。

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

重写 `what` 虚函数也不是必须的，唯一几乎一定的是 **catch 块中形参类型需要是引用**，如果是普通类型，则有**对象切片**的问题，在当前例子不会有问题，但是如果多层就会很明显：

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

最后，我们可以以 [**Connector/C++**](https://dev.mysql.com/doc/connector-cpp/1.1/en/connector-cpp-introduction.html) 库中的异常类 `SQLException` 的源码作为一些参考，结束本小节：

```cpp
class CPPCONN_PUBLIC_EXCEPTION SQLException : public std::runtime_error
{
#ifdef _WIN32
#pragma warning(pop)
#endif
protected:
  const std::string sql_state;
  const int errNo;

public:
  SQLException(const SQLException& e) : std::runtime_error(e.what()), sql_state(e.sql_state), errNo(e.errNo) {}

  SQLException(const std::string& reason, const std::string& SQLState, int vendorCode) :
    std::runtime_error	(reason		),
    sql_state			(SQLState	),
    errNo				(vendorCode)
  {}

  SQLException(const std::string& reason, const std::string& SQLState) : std::runtime_error(reason), sql_state(SQLState), errNo(0) {}

  SQLException(const std::string& reason) : std::runtime_error(reason), sql_state("HY000"), errNo(0) {}

  SQLException() : std::runtime_error(""), sql_state("HY000"), errNo(0) {}

  const std::string & getSQLState() const
  {
    return sql_state;
  }

  const char * getSQLStateCStr() const
  {
    return sql_state.c_str();
  }


  int getErrorCode() const
  {
    return errNo;
  }

  virtual ~SQLException() throw () {};

protected:
  MEMORY_ALLOC_OPERATORS(SQLException)
};
```

你会注意到它有**虚析构**，这也算一个需要注意的点，如果你的异常类型较为复杂，有自己的资源需要释放，那就需要定义。

## RAII

“[**资源获取即初始化**](https://zh.cppreference.com/w/cpp/language/raii)”(RAII，Resource Acquisition Is Initialization)。

许多人对于 RAII 的理解只是：***构造函数申请资源，析构函数释放资源，让对象的生命周期和资源绑定***。

这是极其不准确的，RAII中的一个核心就是**异常**。所以还应该有：**当异常抛出时，C++ 会自动调用对象的析构函数（也就是栈回溯），保障资源不泄露。**

- **RAII = 资源生命周期绑定对象 + 异常安全保证** 

```cpp
class RAII {
public:
    explicit RAII(size_t size) : data_(new int[size]) {
        std::cout << "构造" << std::endl;
    }

    ~RAII() {
        delete[] data_;
        std::cout << "析构" << std::endl;
    }

    int* get() const { return data_; }

private:
    int* data_;
};
```

以这样一个简单的 RAII 类为例，构造函数中申请资源，析构函数释放，那么它在遇到异常时会做什么呢？

```cpp
void f(){
    RAII raii{ 10 };
    //todo.. 假设有其它代码抛出异常
    throw std::runtime_error("error");
}

int main(){
    // 上层进行异常的捕获和处理
    try{
        f();
    }
    catch(std::exception& e){
        std::cerr << e.what() << '\n';
    }
    catch (...){}    
}
```

> 运行[测试](https://godbolt.org/z/nEz7h88vc)。输出：
>
> 构造
> 析构
> error

正常来说，我们知道，当函数 `f` 执行完后，局部对象要逆序销毁，有析构函数就调用析构函数。而**即使执行函数 `f` 的时候抛出了异常，这个销毁依然会发生（前提是你有捕获这个异常）。**

>如果异常被抛出但未被捕获那么就会调用 [std::terminate](https://zh.cppreference.com/w/cpp/error/terminate)。是否对未捕获的异常进行任何栈回溯由**实现定义**。（简单的说就是不一定会调用析构）
>
>我们的建议是一定要进行处理。

是不是现在觉得”*栈回溯*“这个看起来高深的词也没啥难理解的了？

- **“栈回溯”就是在执行函数过程中遇到异常，函数按构造的逆序自动清理局部对象并“返回”的过程。**

我们的许多 C++ 标准库中的类型都封装的很好，不再需要我们用户定义的析构函数进行什么释放，会自动调用成员的析构函数进行释放。

```cpp
class Resource {
public:
    Resource(int id) : id_(id) {
        std::cout << "Resource " << id_ << " created\n";
    }
    ~Resource() {
        std::cout << "Resource " << id_ << " destroyed\n";
    }
private:
    int id_;
};

void exception_safe_demo() {
    std::vector<Resource> resources;
    resources.emplace_back(1);
    resources.emplace_back(2);

    throw std::runtime_error("Oops!");  // 抛出异常

    resources.emplace_back(3);  // 不会执行
} // 栈回溯触发 vector 析构 → Resource 2 和 1 的析构函数被调用
```

> 运行[结果](https://godbolt.org/z/exvKxT4Pe)：
>
> Resource 1 created
> Resource 2 created
> Resource 1 destroyed
> Resource 1 destroyed
> Resource 2 destroyed
> Oops!

## `noexcept`

`noexcept` 是 C++11 引入的关键字，在早期 C++98 还存在[动态异常说明](https://zh.cppreference.com/w/cpp/language/except_spec) `throw(类型标识表达式)` ，我们一起介绍。

```cpp
void f() noexcept;
void f() noexcept(true);
void f() throw();
```

这三种声明方式均表示：**函数 `f` 保证不抛出任何异常**，使编译器能够进行更深度的优化。如 省略生成异常处理的二进制，减小文件体积、移动语义的优化（标准库容器如 `std::vector` 优先使用 `noexcept` 移动构造而非复制构造）。也可以让代码**语义清晰化**：明确告知调用者“此函数不会抛出异常，无需额外处理”。

若被声明为 `noexcept` 的函数内部实际抛出了异常，程序将**直接调用 [`std::terminate()`](https://zh.cppreference.com/w/cpp/error/terminate) 终止运行**，不会进行任何异常传递或栈回溯。
这意味着：`noexcept` 也可用于表明“*此函数的异常无法被合理处理，一旦发生错误应视为致命故障*”。

`noexcept` 关键字存在两种不同用法：**说明符**、**运算符**。

1. **无条件承诺**：`noexcept` 或 `noexcept(true)`

    ```cpp
    void safe() noexcept;  // 保证不抛异常
    ```

2. **条件性承诺：**`noexcept(表达式)`

    ```cpp
    template<typename T>
    void swap(T& a, T& b) noexcept(noexcept(a.swap(b)));  // 仅当 a.swap(b) 不抛异常时，本函数才 noexcept
    ```

    - **外层 `noexcept`**：说明符，声明函数的异常规范。
    - **内层 `noexcept`**：运算符，检测表达式是否会抛异常。

    此处连用了 `noexcept` 说明符与运算符，**noexcept 运算符获取表达式 `a.swap(b)` 是否为 `noexcept` 的**，如果是，得到`true`；带入即为 `noexcept(true)`，反之则 `false`。需要强调的是 noexcept 的表达式必须是编译期的。

3. 从标准库源码中看应用

    `MSVC STL` 中 `std::vector` 的构造函数：

    ```cpp
    _CONSTEXPR20 vector() noexcept(is_nothrow_default_constructible_v<_Alty>) : _Mypair(_Zero_then_variadic_args_t{}) {
        _Mypair._Myval2._Alloc_proxy(_GET_PROXY_ALLOCATOR(_Alty, _Getal()));
    }
    ```

    - **`is_nothrow_default_constructible_v<_Alty>`**：
      编译期检测分配器类型 `_Alty` 的默认构造函数是否 `noexcept`。
    - **结果传递**：
      若为 `true`，则 `vector` 的构造函数标记为 `noexcept`，否则可能抛异常。

    此处也是典型的 `noexcept` 的说明符应用，且也是根据条件设置。

---

noexcept 经过我们的介绍好像有些复杂了？的确，因为标准库的做法都极其规范，对于异常安全的要求极高，有的时候 `noexcept` 的表达式会嵌套再嵌套，甚至使用 `||` 、`&&` 等运算符添加更多条件来设置。不过说来说去，其实 `noexcept` 关键字的作用也就那两个，很简单，用一个小例子来结束我们 noexcept 的介绍。

```cpp
#include <iostream>

void f(); // 默认 noexcept(false)
void f2() noexcept(noexcept(f())); // 等价于 noexcept(false)

template<typename T>
void f3() noexcept(std::is_same_v<T,int>);

int main(){
    constexpr bool b = noexcept(f());
    constexpr bool b2 = noexcept(f2());
    constexpr bool b3 = noexcept(f3<int>());
    static_assert(b == false);
    static_assert(b2 == false);
    static_assert(b3 == true);
}
```

> 运行[测试](https://godbolt.org/z/6a9Mn6Y83)。

另外，关于过时的[动态异常说明](https://zh.cppreference.com/w/cpp/language/except_spec)，并不想过多介绍，我们稍微提一下，让大家能看懂以前的代码就好：

```cpp
struct X{};
struct Y{};
struct Z: X {};
void f() throw(X, Y) // 动态异常说明，要求函数 f 只能抛出 X 或 Y 类型，以及他们的派生类
{
    bool n = false;

    if (n)
        throw X(); // OK，可能调用 std::terminate()
    if (n)
        throw Z(); // 同样 OK

    throw 1; // 将调用 std::unexpected()
}
```

> 运行[测试](https://godbolt.org/z/6a9Mn6Y83)。

## 异常安全

异常安全其实算一个较为高阶的话题，在许多八股或面试中也都常常提及。主要的还是要介绍**四个异常保证等级**。

以下是四个被广泛认可的异常保证等级[[4\]](https://zh.cppreference.com/w/cpp/language/exceptions#cite_note-4)[[5\]](https://zh.cppreference.com/w/cpp/language/exceptions#cite_note-5)[[6\]](https://zh.cppreference.com/w/cpp/language/exceptions#cite_note-6)，每个是另一个的严格超集：

1. *不抛出（或不失败）异常保证*——**函数始终不会抛出异常**。[析构函数](https://zh.cppreference.com/w/cpp/language/destructor)和其他可能在栈回溯中调用的函数被期待为不会抛出（以其他方式报告或隐瞒错误）。[析构函数](https://zh.cppreference.com/w/cpp/language/destructor)默认为 [noexcept](https://zh.cppreference.com/w/cpp/language/noexcept)。(C++11 起)交换函数，[移动构造函数](https://zh.cppreference.com/w/cpp/language/move_constructor)，及为提供强异常保证所使用的其他函数，都被期待为不会失败（函数总是成功）。
2. *强异常保证*——如果函数抛出异常，那么**程序的状态会恰好被回滚到该函数调用前的状态**。（例如 [std::vector::push_back](https://zh.cppreference.com/w/cpp/container/vector/push_back)）。
3. *基本异常保证*——如果函数抛出异常，那么程序处于某个有效状态。**不泄漏任何资源**，且所有**对象的不变式[^1]都保持完好**。
4. *无异常保证*——如果函数抛出异常，那么程序可能不会处于有效的状态：可能已经发生了资源泄漏、内存损坏，或其他摧毁不变式的错误。

第一个和第四个异常保证等级很好理解，基本无需介绍。需要强调的是：

- **异常 ≠ 灾难**：多数异常仅触发参数合法性检查（如无效输入），未实际修改系统状态。
- **栈回溯 ≠ 强保证**：RAII 自动释放资源是基础机制，强保证需额外设计状态回滚逻辑。

**绝大部分的三方库和标准库设计都可以满足强异常保证（最次基本异常保证），许多异常抛出基本都是在表达参数不对，输入不符合要求等，并非是什么严重的错误。**

我们在日常开发设计接口中，也完全可以检查函数参数，不满足直接往外抛出异常即可。

例如在 HTTP 服务端中，函数接受处理客户端发送的数据，根据约定的格式将 json 数据反序列化为对象方便操作

```cpp
void handleJsonPost(const httplib::Request &req, httplib::Response &res) {
    DeviceStatus device_status;
    try {
        auto jsonBody = json::parse(req.body);
        device_status = jsonBody.get<DeviceStatus>(); // 反序列化为对象进行操作
    }catch(std::exception& e){
        res.set_content("Error", "text/plain");
        spdlog::error("handleJsonPost Error: {}", e.what());
        return;
    }
    //todo..
}
```

如果反序列化抛出异常，那只能代表客户端发送的 json 格式不对，正常的记录错误并回复，然后 return 即可，这种异常并没有什么影响。

在平时的编程中，我们尽量让代码至少要达到**基本异常保证**，并且要熟练处理异常。

## 总结

C++ 标准库中还有许多的异常设施可以介绍，如异常指针，不过这样也就够了。剩下的别的内容，就各位自己自行探索吧。



[^1]: 不变式（Invariant）是一个在程序执行过程中永远保持成立的条件。不变式在检测程序是否正确方面非常有用。例如编译器优化就用到了不变式。举个例子：“*类的不变式*”，类的不变式是用于约束类的实例的不变式。成员函数必须使这个不变式保持成立。 不变式约束了类的实例的可能取值。
