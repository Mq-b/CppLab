#include <iostream>
#include <nfd.h>
#include <tinyfiledialogs.h>

int main() {
    tinyfd_messageBox("提示", "请选择一个文件", "ok", "error", 1);
    
    if (NFD_Init() != NFD_OKAY) {
        std::cerr << "NFD 初始化失败: " << NFD_GetError() << std::endl;
        return 1;
    }

    nfdchar_t* outPath = nullptr;
    nfdfilteritem_t filterItem[1] = { { "Text files", "txt" } };
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);

    if (result == NFD_OKAY) {
        std::cout << "你选择的文件是: " << outPath << std::endl;
        free(outPath);
    }
    else if (result == NFD_CANCEL) {
        std::cout << "用户取消了选择。" << std::endl;
    }
    else {
        std::cerr << "错误: " << NFD_GetError() << std::endl;
    }

    NFD_Quit();
}
// 这段代码使用了 Native File Dialog 库来实现文件选择对话框。