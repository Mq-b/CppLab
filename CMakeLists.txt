cmake_minimum_required (VERSION 3.12)

project ("CppLab")

set(CMAKE_CXX_STANDARD 20)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/bin)

#set(CMAKE_PREFIX_PATH "/home/mq-b/project/vcpkg/installed/x64-linux")
#set(wxsqlite3_LIBRARIES "/usr/local/lib/libwxcode_gtk3u_wxsqlite3-3.0.so")

# 硬编码 wxWidgets 的头文件路径 属于 wxsqlite3 的依赖，使用 apt 安装
#include_directories(
#    /usr/lib/x86_64-linux-gnu/wx/include/gtk3-unicode-3.0 
#    /usr/include/wx-3.0
#)
#
#add_compile_options(-D_FILE_OFFSET_BITS=64 -DWXUSINGDLL -D__WXGTK__ -pthread)

# add_compile_options(/Zi)
# add_link_options(/DEBUG)

set(code 
#   "src/01httpserver.cpp"
#   "src/02httpsclient.cpp"
#   "src/03POSThttpServer.cpp"
#   "src/04POSTHttpClient.cpp"
#   "src/05文件HttpServer.cpp"
#   "src/06文件HttpClient.cpp"
#   "src/07使用FTXUI创建终端程序.cpp"
#   "src/08使用FTUXUI创建加密解密终端输入程序.cpp"
#   "src/09FPE加密解密.cpp"
#   "src/11使用wxSQLite操作加密数据库.cpp"
    "src/11test.cpp"
)

include_directories(${CMAKE_SOURCE_DIR}/include)

# # 设置证书文件和私钥文件的路径
# set(CERT_PATH "${CMAKE_SOURCE_DIR}/certs")
# set(CERT_FILE "${CERT_PATH}/server.crt")
# set(KEY_FILE "${CERT_PATH}/server.key")

# # 输出文件路径，确保路径正确
# message(STATUS "Certificate file: ${CERT_FILE}")
# message(STATUS "Private key file: ${KEY_FILE}")

# # 引入两个外部子项目
# add_subdirectory(external/nativefiledialog-extended)
# add_subdirectory(external/tinyfiledialogs)

add_executable (${PROJECT_NAME} ${code})

#target_link_libraries(${PROJECT_NAME} PRIVATE
#    wx_baseu-3.0
#    ${wxsqlite3_LIBRARIES} 
#)

# # 传递文件路径给编译后的应用程序
# target_compile_definitions(${PROJECT_NAME} PRIVATE
#     CERT_FILE="${CERT_FILE}"
#     KEY_FILE="${KEY_FILE}"
# )

# target_link_libraries(${PROJECT_NAME} PRIVATE nfd tinyfiledialogs)

# find_package(fmt CONFIG REQUIRED)
# target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt-header-only)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json)

# find_package(ftxui 5 REQUIRED)

# target_link_libraries(${PROJECT_NAME}
#   PRIVATE ftxui::screen
#   PRIVATE ftxui::dom
#   PRIVATE ftxui::component
#   dbghelp
# )
# 因为此库 cmake 有问题，所以我们设置显式查找头文件路径
#include_directories("D:/CXX_LIB/FTXUI/include")

# 使用 openssl
set(CMAKE_PREFIX_PATH "D:/vcpkg-master/installed/x64-windows")
find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# set(CMAKE_PREFIX_PATH "D:/dagong/test/vcpkg/installed/x64-windows")
# find_package(botan CONFIG REQUIRED)
#   target_link_libraries(${PROJECT_NAME} PRIVATE Botan::Botan)

# find_package(imgui CONFIG REQUIRED)
#   target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui)

# find_package(cpptrace CONFIG REQUIRED)
#   target_link_libraries(${PROJECT_NAME} PRIVATE cpptrace::cpptrace)