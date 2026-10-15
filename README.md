# EndfieldNoLauncher

[简体中文](README.md) | [English](README.en.md)

在 Windows x64 上启动《明日方舟：终末地》，不打开官方鹰角启动器界面。

本项目是 [Better-Endfield](https://github.com/Dr-hydra/Better-Endfield) 的功能提取，适合那些不喜欢乱七八糟额外功能的人。

Linux + Steam + DWProton 也可以正常使用。

> [!WARNING]
> 本仓库绝大部分内容都由LLM生成，不对代码质量，文档质量等做出任何明示或暗示的保证。

## 适用范围

做：把游戏启动起来，使其通过父进程检查。

不做（有意为之）：模型替换、相机、MMD、语音路由、界面修改、战斗统计、Steam 集成、模块加载——即本项目内核所源自的那个项目的其他全部功能。本程序没有注入路径，也没有模块加载器。

## 运行要求

- Windows 10/11 x64 或者 Steam + DWProton
- 已安装且签名有效的官方国服鹰角启动器。缺失时本工具以退出码 7 结束，不会启动任何东西
- 游戏可执行文件必须名为 `Endfield.exe`
- 提权：游戏本身要求管理员权限，因此会出现正常的 UAC 提示，允许它是流程的一部分

## 用法

下面是在 Windows 上的步骤。在 Linux 上用 Steam + Proton 运行请看[在 Steam + Proton 上运行](docs/running-on-proton.md)，大部分用法相同。

### 把程序放好

`EndfieldNoLauncher.exe` 不需要安装，放在哪个文件夹都行。建议专门建一个，比如 `D:\NoLauncher\`，把它丢进去。

### 找到游戏路径

本程序需要游戏 `Endfield.exe` 的完整路径。最可靠的找法是先把游戏正常跑起来：按 `Ctrl + Shift + Esc` 打开任务管理器，在「详细信息」里找到 `Endfield.exe`，右键它 →「打开文件所在位置」，点一下弹出的文件夹的地址栏就能看到完整路径。

记下来，例如：

```
D:\Hypergryph\Games\Endfield\Endfield.exe
```

> 文件名必须正好是 `Endfield.exe`。如果你找到的是 `Games.exe`，那是启动器，不是游戏。

### 第一次运行：用管理员身份的 cmd

第一次请走命令行，因为只有这样才能看到出错信息；直接双击的话窗口一闪就没了。

游戏本身要求管理员权限，所以本程序也必须在管理员身份下运行，否则会以 `创建游戏进程 / Create game process (Win32 740)` 失败。

按 `Win` 键 → 输入 `cmd` → 在「命令提示符」上右键 →「以管理员身份运行」→ UAC 点「是」。然后在黑窗口里输入（路径换成你自己的）：

```cmd
cd /d D:\NoLauncher
EndfieldNoLauncher.exe --game "D:\Hypergryph\Games\Endfield\Endfield.exe"
```

引号要保留，路径里可能有空格。

一切正常的话，游戏会直接启动，鹰角启动器的界面不会出现。游戏起来以后那个黑窗口会一直留着，等游戏退出才关——这是正常的，它在等游戏结束并转发游戏的退出码。

`--parent` 一般不用自己填：省略时本程序会自己找到已安装的官方启动器，从最高版本开始逐个校验签名，使用第一个通过的；都没有通过时打印提示并以退出码 7 结束。

结束后看一眼退出码，反馈问题时用得上：

```cmd
echo %ERRORLEVEL%
```

### 日常使用：做成快捷方式

测试通过以后就可以做成快捷方式，以后双击就行。

在 `D:\NoLauncher\` 文件夹空白处右键 →「新建」→「快捷方式」，「请键入对象的位置」填（路径换成你自己的）：

```
"D:\NoLauncher\EndfieldNoLauncher.exe" --hide-console --game "D:\Hypergryph\Games\Endfield\Endfield.exe"
```

起个名字，比如「终末地（免启动器）」，点「完成」。

然后右键这个快捷方式 →「属性」→「快捷方式」→「高级」→ 勾上「以管理员身份运行」。这一步不能省，否则游戏进程创建不出来。

`--hide-console` 让黑窗口不再出现。这种情况下如果出错，程序会弹一个对话框告诉你原因。

### 加游戏启动参数

我不知道终末地到底接受怎样的命令行参数。话虽如此，本程序确实允许你转发命令行参数。下面是做法：

游戏自己的参数放在 `--` 后面，原样转发：

```
"...\EndfieldNoLauncher.exe" --hide-console --game "...\Endfield.exe" -- -logFile "D:\log.txt"
```

### 用 Steam 或其他启动器调用

参数要填在「启动选项」里，不要填在「目标」里——后者只接受可执行文件路径；游戏路径和启动器路径都需要各自的引号。Steam 具体怎么拼接这个字符串、包装程序（gamescope、mangohud）为什么必须加 `%command%`，见[技术细节](docs/technical-details.md)。

### 出错了怎么办

从快捷方式或启动器启动时看不到任何输出，这时去看**可执行文件旁边**的 `EndfieldNoLauncher.log`。退出码的含义和日志每一行的读法见[技术细节](docs/technical-details.md)。

### 享受游戏

配置到这里就完成了，点开始游戏吧。

### 更新游戏

大版本更新还是要打开官方鹰角启动器更新一次。本程序不更新游戏，也不修改任何游戏文件。

## 工作原理

游戏会校验自己的**直接父进程**：父进程文件必须名为 `Games.exe`、携带可信的 Authenticode 签名、且由鹰角官方组织签名。直接运行 `Endfield.exe` 会失败并提示「游戏启动失败 / 请通过官方启动器打开游戏」，日志中为 `errorCode 10100`。

本工具在不运行启动器界面的前提下满足这个检查：它以**挂起状态**创建官方 `Games.exe` 并永不恢复，再把它指定为游戏进程的父进程。不注入代码，不执行启动器逻辑，不修改游戏文件。

1. **查找**启动器安装位置：`hypergryphlauncher*` 注册表协议、名为 鹰角启动器 / Hypergryph Launcher 的卸载项，以及 `%ProgramFiles%`、`%ProgramFiles(x86)%`、`%LOCALAPPDATA%`
2. **校验**候选：文件必须名为 `Games.exe`，`WinVerifyTrust` 必须接受其签名，且签名证书的组织名必须是官方发行方。从校验到进程创建期间保持文件句柄打开，使该可执行文件在这段窗口期内无法被替换
3. **以挂起状态创建**它：`CREATE_SUSPENDED | CREATE_NO_WINDOW`。主线程永不恢复，因此启动器从不运行也从不显示窗口；只有它的映像和签名存在，而这正是检查所读取的
4. **创建游戏**：使用 `EXTENDED_STARTUPINFO_PRESENT` 与指向挂起启动器的 `PROC_THREAD_ATTRIBUTE_PARENT_PROCESS`，使启动器成为游戏的直接父进程
5. **等待**游戏退出并返回其退出码。收尾仅在「本进程创建了该挂起启动器且它仍在运行」时终止它，然后等待它真正停止

## 构建

MSVC：

```powershell
cmake -B build -G Ninja
cmake --build build
```

在 Linux 上用 mingw-w64 交叉编译：

```bash
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

两套工具链默认都把 C 运行库静态链入映像，因此产出的 `EndfieldNoLauncher.exe` 可直接放在游戏旁运行，不需要可再发行组件包，也不需要额外 DLL。该开关只改变交付形态：

| `NOL_STATIC_RUNTIME` | Release 体积（mingw） | 目标机器需要的运行库 DLL |
| --- | --- | --- |
| `ON`（默认） | 约 1.2 MB | 无 |
| `OFF` | 约 122 KB | `libstdc++-6.dll`、`libgcc_s_seh-1.dll`，或 MSVC 可再发行组件包 |

需要更小的可执行文件时传 `-DNOL_STATIC_RUNTIME=OFF`，并把列出的 DLL 放在它旁边。

## 测试

```bash
cmake --build build --target EndfieldNoLauncherTests
build/EndfieldNoLauncherTests.exe
```

`tests/options_tests.cpp` 覆盖命令行解析与 `CreateProcessW` 参数引号转义，包括未处理时会静默破坏命令行的尾部反斜杠与内嵌引号情形。

**启动路径本身没有自动化测试覆盖。** 它需要签名有效的官方启动器、已安装的游戏和提权，因此只能在真实 Windows 安装上验证。编译通过不能说明游戏会接受该父进程。

## 许可证

AGPL-3.0。本程序是衍生作品：`src/launch.cpp` 逐字复制了 Better Endfield Next 的 `native/loaders/shared/official_launcher.cpp` 中的 20 个函数。AGPL 适用于再分发，包括通过网络提供。见 [`LICENSE`](LICENSE)。

与该来源的差异限于三个函数。

- `ParseOptions` 去掉了注入器与 Steam 选项，新增 `--help`
- `ConfigureConsole` 去掉了 Steam 分支，仅在 `--hide-console` 时分离控制台
- `ShowError` 使用本程序的对话框标题

致谢：[Better Endfield Next](https://github.com/Dr-hydra/Better-Endfield)，作者 Dr-hydra 及贡献者，commit `514f6d6`。
