---
type: 类设计
status: 草稿
project: ""
module: ""
class_name: App
created: 2026-09-22
---

# App

## 当前设计

### 数据成员

| 类型 | 成员 | 初值 / 范围 | 含义与所有权 |
| --- | --- | --- | --- |
| `bool` | `bClassRegistered_` | `false` | App 是否承担窗口类注销责任 |
| `bool` | `bMinimized_` | `false` | WM_SIZE 更新的最小化状态 |
| `bool` | `bRunning_` | `true` | 是否继续主循环，不代表窗口是否存活 |
| `HINSTANCE` | `hInstance_` | `nullptr` | 借用模块句柄；用于注册/注销窗口类，不卸载模块 |
| `HWND` | `hWnd_` | `nullptr` | App 负责销毁窗口；对外只借用句柄 |
| `int` | `selected_` | `1`；仅 `{1,2,3}` | 选择的唯一来源；标题由此派生 |

### 不变量

| 编号 | 条件 | 成立边界 |
| --- | --- | --- |
| I1 | 请求退出后 `bRunning_` 不再变 true；false 时 HWND 可以仍有效 | 单次运行内 |
| I2 | 创建成功后保存 HWND；WM_NCDESTROY 后清空并解绑 | 创建 API 返回前回调用参数 hwnd，不能依赖成员已赋值 |
| I3 | 窗口销毁后才注销窗口类；注销成功才清除注册标志 | 本例单窗口、独占类名；清理边界 |
| I4 | `selected_` 只取 1/2/3；默认值与显示一致 | 构造完成、消息处理返回时 |
| I5 | GWLP_USERDATA 借用 App 期间，App 存活且地址不变 | 绑定至 WM_NCDESTROY 解绑 |

### 接口与生命周期

| 接口 | 行为 | 前提 / 边界 |
| --- | --- | --- |
| `InitializeWindow()` | 注册窗口类并创建窗口 | 仅初始化一次；失败抛出，保留已获取资源的真实标志 |
| `CleanUp()` | DestroyWindow → UnregisterClassW | 循环停止后调用；失败抛出；成功清理后重复调用为空操作 |
| `IsRunning / IsMinimized / GetSelected / GetHwnd` | 只读查询 | HWND 只借用，窗口销毁后失效 |
| `WndProc` | 绑定 / 查找 App → HandleMessage | 无绑定时默认处理；异常不得穿过系统回调边界 |
| `HandleMessage` | 按键改选择；WM_SIZE 改最小化；关闭 / Esc 停止循环 | WM_DESTROY 兜底停止并 PostQuitMessage；WM_NCDESTROY 清空句柄并解绑 |

- **持有与销毁：** main 持有 App；正常路径先显式清理窗口，再结束 App 寿命。禁止拷贝 / 移动（I5）；异常策略见功能笔记。
- **线程与回调：** 在窗口创建线程调用；创建、销毁窗口也可能同步触发回调。

