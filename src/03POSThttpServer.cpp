#include <iostream>
#include <string>
#include <print>
#include <httplib.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    // 创建 httplib 服务器实例
    httplib::Server svr;

    // 定义 /report 接口，处理 POST 请求
    svr.Post("/report", [](const httplib::Request& req, httplib::Response& res) {
        try {
            // 解析请求体中的 JSON 数据
            std::println("Received body: {}", req.body);
            auto j = json::parse(req.body);
            std::string type = j.value("type", "");

            json response;

            if (type == "status") {
                // 处理设备状态上报
                response = {
                    {"type", "status_response"},
                    {"status", "success"},
                    {"message", "Device status updated"}
                };
            }
            else if (type == "file_transfer") {
                // 处理文件上传请求
                response = {
                    {"type", "file_transfer_response"},
                    {"status", "success"},
                    {"message", "Ready to receive file"}
                };
            }
            else {
                // 未知请求类型
                response = {
                    {"type", "error"},
                    {"status", "failed"},
                    {"message", "Unknown request type"}
                };
            }
            // 设置响应内容和 Content-Type
            res.set_content(response.dump(), "application/json");
        }
        catch (const std::exception&) {
            // 解析 JSON 出错时返回错误响应
            json error_response = {
                {"type", "error"},
                {"status", "failed"},
                {"message", "Invalid JSON format"}
            };
            res.status = 400;
            res.set_content(error_response.dump(), "application/json");
        }
        });

    // 启动服务器，监听 0.0.0.0:8080
    std::cout << "Server is running on port 8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
}
// cmd
//  curl.exe -X POST http://localhost:8080/report -d "{\"type\": \"file_transfer\"}" -H "Content-Type: application/json"
// linux shell
//  curl -X POST http://localhost:8080/report -d '{"type": "file_transfer"}' -H "Content-Type: application/json"
//  seq 1 1000 | xargs -P 1000 -I {} curl -s -X POST http://localhost:8080/report -d '{"type": "file_transfer"}' -H "Content-Type: application/json"
