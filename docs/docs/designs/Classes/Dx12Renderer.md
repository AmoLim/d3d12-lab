---
type: 类设计
status: 草稿
project: ""
module: ""
class_name: ""
created: "2026-09-26"
---

# Dx12Renderer

关联功能：[[A2持续清屏]]

<!-- 首次设计：当前设计写“暂无”，将下面的设计表移到本次变更填写候选方案。 -->

## 当前设计

职责 ：初始化DX12图形库入口

### 数据成员


| 类型                                  | 成员                   | 初值 / 范围 | 含义与所有权                        |
| ----------------------------------- | -------------------- | ------- | ----------------------------- |
| ComPtr\<IDXGIFactory4>              | mdxgiFactory         | nullptr | 保存DXGIFactory的COM接口指针；拥有      |
| ComPtr\<ID3D12Device5>              | md3dDevice           | nullptr | 保存d3d12设备的COM接口指针；拥有          |
| ComPtr\<IDXGISwapChain4>            | mSwapChain           | nullptr | 保存交换链；拥有                      |
| ComPtr\<ID3D12Fence>                | mFence               | nullptr | 保存GPU已经完成Fence的COM接口指针；拥有     |
| ComPtr\<ID3D12GraphicsCommandList6> | mCommandList         | nullptr | 保存CPU侧CommandList的COM接口指针；拥有  |
| ComPtr\<ID3D12CommandQueue>         | mCommandQueue        | nullptr | 保存GPU侧CommandQueue的COM接口指针；拥有 |
| ComPtr\<ID3D12CommandAllocator>     | mCommandAllocator    | nullptr | 命令的底层存储COM指针接口；拥有             |
| static constexpr UINT               | SwapChainBufferCount | 2       | 交换链缓冲区的数量；值                   |
| unique_ptr\<CpuDescriptorHeap>      | mRtvHeap             | nullptr | 类型为Rtv的描述符堆；拥有                |
| unique_ptr\<CpuDescriptorHeap>      | mDsvHeap             | nullptr | 类型为Dsv的描述符堆；拥有                |
| array of ComPtr\<ID3D12Resource>    | mSwapChainBuffer     | nullptr | 交换链缓冲区资源的COM接口指针；拥有           |
| ComPtr\<ID3D12Resource>             | mDepthStencilBuffer  | nullptr | 深度/模板缓冲区资源的COM接口指针；拥有         |
| D3D12_VIEWPORT                      | mViewPort            | DEFAULT | ViewPort数据结构体；拥有              |
| UINT                                | mCurrBackBuffer      | 0       | 缓存的该帧内的交换链BackBuffer对应的index  |


### 不变量

| 编号  | 条件                         | 成立边界         |
| --- | -------------------------- | ------------ |
| I1  | 在Ready下，所有的COM指针、智能指针都应为非空 | 实例处于Ready状态下 |

### 接口、函数与生命周期

| 接口                       | 行为                                                                     | 前提 / 边界                                           |
| ------------------------ | ---------------------------------------------------------------------- | ------------------------------------------------- |
| GetCurrentBackBuffer     | ID3D12Resource*；获取当前交换链BackBuffer的观察指针                                 | 当前RAII实例存在；mCurrBackBuffer < SwapChainBufferCount |
| GetCurrentBackBufferView | CD3DX12_CPU_DESCRIPTOR_HANDLE；获取当前mCurrBackBuffer对应的 Descriptor Heap句柄 | 当前RAII实例存在；mCurrBackBuffer < SwapChainBufferCount |
| GetDepthStencilView      | CD3DX12_CPU_DESCRIPTOR_HANDLE；获取DSV Descriptor Heap的句柄                 | 当前RAII实例存在                                        |

| 函数签名                 | 可见性     | 行为                                 | 预期结果                                                                      |
| -------------------- | ------- | ---------------------------------- | ------------------------------------------------------------------------- |
| CreateDevice         | private | 创建DxgiFactory与D3D12Device          | 成功后factory, device指针非空                                                    |
| CreateFence          | private | 创建Fence对象，将CPU侧Fence计数置零           | 成功后mFence指针非空                                                             |
| CreateCommandFlow    | private | 创建Command Queue、Allocator、List     | 成功后mCommandQueue、mCommandAllocator、mCommandList指针非空；mCommandList处于Close状态 |
| CreateSwapChain      | private | 创建交换链                              | 成功后SwapChain指针非空                                                          |
| CreateDescriptorHeap | private | 创建两种类型的CPUDescriptorHeap RAII实例    | 成功后两种DescriptorHeap指针非空                                                   |
| LinkRtvWithResource  | private | 获取SwapChainBackBuffer 资源缓存，并且建立RTV | 成功后数组array of ComPtr\<ID3D12Resource>中的各个元素非空                             |
| CreateDsvResource    | private | 创建Depth/stencil Buffer并且建立DSV      | 成功后mDepthStencilBuffer非空                                                  |
| SetupViewport        | private | 设置好视口参数                            |                                                                           |

==注意CreateSwapChain函数中的CreateSwapChainForHwnd第一个参数传入的是CommandQueue==

- 持有与销毁：谁拥有本对象；借用有效期；释放时机。
- 拷贝 / 移动、线程 / 回调约束：按需填写。

实现位置 / 已验证版本 / 证据：

## 本次变更

目标：
成员 / 不变量 / 接口变化：只写新增或受影响的项；表格沿用上面的列。

```text
待填：操作前提 → 具体修改 → 结果 / 失败状态
```

### 验收案例

验收位置：[Dx12RendererTests.cpp](../../../../a2-window-clear/tests/Dx12RendererTests.cpp)。共 8 个 GoogleTest 用例，不超过 12 个；统一套件名为 `Dx12RendererDeathTest`。

