#include "DriverMemoryClient.h"

#include "../../../Ksword5.1/Ksword5.1/ArkDriverClient/ArkDriverClient.h"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>

namespace Ksword::Features::Memory {
namespace {

// MemoryReadStatusText maps the shared driver read status into stable UI text.
// Input is KSWORD_ARK_MEMORY_READ_STATUS_* from shared/driver; output is a short
// label that supplements ArkDriverClient's transport diagnostic message.
const wchar_t* MemoryReadStatusText(const std::uint32_t status) {
    switch (status) {
    case KSWORD_ARK_MEMORY_READ_STATUS_OK: return L"OK";
    case KSWORD_ARK_MEMORY_READ_STATUS_PARTIAL_COPY: return L"Partial copy";
    case KSWORD_ARK_MEMORY_READ_STATUS_PROCESS_LOOKUP_FAILED: return L"Process lookup failed";
    case KSWORD_ARK_MEMORY_READ_STATUS_COPY_FAILED: return L"Copy failed";
    case KSWORD_ARK_MEMORY_READ_STATUS_RANGE_REJECTED: return L"Range rejected";
    case KSWORD_ARK_MEMORY_READ_STATUS_BUFFER_TOO_SMALL: return L"Buffer too small";
    case KSWORD_ARK_MEMORY_READ_STATUS_ZERO_FILLED: return L"Zero-filled unreadable";
    case KSWORD_ARK_MEMORY_READ_STATUS_UNAVAILABLE:
    default:
        return L"Unavailable";
    }
}

// MemoryWriteStatusText maps the shared driver write status into stable UI
// text. Input is KSWORD_ARK_MEMORY_WRITE_STATUS_*; output is a short label.
const wchar_t* MemoryWriteStatusText(const std::uint32_t status) {
    switch (status) {
    case KSWORD_ARK_MEMORY_WRITE_STATUS_OK: return L"OK";
    case KSWORD_ARK_MEMORY_WRITE_STATUS_PARTIAL_COPY: return L"Partial copy";
    case KSWORD_ARK_MEMORY_WRITE_STATUS_PROCESS_LOOKUP_FAILED: return L"Process lookup failed";
    case KSWORD_ARK_MEMORY_WRITE_STATUS_COPY_FAILED: return L"Copy failed";
    case KSWORD_ARK_MEMORY_WRITE_STATUS_RANGE_REJECTED: return L"Range rejected";
    case KSWORD_ARK_MEMORY_WRITE_STATUS_BUFFER_TOO_SMALL: return L"Buffer too small";
    case KSWORD_ARK_MEMORY_WRITE_STATUS_ACCESS_DENIED: return L"Access denied";
    case KSWORD_ARK_MEMORY_WRITE_STATUS_FORCE_REQUIRED: return L"Force required";
    case KSWORD_ARK_MEMORY_WRITE_STATUS_UNAVAILABLE:
    default:
        return L"Unavailable";
    }
}

// NarrowToWide converts ArkDriverClient's narrow diagnostic messages into the
// Win32-light UI's UTF-16 status text. Input is ASCII/UTF-8-like diagnostic
// text; processing widens byte-for-byte because current client messages are
// English diagnostics; output is displayable UTF-16 text.
std::wstring NarrowToWide(const std::string& text) {
    std::wstring wide;
    wide.reserve(text.size());
    for (const unsigned char ch : text) {
        wide.push_back(static_cast<wchar_t>(ch));
    }
    return wide;
}

// FormatReadStatus creates the final read status line. Inputs are the original
// request and ArkDriverClient result; processing combines Win32, NT, protocol
// and byte-count fields; output is shown in the memory page status box.
std::wstring FormatReadStatus(
    const DriverMemoryReadRequest& request,
    const ksword::ark::VirtualMemoryReadResult& driverResult) {
    std::wostringstream stream;
    stream << L"R0 read "
           << (driverResult.io.ok ? L"transport OK" : L"transport failed")
           << L"; protocol=" << MemoryReadStatusText(driverResult.readStatus)
           << L"; pid=" << request.processId
           << L"; address=0x" << std::hex << std::uppercase << request.address
           << std::dec
           << L"; requested=" << request.length
           << L"; bytesRead=" << driverResult.bytesRead
           << L"; win32=" << driverResult.io.win32Error
           << L"; nt=0x" << std::hex << static_cast<unsigned long>(driverResult.copyStatus)
           << L"; fields=0x" << driverResult.fieldFlags
           << L"; " << NarrowToWide(driverResult.io.message);
    return stream.str();
}

// FormatWriteStatus creates the final write status line. Inputs are the request
// and ArkDriverClient result; processing combines transport/protocol/byte
// counts; output is shown in the memory page status box.
std::wstring FormatWriteStatus(
    const DriverMemoryWriteRequest& request,
    const ksword::ark::VirtualMemoryWriteResult& driverResult) {
    std::wostringstream stream;
    stream << L"R0 write "
           << (driverResult.io.ok ? L"transport OK" : L"transport failed")
           << L"; protocol=" << MemoryWriteStatusText(driverResult.writeStatus)
           << L"; pid=" << request.processId
           << L"; address=0x" << std::hex << std::uppercase << request.address
           << std::dec
           << L"; requested=" << request.bytes.size()
           << L"; bytesWritten=" << driverResult.bytesWritten
           << L"; win32=" << driverResult.io.win32Error
           << L"; nt=0x" << std::hex << static_cast<unsigned long>(driverResult.copyStatus)
           << L"; fields=0x" << driverResult.fieldFlags
           << L"; " << NarrowToWide(driverResult.io.message);
    return stream.str();
}

std::wstring FormatAddress(const std::uint64_t value) {
    std::wostringstream stream;
    stream << L"0x" << std::hex << std::uppercase << value;
    return stream.str();
}

bool IsExactRead(const DriverMemoryReadResult& result, const std::vector<std::uint8_t>& expected) {
    return result.success && result.protocolStatus == KSWORD_ARK_MEMORY_READ_STATUS_OK &&
        result.bytes.size() == expected.size() && std::equal(result.bytes.cbegin(), result.bytes.cend(), expected.cbegin());
}

bool IsExactWrite(const DriverMemoryWriteResult& result, const std::size_t expectedBytes) {
    return result.success && result.protocolStatus == KSWORD_ARK_MEMORY_WRITE_STATUS_OK &&
        result.bytesWritten == expectedBytes;
}

void SetWritebackFailure(DriverMemoryWritebackResult& result,
    const std::uint64_t address,
    const std::wstring& message) {
    result.success = false;
    result.failedAddress = address;
    result.statusText = message;
}

// MakeReadValidationError returns a failed local result before any IOCTL is
// sent. Input is the user request and message; output is a consistent UI result.
DriverMemoryReadResult MakeReadValidationError(const DriverMemoryReadRequest& request, const std::wstring& message) {
    DriverMemoryReadResult result;
    result.success = false;
    result.win32Error = ERROR_INVALID_PARAMETER;
    result.statusText = message + L" Requested " + std::to_wstring(request.length) + L" byte(s).";
    return result;
}

// MakeWriteValidationError returns a failed local result before any IOCTL is
// sent. Input is the user request and message; output is a consistent UI result.
DriverMemoryWriteResult MakeWriteValidationError(const DriverMemoryWriteRequest& request, const std::wstring& message) {
    DriverMemoryWriteResult result;
    result.success = false;
    result.win32Error = ERROR_INVALID_PARAMETER;
    result.bytesWritten = 0;
    result.statusText = message + L" Payload " + std::to_wstring(request.bytes.size()) + L" byte(s).";
    return result;
}

} // namespace

DriverMemoryClient::DriverMemoryClient() = default;

DriverMemoryClient::~DriverMemoryClient() = default;

DriverMemoryReadResult DriverMemoryClient::ReadMemory(const DriverMemoryReadRequest& request) {
    if (request.processId == 0 || request.length == 0) {
        return MakeReadValidationError(request, L"PID and length must be non-zero.");
    }
    if (request.length > KSWORD_ARK_MEMORY_READ_MAX_BYTES) {
        return MakeReadValidationError(request, L"Read length exceeds shared driver limit.");
    }

    const ksword::ark::DriverClient client;
    const ksword::ark::VirtualMemoryReadResult driverResult = client.readVirtualMemory(
        static_cast<std::uint32_t>(request.processId),
        request.address,
        static_cast<std::uint32_t>(request.length),
        0);

    DriverMemoryReadResult result;
    result.success = driverResult.io.ok &&
        (driverResult.readStatus == KSWORD_ARK_MEMORY_READ_STATUS_OK ||
            driverResult.readStatus == KSWORD_ARK_MEMORY_READ_STATUS_PARTIAL_COPY) &&
        !driverResult.data.empty();
    result.win32Error = driverResult.io.win32Error;
    result.protocolStatus = driverResult.readStatus;
    result.copyStatus = static_cast<std::uint32_t>(driverResult.copyStatus);
    result.fieldFlags = driverResult.fieldFlags;
    result.bytes = driverResult.data;
    result.statusText = FormatReadStatus(request, driverResult);
    return result;
}

DriverMemoryWriteResult DriverMemoryClient::WriteMemory(const DriverMemoryWriteRequest& request, const bool forceWrite) {
    if (request.processId == 0 || request.bytes.empty()) {
        return MakeWriteValidationError(request, L"PID and payload must be non-zero.");
    }
    if (request.bytes.size() > KSWORD_ARK_MEMORY_WRITE_MAX_BYTES) {
        return MakeWriteValidationError(request, L"Write payload exceeds shared driver limit.");
    }

    const ksword::ark::DriverClient client;
    std::uint32_t writeFlags = KSWORD_ARK_MEMORY_WRITE_FLAG_UI_CONFIRMED;
    if (forceWrite) {
        writeFlags |= KSWORD_ARK_MEMORY_WRITE_FLAG_FORCE;
    }
    const ksword::ark::VirtualMemoryWriteResult driverResult = client.writeVirtualMemory(
        static_cast<std::uint32_t>(request.processId),
        request.address,
        request.bytes,
        writeFlags);

    DriverMemoryWriteResult result;
    result.success = driverResult.io.ok &&
        (driverResult.writeStatus == KSWORD_ARK_MEMORY_WRITE_STATUS_OK ||
            driverResult.writeStatus == KSWORD_ARK_MEMORY_WRITE_STATUS_PARTIAL_COPY) &&
        driverResult.bytesWritten > 0;
    result.win32Error = driverResult.io.win32Error;
    result.protocolStatus = driverResult.writeStatus;
    result.copyStatus = static_cast<std::uint32_t>(driverResult.copyStatus);
    result.fieldFlags = driverResult.fieldFlags;
    result.bytesWritten = driverResult.bytesWritten;
    result.statusText = FormatWriteStatus(request, driverResult);
    return result;
}

DriverMemoryWritebackResult DriverMemoryClient::ApplyWritePlan(const MemoryWritePlan& plan, const bool forceWrite) {
    DriverMemoryWritebackResult result{};
    result.totalBlocks = plan.blocks.size();
    std::wstring validationError;
    if (!ValidateMemoryWritePlan(plan, validationError)) {
        SetWritebackFailure(result, plan.baseAddress, L"差异写回计划无效：" + validationError);
        return result;
    }
    if (plan.desiredSnapshotBytes.size() > KSWORD_ARK_MEMORY_READ_MAX_BYTES) {
        SetWritebackFailure(result, plan.baseAddress, L"差异写回快照超过共享读取上限。");
        return result;
    }
    if (plan.blocks.empty()) {
        result.success = true;
        result.statusText = L"差异计划没有变化字节，未发送写入请求。";
        return result;
    }

    for (const MemoryWriteBlock& block : plan.blocks) {
        if (block.desiredAfter.size() > KSWORD_ARK_MEMORY_WRITE_MAX_BYTES) {
            SetWritebackFailure(result, block.address, L"差异块超过共享写入上限。");
            return result;
        }
        const DriverMemoryReadRequest preflightRequest{
            static_cast<DWORD>(plan.processId), block.address, block.expectedBefore.size() };
        const DriverMemoryReadResult preflightResult = ReadMemory(preflightRequest);
        if (!IsExactRead(preflightResult, block.expectedBefore)) {
            SetWritebackFailure(result, block.address,
                L"差异写回已停止：写前读取与原始快照不一致，需重新读取并预览。\r\n" + preflightResult.statusText);
            return result;
        }

        DriverMemoryWriteRequest writeRequest{};
        writeRequest.processId = static_cast<DWORD>(plan.processId);
        writeRequest.address = block.address;
        writeRequest.bytes = block.desiredAfter;
        const DriverMemoryWriteResult writeResult = WriteMemory(writeRequest, forceWrite);
        result.requestedBytes += block.desiredAfter.size();
        result.bytesWritten += writeResult.bytesWritten;
        if (!IsExactWrite(writeResult, block.desiredAfter.size())) {
            if (!forceWrite && writeResult.protocolStatus == KSWORD_ARK_MEMORY_WRITE_STATUS_FORCE_REQUIRED &&
                result.verifiedBlocks == 0U) {
                result.forceRequired = true;
                SetWritebackFailure(result, block.address,
                    L"驱动要求 FORCE 才能写入此快照；尚未写入任何差异块。\r\n" + writeResult.statusText);
            } else {
                SetWritebackFailure(result, block.address,
                    L"差异写回已停止：写入未完整成功，未继续后续块。\r\n" + writeResult.statusText);
            }
            return result;
        }

        const DriverMemoryReadRequest verificationRequest{
            static_cast<DWORD>(plan.processId), block.address, block.desiredAfter.size() };
        const DriverMemoryReadResult verificationResult = ReadMemory(verificationRequest);
        if (!IsExactRead(verificationResult, block.desiredAfter)) {
            SetWritebackFailure(result, block.address,
                L"差异写回已停止：写后读取未精确匹配目标字节，未继续后续块。\r\n" + verificationResult.statusText);
            return result;
        }
        ++result.verifiedBlocks;
    }

    const DriverMemoryReadRequest finalReadRequest{
        static_cast<DWORD>(plan.processId), plan.baseAddress, plan.desiredSnapshotBytes.size() };
    result.finalReadResult = ReadMemory(finalReadRequest);
    if (!IsExactRead(result.finalReadResult, plan.desiredSnapshotBytes)) {
        SetWritebackFailure(result, plan.baseAddress,
            L"差异块均已验证，但完整快照复读未精确匹配；未更新快照基线。\r\n" + result.finalReadResult.statusText);
        return result;
    }

    result.success = true;
    result.statusText = L"差异写回完成：已验证 " + std::to_wstring(result.verifiedBlocks) + L" 个块、" +
        std::to_wstring(result.bytesWritten) + L" 字节；完整快照复读一致。目标=" + FormatAddress(plan.baseAddress) + L"。";
    return result;
}

} // namespace Ksword::Features::Memory
