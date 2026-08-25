#pragma once

#include "KswordArkProcessIoctl.h"

// Optional R3 -> R0 packets for the on-demand BGP blue-screen diagnostics,
// VMware legacy bitmap resources, and the explicitly-confirmed one-shot guard.
// The diagnostic feature itself is detected and enabled entirely in R0.
#ifndef FILE_WRITE_ACCESS
#define FILE_WRITE_ACCESS 0x0002
#endif

#define KSWORD_ARK_BUGCHECK_BITMAP_PROTOCOL_VERSION 1UL
#define KSWORD_ARK_BUGCHECK_BITMAP_MAGIC 0x4942534BUL /* 'KSBI' */
#define KSWORD_ARK_BUGCHECK_BITMAP_FORMAT_BGRA32 1UL

#define KSWORD_ARK_BUGCHECK_BITMAP_MAX_WIDTH 1024UL
#define KSWORD_ARK_BUGCHECK_BITMAP_MAX_HEIGHT 384UL
#define KSWORD_ARK_BUGCHECK_BITMAP_MAX_BYTES \
    (KSWORD_ARK_BUGCHECK_BITMAP_MAX_WIDTH * \
     KSWORD_ARK_BUGCHECK_BITMAP_MAX_HEIGHT * 4UL)

#define KSWORD_ARK_IOCTL_FUNCTION_SET_BUGCHECK_BITMAP 0x8FAUL

#define IOCTL_KSWORD_ARK_SET_BUGCHECK_BITMAP \
    CTL_CODE( \
        KSWORD_ARK_IOCTL_DEVICE_TYPE, \
        KSWORD_ARK_IOCTL_FUNCTION_SET_BUGCHECK_BITMAP, \
        METHOD_BUFFERED, \
        FILE_WRITE_ACCESS)

typedef struct _KSWORD_ARK_BUGCHECK_BITMAP_HEADER
{
    unsigned long version;
    unsigned long size;
    unsigned long magic;
    unsigned long width;
    unsigned long height;
    unsigned long stride;
    unsigned long format;
    unsigned long brandColorRgb;
    unsigned long dataLength;
    unsigned long flags;
    unsigned long reserved0;
    unsigned long reserved1;
} KSWORD_ARK_BUGCHECK_BITMAP_HEADER;

// Localized verdict cards are rendered by the Qt client with the system UI
// font, then parsed into BGP rectangles by the driver at PASSIVE_LEVEL.  The
// complete English/Chinese set is installed atomically so the crash callback
// never observes a partially updated resource table.
#define KSWORD_ARK_BUGCHECK_VERDICT_PROTOCOL_VERSION 1UL
#define KSWORD_ARK_BUGCHECK_VERDICT_MAGIC 0x5256534BUL /* 'KSVR' */
#define KSWORD_ARK_BUGCHECK_VERDICT_FORMAT_BGRA32 1UL

#define KSWORD_ARK_BUGCHECK_VERDICT_LANGUAGE_ENGLISH 0UL
#define KSWORD_ARK_BUGCHECK_VERDICT_LANGUAGE_CHINESE 1UL
#define KSWORD_ARK_BUGCHECK_VERDICT_LANGUAGE_COUNT 2UL

#define KSWORD_ARK_BUGCHECK_VERDICT_CLASS_UNKNOWN 0UL
#define KSWORD_ARK_BUGCHECK_VERDICT_CLASS_OURS 1UL
#define KSWORD_ARK_BUGCHECK_VERDICT_CLASS_MICROSOFT 2UL
#define KSWORD_ARK_BUGCHECK_VERDICT_CLASS_THIRD_PARTY 3UL
#define KSWORD_ARK_BUGCHECK_VERDICT_CLASS_COUNT 4UL

#define KSWORD_ARK_BUGCHECK_VERDICT_RESOURCE_COUNT \
    (KSWORD_ARK_BUGCHECK_VERDICT_LANGUAGE_COUNT * \
     KSWORD_ARK_BUGCHECK_VERDICT_CLASS_COUNT)
#define KSWORD_ARK_BUGCHECK_VERDICT_MAX_WIDTH 320UL
#define KSWORD_ARK_BUGCHECK_VERDICT_MAX_HEIGHT 128UL
#define KSWORD_ARK_BUGCHECK_VERDICT_MAX_DATA_BYTES \
    (KSWORD_ARK_BUGCHECK_VERDICT_RESOURCE_COUNT * \
     KSWORD_ARK_BUGCHECK_VERDICT_MAX_WIDTH * \
     KSWORD_ARK_BUGCHECK_VERDICT_MAX_HEIGHT * 4UL)

