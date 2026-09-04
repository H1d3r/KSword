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
<p align="center"><strong>A high-coverage source-available Windows ARK and kernel analysis suite</strong></p>

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
  <a href="#whats-in-the-box">Components</a> ·
  <a href="#features">Features</a> ·
  <a href="#safety-model">Safety Model</a> ·
  <a href="#building-from-source">Build</a> ·
  <a href="#documentation">Docs</a> ·
  <a href="#license">License</a>
</p>

---

## What is KSword?

KSword is an **ARK** (*Anti-Rootkit*) and kernel analysis toolkit for Windows 10/11 x64. It pairs a user-mode desktop application with its own kernel driver, so the same object — a process, a driver, a network endpoint, a file — can be observed from both Ring 3 and Ring 0 and the two answers compared. Where they disagree, something is hiding.

Around that core it collects the tooling you would otherwise gather from a dozen separate utilities: memory search and page-table translation, PE/ELF/Mach-O scanning, packet capture and firewall inspection, raw NTFS forensics, callback and hook inventories, registry and startup auditing, device-stack tracing, and Windows security-policy diagnostics.

**Three things shape the design:**

| | |
|---|---|
| **Evidence, not verdicts** | Every audit page shows where each field came from — R3 API, R0 read, PDB symbol, or dynamic offset — so a result can be checked rather than trusted. |
| **Read-only by default** | Inspection never modifies the system. Unloading, patching, hiding, and disk writes live behind separate entry points with explicit confirmation and, where possible, rollback. See [Safety Model](#safety-model). |
| **Honest degradation** | When a kernel offset, privilege, or hardware feature is unavailable, the UI says `unsupported` or `partial`. It does not guess offsets across Windows builds. |

Source-available under the [KSword Community Source License](LICENSE) — see [License](#license) for what that does and does not permit.

## Getting Started

### Run a release build

1. Extract the release archive; you get a `Release\` directory.
2. Run **`Launcher.exe`** as administrator.

`Launcher.exe` is the supported entry point. It reads the bundled support manifest, checks compatibility with the current system, and starts either the full Qt application (`Ksword5.1.exe`) or the lightweight edition (`KswordARKLight.exe`).

`KswordSetup.exe` is an optional convenience installer — it extracts the same payload, creates shortcuts, and can write appearance/startup settings. Extracting `Release\` by hand is functionally equivalent.

> [!IMPORTANT]
> Ring 0 features need administrator privileges, a running `KswordARK` driver service, and compatible system security settings. Test-signing and driver-signing requirements depend on the target machine. Without the driver, the application still runs — R0 pages simply report their capabilities as unavailable.

### Which edition should I run?

| | **Ksword5.1** (full) | **KswordARKLight** |
|---|---|---|
| UI | Qt 6 + ADS dockable workspace | Native Win32, hand-built docks |
| Dependencies | Qt runtime + plugins | None beyond the system |
| Best for | Complete ARK / debugging / audit workflows | Older systems, low-resource machines, rapid response |
| Coverage | Everything below | Processes, memory, registry, files, drivers, kernel, monitoring, hardware, windows, startup, network, handles, misc security |

Both speak the same `shared/driver/` protocols and drive the same kernel driver. `Launcher.exe` picks one for you if you do not care.

## What's in the box

| Component | What it is |
|---|---|
| `Ksword5.1/` | The full Qt/ADS desktop application and the complete ARK workflow. |
| `KswordARKLight/` | Lightweight native Win32 ARK — no Qt, faster startup, focused feature set. |
| `KswordARKDriver/` | The kernel driver: process, thread, handle, memory, network, kernel-object, device, and security audit protocols. |
| `Launcher/` | Pure Win32 startup and compatibility assistant; can also prepare an offline collection bundle when loaded kernel modules have missing offsets. |
| `KswordCLI/` | Command-line interface for automation, validation, and troubleshooting — see [docs/CLI使用文档.md](docs/CLI使用文档.md). |
| `KswordSetup/` | Optional installer that unpacks the release payload and creates shortcuts. |
| `Taskbar/` | Top AppBar with status display and the `S O S Enter` quick launch. |
| `KswordHUD/`, `APIMonitor_x64/` | HUD helper and API-monitoring injection component. |
| `shared/driver/` | Shared R0/R3 IOCTL protocol headers — the contract between user mode and the driver. |
| `tools/pdb_offset_generator/` | Generates and validates the PDB offset / DynData profile packs. |
| `third_party/systeminformer_dyn/` | Vendored System Informer dynamic-offset snapshot, with upstream LICENSE/NOTICE. |

The [KSwordDEV/Website](https://github.com/KSwordDEV/Website) repository independently maintains the project website and per-module documentation.

## Features

Grouped by what you are investigating. The full dock-by-dock inventory is collapsed at the end of this section.

### Processes and threads

Process tree and list with icons and change highlighting; terminate, suspend, resume, priority, and critical-process actions; thread stacks, modules, and tokens; process details with PDB field diagnostics. **R3/R0 cross-view** compares user-mode enumeration against the driver's own walk to surface hidden processes, threads, and CID entries. Recoverable R0 process hiding is available as an explicitly gated action.

### Memory

Region browsing, pattern search, hex viewing, bookmarks and breakpoints; R0 region reads; kernel executable-memory scanning; kernel and process memory evidence; PTE / virtual-address translation.

### Binaries

Background structural scanning of **PE, ELF, and Mach-O** files. The optional byte editor accepts length-preserving edits only: it revalidates the source snapshot, replaces the target atomically, and can keep a backup — after you acknowledge the risk.

### Network

Packet capture and filtering, TCP/UDP connection management, per-process throttling, request construction, HTTPS analysis, ARP/DNS, live-host discovery, WFP firewall events and rules, NIDS, and segmented downloads — alongside read-only R0 inventories for TCP, UDP, AFD, NSI, NDIS, and WFP.

### Drivers and kernel objects

Driver service registration, load, unload, and delete; loaded modules and DBWIN output; DriverObject / DeviceObject / MajorFunction / FastIo diagnostics; transactional MajorFunction and image-metadata editors; reversible loader-list removal; Driver Integrity; Module Cross-View; unloaded-driver and PiDDB evidence. On the kernel side: recursive object namespace, atom table, SSDT/SSSDT, inline hooks, IAT/EAT, ALPC/IPC, clean loaded-image and IDT baselines, descriptor-table and IOCTL decoding, and kernel disassembly.

Callback inventory covers notify, registry, object, filter, bugcheck, shutdown, file-system, logon, CallbackObject, image-verification, and NMI sources, with module ownership and snapshot/row identity diagnostics.

### Files, storage, and devices

Dual-pane file management, ownership and permissions, hashes, signatures, PE/strings/hex views, file unlocker, NTFS recovery; minifilter, FileObject, Section, storage-stack and BitLocker evidence; **read-only raw filesystem browsing and deleted-entry forensics**; disk I/O monitoring; SetupAPI/CfgMgr device tree and R0 DevNode/USB/HID/PCI/ACPI/GPU/display/watchdog audits.

### Monitoring and system state

Target-process-tree ETW, syscall capture, WinAPI agent, WMI subscriptions, ETW provider/session management, and an ARK risk center. Task-Manager-style performance views for CPU, GPU, memory, disk, and network. Window enumeration, picking, control, and structured win32k GUI/session plus hotkey/hook audit. Registry browsing with CRUD, `.reg` import/export, and async search. Categorized startup items, service management, local accounts and privileges.

### Security posture

AppLocker, WDAC / Code Integrity, Defender / ASR, VBS / Hyper-V, platform security, driver trust, and event-log diagnostics.

### Kernel Knowledge center

A bilingual, full-text-searchable catalog of **71 articles** across 12 categories. Each article carries a complete eight-part write-up, a versioned R3/R0 live-context query (request, CPU, timing, WDF/WDM evidence), centrally verified business IOCTL sources, Microsoft Learn references, and a read-only route into the corresponding evidence page.

### Virtualization (HVM)

Confirmation-gated VMX self-tests, a one-shot test guest, and a guarded resident Intel VT-x/EPT monitor with VM-exit telemetry. Resident start is refused on AMD, under an existing hypervisor, or when power/topology/unload lifecycle guards are unavailable. **Authorized lab and diagnostic use only.**

<details>
<summary><b>Full inventory by dock</b> — 17 workspace docks and 4 auxiliary panels</summary>

<br>

Based on current code, dock-initialization logic, and the R0/R3 protocols. For the OpenArk coverage comparison and remaining gaps, see [docs/OpenArk功能对照与TODO.md](docs/OpenArk功能对照与TODO.md).

Settings have moved from the primary docks to the top menu.

| Dock | Subpages / key areas | Capabilities |
|---|---|---|
| **Welcome** | Welcome page | Version, build time, user information, avatar, and project entry points. |
| **Process** | Process list, create process, details, threads, modules, tokens, Cross-View, PDB Catalog | Process tree/list, icons and difference highlighting, terminate/suspend/resume/priority/critical-process actions, recoverable R0 hiding, R3/R0 process and thread comparison, thread stacks, and risk prompts for PPL/Signature/CID operations. |
| **Network** | Traffic monitor, per-process throttling, connection management, request builder, HTTPS, ARP/DNS, live hosts, firewall, NIDS, downloads, network audit | Packet capture and filtering, TCP/UDP connection management, WFP firewall events and rules, real-time detection, segmented HTTP/HTTPS downloads, and read-only R0 TCP/UDP/AFD/NSI/NDIS/WFP inventories and cross-view. |
| **Memory** | Processes and modules, regions, search, viewer, breakpoints/bookmarks, R0 read/write, Kernel Exec Scan, Memory Evidence, PTE | R3 memory browsing/search, R0 region reads, kernel executable-memory scanning, kernel/process memory evidence, and page-table/virtual-address translation. |
| **File** | File manager, recovery, properties, unlock, Minifilter, FileObject, Section, Storage/BitLocker | Dual-pane management, ownership/permission handling, hashes/signatures/PE/strings/hex, NTFS recovery, file-lock and Section mappings, and read-only storage-stack/BitLocker evidence. |
| **Scanner** | Structured scan, guarded byte editor | Background structural scanning for PE, ELF, and Mach-O files. The optional editor permits only length-preserving edits after explicit risk acknowledgement; it revalidates the original snapshot, atomically replaces the target, and can retain a backup. |
| **Driver** | Overview, operations, debug output, object information, integrity, module Cross-View, Unloaded/PiDDB | Driver-service registration/load/unload/delete, loaded modules, DBWIN output, DriverObject/DeviceObject/MajorFunction/FastIo, atomic MajorFunction and image-metadata transactions, reversible loader-list removal, Driver Integrity, unloaded-driver/PiDDB evidence, and warn-only explicitly confirmed operator actions. |
| **Kernel** | Object namespace, atom table, NtQuery, SSDT, SSSDT, Inline Hook, IAT/EAT, CID, IPC, DynData, driver status, callbacks, baselines, HVM, Kernel Knowledge | Recursive object directories, BaseNamedObjects, NamedPipe, symbolic links, device/driver objects, object-type matrix, CID/cross-view, ALPC/IPC, dynamic offsets, capability matrix, clean loaded-image and IDT baselines, descriptor-table/IOCTL decoding, kernel disassembly, and callback inventory/management. The bilingual Kernel Knowledge center adds 71 searchable articles, a versioned live R0 context/source query, official references, and read-only routes into existing evidence pages without hiding runtime degradation. The HVM workflow has explicit confirmations for VMX self-tests, one-shot test guests, and guarded resident Intel VT-x/EPT monitoring. |
| **Monitor** | Process targeting, direct kernel calls, WinAPI, WMI, ETW, Risk Center | Target-process-tree ETW, syscall capture, WinAPI Agent, WMI subscriptions, ETW provider/session management, and ARK risk aggregation. |
| **Hardware** | Utilization, overview, CPU, GPU, memory, disk monitoring, device management, R0 device audit | Task-Manager-style performance views, dynamic disk/network/GPU cards, process I/O and ETW file activity, SetupAPI/CfgMgr device tree, and DevNode/USB/HID/PCI/ACPI/GPU/display/watchdog audit. |
| **Privileges** | Accounts, privileges | Local users, create user/reset password, group information, and the current process privilege snapshot. |
| **Windows** | Window list, desktop/window details, Win32k/GUI, hotkeys/hooks, clipboard, GPU/display | Window enumeration, filtering, preview, picking, control, desktop management, message monitoring, and structured win32k GUI/session plus hotkey/hook audit. |
| **Registry** | Tree, value list, search results | Registry browsing, key/value CRUD, `.reg` import/export, asynchronous search, and navigation. |
| **Handles** | Handle list, object types, object details | PID/keyword/type filtering, named-object resolution, object-type statistics, and HandleTable/ObjectHeader/ObjectType evidence. |
| **Startup** | Overview, logon, services, drivers, scheduled tasks, advanced registry, WMI | Categorized startup overview, icon rendering, filtering/export, file and registry location lookup, recovery-aware changes, and navigation to service management. Risk-gated actions validate targets before permanent removal and retain recovery transactions where the source supports them. |
| **Services** | Main service table, general, logon, recovery, dependencies, audit | Service filtering/sorting, startup-type changes, start/stop/pause/continue, property editing, dependency/audit information, and TSV/JSON export. |
| **Miscellaneous** | Boot, sound sources, system speed, Shell association management, disk editing, raw filesystem forensics, application control | BCD/boot entry points; Core Audio sound-source attribution; R0 system-wide speedup/slowdown with persistent warnings, two-step confirmation, and a recovery path; management of context menus, URL bindings, file Open With handlers, format-specific menus, and third-party Explorer Home entries; read-only disk editing, raw filesystem browsing, and deleted-entry analysis by default (writes require unlocking); and AppLocker/WDAC/Defender/ASR/platform-security/event-log diagnostics. |

**Auxiliary panels**

| Panel | Key areas | Capabilities |
|---|---|---|
| Current Operations | Task cards | Steps and progress of background tasks; hides automatically when complete. |
| Log Output | Level filters, log table, context menu | Log filtering, copy/export, double-confirmation clearing, and GUID call-chain tracing. |
| Immediate Window | Code/text editor | Quick verification, temporary notes, and immediate output. |
| Monitor Panel | CPU/memory/disk/network charts | Bottom real-time performance monitor with multi-line throughput trends. |

**Workspace behaviour** — ADS layout persistence and restoration, lazy initialization of visible docks, table-freeze controls, smooth scrolling, a cancellable UI stall detector, top-menu settings, and UIAccess / always-on-top policies.

</details>

## Safety Model

This project ships kernel-level read *and* write capabilities. The rules below are not advisory — they are how the code is structured, and contributions are expected to follow them.

- **Audit pages are read-only.** Enumerating, decoding, and comparing never mutate system state.
- **Mutations are separate.** Unload, delete, patch, bypass, disk-write, and similar actions require their own entry point, a risk notice, and a rollback or audit strategy. Some are two-step confirmations with persistent warnings.
- **Capabilities gate R0.** Any feature depending on undocumented kernel fields must declare its required capability. The dispatch layer enforces the gate *before* the handler runs; missing DynData or a mismatched profile degrades safely or fails closed.
- **Offsets are never guessed.** PDB/DynData profiles are applied only after PE/PDB identity checks pass. Unsupported builds report `unsupported` instead of reading a plausible-looking address.
- **Degradation is visible.** Runtime `unsupported`, `partial`, truncation, DynData, privilege, and hardware limits are shown in the UI rather than silently swallowed.

The Kernel dock's **Driver Status** page surfaces all of this at a glance: Driver Loaded/Missing, Protocol Mismatch, DynData Missing, Limited, the active security policy, the most recent R0 error, and the full feature-capability matrix.

> [!WARNING]
> KSword includes system-level debugging, auditing, and management capabilities. Use it only in legally authorized and compliant environments.

## Building from Source

### Requirements

- Windows 10/11 x64
- Visual Studio 2022 with MSVC and MSBuild
- Qt **6.9.3** `msvc2022_64` — for the Qt application and helper UIs. `KswordARKLight` and `Launcher` do not need Qt.
- WDK — only for building the kernel driver

Keep the standard MSVC toolchain; the project is not adapted to LLVM.

### Quick build

Run the repository script first so the local Qt path is discovered and stored centrally, instead of being baked into individual `.vcxproj` files:

```powershell
# From the repository root; substitute your Qt installation path
.\Setup-QtPaths.ps1 -QtDir 'C:\Qt\6.9.3\msvc2022_64'

# Substitute your Visual Studio path; a Developer PowerShell can call msbuild directly
$msbuild = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'

# Full solution: main application, Taskbar, HUD, driver, CLI, installer, lightweight edition
& $msbuild '.\Ksword5.1\Ksword5.1.sln' /t:Build /p:Configuration=Debug /p:Platform=x64 /m
```

Individual targets:

```powershell
# Lightweight ARK only
& $msbuild '.\KswordARKLight\KswordARKLight.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m

# Native launcher (also generates the readable release support manifest)
& $msbuild '.\Launcher\Launcher.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m
```

No WDK or driver-signing environment on this machine? Build the user-mode projects only — the release process can reuse an existing unsigned R0 artifact.

<details>
<summary><b>Troubleshooting: MSVC linker and WDK validator failures</b></summary>

<br>

**`LNK1000` with `IMAGE::BuildImage`, or an `.iobj` failure, when linking the main application**

Run one clean rebuild with Whole Program Optimization and LTCG disabled — **for that build only**. Do not edit the project files to make it permanent, and do not immediately run another normal build afterwards: the WPO-disabled output invalidates the incremental-build cache. Confirm the real MSBuild exit code and a nonzero `Ksword5.1\x64\Release\Ksword5.1.exe` rather than trusting the console tail. Updating, downgrading, or reinstalling MSVC is only worth considering if this one-shot recovery still reproduces the failure.

**WDK post-build `ApiValidator` / `aitstatic` failure on the x64 driver**

This is often an architecture-selection problem that appears *after* `KswordARK.sys` has already linked. Verify the `.sys` was freshly linked and that the stalled MSBuild has no active compiler, linker, or validator child process before stopping that exact process. Then run the validator on its own, pointing at the x64 WDK binary directory (substitute your installed WDK version):

```powershell
$solutionDir = (Resolve-Path '.\Ksword5.1').Path + '\'
$apiValidatorX64 = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64'
& $msbuild '.\KswordARKDriver\KswordARKDriver.vcxproj' /t:ApiValidator `
  /p:Configuration=Release /p:Platform=x64 /p:SolutionDir=$solutionDir `
  /p:ApiValidator_ApiExtractorExePath=$apiValidatorX64 /m:1 /v:minimal
```

`Driver is 'Universal'.` confirms standalone API/architecture validation only. It does **not** prove a full driver build, INF/CAT generation, signing, or load acceptance — validate and report those separately. A compiler or linker failure occurring before the `.sys` is updated is a real build failure and must be fixed directly.

</details>

### Packaging

A release archive has a `Release\` root containing `Launcher.exe`, `Ksword5.1.exe`, `KswordARKLight.exe`, the helper programs, the driver, `profiles\launcher_support_manifest.json`, the DynData packs, and the Qt dependencies and plugin directories. Shortcuts and the installer's post-install action all launch `Launcher.exe`.

## Architecture

The conventions below define how R3 and R0 talk to each other. Full contributor notes are in [AGENTS.md](AGENTS.md) and [CONTRIBUTING.md](CONTRIBUTING.md).

- **One protocol home.** Shared R0/R3 protocols live under `shared/driver/`. New IOCTL headers, structures, and version fields must not be scattered across UI or driver-private directories.
- **One dispatch path.** Driver IOCTL handlers register through `KswordARKDriver/src/dispatch/ioctl_registry.c`; `ioctl_dispatch.c` only looks up, validates, invokes, logs, and completes.
- **One client.** User-mode access to the KswordARK device goes through `Ksword5.1/Ksword5.1/ArkDriverClient/` or the lightweight equivalent. Dock and UI code never opens the device or issues raw `DeviceIoControl` calls.
- **Offsets come from profiles.** PDB/DynData uses the v4 profile pack from `tools/pdb_offset_generator/`; release packages carry only `ark_dyndata_pack_v4.json.qz`. If the packaged profile does not match the running kernel, both applications can resolve an exact runtime PDB profile in a serialized background DbgHelp session, applied only after identity checks pass.
- **Build files are part of the change.** New source files must be added to the matching `.vcxproj` and `.vcxproj.filters`. Third-party integrations keep their upstream license text.

<details>
<summary><b>Protocol reference</b> — where specific capabilities are defined</summary>

<br>

| Area | Header / mechanism | Notes |
|---|---|---|
| Driver status & capabilities | `shared/driver/KswordArkCapabilityIoctl.h` | Backs the Driver Status page: loaded/missing, protocol mismatch, DynData missing, limited, security policy, last R0 error, capability matrix. |
| Dynamic offsets | `shared/driver/KswordArkDynDataIoctl.h` | The Dynamic Offsets page shows profile matches, field sources, and capability gates through `ArkDriverClient`. The main window refreshes and applies the profile automatically once the driver loads. |
| Process extended info | `shared/driver/KswordArkProcessIoctl.h` (v2) | Session, full image path, Protection/SignatureLevel, ObjectTable/SectionObject availability, field source, DynData capability. With DynData missing, ProcessDock/ProcessDetail shows availability only — it does not enumerate the handle table or Section directly. |
| Recoverable process hiding | `IOCTL_KSWORD_ARK_SET_PROCESS_VISIBILITY` | Changes `_EPROCESS.UniqueProcessId` and unlinks `ActiveProcessLinks` while keeping the PspCidTable record, so the original PID can restore it. |
| PPL / protection changes | Capability `KSW_CAP_PROCESS_PROTECTION_PATCH` | The user-mode confirmation dialog must show current/target Protection, SignatureLevel impact, field source, and rollback risk. |
| Vendored DynData | `third_party/systeminformer_dyn/` | Only the System Informer `KphDynConfig` data and a lightweight parser are integrated — not the KPH communication layer, object system, or session tokens. |

</details>

## Documentation

| Document | Contents |
|---|---|
| [docs/CLI使用文档.md](docs/CLI使用文档.md) | KswordCLI command reference. |
| [docs/功能技术文档.md](docs/功能技术文档.md) | Feature-level technical documentation. |
| [docs/内核知识中心.md](docs/内核知识中心.md) | Source material behind the Kernel Knowledge center. |
| [docs/driver_ioctl_audit.md](docs/driver_ioctl_audit.md) | Driver IOCTL surface audit. |
| [docs/OpenArk功能对照与TODO.md](docs/OpenArk功能对照与TODO.md) | OpenArk coverage comparison and remaining gaps. |
| [docs/动态偏移功能接入步骤.md](docs/动态偏移功能接入步骤.md) | How to wire a new feature into the DynData pipeline. |
| [docs/pdb_r0_audit_prep/](docs/pdb_r0_audit_prep/) | PDB/R0 audit-preparation and acceptance documents. |
| [docs/插件系统规范.md](docs/插件系统规范.md) | Plugin system specification. |
| [docs/多语言语言包规范.md](docs/多语言语言包规范.md) | Localization and language-pack rules. |
| [AGENTS.md](AGENTS.md) | Release build, packaging, and verification procedure. |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Contribution workflow. |

<details>
<summary><b>Recent changes</b></summary>

<br>

- **Kernel Knowledge center** — 71 bilingual searchable topics across 12 categories in the Kernel dock, each with a full article, a versioned R3/R0 live-context query, verified business IOCTL sources, and a route into the matching evidence page.
- **Scanner dock** — background structural scanning for PE, ELF, and Mach-O, with a deliberately constrained length-preserving byte editor.
- **Forensics expansion** — clean loaded-image and IDT baselines, descriptor-table and IOCTL decoding, kernel disassembly, wider R0 network inventories, and a raw filesystem browser with deleted-entry analysis.
- **HVM** — confirmation-gated VMX self-tests, a one-shot guest, and a guarded resident Intel VT-x/EPT monitor with VM-exit telemetry.
- **Runtime PDB resolution** — when a packaged DynData profile does not match, both applications can resolve an exact runtime profile in the background, applied only after identity checks.
- **Usability** — table-freeze controls, smooth scrolling, a cancellable UI stall detector, and stronger target validation, recovery, and transaction handling for startup and network-configuration changes.

</details>

## License

KSword is **source-available** under the [KSword Community Source License v1.6](LICENSE). Here, "open source" means the source is visible and accessible — it does **not** mean the project uses an OSI-approved license. Follow the terms in `LICENSE` for what is permitted, especially around redistribution and commercial use. Third-party components keep their own licenses.

Except where a file states otherwise, KSword's own code follows `LICENSE`. The [KSword Community Covenant](COMMUNITY_COVENANT.md) is about honesty, attribution, responsible use, and not passing an unofficial fork off as official — a community promise, not another layer of license restrictions. Contributions follow [CONTRIBUTING.md](CONTRIBUTING.md).

## Star History

<a href="https://www.star-history.com/?repos=KSwordDEV%2FKSword&type=timeline&legend=top-left">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/chart?repos=KSwordDEV/KSword&type=timeline&theme=dark&legend=top-left&sealed_token=hbas9yW4Wjk96TQwUcVo8iWbLMLjxz1Ageym2BTfRw2bV9g97jc35XTCzmb2yHYYxsOm4xNQrBp8kpr-mfkhnFg0-fSBW5otNIhxK0DEocUY0dBWKTMJ0vG7LsEBA0oNQIkZW2pCO44UEI3kps_J3yhO0jN_uvS1AArEXxLA4uGMoiFmiVzWuBuo6KlU" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/chart?repos=KSwordDEV/KSword&type=timeline&legend=top-left&sealed_token=hbas9yW4Wjk96TQwUcVo8iWbLMLjxz1Ageym2BTfRw2bV9g97jc35XTCzmb2yHYYxsOm4xNQrBp8kpr-mfkhnFg0-fSBW5otNIhxK0DEocUY0dBWKTMJ0vG7LsEBA0oNQIkZW2pCO44UEI3kps_J3yhO0jN_uvS1AArEXxLA4uGMoiFmiVzWuBuo6KlU" />
   <img alt="Star History Chart" src="https://api.star-history.com/chart?repos=KSwordDEV/KSword&type=timeline&legend=top-left&sealed_token=hbas9yW4Wjk96TQwUcVo8iWbLMLjxz1Ageym2BTfRw2bV9g97jc35XTCzmb2yHYYxsOm4xNQrBp8kpr-mfkhnFg0-fSBW5otNIhxK0DEocUY0dBWKTMJ0vG7LsEBA0oNQIkZW2pCO44UEI3kps_J3yhO0jN_uvS1AArEXxLA4uGMoiFmiVzWuBuo6KlU" />
 </picture>
</a>
