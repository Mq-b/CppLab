#include <iostream>
#include <string>
#include <random>
#include <vector>
#include <stdexcept>
#include "ftxui/component/captured_mouse.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include <fstream>
using namespace ftxui;

// 扩展字符集，包括数字、字母和常见符号
const std::string EXTENDED_CHARSET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.-";

int char_to_index(char c) {
    auto pos = EXTENDED_CHARSET.find(c);
    if (pos == std::string::npos) {
        throw std::invalid_argument("Invalid character in input string");
    }
    return static_cast<int>(pos);
}

char index_to_char(int x) {
    return EXTENDED_CHARSET[x % EXTENDED_CHARSET.size()];
}

std::string encrypt(const std::string& input, std::string_view key) {
    std::string encrypted;
    for (size_t i = 0; i < input.size(); ++i) {
        // XOR 运算并保证结果在扩展字符集范围内
        int value = (char_to_index(input[i]) ^ char_to_index(key[i])) % EXTENDED_CHARSET.size();
        encrypted += index_to_char(value); // 还原为扩展字符集字符
    }
    return encrypted;
}

std::string decrypt(const std::string& encrypted, std::string_view key) {
    std::string decrypted;
    for (size_t i = 0; i < encrypted.size(); ++i) {
        // XOR 运算并保证结果在扩展字符集范围内
        int value = (char_to_index(encrypted[i]) ^ char_to_index(key[i])) % EXTENDED_CHARSET.size();
        decrypted += index_to_char(value); // 还原为扩展字符集字符
    }
    return decrypted;
}

void save_to_file(const std::string& encrypted_text, const std::string& decrypted_text) {
    std::ofstream file("output.txt");
    if (file.is_open()) {
        file << "加密结果:\n" << encrypted_text << "\n";
        file << "解密结果:\n" << decrypted_text << "\n";
        file.close();
        std::cout << "结果已保存到 output.txt 文件中\n";
    }
    else {
        std::cout << "无法打开文件保存结果！\n";
    }
}

std::string key = "relia123456";   // 生成 11 字符密钥

int main() {


    std::string user_input;
    std::string encrypted_text;
    std::string decrypted_text;

    Component input_text = Input(&user_input, "请输入文本");
    Component input_key = Input(&key, "请输入密钥");

    // 加密按钮
    auto btn_encrypt = Button("加密", [&] {
        encrypted_text = encrypt(user_input, key);
    });

    // 解密按钮
    auto btn_decrypt = Button("解密", [&] {
        decrypted_text = decrypt(user_input, key);
    });

    auto btn_save = Button("保存结果", [&] {
        save_to_file(encrypted_text, decrypted_text);
    });


    // 组件树
    auto component = Container::Vertical({
        input_text,
        input_key,
        btn_encrypt,
        btn_decrypt,
        btn_save
    });

    // 渲染界面
    auto renderer = Renderer(component, [&] {
        return vbox({
            hbox(text("文本: "), input_text->Render()),
            hbox(text("密钥: "), input_key->Render()),
            separator(),
            hbox(btn_encrypt->Render() | flex, btn_decrypt->Render() | flex),
            hbox(btn_save->Render() | flex),
            separator(),
            text("加密结果: " + encrypted_text) | color(Color::Green),
            separator(),
            text("解密结果: " + decrypted_text) | color(Color::Cyan),
            }) | border;
        });

    // 启动交互式界面
    auto screen = ScreenInteractive::TerminalOutput();
    screen.Loop(renderer);

}
