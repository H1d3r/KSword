#include "NtfsRunListDecode.h"

#include <cstring>
#include <limits>
#include <utility>

namespace ks::file
{
    std::int64_t ReadSignedLittleEndian(const std::byte* const ptr, const std::uint8_t byteCount)
    {
        std::uint64_t rawValue = 0;
        if (ptr == nullptr || byteCount == 0 || byteCount > 8)
        {
            return 0;
        }

        for (std::uint8_t i = 0; i < byteCount; ++i)
        {
            rawValue |=
                static_cast<std::uint64_t>(
                    static_cast<std::uint8_t>(ptr[i]))
                << (i * 8);
        }

        // 若最高字节符号位为 1，则需要手动做符号扩展。
        const std::uint64_t signMask =
            std::uint64_t{1} << (byteCount * 8 - 1);
        if (byteCount < 8 && (rawValue & signMask) != 0)
        {
            rawValue |=
                std::numeric_limits<std::uint64_t>::max()
                << (byteCount * 8);
        }
        std::int64_t signedValue = 0;
        static_assert(sizeof(signedValue) == sizeof(rawValue));
        std::memcpy(&signedValue, &rawValue, sizeof(signedValue));
        return signedValue;
    }

    bool ParseNtfsRunList(
        const std::byte* runListPtr,
        const std::byte* const runListEnd,
        std::vector<NtfsDataRun>& dataRunsOut)
    {
        dataRunsOut.clear();
        if (runListPtr == nullptr || runListEnd == nullptr || runListPtr >= runListEnd)
        {
            return false;
        }

        std::int64_t currentLcn = 0;
        while (runListPtr < runListEnd)
        {
            const std::uint8_t headerValue = static_cast<std::uint8_t>(*runListPtr);
            runListPtr += 1;
            if (headerValue == 0)
            {
                return !dataRunsOut.empty();
            }

            const std::uint8_t lengthFieldBytes = (headerValue & 0x0F);
            const std::uint8_t offsetFieldBytes = ((headerValue >> 4) & 0x0F);
            if (lengthFieldBytes == 0
                || lengthFieldBytes > 8
                || offsetFieldBytes > 8
                || runListPtr + lengthFieldBytes + offsetFieldBytes > runListEnd)
            {
                dataRunsOut.clear();
                return false;
            }

            std::uint64_t clusterCountValue = 0;
            for (std::uint8_t i = 0; i < lengthFieldBytes; ++i)
            {
                clusterCountValue |=
                    (static_cast<std::uint64_t>(static_cast<std::uint8_t>(runListPtr[i])) << (i * 8));
            }
            if (clusterCountValue == 0)
            {
                dataRunsOut.clear();
                return false;
            }

            NtfsDataRun runValue{};
            runValue.clusterCount = clusterCountValue;
            if (offsetFieldBytes == 0)
            {
                runValue.isSparse = true;
            }
            else
            {
                const std::int64_t lcnDeltaValue =
                    ReadSignedLittleEndian(runListPtr + lengthFieldBytes, offsetFieldBytes);
                const bool positiveOverflow =
                    lcnDeltaValue > 0 &&
                    currentLcn >
                        std::numeric_limits<std::int64_t>::max() -
                            lcnDeltaValue;
                // 取负号前必须先排除 INT64_MIN：-INT64_MIN 本身就是有符号溢出。
                const bool negativeOrOverflow =
                    lcnDeltaValue ==
                        std::numeric_limits<std::int64_t>::min() ||
                    (lcnDeltaValue < 0 &&
                     currentLcn < -lcnDeltaValue);
                if (positiveOverflow || negativeOrOverflow)
                {
                    dataRunsOut.clear();
                    return false;
                }
                currentLcn += lcnDeltaValue;
                runValue.startLcn = static_cast<std::uint64_t>(currentLcn);
            }

            dataRunsOut.push_back(std::move(runValue));
            runListPtr += lengthFieldBytes + offsetFieldBytes;
        }
        return !dataRunsOut.empty();
    }
}