#define KSWORD_ARK_IOCTL_FUNCTION_SET_BUGCHECK_VERDICT_RESOURCES 0x8FCUL

#define IOCTL_KSWORD_ARK_SET_BUGCHECK_VERDICT_RESOURCES \
    CTL_CODE( \
        KSWORD_ARK_IOCTL_DEVICE_TYPE, \
        KSWORD_ARK_IOCTL_FUNCTION_SET_BUGCHECK_VERDICT_RESOURCES, \
        METHOD_BUFFERED, \
        FILE_WRITE_ACCESS)

typedef struct _KSWORD_ARK_BUGCHECK_VERDICT_RESOURCE_HEADER
{
    unsigned long version;
    unsigned long size;
    unsigned long magic;
    unsigned long resourceCount;
    unsigned long entriesOffset;
    unsigned long totalSize;
    unsigned long flags;
    unsigned long reserved;
} KSWORD_ARK_BUGCHECK_VERDICT_RESOURCE_HEADER;

typedef struct _KSWORD_ARK_BUGCHECK_VERDICT_RESOURCE_ENTRY
{
    unsigned long language;
    unsigned long classification;
    unsigned long width;
    unsigned long height;
    unsigned long stride;
    unsigned long format;
    unsigned long dataOffset;
    unsigned long dataLength;
} KSWORD_ARK_BUGCHECK_VERDICT_RESOURCE_ENTRY;

// 蓝屏诊断的 BGP 解析、页面预生成与转储回调默认不在 DriverEntry 执行。
// R3 仅在用户已配置自动安装，或本次明确点击安装后通过此 IOCTL 请求 R0 安装。
#define KSWORD_ARK_BUGCHECK_DIAGNOSTICS_PROTOCOL_VERSION 1UL
#define KSWORD_ARK_IOCTL_FUNCTION_CONFIGURE_BUGCHECK_DIAGNOSTICS 0x8FDUL

#define IOCTL_KSWORD_ARK_CONFIGURE_BUGCHECK_DIAGNOSTICS \
    CTL_CODE( \
        KSWORD_ARK_IOCTL_DEVICE_TYPE, \
        KSWORD_ARK_IOCTL_FUNCTION_CONFIGURE_BUGCHECK_DIAGNOSTICS, \
        METHOD_BUFFERED, \
        FILE_WRITE_ACCESS)

#define KSWORD_ARK_BUGCHECK_DIAGNOSTICS_ACTION_QUERY   0UL
#define KSWORD_ARK_BUGCHECK_DIAGNOSTICS_ACTION_INSTALL 1UL

#define KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_OK                 0UL
#define KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_INACTIVE           1UL
#define KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_BUSY               2UL
#define KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_UNSUPPORTED        3UL
#define KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_PREPARATION_FAILED 4UL
#define KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATUS_INVALID_REQUEST    5UL

#define KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATE_INSTALLED          0x00000001UL
#define KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATE_CALLBACKS_READY    0x00000002UL
#define KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATE_BGP_BACKEND_READY  0x00000004UL
#define KSWORD_ARK_BUGCHECK_DIAGNOSTICS_STATE_PANEL_READY        0x00000008UL

// 固定长度请求仅区分查询和本次驱动生命周期内的安装，不提供常驻卸载动作。
typedef struct _KSWORD_ARK_BUGCHECK_DIAGNOSTICS_REQUEST
{
    unsigned long size;
    unsigned long version;
    unsigned long action;
    unsigned long flags;
    unsigned long reserved0;
    unsigned long reserved1;
} KSWORD_ARK_BUGCHECK_DIAGNOSTICS_REQUEST;

// 响应保留回调与 BGP 准备摘要，R3 可展示失败阶段但不重新解释私有内核地址。
typedef struct _KSWORD_ARK_BUGCHECK_DIAGNOSTICS_RESPONSE
{
    unsigned long size;
    unsigned long version;
    unsigned long status;
    unsigned long stateFlags;
    long lastStatus;
    unsigned long callbackMask;
    unsigned long bgpState;
    unsigned long bgpPreparationStage;
    long bgpPreparationStatus;
    long panelStatus;
    unsigned long reserved0;
    unsigned long reserved1;
} KSWORD_ARK_BUGCHECK_DIAGNOSTICS_RESPONSE;

