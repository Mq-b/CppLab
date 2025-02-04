#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <print>
#include <thread>
#include <httplib.h>
using namespace std::chrono_literals;

int main(){
    httplib::SSLClient cli{ "localhost", 8080 };
    cli.enable_server_certificate_verification(false); // 忽略 SSL 证书验证
    while (true){
        auto result = cli.Get("/hi");
        std::println("status: {}\ndata: {}", result->status, result->body);
        std::this_thread::sleep_for(1s);
    }
}