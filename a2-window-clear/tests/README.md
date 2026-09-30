# GoogleTest Tests

测试直接使用 GoogleTest，不再接入 CTest。每个 `TEST` / `TEST_F` 是一个独立用例。不同类的测试放在各自文件中，以注释分隔板块。

## 目录与范围

| 文件 | 测试板块 |
| --- | --- |
| `CpuDesciptorHeapTests.cpp` | 创建、getter、CPU 句柄、RAII 四个板块，每个板块两个用例 |
| `Dx12RendererTests.cpp` | 8 个公开接口和生命周期用例；独立目标 `dx12_renderer_tests`，使用隐藏窗口和子进程隔离 |
| `D3D12TestFixture.h` | 每个测试执行前创建独立 WARP 设备，由 ComPtr 自动释放 |

`CpuDescriptorHeap` 共 8 个用例。前三个板块各自覆盖容量 1 / 8：创建板块检查原生容量、类型和非 shader-visible 标志；getter 板块检查返回的容量和类型；句柄板块检查全部合法索引的偏移，包含首尾边界。RAII 板块检查正常离开作用域及异常展开时释放原生堆，每个用例均覆盖 RTV / DSV。断言使用 `ASSERT_*` / `EXPECT_*`，Release 下仍然有效。GoogleTest 提供测试入口、筛选、失败位置和结果统计，不再维护自定义 `main()` 或用例列表。

RAII 测试使用 `SetPrivateDataInterface` 给原生堆附加 COM 生命周期标记，外部仅保存标记令牌的 `weak_ptr`。作用域内令牌有效，退出后失效，验证的是底层堆释放，而非仅仅验证代码没有崩溃。测试不保留额外的堆引用，也不访问悬空指针。这两个用例覆盖唯一持有者的析构，不涵盖拷贝、移动或外部额外持有 COM 引用的情况。依据：[Microsoft SetPrivateDataInterface 文档](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12object-setprivatedatainterface)。

`CpuDescriptorHeap` 测试需要 Windows、MSVC x64、Windows SDK 和可用的 D3D12 WARP，不要求独立显卡或 Debug Layer。由于使用真实 D3D12 软件设备，这些仍属于类级集成测试；切换测试框架不会改变这一点。设备初始化失败会导致对应测试失败，不会静默跳过。

`Dx12Renderer` 测试需要真实 Win32 窗口环境，不复用 WARP 夹具，也不修改私有状态。它按构造后 Ready 的预期验收，并需要满足 renderer 自身要求的调试组件和 feature level 12_2。2026-09-30 用户修正初始化和交换链参数后，最终 Debug / Release 均为 8/8 通过。详细场景、结果和未覆盖范围见 [Dx12Renderer 验收案例](../../docs/docs/designs/Classes/Dx12Renderer.md#验收案例)。

不直接访问私有成员，不覆盖非法类型、零容量、空设备或越界索引等违反接口前提的输入。

## 在 CLion 中运行

1. 打开 `a2-window-clear` 主工程，沿用 MSVC x64 工具链。
2. 确认当前 CMake Profile 未设置 `-DBUILD_TESTING=OFF`，然后 Reload CMake Project。
3. 首次配置会下载固定版本 GoogleTest v1.17.0；需要能访问 GitHub，不需要全局安装。
4. 在测试文件中点击 `TEST_F` 左侧运行或调试按钮；运行整个 `cpu_descriptor_heap_tests` Google Test 配置可执行全部用例。

CLion 自动管理 Google Test 运行配置，不需要为每个用例手动创建配置。旧的 CTest 配置可以忽略或在 IDE 中移除，CTest 插件不再是前提。

`BUILD_TESTING` 现在只是本工程自定义的测试构建开关，不会将本工程测试注册到 CTest。第三方依赖可能在自己的构建目录生成空的 CTest 文件，不影响直接运行 GoogleTest。测试复用主工程的 DirectX-Headers，不需要额外设置头文件路径。

参考：[GoogleTest CMake 接入](https://google.github.io/googletest/quickstart-cmake.html)、[CLion Google Test 支持](https://www.jetbrains.com/help/clion/creating-google-test-run-debug-configuration-for-test.html)。本工程仅采用 GoogleTest 接入，不采用官方示例中的 CTest 注册步骤。

## 命令行运行

从 `a2-window-clear` 目录执行，使用新的构建目录，避免原有 CTest 构建产物造成混淆：

```powershell
cmake -S . -B build-gtest -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build build-gtest --config Debug --target cpu_descriptor_heap_tests
.\build-gtest\tests\Debug\cpu_descriptor_heap_tests.exe
```

只运行句柄板块或查看用例列表：

```powershell
.\build-gtest\tests\Debug\cpu_descriptor_heap_tests.exe --gtest_filter="CpuDescriptorHeapHandleTest.*"
.\build-gtest\tests\Debug\cpu_descriptor_heap_tests.exe --gtest_list_tests
```

同一被测类的测试板块放在同一个文件内，用注释分隔并说明范围。在已有文件内新增 `TEST_F` 不需要改 CMake；新增其他类的测试文件时，再加入本目录 `CMakeLists.txt` 的目标源文件列表。

单独构建和执行 renderer 的 8 个用例：

```powershell
cmake --build build-gtest --config Debug --target dx12_renderer_tests
.\build-gtest\tests\Debug\dx12_renderer_tests.exe
```

`Dx12RendererDeathTest` 使用 `EXPECT_EXIT` 要求子进程正常退出，而不是把崩溃当作通过；内部 GoogleTest 断言失败会传播为非零退出码。CLion 可以自动运行各用例，进入子进程内部调试可能需要附加到子进程。
