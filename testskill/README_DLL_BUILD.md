# SOM C++ Skill DLL Build Notes

这个文档记录当前电脑上生成 SOM 足球 C++ 技能 DLL 的固定流程，方便以后继续让 Codex 或手动生成 DLL。

## 1. 已确认可用的 VS2013 路径

这台电脑的 VS2013 不是装在默认的 `C:\Program Files (x86)\Microsoft Visual Studio 12.0`，而是在：

```bat
D:\Downloads
```

关键文件：

```bat
D:\Downloads\VC\vcvarsall.bat
D:\Downloads\VC\bin\cl.exe
D:\Downloads\VC\bin\link.exe
C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe
```

当前项目使用 `PlatformToolset v120`，所以必须用 VS2013/v120 工具链生成 DLL。

## 2. 项目路径

解决方案根目录：

```bat
D:\huosheng\testskill
```

解决方案文件：

```bat
D:\huosheng\testskill\testskill.sln
```

项目文件：

```bat
D:\huosheng\testskill\testskill\testskill.vcxproj
```

依赖库：

```bat
D:\huosheng\testskill\worldmodel_lib\worldmodel_lib.lib
```

## 3. 生成 Debug DLL 的命令

在 PowerShell 里进入解决方案根目录：

```powershell
cd D:\huosheng\testskill
```

然后运行：

```powershell
cmd.exe /c 'call "D:\Downloads\VC\vcvarsall.bat" x86 && "C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" testskill.sln /t:Rebuild /p:Configuration=Debug /p:Platform=Win32 /m'
```

成功后 DLL 输出到：

```bat
D:\huosheng\testskill\Debug\goalie_timing_shoot.dll
```

当前已经验证过：这个命令可以成功生成 DLL。

## 4. 生成 Release DLL 的命令

```powershell
cd D:\huosheng\testskill
cmd.exe /c 'call "D:\Downloads\VC\vcvarsall.bat" x86 && "C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" testskill.sln /t:Rebuild /p:Configuration=Release /p:Platform=Win32 /m'
```

输出目录通常是：

```bat
D:\huosheng\testskill\Release\
```

## 5. 一次只能启用一个 player_plan

SOM 技能 DLL 的入口函数是：

```cpp
extern "C" __declspec(dllexport) PlayerTask player_plan(const WorldModel* model, int robot_id);
```

一个 DLL 里只能有一个真正启用的 `player_plan`。

所以切换技能文件时：

1. 要生成的 cpp 文件开头改成 `#if 1`。
2. 其它带 `player_plan` 的 cpp 文件开头改成 `#if 0`。
3. 切换后建议用 `/t:Rebuild`，避免旧 `.obj` 残留。

例如要生成官方传球：

```cpp
// official_passball.cpp
#if 1
```

同时确认这些文件仍是 `#if 0`：

```cpp
official_shoot.cpp
official_receiveball.cpp
GetBall.cpp
shootping.cpp
shoottiao.cpp
pass.cpp
sget.cpp
pget.cpp
jie.cpp
jie2ci.cpp
NormalDef.cpp
shoumen.cpp
goalie_timing_shoot.cpp
```

## 6. 修改 DLL 文件名

现在项目的输出名在 `testskill\testskill.vcxproj` 里：

```xml
<TargetName>goalie_timing_shoot</TargetName>
```

如果要生成 `official_passball.dll`，把 Debug 和 Release 两处都改成：

```xml
<TargetName>official_passball</TargetName>
```

也可以在 VS2013 里改：

```text
项目属性 -> 配置属性 -> 常规 -> 目标文件名
```

## 7. 放到仿真软件

生成 DLL 后，一般复制到 SOM 的技能目录：

```bat
D:\SOM v3.4.2 (1)\Team\user_skills
```

然后在 Lua 脚本里把技能名改成 DLL 文件名去调用。

注意：Codex 写 `D:\SOM v3.4.2 (1)` 这种工作区外目录时，可能需要用户授权。

## 8. 常见错误

### 重复定义 player_plan

原因：多个 cpp 同时启用了 `player_plan`。

处理：只保留一个 cpp 是 `#if 1`，其它全部改回 `#if 0`，然后 Rebuild。

### C2014: 预处理器命令必须作为第一个非空白空间启动

原因：cpp 第一行 `#include` 或 `#if` 前面有隐藏字符，常见于 UTF-8 BOM 或复制粘贴。

处理：删除文件开头隐藏字符，确保第一行第一个字符就是 `#`。

### C4819 编码警告

这是 VS2013 对中文编码的警告，一般不影响 DLL 生成。

如果要消除，可以把对应 `.h/.cpp` 保存为 VS2013 能识别的编码，或把编译文件中的中文注释改成英文/ASCII。

### LNK4099 找不到 vc120.pdb

这是 `worldmodel_lib.lib` 缺少调试信息的警告，不影响 DLL 生成。

## 9. 给以后 Codex 的提醒

如果用户让我“生成 DLL”，优先执行：

```powershell
cd D:\huosheng\testskill
cmd.exe /c 'call "D:\Downloads\VC\vcvarsall.bat" x86 && "C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" testskill.sln /t:Rebuild /p:Configuration=Debug /p:Platform=Win32 /m'
```

如果遇到 sandbox/权限导致 `AppData\Local\Microsoft SDKs` 访问失败，重新用提权执行同一条命令。
