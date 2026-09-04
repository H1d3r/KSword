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
<p align="center"><strong>源码公开（非 OSI 认证开源许可证）的 Windows ARK 与内核分析工具集</strong></p>

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
  <a href="#组件构成">组件</a> ·
  <a href="#功能">功能</a> ·
  <a href="#安全模型">安全模型</a> ·
  <a href="#从源码构建">构建</a> ·
  <a href="#文档索引">文档</a> ·
  <a href="#许可证">许可证</a>
</p>

---

## KSword 是什么

KSword 是面向 Windows 10/11 x64 的 **ARK**（*Anti-Rootkit*，反内核级隐藏）与内核分析工具集。它由用户态主程序和自带的内核驱动组成，因此同一个对象——一个进程、一个驱动、一条网络连接、一个文件——可以同时从 Ring 3 和 Ring 0 观察，再把两份结果对照。两边不一致的地方，就是有东西在藏。

围绕这个核心，它把原本需要十几个独立工具才能凑齐的能力收进了同一个界面：内存搜索与页表翻译、PE/ELF/Mach-O 结构扫描、抓包与防火墙审计、原始 NTFS 取证、回调与 Hook 清单、注册表与启动项审计、设备栈追踪，以及 Windows 安全策略诊断。

**设计上有三条主线：**

