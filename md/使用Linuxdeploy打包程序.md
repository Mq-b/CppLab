# 使用 打包程序

## 前言

[`linuxdeploy`](https://github.com/linuxdeploy/linuxdeploy) 是一个开源、模块化的工具，用于在 Linux 系统中打包可执行程序及其依赖项。它可以自动分析并收集程序运行所需的库文件，并将其打包为一个独立的 `.AppImage` 文件，从而实现跨发行版运行的能力。

> 在 Linux 中，一提到应用程序打包，往往第一个想到的就是 `AppImage`。它几乎成了 **Linux 下最通用、最流行的打包格式之一**，尤其也适合单文件分发、无需安装的应用场景。

`AppImage` 格式是一种打包应用程序的格式，允许它们在各种不同的目标系统（基础操作系统、发行版）上运行，而无需进一步修改。使用 `AppImage` 格式，您可以打包桌面应用程序为 AppImage，使其能够在常见的基于 Linux 的操作系统上运行，例如 RHEL、CentOS、Ubuntu、Fedora、Debian 及其衍生版。

> [!WARNING]
>
> `linuxdeploy` 打包 `AppImage` 依赖于 [`appimagetool`](https://github.com/AppImage/AppImageKit)。
>
> 该工具采用插件机制，不会将依赖自动安装到本地系统中，而是在打包过程中按需**联网从 GitHub 下载**使用。
>
> 准确的来说是会联网下载 [AppImage runtime](https://github.com/AppImage/type2-runtime) 文件，然后将其嵌入 AppImage  本身中。
>
> 视频内容中会展示实际打包过程，需注意以下几点：目录结构要清晰，确保系统联网；默认情况下不会打包如 libstdc++ 等系统库。如果你的系统上使用了较新版本的 libstdc++，而目标平台版本较旧，则可能需要将其手动打入 AppImage。

> [!NOTE]
>
> AppImage 打包的做法本质上是一个“压缩壳”，它不会修改你的程序，它可以将所有依赖打包到可执行文件中，也可以将它们放入一个独立的目录。它的工作方式是：
>
> 1. 启动时自动挂载自身为只读的临时文件系统（例如 `/tmp/.mount_xxx/`）；
> 2. 设置一系列环境变量（特别是 `LD_LIBRARY_PATH` 指向如 `AppDir/usr/lib`）；
> 3. 启动你打包进去的普通 ELF 可执行程序；
> 4. 可执行程序从 `AppDir/usr/lib` 加载你放进去的 `.so` 依赖库（比如 Qt、wxWidgets）；
> 5. 如果找不到，再退回系统库。
>
> > AppImage 本身支持将所有依赖都打包到自身中，只有一个单可执行文件进行分发，但是在我们本节内容中不会提及。
>
> 由于 AppImage 在运行时会挂载自身并设置加载路径，其内部的可执行程序是在“虚拟路径”中运行的，因此**不能直接配合 dump 文件进行 GDB 调试**。如果你需要调试程序，建议先使用命令将 AppImage 解压，**提取出原始的可执行文件和依赖目录**，然后在真实文件路径下进行调试。
>
> 你可以使用如下命令来解包 AppImage 文件：
>
> ```shell
> ./YourApp.AppImage --appimage-extract
> ```
>
> 会生成一个 `squashfs-root` 的文件夹，可执行文件存放在 `usr/bin`。

## 使用示例

说一千道一万不如自己上手试试。

**有些时候我们只是一些简单的需求所引入的库，就会造成非常多的依赖。**

例如：我需要使用 sqlite 加密数据库，然而官方的 sqlite3 默认是不支持加密的。

于是寻找了 [wxSQLite3](https://github.com/utelle/wxsqlite3) 三方库，虽然它本身并没有依赖很多，但是却依赖了 [wxWidgets](https://github.com/wxWidgets/wxWidgets)，而这是一个庞大的库，使用 `ldd` 命令查看可执行程序的依赖就会注意到。

> 视频展示项目

靠我们手动去添加依赖，非常麻烦，所以，我们使用 `linuxdeploy`。

**下载 linuxdeploy**：

```shell
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
```

将其放入 `/usr/local/bin/` 目录中方便使用。

使用 `linuxdeploy` 打包 Linux 可执行程序时，**需要遵循 AppDir 结构规范**：

```txt
AppDir/
├── AppRun            ← 可执行入口脚本或程序（自动生成）
├── myapp.desktop     ← 桌面启动项文件（打包后自动生成）
├── myapp.png         ← 图标文件（打包后自动生成）
└── usr/
    ├── bin/          ← 需要打包的主程序和依赖的二进制文件
    ├── lib/          ← 所需的 .so 动态库文件
    └── share/        ← 可选资源（icons、desktop 文件等）
```

1. **新建目录**

    ```shell
    mkdir AppDir
    mkdir AppDir/usr
    mkdir AppDir/usr/bin
    mkdir AppDir/usr/lib
    mkdir AppDir/usr/share
    mkdir AppDir/usr/share/applications
    ```

2. **创建 `.desktop` 文件**

   ```shell
   touch AppDir/usr/share/applications/CppLab.desktop
   ```

   内容：

   ```ini
   [Desktop Entry]
   Type=Application         # 应用类型，通常为 Application
   Name=CppLab              # 应用的显示名称
   Exec=CppLab              # 应用的执行命令（可以是相对路径或绝对路径）
   Terminal=true            # 是否在终端中运行，true 表示在终端中运行
   Icon=icons               # 设置应用图标，这里指定了图标名称，需要额外设置图标
   Categories=Utility       # 设置该应用的分类，用于菜单中的分类显示
   ```

3. **设置图标**

   为了让 `.desktop` 文件生效，你需要为应用指定一个图标，并将图标放在合适的位置。图标文件必须是 `.png` 格式，可以选择不同的尺寸来适配不同的显示设备。

   将图标放在以下目录：

   ```shell
   AppDir/usr/share/icons/hicolor/256x256/apps
   ```

   你可以使用多种尺寸的图标，最常见的是 `256x256`、`32x32`、`16x16` 等，系统会根据需要选择适当的图标大小。也不要求你的图片一定是这些分辨率。

4. **打包命令**

   ```shell
   linuxdeploy   --appdir AppDir   --executable AppDir/usr/bin/CppLab   --desktop-file AppDir/usr/share/applications/CppLab.desktop   --output appimage
   ```

   - **`--appdir`**: 指定应用程序的文件夹，`AppDir` 包含所有必要的文件和资源。
   - **`--executable`**: 指定被打包的应用程序的可执行文件。
   - **`--desktop-file`**: 指定 `.desktop` 文件，描述了应用的元数据和启动方式。
   - **`--output`**: 指定输出格式，这里是 `appimage`，表示生成 `.AppImage` 文件。

   执行此命令后，`linuxdeploy` 将会生成一个 `.AppImage` 文件，生成的文件名通常是 `CppLab-x86_64.AppImage`。启动时，系统会自动加载 `AppDir/usr/lib` 目录中的 `.so` 依赖库，而其他配置文件则会根据当前可执行程序的相对路径查找并加载。

## 添加系统库以避免兼容性问题

`linuxdeploy` 默认不会打包系统库，非常经典的就是 `libstdc++.so.6` 。

这会导致什么问题？举个例子：我在 Ubuntu22.04 中为了使用 C++20 安装了 gcc13.1 编译器进行开发（同时也使用了更新的 `libstdc++.so.6`）。而让你用 `linuxdeploy` 打包你的程序，到目标平台，运行时大概会报类似这样的错误：

```shell
GLIBCXX_3.4.30 not found (required by your_program)
undefined symbol: _ZSt28__throw_bad_array_new_lengthv
```

这类错误的根本原因是：**目标机器的 `libstdc++.so.6` 版本太老，不支持你使用的新编译器生成的二进制代码中所依赖的符号或宏定义。**

为了解决这个问题，可以在使用 `linuxdeploy` 工具打包时，添加额外的 `--library` 选项，显式指定打包你当前使用的 `libstdc++.so.6`：

```shell
--library /usr/lib/x86_64-linux-gnu/libstdc++.so.6
```

具体路径可以使用 `ldd` 命令来查看你的程序实际依赖的库位置：

```shell
ldd your_program | grep libstdc++
```

## 总结

整体打包流程其实很简单，只不过是更多的介绍了一些 `.AppImage` 格式的事情，以及注意事项，如：

- `linuxdeploy` 打包会**联网**从 GitHub 下载 AppImage Runtime。
- `.AppImage` 格式的特殊，要想**调试 Dump 文件**的话需要从中提取出未打包的原始程序。
- 默认不打包系统库，如 `libstdc++`，可能出现当前机器与目标机器的 `libstdc++` 版本不兼容问题，建议还是额外添加 `--library` 选项打包。
