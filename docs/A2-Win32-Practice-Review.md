# A2 Win32 Practice 复盘

日期：2026-09-21  
对象：`GPUVisibilityLab/a2-win32-practice`  
范围：纯 Win32 窗口、消息、输入、生命周期与错误处理，不包含 D3D12。

## 1. 复盘结论

这次练习的主要收获，不只是创建了一个窗口，而是把“系统如何调用我的函数”和“窗口如何找到我的 C++ 对象”串成了完整流程。

用户已反馈异常修复完成；本次静态检查确认，先前发现的四处问题均已在当前源码中修正。当前结构适合作为下一阶段 D3D12 清屏的窗口基础，但仍有一个初始状态不一致的问题需要留意。

### 当前发现与边界

| 优先级 | 位置 | 发现 | 影响与建议 |
|---|---|---|---|
| 应修正 | [App.h:44](F:/GameDevelop/SomeProjects/GPUVisibilityLab/a2-win32-practice/App.h:44)、[App.cpp:39](F:/GameDevelop/SomeProjects/GPUVisibilityLab/a2-win32-practice/App.cpp:39) | `selected_` 初始为 `0`，创建窗口时标题却显示 `1` | 启动时界面与真实状态不同，且 `0` 不属于按键允许的 `1/2/3`。按当前练习约定，应统一默认选择为 `1`，最好让标题也由状态生成。不是本次崩溃原因。 |
| 后续完善 | [main.cpp:53](F:/GameDevelop/SomeProjects/GPUVisibilityLab/a2-win32-practice/main.cpp:53) | 异常路径直接调用 `ExitProcess(1)`，不进入正常 `Shutdown()` | 对一次性练习是明确的失败退出策略，系统会回收进程资源；但不能据此证明部分初始化后的主动清理正确。扩展为可重复初始化或可恢复程序时，应补充清理设计。 |
| 理解纠正 | [App.cpp:27](F:/GameDevelop/SomeProjects/GPUVisibilityLab/a2-win32-practice/App.cpp:27)、[App.cpp:65](F:/GameDevelop/SomeProjects/GPUVisibilityLab/a2-win32-practice/App.cpp:65)、[App.cpp:73](F:/GameDevelop/SomeProjects/GPUVisibilityLab/a2-win32-practice/App.cpp:73) | 三处注释表述不准确 | 注册窗口类不是写 Windows 注册表；`GWLP_USERDATA` 存的是 `App*`，不是 `HWND`；`ERROR_SUCCESS` 是零值成功码，不是“不可能的错误码”。代码对应的指针存取和错误判断思路是正确的。 |

**验证范围：** 本次只读取源码、原学习指南并核对微软 API 文档；仅新增这份复盘，没有修改源码、构建配置或原指南，也没有重新构建、运行窗口或执行故障注入。下文“已确认修复”指源码检查，不代表 W1～W8 已全部运行通过。

## 2. 本次已经修复的四个问题

### 2.1 窗口过程没有注册：本次异常的首要原因

原现象：

```text
Exception 0xc000041d encountered at address 0x000000
```

旧代码使用 `WNDCLASSW wndClass{};` 初始化结构体，但遗漏了 `lpfnWndProc`，导致窗口过程指针保持为空。仅仅写出名为 `WndProc` 的函数，不会自动把它交给 Windows。

当前 [App.cpp:23](F:/GameDevelop/SomeProjects/GPUVisibilityLab/a2-win32-practice/App.cpp:23) 已补充：

```cpp
wndClass.lpfnWndProc = WndProc;
```

与旧代码和零地址最吻合的解释是：创建窗口期间，系统需要调用窗口过程，却遇到了空回调地址。`0xC000041D` 表示用户回调发生未处理异常，并不直接说明最初是哪一类异常。没有当时的首次异常记录和调用栈，不能把“空地址调用的完整运行过程”写成已经动态验证的事实。[WNDCLASSW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassw)、[NTSTATUS 状态码](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-erref/596a1078-e883-4972-9bbc-49e60bebca55)

应形成的检查习惯：结构体清零只是可靠的起点，不意味着必填字段已经有效。注册前要检查回调地址、类名和模块句柄。

### 2.2 GetMessage 的错误分支写反

旧代码用 `result == 1` 判断失败，会把正常消息当成错误，并漏掉真正的 `-1`。

当前 [main.cpp:29](F:/GameDevelop/SomeProjects/GPUVisibilityLab/a2-win32-practice/main.cpp:29) 已改为：

```cpp
if (result == -1) CheckWin32(FALSE, "GetMessageW");
```

