

# C++26 契约 `Contracts`

C++26 引入了契约（Contracts）机制，允许程序员指定程序在执行中的特定时间点的状态应有的属性。

截止 2026年6月，仅由 GCC16 实现了契约，目前依旧处于实验阶段，默认不启用。

通过设置编译选项：`-std=c++26` 与  `-fcontracts` 来启用契约功能。

## 函数契约说明符

### 前条件断言

```cpp
int divide(int a, int b)
    pre(b != 0)
{
    return a / b;
}

int main(){
    std::println("{}", divide(10, 2));
    // 契约失败
    std::println("{}", divide(10, 0));
    std::println("-----------------");
}
```

如你所见，这是一个很简单的示例，函数声明可以包含契约，我们称之为***函数契约说明符***。

`pre(谓词)` 引入了一个*前条件断言*，前条件断言与进入函数关联。要求函数调用必须满足 `b!=0`，而程序违背了契约。

当契约被违背时，具体行为由编译器配置的**求值语义（evaluation semantics）**决定。

当契约被违背时，会发生一次 *contract violation*（契约违背）。

契约违背后的处理方式由编译器的求值语义决定，例如记录日志、调用违背处理函数或直接终止程序。契约本身只负责描述规则，而不规定违背后的具体处理策略。

GCC 16 使用 `-fcontract-evaluation-semantic` 指定契约求值语义。

```bash
-fcontract-evaluation-semantic=ignore
-fcontract-evaluation-semantic=observe
-fcontract-evaluation-semantic=enforce
-fcontract-evaluation-semantic=quick_enforce
```

| 求值语义        | 检查契约 | 调用契约违背处理函数 | 终止程序 |
| --------------- | -------- | -------------------- | -------- |
| `ignored        | ❌        | ❌                    | ❌        |
| `observe`       | ✅        | ✅                    | ❌        |
| `enforce`       | ✅        | ✅                    | ✅        |
| `quick_enforce` | ✅        | ❌                    | ✅        |

默认情况下使用 `enforce` ，也就是检查契约，如果不满足则调用违背契约处理函数 `::handle_contract_violation` 再终止程序。

但是完全有可能 `::handle_contract_violation` 本身就会终止程序，所以即使使用别的求值语义依然无法避免程序被终止，它是实现定义的。而能否替换它也是由实现定义的。

总而言之以上示例默认情况下会输出第一个结果，然后第二个调用因为违背契约，打印日志后终止程序，见 [Complier Explorer](https://godbolt.org/z/GGYfzscWv)。

```txt
Program returned: 139
Program stdout
5
Program stderr
contract violation in function int divide(int, int) at /app/example.cpp:4: b != 0
[assertion_kind: pre, semantic: enforce, mode: predicate_false, terminating: yes]
terminate called without an active exception
Program terminated with signal: SIGSEGV
```

---

### 后条件断言

后条件断言与正常退出函数关联。使用 `post` 关键字。

如果后条件断言有*标识符*，那么函数契约说明符会引入*标识符* 作为关联函数的*结果绑定*。结果绑定表示对该函数的调用返回的对象或引用。结果绑定的类型是关联函数的返回类型。

```cpp
// 带结果绑定的后条件断言
int safe_add(const int a, int b)
    pre(b != 0)
    post(r: r > a)  // r 是结果绑定，代表返回值
{
    return a + b;
}
```

> [Complier Explorer](https://godbolt.org/z/x5o4W7hKc)

`post(r: r > a)` 中的 `r` 就是*结果绑定*，它在后条件断言的谓词中代表函数的返回值。标识符可以自定义命名，但常用 `r` 或 `result`。

```cpp
// 不带标识符的后条件断言（无法引用返回值）
void log_message(const std::string_view msg)
    post(msg.size() > 0)
{
    std::println("{}", msg);
}
```

> [Complier Explorer](https://godbolt.org/z/a4j3jEbca)

**注意**：后条件断言仅在函数**正常返回**时检查，如果函数通过异常退出，则不会检查后条件。

```cpp
int f()
    post(r: r > 0)
{
    throw std::runtime_error("error");
}
```

> [Complier Explorer](https://godbolt.org/z/rrvz1M7sa)

>[!TIP]
>
>需要注意的是后条件断言的谓词中如果要引用函数参数，该参数必须是 `const` 限定的（按值传递加 `const`，或 `const` 引用）。这是因为后条件在函数体内可能被修改的参数上求值是没有意义的——编译器需要确保后条件检查时参数的值与进入函数时一致。
>
>我们以上的示例都遵守了，否则无法通过编译。

## `contract_assert` 关键字

它可以简单视作 `assert` 宏的一个替代，用于函数体内部的断言。

```cpp
int process(std::vector<int>& data)
    pre(!data.empty())
{
    // 函数体内部的断言
    contract_assert(data.size() > 0);

    int sum = 0;
    for (auto v : data) {
        contract_assert(v >= 0);  // 每个元素非负
        sum += v;
    }

    contract_assert(sum >= 0);
    return sum;
}
```

> [Complier Explorer](https://godbolt.org/z/f6WhqMfvb)

与 `assert` 宏的区别：

| 特性 | `assert` | `contract_assert` |
| --- | --- | --- |
| 标准 | C89 宏 | C++26 关键字 |
| 求值语义控制 | 仅 `NDEBUG` | 可通过编译选项精细控制 |
| 与契约系统集成 | ❌ | ✅ 统一的违背处理机制 |

---

## 多契约与组合

一个函数可以同时拥有多个 `pre` 和 `post`，它们按书写顺序依次检查：

```cpp
double divide(const double a, const double b)
    pre(a >= 0)
    pre(b != 0)
    post(r: r * b == a)  // 可以用结果绑定做数学验证
{
    return a / b;
}
```

> [Complier Explorer](https://godbolt.org/z/ecrexT77x)

契约也可以与 `noexcept` 等说明符共存：

```cpp
int square(int x) noexcept
    post(r: r >= 0)
{
    return x * x;
}
```

> [Complier Explorer](https://godbolt.org/z/33Eszavdo)

---

## 现代 C++ 一个普通模板

由于 C++26 又更新的契约，这导致一个普通的函数模板的修饰多的可能令人难绷😅。

以下是一个非常合理但用到了非常多修饰的函数模板，感受一下现代 C++ 的"普通"：

```cpp
#include <concepts>
#include <type_traits>

