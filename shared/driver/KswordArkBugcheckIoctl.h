#pragma once

#include "KswordArkProcessIoctl.h"

// Optional R3 -> R0 packets for the VMware-only bugcheck panel and the
// explicitly-confirmed one-shot KeBugCheckEx delay guard.
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

// The application rasterizes the current Windows fixed-width UI font into
// two immutable 8-bit coverage atlases. The driver never opens a font file or
// calls a user-mode text API from the bugcheck path.
#define KSWORD_ARK_BUGCHECK_FONT_PROTOCOL_VERSION 1UL
#define KSWORD_ARK_BUGCHECK_FONT_MAGIC 0x4642534BUL /* 'KSBF' */
#define KSWORD_ARK_BUGCHECK_FONT_FORMAT_A8 1UL

#define KSWORD_ARK_BUGCHECK_FONT_ASCII_FIRST 32UL
#define KSWORD_ARK_BUGCHECK_FONT_ASCII_LAST 126UL
#define KSWORD_ARK_BUGCHECK_FONT_GLYPH_COUNT \
    (KSWORD_ARK_BUGCHECK_FONT_ASCII_LAST - \
     KSWORD_ARK_BUGCHECK_FONT_ASCII_FIRST + 1UL)

#define KSWORD_ARK_BUGCHECK_FONT_BODY_WIDTH 10UL
#define KSWORD_ARK_BUGCHECK_FONT_BODY_HEIGHT 17UL
#define KSWORD_ARK_BUGCHECK_FONT_BODY_ADVANCE 11UL
#define KSWORD_ARK_BUGCHECK_FONT_HERO_WIDTH 20UL
#define KSWORD_ARK_BUGCHECK_FONT_HERO_HEIGHT 34UL
#define KSWORD_ARK_BUGCHECK_FONT_HERO_ADVANCE 21UL

#define KSWORD_ARK_BUGCHECK_FONT_BODY_BYTES \
    (KSWORD_ARK_BUGCHECK_FONT_GLYPH_COUNT * \
     KSWORD_ARK_BUGCHECK_FONT_BODY_WIDTH * \
     KSWORD_ARK_BUGCHECK_FONT_BODY_HEIGHT)
#define KSWORD_ARK_BUGCHECK_FONT_HERO_BYTES \
    (KSWORD_ARK_BUGCHECK_FONT_GLYPH_COUNT * \
     KSWORD_ARK_BUGCHECK_FONT_HERO_WIDTH * \
     KSWORD_ARK_BUGCHECK_FONT_HERO_HEIGHT)
#define KSWORD_ARK_BUGCHECK_FONT_MAX_BYTES \
    (KSWORD_ARK_BUGCHECK_FONT_BODY_BYTES + \
     KSWORD_ARK_BUGCHECK_FONT_HERO_BYTES)

#define KSWORD_ARK_IOCTL_FUNCTION_SET_BUGCHECK_FONT 0x8FCUL

#define IOCTL_KSWORD_ARK_SET_BUGCHECK_FONT \
    CTL_CODE( \
        KSWORD_ARK_IOCTL_DEVICE_TYPE, \
        KSWORD_ARK_IOCTL_FUNCTION_SET_BUGCHECK_FONT, \
        METHOD_BUFFERED, \
        FILE_WRITE_ACCESS)

typedef struct _KSWORD_ARK_BUGCHECK_FONT_HEADER
{
    unsigned long version;
    unsigned long size;
    unsigned long magic;
    unsigned long format;
    unsigned long firstCharacter;
    unsigned long glyphCount;
    unsigned long bodyWidth;
    unsigned long bodyHeight;
    unsigned long bodyAdvance;
    unsigned long bodyDataLength;
    unsigned long heroWidth;
    unsigned long heroHeight;
    unsigned long heroAdvance;
    unsigned long heroDataLength;
    unsigned long dataLength;
    unsigned long flags;
    unsigned long reserved0;
    unsigned long reserved1;
} KSWORD_ARK_BUGCHECK_FONT_HEADER;

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

