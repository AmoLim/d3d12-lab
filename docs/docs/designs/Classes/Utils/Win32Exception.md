---
type: 类设计
status: 草稿
project: ""
module: ""
class_name: ""
inheritance: std::exception (https://en.cppreference.com/cpp/error/exception)
created: 2026-09-28
---

# Win32Exception

关联功能：

暂无

## 当前设计

提供RAII exception对象给ThrowIfFailWin32宏使用
### 数据成员

| 类型       | 成员          | 初值 / 范围 | 含义与所有权              |
| -------- | ----------- | ------- | ------------------- |
| DWORD    | mErrorCode  | Empty   | Error Code；值        |
| uint32_t | mLine       | 0       | 抛出异常表达式对应的在文件中的行数；值 |
| string   | mExpression | NULL    | 抛出异常的表达式；值          |
| string   | mFile       | NULL    | 抛出异常的文件名；值          |
| string   | mMessage    | NULL    | 组装后的异常信息；值          |

### 不变量

| 编号  | 条件                                            | 成立边界      |
| --- | --------------------------------------------- | --------- |
| I1  | 实例生命周期开始后，mErrorCode, mExpression, mFile都为有效值 | 实例生命周期开始后 |

### 接口与生命周期

| 接口        | 行为                   | 前提 / 边界      |
| --------- | -------------------- | ------------ |
| ErrorCode | DWORD；返回错误码          | [I1]；保证不抛出异常 |
| What      | const char*；返回抛出异常信息 | [I1]；保证不抛出异常 |

实现位置 ：Utils/Win32Exception
