#include <iostream>
#include <string>
#include <fstream>
#include <httplib.h>
#include <nlohmann/json.hpp>
using namespace std::string_literals;
using json = nlohmann::json;

int main(){
    httplib::Client cli{ "http://localhost:8080" };
    // 设备身份验证
    httplib::Headers headers = {
        {"Device-ID", "device123"},
        {"Password", "password123"},
        {"File-Type", "log"}
    };

    std::ifstream file{ "./data.json",std::ios::binary };
    if (!file.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return -1;
    }
    const std::string file_contents{ std::istreambuf_iterator{ file }, {} };

    // 创建 multipart 请求
    httplib::MultipartFormDataItems items = {
        {"file", file_contents, "data.json"}
    };

    auto upload_res = cli.Post("/upload", headers, items);

    if (upload_res) {
        std::cout << "Response code: " << upload_res->status << std::endl;
        std::cout << "Response body: " << upload_res->body << std::endl;
    }
    else {
        std::cerr << "File upload failed!" << std::endl;
    }
}