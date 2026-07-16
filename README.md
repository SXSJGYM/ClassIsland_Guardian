# ClassIsland Guardian
> ⌜念念不忘，必有回响⌟

`ClassIsland Guardian` 是一款适用于 `Windows` 平台的，功能强大的 `ClassIsland` 守护工具。

有关 `ClassIsland` 的信息，敬请参阅 [
ClassIsland Github](https://github.com/ClassIsland/ClassIsland)。

> [!WARNING]
> **项目正处于早期开发阶段，核心功能尚未完整实现。**
>
> 目前仅有 `file.sys` 的基础文件保护功能与Ring3守护部分可用。
> 
> 许多纸面功能仍在开发中，预启动修复环境尚未构建。
>
> **请勿将其视为可使用的产品。也请勿因为功能未完善提出Issue或Pr。** 欢迎先行 Star 关注，或通过 Issue 交流想法。

## 功能

### 应用层守护
- 监控 `ClassIsland.Desktop.exe` 进程状态，异常退出时自动拉起。
- 支持手动创建、列出和恢复 `ClassIsland` 目录的历史快照，检测到目录异常时自动回滚。
- 映像劫持对抗：启动前自动检测并清理针对 `ClassIsland` 与 `ClassIsland Guardian` 进程的恶意 Debugger 劫持项，确保程序不被劫持或禁用。
- 完整的操作日志记录，便于排查问题。

### 驱动级守护
- 保护 `ClassIsland Guardian` 程序本体与预启动修复文件不受破坏。
- 阻止攻击者结束 `ClassIsland` 与 `ClassIsland Guardian` 进程。（开发中）

### 高级主动防御 （开发中）
- 检测试图结束受保护进程的攻击者，并通过内核工作项异步终止攻击者进程。
- 将保护驱动设置为 `SERVICE_BOOT_START`（启动等级 0），与 Windows 启动链深度绑定，提高驱动被恶意卸载的难度。

### 预启动修复 （开发中）
- 独立于 Windows 系统的预启动环境，不依赖任何驱动。
- 在 Windows 启动之前抢先运行。
- 自动检测 `ClassIsland` 与 `ClassIsland Guardian` 是否损坏，发现损坏时自动从快照完成检查和修复。

### 其他
- 提供应用层守护、驱动级守护、高级主动防御三级保护机制，支持按需启用。
- 通过完善的命令行菜单配置保护策略。
- 支持密码锁定，保护保护策略不受篡改。

## 软件截图/演示视频 （截图待补充，敬请期待）
![config.exe的配置页面](https://cn.bing.com/th?id=OHR.LakeAtitlan_ZH-CN1920221893_UHD.jpg&pid=hp&w=1920)

## 使用

> [!IMPORTANT]  
> **详细安装与配置说明请参阅 [ClassIsland Guardian 文档](https://github.com/SXSJGYM/ClassIsland_Guardian/wiki)。**

### 系统要求：

- Windows 10/11 x64。
- 管理员权限。
- 建议在虚拟机或测试环境中先行验证。

### 下载与安装

- [GitHub Releases](https://github.com/SXSJGYM/ClassIsland_Guardian/releases)

## 开发/编译
> [!IMPORTANT]  
> **详细的开发环境配置与编译指南请参阅 [ClassIsland Guardian 开发文档](https://github.com/SXSJGYM/ClassIsland_Guardian/wiki/开发指南)。**

### 项目结构

- `src/`：Ring3 用户态 Python 源码。
- `drivers/`：Ring0 内核驱动源码（`file.sys` / `process.sys` / `advanced.sys`）。

### 快速编译

在项目根目录执行编译脚本（需先安装依赖）：

```bash
build.bat
```


## 许可证

本项目采用 [GPL-3.0 License](https://www.gnu.org/licenses/gpl-3.0.html) 许可证。有关详细信息，敬请参见 [LICENCE.txt](LICENCE.txt) 。


## 致谢

1. 感谢 [ClassIsland](https://github.com/ClassIsland/ClassIsland) 本体——这个项目因你而生，也因你而不断进化。
2. 感谢我的班主任张老师——对我和多媒体的“严加调教”促生了这个项目，并推动它不断完善。
3. 感谢所有贡献者——每一行代码、每一个 Issue、每一次讨论，都在让 CIG 变得更好。
4. 感谢 [DeepSeek](https://deepseek.com)——在无数个卡壳的深夜提供思路与陪伴。
5. 感谢 [SignPath Foundation](https://signpath.org)——为开源项目提供免费的代码签名服务，让驱动能够被信任。
6. 感谢 [热铁盒网页托管](https://host-intro.retiehe.com/)——提供高速且价格友好的网站托管服务。
7. 感谢 [洛谷云图床](https://www.luogu.com.cn/image)——稳定的图床支持，让文档和 README 得以清晰呈现。
8. 感谢你——让这个项目有了存在的意义。
9. 磅十五便士。