---
type: 类设计
status: 草稿
project: ""
module: ""
class_name: ""
created: "2026-09-30"
---

# FenceEvent

## 当前设计

为CPU与GPU同步封装的管理Win32 Event Handle的RAII类，生命周期结束自动CloseHandle
### 数据成员

| 类型     | 成员      | 初值 / 范围 | 含义与所有权         |
| ------ | ------- | ------- | -------------- |
| Handle | mHandle | nullptr | Win32的事件句柄；拥有  |

### 不变量

| 编号  | 条件                 | 成立边界   |
| --- | ------------------ | ------ |
| I1  | mHandle != nullptr | 生命周期开始 |

### 接口与生命周期

| 接口   | 行为                                       | 前提 / 边界              |
| ---- | ---------------------------------------- | -------------------- |
| void | Wait；传入mFence 与 Cpu侧FenceValue，监听GPU完成事件 | I1；mFence != nullptr |

- 拷贝 / 移动 ： 删除拷贝构造与拷贝赋值，保留移动语义

实现位置：[Core/Dx12/FenceEvent]

#### 伪代码

```cpp
FenceEvent() {
	mHandle = CreateHandle(...)
}

~FenceEvent() {
	if (mHandle != nullptr) {
		CloseHandle
	}
}

FenceEvent(&&) : mHandle(nullptr) {
	mHandle = other.mHandle;
	other.mHandle = nullptr;
}

operator =(&&) {
	if (this != &other) {
		if (mHandle != nullptr) {
			CloseHandle
		}
		
		mHandle = other.mHandle;
		other.mHandle = nullptr;
	}
	return this;
}

void Wait(DX12_FENCE mFence, UINT fenceValue) {
	assert(mFence != nullptr);
	
	if (mFence->CurrentFenceValue < fenceValue) {
		
		ThrowIfFailed(
			mFence -> SetEventOnCompleted
		)
		
		ThrowIfFailedWin32(
			WaitForSingleObject(...)
		)
	}
}
```
### 验收案例

GPT generated

| 状态  | 场景              | 预期行为     |
| --- | --------------- | -------- |
| [ ] | 正常操作序列：待填       | 行为与关联不变量 |

## 后续考虑

| 触发条件              | 再考虑的变化 |
| ----------------- | ------ |
| 需要抽象通用的Win32Event | 。。。    |
