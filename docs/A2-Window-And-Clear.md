# A2 技术指南：自己的 D3D12 窗口与持续清屏

版本：1.1 | 更新日期：2026-09-20 | 零 Windows 桌面开发基础建议预算：10 小时，可分多次完成

前置：[A1：CLion 与 D3D12 环境](<F:/GameDevelop/SomeProjects/GPUVisibilityLab/docs/A1-CLion-D3D12-Setup.md>)。继续使用 Windows、CLion、MSVC、x64、Debug、CMake 和 Ninja，实际设备仍明确选择 RTX 5070 Ti。

### 先读这里：这次不假设你会 Win32

本版面向“会一些 C++，但从未写过 Windows 桌面程序”的读者，为 DX12Lab 的窗口、输入、运行循环和资源管理补齐基础。当前本地学习目录叫 `GPUVisibilityLab`，这里沿用现有路径，不要求另建一个名为 DX12Lab 的项目。

你只需先能理解 C++ 的函数、结构体、指针、`switch` 和基本作用域；Windows 特有的类型、回调与 API 约定会在第 4 节解释。遇到 `App*`、`&app`、`app->running` 就完全读不懂时，先补对应的 C++ 语法，不必先学一整套 GUI 框架。

**零基础阅读顺序：** 第 1、2 节 → 第 3 节的“建立工程” → 第 4 节与附录 D 的纯 Win32 练习 → 回看第 3 节七类图形对象 → 第 5～12 节 → 用附录 A/B 核对完整 D3D12 实现。不要从几百行完整清屏源码开始逐行硬背。

本次修订只补充文档，不替你实现已有的 `a2-window-clear/main.cpp`、`App.h` 和 `App.cpp`，也不代勾学习验收。

## 1. 本关交付与边界

**本关只做一件事：让自己创建的窗口，通过自己的 D3D12 命令循环持续显示清屏色，并能解释它为什么工作。**

最终程序应满足：

1. 启动后出现独立窗口，持续执行清屏与 Present，而不是只显示一次背景色。
2. 窗口获得焦点时，按 `1 / 2 / 3` 切换三种清屏色，不重启、不重新编译。
3. 点击关闭按钮、按 `Esc` 或 `Alt+F4` 均能正常结束，释放资源前先确认 GPU 工作完成。
4. 能在自己的代码中指出命令在哪里录制、在哪里提交、在哪里等待 GPU。
5. 在 Debug Layer 开启的情况下运行，无未解决的 WARNING、ERROR 或 CORRUPTION 消息。

这里“立即生效”指：输入被消息循环处理后，新颜色进入下一次命令录制，并在后续显示刷新时可见；不是零延迟承诺，也不是保存 C++ 文件就能热更新运行中的程序。

### 有意不做的内容

- 不画三角形，不写 HLSL，不需要 DXC、Root Signature、PSO、顶点缓冲或深度缓冲。
- 不引入引擎框架、ImGui、资源管理器、多线程、多队列或多帧并行。
- 不实现自由缩放、最大化、独占全屏、HDR、MSAA、设备丢失后的自动恢复。
- 固定初始客户区尺寸为 `1280 x 720`，允许移动、最小化和恢复；跨显示器 DPI 行为不属于验收项。
- 本关每帧等待一次 GPU，优先保证生命周期容易理解，不以吞吐量或帧率作为成绩。

**两个交换链缓冲不等于允许 CPU 连续录制两帧。** 本文只有一个命令分配器，每帧必须等它上一次的 GPU 使用结束后才能重置。

本文交付的是学习文档，附录提供完整参考源码；没有替你完成学习验收，也没有改动 A1/A2 工程或官方参考工程。附录 C 记录文档代码的检查范围，验收框全部留空。

## 2. 零基础路线与时间分配

原版 6 小时路线默认已经会创建 Win32 窗口。本版把窗口阶段从 40 分钟扩展为约 4 小时 40 分钟，总预算约 10 小时。预算用于安排学习，不是必须在截止时间前跳过基础的要求；环境问题和 C++ 语法补课另计。

| 检查点 | 内容 | 预算 | 阶段证据 |
|---|---|---:|---|
| A2.0 | 建立自己的工程、浏览对象职责 | 20 分钟 | CMake 配置正确，对象表先有印象即可 |
| A2.1a | 第 4.1～4.3 节：程序、类型与构建 | 50 分钟 | 能读懂入口、句柄、宽字符串与链接库 |
| A2.1b | 第 4.4～4.6 节：窗口创建、回调、状态 | 70 分钟 | 能跟踪 HWND 与 App 指针的来回传递 |
| A2.1c | 第 4.7～4.8 节：消息与生命周期 | 45 分钟 | 能区分消息循环、绘制通知和退出请求 |
| A2.1d | 第 4.9～4.10 节：错误、资源与 Event | 40 分钟 | 不混用错误检查和资源释放方式 |
| A2.1e | 第 4.11～4.12 节与附录 D：纯窗口实作 | 75 分钟 | 不含 D3D12 的窗口、输入、关闭均通过 |
| A2.2 | 设备、队列、交换链 | 55 分钟 | 正确显卡、Debug Layer、双缓冲交换链 |
| A2.3 | 后缓冲资源与 RTV | 40 分钟 | 两个资源对应两个 RTV，索引正确 |
| A2.4 | 命令录制、提交与 Fence 等待 | 80 分钟 | 首次清屏，并持续提交新帧 |
| A2.5 | 运行时改色与断点追踪 | 45 分钟 | 不重启切色，解释颜色传递过程 |
| A2.6 | 关闭、最小化与错误处理 | 40 分钟 | 正常退出，无未解决调试消息 |
| A2.7 | 回归测试与口头自测 | 40 分钟 | 完成第 12 节验收记录 |
| 合计 | 不含 A1 环境问题返工 | **600 分钟** | **10 小时** |

可分为三次：先学会看懂并创建窗口，再独立完成窗口实验，最后接入 D3D12。**纯窗口练习未通过时，先不要叠加设备和交换链问题。** 已掌握 Win32 的读者可用第 4.12 节自测后走原来的约 6 小时路线。

同一问题连续定位 30 分钟没有进展，先记录操作、第一条有效错误、HRESULT 和当前调用位置，不用增加新功能来绕过它。

## 3. A2.0：先认识七类对象

| 对象 | 本关实例 | 它负责什么 | 它不是什么 |
|---|---|---|---|
| Device | `ID3D12Device` | 对所选适配器创建设备侧对象和资源 | 不是窗口，也不是提交入口 |
| Swap Chain | `IDXGISwapChain3` | 管理用于窗口呈现的缓冲，提供当前后缓冲索引 | 不会替你录制清屏命令 |
| RTV | RTV 堆中的两个描述符 | 描述把哪个资源作为渲染目标访问 | 不是像素存储，也不是一个独立 `ComPtr` 对象 |
| Command Allocator | `ID3D12CommandAllocator` | 管理命令录制所用的底层存储 | 不是纹理分配器；Reset 不会等待 GPU |
| Command List | `ID3D12GraphicsCommandList` | 在 CPU 侧录制屏障、清屏等命令 | 调用清屏方法不等于 GPU 已执行清屏 |
| Command Queue | `ID3D12CommandQueue` | 接收已关闭的命令列表，安排 GPU 执行 | 提交返回不代表执行完成 |
| Fence | `ID3D12Fence` | 以数值标记执行进度，配合事件进行 CPU 等待 | 不是帧率限制器，也不是自动加锁机制 |

另外还有三个配套概念：

- `HWND`：Win32 窗口句柄，表示画面要呈现到哪个窗口。
- Back Buffer：交换链提供的 `ID3D12Resource`，真正保存图像像素。
- RTV Descriptor Heap：存放 RTV 描述符的堆；本关不是 Shader Visible 堆。

对象关系：

```text
Win32 -> HWND
DXGI Factory -> RTX 5070 Ti Adapter -> D3D12 Device
Device -> Direct Queue
Factory + Direct Queue + HWND -> Swap Chain
Swap Chain -> BackBuffer[0], BackBuffer[1]
Device -> RTV Heap -> RTV[0], RTV[1] -> 对应的 BackBuffer
Device -> Allocator -> Command List 的命令存储
CPU 录制 Command List -> Queue 提交 -> GPU 执行
Queue Signal -> Fence 数值到达目标 -> Event 唤醒 CPU
```

RTV 堆与后缓冲资源是两类不同对象：创建 RTV 不会新建一张同尺寸图像。命令分配器管理的是命令存储，不应和保存像素的后缓冲混淆。[描述符堆](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createdescriptorheap)、[命令分配器](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandallocator-reset)

### 建立工程

建议学习工程目录为 `F:\GameDevelop\SomeProjects\GPUVisibilityLab\a2-window-clear`，与 `a1-environment-check` 并列，不放进 `references`，不覆盖 A1。

目标结构如下；当前 A2 已有初步骨架，这里描述完成后的职责，不代表已经完成验收：

```text
GPUVisibilityLab/
  a1-environment-check/
  a2-window-clear/
    CMakeLists.txt
    main.cpp
    App.h       # 已有的拆分方式可以保留
    App.cpp
  docs/
    A2-Window-And-Clear.md
    CheckPoints/
  references/
```

CLion 打开 A2 目录，复用 A1 的 `MSVC-D3D12` 工具链，建立 `A2-Debug` Profile，选择 `Debug / Ninja / amd64`；目标为 `a2_window_clear`。已经建立的目录和 Profile 直接继续使用，不要覆盖自己的代码。

附录 A 是完整 CMake，附录 B 是完整单文件参考实现。建议先按检查点实现，再用附录核对遗漏；不要把“参考代码能编译”当成“自己能解释”。源码使用系统 D3D12 路线，不依赖官方样例的 Agility SDK 导出、NuGet、`d3dx12.h` 或预编译头。

如果保留 `App.h / App.cpp`，把类型声明放在头文件、成员函数实现放在 `.cpp`，`main.cpp` 保留入口与主循环即可；CMake 要包含 `App.cpp`。不要在已有多文件实现外再加入附录 B 的整份文件，否则会出现重复的 `App`、`main` 或函数定义。窗口阶段先做附录 D 的独立练习，不必提前填满图形对象成员。

**验收：** 能回答“窗口、设备、交换链为什么是三个不同对象”，并确认新目标使用 MSVC、x64、Debug。

## 4. A2.1：从零理解 Windows 桌面程序

这一节的目标不是熟悉整个 Windows SDK，而是能够**独立写出并调试一个适合承载 DX12Lab 的窗口**。本关选择原生 C++ Win32，不需要先学习 MFC、WinForms、WPF、WinUI 或 Qt；也暂不学习菜单资源、安装包、注册表、多窗口框架和多线程 UI。

### 4.1 从“执行一次”到“持续响应”

普通练习程序通常是 `main → 计算 → 输出 → return`。桌面实验程序则需要在用户没有操作时继续存活，在有操作时及时响应：

