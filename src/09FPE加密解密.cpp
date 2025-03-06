#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <botan/fpe_fe1.h>
#include <iostream>

using namespace Botan;

inline std::string format_output(const BigInt& num) {
    std::string s = num.to_dec_string();
    if (s.length() > 11) {
        throw std::runtime_error("Result exceeds 11 digits");
    }
    return std::string(11 - s.length(), '0') + s;
}

int main() {
    try {
        std::string key_str = "0123456789ABCDEF0123456789ABCDEF";  // 加密 key
        SymmetricKey key(reinterpret_cast<const uint8_t*>(key_str.data()), key_str.size());

        // 设置处理范围 10^11 也就是 11 位
        const auto n = BigInt("100000000000");

        FPE_FE1 fpe(n);
        fpe.set_key(key);

        std::string original = "12345678901";
        std::cout << "原始输入: " << original << std::endl;

        std::string encrypted = format_output(fpe.encrypt(BigInt::from_string(original), nullptr, 0));
        std::cout << "加密后:   " << encrypted << std::endl;

        std::string decrypted = format_output(fpe.decrypt(BigInt::from_string(encrypted), nullptr, 0));
        std::cout << "解密后:   " << decrypted << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}