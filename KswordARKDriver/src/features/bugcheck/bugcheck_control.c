/*++

Module Name:

    bugcheck_control.c

Abstract:

    按需安装蓝屏诊断的控制层。驱动加载只初始化本文件的同步状态，
    不扫描 ntoskrnl、不解析 BGP 私有函数，也不注册 BugCheck 回调。

Environment:

    Kernel-mode Driver Framework

--*/

#include "bugcheck_internal.h"
#include "bugcheck_bgp.h"

// 生命周期状态只允许由控制锁持有者修改，避免并发 IOCTL 重复扫描和重复注册回调。
#define KSWORD_ARK_BUGCHECK_CONTROL_INACTIVE     0L
#define KSWORD_ARK_BUGCHECK_CONTROL_INSTALLING   1L
#define KSWORD_ARK_BUGCHECK_CONTROL_INSTALLED    2L
#define KSWORD_ARK_BUGCHECK_CONTROL_UNINSTALLING 3L

// 控制锁在 DriverEntry 阶段建立，不放入会被完整初始化例程清零的诊断状态结构。
static FAST_MUTEX g_KswordArkBugcheckControlLock;
static volatile LONG g_KswordArkBugcheckControlReady = 0L;
static volatile LONG g_KswordArkBugcheckControlLifecycle =
    KSWORD_ARK_BUGCHECK_CONTROL_INACTIVE;
static PDRIVER_OBJECT g_KswordArkBugcheckControlDriverObject = NULL;
static WDFDEVICE g_KswordArkBugcheckControlDevice = WDF_NO_HANDLE;

static ULONG
KswordARKBugcheckControlCallbackMask(
    VOID
    )
{
    ULONG callbackMask = 0UL;

    // 四种 Windows BugCheck 回调都完成注册，才向 R3 报告完整回调集合可用。
    if (g_KswordArkBugcheckState.ClassicRegistered) {
        callbackMask |= 0x00000001UL;
    }
    if (g_KswordArkBugcheckState.SecondaryRegistered) {
        callbackMask |= 0x00000002UL;
    }
    if (g_KswordArkBugcheckState.DumpIoRegistered) {
        callbackMask |= 0x00000004UL;
    }
    if (g_KswordArkBugcheckState.TriageRegistered) {
        callbackMask |= 0x00000008UL;
    }
    return callbackMask;
}

static VOID
KswordARKBugcheckControlFillResponse(
    _Out_ KSWORD_ARK_BUGCHECK_DIAGNOSTICS_RESPONSE* Response,
    _In_ ULONG ProtocolStatus,
    _In_ NTSTATUS LastStatus
    )
{
    KSWORD_ARK_BGP_DUMP_STATE bgpSnapshot;
    ULONG callbackMask = 0UL;
    LONG lifecycle = KSWORD_ARK_BUGCHECK_CONTROL_INACTIVE;

    // 所有响应字段先归零，防止失败路径向 R3 泄漏未初始化的内核栈内容。
    RtlZeroMemory(Response, sizeof(*Response));
    Response->size = sizeof(*Response);
    Response->version = KSWORD_ARK_BUGCHECK_DIAGNOSTICS_PROTOCOL_VERSION;
    Response->status = ProtocolStatus;
    Response->lastStatus = (LONG)LastStatus;

    // BGP 快照只读取已发布的非分页状态，即使诊断尚未安装也会返回零值摘要。
    RtlZeroMemory(&bgpSnapshot, sizeof(bgpSnapshot));
    KswordARKBugcheckBgpSnapshot(&bgpSnapshot);
    Response->bgpState = bgpSnapshot.State;
    Response->bgpPreparationStage = bgpSnapshot.PreparationStage;
    Response->bgpPreparationStatus = (LONG)bgpSnapshot.PreparationStatus;
    Response->panelStatus = (LONG)bgpSnapshot.PreparationStatus;

    lifecycle = InterlockedCompareExchange(
        &g_KswordArkBugcheckControlLifecycle,
        KSWORD_ARK_BUGCHECK_CONTROL_INACTIVE,
        KSWORD_ARK_BUGCHECK_CONTROL_INACTIVE);
    callbackMask = KswordARKBugcheckControlCallbackMask();
    Response->callbackMask = callbackMask;
    if (lifecycle == KSWORD_ARK_BUGCHECK_CONTROL_INSTALLED) {
        Response->stateFlags |= KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATE_INSTALLED;
    }
    if (callbackMask == 0x0000000FUL) {
        Response->stateFlags |= KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATE_CALLBACKS_READY;
    }
    if (bgpSnapshot.State == KswordArkBgpStateReady ||
        bgpSnapshot.State == KswordArkBgpStateArmed ||
        bgpSnapshot.State == KswordArkBgpStateDrawn) {
        Response->stateFlags |= KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATE_BGP_BACKEND_READY;
    }
    if (bgpSnapshot.State == KswordArkBgpStateArmed ||
        bgpSnapshot.State == KswordArkBgpStateDrawn) {
        Response->stateFlags |= KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATE_PANEL_READY;
    }
}