| 返回值 | 含义 | 当前应走的路径 |
|---|---|---|
| 正数 | 取得普通消息 | 翻译、分发 |
| `0` | 取得 `WM_QUIT` | 退出循环 |
| `-1` | 调用失败 | 读取错误并报告 |

这里的返回类型虽然叫 `BOOL`，却不是只需区分真假。不能直接把 `GetMessageW` 的结果交给通用 `CheckWin32`，因为 `-1` 也是非零值。[GetMessageW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmessagew)

这是独立于空回调的第二个问题，不应把两个异常来源混在一起。

### 2.3 标题使用了旧选择值

旧逻辑先用 `shownSelection` 生成标题，再更新该变量，导致标题落后于真实选择。

当前 [main.cpp:43](F:/GameDevelop/SomeProjects/GPUVisibilityLab/a2-win32-practice/main.cpp:43) 已从 `app.GetSelected()` 生成标题，再同步 `shownSelection`。这让两个变量的职责明确起来：

- `selected_`：应用当前的真实选择。
- `shownSelection`：上一次已经反映到标题和日志的选择，用来避免重复更新。

这次修复解决了按键后的更新顺序，但不自动解决第 1 节提到的启动默认值不一致。

### 2.4 创建窗口后缺少就地检查

当前 [App.cpp:51](F:/GameDevelop/SomeProjects/GPUVisibilityLab/a2-win32-practice/App.cpp:51) 已补充：

```cpp
CheckWin32(hwnd_ != nullptr, "CreateWindowExW");
```

窗口创建失败现在会在创建位置被报告，而不是等后面的 `GetClientRect` 使用无效句柄时才暴露。应在 API 返回失败后尽快取得错误码，避免后续调用干扰诊断。[CreateWindowExW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexw)

注意：返回值检查只能处理“函数返回失败”的情况，不能替代对空回调、非法内存访问等异常的修复。

## 3. 用自己的代码串起完整流程

### 文件职责

| 文件 | 当前职责 | 评价 |
|---|---|---|
| [CMakeLists.txt](F:/GameDevelop/SomeProjects/GPUVisibilityLab/a2-win32-practice/CMakeLists.txt) | Windows/MSVC/x64 限制、C++20、Unicode、警告级别、链接 user32 | 配置意图清楚；`main` 中另有 Debug 运行检查。配置要求不等于本次已验证实际构建产物。 |
| [main.cpp](F:/GameDevelop/SomeProjects/GPUVisibilityLab/a2-win32-practice/main.cpp) | 初始化、消息循环、标题同步、正常退出与失败报告 | 生命周期集中，方便设断点追踪。 |
| [App.h](F:/GameDevelop/SomeProjects/GPUVisibilityLab/a2-win32-practice/App.h) | 窗口句柄、运行/最小化/选择状态、接口声明 | 状态放在实例中，不依赖全局变量。 |
| [App.cpp](F:/GameDevelop/SomeProjects/GPUVisibilityLab/a2-win32-practice/App.cpp) | 创建窗口、回调转发、消息处理、窗口清理 | 静态回调与实例处理分离，拆分适度。 |
| [utilities.h](F:/GameDevelop/SomeProjects/GPUVisibilityLab/a2-win32-practice/utilities.h) | 将明确的 Win32 失败转换为带操作名和错误码的异常 | 简单有效，但调用方必须先理解各 API 的返回值。 |

原指南附录 D 是单文件参考。你拆成多个文件并不是偏离目标；关键是拆分后仍能解释执行顺序，而不是为了封装增加层次。

### 启动、交互与结束

```text
main 创建 App 对象
  -> InitializeWindow
       -> 获取模块句柄
       -> 设置并注册窗口类（包括 WndProc）
       -> 根据目标客户区计算外框尺寸
       -> CreateWindowExW(..., this)
            -> 创建期间回调 WndProc
            -> WM_NCCREATE 中把 App* 存入窗口用户数据
       -> 函数返回，结果赋给 hwnd_
       -> 检查创建结果
  -> GetClientRect 核对客户区，ShowWindow 显示窗口
  -> 消息循环
       -> GetMessageW
       -> TranslateMessage / DispatchMessageW
       -> WndProc 找到 App*，转到 HandleMessage
       -> 状态改变后，main 更新标题和日志
  -> 收到退出请求，running_ 变为 false
  -> Shutdown：DestroyWindow，再 UnregisterClassW
  -> 输出正常关闭日志，main 返回 0
```

这张图是主流程，不代表所有回调都只发生在 `DispatchMessageW` 中。创建、销毁以及其他同步窗口操作也可能在 API 返回前调用窗口过程。[CreateWindowExW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexw)、[DestroyWindow](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroywindow)

## 4. 需要真正掌握的五个知识点

