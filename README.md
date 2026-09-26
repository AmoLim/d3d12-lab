# GPUVisibilityLab

GPU 驱动可见性试验场：一个基于 C++20、Win32 和 Direct3D 12 的学习型实验项目。

目标是在同一个可重复生成的实例场景中，对比不剔除、CPU 单线程剔除、CPU 多线程剔除，以及 GPU 剔除与间接绘制，通过正确性校验和 CPU / GPU 分项计时，分析不同方案的收益与成本。

第一版的可见性范围是**视锥剔除**，不是遮挡剔除。项目不以“GPU 一定更快”为结论，也不打算实现完整游戏引擎。

## 当前进度

目前处于 A1 / A2 基础阶段，以下说明以现有源码为准，规划中的渲染与剔除功能尚未实现。

| 模块 | 当前内容 |
| --- | --- |
| `a1-environment-check` | 检查 MSVC、x64、Debug 配置，枚举显卡，选择 RTX 5070 Ti，启用 D3D12 Debug Layer，创建设备并验证调试消息队列。学习记录中已完成 A1 验收。 |
| `a2-win32-practice` | Win32 窗口、消息分发、键盘输入、最小化状态与退出清理。`Dx12Renderer` 目前仅为骨架，未接入渲染循环。 |
| `a2-window-clear` | A2 开发中的工程，目前实现窗口与消息循环；`LoadPipeline()` 仍为空，尚未实现 D3D12 持续清屏。 |

## 目录结构

```text
GPUVisibilityLab/
  a1-environment-check/    # D3D12 环境探针与 CTest 检查
  a2-win32-practice/       # Win32 基础练习，Core/ 存放窗口及渲染器骨架
  a2-window-clear/         # 自己的窗口与清屏工程（开发中）
  docs/                   # 路线、技术指南与学习记录
    CheckPoints/          # 阶段验收截图
    docs/                 # Obsidian 项目笔记、设计与模板
  references/             # 本地参考代码，不纳入版本管理
```

各练习是独立的 CMake 工程，根目录没有统一的 `CMakeLists.txt`。请分别配置和构建，不要共用构建缓存。

## 开发环境

- Windows x64，MSVC 工具链及 Windows SDK，支持 C++20。A1 和 Win32 练习会拒绝非 Windows、非 MSVC 或非 64 位配置。
- CMake：A1 和 Win32 练习要求 3.24 或以上；`a2-window-clear` 当前要求 4.3 或以上。
- Ninja：用于下方命令行构建，也可使用 CLion 自带的 CMake / Ninja。
- CLion：当前学习使用的 IDE，不是命令行构建的必需依赖。
- A1 运行要求安装 Windows 可选功能 **Graphics Tools（图形工具）**，以提供 D3D12 调试层。
- A1 当前明确匹配 **NVIDIA GeForce RTX 5070 Ti**，不会自动回退到其他显卡或 WARP。使用其他显卡需要先调整 `a1-environment-check/main.cpp` 中的适配器筛选逻辑。

A1 和 Win32 练习均需以 **Debug** 运行，Release 会直接退出。当前工程尚不需要 DXC；HLSL 编译器属于后续渲染阶段的工具准备。

## 构建与运行

以下 PowerShell 命令均在项目根目录执行。先进入已初始化 **MSVC x64** 环境的开发者终端，确保 `cmake`、`ninja` 和 `cl` 可用；普通终端可能无法找到编译器。

### A1：环境检查

```powershell
cmake -S a1-environment-check -B a1-environment-check/build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build a1-environment-check/build-debug
.\a1-environment-check\build-debug\a1_environment_check.exe
ctest --test-dir a1-environment-check/build-debug --output-on-failure
```

成功时应看到显卡选择、调试层、设备创建和调试消息验证的 `[PASS]` 输出，最后显示 `A1 environment probe complete`，退出码为 `0`。CTest 执行的也是这一环境探针，因此需要真实满足显卡和调试层要求，不是脱离硬件的单元测试。

该程序不绘制三角形；官方 HelloTriangle 是单独的学习检查点。

### A2：Win32 练习

```powershell
cmake -S a2-win32-practice -B a2-win32-practice/build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build a2-win32-practice/build-debug
.\a2-win32-practice\build-debug\a2_win32_practice.exe
```

预期出现初始客户区为 `1280 x 720` 的窗口，背景由 GDI 填充，并非 D3D12 清屏。

- 窗口获得焦点后，按主键盘 `1`、`2`、`3` 更新选择状态、窗口标题和控制台日志，不改变背景色。
- 支持最小化与恢复；当前不支持拖动调整窗口大小或最大化。
- 按 `Esc`、`Alt+F4` 或点击关闭按钮退出，正常路径输出 `Normal Shutdown.`。
- 没有输入时，`GetMessageW` 阻塞等待消息属于正常行为，不是持续渲染循环。

### A2：窗口与清屏工程（开发中）

此目录当前仍是窗口阶段，下面的命令只用于构建现有代码，不代表持续清屏已经完成。需要 CMake 4.3 或以上。

```powershell
cmake -S a2-window-clear -B a2-window-clear/build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build a2-window-clear/build-debug
.\a2-window-clear\build-debug\a2_window_clear.exe
```

### 在 CLion 中使用

分别打开需要学习的子工程目录，选择 Visual Studio / MSVC 工具链和 `amd64` / `x64` 架构，使用 Ninja、Debug 配置及独立构建目录（例如 `cmake-build-debug`），再选择该工程的可执行目标运行或调试。

## 学习文档

- [项目总览与实验路线](docs/overview.md)
- [A1：CLion 与 D3D12 环境搭建](docs/A1-CLion-D3D12-Setup.md)
- [A2：自己的 D3D12 窗口与持续清屏](docs/A2-Window-And-Clear.md)
- [A2：Win32 练习复盘](docs/A2-Win32-Practice-Review.md)
- [设计笔记与模板](docs/docs/)
- [阶段验收截图](docs/CheckPoints/)

技术指南中的完整示例和验收目标不等同于当前工程已实现的功能；部分学习笔记保留了本机绝对路径，换机器时需按实际目录调整。

## 后续计划

1. 完成 D3D12 交换链、命令录制与提交、Fence 同步和持续清屏。
2. 实现基础绘制、相机、深度测试及固定随机种子的实例场景。
3. 建立不剔除、CPU 单线程和 CPU 多线程视锥剔除的对照路径。
4. 实现 Compute Shader 剔除、可见 ID 压缩和 `ExecuteIndirect` 绘制。
5. 增加可见 ID 一致性检查、CPU / GPU 分项计时及 CSV 实验报告。

详细范围与验收标准见项目总览。以上均为计划，不代表已有性能结果。

## 版本管理约定

提交源码、CMake 配置、学习文档、设计笔记和必要的验收截图。`.gitignore` 排除构建目录、编译产物、IDE 本地配置、Obsidian 工作区状态和 `references/`。

`references/` 是本地参考资料，不是构建依赖。现有练习无需它即可配置构建；参考工程的源码与许可证由其各自项目维护。不要将整个参考仓库或本机生成的构建缓存一并提交。