// The delay guard is deliberately separate from the VMware display panel.
// On systems where HVCI protects kernel code it uses a supported BugCheck
// callback as a delay-only backend. Otherwise it can intercept the exported
// KeBugCheckEx entry for one bugcheck and restore the entry before forwarding
// the call or attempting an unsupported return. Neither backend is a
// crash-recovery API.
#define KSWORD_ARK_BUGCHECK_GUARD_PROTOCOL_VERSION 4UL
#define KSWORD_ARK_IOCTL_FUNCTION_CONFIGURE_BUGCHECK_GUARD 0x8FBUL

#define IOCTL_KSWORD_ARK_CONFIGURE_BUGCHECK_GUARD \
    CTL_CODE( \
        KSWORD_ARK_IOCTL_DEVICE_TYPE, \
        KSWORD_ARK_IOCTL_FUNCTION_CONFIGURE_BUGCHECK_GUARD, \
        METHOD_BUFFERED, \
        FILE_WRITE_ACCESS)

#define KSWORD_ARK_BUGCHECK_GUARD_ACTION_QUERY   0UL
#define KSWORD_ARK_BUGCHECK_GUARD_ACTION_ENABLE  1UL
#define KSWORD_ARK_BUGCHECK_GUARD_ACTION_DISABLE 2UL

#define KSWORD_ARK_BUGCHECK_GUARD_FLAG_UI_CONFIRMED     0x00000001UL
#define KSWORD_ARK_BUGCHECK_GUARD_FLAG_TRY_IGNORE_ERROR 0x00000002UL
#define KSWORD_ARK_BUGCHECK_GUARD_CONFIRMATION_TOKEN 0x4452474BUL /* 'KGRD' */
#define KSWORD_ARK_BUGCHECK_GUARD_MIN_DELAY_SECONDS 1UL
// delaySeconds accepts any nonzero value representable by its ULONG field.

#define KSWORD_ARK_BUGCHECK_GUARD_STATUS_OK                  0UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATUS_INACTIVE            1UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATUS_ACTIVE              2UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATUS_CONFIRMATION_NEEDED 3UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATUS_UNSUPPORTED          4UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATUS_CONFLICT            5UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATUS_PATCH_FAILED        6UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATUS_BUSY                7UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATUS_INVALID_REQUEST     8UL

#define KSWORD_ARK_BUGCHECK_GUARD_STATE_TARGET_RESOLVED  0x00000001UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATE_ACTIVE           0x00000002UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATE_PATCH_INSTALLED  0x00000004UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATE_FIRED            0x00000008UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATE_PREEXISTING_HOOK 0x00000010UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATE_TRY_IGNORE_ERROR 0x00000020UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATE_ERROR_IGNORED    0x00000040UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATE_HOOK_EXECUTING   0x00000080UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATE_HVCI_ENABLED      0x00000100UL
#define KSWORD_ARK_BUGCHECK_GUARD_STATE_CALLBACK_REGISTERED 0x00000200UL

// 补丁快照属于固定线协议，Win32 R3 也必须与 x64 R0 保持相同的响应布局。
#define KSWORD_ARK_BUGCHECK_GUARD_HOOK_BYTES 12UL

typedef struct _KSWORD_ARK_BUGCHECK_GUARD_REQUEST
{
    unsigned long size;
    unsigned long version;
    unsigned long action;
    unsigned long flags;
    unsigned long delaySeconds;
    unsigned long confirmationToken;
    unsigned long reserved0;
    unsigned long reserved1;
} KSWORD_ARK_BUGCHECK_GUARD_REQUEST;

typedef struct _KSWORD_ARK_BUGCHECK_GUARD_RESPONSE
{
    unsigned long size;
    unsigned long version;
    unsigned long status;
    unsigned long stateFlags;
    unsigned long delaySeconds;
    long lastStatus;
    unsigned long reserved0;
    unsigned long reserved1;
    unsigned long long targetAddress;
    unsigned char originalBytes[KSWORD_ARK_BUGCHECK_GUARD_HOOK_BYTES];
    unsigned char hookBytes[KSWORD_ARK_BUGCHECK_GUARD_HOOK_BYTES];
} KSWORD_ARK_BUGCHECK_GUARD_RESPONSE;