```text
启动 exe，建立进程与主线程
  -> main 初始化应用状态
  -> 创建窗口，得到 HWND
  -> 循环：处理窗口消息；需要连续画面时推进一帧
  -> 收到退出请求，停止循环
  -> 清理自己拥有的资源
  -> main 返回，进程结束
```

进程是运行中的程序及其资源环境，线程是执行代码的路径，窗口是由系统管理的界面对象。它们不是一一等同的：一个进程可以有多个线程、多个窗口，也可以没有窗口。A2 只用一个主线程管理窗口与提交渲染命令；GPU 在另一条执行时间线上工作，不是 `WndProc` 的另一种叫法。

Win32 在这里指 Windows 原生桌面 API，并不要求构建 32 位程序。我们仍然用 x64。Win32 负责窗口与输入，DXGI 连接窗口和呈现，D3D12 负责图形命令；创建 `HWND` 本身不会创建设备或自动画出三角形。

### 4.2 为什么有 `main`，也有 `wWinMain`

**带图形窗口的程序可以使用 `main`。** 本关选择控制台子系统，方便保留 `std::cout / std::cerr` 日志；控制台程序同样可以调用 Win32 创建独立窗口。

| 构建选择 | 常见用户入口 | 对 A2 的影响 |
|---|---|---|
| `add_executable(a2_window_clear main.cpp)` | `int main()` | 本文采用；可在运行控制台查看日志 |
| `add_executable(a2_window_clear WIN32 main.cpp)` | `WinMain` 或 Unicode 形式 `wWinMain` | GUI 子系统；不能只加 `WIN32` 而仍期待原入口配置不变 |

