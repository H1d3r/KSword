# 蓝屏 BGP 准备与 BPP 哨兵

## 已确认行为

- Windows 10 19042 的 BGP 私有 `GetBpp` 在驱动加载期尚未调用 `InbvAcquireDisplayOwnership` 时可能返回 `1`，同时分辨率返回 `0×0`。这是未取得显示所有权的延迟探测状态，不能直接判定为不支持。
- 加载期仍需在 `PASSIVE_LEVEL` 完成全部资源准备。当前实现同时生成并解析 24 BPP、32 BPP 的 Logo 与黑色/蓝色 ASCII 字形矩形。
- 崩溃回调中的顺序保持为 `InbvAcquireDisplayOwnership → BgpFwAcquireLock → 重新读取分辨率/BPP → BgpClearScreen → BgpGxDrawRectangle → BgpFwReleaseLock`。
- 取得显示所有权后只接受实际 BPP 为 24 或 32。分辨率、BPP、私有特征或节属性不满足时，在清屏前释放锁并退出，保留 Windows 原蓝屏。
- VMware 的 Windows 10 19042 蓝屏显示模式可能固定回落到 `640×480×32`，即使桌面分辨率更高。面板必须保留 `640×480` 紧凑布局；`1024×768` 只能作为完整布局阈值，不能作为 BGP 可用性的最低门槛。
- `640×480`/`800×600` 紧凑页使用左上角 `240×84` Logo 和双栏正文。正文中间需为 Windows 转储进度文字保留空白横带；内部 BGP 阶段、锁状态和回调位图保存在 SecondaryDumpData，不占用户可见页面。

## 诊断依据

- `C:\Windows\Temp\KswordARK-bgp-preparation.log` 用于判断加载期是否成功 Arm。
- 修复前典型状态为 `state=1 (query-only)`、`preparation_stage=2 (read-screen)`、`0xC00000BB`，即回调已注册但绘制不会启动。
- 修复后应看到 `state=3 (armed)`、`preparation_stage=8 (complete)`、`feature_mask=0x000001FF`。加载期完全隐藏模式时，`screen=0x0x1` 与 `last_probe=0x0x1` 属于预期状态。
- 日志中没有 `last_probe=` 时，目标机仍在使用旧驱动。
- 崩溃阶段数据继续通过 GUID `956d0947-326a-4ba7-92f1-4c8b5a5c712d` 写入 `KbCallbackSecondaryDumpData`。
- 阶段序列结束于 `ScreenAfter` 后的 `Rejected|2`，且快照显示真实屏幕 `640×480×32`、要求 `1024×768`、`ClearStatus=STATUS_PENDING` 时，说明回调与 BGP 获取链路均已执行，未清屏仅由尺寸门槛触发。

## 图像资源

- #174 起 Logo 统一采用 qrc 中的 `KswordHome-En.png`（KSwordDEV 版本），不要改回带 KSwordDEV Team 字样的 `MainLogo.png`；BGP 使用离线缩放为 `240x84` 的 `Generated/MainLogoBitmap.h`，SVGA 由主程序上传同一资源。
- 运行时不读取外部 BMP 文件。BMP 头、像素缓冲和 BGP 矩形均在驱动加载期动态建立。
- 当前目标机的 BGP 32 BPP 矩形路径不按 alpha 混合字形背景；`BGRA=00 00 00 00` 会显示成黑色字符块。字形背景必须与当前实色画布完全一致并保持不透明；#174 深色布局使用 `RGB(5,15,33)`。
- 主程序在 R3 使用 Qt 返回的 Windows 系统 `FixedFont` 栅格化可打印 ASCII 的 A8 覆盖率图集，再通过版本化 IOCTL 一次性上传并固定在驱动非分页内存；仓库不写死或捆绑字体文件，也不新增字体许可证。蓝屏阶段不得调用 GDI、DirectWrite 或读取字体文件。
- BGP 的普通字形单元固定为 `10x17`、大号字形为 `20x34`，外围都保留 1 像素不透明同底色边框，因此解析矩形分别为 `12x19` 和 `22x36`；未收到 R3 图集时，把内置 `8x12` 字模居中到相同单元中作为依赖无关的兜底，布局坐标不得改变。

