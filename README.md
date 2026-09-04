<div align="right">
  <a href="./docs/readme_zh.md">简体中文</a> |
  <strong>English</strong>
</div>

<div align="center">

  <img
    src="./Ksword5.1/Ksword5.1/Resource/Logo/KswordHome-En.png"
    alt="KSword ARK Logo"
    width="520"
  />

  <a href="https://github.com/user-attachments/assets/02085a90-af21-4880-b956-d059a655a4da">
    <img
      src="https://github.com/user-attachments/assets/02085a90-af21-4880-b956-d059a655a4da"
      alt="KSword ARK dark interface"
      width="49%"
    />
  </a>
  <a href="https://github.com/user-attachments/assets/aeda0d71-c2c0-4317-abac-0fac811c153d">
    <img
      src="https://github.com/user-attachments/assets/aeda0d71-c2c0-4317-abac-0fac811c153d"
      alt="KSword ARK light interface"
      width="49%"
    />
  </a>

  <br>

  <sub>Dark Mode　|　Light Mode</sub>

</div>

<h1 align="center">Ksword5.1</h1>
<p align="center"><strong>A source-available Windows Anti-Rootkit and kernel analysis suite</strong></p>

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
  <a href="#getting-started">Getting Started</a> ·
  <a href="#features">Features</a> ·
  <a href="#which-edition">Editions</a> ·
  <a href="#building-from-source">Build</a> ·
  <a href="#license">License</a>
</p>

---

## What is KSword?

KSword is an **ARK** (Anti-Rootkit) toolkit for Windows 10/11. It comes with its own kernel driver, so it can look at the same thing — a process, a driver, a network connection — from both user mode and kernel mode at the same time. When the two views don't match, something is hiding.

Think of it as a system-wide X-ray machine: it replaces the dozen separate tools you'd otherwise need for process inspection, memory forensics, network monitoring, disk analysis, driver debugging, and security auditing — all in one window.

**Design philosophy:**

- **Show the evidence, not just the conclusion.** Every field tells you where the data came from, so you can verify it yourself.
- **Look but don't touch (by default).** Viewing never changes your system. Dangerous actions like unloading drivers or writing to disk are behind separate buttons with explicit warnings and, where possible, undo.
- **Tell the truth when something doesn't work.** If a feature needs a kernel offset that isn't available on your Windows build, it says "unsupported" instead of guessing.