Windows 示例常见 `int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)`。你目前只需认出它是另一种启动入口，不要为了照抄教程而同时保留两个入口，更不要手工设置链接器 `/ENTRY` 跳过运行库初始化。`WINAPI` 与稍后的 `CALLBACK` 都涉及函数调用约定，并不代表它们会创建窗口。[微软入口说明](https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point)、[CMake GUI 子系统](https://cmake.org/cmake/help/latest/prop_tgt/WIN32_EXECUTABLE.html)

`main` 内的 `GetModuleHandleW(nullptr)` 取得当前 exe 模块的句柄，用来填写窗口类和创建窗口所需的 `hInstance`。模块句柄不是窗口句柄，也不是当前显卡。[GetModuleHandleW](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlew)

### 4.3 先学会读 Windows 风格的 C++

#### 类型和参数

| 写法 | 在 A2 中怎么理解 | 避免的误解 |
|---|---|---|
| `HWND` | 系统窗口的句柄；交给窗口 API 使用 | 不是 `App*`，不能解引用或 `delete` |
| `HINSTANCE / HMODULE` | 标识加载进进程的 exe / DLL 模块 | 不是对象实例的 C++ 地址 |
| `HANDLE` | 这里用于 Fence 等待的 Event | 不是所有带 H 的类型都能 `CloseHandle` |
| `UINT / DWORD` | Windows 中的 32 位无符号整数 | x64 不会让它们自动变成 64 位 |
| `BOOL` | 整数类型；许多 API 用零表示假、非零表示真 | 不是 C++ `bool`；返回含义仍要查具体 API |
| `WPARAM / LPARAM / LRESULT` | 指针精度的消息参数或结果类型 | 不是固定的坐标或键值；解释随消息而变 |
| `LONG_PTR / SIZE_T` | 能容纳指针值或大小的整数类型 | x64 下不能随意用 32 位 `LONG` 替换 |
| `LPCWSTR` | 指向只读宽字符字符串的指针 | 与 `const char*` 不兼容 |
| `HRESULT` | D3D12 / DXGI 常见的状态码 | 不是 BOOL，第 4.9 节单独检查 |

句柄是一种“不需要知道内部布局的标识”。复制句柄数值不等于复制窗口或取得新的所有权；保存一个非空句柄，也不保证其对应对象永远存在。窗口销毁后就不应再用旧 HWND 操作它。[Windows 类型约定](https://learn.microsoft.com/en-us/windows/win32/learnwin32/windows-coding-conventions)

#### 字符串、宏和标志

本关统一使用带 `W` 后缀的接口，如 `CreateWindowExW`、`SetWindowTextW`，并配合 `L"A2 Window Clear"`、`std::wstring`。`W` 路线使用 Windows 的 UTF-16 宽字符串；`L` 是 C++ 宽字符串字面量前缀。不要把 `"标题"` 强转为 `LPCWSTR`，那不是编码转换。源码文件使用 UTF-8 和编译选项 `/utf-8`，与运行时 API 接收宽字符串是两件事。[字符串与 A/W 接口](https://learn.microsoft.com/en-us/windows/win32/learnwin32/working-with-strings)

附录 CMake 中四个定义的意义：`UNICODE` 让无后缀 Win32 名称选择 Unicode 版本，`_UNICODE` 对应 C 运行库的通用文本宏；`WIN32_LEAN_AND_MEAN` 减少部分 Windows 头文件内容，`NOMINMAX` 避免 Windows 的 `min/max` 宏干扰 C++。它们都不是“开启 D3D12”的开关。

```cpp
WNDCLASSW windowClass{};       // Zero-initialize before filling fields.
windowClass.lpfnWndProc = WndProc; // Function address, not WndProc().
const DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
```

`{}` 在这里把结构体成员初始化为零值，避免把未填写字段变成随机参数。`&windowClass` 是把结构体地址交给 API；API 文档的 `[in]`、`[out]`、`[in, out]` 分别说明读入、写出和两者兼有。`|` 是按位组合多个样式标志，不是逻辑或 `||`；检查某个位用 `flags & flag`。`nullptr` 表示没有指针或句柄，能否传空取决于参数约定。

#### 从头文件到程序

`#include <windows.h>` 让编译器认识类型和函数声明；链接库解决外部函数的链接；运行时再由系统 DLL 提供实现。**包含头文件不等于完成链接。** 例如窗口 API 的官方页末尾会列出 Header、Library 和 DLL；`CreateWindowExW` 对应 `Windows.h / User32.lib / User32.dll`。在 CMake 中写 `target_link_libraries(... PRIVATE user32)`。[API 的构建要求](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexw)

纯窗口练习先只显式链接 `user32`，进入图形阶段再加入 `d3d12 dxgi`。不需要把系统 DLL 下载到工程，也不需要把 Windows SDK 头文件复制到项目目录。

### 4.4 创建一个窗口，实际发生了哪些事

按下面顺序在附录 D 找到对应代码：

1. `GetModuleHandleW(nullptr)`：取得当前模块句柄。
2. 填写 `WNDCLASSW`：最重要的是窗口类名、模块句柄和 `WndProc` 地址，另设默认箭头光标。
3. `RegisterClassW`：注册这种窗口的行为描述。**窗口类不是 C++ 的 `class App`，注册它也还没有创建窗口。**
4. `AdjustWindowRect`：由期望的客户区尺寸算出包含边框、标题栏的外部尺寸。
5. `CreateWindowExW`：创建窗口实例，成功后取得 HWND。
6. `ShowWindow`：让已经创建的窗口显示出来。
7. 进入消息循环：保持程序存活并响应系统与用户。

客户区是标题栏、边框以内用于显示内容的区域；非客户区包含标题栏和边框等。我们的 `1280 × 720` 指初始客户区，不是整个窗口外框。`RECT` 是 `left / top / right / bottom` 四个边界，不是 `x / y / width / height`；宽高要用差值计算。屏幕坐标与客户区坐标也不同，后者通常以客户区左上角为 `(0, 0)`，x 向右、y 向下。[窗口创建](https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window)、[客户区与外框尺寸](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-adjustwindowrect)

把附录 B 中的一长串创建参数分组看，而不是按位置死记：

| 参数组 | 本关传值 | 用途 |
|---|---|---|
| 扩展样式 | `0` | 不开启额外窗口行为 |
| 窗口类名、标题 | 注册时的类名、`L"A2 Window Clear"` | 前者查找行为，后者显示在标题栏 |
| 普通样式 | `style` | 标题栏、系统菜单、最小化按钮；不启用缩放、最大化 |
| 位置 | `CW_USEDEFAULT, CW_USEDEFAULT` | 让系统选择初始位置 |
| 外框宽高 | 调整后的 `right-left, bottom-top` | 为期望客户区留出边框与标题栏 |
| 父/所有者窗口、菜单 | `nullptr, nullptr` | 本关是无自定义菜单的独立顶层窗口 |
| 模块、用户参数 | `instance, this` | 关联模块，并把当前 App 地址交给创建过程 |

在成员函数里 `this` 是当前 App 地址；附录 D 在 `main` 中创建，所以最后一个参数是 `&app`。两者目的相同。不要把窗口标题当窗口类名，也不要注册一个名字、创建时却传另一个名字。

高 DPI 显示缩放会影响逻辑尺寸与屏幕物理像素的关系。本关沿用固定窗口方案，不承诺截图恰好占 `1280 × 720` 个物理像素；先用 `GetClientRect` 检查程序坐标下的客户区。之后做可缩放窗口时再统一处理 DPI awareness、`WM_DPICHANGED`、尺寸调整与交换链重建，不要仅添加最大化按钮就认为已经支持它们。[DPI 与逻辑尺寸](https://learn.microsoft.com/en-us/windows/win32/learnwin32/dpi-and-device-independent-pixels)

### 4.5 回调：谁在调用 `WndProc`

回调的意思是：**你提供函数地址，Windows 在需要时调用它。** 不是自己在 `main` 中不断调用 `WndProc()`，也不是自动新建了一条线程。

```cpp
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
```

| 部分                | 你需要理解的含义                  |
| ----------------- | ------------------------- |
| `hwnd`            | 这次消息针对哪个窗口                |
| `message`         | 消息种类，例如 `WM_KEYDOWN`      |
| `wParam / lParam` | 该种消息附带的数据；必须按该消息的外部协议文档解释 |
| `LRESULT`         | 给 Windows 的处理结果，不是程序退出码   |
| `CALLBACK`        | 与系统约定匹配的函数声明形式，照接口签名保留    |

例如 `WM_KEYDOWN` 的 `wParam` 是虚拟键码，`WM_SIZE` 的 `wParam` 是尺寸变化类型，`WM_NCCREATE` 的 `lParam` 则可解释为 `CREATESTRUCTW*`。不能对所有消息都把同一个参数强转成同一种数据。

只处理自己需要的消息；其余调用 `DefWindowProcW(hwnd, message, wParam, lParam)`，保留系统默认行为。不能在函数末尾无条件 `return 0` 吞掉所有消息。返回值也不是一律为零：`WM_NCCREATE` 返回 `TRUE` 才继续创建，返回 `FALSE` 会拒绝创建；附录 B 将这条消息继续交给默认处理，附录 D 显式返回 `TRUE`。[窗口过程](https://learn.microsoft.com/en-us/windows/win32/learnwin32/writing-the-window-procedure)

普通非静态成员函数带有隐含的 `this`，不能直接当作这个回调地址。先沿用附录的自由函数 `WndProc`，或以后使用签名匹配的 `static` 成员函数，再通过窗口关联的 App 指针访问状态。无需为了这一点马上写通用窗口框架。
\\\\
回调应短小，不做无限循环、磁盘长任务或长时间等待；不要让 C++ 异常穿过 Windows 回调边界。需要失败处理时，在回调内记录错误和退出请求，再由自己的主循环处理。附录 D 的会抛异常的 `CheckWin32` 只在主流程使用，不放进 `WndProc`。

### 4.6 `WndProc` 怎样找到我的 App

`WndProc` 没有 `App*` 参数。窗口却可以保存一个由应用自己解释的指针大小的数据槽：`GWLP_USERDATA`。本关用它记住 App 地址，不需要全局单例。

```text
main 中 App app（或 App 成员函数中的 this）
  -> CreateWindowExW 的最后一个参数
  -> 创建期间 WM_NCCREATE 的 lParam 指向 CREATESTRUCTW
  -> CREATESTRUCTW::lpCreateParams 取回 App*
  -> SetWindowLongPtrW(hwnd, GWLP_USERDATA, App 地址)
  -> 后续回调 GetWindowLongPtrW -> App* -> 修改 running / clearColor
```

`static_cast<App*>(create->lpCreateParams)` 把先前传入的 `void*` 还原成 App 指针；`reinterpret_cast<LONG_PTR>(app)` 把地址放进可容纳它的整数槽，读取时反向转换。它们没有创建或复制 App，也没有延长 App 的寿命。只有在这里“放入和取回同一种对象地址”的约定下才这样转换。[窗口用户数据槽](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowlongptrw)

这里有三个必须知道的生命周期约束：

1. `App` 必须在创建窗口前存在，在窗口所有回调结束后才销毁。不要传临时局部对象地址，也不要在窗口存有指针时移动 App。
2. 创建期间就会收到消息，**`CreateWindowExW` 尚未返回时已经可能进入 `WndProc`**；此时 `app.hwnd = CreateWindowExW(...)` 这个赋值还没完成。在回调里使用参数 `hwnd`，不要假设 `app.hwnd` 或 D3D12 对象已经初始化。
3. 状态尚未关联时取回的指针可能为空，先判断再访问；关闭阶段 App 也要活到 `DestroyWindow` 返回。`GWLP_USERDATA` 不负责自动 `delete` App。[创建期间的消息](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexw)

这也说明“单线程”不等于“只会从循环顶部进入回调”：某些系统调用会同步触发回调，产生嵌套调用。暂时只需学会在断点的调用栈中辨认它，不需要写多线程代码。

### 4.7 消息循环不是渲染循环

窗口消息通常是一个编号加参数。键盘等排队消息的典型路径如下；注意并非所有消息都经过这条队列路径，创建与销毁期间还会直接发生回调。

```text
用户按键 -> 线程消息队列
  -> GetMessageW / PeekMessageW 取出 MSG
  -> TranslateMessage 处理字符转换
  -> DispatchMessageW 分派到目标窗口的 WndProc
  -> WndProc 修改应用状态并返回
  -> 主循环继续
```

`MSG` 是接收一条消息的结构体，不是消息队列本身。`TranslateMessage` 不是把英文翻译成中文，也不负责调用 `WndProc`；它可根据按键消息产生字符消息（例如 `WM_CHAR`）。`DispatchMessageW` 才负责分派。队列属于线程，所以本关取消息时 HWND 参数用 `nullptr`，取得该线程的窗口消息和线程消息。[消息模型](https://learn.microsoft.com/en-us/windows/win32/learnwin32/window-messages)

| 函数 | 没有待取的排队消息时 | 本关怎么用 |
|---|---|---|
| `GetMessageW` | 等待新消息 | 适合附录 D 的纯窗口练习，没有连续渲染任务 |
| `PeekMessageW(..., PM_REMOVE)` | 不为了新消息而一直等待；取到时移除消息 | 最终 D3D12 版本用它给逐帧渲染留出执行机会 |
| `WaitMessage` | 等待新的消息 | 最终版本最小化且消息已处理完时休眠 |

**GetMessage 的等待不是“窗口卡死”。** 无事可做时等待正是事件驱动窗口的正常状态。但若把持续渲染放在阻塞取消息之后，没有新消息就没有下一帧。因此纯窗口和最终清屏版本使用不同的循环。

`GetMessageW` 返回值分三类：`> 0` 为普通消息，`0` 表示收到 `WM_QUIT`，`-1` 表示错误。不能只写 `while (GetMessageW(...))` 而把 `-1` 当成功；附录 D 明确分支。`PeekMessageW` 返回零则表示没有取到消息，不是要用 `GetLastError` 报错的同一情形。[GetMessageW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmessagew)、[PeekMessageW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-peekmessagew)

最终循环可以先用中文伪代码理解，再对照附录 B：

```text
while 仍在运行：
    while 有消息：
        若 WM_QUIT：标记退出并停止取消息
        否则 TranslateMessage + DispatchMessageW
    若已请求退出：break，不再渲染
    若最小化：WaitMessage，然后回到循环顶部
    否则 RenderFrame
```

主循环负责“接下来做什么”，回调负责“收到这件事后改什么状态”。A2 同一线程上处理输入并录制下一帧，所以简单成员变量不需要互斥锁。拖动标题栏时系统可能进入模态消息处理，使主循环中的连续渲染暂时停顿；本关不为此引入渲染线程。

### 4.8 六类消息与关闭顺序

| 消息 | 它告诉你什么 | 本关的响应 |
|---|---|---|
| `WM_PAINT` | 客户区需要重绘 | `BeginPaint / EndPaint` 处理重绘请求；最终版不在此推进 D3D12 帧 |
| `WM_KEYDOWN` | 一个非系统按键按下 | `1 / 2 / 3` 修改状态，`VK_ESCAPE` 设置退出标志 |
| `WM_SIZE` | 尺寸或显示状态变化 | 检查 `wParam == SIZE_MINIMIZED`，记录最小化状态 |
| `WM_CLOSE` | 用户请求关闭 | 设置 `running = false`，返回 0，延迟真正销毁 |
| `WM_DESTROY` | 窗口正在销毁 | 标记停止，调用 `PostQuitMessage(0)` |
| `WM_QUIT` | 请求线程消息循环退出 | 主循环识别，不交给 `WndProc` |

键盘消息发给拥有键盘焦点的窗口。先点击实验窗口再按键；焦点在 CLion 或日志控制台时，实验窗口不会因此改色。本关用主键盘数字键 `'1' / '2' / '3'`，不是整数 `1 / 2 / 3`；数字小键盘是另一组虚拟键码，暂不处理。长按可能重复收到 `WM_KEYDOWN`，设置同一个颜色没有问题。文本输入不是本关需求，后续输入名称时再处理 `WM_CHAR` 等机制。Alt+F4 的系统按键处理交给 `DefWindowProcW`，最终产生关闭请求，不要把未处理的系统消息全部吞掉。

`WM_PAINT` 不等于“刷新率到了”。在需要重绘时，Windows 用更新区域记录需要处理的部分；`BeginPaint / EndPaint` 是一次成对的绘制处理，会验证更新区域。只收到消息就 `return 0` 而不验证区域，可能反复产生重绘请求。附录 D 使用系统画刷填充客户区以观察窗口；那是 GDI 路线的普通背景，不是 D3D12 清屏。接入 D3D12 后，改用附录 B 的 paint 处理，由交换链呈现画面。[BeginPaint](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-beginpaint)

关闭的三个名字必须分开：

```text
WM_CLOSE：请求关闭，可以推迟，不等于已经销毁
DestroyWindow(hwnd)：真正开始销毁窗口，会触发 WM_DESTROY 等消息
PostQuitMessage(0)：给当前线程留下退出消息，不负责销毁窗口或释放 GPU 资源
```

未自行处理 `WM_CLOSE` 时，默认处理会销毁窗口。A2 刻意接管它，走 `running=false → 主循环退出 → 等 GPU → 释放图形对象 → DestroyWindow`。因此附录 B/D 正常关闭时，循环可能已靠 `running` 退出，随后清理才触发 `WM_DESTROY / PostQuitMessage`；不需要再等一轮 `WM_QUIT` 才允许 `main` 返回。这与其他教程直接在 `WM_CLOSE` 中销毁窗口的设计不同，不要拼接成两套同时运行的关闭流程。[窗口关闭](https://learn.microsoft.com/en-us/windows/win32/learnwin32/closing-the-window)

`WM_SIZE` 也不是让你在本关立刻重建交换链。去掉 `WS_THICKFRAME / WS_MAXIMIZEBOX` 并禁用 DXGI Alt+Enter，是有意缩小范围；以后加入缩放时，需要等待 GPU、释放旧后缓冲引用、`ResizeBuffers` 并重建 RTV。最小化时宽高可能为零，不能直接拿去创建图形资源。

### 4.9 查错误：先看返回值约定

不要给所有 API 套同一种 `if (!result)`，也不要调用完任何函数都打印 `GetLastError()`。

| API 类别与例子 | 如何判断 | 错误从哪里来 |
|---|---|---|
| BOOL 成败型，例如 `AdjustWindowRect`、`SetWindowTextW` | 零失败，非零成功 | 失败后立即保存 `GetLastError()` |
| 句柄型，例如 `CreateWindowExW`、`CreateEventW` | 这两个 API 用 `nullptr` 表示失败 | 失败后立即保存 `GetLastError()`；别推广为所有句柄 API 都如此 |
| HRESULT 型，例如 `D3D12CreateDevice`、`Present` | `FAILED(hr)` / `SUCCEEDED(hr)` | 返回的 hr 本身，通常记录十六进制 |
| 取消息 `GetMessageW` | `>0 / 0 / -1` 三分支 | 仅 `-1` 时读取 Win32 错误 |
| 等待 `WaitForSingleObject` | 区分完成、超时、失败等状态 | 只有 `WAIT_FAILED` 时读取 Win32 错误；超时不是完成 |
| 状态型 `ShowWindow` | 返回“此前是否可见” | 首次显示返回零是正常现象，不能套 BOOL 成败检查 |

`GetLastError` 是调用线程保存的错误码，必须在文档约定的失败之后立即取得，避免后续调用改变它。它不是 HRESULT，也不保证成功调用会清零。`S_OK == 0` 在 HRESULT 中表示成功，恰好与许多 BOOL 接口的零失败相反；其他非零成功状态也存在，不能用 `hr != S_OK` 统一判错。[GetLastError](https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror)、[HRESULT 检查](https://learn.microsoft.com/en-us/windows/win32/learnwin32/error-handling-in-com)、[ShowWindow 返回值](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow)

==另一个特殊例子是 `SetWindowLongPtrW`：返回的是旧值，原来为零时成功也返回零。需要检查时，先 `SetLastError(0)`，调用后只有“返回零且 last-error 非零”才判失败；附录 D 演示了这一点。不要对它直接调用 `CheckWin32(result != 0, ...)`。==

读陌生 API 的顺序建议固定为：**用途 → 参数的输入/输出与可空性 → 返回值 → 资源归谁释放 → Remarks 的时序限制 → 头文件和链接库**。报错时保留 API 名、代码位置、原始错误码和触发操作，而不是只写“创建失败”。

### 4.10 资源归谁：句柄、COM、Event 不是同一回事

| 取得的东西 | 本关释放/结束方式 | 不要做什么 |
|---|---|---|
| `CreateWindowExW` 返回的 HWND | 创建它的线程调用 `DestroyWindow` | 不 `delete`，不 `CloseHandle(hwnd)` |
| `RegisterClassW` 注册的类 | 窗口销毁后 `UnregisterClassW` | 不先注销仍有实例的窗口类 |
| `GetModuleHandleW(nullptr)` 取得的模块句柄 | 只是借用，本关不释放 | 不 `CloseHandle`，不为这次借用调用 `FreeLibrary` |
| `LoadCursorW(nullptr, IDC_ARROW)` 的共享系统光标、`GetSysColorBrush` 的系统画刷 | 系统管理，本关不销毁 | 不当作自己创建的资源释放 |
| `CreateEventW` 创建的 Event | 使用结束且不再有等待时 `CloseHandle` | 不拿销毁窗口代替关闭 Event |
| `ComPtr<ID3D12Device>` 等接口引用 | `ComPtr` 析构或 `.Reset()` 释放引用 | 不 `delete device.Get()`，不重复手工 `Release` |

这些差异就是所有权：你是否负责结束该资源，以及用哪个 API 结束。RAII 是让 C++ 对象生命周期管理资源的习惯；`ComPtr` 对 COM 引用做了这件事，但裸 `HWND / HANDLE` 自身不会自动清理。本关先显式写清关闭顺序，后续再引入经过验证的句柄封装，不急着做大框架。

对照官方的所有权与销毁约定：[DestroyWindow](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroywindow)、[UnregisterClassW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-unregisterclassw)、[系统光标](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-loadcursorw)、[系统画刷](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getsyscolorbrush)。

#### 读懂 D3D12 中的 COM 写法

COM（Component Object Model）在这里首先意味着“通过接口调用对象，并用引用计数管理接口寿命”，不是要求你先学 COM 服务器开发。`ID3D12Device` 是接口，不能自己 `new ID3D12Device`；让创建 API 返回它。

| 代码 | 含义 |
|---|---|
| `ComPtr<ID3D12Device> device;` | 声明一个初始为空的、管理 COM 引用的智能指针 |
| `device.Get()` | 取得借用的原始接口指针传给 API，不移交所有权 |
| `IID_PPV_ARGS(&device)` | 为创建/查询 API 提供接口标识和输出位置；API 把接口指针写回来 |
| `initialSwapChain.As(&swapChain)` | 查询所需接口，检查返回 HRESULT；不是随意强转，也不是创建第二套交换链 |
| `device.Reset()` | 释放当前持有的引用并变空；不等于等待 GPU，也不等于恢复设备 |

`ComPtr` 重载了取地址运算符，`&device` 不是普通对象的简单取地址；它可能先释放旧引用。本关创建时使用空指针，别在对象仍被使用时拿同一变量反复接收输出。`GetAddressOf()` 不会先清空旧引用，`ReleaseAndGetAddressOf()` 会；后续需要重建时，先明确旧对象的所有权和 GPU 使用已结束。[ComPtr 官方接口](https://learn.microsoft.com/en-us/cpp/cppcx/wrl/comptr-class?view=msvc-170)

还要区分 `allocator.Reset()` 与 `allocator->Reset()`：前者是智能指针释放引用，后者才是调用命令分配器的 Reset 方法。一个点号和一个箭头，改变的是完全不同的操作。

#### Event 不是窗口消息

附录 B 的 `CreateEventW(nullptr, FALSE, FALSE, nullptr)` 创建一个无名、自动复位、初始未触发的同步事件。四个参数分别是默认安全属性、是否手动复位、初始是否触发、名字。它不是鼠标事件，也不会调用 `WndProc`。自动复位意味着满足一次等待后会回到未触发状态，不是每隔一段时间自动触发。[CreateEventW](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createeventw)

后续 D3D12 的 Fence 表示 GPU 进度，Event 是进度达到目标时用来唤醒 CPU 的系统对象；`WaitForSingleObject` 才是阻塞等待。主线程阻塞时也不能正常推进自己的消息循环，因此 A2 的每帧等待是易理解但低并行度的学习选择，不应在窗口回调里再加无限等待。

### 4.11 先做纯窗口，再把它接到 D3D12

**先完成附录 D。** 它是完整的独立参考程序：没有设备、交换链、Shader 或 GPU 同步，只有固定窗口、数字键改标题和正常退出。使用单独的 `a2-win32-practice` 练习目录，不覆盖已经开始写的 A2；附录 D 给出专用 CMake。

做完纯窗口后，按下面顺序迁移，不要把两个 `main` 同时放进一个目标：

| 纯窗口练习 | 接入最终 A2 时怎么改 |
|---|---|
| `App::selected` 与按键改标题 | 改成 `App::clearColor`，输入修改下一帧的清屏色 |
| `GetMessageW` 阻塞循环 | 换成附录 B 的 PeekMessage 循环，保留退出与最小化判断 |
| `WM_PAINT` 中填充普通背景 | 换成附录 B 的 BeginPaint / EndPaint 处理，不混用 GDI 背景和 D3D12 持续绘制 |
| 创建窗口后直接显示 | 创建窗口 → 初始化图形对象 → 显示；交换链创建需要有效 HWND |
| 正常退出直接销毁窗口 | 先完成第 9 节 GPU 等待和图形资源释放，再销毁窗口 |

最终结构是：

```text
main：App 活到窗口销毁之后；组织初始化、循环、关闭
InitializeWindow：只负责窗口类、尺寸、HWND
WndProc：处理输入、记录状态、请求退出
InitializeGraphics：使用 HWND 创建呈现与图形对象
主循环：处理消息 -> 根据 running/minimized 决定是否 RenderFrame
Shutdown：停止提交 -> 等 GPU -> 释放资源 -> 销毁窗口
```

### 4.12 窗口基础的验收，不靠“看起来能跑”

| 实验 | 怎么做 | 你应当观察/解释什么 |
|---|---|---|
| W1 创建顺序 | 在 `CreateWindowExW` 前、`WM_NCCREATE` 内、调用后各设断点 | 回调先于创建函数返回；查看 Call Stack，区分回调的 hwnd 参数与主流程尚未赋值的 hwnd 变量（完整 A2 中为 app.hwnd） |
| W2 指针传递 | 比较 `&app`、`lpCreateParams`、取回的 App 指针 | 它们指向同一对象；能说明对象何时可以销毁 |
| W3 尺寸 | 在附录 D 的 `GetClientRect` 后看 `client` | 初始宽 1280、高 720；外框还含标题栏和边框；不拿截图物理像素替代程序坐标 |
| W4 输入 | 同次运行点击窗口后按主键盘 `1 → 2 → 3`，再把焦点移到 CLion | 标题只随目标窗口收到的按键变化；纯窗口阶段不要求客户区改色 |
| W5 最小化 | 在 `WM_SIZE` 分支断点观察最小化和恢复 | `minimized` 切换；不是直接销毁窗口，也不应创建零尺寸缓冲 |
| W6 退出 | 分三次用关闭按钮、Esc、Alt+F4；各查看原生退出码 | 均为 0；能按实际设计解释 running、DestroyWindow、WM_DESTROY，而不是靠 IDE Stop |
| W7 错误定位 | 仅在独立练习中故意让创建使用未注册的类名，记录后恢复 | 捕获创建失败与 Win32 错误码，不进入消息循环；恢复后重新通过 W6 |
| W8 两类循环 | 对比附录 D 与附录 B 的取消息位置 | 能解释纯窗口空闲为什么等待，以及连续渲染为何用 PeekMessage |

只在对应消息分支设断点，不要在 `WndProc` 入口对每条消息都停，否则系统正常的频繁消息会让窗口看起来一直无响应。看不懂调用栈时，先找到自己的 `main / WndProc`，无需单步进入系统 DLL。

- [x] 能从空工程完成“注册 → 创建 → 显示 → 消息处理 → 关闭”，不依赖 D3D12 才能让窗口存活。
- [x] 能解释 `HWND / App*`、窗口类 / C++ 类、回调 / 主循环、客户区 / 外框的区别。
- [ ] W1～W8 已实际检查；能够区分 BOOL、HRESULT 和 GetMessage 的返回值约定。
- [ ] 能说清哪些资源需要自己释放，以及为何 ComPtr 不会替自己等待 GPU。

通过后回看第 3 节的图形对象表，再进入第 5 节。基础部分的毕业标准是：**我知道系统怎样调用我的代码，也知道这段代码中的状态和资源由谁负责。**

## 5. A2.2：设备、队列与交换链

### 初始化顺序

1. `D3D12GetDebugInterface` 成功后调用 `EnableDebugLayer`，失败直接报告，不静默跳过。
2. 创建 DXGI Factory，枚举高性能适配器，明确选择 RTX 5070 Ti 硬件设备。
3. 将实际选中的 Adapter 传给 `D3D12CreateDevice`，最低请求 `D3D_FEATURE_LEVEL_12_0`。
4. 创建设备的 `ID3D12InfoQueue` 诊断通道。
5. 创建 `D3D12_COMMAND_LIST_TYPE_DIRECT` 类型的命令队列。
6. 使用该队列和窗口句柄创建交换链，取得 `IDXGISwapChain3`。

严格沿用 A1 的显卡与调试层策略。`HIGH_PERFORMANCE` 是枚举偏好，不是“必然选到 5070 Ti”的保证；最终仍检查适配器描述。启用调试层必须在创建设备前。[初始化顺序](https://learn.microsoft.com/en-us/windows/win32/direct3d12/creating-a-basic-direct3d-12-component)

### 交换链参数

| 字段                 | 本关取值                              | 原因                  |
| ------------------ | --------------------------------- | ------------------- |
| `Width / Height`   | `1280 / 720`                      | 固定初始客户区大小           |
| `Format`           | `DXGI_FORMAT_R8G8B8A8_UNORM`      | 普通 RGBA 颜色缓冲        |
| `BufferCount`      | `2`                               | 双缓冲                 |
| `BufferUsage`      | `DXGI_USAGE_RENDER_TARGET_OUTPUT` | 将缓冲用作渲染目标           |
| `SwapEffect`       | `DXGI_SWAP_EFFECT_FLIP_DISCARD`   | Flip 模型，本关每帧重写完整画面  |
| `SampleDesc.Count` | `1`                               | 不使用 MSAA            |
| `Scaling`          | `DXGI_SCALING_STRETCH`            | 明确默认呈现缩放策略，本关不动态改尺寸 |
| `AlphaMode`        | `DXGI_ALPHA_MODE_UNSPECIFIED`     | 普通 HWND 窗口，不实现透明合成  |
| `Flags`            | `0`                               | 不启用 tearing 或额外模式   |

**容易误用的参数：** `CreateSwapChainForHwnd` 第一个参数虽然叫 `pDevice`，D3D12 路径必须传 **Direct Command Queue**，不是 `ID3D12Device*`。[官方参数说明](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgifactory2-createswapchainforhwnd)

**验收：** 日志来自实际参与设备创建的 Adapter；窗口标题中的 Debug Layer 标记只在严格初始化成功后设置。不能靠写死标题证明设备或调试层正确。

## 6. A2.3：后缓冲与 RTV 一一对应

初始化时完成：

1. 创建容量为 `2`、类型为 `D3D12_DESCRIPTOR_HEAP_TYPE_RTV`、Flags 为 `NONE` 的描述符堆。
2. 用 `GetDescriptorHandleIncrementSize(RTV)` 查询本设备的描述符步长。
3. 对索引 `0` 和 `1` 分别调用 `swapChain->GetBuffer`，取得两个 `ID3D12Resource`。
4. 在堆中相应位置调用 `CreateRenderTargetView`，建立 RTV 与资源的对应关系。

本关第 i 个句柄的位置为：

```text
RTV(i).ptr = RTV 堆起始 CPU 句柄.ptr + i * rtvIncrement
RTV(0) -> backBuffers[0]
RTV(1) -> backBuffers[1]
```

不能写死步长，也不能把 `sizeof(D3D12_CPU_DESCRIPTOR_HANDLE)` 当作描述符大小；句柄结构的大小与堆中描述符间距不是同一概念。不要解引用句柄的 `ptr` 去读写描述符内容。[描述符步长 API](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-getdescriptorhandleincrementsize)

每帧从 `GetCurrentBackBufferIndex()` 取得索引，同时用它选择 **资源和 RTV**。不要一边操作 `backBuffers[0]`，一边清除 `RTV(1)`，也不要用 Fence 值取模猜测当前缓冲。

**验收：** 在断点处确认两个资源均有效，能指出当前索引对应的资源和描述符。两个 RTV 不是每帧重新创建，而是初始化时创建、每帧选择。

## 7. A2.4：核心链路，录制、提交、等待

### 7.1 本关的同步模型

创建一个 Direct 命令分配器、一个 Direct 命令列表、一个初值为 `0` 的 Fence，以及一个自动复位、初始未触发的 Win32 Event；下一次要发出的 Fence 值从 `1` 开始。

使用普通 `CreateCommandList` 创建的列表最初处于录制状态。初始化后先 `Close()`，以后每帧再 `Reset()` 开始新一轮录制。纯清屏不需要自定义 PSO，创建和 Reset 的 PSO 参数均可为 `nullptr`。[命令列表创建](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createcommandlist)、[微软 HelloWindow 参考实现](https://github.com/microsoft/DirectX-Graphics-Samples/blob/213dd4fd4918ea009dd8f35adee1aff1f2ecaba4/Samples/Desktop/D3D12HelloWorld/src/HelloWindow/D3D12HelloWindow.cpp)

```text
CPU 每帧：
  处理输入，得到 clearColor
  -> 查询当前 back-buffer index
  -> allocator.Reset（上一帧已等待完成；第一帧尚无 GPU 使用）
  -> commandList.Reset
  -> 录制 PRESENT -> RENDER_TARGET 屏障
  -> 录制 ClearRenderTargetView
  -> 录制 RENDER_TARGET -> PRESENT 屏障
  -> commandList.Close
  -> queue.ExecuteCommandLists
  -> swapChain.Present(1, 0)
  -> queue.Signal(fence, 本次目标值)
  -> 如未完成，注册事件并阻塞 CPU
  -> 确认完成，下一帧才复用 allocator

GPU 队列：
  转为渲染目标 -> 清屏 -> 转回呈现状态 -> ... -> 更新 Fence
```

### 7.2 录制：`App::RecordCommands`

这里只把命令写入列表，不直接让 GPU 执行。其关键行为是：

| 位置 | 调用 | 目的 |
|---|---|---|
| 开始录制前 | `allocator->Reset()` | 复用上次的命令存储，前提是 GPU 已用完 |
| 开始录制 | `list->Reset(allocator.Get(), nullptr)` | 将已关闭的列表置回录制状态 |
| 清屏前 | `ResourceBarrier(PRESENT -> RENDER_TARGET)` | 声明当前后缓冲改作渲染目标 |
| 清屏 | `ClearRenderTargetView(Rtv(index), clearColor.data(), 0, nullptr)` | 录制对整个目标的清除 |
| 清屏后 | `ResourceBarrier(RENDER_TARGET -> PRESENT)` | 为后续呈现准备资源状态 |
| 结束录制 | `list->Close()` | 形成可以提交的已关闭列表 |

交换链后缓冲初始可按 `PRESENT` 使用；每帧末也转回该状态。这里固定使用传统 Transition Barrier，不混用 Enhanced Barriers。`StateBefore` 必须与程序追踪的实际状态一致。屏障不是 CPU 等待，不能代替 Fence。[资源状态与屏障](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12)

`ClearRenderTargetView` 直接接收目标 RTV 和四个颜色分量。`0, nullptr` 表示清除整个视图；目标必须处于 `RENDER_TARGET` 状态。本关不执行 Draw，所以不需要先绑定 viewport、scissor、Root Signature 或 `OMSetRenderTargets`。[清屏 API](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-clearrendertargetview)

### 7.3 提交与呈现：`App::RenderFrame`

- **提交点：** `queue->ExecuteCommandLists(...)`。
- **呈现点：** `swapChain->Present(1, 0)`，SyncInterval 为 1，按刷新同步呈现，不开启 tearing。
- **随后的同步点：** 调用 `WaitForGpu()`，确保下一帧复用分配器之前，本帧 GPU 工作已经完成。

`ExecuteCommandLists` 没有 HRESULT 返回值；要检查列表 `Close` 的结果并读取 Debug Layer。`Present` 有 HRESULT，必须检查；它可能受显示节奏影响而阻塞，但不能拿这种阻塞代替显式 Fence 完成条件。[命令提交规则](https://learn.microsoft.com/en-us/windows/win32/direct3d12/executing-and-synchronizing-command-lists)、[Present](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present)

### 7.4 等待：`App::WaitForGpu`

必须能逐句解释这四件事：

1. 保存本次目标值，例如 `target = 7`，对同一条队列调用 `queue->Signal(fence, target)`。
2. CPU 上把下一次值递增为 `8`，但本次继续等待保存下来的 `7`，不是等待尚未提交的 `8`。
3. 查询 `GetCompletedValue()`；未达到目标时，使用非空 Event 调用 `SetEventOnCompletion(target, event)` 注册完成通知。
4. `WaitForSingleObject(event, ...)` 才是实际阻塞 CPU 的位置；唤醒后再次检查设备状态和 Fence 进度。

`queue->Signal` 在 GPU 队列执行到相应位置时更新 Fence；不要换成 CPU 侧的 `fence->Signal` 伪造 GPU 完成。非空事件版本的 `SetEventOnCompletion` 负责注册通知，本身不等于等待。[队列 Signal](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandqueue-signal)、[完成事件](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-seteventoncompletion)

`GetCompletedValue()` 在设备移除时可能返回 `UINT64_MAX`，不能简单判断“大于目标，所以成功”。附录显式识别这一情况，并为等待设置 5 秒失败出口；超时是诊断失败，不是完成，也不能据此继续 Reset。5 秒只是本关选择的诊断阈值，调试暂停等情况也可能触发，不等于已证明驱动损坏。[Fence 完成值](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-getcompletedvalue)

**重要区分：** 命令列表 Reset 与命令分配器 Reset 的约束不同。列表可以在满足其 API 条件时开始重新录制；分配器的存储仍可能被 GPU 读取，必须等相关执行结束才可重用。A2 统一每帧等待是为了简化，不要把“列表和分配器永远都必须一起等到 GPU 空闲”背成通用规则。[Allocator Reset](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandallocator-reset)、[Command List Reset](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-reset)

**验收：** 正常运行时观察帧计数持续增长；下一轮 `allocator->Reset` 之前，上一次 Fence 目标已完成。不要仅凭一个纯色截图证明“持续清屏”。

## 8. A2.5：改色为什么能在运行时生效

颜色存放在 `App::clearColor`，而不是函数里永远不变的局部常量。附录规定：

| 按键 | RGBA | 预期画面 |
|---|---|---|
| `1` | `(0.08, 0.20, 0.36, 1.0)` | 蓝色 |
| `2` | `(0.10, 0.55, 0.22, 1.0)` | 绿色 |
| `3` | `(0.65, 0.12, 0.18, 1.0)` | 红色 |

数据流：

```text
WM_KEYDOWN
  -> 修改 App::clearColor
  -> 下一次 RecordCommands 读取这四个数值
  -> 新列表记录新的清屏参数
  -> ExecuteCommandLists / Present
  -> 新颜色显示
```

这里消息处理和命令录制都在同一个线程，不需要互斥锁。改色不需要重建设备、交换链或 RTV，也不需要 Shader 编译。

CPU 修改颜色不会追溯修改已经录好的命令。A2 每帧重新录制，所以下一帧能取得新值；如果只在初始化时录一次清屏命令并反复执行，单改变量不足以达到目标。

### 三个断点实验

1. 在 `WndProc` 的按键分支断下，确认 `clearColor` 被修改。
2. 单步到 `RecordCommands` 的清屏调用，确认传入的是新值；此时只是录制，不能要求显示器已经变色。
3. 恢复运行，经提交与呈现后观察颜色，再在下一帧入口检查计数。

不要长时间在 GPU 等待相关位置反复暂停，以免干扰正常显示和诊断阈值。先用短暂断点认识顺序，再恢复运行做视觉验收。

**验收：** 一次运行中按 `1 -> 2 -> 3 -> 1`，每次颜色都变化；窗口句柄和设备没有被重建。仅编辑源码、重新编译后看到新色，不算本项通过。

## 9. A2.6：关闭与错误处理

### 正常关闭顺序

```text
关闭请求 / Esc / Alt+F4
  -> running = false
  -> 不再提交新帧
  -> WaitForGpu，确认已提交工作完成
  -> 检查剩余调试消息
  -> 释放列表、分配器、RTV 堆、后缓冲引用、交换链等图形对象
  -> 关闭 Fence Event
  -> DestroyWindow
  -> 退出码 0
```

附录让 `WM_CLOSE` 延迟真正销毁窗口，直到 GPU 等待和图形资源释放完成。`ComPtr` 管理 COM 引用计数，但不能替代 GPU 同步；C++ 对象离开作用域不代表 GPU 已停止访问资源。

每帧已经等待，为什么退出时还调用一次等待？这是明确的关闭边界，保证进入释放流程前已排空本队列之前的工作；后续增加初始化上传或其他提交时，也更容易维持这个约定。

### 错误不能伪装成成功

- 所有返回 HRESULT 的关键创建、Reset、Close、Present、Signal 和事件注册操作都检查结果。
- `CreateEventW`、窗口创建、等待返回值等 Win32 操作单独检查，不能把 BOOL 或 HANDLE 当 HRESULT。
- 日志保留 API 名称和十六进制错误码；设备相关故障额外读取 `GetDeviceRemovedReason`。
- 从 `ID3D12InfoQueue` 输出 WARNING 及以上消息；出现它们后停止本次验收，不通过过滤掉错误来“修复”。
- 正常路径释放句柄与图形对象。设备丢失、等待超时等致命错误下，附录选择记录日志并结束进程，不在同步状态未知时尝试继续渲染或把清理记成成功。

附录的 `ExitProcess(1)` 仅用于这个独立学习程序的致命错误出口，由系统回收进程资源；它不是可复用引擎的设备恢复或资源管理方案。正常关闭不走这个出口。也不要用 CLion 的强制 Stop 按钮验收正常关闭。

**验收：** 三种关闭方式均成功；最小化再恢复后继续清屏；记录原生退出码和调试消息，而不只是“窗口消失了”。

## 10. 常见故障定位

| 现象 | 首先检查 | 不要采取的绕过方式 |
|---|---|---|
| 窗口一闪就结束 | main 是否直接 return；是否进入消息循环 | 不用末尾无限 Sleep 代替循环 |
| 窗口无响应或关不掉 | 是否分派消息；WndProc 是否被长循环阻塞；是否保留默认处理 | 不先添加线程掩盖错误 |
| 创建窗口失败或创建期间崩溃 | 类名是否一致；WM_NCCREATE 的返回值、App 指针与寿命 | 不在创建回调里访问尚未初始化的图形对象 |
| 窗口 API 出现未解析外部符号 | 报错处于链接阶段；user32 链接依赖、函数定义与签名 | 不重装显卡驱动 |
| char 字符串不能传给 W 接口 | 使用宽字符串字面量与 std::wstring | 不用指针强转冒充编码转换 |
| 窗口空白，帧数不增长 | 是否真正进入 RenderFrame；是否把渲染只写在 WM_PAINT 中 | 不先添加 Shader |
| 画面交替出现旧色或异常色 | 当前索引、资源和 RTV 是否一致；是否每帧重录 | 不把所有帧强行固定到 buffer 0 |
| 修改变量但颜色没变 | 消息是否到达本窗口；Clear 使用的是不是成员颜色；是否再次提交 | 不靠重启程序代替运行时验收 |
| `CreateSwapChainForHwnd` 失败 | 第一个参数是否为 Direct Queue；HWND、格式、采样数与缓冲数 | 不传设备指针碰运气 |
| Reset 提示分配器仍在使用 | 上一帧 Fence 是否完成，是否等待错目标值 | 不加 `Sleep(16)` 当同步 |
| 清屏报告资源状态错误 | 前后两道屏障是否作用于当前资源，Before/After 是否正确 | 不关闭 Debug Layer |
| 首帧 Command List Reset 失败 | 创建后的列表是否先 Close；上次 Close 是否成功 | 不忽略 Close 的错误码 |
| Fence 等待卡住或超时 | Signal 是否在同一队列；目标值是否已发出；设备是否移除 | 不将超时当作成功后继续 Reset |
| 退出时有资源仍在使用的错误 | 是否停止提交并完成最后一次等待 | 不先释放资源再等待 |
| 最小化时 CPU 占用异常 | 是否暂停渲染并等待窗口消息；是否对遮挡状态做节流 | 不忙轮询空消息队列 |
| `0x887A002D` | 回到 A1 检查系统 Graphics Tools 与当前运行时路径 | 不静默禁用调试层 |
| 找不到 `WinMain` 或入口链接失败 | 本文使用控制台 `main`，CMake 不要加 `WIN32` 子系统参数 | 不随意混用 main / wWinMain |

`DXGI_STATUS_OCCLUDED` 是状态码而非 FAILED HRESULT。附录在完全遮挡时短暂休眠以节流；这段 Sleep 与 GPU 正确性无关，Fence 等待仍然保留。[Present 返回状态](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present)

## 11. 能指出三处代码，才算理解主线

| 问题 | 自己应能指出的位置 | 应能说出的理由 |
|---|---|---|
| 命令在哪里录制？ | `App::RecordCommands` | Reset 后把屏障和清屏写入命令列表，Close 结束录制 |
| 命令在哪里提交？ | `App::RenderFrame` 中的 `ExecuteCommandLists` | 将已关闭的命令列表交给 Direct Queue |
| CPU 在哪里等待？ | `App::WaitForGpu` 中的 `WaitForSingleObject` | 仅在 Fence 未完成时阻塞，完成后才安全重用分配器 |
| Queue Signal 返回就执行完了吗？ | `Signal` 后的完成值检查 | 返回说明信号已安排，不等于 GPU 已到该位置 |
| Present 能否代替 Fence？ | `Present` 后仍调用 `WaitForGpu` | 呈现与资源复用的同步判断是不同职责 |
| Barrier 能否代替 Fence？ | 录制函数与等待函数对比 | 前者处理 GPU 资源使用状态，后者让 CPU 判断执行进度 |
| RTV 和 Back Buffer 谁保存像素？ | `rtvHeap` 与 `backBuffers` | 后缓冲资源保存图像，RTV 是访问它的描述符 |
| 为什么不需要 HLSL？ | 清屏调用，无 Draw | 本关执行清屏命令，不执行自己编写的着色器绘制 |
| 为什么本关只用一个 allocator？ | 每帧提交后立即等待 | 没有让它的两次 GPU 使用重叠；不代表高性能项目也这样设计 |

推荐口头复述：**CPU 录制并关闭列表，队列提交给 GPU，交换链负责呈现；我用队列上的 Fence 标记完成位置，确认完成后才重用命令存储。**

## 12. A2.7：最终验收与证据

### 操作测试

| 用例 | 操作 | 通过条件 |
|---|---|---|
| T0 窗口基础 | 完成第 4.12 节与附录 D，再接入图形代码 | 能独立调试窗口生命周期，不把所有窗口问题归给 D3D12 |
| T1 启动 | CLion Debug 启动自己的目标 | 5070 Ti、调试层启用、窗口出现，非官方样例 exe |
| T2 持续清屏 | 保持窗口可见运行 60 秒 | 标题帧数持续增长，无闪烁和未解决调试消息 |
| T3 即时改色 | 同一次运行按 `1 -> 2 -> 3 -> 1` | 无需编译或重启，各颜色在后续刷新中出现 |
| T4 最小化恢复 | 最小化 5 秒后恢复 | 最小化期间不忙渲染；恢复后帧数与改色继续工作 |
| T5 正常关闭 | 分三次启动，分别用关闭按钮、Esc、Alt+F4 | 三次均退出码 0；不靠 IDE Stop 或结束进程 |
| T6 追踪顺序 | 对照断点、函数名和 Fence 目标值 | 能指出录制、提交、等待三处，并说明 Reset 前提 |
| T7 调试层 | 查看启动、运行和退出期间消息 | 无未解决 WARNING / ERROR / CORRUPTION；故障保留第一条完整消息 |

帧数是成功完成本关提交与同步循环的次数，不是显示器实际扫描帧数，也不是性能基准。单张截图不能证明持续运行或运行时改色，T2/T3 最好保留短录屏或多时刻记录。

### 最终勾选

- [ ] G0：完成 W1～W8，能解释窗口创建、回调状态、消息循环、错误检查与资源所有权。
- [ ] G1：独立 A2 工程以 MSVC / x64 / Debug 构建，未覆盖 A1。
- [ ] G2：实际设备为 RTX 5070 Ti；设备创建前严格启用 Debug Layer。
- [ ] G3：自己的窗口持续清屏至少 60 秒，有帧数增长证据。
- [ ] G4：运行时三色切换成功，不需要重启或重新构建。
- [ ] G5：正常启动和三种关闭路径通过，原生退出码均为 0。
- [ ] G6：能指出录制、提交、等待的具体函数及 API。
- [ ] G7：能解释 RTV / Resource、Allocator / List、Barrier / Fence 的区别。
- [ ] G8：无未解决的调试层警告或错误，能说明单分配器每帧等待的限制。

证据建议保留在现有 `F:\GameDevelop\SomeProjects\GPUVisibilityLab\docs\CheckPoints` 中，文件名前缀使用 `A2-`，不要覆盖 A1 截图。

```text
日期：
CLion / MSVC / Windows SDK 版本：
自己的工程路径与目标名称：
纯 Win32 练习 W1～W8 的结果与未解问题：
WM_NCCREATE 时观察到的 App 地址与调用栈：
实际 Adapter / Debug Layer 状态：
已通过的 G 编号：
60 秒前后帧数：
运行时改色证据：
关闭按钮 / Esc / Alt+F4 的退出码：
录制位置：
提交位置：
等待位置：
本次等待的 Fence 目标值与完成值：
第一条有效调试消息 / HRESULT：
证据位置：
尚未解决的问题：
```

通过 A2 后再进入后续三角形绘制。此时保留窗口、交换链、命令提交与同步主线，再增加 Shader、Root Signature、PSO 和顶点数据，不必重写整套初始化。

## 附录 A：完整 CMakeLists.txt

使用控制台 `main`，保留控制台日志；**不要**给 `add_executable` 加 `WIN32`。此项目只有 C++，不添加 HLSL 编译步骤。

```cmake
cmake_minimum_required(VERSION 3.24)
project(A2WindowClear LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "A2 requires Windows and MSVC.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Select the amd64/x64 toolchain for A2.")
endif()

add_executable(a2_window_clear main.cpp)
target_compile_features(a2_window_clear PRIVATE cxx_std_20)
target_compile_definitions(a2_window_clear PRIVATE
    WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_compile_options(a2_window_clear PRIVATE /W4 /permissive- /utf-8)
target_link_libraries(a2_window_clear PRIVATE d3d12 dxgi user32)
```

## 附录 B：完整 main.cpp 参考实现

以下为一整个文件，没有省略号，也不依赖未给出的辅助头文件。完成第 4 节和附录 D 后，先对照窗口代码与主循环的变化，再重点阅读 `InitializeGraphics`、`RecordCommands`、`RenderFrame`、`WaitForGpu` 和 `Shutdown`；Win32 代码为它们提供生命周期与输入。

参考实现使用传统 D3D12 基础接口。严格选择本机目标显卡是 A1/A2 的学习约定，不是跨机器产品的通用选卡策略。

```cpp
#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

std::string Hex(HRESULT result) {
    std::ostringstream text;
    text << "0x" << std::hex << std::uppercase << std::setfill('0')
         << std::setw(8) << static_cast<std::uint32_t>(result);
    return text.str();
}

void Check(HRESULT result, const char* operation) {
    if (FAILED(result)) {
        throw std::runtime_error(std::string(operation) + ": " + Hex(result));
    }
}

void CheckWin32(BOOL success, const char* operation) {
    if (!success) {
        const DWORD error = GetLastError();
        throw std::runtime_error(std::string(operation) + ": Win32 "
                                 + std::to_string(error));
    }
}

struct App {
    static constexpr UINT BufferCount = 2;
    static constexpr UINT Width = 1280;
    static constexpr UINT Height = 720;

    HWND hwnd = nullptr;
    bool running = true;
    bool minimized = false;
    std::array<float, 4> clearColor{0.08f, 0.20f, 0.36f, 1.0f};
    UINT64 frames = 0;
    std::wstring adapterName;

    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12InfoQueue> infoQueue;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain3> swapChain;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    std::array<ComPtr<ID3D12Resource>, BufferCount> backBuffers;
    UINT rtvIncrement = 0;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> fence;
    HANDLE fenceEvent = nullptr;
    UINT64 nextFenceValue = 1;

    void InitializeWindow();
    void InitializeGraphics();
    D3D12_CPU_DESCRIPTOR_HANDLE Rtv(UINT index) const;
    void RecordCommands(UINT index);
    void RenderFrame();
    void WaitForGpu();
    void CheckDebugMessages();
    void UpdateTitle();
    void Shutdown();
};

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }

    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        BeginPaint(hwnd, &paint);
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_SIZE:
        if (app) app->minimized = (wParam == SIZE_MINIMIZED);
        return 0;
    case WM_KEYDOWN:
        if (app) {
            if (wParam == '1') app->clearColor = {0.08f, 0.20f, 0.36f, 1.0f};
            if (wParam == '2') app->clearColor = {0.10f, 0.55f, 0.22f, 1.0f};
            if (wParam == '3') app->clearColor = {0.65f, 0.12f, 0.18f, 1.0f};
            if (wParam == VK_ESCAPE) app->running = false;
        }
        return 0;
    case WM_CLOSE:
        if (app) {
            app->running = false;
            return 0;
        }
        break;
    case WM_DESTROY:
        if (app) app->running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void App::InitializeWindow() {
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    CheckWin32(instance != nullptr, "GetModuleHandleW");
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = L"A2WindowClearClass";
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    CheckWin32(windowClass.hCursor != nullptr, "LoadCursorW");
    CheckWin32(RegisterClassW(&windowClass) != 0, "RegisterClassW");

    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT rect{0, 0, static_cast<LONG>(Width), static_cast<LONG>(Height)};
    CheckWin32(AdjustWindowRect(&rect, style, FALSE), "AdjustWindowRect");
    hwnd = CreateWindowExW(0, windowClass.lpszClassName, L"A2 Window Clear",
                          style, CW_USEDEFAULT, CW_USEDEFAULT,
                          rect.right - rect.left, rect.bottom - rect.top,
                          nullptr, nullptr, instance, this);
    CheckWin32(hwnd != nullptr, "CreateWindowExW");
}

void App::InitializeGraphics() {
	// 创建Debug层
    ComPtr<ID3D12Debug> debug;
    Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)), "D3D12GetDebugInterface");
    debug->EnableDebugLayer();

	// 创建设备
    ComPtr<IDXGIFactory6> factory;
    Check(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)),
          "CreateDXGIFactory2");
    ComPtr<IDXGIAdapter1> selected;
    for (UINT index = 0;; ++index) {
        ComPtr<IDXGIAdapter1> candidate;
        const HRESULT result = factory->EnumAdapterByGpuPreference(
            index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&candidate));
        if (result == DXGI_ERROR_NOT_FOUND) break;
        Check(result, "EnumAdapterByGpuPreference");
        DXGI_ADAPTER_DESC1 desc{};
        Check(candidate->GetDesc1(&desc), "GetDesc1");
        const std::wstring name(desc.Description);
        if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) && desc.VendorId == 0x10DE
            && name.find(L"RTX 5070 Ti") != std::wstring::npos) {
            selected = candidate;
            adapterName = name;
            break;
        }
    }
    if (!selected) throw std::runtime_error("RTX 5070 Ti not found; no fallback.");
    Check(D3D12CreateDevice(selected.Get(), D3D_FEATURE_LEVEL_12_0,
                           IID_PPV_ARGS(&device)), "D3D12CreateDevice");
    std::wcout << L"[INFO] Selected adapter: " << adapterName << L'\n';
    std::cout << "[INFO] Debug Layer enabled before device creation.\n";
    Check(device.As(&infoQueue), "Query ID3D12InfoQueue");
    Check(infoQueue->PushEmptyStorageFilter(), "PushEmptyStorageFilter");
    Check(infoQueue->PushEmptyRetrievalFilter(), "PushEmptyRetrievalFilter");
	
	// 创建命令队列
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    Check(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)),
          "CreateCommandQueue");

	// 创建交换链
    DXGI_SWAP_CHAIN_DESC1 swapDesc{};
    swapDesc.Width = Width;
    swapDesc.Height = Height;
    swapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapDesc.SampleDesc.Count = 1;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.BufferCount = BufferCount;
    swapDesc.Scaling = DXGI_SCALING_STRETCH;
    swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    ComPtr<IDXGISwapChain1> initialSwapChain;
    Check(factory->CreateSwapChainForHwnd(queue.Get(), hwnd, &swapDesc,
          nullptr, nullptr, &initialSwapChain), "CreateSwapChainForHwnd");
    Check(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER),
          "MakeWindowAssociation");
    Check(initialSwapChain.As(&swapChain), "Query IDXGISwapChain3");

	// 创建渲染目标视图RTV 描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NumDescriptors = BufferCount;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    Check(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeap)),
          "CreateDescriptorHeap RTV");
    rtvIncrement = device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (UINT index = 0; index < BufferCount; ++index) {
        Check(swapChain->GetBuffer(index, IID_PPV_ARGS(&backBuffers[index])),
              "GetBuffer");
        device->CreateRenderTargetView(backBuffers[index].Get(), nullptr, Rtv(index));
    }

    Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
          IID_PPV_ARGS(&allocator)), "CreateCommandAllocator");
    Check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
          allocator.Get(), nullptr, IID_PPV_ARGS(&list)), "CreateCommandList");
    Check(list->Close(), "Initial command list Close");
    Check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
          "CreateFence");
    fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    CheckWin32(fenceEvent != nullptr, "CreateEventW");
    CheckDebugMessages();
    UpdateTitle();
}

D3D12_CPU_DESCRIPTOR_HANDLE App::Rtv(UINT index) const {
    auto handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * rtvIncrement;
    return handle;
}

void App::RecordCommands(UINT index) {
    // The previous frame completed before this allocator is reused.
    Check(allocator->Reset(), "Command allocator Reset");
    Check(list->Reset(allocator.Get(), nullptr), "Command list Reset");

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = backBuffers[index].Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    list->ResourceBarrier(1, &barrier);

    // RECORD: this records the current color; it does not submit GPU work.
    list->ClearRenderTargetView(Rtv(index), clearColor.data(), 0, nullptr);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    list->ResourceBarrier(1, &barrier);
    Check(list->Close(), "Command list Close");
}

void App::RenderFrame() {
    const UINT index = swapChain->GetCurrentBackBufferIndex();
    RecordCommands(index);
    ID3D12CommandList* submitted[] = {list.Get()};
    // SUBMIT: queue execution is asynchronous with respect to the CPU.
    queue->ExecuteCommandLists(1, submitted);
    const HRESULT presentResult = swapChain->Present(1, 0);
    Check(presentResult, "Present");
    WaitForGpu();
    CheckDebugMessages();
    ++frames;
    if (frames % 60 == 0) UpdateTitle();
    if (presentResult == DXGI_STATUS_OCCLUDED) Sleep(50);
}

void App::WaitForGpu() {
    const UINT64 target = nextFenceValue;
    Check(queue->Signal(fence.Get(), target), "Queue Signal");
    ++nextFenceValue;
    const auto completed = [this]() {
        const UINT64 value = fence->GetCompletedValue();
        if (value == std::numeric_limits<UINT64>::max()) {
            throw std::runtime_error("Device removed: "
                                     + Hex(device->GetDeviceRemovedReason()));
        }
        return value;
    };
    if (completed() < target) {
        Check(fence->SetEventOnCompletion(target, fenceEvent), "SetEventOnCompletion");
        // WAIT: the CPU blocks here only when this fence target is unfinished.
        const DWORD result = WaitForSingleObject(fenceEvent, 5000);
        if (result == WAIT_FAILED) CheckWin32(FALSE, "WaitForSingleObject");
        if (result != WAIT_OBJECT_0) {
            throw std::runtime_error("GPU wait timed out; device status: "
                                     + Hex(device->GetDeviceRemovedReason()));
        }
        if (completed() < target) {
            throw std::runtime_error("Fence event woke before target completion.");
        }
    }
}

void App::CheckDebugMessages() {
    if (!infoQueue) return;
    bool problem = false;
    const UINT64 count = infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
    for (UINT64 index = 0; index < count; ++index) {
        SIZE_T size = 0;
        Check(infoQueue->GetMessage(index, nullptr, &size), "GetMessage size");
        std::vector<std::uint64_t> storage(
            (size + sizeof(std::uint64_t) - 1) / sizeof(std::uint64_t));
        auto* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
        Check(infoQueue->GetMessage(index, message, &size), "GetMessage data");
        if (message->Severity <= D3D12_MESSAGE_SEVERITY_WARNING) {
            std::cerr << "[D3D12] " << message->pDescription << '\n';
            problem = true;
        }
    }
    infoQueue->ClearStoredMessages();
    if (problem) throw std::runtime_error("Unresolved D3D12 validation messages.");
}

void App::UpdateTitle() {
    const std::wstring title = L"A2 | " + adapterName
        + L" | Debug Layer ON | Frames: " + std::to_wstring(frames);
    CheckWin32(SetWindowTextW(hwnd, title.c_str()), "SetWindowTextW");
}

void App::Shutdown() {
    WaitForGpu();
    CheckDebugMessages();
    list.Reset();
    allocator.Reset();
    rtvHeap.Reset();
    for (auto& buffer : backBuffers) buffer.Reset();
    swapChain.Reset();
    queue.Reset();
    fence.Reset();
    CheckWin32(CloseHandle(fenceEvent), "CloseHandle fence event");
    fenceEvent = nullptr;
    CheckDebugMessages();
    infoQueue.Reset();
    device.Reset();
    if (hwnd && IsWindow(hwnd)) CheckWin32(DestroyWindow(hwnd), "DestroyWindow");
    hwnd = nullptr;
    CheckWin32(UnregisterClassW(L"A2WindowClearClass", GetModuleHandleW(nullptr)),
               "UnregisterClassW");
}

int main() {
#if !defined(_DEBUG)
    std::cerr << "[FAIL] A2 requires a Debug build.\n";
    return 1;
#endif
    // Keep ownership outside try: fatal errors use process teardown, not unwinding App.
    App app;
    try {
        app.InitializeWindow();
        app.InitializeGraphics();
        ShowWindow(app.hwnd, SW_SHOW);
        MSG message{};
        while (app.running) {
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT) {
                    app.running = false;
                    break;
                }
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            if (!app.running) break;
            if (app.minimized) {
                CheckWin32(WaitMessage(), "WaitMessage");
                continue;
            }
            app.RenderFrame();
        }
        app.Shutdown();
        std::cout << "[INFO] Normal shutdown. Completed frames: " << app.frames << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        if (app.device) {
            std::cerr << "[INFO] Device status: "
                      << Hex(app.device->GetDeviceRemovedReason()) << '\n';
        }
        try {
            app.CheckDebugMessages();
        } catch (const std::exception& diagnostic) {
            std::cerr << "[DIAGNOSTIC] " << diagnostic.what() << '\n';
        }
        std::cerr << "[FAIL] Fatal exit; this run is not an A2 pass.\n" << std::flush;
        ExitProcess(1);
    }
}
```

## 附录 C：构建与检查说明

在 CLion 中使用 A2-Debug Profile 构建 `a2_window_clear`，随后 Run 或 Debug。窗口必须先获得输入焦点，数字键才会触发改色。正常关闭后查看控制台原生退出码。

若要在 A1 已介绍的 Developer PowerShell 环境中排查构建，使用同一条 CMake 路线，将源目录与构建目录换成 A2；不要复用 A1 的构建缓存：

```powershell
& 'E:\Applications\JetBrains\CLion\bin\cmake\win\x64\bin\cmake.exe' -S 'F:\GameDevelop\SomeProjects\GPUVisibilityLab\a2-window-clear' -B 'F:\GameDevelop\SomeProjects\GPUVisibilityLab\a2-window-clear\cmake-build-debug' -G Ninja '-DCMAKE_BUILD_TYPE=Debug' '-DCMAKE_MAKE_PROGRAM=E:/Applications/JetBrains/CLion/bin/ninja/win/x64/ninja.exe'
```

```powershell
& 'E:\Applications\JetBrains\CLion\bin\cmake\win\x64\bin\cmake.exe' --build 'F:\GameDevelop\SomeProjects\GPUVisibilityLab\a2-window-clear\cmake-build-debug'
```

### 本文交付验证记录

- 已对照本地官方 HelloWindow 参考实现与微软 API 文档核对生命周期、资源状态、描述符和 Fence 约束。
- 初版检查，2026-09-20：从本文直接提取附录 A/B 到临时目录，使用 CMake / Ninja / MSVC `19.38.33145.0` 完成 x64 Debug 配置、编译和链接，退出码为 `0`，构建输出无编译警告。该检查没有创建正式 A2 工程。
- 1.1 修订检查，2026-09-20：重新从本文原样提取附录 A/B 和附录 D 两套完整代码，分别在独立临时目录完成 CMake / Ninja / MSVC `19.38.33145.0` 的 x64 Debug 配置、编译与链接。两套均退出码 `0`，构建输出无编译警告；同时检查了 Markdown 代码围栏配对。未改动已有 A2 源码，也未创建正式纯窗口练习目录。
- 新增 Windows 基础已对照微软 Win32、ComPtr 和 CMake 官方文档核对；相关链接放在知识点旁，便于按需查阅。
- 两次检查都没有操作 CLion 界面或运行图形窗口；纯窗口输入与关闭、持续清屏、运行时改色、调试层运行结果仍需实际验收，不能把编译通过视为 W/T/G 验收通过。
- A1 文档中的历史故障描述与后续勾选状态保留原样；本文不把旧故障描述当作当前仍未修复的事实。

学习完成的标准不是“这份附录能跑”，而是你既能解释窗口与消息的生命周期，也能在自己的工程里独立定位并解释 **录制、提交、等待** 三个位置。

## 附录 D：不含 D3D12 的纯 Win32 练习

这是第 4 节的配套练习，不是另一个图形框架。建议路径为 `F:\GameDevelop\SomeProjects\GPUVisibilityLab\a2-win32-practice`，与正式 A2 并列；本次仅在文档里提供代码，没有在该路径创建或覆盖工程。练习只需要以下两个文件，不添加 A2 的 `App.cpp`。

### D.1 练习专用 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.24)
project(A2Win32Practice LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "This exercise requires Windows and MSVC.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Select the amd64/x64 toolchain.")
endif()

add_executable(a2_win32_practice main.cpp)
target_compile_features(a2_win32_practice PRIVATE cxx_std_20)
target_compile_definitions(a2_win32_practice PRIVATE
    WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_compile_options(a2_win32_practice PRIVATE /W4 /permissive- /utf-8)
target_link_libraries(a2_win32_practice PRIVATE user32)
```

### D.2 完整 main.cpp

阅读时分三段：App 保存状态，WndProc 响应消息，main 控制整个生命周期。这里故意不用全局 App、图形对象或多文件封装，让你在一个文件里跟踪状态的来回传递。

```cpp
#include <windows.h>

#include <iostream>
#include <stdexcept>
#include <string>

struct App {
    bool running = true;
    bool minimized = false;
    int selected = 1;
};

void CheckWin32(BOOL success, const char* operation) {
    if (!success) {
        const DWORD error = GetLastError();
        throw std::runtime_error(std::string(operation) + ": Win32 "
                                 + std::to_string(error));
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* creation = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(creation->lpCreateParams);
        if (!app) return FALSE;

        // A zero previous value can also mean success.
        SetLastError(ERROR_SUCCESS);
        const LONG_PTR previous = SetWindowLongPtrW(
            hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        if (previous == 0 && GetLastError() != ERROR_SUCCESS) return FALSE;
        return TRUE;
    }
    if (!app) return DefWindowProcW(hwnd, message, wParam, lParam);

    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        const HDC dc = BeginPaint(hwnd, &paint);
        FillRect(dc, &paint.rcPaint, GetSysColorBrush(COLOR_WINDOW));
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_SIZE:
        app->minimized = (wParam == SIZE_MINIMIZED);
        return 0;
    case WM_KEYDOWN:
        if (wParam >= '1' && wParam <= '3') {
            app->selected = static_cast<int>(wParam - '0');
        }
        if (wParam == VK_ESCAPE) app->running = false;
        return 0;
    case WM_CLOSE:
        app->running = false;
        return 0;
    case WM_DESTROY:
        app->running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

int main() {
    // App remains alive until after DestroyWindow has finished its callbacks.
    App app;
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"A2Win32PracticeClass";
    HWND hwnd = nullptr;
    bool classRegistered = false;
    int exitCode = 0;

    try {
        CheckWin32(instance != nullptr, "GetModuleHandleW");
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = WndProc;
        windowClass.hInstance = instance;
        windowClass.lpszClassName = className;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        CheckWin32(windowClass.hCursor != nullptr, "LoadCursorW");
        CheckWin32(RegisterClassW(&windowClass) != 0, "RegisterClassW");
        classRegistered = true;

        const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        RECT outer{0, 0, 1280, 720};
        CheckWin32(AdjustWindowRect(&outer, style, FALSE), "AdjustWindowRect");
        hwnd = CreateWindowExW(
            0, className, L"A2 Win32 | Key: 1", style,
            CW_USEDEFAULT, CW_USEDEFAULT,
            outer.right - outer.left, outer.bottom - outer.top,
            nullptr, nullptr, instance, &app);
        CheckWin32(hwnd != nullptr, "CreateWindowExW");

        RECT client{};
        CheckWin32(GetClientRect(hwnd, &client), "GetClientRect");
        std::cout << "[INFO] Client: " << client.right - client.left
                  << " x " << client.bottom - client.top << '\n';
        ShowWindow(hwnd, SW_SHOW);

        MSG message{};
        int shownSelection = app.selected;
        while (app.running) {
            const BOOL result = GetMessageW(&message, nullptr, 0, 0);
            if (result == -1) CheckWin32(FALSE, "GetMessageW");
            if (result == 0) {
                exitCode = static_cast<int>(message.wParam);
                break;
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
            if (!app.running) break;

            if (shownSelection != app.selected) {
                const std::wstring title = L"A2 Win32 | Key: "
                                           + std::to_wstring(app.selected);
                CheckWin32(SetWindowTextW(hwnd, title.c_str()), "SetWindowTextW");
                shownSelection = app.selected;
                std::cout << "[INFO] Selected: " << shownSelection << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        exitCode = 1;
    }

    // No GPU work exists in this exercise; window cleanup is sufficient.
    if (hwnd && IsWindow(hwnd) && !DestroyWindow(hwnd)) {
        const DWORD error = GetLastError();
        std::cerr << "[FAIL] DestroyWindow: Win32 " << error << '\n';
        exitCode = 1;
    }
    if (classRegistered && !UnregisterClassW(className, instance)) {
        const DWORD error = GetLastError();
        std::cerr << "[FAIL] UnregisterClassW: Win32 " << error << '\n';
        exitCode = 1;
    }
    if (exitCode == 0) std::cout << "[INFO] Normal shutdown.\n";
    return exitCode;
}
```

### D.3 运行时你应当看到什么

1. 在 CLion 打开练习目录，选择 A1 已验证的 MSVC / amd64 工具链，以 Debug 构建 `a2_win32_practice`；这里同样不要给 `add_executable` 加 `WIN32`。
2. 运行后出现标题为 `A2 Win32 | Key: 1` 的窗口，客户区是系统背景色；日志输出 `Client: 1280 x 720`。这不是蓝色 D3D12 清屏，也没有 GPU 帧数。
3. 点击窗口后按主键盘 `2 / 3 / 1`，标题和日志变化。没有操作时 GetMessage 等待是正常状态，不要求计数持续增加。
4. 分别检查最小化恢复与三条关闭路径，正常关闭日志为 `Normal shutdown.`、原生退出码为 0。回到第 4.12 节完成断点实验。

这个练习中的 GDI 只用来获得可见背景，无需在本关继续学画笔、字体或复杂 GDI 绘制。此处也不把客户区背景变化作为按键证据，按键证据是标题和状态变量。Windows 基础完成后，按第 4.11 节替换循环并接入 D3D12，而不是继续扩展成通用桌面应用课程。
