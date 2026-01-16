# vcpkg 导出库

在开发过程中，我们经常使用 `vcpkg` 编译依赖复杂的第三方库。为了实现项目集成、环境解耦或方便离线分发，通常需要将编译好的二进制文件导出。

通过 `vcpkg export` 命令，可以将指定包及其完整的依赖链路（Dependency Chain）提取到独立目录中，方便在其他项目中直接引用。

## 导出指令

使用 `--raw` 参数可导出原始的目录结构（含 `include`, `lib`, `bin`），这是构建自定义 SDK 最为直观的方式。

```bash
vcpkg export <package_name>:x64-windows --raw --output=<output_path>
```

它不仅仅帮我们打包导出了库本身，还有它所有的依赖项，确保在新环境中能够无缝运行。

例如 `vcpkg export aravis`，那它依赖的 glib-2.0、gobject-2.0、gio-2.0、libxml-2.0、zlib、gmodule-2.0、libusb-1.0 等库，也会一并被导出。

导出的目录会自动包含 `CMake` 配置文件（`*.cmake`）与 `pkg-config` 描述文件（`*.pc`），确保了依赖项的完整性与可迁移性。在实际集成时，开发者拥有极高的灵活性：

* **标准集成**：利用 CMake 的 `find_package` 机制或 `pkg-config` 工具实现自动化探测与路径关联。
* **手动集成**：通过最原始的相对路径拼接方式编写构建文件（如 `CMakeLists.txt` 或 `Makefile`），直接指向 `include` 和 `lib` 目录。或者 `.sln` 解决方案手动添加库引用。

这种多模式支持既保障了专业项目的规范性，也兼顾了快速原型开发的便捷性。