| | |
|---|---|
| **给证据，不给结论** | 每个审计页都会显示字段的来源——R3 API、R0 读取、PDB 符号还是动态偏移——结果可以复核，而不是只能选择相信。 |
| **默认只读** | 查看不会改动系统。卸载、patch、隐藏、磁盘写入都在独立入口后面，需要显式确认，并尽可能提供回滚。见[安全模型](#安全模型)。 |
| **如实降级** | 内核偏移、权限或硬件特性不可用时，界面直接显示 `unsupported` 或 `partial`，绝不跨 Windows 版本猜偏移。 |

本项目按 [KSword Community Source License](../LICENSE) 源码公开——具体允许和不允许什么，见[许可证](#许可证)。

## 快速开始

### 运行发行版

1. 解压发行包，得到一个 `Release\` 目录。
2. 以管理员身份运行 **`Launcher.exe`**。

`Launcher.exe` 是推荐入口。它会读取发行包内的支持清单，检查与当前系统的兼容性，然后启动完整 Qt 主程序（`Ksword5.1.exe`）或轻量版（`KswordARKLight.exe`）。

`KswordSetup.exe` 只是可选的便利安装器——它释放同样的 payload、创建快捷方式，并可选写入外观/启动配置。手动解压 `Release\` 效果完全等同。

> [!IMPORTANT]
> R0 功能需要管理员权限、正在运行的 `KswordARK` 驱动服务，以及匹配的系统安全策略。测试签名/驱动签名要求取决于目标机器配置。没有驱动时程序照常运行，只是 R0 页会报告对应能力不可用。

### 该用哪个版本

| | **Ksword5.1**（完整版） | **KswordARKLight**（轻量版） |
|---|---|---|
| 界面 | Qt 6 + ADS 可停靠工作区 | 原生 Win32，手写 Dock |
| 依赖 | Qt 运行时 + 插件 | 除系统外无依赖 |
| 适合 | 完整 ARK / 调试 / 审计工作流 | 更早的系统、低资源机器、快速处置 |
| 覆盖 | 下文全部功能 | 进程、内存、注册表、文件、驱动、内核、监控、硬件、窗口、启动项、网络、句柄、杂项安全 |

两者使用同一套 `shared/driver/` 协议，驱动同一个内核驱动。不想选的话，交给 `Launcher.exe` 判断即可。

## 组件构成

| 组件 | 说明 |
|---|---|
| `Ksword5.1/` | 完整 Qt/ADS 桌面主程序，承载完整 ARK 工作流。 |
| `KswordARKLight/` | 轻量原生 Win32 ARK——不依赖 Qt，启动更快，功能精简。 |
| `KswordARKDriver/` | 内核驱动：进程、线程、句柄、内存、网络、内核对象、设备与安全审计协议。 |
| `Launcher/` | 纯 Win32 启动与兼容性助手；发现已加载内核模块缺少偏移时，还能准备离线上报收集包。 |
| `KswordCLI/` | 面向自动化、验收与排障的命令行入口——见 [docs/CLI使用文档.md](CLI使用文档.md)。 |
| `KswordSetup/` | 可选安装器，释放发行 payload 并创建快捷方式。 |
| `Taskbar/` | 顶部 AppBar，状态展示与 `S O S Enter` 快速拉起。 |
| `KswordHUD/`、`APIMonitor_x64/` | HUD 辅助程序与 API Monitor 注入监控组件。 |
| `shared/driver/` | R0/R3 共享 IOCTL 协议头——用户态与驱动之间的契约。 |
| `tools/pdb_offset_generator/` | 生成与校验 PDB offset / DynData profile pack。 |
| `third_party/systeminformer_dyn/` | vendored System Informer 动态偏移快照，附上游 LICENSE/NOTICE。 |

[KSwordDEV/Website](https://github.com/KSwordDEV/Website) 仓库独立维护项目官网和分模块介绍页面。

## 功能

按"你在查什么"分组。完整的逐 Dock 清单折叠在本节末尾。

### 进程与线程

进程树/列表，带图标与差异高亮；结束、挂起、恢复、优先级、关键进程标记；线程栈、模块与令牌；带 PDB 字段诊断的进程详情。**R3/R0 cross-view** 把用户态枚举结果与驱动自己的遍历结果对照，暴露被隐藏的进程、线程和 CID 记录。R0 可恢复进程隐藏作为显式门禁操作提供。

### 内存

区域浏览、特征搜索、Hex 查看、书签与断点；R0 区域读取；内核可执行内存扫描；内核与进程内存证据；PTE / 虚拟地址翻译。

### 二进制

后台结构化扫描 **PE、ELF 与 Mach-O** 文件。可选字节编辑器只接受等长修改：复核源文件快照、原子替换目标、可保留备份——且必须先确认风险。

### 网络

抓包与过滤、TCP/UDP 连接管理、按进程限速、请求构造、HTTPS 解析、ARP/DNS、存活主机发现、WFP 防火墙事件与规则、NIDS、分段下载——同时提供 TCP、UDP、AFD、NSI、NDIS、WFP 的只读 R0 清单。

### 驱动与内核对象

驱动服务注册、加载、卸载、删除；已加载模块与 DBWIN 输出；DriverObject / DeviceObject / MajorFunction / FastIo 诊断；MajorFunction 与镜像元数据的事务式编辑器；可恢复的加载链摘除；Driver Integrity；Module Cross-View；已卸载驱动与 PiDDB 证据。内核侧：递归对象命名空间、原子表、SSDT/SSSDT、Inline Hook、IAT/EAT、ALPC/IPC、干净已加载镜像与 IDT 基线、描述符表与 IOCTL 解码、内核反汇编。

回调清单覆盖 Notify、注册表、对象、过滤器、BugCheck、Shutdown、文件系统、登录会话、CallbackObject、镜像验证与 NMI 来源，并展示模块归属及快照/逐行身份诊断。

### 文件、存储与设备

双面板文件管理、权限接管、哈希、签名、PE/字符串/Hex 视图、文件解锁、NTFS 恢复；Minifilter、FileObject、Section、存储栈与 BitLocker 证据；**只读的原始文件系统浏览与已删除条目取证**；磁盘 IO 监控；SetupAPI/CfgMgr 设备树与 R0 DevNode/USB/HID/PCI/ACPI/GPU/display/watchdog 审计。

### 监控与系统状态

目标进程树 ETW、syscall 采集、WinAPI Agent、WMI 订阅、ETW Provider/Session 管理，以及 ARK 风险聚合中心。任务管理器风格的 CPU/GPU/内存/磁盘/网络性能页。窗口枚举、拾取、控制，以及结构化的 win32k GUI/session 与热键/Hook 审计。注册表浏览与增删改查、`.reg` 导入导出、异步搜索。分类启动项、服务管理、本地账号与权限。

### 安全姿态

AppLocker、WDAC / Code Integrity、Defender / ASR、VBS / Hyper-V、平台安全、驱动信任与事件日志诊断。

### 内核知识中心

双语、可全文检索的 **71 篇专题**，分 12 类。每篇包含完整八段正文、版本化 R3/R0 现场查询（请求、CPU、时间、WDF/WDM 证据）、经中央表核实的业务 IOCTL 来源、Microsoft Learn 官方参考，以及通往对应证据页的只读路由。

### 虚拟化（HVM）

需确认的 VMX 自检、一次性测试来宾，以及带 VM-exit 遥测、受生命周期保护的 Intel VT-x/EPT 常驻监控。AMD 平台、已存在 Hypervisor，或电源/拓扑/卸载保护不完整时，常驻启动会被拒绝。**仅限已获授权的实验与诊断场景。**

<details>
<summary><b>完整功能清单（按 Dock）</b>——17 个主工作区 Dock 与 4 个辅助面板</summary>

<br>

以下内容基于当前代码、Dock 初始化逻辑与 R0/R3 协议整理。OpenArk 覆盖对照与缺口 TODO 见 [docs/OpenArk功能对照与TODO.md](OpenArk功能对照与TODO.md)。

"设置"已从主 Dock 移到顶部菜单。

| 一级 Dock | 二级页 / 关键区 | 主要功能 |
|---|---|---|
| **欢迎** | 欢迎页主体 | 展示版本、编译时间、用户信息、头像和项目入口。 |
| **进程** | 进程列表、创建进程、详情窗口、线程、模块、令牌、Cross-View、PDB Catalog | 进程树/列表、图标与差异高亮、结束/挂起/恢复/优先级/关键进程/R0 可恢复隐藏、R3/R0 进程线程对照、线程栈、PPL/Signature/CID 等高风险操作提示。 |
| **网络** | 流量监控、进程限速、连接管理、请求构造、HTTPS、ARP/DNS、存活主机、防火墙、NIDS、下载、网络审计 | 抓包过滤、TCP/UDP 连接管理、WFP 防火墙事件与规则、实时检测、分段 HTTP/HTTPS 下载，以及 TCP/UDP/AFD/NSI/NDIS/WFP 的只读 R0 清单和 cross-view。 |
| **内存** | 进程与模块、区域、搜索、查看器、断点/书签、R0 读写、Kernel Exec Scan、Memory Evidence、PTE | R3 内存浏览与搜索、R0 区域读取、内核可执行内存扫描、内核/进程内存证据、页表项与虚拟地址翻译。 |
| **文件** | 文件管理、文件恢复、属性、解锁、Minifilter、FileObject、Section、Storage/BitLocker | 双面板管理、权限接管、哈希/签名/PE/字符串/Hex、NTFS 恢复、文件占用与 Section 映射、存储栈与 BitLocker 只读证据。 |
| **扫描器** | 结构化扫描、安全字节编辑 | 在后台结构化扫描 PE、ELF 与 Mach-O。可选编辑器只允许等长修改，需明确确认风险；它会复核原始快照、原子替换目标，并可保留备份。 |
| **驱动** | 驱动概览、驱动操作、调试输出、对象信息、完整性、模块 Cross-View、Unloaded/PiDDB | 驱动服务注册/加载/卸载/删除、已加载模块、DBWIN 输出、DriverObject/DeviceObject/MajorFunction/FastIo、MajorFunction/镜像字段原子事务、加载链摘除与恢复、Driver Integrity、Unloaded/PiDDB 证据和只告警、显式确认的操作入口。 |
| **内核** | 对象命名空间、原子表、NtQuery、SSDT、SSSDT、Inline Hook、IAT/EAT、CID、IPC、DynData、驱动状态、回调、基线、HVM、内核知识 | 对象目录递归、BaseNamedObjects、NamedPipe、符号链接、设备/驱动对象、对象类型矩阵、CID/cross-view、ALPC/IPC、动态偏移、能力矩阵、干净已加载镜像与 IDT 基线、描述符表/IOCTL 解码、内核反汇编和回调遍历/管理。"内核知识"提供 71 篇双语可搜索文章、版本化 R0 现场上下文/来源查询、官方参考和通往现有证据页的只读路由，并保留运行态降级显示。HVM 流程对 VMX 自检、一次性测试来宾和受保护的 Intel VT-x/EPT 常驻监控设有显式确认。 |
| **监控** | 进程定向、直接内核调用、WinAPI、WMI、ETW、Risk Center | 目标进程树 ETW、syscall 采集、WinAPI Agent、WMI 订阅、ETW Provider/Session 管理、ARK 风险聚合。 |
| **硬件** | 利用率、概览、CPU、GPU、内存、硬盘监控、设备管理、R0 设备审计 | 任务管理器风格性能页、磁盘/网络/GPU 动态卡片、进程 IO 与 ETW 文件活动、SetupAPI/CfgMgr 设备树、DevNode/USB/HID/PCI/ACPI/GPU/display/watchdog 审计。 |
| **权限** | 账号、权限 | 本地用户、创建用户/重置密码、组信息和当前进程权限快照。 |
| **窗口** | 窗口列表、桌面/窗口详情、Win32k/GUI、热键/Hook、剪贴板、GPU/Display | 窗口枚举、筛选、预览、拾取、控制、桌面管理、消息监控、win32k GUI/session 与热键/Hook 结构化审计。 |
| **注册表** | 树、值列表、搜索结果 | 注册表浏览、键值增删改查、导入/导出 `.reg`、异步搜索和跳转。 |
| **句柄** | 句柄列表、对象类型、对象详情 | 按 PID/关键字/类型过滤，命名对象解析，对象类型统计，HandleTable/ObjectHeader/ObjectType 证据。 |
| **启动项** | 总览、登录、服务、驱动、计划任务、高级注册表、WMI | 启动项分类汇总、图标渲染、过滤/导出、定位文件和注册表位置；展示风险等级、影响与恢复能力后由用户确认修改。注册表值和启动文件夹使用备份/暂存恢复，计划任务启停后复核，服务与驱动修改 SCM 启动类型；WMI 禁用和 Winsock/整子键删除属于明确标记的不可恢复操作。 |
| **服务** | 服务主表、常规、登录、恢复、依存关系、审计 | 服务筛选排序、启动类型调整、启动/停止/暂停/继续、属性编辑、依赖和审计信息、TSV/JSON 导出。 |
| **杂项** | 引导、声音来源、系统变速、Shell 关联管理、磁盘编辑、原始文件系统取证、应用控制 | BCD/引导相关入口；Core Audio 声音来源归因；带永久警告、双重确认和恢复路径的 R0 系统全局加速/减速；右键菜单、URL 绑定、文件打开方式、按格式右键菜单及资源管理器主页第三方程序管理；默认只读的磁盘编辑、原始文件系统浏览和已删除条目分析（写入需解锁）；AppLocker/WDAC/Defender/ASR/平台安全/事件日志诊断。 |

**辅助面板**

| 面板 | 关键区 | 主要功能 |
|---|---|---|
| 当前操作 | 任务卡片 | 展示后台任务步骤和进度，完成后自动隐藏。 |
| 日志输出 | 级别过滤、日志表格、右键菜单 | 日志过滤、复制、导出、双重确认清空、GUID 调用链追踪。 |
| 即时窗口 | 代码/文本编辑器 | 快速验证、临时记录和即时输出。 |
| 监视面板 | CPU/内存/磁盘/网络图 | 底部实时性能监视，显示多曲线吞吐趋势。 |

**工作区行为**——ADS 布局保存与恢复、可见 Dock 惰性初始化、表格冻结、平滑滚动、可取消的 UI 卡顿检测、顶部菜单设置，以及 UIAccess / 置顶策略。

</details>

## 安全模型

本项目同时提供内核级的读**和**写能力。下面几条不是建议，而是代码的实际组织方式，贡献代码时也应遵循：

- **审计页默认只读。** 枚举、解码、对照都不改动系统状态。
- **mutation 走独立入口。** 卸载、删除、patch、bypass、磁盘写入等操作必须有自己的入口、风险提示，以及回滚或审计策略；部分操作是带永久警告的两步确认。
- **capability 门禁 R0。** 依赖未公开内核字段的功能必须声明所需 capability，dispatch 层在 handler **之前**执行 gate；DynData 缺失或 profile 不匹配时降级或 fail closed。
- **绝不猜偏移。** PDB/DynData profile 只在 PE/PDB 身份校验通过后才下发。不受支持的系统版本会报 `unsupported`，而不是去读一个看着像的地址。
- **降级可见。** 运行时的 `unsupported`、`partial`、截断、DynData、权限和硬件限制会显示在界面上，不会被悄悄吞掉。

内核 Dock 的**驱动状态**页把这些集中呈现：Driver Loaded/Missing、Protocol Mismatch、DynData Missing、Limited、当前安全策略、最近一次 R0 错误，以及完整的功能能力矩阵。

> [!WARNING]
> KSword 包含系统级调试、审计和管理能力，请仅在合法授权和合规的环境中使用。

## 从源码构建

### 环境要求

- Windows 10/11 x64
- Visual Studio 2022，含 MSVC 与 MSBuild
- Qt **6.9.3** `msvc2022_64`——Qt 主程序与辅助 UI 需要；`KswordARKLight` 和 `Launcher` 不需要 Qt
- WDK——仅构建内核驱动时需要

请保持标准 MSVC 工具链，本项目未适配 LLVM。

### 快速构建

建议先用仓库脚本发现并集中写入本机 Qt 路径，避免把个人路径写进各 `.vcxproj`：

```powershell
# 在仓库根目录执行；路径按本机安装位置替换
.\Setup-QtPaths.ps1 -QtDir 'C:\Qt\6.9.3\msvc2022_64'

# MSBuild 路径按本机 VS 安装位置替换；Developer PowerShell 中可直接使用 msbuild
$msbuild = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'

# 完整解决方案：主程序、Taskbar、HUD、驱动、CLI、安装器、轻量版
& $msbuild '.\Ksword5.1\Ksword5.1.sln' /t:Build /p:Configuration=Debug /p:Platform=x64 /m
```

单独构建：

```powershell
# 只构建轻量版
& $msbuild '.\KswordARKLight\KswordARKLight.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m

# 构建原生 Launcher（同时生成可直接阅读的发行支持清单）
& $msbuild '.\Launcher\Launcher.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m
```

本机没有 WDK 或驱动签名环境？先只构建用户态项目——制作发行包时可以沿用已有的未签名 R0 产物。

<details>
<summary><b>排障：MSVC 链接与 WDK 校验器失败</b></summary>

<br>

**主程序链接报 `LNK1000` / `IMAGE::BuildImage` 或 `.iobj` 失败**

关闭 Whole Program Optimization 与 LTCG，做**仅此一次**的干净重建。不要改项目文件把它变成常态，也不要紧接着再跑一次普通构建：关掉 WPO 的产物会让正常增量构建缓存失效。复核方式是确认真实的 MSBuild 退出码和非零的 `Ksword5.1\x64\Release\Ksword5.1.exe`，而不是只看控制台末尾输出。只有当这次一次性恢复仍然复现失败时，才考虑更新、降级或重装 MSVC。

**x64 驱动的 WDK 后置 `ApiValidator` / `aitstatic` 失败**

这通常是架构选择问题，且发生在 `KswordARK.sys` 已经链接完成**之后**。先确认 `.sys` 是刚链接出来的，并确认卡住的 MSBuild 没有活跃的编译器、链接器或校验器子进程，再结束那个确切进程。然后指定 x64 WDK 二进制目录，单独运行校验器（WDK 版本按本机安装替换）：

```powershell
$solutionDir = (Resolve-Path '.\Ksword5.1').Path + '\'
$apiValidatorX64 = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64'
& $msbuild '.\KswordARKDriver\KswordARKDriver.vcxproj' /t:ApiValidator `
  /p:Configuration=Release /p:Platform=x64 /p:SolutionDir=$solutionDir `
  /p:ApiValidator_ApiExtractorExePath=$apiValidatorX64 /m:1 /v:minimal
```

输出 `Driver is 'Universal'.` 只能确认独立的 API/架构校验通过，**不能**证明完整驱动构建、INF/CAT 生成、签名或驱动加载被接受——这些阶段要分别验证和汇报。如果失败发生在 `.sys` 更新之前，那就是真实的构建失败，必须直接修复。

</details>

### 发行打包

发行包根目录为 `Release\`，包含 `Launcher.exe`、`Ksword5.1.exe`、`KswordARKLight.exe`、辅助程序、驱动、`profiles\launcher_support_manifest.json`、DynData packs，以及 Qt 依赖和插件目录。快捷方式与安装器的安装后动作都启动 `Launcher.exe`。

## 架构约定

下面几条定义了 R3 与 R0 之间的协作方式。完整协作规范见 [AGENTS.md](../AGENTS.md) 与 [CONTRIBUTING.md](../CONTRIBUTING.md)。

- **协议只有一个家。** R0/R3 共享协议统一放在 `shared/driver/`；新增 IOCTL 头、结构体和版本字段不要散落到 UI 或驱动私有目录。
- **分发只有一条路。** 驱动 IOCTL handler 通过 `KswordARKDriver/src/dispatch/ioctl_registry.c` 注册；`ioctl_dispatch.c` 只负责查表、校验、调用、日志和完成请求。
- **访问只有一个客户端。** 用户态访问 KswordARK 设备统一走 `Ksword5.1/Ksword5.1/ArkDriverClient/` 或轻量版对应封装。Dock/UI 代码不打开设备，也不直接调用 `DeviceIoControl`。
- **偏移来自 profile。** PDB/DynData 使用 `tools/pdb_offset_generator/` 生成的 v4 profile pack，发行包只携带 `ark_dyndata_pack_v4.json.qz`。发行包 profile 与运行内核不匹配时，完整版和轻量版都可在串行后台 DbgHelp 会话中解析精确运行时 profile，且只在身份校验通过后下发。
- **构建文件属于本次改动。** 新增源码必须同步加入对应的 `.vcxproj` 和 `.vcxproj.filters`；第三方接入必须保留上游许可证文本。

<details>
<summary><b>协议索引</b>——各项能力定义在哪里</summary>

<br>

| 领域 | 协议头 / 机制 | 说明 |
|---|---|---|
| 驱动状态与能力 | `shared/driver/KswordArkCapabilityIoctl.h` | 支撑"驱动状态"页：Loaded/Missing、Protocol Mismatch、DynData Missing、Limited、安全策略、最近 R0 错误、能力矩阵。 |
| 动态偏移 | `shared/driver/KswordArkDynDataIoctl.h` | "动态偏移"页通过 `ArkDriverClient` 展示 profile 命中、字段来源和 capability gating；驱动装载后主窗口会自动刷新并下发 profile。 |
| 进程扩展信息 | `shared/driver/KswordArkProcessIoctl.h`（v2） | Session、完整镜像路径、Protection/SignatureLevel、ObjectTable/SectionObject 可用性、字段来源、DynData capability。DynData 缺失时 ProcessDock/ProcessDetail 只展示可用性，不直接枚举句柄表或 Section。 |
| 可恢复进程隐藏 | `IOCTL_KSWORD_ARK_SET_PROCESS_VISIBILITY` | 同时修改 `_EPROCESS.UniqueProcessId` 并摘除 `ActiveProcessLinks`，保留 PspCidTable 记录以便按原 PID 恢复。 |
| PPL / 保护位修改 | capability `KSW_CAP_PROCESS_PROTECTION_PATCH` | 用户态确认框必须展示当前/目标 Protection、SignatureLevel 影响、字段来源和回滚风险。 |
| vendored DynData | `third_party/systeminformer_dyn/` | 只接入 System Informer 的 `KphDynConfig` 数据和轻量解析器，不引入 KPH 通信层、对象系统或 session token。 |

</details>

## 文档索引

| 文档 | 内容 |
|---|---|
| [docs/CLI使用文档.md](CLI使用文档.md) | KswordCLI 命令参考。 |
| [docs/功能技术文档.md](功能技术文档.md) | 功能级技术文档。 |
| [docs/内核知识中心.md](内核知识中心.md) | 内核知识中心的素材来源。 |
| [docs/driver_ioctl_audit.md](driver_ioctl_audit.md) | 驱动 IOCTL 面审计。 |
| [docs/OpenArk功能对照与TODO.md](OpenArk功能对照与TODO.md) | OpenArk 覆盖对照与缺口 TODO。 |
| [docs/动态偏移功能接入步骤.md](动态偏移功能接入步骤.md) | 如何把新功能接入 DynData 流水线。 |
| [docs/pdb_r0_audit_prep/](pdb_r0_audit_prep/) | PDB/R0 审计能力准备、接入与验收文档。 |
| [docs/插件系统规范.md](插件系统规范.md) | 插件系统规范。 |
| [docs/多语言语言包规范.md](多语言语言包规范.md) | 多语言与语言包规则。 |
| [AGENTS.md](../AGENTS.md) | 发行构建、打包与校验流程。 |
| [CONTRIBUTING.md](../CONTRIBUTING.md) | 贡献流程。 |

<details>
<summary><b>近期更新</b></summary>

<br>

- **内核知识中心**——内核 Dock 内 12 类、71 个双语可检索专题，每篇含完整正文、版本化 R3/R0 现场查询、经核实的业务 IOCTL 来源，以及通往对应证据页的路由。
- **扫描器 Dock**——后台结构化扫描 PE、ELF、Mach-O，配一个刻意受限的等长字节编辑器。
- **取证能力扩展**——干净已加载镜像与 IDT 基线、描述符表与 IOCTL 解码、内核反汇编、更广的 R0 网络清单，以及带已删除条目分析的原始文件系统浏览。
- **HVM**——需确认的 VMX 自检、一次性测试来宾，以及带 VM-exit 遥测、受保护的 Intel VT-x/EPT 常驻监控。
- **运行时 PDB 解析**——发行包 DynData profile 不匹配时，两个版本都能在后台解析精确运行时 profile，且只在身份校验通过后应用。
- **易用性**——表格冻结、平滑滚动、可取消的 UI 卡顿检测，以及启动项与网络配置修改上更强的目标校验、恢复和事务处理。

</details>

## 许可证

KSword 按 [KSword Community Source License v1.6](../LICENSE) **源码公开**发布。这里的"开源"仅表示源码可见、可获取，**不**代表本项目使用 OSI 认证的开源许可证。允许做什么以 `LICENSE` 为准，尤其是再分发与商业使用部分。第三方组件继续适用它们自己的许可证。

除另有文件说明外，KSword 自有代码均按 `LICENSE` 发布。[KSword 社区公约](../COMMUNITY_COVENANT.md) 是关于诚实、署名、负责任使用，以及不要把非官方 fork 冒充官方的社区约定，不是另一层许可限制。贡献规则见 [CONTRIBUTING.md](../CONTRIBUTING.md)。