### 4.1 静态回调是入口，App* 才决定操作哪个对象

`WndProc` 是静态成员函数，没有普通成员函数隐含的 `this` 参数，适合提供给 Win32 作为回调。`HandleMessage` 是普通成员函数，可以直接更新当前对象的成员。

当前对象的传递路线是：

```text
main 中的 &app
  -> InitializeWindow 中的 this
  -> CreateWindowExW 的最后一个参数
  -> WM_NCCREATE 的 CREATESTRUCTW::lpCreateParams
  -> SetWindowLongPtrW(hwnd, GWLP_USERDATA, App*)
  -> 后续消息通过 GetWindowLongPtrW 取回 App*
  -> app->HandleMessage(...)
```

Windows 保存的是一个借用的对象地址，不复制 `App`，也不替你管理其寿命。当前 `App app` 活到正常 `Shutdown()` 完成之后，因此销毁窗口期间的回调仍能访问它。这符合窗口实例关联应用状态的基本方式。[管理应用状态](https://learn.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-)

创建期间还有一个容易忽略的时序：`CreateWindowExW` 尚未返回时，赋值给成员 `hwnd_` 的动作还没完成。回调应使用参数中的 `hwnd`；当前实现就是这样做的。

### 4.2 零返回值不总是失败

`SetWindowLongPtrW` 返回的是旧值；第一次绑定时旧用户数据通常就是零。当前代码先 `SetLastError(ERROR_SUCCESS)`，再检查“返回零且错误码非零”，这是正确做法。清零是为了排除旧错误残留，不是设置一个特殊的不可能值。[SetWindowLongPtrW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowlongptrw)

把这一点和 `GetMessageW` 放在一起记：**先读返回值契约，再选择检查方式；不要根据类型名字猜成功条件。**

### 4.3 消息通知与应用状态分工明确

| 消息 | 当前行为 | 不应误解为 |
|---|---|---|
| `WM_PAINT` | `BeginPaint`、填充待绘制区域、`EndPaint` | GPU 持续渲染或每帧回调 |
| `WM_SIZE` | 记录是否最小化 | 立即重建交换链；当前根本没有交换链 |
| `WM_KEYDOWN` | 主键盘 `1/2/3` 更新选择，Esc 请求退出 | 全局键盘监听 |
| `WM_CLOSE` | 只把 `running_` 设为 false | 窗口已经销毁 |
| `WM_DESTROY` | 标记停止并调用 `PostQuitMessage(0)` | 自动释放所有应用资源 |
| 未处理的消息 | 交给 `DefWindowProc` | 可以无条件返回零忽略 |

`BeginPaint` 与 `EndPaint` 应配对。当前 GDI 填充只是为纯窗口提供背景，不是 D3D12 清屏；数字键的验收证据是状态、标题和日志，不是客户区变色。[BeginPaint](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-beginpaint)

### 4.4 退出请求、销毁窗口、退出消息不是同一件事

按当前设计，常见关闭路线是：

```text
关闭按钮 / Alt+F4 -> WM_CLOSE -> running_ = false
Esc              -> WM_KEYDOWN -> running_ = false

主循环发现停止
  -> Shutdown
  -> DestroyWindow
       -> WM_DESTROY -> PostQuitMessage(0)
  -> 注销窗口类
  -> main 返回
```

因此，正常关闭时主循环可能早已退出，之后才在销毁回调中投递 `WM_QUIT`。这不是必须再取一次消息才能结束的错误；当前程序用 `running_` 主动退出循环，同时保留了 `GetMessageW == 0` 的退出路径。

这种“回调提出退出请求，主流程统一清理”的设计值得保留，之后可以在销毁窗口前插入 GPU 等待与图形资源释放。

### 4.5 标准 C++ 异常处理不是所有故障的兜底

`CheckWin32` 抛出的 `std::runtime_error` 可以由主流程中的 `catch(std::exception&)` 处理。但不能指望这个 catch 自动兜住非法地址访问，也不应让 C++ 异常穿越 Windows 窗口回调边界。窗口过程的未处理异常行为还与平台和系统有关。[WNDPROC](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-wndproc)

当前消息处理逻辑没有主动调用抛异常的 `CheckWin32`；`WM_NCCREATE` 绑定失败时也直接返回 `FALSE`。后续若在回调中加入可能抛异常的操作，应在回调边界内处理并记录失败，再让主流程结束，而不是仅依赖 `main` 的 catch。

## 5. 排错方法复盘

这次最有效的路径是从“零地址 + 回调异常”回到窗口类注册，而不是先怀疑显卡、D3D12 或编译器。当前练习没有图形设备与交换链，GPU 排查不是优先方向。

下次遇到类似问题，按下面的顺序缩小范围：

1. **确认崩溃阶段。** 区分创建窗口、显示窗口、取消息、分发消息、关闭清理，不只记录最后一个错误码。
2. **看最早的异常和调用栈。** 最后报告的回调异常可能是外层结果；优先保存最初的异常类型和自己的代码位置。
3. **检查 API 契约。** 输入结构体的必填字段、回调地址、返回值含义、对象寿命分别核对。
4. **使用少量关键断点。** 在创建前、`WM_NCCREATE`、创建后设断点，比每条窗口消息都停一次更有效。
5. **一次解决一个原因。** 空回调、消息判错和标题滞后是不同问题，修完一个还要走完整流程。
6. **保留证据。** 记录日志、退出码、状态变量和调用顺序，不把“窗口弹出来了”当作全部通过。

## 6. 运行验收清单

以下依据原指南第 4.12 节整理。本次未执行这些运行实验，全部保留待验收，不修改原指南已有勾选。

| 项目 | 操作与观察 | 状态 |
|---|---|---|
| W1 创建顺序 | 创建前、`WM_NCCREATE` 内、创建后各停一次；确认回调发生时成员 `hwnd_` 尚未由返回值赋值 | 待运行 |
| W2 指针传递 | 比较 `&app`、`this`、`lpCreateParams` 和后续取回的指针；应指向同一个对象 | 待运行 |
| W3 客户区 | `GetClientRect` 后核对宽 1280、高 720；不要把含标题栏的外框尺寸当成客户区 | 待运行 |
| W4 输入 | 先核对启动标题与内部选择，再按主键盘 `1 -> 2 -> 3 -> 1`；标题、状态、日志一致；移走焦点后不应继续响应目标窗口按键 | 初始值不一致已静态发现；交互待运行 |
| W5 最小化 | 最小化与恢复时检查 `minimized_` 变化，并确认恢复后可继续输入 | 待运行 |
| W6 关闭 | 分三次测试关闭按钮、Esc、Alt+F4；均应输出 `[INFO] Normal Shutdown.`，原生退出码为 0 | 待运行 |
| W7 失败报告 | 原指南建议在独立练习中临时使用未注册类名，验证创建失败日志且不进入主循环，再恢复并复测；这需要临时改动，由学习者单独进行 | 本次未执行、未改代码 |
| W8 两类循环 | 解释无输入时 `GetMessageW` 等待的合理性，以及持续渲染为何不能依赖它不断返回 | 待自测 |

建议另做一次遮挡后恢复显示的检查，确认背景能正确重绘。相同数字连续按下时，当前程序不会重复打印选择日志，这是 `shownSelection` 去重的预期行为。

## 7. 进入 D3D12 前的交接

不需要把这个练习扩展成通用 GUI 框架。先统一初始选择并完成窗口验收，再回到 [A2 技术指南](F:/GameDevelop/SomeProjects/GPUVisibilityLab/docs/A2-Window-And-Clear.md) 的图形部分。

| 当前应保留 | 下一阶段的变化 |
|---|---|
| `HWND` 创建、静态回调、实例状态绑定 | 创建窗口后初始化图形对象，再显示窗口 |
| 回调只记录输入和退出意图 | 数字键修改下一帧使用的清屏色 |
| `running_`、`minimized_` | 渲染前判断是否退出或最小化 |
| 主流程统一 `Shutdown` | 停止提交、等待 GPU、释放图形资源、最后销毁窗口 |
| 明确的错误操作名 | 区分 Win32 错误检查与 D3D12 的 HRESULT 检查 |

纯窗口使用 `GetMessageW` 阻塞等待是合适的。连续渲染阶段按指南改为 `PeekMessageW` 排空消息后推进一帧，并在最小化时避免空转；不是简单地把一个 API 名称替换成另一个。[PeekMessageW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-peekmessagew)

### 离开本练习前能回答的问题

1. 为什么定义了 `WndProc`，还必须填写 `lpfnWndProc`？
2. 为什么创建窗口的函数还没返回，自己的回调就已经执行了？
3. `HWND` 和 `App*` 分别指什么，谁负责其生命周期？
4. 为什么 `SetWindowLongPtrW` 返回零可能成功，而 `GetMessageW` 返回非零也可能失败？
5. 为什么关闭请求可以只改标志位，而不在回调里直接做全部清理？
6. 为什么窗口背景已经画出来，并不能证明 D3D12 已经工作？

**本次复盘的核心结论：窗口程序不是从上到下只执行一次的函数列表，而是“主流程管理生命周期，系统回调驱动状态变化”。你已经把这两条路径接起来；下一步是用运行证据确认它们，再接入 GPU 命令流程。**
