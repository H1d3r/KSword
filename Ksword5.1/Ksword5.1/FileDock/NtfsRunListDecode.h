#pragma once

// ============================================================
// NtfsRunListDecode.h
// 作用：
// 1) 解码 NTFS 非常驻属性的 runlist，得到每一段的 LCN 范围或稀疏标记；
// 2) 把这段纯字节解析从 ManualFileSystemParser 中独立出来，使其可离线测试。
//
// 为什么单独成文件：runlist 来自磁盘，是不可信输入 —— 长度字段、有符号的
// LCN 增量、稀疏段和终止符都可以被构造成越界或溢出。原实现藏在
// ManualFileSystemParser.cpp 的匿名命名空间里，只能通过"真实卷 + 真实删除文件"
// 间接触发，边界分支基本无法覆盖。剥离之后可以直接喂构造好的字节数组。
//
// 本文件只依赖标准库，不引入 Qt 与 Windows 头，测试工程可以直接编译它。
// ============================================================

#include <cstdint>
#include <cstddef>
#include <vector>

namespace ks::file
{
    // NtfsDataRun 作用：runlist 中的一段。
    struct NtfsDataRun
    {
        std::uint64_t startLcn = 0;            // 数据段起始 LCN，稀疏段时为 0。
        std::uint64_t clusterCount = 0;        // 当前数据段占用的簇数量。
        bool isSparse = false;                 // 是否稀疏段，true 表示该段逻辑上为 0 填充。
    };

    // ReadSignedLittleEndian 作用：读取长度 1~8 字节的有符号小端整数。
    // 输入：起始指针与字节数；指针为空或字节数不在 1~8 之间时返回 0。
    // 处理：按小端拼装后，若最高字节符号位为 1 则手动做符号扩展 ——
    //       runlist 的 LCN 增量是相对量，向前跳时为负数。
    std::int64_t ReadSignedLittleEndian(const std::byte* ptr, std::uint8_t byteCount);

    // ParseNtfsRunList 作用：解析非常驻属性的 runlist。
    // 输入：runlist 起止指针（左闭右开）。
    // 输出：dataRunsOut 依次收到每一段；失败时被清空。
    // 返回：解析出至少一段且未越界为 true。以下情形一律判失败并清空输出：
    //       指针无效或区间为空；长度字段为 0 或超过 8 字节；偏移字段超过 8 字节；
    //       字段跨过 runListEnd；簇数为 0；LCN 增量导致溢出或使 LCN 变负。
    //       首字节即终止符（空 runlist）同样返回 false —— 非常驻属性至少应有一段。
    bool ParseNtfsRunList(
        const std::byte* runListPtr,
        const std::byte* runListEnd,
        std::vector<NtfsDataRun>& dataRunsOut);
}
