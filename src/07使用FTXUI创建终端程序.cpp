#include <memory>
#include <string>
#include <format>
#include "ftxui/component/captured_mouse.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
using namespace ftxui;

// 创建按钮样式选项：文字居中、带边框、自适应宽度
ButtonOption Style() {
    auto option = ButtonOption::Animated();
    option.transform = [](const EntryState& s) {
        auto element = text(s.label);
        if (s.focused) {
            element |= bold;  // 聚焦状态时加粗文字
        }
        return element | center | borderEmpty | flex; // 组合样式：居中+边框+弹性布局
        };
    return option;
}

int main() {
    int value = 50;

    // 创建四个操作按钮（-1/+1/-10/+10）
    auto btn_dec_01 = Button("-1", [&] { value -= 1; }, Style());
    auto btn_inc_01 = Button("+1", [&] { value += 1; }, Style());
    auto btn_dec_10 = Button("-10", [&] { value -= 10; }, Style());
    auto btn_inc_10 = Button("+10", [&] { value += 10; }, Style());

    // 构建组件布局结构
    int row = 0;  // 跟踪当前聚焦的行
    auto buttons = Container::Vertical({
        // 第一行水平排列 -1/+1 按钮
        Container::Horizontal({btn_dec_01, btn_inc_01}, &row) | flex,
        // 第二行水平排列 -10/+10 按钮
        Container::Horizontal({btn_dec_10, btn_inc_10}, &row) | flex,
    });

    // 组合界面渲染元素
    auto component = Renderer(buttons, [&] {
        return vbox({
                   text(std::format("mq白伟大次数：{} 😋 ", value)),   // 第一行：显示当前值
                   separator(),                                // 分隔线
                   buttons->Render() | flex,                   // 渲染显示按钮
            }) | flex | border;  // 整体布局：弹性填充+边框
    });

    // 启动交互式界面
    auto screen = ScreenInteractive::FitComponent();
    screen.Loop(component);
}