struct Math {
    template<typename T>
    requires std::floating_point<T>
    [[nodiscard]] static constexpr auto
    normalize(T value) noexcept
        pre(value >= 0) 
    	pre(value <= 1)
        post(r: r >= 0 && r <= 1)
    {
        contract_assert(value >= 0);
        return value > 0.5 ? value : 0;
    }
};

int main() {
    auto result = Math::normalize(0.8);   // 正常使用
    Math::normalize(0.8);                 // 警告：忽略了 [[nodiscard]] 返回值
}
```

> [Compiler Explorer](https://godbolt.org/z/PKEEeo6EW)

`[[nodiscard]]` 的效果：忽略返回值时编译器会发出警告（`-Wall` 下）。

```txt
warning: ignoring return value of function declared with 'nodiscard' attribute
```

逐行拆解：

| 修饰 | 来源 | 作用 |
| --- | --- | --- |
| `template<typename T>` | C++98 | 模板参数 |
| `requires std::floating_point<T>` | C++20 | 约束模板参数为浮点类型 |
| `[[nodiscard]]` | C++17 | 禁止忽略返回值 |
| `static` | C++98 | 静态成员函数 |
| `constexpr` | C++11 | 编译期可求值 |
| `auto` | C++14 | 返回类型推导 |
| `normalize(T value)` | — | 函数名和参数 |
| `noexcept` | C++11 | 不抛异常 |
| `pre(...)` | C++26 | 前条件断言 × 2 |
| `post(r: ...)` | C++26 | 后条件断言 |
| `contract_assert(...)` | C++26 | 函数体内断言 |

一个函数声明，横跨 **7 个标准版本**，叠加 **10+ 种修饰**。这就是 2026 年的 C++。

然后你就发现，GCC 直接就 **ICE（内部编译器错误）**。

> Please submit a full bug report, with preprocessed source (by using -freport-bug). Please include the complete backtrace with any bug report. See <https://gcc.gnu.org/bugs/> for instructions.

---

## 编译与运行

```bash
# 启用契约（GCC 16+）
g++ -std=c++26 -fcontracts main.cpp -o main

# 指定求值语义
g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe main.cpp
```

在 [Compiler Explorer](https://godbolt.org) 上可以在线体验。

## 参考

- [契约断言-cppreference](https://zh.cppreference.com/cpp/language/contracts)
- [函数契约说明符-cppreference](https://zh.cppreference.com/cpp/language/function#.E5.87.BD.E6.95.B0.E5.A5.91.E7.BA.A6.E8.AF.B4.E6.98.8E.E7.AC.A6)
