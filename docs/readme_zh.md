<div align="right">
  <a href="../README.md">English</a> |
  <strong>简体中文</strong>
</div>

<div align="center">

  <img
    src="../Ksword5.1/Ksword5.1/Resource/Logo/KswordHome-ZH.png"
    alt="KSword ARK Logo"
    width="520"
  />

  <a href="https://github.com/user-attachments/assets/25a3b2e2-4ee0-49aa-bd90-ee6e3ba01fe4">
    <img
      src="https://github.com/user-attachments/assets/25a3b2e2-4ee0-49aa-bd90-ee6e3ba01fe4"
      alt="KSword ARK Dark Interface"
      width="49%"
    />
  </a>
  <a href="https://github.com/user-attachments/assets/217769a2-0521-41f9-9933-ca7c2fbb1d13">
    <img
      src="https://github.com/user-attachments/assets/217769a2-0521-41f9-9933-ca7c2fbb1d13"
      alt="KSword ARK Light Interface"
      width="49%"
    />
  </a>

  <br>

  <sub>深色模式 · Dark Mode　｜　浅色模式 · Light Mode</sub>

</div>

<h1 align="center">Ksword5.1</h1>
<p align="center"><strong>源码公开的 Windows 反内核隐藏（ARK）与内核分析工具集</strong></p>

<p align="center">
  <a href="https://github.com/KSwordDEV/KSword/stargazers">
    <img alt="GitHub stars" src="https://img.shields.io/github/stars/KSwordDEV/KSword.svg?style=for-the-badge" />
  </a>
  <a href="https://github.com/KSwordDEV/KSword/network/members">
    <img alt="GitHub forks" src="https://img.shields.io/github/forks/KSwordDEV/KSword.svg?style=for-the-badge" />
  </a>
  <a href="https://github.com/KSwordDEV/KSword/issues">
    <img alt="GitHub issues" src="https://img.shields.io/github/issues/KSwordDEV/KSword.svg?style=for-the-badge" />
  </a>
  <a href="https://github.com/KSwordDEV/KSword/blob/main/LICENSE">
    <img alt="License" src="https://img.shields.io/github/license/KSwordDEV/KSword.svg?style=for-the-badge" />
  </a>
</p>

<p align="center">
  <a href="#快速开始">快速开始</a> ·
  <a href="#功能">功能</a> ·
  <a href="#选哪个版本">版本</a> ·
  <a href="#从源码构建">构建</a> ·
  <a href="#许可证">许可证</a>
</p>

---

## KSword 是什么

KSword 是一个 Windows 10/11 上的 **ARK**（Anti-Rootkit，反内核级隐藏）工具。它自带一个内核驱动，能同时从用户态和内核态两个角度看同一个东西——同一个进程、同一个驱动、同一条网络连接。两边看到的不一样？那就是有东西在藏。

你可以把它理解成一台系统级的 X 光机：它把你原来需要十几个工具才能做到的事——查进程、翻内存、抓包、分析磁盘、调试驱动、审计安全策略——收到了同一个窗口里。

**设计思路：**

- **给你证据，不只给结论。** 每个字段都告诉你数据是从哪来的，你可以自己验证。
- **只看不动（除非你主动要求）。** 看任何东西都不会改动你的系统。卸载驱动、写磁盘之类的危险操作在单独的按钮后面，要你亲手确认，并且尽量提供撤销。
- **不能做到就直说。** 如果某个功能在你的 Windows 版本上缺少对应的内核偏移，它会显示"不支持"，而不是猜一个地址去读。

