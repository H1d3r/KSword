#pragma once

#include "DriverMemoryModel.h"
#include "MemoryWritePlan.h"

namespace Ksword::Features::Memory {

// DriverMemoryWritebackResult describes a staged, verified plan application.
// success means every block passed exact before/after reads and the full desired
// snapshot was read back exactly. forceRequired is a non-mutating response that
// the UI may present for a separate explicit confirmation.
struct DriverMemoryWritebackResult {
    bool success = false;
    bool forceRequired = false;
    std::size_t totalBlocks = 0;
    std::size_t verifiedBlocks = 0;
    std::size_t requestedBytes = 0;
    std::size_t bytesWritten = 0;
    std::uint64_t failedAddress = 0;
    std::wstring statusText;
    DriverMemoryReadResult finalReadResult;
};

// DriverMemoryClient is the module-local facade for driver memory operations.
// The view depends only on this class, so UI code never performs raw driver I/O
// directly. Requests are forwarded to the shared ArkDriverClient implementation
// used by the full KswordARK project, which keeps IOCTL structure ownership in
// shared/driver and avoids duplicate protocol definitions.
class DriverMemoryClient final {
public:
    DriverMemoryClient();
    ~DriverMemoryClient();

    DriverMemoryClient(const DriverMemoryClient&) = delete;
    DriverMemoryClient& operator=(const DriverMemoryClient&) = delete;

    // ReadMemory sends a validated read request to the driver facade. Input is a
    // request produced by DriverMemoryModel; processing calls ArkDriverClient
    // readVirtualMemory without zero-fill fallback so unreadable ranges are not
    // misreported as successful all-zero buffers; output describes success,
    // status and returned bytes.
    DriverMemoryReadResult ReadMemory(const DriverMemoryReadRequest& request);

    // WriteMemory sends a validated write request to the driver facade. The
    // default uses only UI_CONFIRMED for compatibility with older drivers;
    // forceWrite is available solely after a separate explicit UI confirmation.
    DriverMemoryWriteResult WriteMemory(const DriverMemoryWriteRequest& request, bool forceWrite = false);

    // ApplyWritePlan freezes the supplied snapshot target, exact-preflights each
    // changed block, writes it, verifies it, then exactly re-reads the whole
    // desired snapshot. It never retries with FORCE unless forceWrite is true.
    DriverMemoryWritebackResult ApplyWritePlan(const MemoryWritePlan& plan, bool forceWrite);
};

} // namespace Ksword::Features::Memory
