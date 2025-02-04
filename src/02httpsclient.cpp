#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <print>
#include <httplib.h>

int main(){
    httplib::SSLClient cli{ "localhost", 8080 };
    cli.enable_server_certificate_verification(false); // 忽略 SSL 证书验证
    auto result = cli.Get("/hi");
    std::println("status: {}\ndata: {}", result->status, result->body);
}