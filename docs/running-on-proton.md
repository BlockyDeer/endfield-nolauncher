# 在 Steam + Proton 上运行

## 先决条件

据我所知，目前运行这个游戏最好的Proton版本是[DWProton](https://dawn.wine/dawn-winery/dwproton)，所以你需要一份DWProton。

由于1.6版本更新以后ACE反作弊的一些问题，你可能需要从DWProton的最新版本往前试验多个版本，直到找到一个能用的版本，你可以从那个版本开始。

## 用法

大部分用法和Windows上的用法相同。

### 创建Proton容器

如果你曾经在系统上下载过游戏，可以跳过这一步。

从[终末地网站](https://endfield.hypergryph.com/)下载鹰角启动器。打开Steam->「库」->左下角「添加游戏」->「添加非Steam游戏」->「浏览」->选择你刚刚下载的鹰角启动器

在库中右键你刚刚添加的游戏。选择「属性」->左边的「兼容性」->「选择兼容性工具」->选择「dwproton-xx.x-xx」（或类似的名字）

点选「开始游戏」，鹰角启动器的安装器应该能正常启动。安装鹰角启动器。

打开compatdata目录（默认是 `~/.local/share/Steam/steamapps/compatdata/`）。在文件管理器里选择按修改时间排序，选择最新的点进去。在这里面手工找到刚刚安装的启动器的启动可执行文件（如果你安装启动器的时候选择的默认位置，那么它应该在`~/.local/share/Steam/steamapps/compatdata/<appid>/pfx/drive_c/Program Files/Hypergryph Launcher/`）。把它的完整路径填写到游戏「目标位置」里（右键你的游戏->「属性」->「快捷方式」->「目标」）。

再次点击Steam中的开始游戏。选择「下载游戏」。等待完成。

退出鹰角启动器。

### 获取选项路径

我们需要获取游戏在Proton容器中的具体路径（而不是在Linux系统中的实际路径）。

首先你需要知道你创建的容器的Appid。方法有很多，作为参考，我列出一部分：你可以使用[Protontricks](https://protontricks.com/)、[ProtonPlus](https://github.com/Vysp3r/protonplus)或者干脆在`compatdata`里面按修改时间排序找最新（如果你刚刚创建容器）。这些都可以实际显示实际appid。记住它，你后面会用到。

#### 获取游戏路径

打开compatdata目录（默认是 `~/.local/share/Steam/steamapps/compatdata/`），找到你的appid。然后打开`pfx`。找到你的游戏可执行文件（默认是`drive_c/Program Files/Hypergryph Launcher/games/Endfield Game/Endfield.exe`）把从代表盘符的目录（`drive_c`）开始的东西贴到文本编辑器或者便签应用里。然后把所有正斜杠改成Windows的反斜杠，把盘符号添加上去。

例子（对于全默认路径）：

```
/home/blockydeer/.local/share/Steam/steamapps/compatdata/2733711885/pfx/drive_c/Program Files/Hypergryph Launcher/games/Endfield Game/
```

```
C:\Program Files\Hypergryph Launcher\games\Endfield Game\Endfield.exe
```

#### 获取启动器路径 `Games.exe`

和获取游戏路径基本上一样，唯一区别是你需要找的可执行文件名字叫`Games.exe`。它的默认位置是`pfx/drive_c/Program Files/Hypergryph Launcher/<版本号>/`。然后和获取游戏路径一样地处理路径格式。

例子（对于全默认路径）：

```
/home/blockydeer/.local/share/Steam/steamapps/compatdata/2733711885/pfx/drive_c/Program Files/Hypergryph Launcher/1.6.0/Games.exe
```

```
改成
C:\Program Files\Hypergryph Launcher\1.6.0\Games.exe
```

### 填写目标和路径

右键你的游戏->「快捷方式」

目标填`EndfieldNoLauncher.exe`的实际路径。

起始位置是随意的，我填的`.../pfx/drive_c`

启动选项：

```
--game "<Endfield.exe路径>" --parent "<Games.exe路径>"
```

例如：

```
--game "C:\Program Files\Hypergryph Launcher\games\Endfield Game\Endfield.exe" --parent "C:\Program Files\Hypergryph Launcher\1.6.0\Games.exe"
```

#### 通过Gamescope之类的包装启动

和普通的包装启动一样，唯一不同的是你需要把`%command%`加上。

这是我的启动选项例子：

```
gamescope -w 2560 -h 1440 -fb -- %command% --game "<Endfield.exe在容器内路径>" --parent "<Games.exe在容器内路径>"
```

### 享受游戏

你可以点开始游戏了。它应该能正常启动。

### 怎样更新游戏

游戏大版本更新以后需要去鹰角启动器更新。为了方便启动鹰角启动器，你可以添加鹰角启动器成额外的非Steam游戏：

打开Steam->「库」->左下角「添加游戏」->「添加非Steam游戏」->「浏览」->选择鹰角启动器的可执行文件（`Launcher.exe`）

试一下鹰角启动器能不能正常跑。

## 这太麻烦了，怎么办？

欢迎贡献bash脚本帮大家自动完成配置。
