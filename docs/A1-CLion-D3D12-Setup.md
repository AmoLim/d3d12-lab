# A1 技术指南：在 CLion 中建立 D3D12 学习环境

版本：1.0 | 编写日期：2026-09-17 | 预算：约 3 小时，系统组件安装与网络等待可能另计

适用配置：Windows、CLion、RTX 5070 Ti、Ryzen 7 9700X、64GB 内存。

## 1. 本关目标与范围

A1 的目的不是自己写出渲染器，而是证明后续学习所依赖的工具链和运行环境可靠。

最终需要拿出四项证据：

1. CLion 使用 MSVC 编译一个 x64、Debug 的 CMake 项目，并能命中断点。
2. 程序明确选择 RTX 5070 Ti，而不是 CPU 核显或 WARP 软件设备。
3. D3D12 Debug Layer 在设备创建前成功启用，并能读取调试消息。
4. 微软官方 HelloTriangle 样例完成 Debug 构建，显示三角形，并确认实际使用的适配器与调试层。

本关不要求理解全部初始化代码，不实现剔除，不搭引擎框架，不改造官方样例的构建系统。

**结论：CLion 完全可以作为主 IDE。Visual Studio 工具链不等于必须使用 Visual Studio IDE。** CLion 支持 MSVC 工具链；本项目采用 `CLion + MSVC + Windows SDK + CMake + Ninja`。[JetBrains 工具链说明](https://www.jetbrains.com/help/clion/quick-tutorial-on-configuring-clion-on-windows.html)

### 两条路径不要混淆

| 路径 | 工程 | 构建方式 | 用途 |
|---|---|---|---|
| 自己的学习工程 | 附带的 `a1-environment-check` | CLion / CMake / Ninja / MSVC | 验证工具链、断点、显卡选择和调试层 |
| 官方参考工程 | 微软 `D3D12HelloTriangle.vcxproj` | 在 CLion 终端调用 MSBuild | 验证官方完整绘制链路能够运行 |

CLion 的 CMake 项目不会因为目录里存在 `.vcxproj` 就自动获得其全部构建规则。当前官方样例还有 NuGet、Agility SDK、DXC 和 Shader 编译步骤；不要只加几个 `.cpp` 文件就认为已经迁移完毕。[CLion 工程格式](https://www.jetbrains.com/help/clion/project-models.html)、[官方样例工程](https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTriangle/D3D12HelloTriangle.vcxproj)

## 2. 本机检查结果

以下是本次实际读取或运行得到的结果，不是推荐安装清单：

| 项目 | 已确认的状态 |
|---|---|
| CLion | `2026.2.1`，目录 `E:\Applications\JetBrains\CLion` |
| Visual Studio 工具链来源 | Community 2022，`17.14.24`，目录 `E:\Applications\Microsoft VS\2022` |
| MSVC | 已安装 `14.38.33130` 和 `14.44.35207`；本次命令行验证实际使用 `14.38.33130`，编译器识别为 `19.38.33145.0` |
| Windows SDK | `10.0.22621.0`，已确认存在 `d3d12.h`、x64 `d3d12.lib` 和 `dxc.exe` |
| CLion 自带 CMake / Ninja | `4.3.1` / `1.13.2` |
| 附带检查项目 | 已通过 CMake 配置、MSVC 编译和链接 |
| 显卡枚举 | 已发现 RTX 5070 Ti、AMD 核显与软件设备，检查程序选择 RTX 5070 Ti |
| 系统 D3D12 调试接口 | **未通过**：`D3D12GetDebugInterface` 返回 `0x887A002D` |
| CLion 界面设置与断点 | 尚未操作验收，需要你按本文步骤完成 |
| 官方三角形 | 尚未下载、构建或运行；本文给出构建流程，不标记为已通过 |

`0x887A002D` 是 `DXGI_ERROR_SDK_COMPONENT_MISSING`，含义是依赖的 SDK 组件缺失或不匹配。当前应优先检查 Windows 的“图形工具 / Graphics Tools”可选功能，而不是重装显卡或关闭调试层。[微软错误码定义](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-error)

普通 PowerShell 里找不到 `cl`、`cmake` 或 `dxc`，不代表它们没有安装。CLion 会为选定工具链准备构建环境，本机这些工具确实存在；不要因此盲目修改系统 PATH。

## 3. 检查点 A1.0：理解各工具的职责

预算：15 分钟。

| 工具 | 本关职责 |
|---|---|
| CLion | 编辑、组织工程、发起构建、调试 C++ |
| MSVC / `cl.exe` | 将 C++ 编译成机器代码 |
| Windows SDK | 提供 Windows / DXGI / D3D12 的头文件、链接库与工具 |
| CMake | 根据工程描述生成构建规则 |
| Ninja | 执行 CMake 生成的构建任务 |
| MSBuild | 执行微软官方 `.vcxproj` 的构建规则 |
| DXC | 编译 HLSL Shader，不负责编译 C++ |
| D3D12 Debug Layer | 检查图形 API 的错误使用，不等同于 C++ 的 Debug 编译模式 |

**验收：** 能用自己的话解释为什么“CLion + MSVC”并不矛盾，以及为什么 C++ 编译成功还不能证明图形调试层可用。

## 4. 检查点 A1.1：配置 CLion 工具链

预算：25 分钟。

1. 在 CLion 中选择 `File > Open`，打开以下目录，作为 CMake 项目加载：

   `F:\GameDevelop\SomeProjects\GPUVisibilityLab\a1-environment-check`

2. 进入 `Settings > Build, Execution, Deployment > Toolchains`。
3. 新建一个 `Visual Studio` 类型工具链，命名为 `MSVC-D3D12`。
4. 按下表设置。不要选择 CLion 默认的 MinGW，也不要使用 WSL。

| 字段 | 设置 |
|---|---|
| Toolset | `E:\Applications\Microsoft VS\2022` |
| Architecture | `amd64`，目标为 x64 |
| Platform | 留空，普通 Windows 桌面应用 |
| Version / 版本 | 初次留空自动识别，不选择 `8.1`。本机已安装 Windows SDK `10.0.22621.0`；若界面单独提供 Windows SDK 版本字段，可选此版本。锁定 MSVC 工具集是另一回事，暂不添加 `-vcvars_ver` 参数 |
| CMake | Bundled |
| Build Tool | Bundled Ninja |
| C / C++ Compiler | 自动识别到此 VS 安装下的 `cl.exe` |
| Debugger | 与 MSVC 工具链配套的 Bundled LLDB，不选 MinGW GDB |

5. 等待工具检测完成，再进入 `Settings > Build, Execution, Deployment > CMake`。
6. 创建或调整一个 Profile：

| 字段 | 设置 |
|---|---|
| Name | `A1-Debug` |
| Build type | `Debug` |
| Toolchain | `MSVC-D3D12` |
| Generator | `Ninja` |
| Build directory | `cmake-build-debug` |
| CMake options | 初次留空 |

7. Reload CMake。若此前用 MinGW 配置过此项目，使用 CLion 的 `Reset Cache and Reload Project`，或选择一个新的构建目录，不混用旧缓存。

**验收：** CMake 输出中的编译器标识为 `MSVC`，编译器路径来自上述 VS 安装目录，配置生成完成且没有错误。`14.xx` 是工具集版本，`19.xx` 是编译器版本；两者数字不同正常。

**保留证据：** Toolchains 设置截图、CMake Profile 截图、CMake 配置输出。

### 已定位问题：Bundled CMake 显示无法在此环境下工作

2026-09-17 本机 CLion 日志显示，失败的工具链检测实际执行了：

```text
vcvarsall.bat amd64 8.1
[ERROR:winsdk.bat] Windows SDK 8.1 : 'include' not found
```

同一日志中，不带 `8.1` 的 `vcvarsall.bat amd64` 已成功初始化 x64 环境；独立运行 Bundled CMake 也能正常输出 `4.3.1`。因此此处是错误的 SDK 版本约束导致环境初始化失败，不是 CMake 文件损坏。

处理：在当前 `MSVC-D3D12` 工具链的 Version / SDK 版本参数中清除 `8.1`，Architecture 保持 `amd64`、Platform 留空，CMake 与 Ninja 继续使用 Bundled。点击 Apply 并等待重新检测，然后确认 CMake Profile 引用的是这个已修正的工具链，再 Reload CMake。不要为此安装 Windows SDK 8.1 或重装 CLion。

修复后的验收仍需在 IDE 中完成：CMake 能识别版本、编译器检测成功、项目重新配置成功。本次只确认了原因和正确参数在日志中的成功记录，没有代替用户修改正在编辑的全局工具链设置。

参考：[JetBrains Windows 配置](https://www.jetbrains.com/help/clion/quick-tutorial-on-configuring-clion-on-windows.html)、[CMake Profiles](https://www.jetbrains.com/help/clion/cmake-profile.html)。

## 5. 检查点 A1.2：构建、运行与调试检查程序

预算：20 分钟。

附带文件已经就位，不需要另写测试工程：

- [CMakeLists.txt](<F:/GameDevelop/SomeProjects/GPUVisibilityLab/a1-environment-check/CMakeLists.txt>)：要求 Windows、MSVC、64 位，链接 D3D12 与 DXGI。
- [main.cpp](<F:/GameDevelop/SomeProjects/GPUVisibilityLab/a1-environment-check/main.cpp>)：枚举显卡、严格选择 5070 Ti、启用调试层、创建设备、验证调试消息通道。

1. 选择 `A1-Debug` Profile 和 `a1_environment_check` Target，点击 Build。
2. 点击 Run，查看控制台。此程序没有图形窗口，这是预期行为。
3. 在 `main` 的第一条输出语句设置断点，使用 Debug 启动。
4. 命中断点后单步执行，确认可以查看局部变量；恢复执行后记录完整输出。

**工具链部分验收：** Build 成功；程序显示 `Architecture: 64-bit`、`Build configuration: Debug`；断点能命中；枚举结果包含并选择 `NVIDIA GeForce RTX 5070 Ti`。

**当前预期：** 在修复系统调试组件前，运行会输出如下失败并以原生退出码 `2` 结束。这意味着检查正常拦住了问题，不是整个 A1 已通过：

```text
[PASS] Selected adapter: NVIDIA GeForce RTX 5070 Ti
[FAIL] D3D12GetDebugInterface: 0x887A002D
[ACTION] Check/install the Windows Graphics Tools optional feature, then run this Debug build again.
[INFO] No D3D12 device was created; this is not an A1 pass.
```

不要删除失败分支，也不要切成 Release 绕过检查。Release 运行本项目会直接失败；本检查故意不提供静默关闭调试层的选项。

## 6. 检查点 A1.3：修复并确认图形调试组件

预算：30 分钟；下载或系统维护等待不受此预算保证。

1. 打开 Windows 设置，搜索“可选功能 / Optional features”。
2. 查看 `Graphics Tools / 图形工具` 是否安装。若未安装，通过“添加可选功能 / 查看功能”搜索并安装。
3. 安装可能需要管理员权限和网络。如果系统要求重启，先完成重启。
4. 若已显示安装但仍报 `0x887A002D`，先记录系统版本和组件状态，再检查组件维护或修复；不要从第三方网站下载 DLL 放进 System32。
5. 回到 CLion，重新运行同一个 Debug 检查程序。

可选诊断：在管理员 PowerShell 中查询组件状态，命令只查询、不安装：

```powershell
Get-WindowsCapability -Online -Name 'Tools.Graphics.DirectX*'
```

本次交付没有替你安装系统组件，也没有修改全局 IDE 或系统配置。

**完整通过时的关键输出如下；这是预期结果，不是当前已取得的日志：**

```text
[PASS] Debug layer enabled before device creation
[PASS] D3D12 device created (minimum feature level 12_0)
[DEBUG] A1 controlled debug-queue marker.
[PASS] Controlled debug message retrieved; no warnings/errors in this probe
[PASS] A1 environment probe complete. Official triangle is a separate checkpoint.
```

**验收：** 原生退出码为 `0`；调试层在 `D3D12CreateDevice` 之前启用；能通过 `ID3D12InfoQueue` 读回受控测试标记；没有未处理的警告或错误。

这个标记是程序主动写入的 INFO 消息，用于证明调试消息通道可用，不是故意制造 GPU 错误，也不能证明尚未执行的渲染代码正确。`feature level 12_0` 是本检查请求的最低功能级别，不是已检测到显卡全部高级特性的声明。

参考：[D3D12 环境设置](https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-environment-set-up)、[D3D12GetDebugInterface](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-d3d12getdebuginterface)、[调试消息接口](https://learn.microsoft.com/en-us/windows/win32/api/d3d12sdklayers/nf-d3d12sdklayers-id3d12infoqueue-addapplicationmessage)。

## 7. 检查点 A1.4：获取并构建官方三角形

预算：60 分钟；网络下载时间可能延长。以下官方样例命令经过项目文件与文档核对，尚未在本机执行。

### 7.1 获取最小参考目录

在 CLion 的 PowerShell Terminal 中逐条执行。目标 `references\DirectX-Graphics-Samples` 尚不存在时才运行 clone；已经存在则检查现有仓库，不重复克隆或删除它。

```powershell
git clone --depth 1 --filter=blob:none --sparse https://github.com/microsoft/DirectX-Graphics-Samples.git 'F:\GameDevelop\SomeProjects\GPUVisibilityLab\references\DirectX-Graphics-Samples'
```

```powershell
git -C 'F:\GameDevelop\SomeProjects\GPUVisibilityLab\references\DirectX-Graphics-Samples' sparse-checkout set Samples/Desktop/D3D12HelloWorld
```

```powershell
git -C 'F:\GameDevelop\SomeProjects\GPUVisibilityLab\references\DirectX-Graphics-Samples' rev-parse HEAD
```

记录最后输出的 commit SHA。在线 `master` 可能变化，之后排错以你实际拉取的版本为准。本关只构建 HelloTriangle，不构建整个示例集合。

commit SHA: rse HEAD
213dd4fd4918ea009dd8f35adee1aff1f2ecaba4

### 7.2 理解当前样例依赖

本次查阅的官方工程引用 `Microsoft.Direct3D.D3D12 1.618.3` 和 `Microsoft.Direct3D.DXC 1.8.2505.32`。MSBuild 会处理 NuGet 导入、HLSL 编译以及运行时文件部署，不能用系统 SDK 里的一个 `dxc.exe` 替代全部规则。

官方样例采用 Agility SDK，其随程序部署的运行时/调试层与独立检查程序使用的系统组件路径不同。因此“官方样例能跑”不自动等于“系统调试组件检查通过”，两项分别验收。

来源：[packages.config](https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTriangle/packages.config)、[vcxproj 构建规则](https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTriangle/D3D12HelloTriangle.vcxproj)。

### 7.3 使用本机 MSBuild 恢复依赖并构建

在同一个 PowerShell Terminal 中先定义路径：

```powershell
$msbuild = 'E:\Applications\Microsoft VS\2022\MSBuild\Current\Bin\amd64\MSBuild.exe'
$sampleRoot = 'F:\GameDevelop\SomeProjects\GPUVisibilityLab\references\DirectX-Graphics-Samples\Samples\Desktop\D3D12HelloWorld\src'
$project = Join-Path $sampleRoot 'HelloTriangle\D3D12HelloTriangle.vcxproj'
$packages = Join-Path $sampleRoot 'packages'
```

然后执行：

```powershell
& $msbuild $project /restore /t:Build /p:RestorePackagesConfig=true "/p:RestoreRepositoryPath=$packages" /p:Configuration=Debug /p:Platform=x64 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0.22621.0
```

单独检查本条构建的退出码：

```powershell
$LASTEXITCODE
```

此步骤需要访问 GitHub / NuGet，并会下载工程依赖。`RestoreRepositoryPath` 指向 `src\packages`，因为当前工程从 `..\packages` 导入依赖；不要任意改成 `HelloTriangle\packages`。这里使用 `/restore /t:Build`，不要替换成 `/t:Restore,Build` 或 `dotnet restore`。[NuGet 构建与恢复说明](https://learn.microsoft.com/en-us/nuget/reference/msbuild-targets)

**验收：** 退出码为 `0`，构建没有错误，并在以下目录找到 exe、Shader 编译结果及所需运行时文件：

`...\HelloTriangle\bin\x64\Debug\`

关键产物包括 `D3D12HelloTriangle.exe`、`shaders_VSMain.cso`、`shaders_PSMain.cso`，以及由 NuGet 构建规则部署的 D3D12 运行时文件。以实际工程配置为准，不把 exe 单独挪出去运行。

## 8. 检查点 A1.5：运行官方样例并补齐证据

预算：30 分钟。

### 8.1 先运行原样例

在上一节同一个 PowerShell Terminal 中执行：

```powershell
$output = Join-Path $sampleRoot 'HelloTriangle\bin\x64\Debug'
Push-Location $output
```

```powershell
& '.\D3D12HelloTriangle.exe'
```

关闭窗口后回到原目录：

```powershell
Pop-Location
```

应该出现绘制了三角形的窗口。不要传入 `-warp` 或 `/warp` 参数。暂不以窗口缩放、帧率或自定义相机作为本关要求。

### 8.2 让调试层状态可验证

在 CLion 中打开参考工程的 `HelloTriangle\D3D12HelloTriangle.cpp`，定位 `LoadPipeline`。当前样例可能使用“获取调试接口成功才启用，否则继续运行”的逻辑；仅仅显示三角形不能证明调试层已启用。

在原来的 `_DEBUG` 调试层初始化位置，改为失败就明确报错；不要在已经创建设备后再重复启用：

```cpp
ComPtr<ID3D12Debug> debugController;
ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
debugController->EnableDebugLayer();
```

这一小段只用于参考副本的 A1 诊断，保留上游版权说明。成功运行说明获取调试接口没有被静默跳过。

### 8.3 核对真正用于创建设备的显卡

仍在 `LoadPipeline` 中找到硬件路径：局部变量 `hardwareAdapter` 被传给 `D3D12CreateDevice`。在这次设备创建成功后、且仍位于 `hardwareAdapter` 的作用域内，加入以下诊断。不是放在 WARP 分支，也不是单独枚举一个不参与创建设备的适配器：

```cpp
DXGI_ADAPTER_DESC1 a1Description{};
ThrowIfFailed(hardwareAdapter->GetDesc1(&a1Description));
#if defined(_DEBUG)
const std::wstring a1Title =
    std::wstring(a1Description.Description) + L" | A1 Debug Layer ON";
SetWindowTextW(Win32Application::GetHwnd(), a1Title.c_str());
#endif
```

窗口标题的调试层标记成立的前提是上一步已经改为严格检查，且本次确实构建了 Debug。不要只添加一个写死的“开启”标签。

重新运行 A1.4 的同一条构建命令，再启动 exe。

**验收：** 三角形正常显示，窗口标题包含 `NVIDIA GeForce RTX 5070 Ti` 和 `A1 Debug Layer ON`；连续正常启动、关闭三次，无异常退出。

如果标题显示 AMD 核显，验收不通过。需要在实际适配器选择逻辑中明确选择 NVIDIA 硬件适配器，并把它传给设备创建函数；仅改变日志或窗口标题不算修复。附带检查程序展示了“拒绝静默回退”的选择逻辑。

**保留证据：** 官方样例 commit SHA、完整构建结果、带标题的三角形截图、这两处诊断修改的 diff，以及独立环境检查的完整成功输出。

## 9. 常见问题与停止条件

| 现象 | 优先检查 | 不要做什么 |
|---|---|---|
| Bundled CMake 显示无法在此环境下工作 | 本机曾误传 `vcvarsall.bat amd64 8.1`；清除工具链中的 `8.1` 版本参数，让现有 Windows 10 SDK 自动参与初始化 | 不重装 CMake，不补装旧 Windows SDK 8.1 |
| CMake 显示 GNU / MinGW | Profile 是否真的选中 MSVC 工具链，缓存是否来自旧工具链 | 不混装一堆编译器试运气 |
| `windows.h` / `d3d12.h` 找不到 | Windows SDK 组件、工具链环境；本机相关文件已确认存在 | 不手工把系统头文件复制到项目 |
| 链接不到 `D3D12CreateDevice` | 是否链接 `d3d12`，是否使用正确的 x64 SDK 库 | 不把 DLL 当成链接库 |
| `0x887A002D` | 系统 Graphics Tools 状态，以及当前使用系统运行时还是 Agility SDK | 不关闭调试层伪装通过 |
| 运行后控制台很快退出 | 用 CLion Run 看日志和退出码；检查程序本来就不创建窗口 | 不把没有窗口当成图形故障 |
| 官方样例缺少 NuGet props/targets | 网络、恢复输出、`src\packages` 路径、实际 packages.config | 不删除依赖导入行 |
| 找不到 Shader `.cso` 或 D3D12 DLL | 是否完整构建并保留整个输出目录 | 不单独移动 exe，不从第三方网站找 DLL |
| CLion 无法按 CMake 加载官方样例 | 官方样例不是本次附带的 CMake 检查项目 | 不在 A1 重写整套构建系统 |
| 普通终端找不到 `cl.exe` | 用 CLion 工具链或 Developer PowerShell 初始化环境 | 不据此认定 MSVC 没装 |
| 能编译但断点不命中 | Debug Profile、MSVC 配套 debugger、实际运行的 exe、符号文件 | 不把 Run 当成 Debug |

同一个问题连续定位 30 分钟仍没有进展时，记录“操作步骤、第一条有效错误、完整日志、当前版本”，先解决这个节点，不靠继续堆功能掩盖它。

## 10. 最终验收表

以下保持未勾选，需由实际操作与证据确认；本次命令行编译成功不代替你在 CLion 中完成验收。

- [x] G1：CLion 使用 MSVC / amd64，CMake Profile 为 Debug / Ninja。
- [x] G2：检查项目构建成功，能命中 `main` 中的断点并查看变量。
- [x] G3：检查项目选中 RTX 5070 Ti，未回退到核显或 WARP。
- [x] G4：系统 Debug Layer 严格检查通过，读回受控消息，原生退出码 `0`。
- [x] G5：记录官方样例 commit SHA，完成 Debug x64 构建。
- [x] G6：官方三角形正常显示，实际设备为 RTX 5070 Ti，调试层未被静默跳过。
- [x] G7：官方样例连续正常启动、关闭三次，记录截图、日志与诊断 diff。
- [x] G8：能解释 CLion、MSVC、CMake、Windows SDK、DXC 与 Debug Layer 的区别。

状态记录：

```text
日期：
CLion / MSVC / Windows SDK 版本：
已通过的 G 编号：
当前阻塞节点：G8
第一条有效错误及 HRESULT：
官方样例 commit SHA：commit SHA: rse HEAD  
213dd4fd4918ea009dd8f35adee1aff1f2ecaba4
证据位置：CheckPoints
下一步：我对Windows SDK、DXC与Debug Layer不熟悉
```

全部完成后再进入 A2：保留参考工程，开始自己的最小窗口与清屏程序。A1 参考样例跑通不等于已经掌握它的渲染实现，这部分会在 A2/A3 按职责逐步学习。

## 附录：复现已执行的命令行编译

这不是 CLion 主流程，只用于排查“IDE 配置问题还是编译器问题”。三个命令块需在同一个 PowerShell 会话执行，作用域仅为当前进程环境：

```powershell
& 'E:\Applications\Microsoft VS\2022\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
```

```powershell
& 'E:\Applications\JetBrains\CLion\bin\cmake\win\x64\bin\cmake.exe' -S 'F:\GameDevelop\SomeProjects\GPUVisibilityLab\a1-environment-check' -B 'F:\GameDevelop\SomeProjects\GPUVisibilityLab\a1-environment-check\build-verification' -G Ninja '-DCMAKE_BUILD_TYPE=Debug' '-DCMAKE_MAKE_PROGRAM=E:/Applications/JetBrains/CLion/bin/ninja/win/x64/ninja.exe'
```

```powershell
& 'E:\Applications\JetBrains\CLion\bin\cmake\win\x64\bin\cmake.exe' --build 'F:\GameDevelop\SomeProjects\GPUVisibilityLab\a1-environment-check\build-verification' --parallel 4
```

运行：

```powershell
& 'F:\GameDevelop\SomeProjects\GPUVisibilityLab\a1-environment-check\build-verification\a1_environment_check.exe'
```

然后单独查询 `$LASTEXITCODE`。环境修复前当前实际结果为 `2`；不要把这个运行失败记成测试通过。

该工程也注册了一个 CTest 环境检查。系统调试组件没有修复时，该检查预期失败；它不是无需 GPU / 系统组件的纯单元测试。

## 附录：G8 工具职责与概念辨析

本附录对应 G8。目标不是背诵缩写，而是能够说明：从源码到程序运行，每个工具负责什么，以及遇到问题时应先检查哪一环。

CLion 是 IDE，负责编辑代码、组织开发操作，并整合构建与调试入口。MSVC 是微软的 C/C++ 编译工具集，包含 `cl.exe` 编译器、`link.exe` 链接器等。CLion 中的 Toolchain 配置不只是选择 MSVC，还会关联构建工具、调试器及相应的开发环境。

### 一、CMake：描述和组织项目的构建

**CMake 不是 C++ 编译器。它读取项目描述，生成构建工具能够执行的规则。**

假设一个项目有十个 `.cpp` 文件，还需要 C++20、D3D12 和 DXGI，就需要明确：哪些源文件组成哪个目标、使用什么编译选项、链接什么库、各个目标之间有什么依赖。这些要求写在 `CMakeLists.txt` 中。

当前环境检查项目的简化流程是：

```text
CMakeLists.txt
    -> CMake 配置项目并生成构建规则
    -> Ninja 根据规则执行需要的构建任务
    -> 调用 MSVC 编译 C++ 并链接
    -> 得到 a1_environment_check.exe
```

例如，下面是构建描述，不是需要在 PowerShell 中运行的命令：

```cmake
add_executable(a1_environment_check main.cpp)
target_compile_features(a1_environment_check PRIVATE cxx_std_20)
target_link_libraries(a1_environment_check PRIVATE d3d12 dxgi)
```

这三行分别表示：用 `main.cpp` 创建可执行目标、要求支持 C++20、为目标链接 D3D12 和 DXGI。完整配置见 [CMakeLists.txt](<F:/GameDevelop/SomeProjects/GPUVisibilityLab/a1-environment-check/CMakeLists.txt>)。

**区分职责：CMake 组织构建，Ninja 执行任务，MSVC 编译和链接 C++，CLion 提供统一的操作入口。** 即使通过 `cmake --build` 发起构建，底层仍会调用所选的构建工具。

本关的两条构建路径也要分清：自己的检查项目使用 `CMake + Ninja`；官方三角形使用已有的 `.vcxproj`，由 MSBuild 执行构建规则。这两条路径都可以调用 MSVC，但官方工程不经过检查项目的 CMake。

参考：[CMake 构建系统说明](https://cmake.org/cmake/help/latest/manual/cmake.1.html)。

### 二、Windows SDK：提供 Windows 开发接口和工具

SDK 是 Software Development Kit，即软件开发工具包。**Windows SDK 为开发 Windows 程序提供头文件、链接库和开发工具，不负责替代 C++ 编译器。**

| 内容 | 本项目中的例子 | 用途 |
|---|---|---|
| 头文件 | `windows.h`、`d3d12.h`、`dxgi1_6.h` | 声明可使用的类型、函数和接口，让编译器知道如何检查这些 API 的调用 |
| 链接库 | `d3d12.lib`、`dxgi.lib` | 帮助链接器建立程序对相应 API 的引用 |
| 开发工具 | SDK 随附的 `dxc.exe` 等 | 支持着色器编译及其他开发任务；DXC 也可以由独立包提供 |

例如，你写下 `D3D12CreateDevice(...)` 时，MSVC 根据 SDK 头文件中的声明检查调用，再由链接器结合相应导入库完成链接。这里的 `d3d12.lib` 是导入库，不是把整个 D3D12 运行时或显卡驱动装进 exe。程序运行时还需要 D3D12 运行时和显卡驱动；官方样例另外使用 Agility SDK，见 7.2。

可以这样记：**MSVC 知道怎么编译 C++，Windows SDK 提供调用 Windows / D3D12 所需的开发接口。** 因此，MSVC 安装正常，不代表 Windows SDK 一定安装完整或搜索路径一定正确。

Windows SDK 和 Windows 的 Graphics Tools 可选功能也不是一回事。前者解决开发时的接口与工具需求；本关通过后者补齐系统 D3D12 调试组件。能够编译 `d3d12.h`，不代表运行时一定能获取调试接口。

参考：[Windows SDK 概述](https://learn.microsoft.com/en-us/windows/apps/windows-sdk/)、[D3D12 开发环境设置](https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-environment-set-up)。

### 三、DXC：编译 GPU 着色器程序

DXC 是 DirectX Shader Compiler。**它负责编译 HLSL 着色器，不负责把普通 C++ 源文件编译成 exe。**

| 代码 | 编译工具 | 最终用途 |
|---|---|---|
| C++：创建窗口、管理资源、提交绘制命令、CPU 视锥剔除 | MSVC | CPU 侧程序 |
| HLSL：计算顶点位置、像素颜色、Compute Shader 中的 GPU 剔除 | DXC | GPU 侧着色器程序 |

当前 D3D12 学习路径中的流程是：

```text
HLSL 源码 -> DXC -> DXIL 中间表示 -> 显卡驱动进一步处理 -> GPU 执行
```

DXIL 不是 RTX 5070 Ti 的最终原生机器码。DXC 生成供图形驱动使用的着色器二进制；编译后的文件可以保存为 `.cso`，但扩展名本身不能确定其中的具体格式。

DXC 本身仍是在 CPU 上运行的开发工具；所谓“编译 GPU 程序”，说的是编译产物的用途，不是编译过程在 GPU 上执行。后续将视锥剔除从 CPU 转到 GPU 时，会把相应的并行计算逻辑写进 HLSL Compute Shader，再由 C++ 程序安排资源和提交执行。

官方三角形通过工程构建规则调用 DXC；安装了 SDK 中的 `dxc.exe`，并不意味着可以忽略官方工程指定的 NuGet 依赖版本和部署规则。

参考：[微软 DirectX Shader Compiler 项目](https://github.com/microsoft/DirectXShaderCompiler)。

### 四、三个不同的 Debug 概念

**G8 中的 Debug Layer 指 D3D12 调试层，不是 IDE 的 Debug 按钮，也不是 Debug 构建配置。**

| 概念 | 它是什么 | 在本关中的体现 |
|---|---|---|
| Debug 构建配置 | 一组编译、链接等构建选项，通常生成调试信息并关闭或减少优化；具体行为由项目配置决定 | 在 CMake Profile 或 MSBuild 参数中选择 Debug |
| Debugger 调试器 | 控制程序执行、检查程序状态的工具 | 在 CLion 中命中断点、单步执行、查看变量与调用栈 |
| D3D12 Debug Layer 调试层 | 运行时检查 D3D12 API 使用的验证层 | 报告资源状态、参数或其他 API 使用问题，通过调试输出或 `ID3D12InfoQueue` 读取消息 |

调试信息帮助调试器把机器指令对应到源码、函数和变量。例如 MSVC 的调试符号通常保存在 `.pdb` 文件中。Debug 配置主要让程序更容易调试，不会自动找出所有逻辑错误；Release 也可以配置为保留调试信息。

D3D12 Debug Layer 检查的是图形 API 的使用。程序需要先获取 `ID3D12Debug` 并调用 `EnableDebugLayer()`，然后才能创建 D3D12 设备。可以对照 [main.cpp](<F:/GameDevelop/SomeProjects/GPUVisibilityLab/a1-environment-check/main.cpp>) 中的初始化顺序。

**选择 Debug 构建，不等于自动启用 D3D12 调试层；启用调试层，也不等于在 IDE 中启动了断点调试。** 常见代码会用 `_DEBUG` 条件编译将两者关联起来，但真正启用调试层的仍是那段显式调用。

本关此前出现的 `Debug-queue verification needs investigation`，是检查程序自己输出的验收失败提示。该检查主动写入 INFO 标记再尝试读回；本机诊断发现默认存储过滤器拒绝了 INFO 消息。这检查的是日志消息通道，不是“CLion 能否命中断点”，也不能直接据此认定设备创建失败。

调试层没有警告，不代表渲染算法或视锥剔除数学一定正确。API 使用检查、C++ 断点调试和算法结果验证是不同的工作，需要分别完成。

参考：[MSVC 调试信息](https://learn.microsoft.com/en-us/cpp/build/reference/debug-generate-debug-info?view=msvc-170)、[D3D12 初始化与调试层示例](https://learn.microsoft.com/en-us/windows/win32/direct3d12/creating-a-basic-direct3d-12-component)、[允许所有消息通过的空存储过滤器](https://learn.microsoft.com/en-us/windows/win32/api/d3d12sdklayers/nf-d3d12sdklayers-id3d12infoqueue-pushemptystoragefilter)。

### 五、G8 自测与验收

先尝试不看答案，用自己的话说明下面每种情况应该优先检查哪里。这里列的是排查入口，不是只凭一句报错就能确定的唯一原因。

| 情况或问题 | 应能解释的要点 |
|---|---|
| 新增 `.cpp` 后没有参与构建 | 检查 CMake 的目标源文件配置；不是先重装 MSVC |
| 编译时找不到 `d3d12.h` | 检查 Windows SDK 安装、所选开发环境和头文件搜索路径 |
| 链接时找不到 `D3D12CreateDevice` | 区分编译与链接；检查 `d3d12` 链接配置、库路径和目标架构 |
| HLSL 编译失败 | 看 DXC 的报错、入口函数、目标 Shader Profile 等配置，而不是把 HLSL 交给 MSVC 编译 |
| C++ 编译成功，运行时报告资源状态不匹配 | 阅读 D3D12 Debug Layer 的诊断，定位图形 API 使用问题 |
| 能在 CLion 命中断点，是否证明调试层已开启？ | 不能。Debugger 与 Debug Layer 必须分别验证 |
| 为什么官方三角形能在没有 CMake 配置的情况下构建？ | 它已有 `.vcxproj`，由 MSBuild 执行构建规则，而不是使用检查项目的 CMake |

**验收方式：** 能说明 `CLion -> CMake -> Ninja -> MSVC` 在检查项目中的协作关系；能区分 SDK、CPU 编译器和 Shader 编译器；能解释三个 Debug 概念，并给出一个相应的排查例子。达到这些要求后，再自行勾选 G8；阅读完本附录不自动代表验收完成。

一句话记忆：**CMake 组织构建，Windows SDK 提供接口和工具，DXC 编译着色器，D3D12 Debug Layer 检查图形 API 的使用。**