NTSTATUS
KswordARKBugcheckControlInitialize(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ WDFDEVICE ControlDevice
    )
{
#if !KSWORD_ARK_BUGCHECK_DIAGNOSTICS_ENABLED
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(ControlDevice);
    return STATUS_NOT_SUPPORTED;
#else
    // DriverEntry 只调用一次；拒绝无效对象，避免后续安装动作持有空设备指针。
    if (DriverObject == NULL || ControlDevice == WDF_NO_HANDLE) {
        return STATUS_INVALID_PARAMETER;
    }

    // 设置完成前不发布 ready，后续 IOCTL 不会使用半初始化的 FAST_MUTEX。
    ExInitializeFastMutex(&g_KswordArkBugcheckControlLock);
    g_KswordArkBugcheckControlDriverObject = DriverObject;
    g_KswordArkBugcheckControlDevice = ControlDevice;
    InterlockedExchange(
        &g_KswordArkBugcheckControlLifecycle,
        KSWORD_ARK_BUGCHECK_CONTROL_INACTIVE);
    InterlockedExchange(&g_KswordArkBugcheckControlReady, 1L);
    return STATUS_SUCCESS;
#endif
}

VOID
KswordARKBugcheckControlUninitialize(
    VOID
    )
{
#if !KSWORD_ARK_BUGCHECK_DIAGNOSTICS_ENABLED
    return;
#else
    // 卸载先撤销 ready，新的配置 IOCTL 会被拒绝，再串行等待已开始的安装完成。
    if (InterlockedExchange(&g_KswordArkBugcheckControlReady, 0L) == 0L) {
        return;
    }

    ExAcquireFastMutex(&g_KswordArkBugcheckControlLock);
    if (InterlockedCompareExchange(
            &g_KswordArkBugcheckControlLifecycle,
            KSWORD_ARK_BUGCHECK_CONTROL_INACTIVE,
            KSWORD_ARK_BUGCHECK_CONTROL_INACTIVE) ==
        KSWORD_ARK_BUGCHECK_CONTROL_INSTALLED) {
        // 回调、BGP 资源和预生成矩形必须在驱动映像卸载前统一清理。
        InterlockedExchange(
            &g_KswordArkBugcheckControlLifecycle,
            KSWORD_ARK_BUGCHECK_CONTROL_UNINSTALLING);
        KswordARKBugcheckUninitialize();
    }
    g_KswordArkBugcheckControlDriverObject = NULL;
    g_KswordArkBugcheckControlDevice = WDF_NO_HANDLE;
    InterlockedExchange(
        &g_KswordArkBugcheckControlLifecycle,
        KSWORD_ARK_BUGCHECK_CONTROL_INACTIVE);
    ExReleaseFastMutex(&g_KswordArkBugcheckControlLock);
#endif
}