源码公开，按 [KSword Community Source License](../LICENSE) 授权——具体说明见[许可证](#许可证)。

## 快速开始

1. 下载并解压发行包，得到一个 `Release\` 文件夹。
2. 以管理员身份运行 **`Launcher.exe`**。

搞定。Launcher 会检查你的系统，自动选择合适的版本启动。

另外也可以用 `KswordSetup.exe` 安装器——多做了创建快捷方式等事情。但直接解压也完全够用。

> [!IMPORTANT]
> 内核级功能需要管理员权限和已加载的驱动。没有驱动的话程序照样能用，只是内核侧的数据看不到。

## 功能

### 看清楚到底在跑什么

浏览所有进程、线程和句柄——然后把 Windows 报告的结果和内核驱动亲自遍历的结果做对照。不一致的地方就是有东西被藏了。还能看线程栈、模块、令牌和进程保护级别。

### 翻内存

按特征搜索进程内存，用 Hex 视图浏览，打书签。驱动可以读到用户态工具读不到的内存区域，扫描内核中的可执行内存，还能做页表翻译——从虚拟地址查到物理地址。

### 分析二进制文件

拖一个 PE、ELF 或 Mach-O 文件进来做结构化扫描。也有字节编辑器，但故意做得很严格：只允许等长修改，写之前会检查文件有没有被改过，写入是原子的，可选保留备份。

### 监控网络

抓包和过滤、管理 TCP/UDP 连接、按进程限速、构造 HTTP 请求、查看 HTTPS 会话。驱动还会从网络栈内部（TCP、UDP、AFD、NSI、NDIS、WFP）给出自己的连接清单，让你发现用户态工具看不到的连接。

### 调试驱动和内核

加载、卸载、检查驱动服务。查看驱动对象、设备对象、派发表和快速 I/O 路径。浏览内核的对象命名空间、Hook 表（SSDT、IAT/EAT、inline）、回调、中断描述符。内置反汇编器，直接读内核代码。

**内核知识中心**有 71 篇可搜索的文章（中英双语），每篇都链接到对应的实时数据证据页。

### 分析文件和磁盘

双面板文件管理器，带哈希校验、数字签名、PE 结构视图和文件解锁。取证方面：浏览原始 NTFS 卷、找回已删除文件、查看 minifilter 栈和存储设备信息——默认全是只读的，磁盘写入需要手动解锁。

### 监视系统活动

基于 ETW 的进程监控、系统调用采集、WMI 事件订阅和风险汇聚中心。类似任务管理器的 CPU/GPU/内存/磁盘/网络实时图表。窗口枚举和消息监控。注册表浏览和搜索。启动项管理和服务管理。

### 审计安全策略

在一个地方检查 AppLocker 规则、WDAC/Code Integrity 策略、Defender 和 ASR 设置、VBS/Hyper-V 配置、驱动信任和 Windows 事件日志。

### 虚拟化实验（HVM）

运行 VMX 能力自检，启动一次性测试虚拟机，或启动一个受保护的 Intel VT-x/EPT 监控器来记录 VM exit 事件。硬件不支持时会自动阻止启动。**仅限已获授权的实验场景。**

<details>
<summary><b>完整功能清单（按 Dock 分）</b>——17 个主工作区 + 4 个辅助面板</summary>

<br>

OpenArk 覆盖对照见 [docs/OpenArk功能对照与TODO.md](OpenArk功能对照与TODO.md)。

| Dock | 包含什么 |
|---|---|
| **欢迎** | 版本信息、编译时间、用户信息、项目链接。 |
| **进程** | 进程树/列表，带图标和差异高亮。结束/挂起/恢复/优先级操作。线程栈、模块、令牌。R3/R0 对照查隐藏。可恢复的 R0 进程隐藏（需确认）。PPL/签名操作有风险提示。 |
| **网络** | 抓包和过滤。TCP/UDP 连接管理。按进程限速。请求构造器。HTTPS 检查。ARP/DNS 表。存活主机发现。WFP 防火墙事件和规则。NIDS。分段下载。R0 网络栈清单（TCP/UDP/AFD/NSI/NDIS/WFP）。 |
| **内存** | 进程内存浏览和搜索。Hex 查看器，带书签和断点。R0 内存读取。内核可执行内存扫描。内存证据页。PTE / 虚拟地址翻译。 |
| **文件** | 双面板文件管理。哈希、签名、PE/字符串/Hex 视图。文件解锁。NTFS 恢复。Minifilter/FileObject/Section 证据。存储栈和 BitLocker 信息。 |
| **扫描器** | 后台 PE/ELF/Mach-O 结构化扫描。安全字节编辑器（只允许等长修改，原子写入，可选备份）。 |
| **驱动** | 驱动服务管理（注册/加载/卸载/删除）。已加载模块。DBWIN 调试输出。DriverObject/DeviceObject/MajorFunction/FastIo 检查。派发表和镜像元数据的事务式编辑器。可恢复的加载链摘除。Driver Integrity。模块 Cross-View。已卸载驱动和 PiDDB 证据。 |
| **内核** | 对象命名空间浏览。原子表。SSDT/SSSDT。Inline/IAT/EAT Hook。CID Cross-View。ALPC/IPC。动态偏移管理。能力矩阵。干净的已加载镜像和 IDT 基线。描述符表和 IOCTL 解码。内核反汇编。完整回调清单（notify、注册表、对象、过滤器、bugcheck、shutdown、文件系统、登录、NMI 等）。内核知识中心（71 篇文章）。HVM 虚拟化实验。 |
| **监控** | 按进程 ETW 追踪。系统调用采集。WinAPI Agent。WMI 订阅。ETW Provider/Session 管理。ARK 风险中心。 |
| **硬件** | 任务管理器风格的 CPU/GPU/内存/磁盘/网络图表。进程 I/O 和 ETW 文件活动。设备树（SetupAPI/CfgMgr）。R0 设备栈审计（DevNode/USB/HID/PCI/ACPI/GPU/display/watchdog）。 |
| **权限** | 本地用户账号。创建用户/重置密码。组信息。当前进程权限快照。 |
| **窗口** | 窗口枚举、筛选、预览、拾取、控制。桌面管理。消息监控。Win32k GUI/session 审计。热键/Hook 审计。 |
| **注册表** | 注册表树浏览。键值增删改查。`.reg` 导入导出。异步搜索。 |
| **句柄** | 句柄列表，按 PID/关键字/类型过滤。命名对象解析。对象类型统计。HandleTable/ObjectHeader/ObjectType 证据。 |
| **启动项** | 按类别汇总的启动项：登录、服务、驱动、计划任务、注册表、WMI。修改前有风险门禁，尽可能提供恢复。 |
| **服务** | 服务表，带筛选和排序。启动/停止/暂停。启动类型调整。属性编辑。依赖关系。TSV/JSON 导出。 |
| **杂项** | BCD/引导配置。声音来源归因。系统变速（有警告和恢复路径）。Shell 关联管理（右键菜单、URL 处理、文件打开方式、资源管理器主页）。只读磁盘编辑和原始文件系统取证（写入需解锁）。AppLocker/WDAC/Defender/ASR/平台安全诊断。 |

**辅助面板：** 任务进度卡片、带调用链追踪的日志输出、即时/草稿窗口，和实时性能监视器。

</details>

## 选哪个版本

| | **Ksword5.1**（完整版） | **KswordARKLight**（轻量版） |
|---|---|---|
| 开发技术 | Qt 6，可停靠工作区 | 纯 Win32，零依赖 |
| 适合 | 完整 ARK 工作流，深度分析 | 老机器、快速处置、最小体积 |
| 功能范围 | 上面说的全部 | 核心集：进程、内存、注册表、文件、驱动、内核、监控、硬件、窗口、启动项、网络、句柄、安全 |

两个版本用的是同一个内核驱动。`Launcher.exe` 会自动帮你选。

## 仓库结构

| 目录 | 是什么 |
|---|---|
| `Ksword5.1/` | 完整 Qt 桌面主程序。 |
| `KswordARKLight/` | 轻量 Win32 版。 |
| `KswordARKDriver/` | 内核驱动。 |
| `Launcher/` | 启动助手，帮你选对版本。 |
| `KswordCLI/` | 命令行工具——见 [CLI 文档](CLI使用文档.md)。 |
| `KswordSetup/` | 可选安装器。 |
| `Taskbar/` | 顶部 AppBar，`S O S Enter` 快速拉起。 |
| `KswordHUD/`、`APIMonitor_x64/` | HUD 覆盖层和 API 监控辅助。 |
| `shared/driver/` | 用户态和内核之间的共享协议头。 |
| `tools/` | PDB 偏移生成器等构建工具。 |
| `docs/` | 技术文档——见[完整索引](#文档索引)。 |

项目网站：[KSwordDEV/Website](https://github.com/KSwordDEV/Website)

## 给贡献者：代码怎么组织的

程序和驱动通过一层共享协议通信。如果你要贡献代码，需要知道：

- **协议头放在 `shared/driver/`。** 不要把 IOCTL 定义散到 UI 或驱动私有目录里。
- **UI 代码不直接和驱动说话。** 一切都走 `ArkDriverClient`（或轻量版的对应封装）。Dock 代码里不许有裸的 `DeviceIoControl` 调用。
- **内核偏移来自经过验证的 profile**，不是写死的值。如果发行包里的 profile 和你的 Windows 版本不匹配，程序会在后台从 PDB 符号解析——但只有在确认二进制身份一致后才会使用。绝不猜偏移。
- **新增源码要同步更新 `.vcxproj` 和 `.vcxproj.filters`。** 第三方代码保留上游许可证。

完整贡献规范：[AGENTS.md](../AGENTS.md) · [CONTRIBUTING.md](../CONTRIBUTING.md)

<details>
<summary><b>协议索引</b>——各项能力在代码的什么位置</summary>

<br>

| 功能 | 位置 | 说明 |
|---|---|---|
| 驱动状态和功能矩阵 | `KswordArkCapabilityIoctl.h` | 支撑"驱动状态"页面。 |
| 动态内核偏移 | `KswordArkDynDataIoctl.h` | Profile 匹配、字段来源、能力门禁。 |
| 进程扩展信息 | `KswordArkProcessIoctl.h`（v2） | Session、镜像路径、保护级别、字段可用性。 |
| 可恢复进程隐藏 | `IOCTL_KSWORD_ARK_SET_PROCESS_VISIBILITY` | 从链表摘除进程但保留 CID 表记录，以便恢复。 |
| 保护级别修改 | `KSW_CAP_PROCESS_PROTECTION_PATCH` | 有门禁；确认对话框展示影响和回滚风险。 |
| vendored 偏移数据 | `third_party/systeminformer_dyn/` | 只用了 System Informer 的偏移数据，没引入 KPH 通信层。 |

所有协议头都在 `shared/driver/` 下面。

</details>

## 从源码构建

**你需要：** Windows 10/11、Visual Studio 2022（MSVC）、Qt 6.9.3 `msvc2022_64`（轻量版不需要 Qt）、WDK（只有构建驱动时需要）。

```powershell
# 1. 告诉构建系统 Qt 装在哪（只需运行一次）
.\Setup-QtPaths.ps1 -QtDir 'C:\Qt\6.9.3\msvc2022_64'

# 2. 构建全部
$msbuild = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'
& $msbuild '.\Ksword5.1\Ksword5.1.sln' /t:Build /p:Configuration=Debug /p:Platform=x64 /m
```

或者只构建其中一部分：

```powershell
# 只构建轻量版（不需要 Qt）
& $msbuild '.\KswordARKLight\KswordARKLight.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m

# 只构建 Launcher
& $msbuild '.\Launcher\Launcher.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m
```

没装 WDK？跳过驱动——用户态部分可以单独构建，发行包制作时复用已有的驱动二进制就行。

<details>
<summary><b>构建排障</b></summary>

<br>

**主程序链接报 `LNK1000` / `IMAGE::BuildImage`**

关掉 Whole Program Optimization，做一次干净重建。不要改项目文件把它变成常态。验证方法：确认真实的 MSBuild 退出码和 `Ksword5.1\x64\Release\Ksword5.1.exe` 非零。

**驱动的 `ApiValidator` 在链接后失败**

通常是 WDK 后置步骤的架构选择问题。单独用 x64 WDK 路径跑校验器：

```powershell
$solutionDir = (Resolve-Path '.\Ksword5.1').Path + '\'
$apiValidatorX64 = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64'
& $msbuild '.\KswordARKDriver\KswordARKDriver.vcxproj' /t:ApiValidator `
  /p:Configuration=Release /p:Platform=x64 /p:SolutionDir=$solutionDir `
  /p:ApiValidator_ApiExtractorExePath=$apiValidatorX64 /m:1 /v:minimal
```

输出 `Driver is 'Universal'.` 只说明 API 检查通过，不代表完整的构建/签名/加载流水线没问题。

</details>

## 文档索引

| 文档 | 内容 |
|---|---|
| [CLI使用文档.md](CLI使用文档.md) | 命令行工具参考 |
| [功能技术文档.md](功能技术文档.md) | 功能级技术文档 |
| [内核知识中心.md](内核知识中心.md) | 内核知识中心素材来源 |
| [driver_ioctl_audit.md](driver_ioctl_audit.md) | 驱动 IOCTL 面审计 |
| [OpenArk功能对照与TODO.md](OpenArk功能对照与TODO.md) | OpenArk 覆盖对照和缺口 |
| [动态偏移功能接入步骤.md](动态偏移功能接入步骤.md) | 怎么把新功能接入动态偏移 |
| [pdb_r0_audit_prep/](pdb_r0_audit_prep/) | PDB/R0 审计准备 |
| [插件系统规范.md](插件系统规范.md) | 插件系统规范 |
| [多语言语言包规范.md](多语言语言包规范.md) | 多语言和语言包规则 |

<details>
<summary><b>近期更新</b></summary>

<br>

- **内核知识中心** — 71 篇可搜索的内核专题文章，每篇都链接到实时数据证据页。
- **扫描器 Dock** — PE/ELF/Mach-O 结构化扫描，配安全字节编辑器。
- **取证能力扩展** — 干净的已加载镜像和 IDT 基线、描述符表解码、内核反汇编、更广的 R0 网络覆盖、带已删除文件恢复的原始文件系统浏览。
- **HVM** — VMX 自检、一次性测试虚拟机、受保护的 VT-x/EPT 监控。
- **运行时 PDB 解析** — 发行包 profile 不匹配时自动在后台解析对应偏移。
- **易用性** — 表格冻结、平滑滚动、可取消的卡顿检测、启动项和网络配置修改的更强校验与恢复。

</details>

## 声明

> [!WARNING]
> 本项目包含系统级调试、审计和管理能力。请仅在合法授权和合规的环境中使用。

## 许可证

KSword 按 [KSword Community Source License v1.6](../LICENSE) **源码公开**发布。

这里的"开源"只是说你能看到和获取源码——**不**代表使用了 OSI 认证的开源许可证。允许做什么以 `LICENSE` 为准，尤其是再分发和商业使用部分。第三方组件使用它们自己的许可证。

[社区公约](../COMMUNITY_COVENANT.md)是关于诚实、署名和负责任使用的社区约定，不是额外的许可限制。贡献规则见 [CONTRIBUTING.md](../CONTRIBUTING.md)。
