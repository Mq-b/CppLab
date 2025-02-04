#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <format>
#include <httplib.h>

int main(){
    httplib::SSLServer svr{ CERT_FILE,KEY_FILE };
    int count = 1;
    svr.Get("/hi", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content(std::format("Hello World {:02}", count++), "text/html");
    });

    svr.listen("0.0.0.0", 8080);
}