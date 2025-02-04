cmake_minimum_required (VERSION 3.12)

project ("CppLab")

set(CMAKE_CXX_STANDARD 23)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build/bin)

set(code "src/02httpsclient.cpp")

include_directories(${CMAKE_SOURCE_DIR}/include)

# 设置证书文件和私钥文件的路径
set(CERT_PATH "${CMAKE_SOURCE_DIR}/certs")
set(CERT_FILE "${CERT_PATH}/server.crt")
set(KEY_FILE "${CERT_PATH}/server.key")

# 输出文件路径，确保路径正确
message(STATUS "Certificate file: ${CERT_FILE}")
message(STATUS "Private key file: ${KEY_FILE}")

add_executable (CppLab ${code})

# 传递文件路径给编译后的应用程序
target_compile_definitions(${PROJECT_NAME} PRIVATE
    CERT_FILE="${CERT_FILE}"
    KEY_FILE="${KEY_FILE}"
)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt-header-only)

find_package(GTest REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::GTest)

find_package(benchmark REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE benchmark::benchmark)

# 使用 openssl
set(CMAKE_PREFIX_PATH "D:/vcpkg-master/installed/x64-windows")
find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