## #174 分区式诊断布局

- `bugcheck_layout.c/.h` 是 BGP 与 VMware SVGA 共用的信息架构：深海军蓝底、蓝色强调、浅色正文、弱化标签和一像素分区边框；所有文字统一通过 `DrawText(..., TextStyle)` 回调，正文和大号停止码只由 `TextStyle` 区分，分区框继续使用 `DrawFrame`。
- #174 信息完整版优先匹配 `1280x720` 或更大画布；`1024x768` 继续使用原完整布局，其余受支持画布走 `640x480` 双栏紧凑版。紧凑版 `Y=228..284`、`1024x768` 版底部状态带保持空白，给 Windows 自己的转储进度文字使用。
- 崩溃页只显示已捕获的 BugCheck 参数、候选模块、CPU/IRQL、回调与转储阶段；没有可靠来源的栈、反汇编、进程名和百分比不得按参考图伪造。
- BGP 的字形、Logo 与每种固定尺寸的横/竖边框矩形都在 `PASSIVE_LEVEL` 解析并保留 backing buffer；崩溃回调只清屏、绘制预生成矩形并格式化到栈上固定缓冲，不分配内存。
- `1280x720` 信息完整版使用 8 个预生成分区框。删除 `DUMP STATUS/INFO` 以及注定无法在高 IRQL 回调解析的线程、栈、指令、Blackbox、事件等空面板，也不再显示成排的 `DUMP ONLY`/`NOT CAPTURED` 占位行；空间改用于参数语义、故障证据、模块归因、回调健康、缓存状态、渲染契约和行动建议。
- 诊断配色保持克制：红色只用于页眉中的停止码名称和 2 倍字号数值，正文不再重复红色；蓝色仅作标题或已解码值的轻量强调；已捕获状态显示正文白色，依赖转储的内容显示弱化灰色，不再同时使用红、黄、绿、蓝争抢注意力。BGP 的 24/32 BPP 普通字形、红色 2 倍字号字形和 VMware SVGA 调色板必须同步。
- 三种固定布局都必须把 BugCheck 名称与大号 `0x........` 放在页眉第一视觉层；完整版和详细版的右上角保留原 8x12 白色玩笑文案，并在最右侧单独显示 `CRASH CPU` 与 `IRQL`。640x480 紧凑版空间不足时保留第一句玩笑，但 CPU/IRQL 仍固定在最右侧。
- BGP 的大号字形与普通字形一样，必须在 `PASSIVE_LEVEL` 从同一系统字体图集预生成；崩溃回调只能绘制已经解析的矩形。VMware SVGA 直接混合相同 A8 覆盖率，两条渲染路径必须使用同一 `DrawText` 接口、布局坐标和色彩角色。
- `0xEF CRITICAL_PROCESS_DIED` 的参数 1 是进程对象，参数 2 只区分进程/线程，参数 3/4 保留；它没有直接故障 IP，页面应显示对象语义并明确要求转储归因，不能输出 `ARG0`、零 RIP 或伪造模块。`KBUGCHECK_DUMP_IO.Offset == -1` 表示顺序写入，应显示 `SEQUENTIAL` 而不是全 `F` 地址。
- 直接地址解码以微软参数定义为准：`0x116/0x117` 使用参数 2，`0xC5` 使用参数 4，`0x50/0xD5` 使用参数 3；`0xBE` 的参数 3/4 是保留项，不得参与模块归因。不得再把四个原始参数无差别扫描为模块地址。纯解码逻辑位于 `bugcheck_decode.c`，三种布局由 `tools/bugcheck_layout_replay/run.ps1` 回放验证。
- Release x64 驱动链接后，工程必须把最新 `KswordARK.sys/.pdb/.inf` 同步到 `Ksword5.1/x64/Release/KswordARKDriver/`；变体签名完成后再同步最终 `.sys`，避免测试包继续携带旧驱动。
- 替换磁盘上的 `.sys` 不会更新已经加载的内核映像。实体机或虚拟机复测蓝屏重绘前，必须停止/卸载旧服务并重新加载驱动；无法安全卸载时重启系统，再核对目标 `.sys` 的哈希或加载日志。
