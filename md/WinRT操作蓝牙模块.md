# WinRT 操作蓝牙模块

## 查找蓝牙设备

1. **查看已经配对的蓝牙设备**

    不在乎是否在线，只要是配对过的蓝牙设备，都可以获取到，非常快速。

    ```cpp
    #include <winrt/Windows.Devices.Enumeration.h>
    #include <winrt/Windows.Devices.Bluetooth.h>
    #include <winrt/Windows.Foundation.Collections.h>
    #include <iostream>
    #include <string>
    #include <Windows.h>

    std::string WideToUTF8(const wchar_t* wideStr) {
        int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
        std::string utf8Str(bufferSize, 0);
        WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, &utf8Str[0], bufferSize, nullptr, nullptr);
        return utf8Str;
    }

    int main() {
        winrt::init_apartment();

        try {
            // 获取已配对的蓝牙设备列表
            auto selector = winrt::Windows::Devices::Bluetooth::BluetoothDevice::GetDeviceSelector();
            auto devices = winrt::Windows::Devices::Enumeration::DeviceInformation::FindAllAsync(selector).get();

            std::cout << "发现的蓝牙设备数量: " << devices.Size() << "\n\n";

            for (auto const& device : devices) {
                std::string name = WideToUTF8(device.Name().c_str());
                std::string id = WideToUTF8(device.Id().c_str());

                std::cout << "名称: " << (name.empty() ? "(未知)" : name) << "\n";
                std::cout << "ID: " << id << "\n\n";
            }

        }
        catch (const winrt::hresult_error& ex) {
            std::string errorMsg = WideToUTF8(ex.message().c_str());
            std::cerr << "错误: 0x" << std::hex << ex.code()
                << " - " << errorMsg << "\n";
        }
    }
    ```

2. **搜索蓝牙设备**

    不管是否配对的设备均会显示，需要进行蓝牙广播，所以需要等待一段时间。

    ```cpp
    #include <winrt/Windows.Devices.Enumeration.h>
    #include <winrt/Windows.Foundation.Collections.h>
    #include <iostream>
    #include <string>
    #include <Windows.h>

    std::string WideToUTF8(const wchar_t* wideStr) {
        int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
        std::string utf8Str(bufferSize, 0);
        WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, &utf8Str[0], bufferSize, nullptr, nullptr);
        return utf8Str;
    }

    int main() {
        winrt::init_apartment();

        try {
            // 蓝牙 AQS 协议 ID（包含附近所有设备）
            std::wstring selector = L"System.Devices.Aep.ProtocolId:=\"{e0cbf06c-cd8b-4647-bb8a-263b43f0f974}\"";
            auto devices = winrt::Windows::Devices::Enumeration::DeviceInformation::FindAllAsync(
                selector,
                nullptr,
                winrt::Windows::Devices::Enumeration::DeviceInformationKind::AssociationEndpoint
            ).get();

            std::cout << "附近蓝牙设备数量: " << devices.Size() << "\n\n";

            for (auto const& device : devices) {
                std::string name = WideToUTF8(device.Name().c_str());
                std::string id = WideToUTF8(device.Id().c_str());

                std::cout << "名称: " << (name.empty() ? "(未知)" : name) << "\n";
                std::cout << "ID: " << id << "\n";
                std::cout << "已配对: " << (device.Pairing().IsPaired() ? "是" : "否") << "\n\n";
            }

        }
        catch (const winrt::hresult_error& ex) {
            std::string errorMsg = WideToUTF8(ex.message().c_str());
            std::cerr << "错误: 0x" << std::hex << ex.code()
                << " - " << errorMsg << "\n";
        }
    }
    ```

    可以将代码修改为每次发现一个设备时立即打印，而不是等扫描完成后统一打印：

    ```cpp
    std::string WideToUTF8(const wchar_t* wideStr) {
        int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
        std::string utf8Str(bufferSize, 0);
        WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, &utf8Str[0], bufferSize, nullptr, nullptr);
        return utf8Str;
    }
    int main() {
        winrt::init_apartment();

        // AQS 选择器：扫描附近蓝牙设备（不论是否配对）
        std::wstring selector = L"System.Devices.Aep.ProtocolId:=\"{e0cbf06c-cd8b-4647-bb8a-263b43f0f974}\"";

        // 创建 DeviceWatcher
        auto watcher = winrt::Windows::Devices::Enumeration::DeviceInformation::CreateWatcher(
            selector,
            nullptr,
            winrt::Windows::Devices::Enumeration::DeviceInformationKind::AssociationEndpoint
        );

        // 当发现新设备时触发
        watcher.Added([](auto const&, winrt::Windows::Devices::Enumeration::DeviceInformation const& device) {
            std::string name = WideToUTF8(device.Name().c_str());
            std::string id = WideToUTF8(device.Id().c_str());
            bool paired = device.Pairing().IsPaired();

            std::cout << "发现设备:\n";
            std::cout << "  名称: " << (name.empty() ? "(未知)" : name) << "\n";
            std::cout << "  ID: " << id << "\n";
            std::cout << "  已配对: " << (paired ? "是" : "否") << "\n\n";
        });

        watcher.Updated([](auto const&, winrt::Windows::Devices::Enumeration::DeviceInformationUpdate const& update) {
            std::cout << "设备更新: " << WideToUTF8(update.Id().c_str()) << "\n";
        });

        watcher.Removed([](auto const&, winrt::Windows::Devices::Enumeration::DeviceInformationUpdate const& update) {
            std::cout << "设备移除: " << WideToUTF8(update.Id().c_str()) << "\n";
        });

        watcher.EnumerationCompleted([](auto const&, auto const&) {
            std::cout << "扫描完成。\n";
        });

        watcher.Stopped([](auto const&, auto const&) {
            std::cout << "设备监听器已停止。\n";
        });

        watcher.Start();

        std::cout << "扫描中，请等待...\n";
        std::cout << "按 Enter 键退出。\n";
        std::cin.get(); // 等待用户输入，保持程序运行
    }
    ```