NTSTATUS
KswordARKBugcheckControlConfigure(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS_REQUEST* Request,
    _Out_ KSWORD_ARK_BUGCHECK_DIAGNOSTICS_RESPONSE* Response
    )
{
#if !KSWORD_ARK_BUGCHECK_DIAGNOSTICS_ENABLED
    UNREFERENCED_PARAMETER(Request);
    KswordARKBugcheckControlFillResponse(
        Response,
        KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_UNSUPPORTED,
        STATUS_NOT_SUPPORTED);
    return STATUS_SUCCESS;
#else
    NTSTATUS installStatus = STATUS_SUCCESS;
    ULONG protocolStatus = KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_INVALID_REQUEST;
    LONG lifecycle = KSWORD_ARK_BUGCHECK_CONTROL_INACTIVE;

    // Handler 已完成 WDF 缓冲区长度检查，这里继续严格校验版本、动作和保留位。
    if (Request == NULL || Response == NULL ||
        Request->size != sizeof(*Request) ||
        Request->version != KSWORD_ARK_BUGCHECK_DIAGNOSTICS_PROTOCOL_VERSION ||
        Request->flags != 0UL ||
        Request->reserved0 != 0UL ||
        Request->reserved1 != 0UL) {
        KswordARKBugcheckControlFillResponse(
            Response,
            KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_INVALID_REQUEST,
            STATUS_INVALID_PARAMETER);
        return STATUS_SUCCESS;
    }

    if (InterlockedCompareExchange(&g_KswordArkBugcheckControlReady, 1L, 1L) == 0L) {
        KswordARKBugcheckControlFillResponse(
            Response,
            KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_UNSUPPORTED,
            STATUS_DEVICE_NOT_READY);
        return STATUS_SUCCESS;
    }

    ExAcquireFastMutex(&g_KswordArkBugcheckControlLock);
    lifecycle = InterlockedCompareExchange(
        &g_KswordArkBugcheckControlLifecycle,
        KSWORD_ARK_BUGCHECK_CONTROL_INACTIVE,
        KSWORD_ARK_BUGCHECK_CONTROL_INACTIVE);
    if (Request->action == KSWORD_ARK_BUGCHECK_DIAGNOSTICS_ACTION_QUERY) {
        protocolStatus = lifecycle == KSWORD_ARK_BUGCHECK_CONTROL_INSTALLED
            ? KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_OK
            : KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_INACTIVE;
        installStatus = STATUS_SUCCESS;
    }
    else if (Request->action == KSWORD_ARK_BUGCHECK_DIAGNOSTICS_ACTION_INSTALL) {
        if (lifecycle == KSWORD_ARK_BUGCHECK_CONTROL_INSTALLED) {
            protocolStatus = KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_OK;
            installStatus = STATUS_SUCCESS;
        }
        else if (lifecycle != KSWORD_ARK_BUGCHECK_CONTROL_INACTIVE) {
            protocolStatus = KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_BUSY;
            installStatus = STATUS_DEVICE_BUSY;
        }
        else if (g_KswordArkBugcheckControlDriverObject == NULL ||
                 g_KswordArkBugcheckControlDevice == WDF_NO_HANDLE) {
            protocolStatus = KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_UNSUPPORTED;
            installStatus = STATUS_DEVICE_NOT_READY;
        }
        else {
            // 私有 BGP 扫描与回调注册只从这个受串行化的、用户明确请求的入口执行。
            InterlockedExchange(
                &g_KswordArkBugcheckControlLifecycle,
                KSWORD_ARK_BUGCHECK_CONTROL_INSTALLING);
            installStatus = KswordARKBugcheckInitialize(
                g_KswordArkBugcheckControlDriverObject,
                g_KswordArkBugcheckControlDevice);
            if (NT_SUCCESS(installStatus)) {
                InterlockedExchange(
                    &g_KswordArkBugcheckControlLifecycle,
                    KSWORD_ARK_BUGCHECK_CONTROL_INSTALLED);
                protocolStatus = KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_OK;
            }
            else {
                // 初始化失败后保留原生 Windows 蓝屏路径，下一次手动安装可以重新尝试。
                InterlockedExchange(
                    &g_KswordArkBugcheckControlLifecycle,
                    KSWORD_ARK_BUGCHECK_CONTROL_INACTIVE);
                protocolStatus = KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_PREPARATION_FAILED;
            }
        }
    }

    KswordARKBugcheckControlFillResponse(
        Response,
        protocolStatus,
        installStatus);
    ExReleaseFastMutex(&g_KswordArkBugcheckControlLock);
    return STATUS_SUCCESS;
#endif
}
