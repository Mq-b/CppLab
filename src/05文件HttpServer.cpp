#include <iostream>
#include <string>
#include <filesystem>
#include <httplib.h>
#include <nlohmann/json.hpp>
using namespace std::string_literals;
using json = nlohmann::json;

// 模拟设备和密码验证
bool verify_device(const std::string& device_id, const std::string& password) {
    // 这里简单验证设备ID和密码，实际应用可以查询数据库或配置文件
    return (device_id == "device123" && password == "password123");
}

int main() {
    httplib::Server svr;

    // 处理文件上传请求
    svr.Post("/upload", [](const httplib::Request& req, httplib::Response& res) {
        // 获取请求头中的 Device-ID
        std::string device_id = req.get_header_value("Device-ID");
        std::string Password = req.get_header_value("Password");

        // 检查设备是否是已验证的
        if (!verify_device(device_id, Password)) {
            // 设备未验证，返回错误
            res.status = 401;
            res.set_content("Unauthorized", "text/plain");
            std::clog << "Unauthorized device: "s + device_id + '\n';
            return;
        }

        // 检查是否包含文件
        if (req.has_file("file")) {
            auto file = req.get_file_value("file");

            // 从请求中获取文件名和类型
            std::string file_name = file.filename;
            std::string file_type = req.get_header_value("File-Type");

            // 创建存储目录
            std::string storage_dir = "./uploaded_files/" + device_id + "/";
            if (!std::filesystem::exists(storage_dir)) {
                std::filesystem::create_directories(storage_dir);
            }

            // 保存文件
            std::string file_path = storage_dir + file_name;
            std::ofstream ofs(file_path, std::ios::binary);
            ofs.write(file.content.data(), file.content.size());
            ofs.close();

            // 响应成功
            res.set_content("File uploaded successfully: " + file_name, "text/plain");
            std::clog << device_id + " uploaded file: "s + file_name + '\n';
        }
        else {
            // 文件缺失，返回错误
            res.status = 400;
            res.set_content("No file uploaded", "text/plain");
            std::clog << device_id + " failed to upload file: No file uploaded\n";
        }
        });
    // 启动服务器，监听 0.0.0.0:8080
    std::cout << "Server is running on port 8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
}

// cmd
//  curl -X POST http://localhost:8080/upload -H "Device-ID: device123" -H "Password: password123" -H "File-Type: log" -F "file=@D:\project\CppLab\build\bin\test\data.json"