Source-available under the [KSword Community Source License](LICENSE) — see [License](#license).

## Getting Started

1. Download and extract the release archive — you get a `Release\` folder.
2. Run **`Launcher.exe`** as administrator.

That's it. The launcher checks your system and picks the right edition for you.

Alternatively, `KswordSetup.exe` is a convenience installer that does the same thing plus creates shortcuts. But just extracting the folder works fine.

> [!IMPORTANT]
> Kernel-level features need admin rights and a loaded driver. Without the driver, the app still works — you just won't see the kernel-side data.

## Features

### See what's really running

Browse all processes, threads, and handles — then compare what Windows reports against what the kernel driver actually finds. Mismatches mean something is hidden. You can also inspect thread stacks, modules, tokens, and process protection levels.

### Inspect memory

Search process memory by pattern, browse regions in hex, set bookmarks. The driver can read memory regions that user-mode tools can't reach, scan kernel memory for executable code, and translate virtual addresses through the page tables.

### Analyze binaries

Drop a PE, ELF, or Mach-O file for a structural scan. There's also a byte editor, but it's deliberately strict: edits must be the same length, it double-checks the file hasn't changed since you opened it, and it writes atomically to prevent corruption.

### Monitor network traffic

Capture and filter packets, manage TCP/UDP connections, throttle traffic per-process, construct HTTP requests, inspect HTTPS sessions. The driver adds its own view of the network stack (TCP, UDP, AFD, NSI, NDIS, WFP) so you can spot connections that user-mode tools miss.

### Debug drivers and kernel internals

Load, unload, and inspect driver services. Examine driver objects, device objects, dispatch tables, and I/O fast paths. Browse the kernel's object namespace, hook tables (SSDT, IAT/EAT, inline), callbacks, and interrupt descriptors. A built-in disassembler lets you read kernel code in place.

The **Kernel Knowledge** center has 71 searchable articles (bilingual) covering kernel concepts, each linked to the live evidence pages where you can see the real data.

### Dig into files and disks

Two-pane file manager with hash checking, digital signatures, PE structure views, and a file unlocker. For forensics: browse raw NTFS volumes, find deleted files, inspect minifilter stacks, and audit storage devices — all read-only by default, with disk writes requiring an explicit unlock.

### Monitor system activity

ETW-based process monitoring, syscall capture, WMI event subscriptions, and a risk aggregation center. Task-Manager-style live charts for CPU, GPU, memory, disk, and network. Window enumeration and message monitoring. Registry browsing with search. Startup item and service management.

### Audit security posture

Check AppLocker rules, WDAC/Code Integrity policies, Defender and ASR settings, VBS/Hyper-V configuration, driver trust, and Windows event logs — all from one place.

### Virtualization lab (HVM)

Run VMX capability tests, launch a one-shot test VM, or start a guarded Intel VT-x/EPT monitor that logs VM exits. Safety checks prevent activation on unsupported hardware. **For authorized lab use only.**

<details>
<summary><b>Full feature list by dock</b> — 17 workspace docks + 4 auxiliary panels</summary>

<br>

For the OpenArk coverage comparison, see [docs/OpenArk功能对照与TODO.md](docs/OpenArk功能对照与TODO.md).

| Dock | What's in it |
|---|---|
| **Welcome** | Version info, build time, user info, project links. |
| **Process** | Process tree/list with icons and change highlighting. Kill/suspend/resume/priority actions. Thread stacks, modules, tokens. R3 vs R0 cross-view for hidden process detection. Recoverable R0 process hiding (gated). PPL/signature operations with risk prompts. |
| **Network** | Packet capture & filtering. TCP/UDP connection management. Per-process throttling. Request builder. HTTPS inspection. ARP/DNS tables. Live host discovery. WFP firewall events & rules. NIDS. Segmented downloads. R0 network stack inventories (TCP/UDP/AFD/NSI/NDIS/WFP). |
| **Memory** | Process memory browsing & search. Hex viewer with bookmarks & breakpoints. R0 memory reads. Kernel executable-memory scan. Memory evidence pages. PTE / virtual-address translation. |
| **File** | Dual-pane file manager. Hashes, signatures, PE/strings/hex views. File unlocker. NTFS recovery. Minifilter/FileObject/Section evidence. Storage-stack & BitLocker info. |
| **Scanner** | Background PE/ELF/Mach-O structural scanning. Guarded byte editor (length-preserving only, atomic writes, optional backup). |
| **Driver** | Driver service management (register/load/unload/delete). Loaded modules. DBWIN debug output. DriverObject/DeviceObject/MajorFunction/FastIo inspection. Transactional editors for dispatch tables and image metadata. Reversible loader-list removal. Driver Integrity checks. Module cross-view. Unloaded-driver & PiDDB evidence. |
| **Kernel** | Object namespace browser. Atom table. SSDT/SSSDT tables. Inline/IAT/EAT hooks. CID cross-view. ALPC/IPC. Dynamic offset management. Capability matrix. Clean loaded-image & IDT baselines. Descriptor-table & IOCTL decoding. Kernel disassembly. Full callback inventory (notify, registry, object, filter, bugcheck, shutdown, filesystem, logon, NMI, etc.). Kernel Knowledge center (71 articles). HVM virtualization lab. |
| **Monitor** | Per-process ETW tracing. Syscall capture. WinAPI agent. WMI subscriptions. ETW provider/session management. ARK risk center. |
| **Hardware** | Task-Manager-style CPU/GPU/memory/disk/network charts. Process I/O & ETW file activity. Device tree (SetupAPI/CfgMgr). R0 device-stack audits (DevNode/USB/HID/PCI/ACPI/GPU/display/watchdog). |
| **Privileges** | Local user accounts. Create user / reset password. Group info. Current process privilege snapshot. |
| **Windows** | Window enumeration, filtering, preview, picking, control. Desktop management. Message monitoring. Win32k GUI/session audit. Hotkey/hook audit. |
| **Registry** | Registry tree browsing. Key/value CRUD. `.reg` import/export. Async search. |
| **Handles** | Handle list with PID/keyword/type filtering. Named-object resolution. Object-type statistics. HandleTable/ObjectHeader/ObjectType evidence. |
| **Startup** | Categorized startup items across logon, services, drivers, scheduled tasks, registry, WMI. Risk-gated modifications with recovery where possible. |
| **Services** | Service table with filtering/sorting. Start/stop/pause. Startup-type changes. Property editing. Dependencies. TSV/JSON export. |
| **Miscellaneous** | BCD/boot config. Audio source attribution. System speed control (with warnings & recovery). Shell association management (context menus, URL handlers, Open With, Explorer Home). Read-only disk editor & raw filesystem forensics (write requires unlock). AppLocker/WDAC/Defender/ASR/platform security diagnostics. |

**Auxiliary panels:** task progress cards, filtered log output with call-chain tracing, an immediate/scratch window, and a real-time performance monitor.

</details>

## Which Edition?

| | **Ksword5.1** (full) | **KswordARKLight** (lightweight) |
|---|---|---|
| Built with | Qt 6, dockable workspace | Pure Win32, no dependencies |
| Best for | Full ARK workflows, deep analysis | Older machines, quick response, minimal footprint |
| Feature set | Everything above | Core set: processes, memory, registry, files, drivers, kernel, monitoring, hardware, windows, startup, network, handles, security |

Both editions talk to the same kernel driver. `Launcher.exe` picks one for you automatically.

## What's in the Repository

| Folder | What it is |
|---|---|
| `Ksword5.1/` | Full Qt desktop application. |
| `KswordARKLight/` | Lightweight Win32 edition. |
| `KswordARKDriver/` | The kernel driver. |
| `Launcher/` | Startup helper that picks the right edition. |
| `KswordCLI/` | Command-line interface — see [CLI docs](docs/CLI使用文档.md). |
| `KswordSetup/` | Optional installer. |
| `Taskbar/` | Top AppBar with `S O S Enter` quick launch. |
| `KswordHUD/`, `APIMonitor_x64/` | HUD overlay and API monitoring helpers. |
| `shared/driver/` | Shared protocol headers between user mode and kernel. |
| `tools/` | PDB offset generator and other build tools. |
| `docs/` | Technical documentation — see [full index](#documentation). |

Website: [KSwordDEV/Website](https://github.com/KSwordDEV/Website)

## How It Works (for contributors)

The app and the driver communicate through a shared protocol layer. If you're contributing code, here's what matters:

- **Protocol headers go in `shared/driver/`.** Don't scatter IOCTL definitions across UI or driver-private folders.
- **UI code never talks to the driver directly.** Everything goes through `ArkDriverClient` (or the lightweight equivalent). No raw `DeviceIoControl` calls from dock code.
- **Kernel offsets come from verified profiles**, not hardcoded values. If the bundled profile doesn't match your Windows build, the app resolves it from PDB symbols at runtime — but only after verifying the binary identity. It never guesses.
- **New source files must be added to `.vcxproj` and `.vcxproj.filters`.** Third-party code keeps its upstream license.

Full contributor notes: [AGENTS.md](AGENTS.md) · [CONTRIBUTING.md](CONTRIBUTING.md)

<details>
<summary><b>Protocol reference</b> — where capabilities are defined in code</summary>

<br>

| What | Where | Notes |
|---|---|---|
| Driver status & feature matrix | `KswordArkCapabilityIoctl.h` | Powers the "Driver Status" page. |
| Dynamic kernel offsets | `KswordArkDynDataIoctl.h` | Profile matching, field sources, capability gates. |
| Process extended info | `KswordArkProcessIoctl.h` (v2) | Session, image path, protection level, field availability. |
| Recoverable process hiding | `IOCTL_KSWORD_ARK_SET_PROCESS_VISIBILITY` | Unlinks process from lists but keeps CID table entry for restore. |
| Protection level changes | `KSW_CAP_PROCESS_PROTECTION_PATCH` | Gated; confirmation dialog shows impact and rollback risk. |
| Vendored offsets | `third_party/systeminformer_dyn/` | System Informer offset data only — no KPH communication layer. |

All headers live under `shared/driver/`.

</details>

## Building from Source

**You need:** Windows 10/11, Visual Studio 2022 (MSVC), Qt 6.9.3 `msvc2022_64` (not needed for the lightweight edition), and WDK (only for the driver).

```powershell
# 1. Tell the build system where Qt lives (run once)
.\Setup-QtPaths.ps1 -QtDir 'C:\Qt\6.9.3\msvc2022_64'

# 2. Build everything
$msbuild = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'
& $msbuild '.\Ksword5.1\Ksword5.1.sln' /t:Build /p:Configuration=Debug /p:Platform=x64 /m
```

Or build just one piece:

```powershell
# Lightweight edition only (no Qt needed)
& $msbuild '.\KswordARKLight\KswordARKLight.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m

# Launcher only
& $msbuild '.\Launcher\Launcher.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m
```

No WDK? Skip the driver — you can build the user-mode parts separately and reuse an existing driver binary for the release.

<details>
<summary><b>Build troubleshooting</b></summary>

<br>

**Main app link fails with `LNK1000` / `IMAGE::BuildImage`**

Do one clean rebuild with Whole Program Optimization off. Don't make it permanent — just that one build. Check the real exit code and that `Ksword5.1\x64\Release\Ksword5.1.exe` is non-zero.

**Driver `ApiValidator` fails after linking**

Usually an architecture mismatch in the WDK post-build step. Run the validator standalone with the x64 WDK path:

```powershell
$solutionDir = (Resolve-Path '.\Ksword5.1').Path + '\'
$apiValidatorX64 = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64'
& $msbuild '.\KswordARKDriver\KswordARKDriver.vcxproj' /t:ApiValidator `
  /p:Configuration=Release /p:Platform=x64 /p:SolutionDir=$solutionDir `
  /p:ApiValidator_ApiExtractorExePath=$apiValidatorX64 /m:1 /v:minimal
```

`Driver is 'Universal'.` means the API check passed, but doesn't prove the full build/sign/load pipeline works.

</details>

## Documentation

| Document | What it covers |
|---|---|
| [CLI使用文档.md](docs/CLI使用文档.md) | Command-line tool reference |
| [功能技术文档.md](docs/功能技术文档.md) | Feature-level technical docs |
| [内核知识中心.md](docs/内核知识中心.md) | Kernel Knowledge center source material |
| [driver_ioctl_audit.md](docs/driver_ioctl_audit.md) | Driver IOCTL audit |
| [OpenArk功能对照与TODO.md](docs/OpenArk功能对照与TODO.md) | OpenArk feature comparison & gaps |
| [动态偏移功能接入步骤.md](docs/动态偏移功能接入步骤.md) | Wiring new features into dynamic offsets |
| [pdb_r0_audit_prep/](docs/pdb_r0_audit_prep/) | PDB/R0 audit preparation |
| [插件系统规范.md](docs/插件系统规范.md) | Plugin system spec |
| [多语言语言包规范.md](docs/多语言语言包规范.md) | Localization rules |

<details>
<summary><b>Recent changes</b></summary>

<br>

- **Kernel Knowledge center** — 71 searchable articles covering kernel internals, each linked to live evidence pages.
- **Scanner dock** — structural scanning for PE/ELF/Mach-O with a safe byte editor.
- **Forensics expansion** — clean baselines for loaded images and IDT, descriptor-table decoding, kernel disassembly, wider R0 network coverage, raw filesystem browser with deleted-entry recovery.
- **HVM** — VMX self-tests, one-shot test guests, and a guarded VT-x/EPT monitor.
- **Runtime PDB resolution** — automatic offset resolution when the bundled profile doesn't match your kernel.
- **Usability** — table-freeze controls, smooth scrolling, cancellable stall detector, better validation and recovery for startup/network changes.

</details>

## Notice

> [!WARNING]
> This project includes system-level debugging, auditing, and management capabilities. Use it only in legally authorized and compliant environments.

## License

KSword is **source-available** under the [KSword Community Source License v1.6](LICENSE).

"Open source" here means you can see and access the source code — it does **not** mean an OSI-approved license. Check `LICENSE` for what's allowed, especially around redistribution and commercial use. Third-party components keep their own licenses.

The [Community Covenant](COMMUNITY_COVENANT.md) is a community promise about honesty, attribution, and responsible use — not additional license restrictions. Contributions follow [CONTRIBUTING.md](CONTRIBUTING.md).

## Star History

<a href="https://www.star-history.com/?repos=KSwordDEV%2FKSword&type=timeline&legend=top-left">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/chart?repos=KSwordDEV/KSword&type=timeline&theme=dark&legend=top-left&sealed_token=hbas9yW4Wjk96TQwUcVo8iWbLMLjxz1Ageym2BTfRw2bV9g97jc35XTCzmb2yHYYxsOm4xNQrBp8kpr-mfkhnFg0-fSBW5otNIhxK0DEocUY0dBWKTMJ0vG7LsEBA0oNQIkZW2pCO44UEI3kps_J3yhO0jN_uvS1AArEXxLA4uGMoiFmiVzWuBuo6KlU" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/chart?repos=KSwordDEV/KSword&type=timeline&legend=top-left&sealed_token=hbas9yW4Wjk96TQwUcVo8iWbLMLjxz1Ageym2BTfRw2bV9g97jc35XTCzmb2yHYYxsOm4xNQrBp8kpr-mfkhnFg0-fSBW5otNIhxK0DEocUY0dBWKTMJ0vG7LsEBA0oNQIkZW2pCO44UEI3kps_J3yhO0jN_uvS1AArEXxLA4uGMoiFmiVzWuBuo6KlU" />
   <img alt="Star History Chart" src="https://api.star-history.com/chart?repos=KSwordDEV/KSword&type=timeline&legend=top-left&sealed_token=hbas9yW4Wjk96TQwUcVo8iWbLMLjxz1Ageym2BTfRw2bV9g97jc35XTCzmb2yHYYxsOm4xNQrBp8kpr-mfkhnFg0-fSBW5otNIhxK0DEocUY0dBWKTMJ0vG7LsEBA0oNQIkZW2pCO44UEI3kps_J3yhO0jN_uvS1AArEXxLA4uGMoiFmiVzWuBuo6KlU" />
 </picture>
</a>