验收前提：当前仅公开构造、析构和三个 getter，没有单独的初始化入口。因此本表按“有效 HWND 构造成功后进入 Ready”的 RAII 契约描述预期，而不是将当前未初始化的状态当作正确结果。若后续改为显式初始化接口，需要同步调整用例。

| 状态   | 场景 / 用例名                                           | 操作与预期行为                                                                                                   | 本次实际结果                                   |
| ---- | -------------------------------------------------- | --------------------------------------------------------------------------------------------------------- | ---------------------------------------- |
| [x]  | 构造与观察指针：`ConstructorProvidesStableBackBuffer`      | 创建有效隐藏窗口并构造实例；当前 back buffer 非空，同一帧连续读取返回同一观察指针；验证 I1 的公开可观察部分                                            | Debug / Release 均通过                      |
| [x]  | 资源描述：`BackBufferMatchesConfiguredSizeAndFormat`    | 当前 back buffer 为 Texture2D，宽高对应 `A2WindowClear::WIDTH/HEIGHT`，格式为 `R8G8B8A8_UNORM`，单采样并允许作为 render target | Debug / Release 均通过，本次尺寸为 1280 x 720     |
| [x]  | RTV getter：`CurrentRtvHandleIsValidAndStable`      | Ready 后 RTV CPU 句柄非零；未推进帧时重复读取结果一致                                                                        | Debug / Release 均通过                      |
| [x]  | DSV getter：`DepthStencilHandleIsValidAndStable`    | Ready 后 DSV CPU 句柄非零；重复读取结果一致                                                                             | Debug / Release 均通过                      |
| [x]  | 正常销毁：`DestructionCompletesBeforeWindowDestruction` | 窗口仍存活时让 renderer 离开作用域；析构不崩溃、不抛出，且不销毁借用的 HWND                                                             | Debug / Release 均正常退出，HWND 仍有效           |
| [x]  | 重建：`CanRecreateRendererForSameWindow`              | 同一 HWND 上顺序构造、销毁两次 renderer，每次都能获取 back buffer，旧实例不妨碍重建                                                   | Debug / Release 均完成两轮构造和销毁               |
| [x]  | RAII 资源释放：`ReleasesOwnedBackBufferAtScopeExit`     | 给 back buffer 附加生命周期标记，不保留额外资源引用；实例存在时标记有效，实例析构后标记失效                                                      | Debug / Release 均观察到标记随当前 back buffer 释放 |
| [x]  | 异常展开：`DestructionIsSafeDuringExceptionUnwinding`   | 构造后主动抛出测试异常；析构安全完成，调用方捕获原异常，HWND 仍有效                                                                      | Debug / Release 均捕获预期异常并正常退出             |

环境 / 运行入口 / 版本 / 结果与证据：

- 2026-09-30，Windows x64，MSVC 19.38，Windows SDK 10.0.22621.0，GoogleTest v1.17.0；Debug、Release 均构建成功，两种配置最终执行结果均为 **8/8 通过、0 失败、0 跳过**。
- 本地 XML 结果：`a2-window-clear/build-gtest/dx12-renderer-debug.xml`、`a2-window-clear/build-gtest/dx12-renderer-release.xml`（构建产物，不提交）。现有 `CpuDescriptorHeap` Debug 回归测试仍为 **8/8 通过**。
- 每个场景在 `EXPECT_EXIT` 子进程内创建真实但不显示的 Win32 窗口，要求正常退出码 0；崩溃不会中断其他用例。套件名中的 `DeathTest` 仅表示使用 GoogleTest 的进程隔离机制，不表示期望 renderer 崩溃。子进程内部断言失败也会转换成非零退出码。
- 这些是公开接口驱动的类级集成测试：没有访问 private、没有注入伪造 Ready 状态，也没有直接调用私有初始化函数。测试不强制 WARP；运行环境需满足 renderer 对图形调试组件和 D3D feature level 12_2 的要求。本机满足本轮构造所需条件，但测试通过不证明 Debug Layer 已实际启用。
- 本轮实现变化：初测暴露了构造尚未初始化的问题；用户接通 `Initialize()` 后，又在 `CreateSwapChainForHwnd` 处遇到 `0x887A0001`。用户随后将首参数改为 `mCommandQueue.Get()`，最终 Debug / Release 的 8 个用例全部通过。上述中间失败已被最终报告替代，本次测试工作没有修改 renderer 实现。D3D12 的该参数要求 Direct command queue，依据：[Microsoft API 参数说明](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgifactory2-createswapchainforhwnd)。
- 覆盖边界：不宣称已验证全部私有 COM 成员、实际 RTV/DSV 绑定内容、深度资源状态、视口命令、GPU 队列完成等待、Present/Resize 或初始化中途失败回滚。无效 HWND 不属于本轮用例。非零 CPU 句柄只能验证公开返回值，不能替代实际绘制验收。

从 `a2-window-clear` 目录运行：

```powershell
cmake --build build-gtest --config Debug --target dx12_renderer_tests
.\build-gtest\tests\Debug\dx12_renderer_tests.exe --gtest_output=xml:build-gtest/dx12-renderer-debug.xml
```

CLion 中重新加载 CMake 后，使用 `dx12_renderer_tests` Google Test 配置，或从各 `TEST` 左侧按钮单独运行。后续实现变更后需重新运行；上表通过仅对应上述明确范围，不代表完整渲染流程已验收。


## 后续考虑

| 触发条件 | 再考虑的变化 |
| --- | --- |
| 待填 | 待填 |
