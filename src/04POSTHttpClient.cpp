#include <iostream>
#include <thread>
#include <vector>
#include <httplib.h>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

// 发送 POST 请求的函数
void send_request(int id) {
    httplib::Client cli("http://localhost:8080");

    // 构造 JSON 请求体
    json request_data = {
        {"type", "file_transfer"}
    };

    // 发送 POST 请求
    auto res = cli.Post("/report", request_data.dump(), "application/json");

    // 处理响应
    if (res && res->status == 200) {
        cout << "Thread " << id << " received response: " << res->body << endl;
    }
    else {
        cout << "Thread " << id << " request failed!" << endl;
    }
}

int main() {
    const int num_threads = 1000; // 并发请求数
    vector<thread> threads;

    cout << "Starting " << num_threads << " concurrent requests..." << endl;

    // 启动多个线程
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(send_request, i);
    }

    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }

    cout << "All requests completed." << endl;
}
