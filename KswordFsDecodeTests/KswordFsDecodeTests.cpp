// ============================================================
// KswordFsDecodeTests.cpp
// 作用：对文件系统结构解码做离线单元测试。
//
// 这些测试只喂字节数组，不打开卷、不加载驱动、不需要管理员权限，
// 因此可以覆盖真实磁盘上极难构造的分支：越界的长度字段、导致 LCN 变负的
// 增量、INT64_MIN 增量、稀疏段与终止符的组合。runlist 来自磁盘，属于
// 不可信输入，这些正是必须钉死的地方。
// ============================================================

#include "../Ksword5.1/Ksword5.1/FileDock/NtfsRunListDecode.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <vector>

namespace {

int failures = 0;

void Expect(const bool condition, const char* const label)
{
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << label << '\n';
    }
}

// Bytes 作用：把可读的十六进制序列变成 runlist 字节缓冲。
std::vector<std::byte> Bytes(const std::initializer_list<int> values)
{
    std::vector<std::byte> buffer;
    buffer.reserve(values.size());
    for (const int value : values) {
        buffer.push_back(static_cast<std::byte>(static_cast<unsigned char>(value)));
    }
    return buffer;
}

// Parse 作用：按左闭右开区间调用被测函数，省去每个用例重复取首尾指针。
bool Parse(const std::vector<std::byte>& buffer, std::vector<ks::file::NtfsDataRun>& runsOut)
{
    return ks::file::ParseNtfsRunList(buffer.data(), buffer.data() + buffer.size(), runsOut);
}

void TestReadSignedLittleEndian()
{
    using ks::file::ReadSignedLittleEndian;

    const std::vector<std::byte> positive = Bytes({0x7F});
    Expect(ReadSignedLittleEndian(positive.data(), 1) == 127, "read: 1-byte positive");

    // 最高位为 1 必须扩展成负数，否则向前跳的 run 会被算成一个巨大的正 LCN。
    const std::vector<std::byte> negative = Bytes({0x80});
    Expect(ReadSignedLittleEndian(negative.data(), 1) == -128, "read: 1-byte sign extension");

    const std::vector<std::byte> minusOne = Bytes({0xFF, 0xFF});
    Expect(ReadSignedLittleEndian(minusOne.data(), 2) == -1, "read: 2-byte -1");

    const std::vector<std::byte> minShort = Bytes({0x00, 0x80});
    Expect(ReadSignedLittleEndian(minShort.data(), 2) == -32768, "read: 2-byte minimum");

    const std::vector<std::byte> threeByte = Bytes({0x00, 0x00, 0xFF});
    Expect(ReadSignedLittleEndian(threeByte.data(), 3) == -65536, "read: 3-byte sign extension");

    // 满 8 字节时没有可扩展的高位，原样按补码解释。
    const std::vector<std::byte> fullWidth = Bytes({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
    Expect(ReadSignedLittleEndian(fullWidth.data(), 8) == -1, "read: 8-byte no extension");

    const std::vector<std::byte> ignored = Bytes({0x11, 0x22});
    Expect(ReadSignedLittleEndian(ignored.data(), 0) == 0, "read: zero width rejected");
    Expect(ReadSignedLittleEndian(ignored.data(), 9) == 0, "read: width over 8 rejected");
    Expect(ReadSignedLittleEndian(nullptr, 4) == 0, "read: null pointer rejected");
}

void TestRunListHappyPath()
{
    std::vector<ks::file::NtfsDataRun> runs;

    // 0x21 = 长度字段 1 字节、偏移字段 2 字节；随后 0x00 是终止符。
    Expect(Parse(Bytes({0x21, 0x08, 0x34, 0x12, 0x00}), runs), "single: parsed");
    Expect(runs.size() == 1, "single: one run");
    if (runs.size() == 1) {
        Expect(runs[0].clusterCount == 8, "single: cluster count");
        Expect(runs[0].startLcn == 0x1234, "single: start lcn");
        Expect(!runs[0].isSparse, "single: not sparse");
    }

    // 第二段的偏移是相对上一段起点的增量，不是绝对 LCN。
    Expect(Parse(Bytes({0x21, 0x08, 0x34, 0x12, 0x21, 0x04, 0x10, 0x00, 0x00}), runs),
           "chained: parsed");
    Expect(runs.size() == 2, "chained: two runs");
    if (runs.size() == 2) {
        Expect(runs[0].startLcn == 0x1234, "chained: first lcn");
        Expect(runs[1].startLcn == 0x1234 + 0x10, "chained: second lcn accumulates");
        Expect(runs[1].clusterCount == 4, "chained: second cluster count");
    }

    // 负增量表示向卷起始方向回跳，是碎片文件的常态。
    Expect(Parse(Bytes({0x21, 0x08, 0x34, 0x12, 0x21, 0x04, 0xF0, 0xFF, 0x00}), runs),
           "backward: parsed");
    Expect(runs.size() == 2, "backward: two runs");
    if (runs.size() == 2) {
        Expect(runs[1].startLcn == 0x1234 - 0x10, "backward: second lcn moves back");
    }

    // 没有终止符、字节正好用尽也算解析完成。
    Expect(Parse(Bytes({0x21, 0x08, 0x34, 0x12}), runs), "unterminated: parsed");
    Expect(runs.size() == 1, "unterminated: one run");
}

void TestRunListSparse()
{
    std::vector<ks::file::NtfsDataRun> runs;

    // 0x01 = 长度字段 1 字节、偏移字段 0 字节，即稀疏段。
    Expect(Parse(Bytes({0x01, 0x05, 0x00}), runs), "sparse: parsed");
    Expect(runs.size() == 1, "sparse: one run");
    if (runs.size() == 1) {
        Expect(runs[0].isSparse, "sparse: flagged");
        Expect(runs[0].clusterCount == 5, "sparse: cluster count");
        Expect(runs[0].startLcn == 0, "sparse: lcn stays zero");
    }

    // 稀疏段不携带偏移，因此不能推进 LCN 基准：后一段仍以 0 为基准算出 0x1234。
    // 若实现误把稀疏段计入累加，这里会得到别的地址，进而读错簇。
    Expect(Parse(Bytes({0x01, 0x05, 0x21, 0x08, 0x34, 0x12, 0x00}), runs),
           "sparse: followed by real run");
    Expect(runs.size() == 2, "sparse: two runs");
    if (runs.size() == 2) {
        Expect(runs[0].isSparse, "sparse: first is sparse");
        Expect(!runs[1].isSparse, "sparse: second is real");
        Expect(runs[1].startLcn == 0x1234, "sparse: does not advance lcn base");
    }
}

void TestRunListRejects()
{
    std::vector<ks::file::NtfsDataRun> runs;

    Expect(!ks::file::ParseNtfsRunList(nullptr, nullptr, runs), "reject: null pointers");

    const std::vector<std::byte> single = Bytes({0x21});
    Expect(!ks::file::ParseNtfsRunList(single.data(), single.data(), runs), "reject: empty range");

    // 首字节即终止符：非常驻属性至少应有一段，空 runlist 视为无效。
    Expect(!Parse(Bytes({0x00}), runs), "reject: empty run list");

    // 0x20 的长度字段为 0，无法确定簇数。
    Expect(!Parse(Bytes({0x20, 0x34, 0x12, 0x00}), runs), "reject: zero length field");

    // 长度/偏移字段声明超过 8 字节，超出 LCN 与簇数的表示范围。
    Expect(!Parse(Bytes({0x29, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00}), runs),
           "reject: length field over 8 bytes");
    Expect(!Parse(Bytes({0x91, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00}), runs),
           "reject: offset field over 8 bytes");

    // 声明的字段跨过缓冲末尾。越界读正是这类解析器最容易崩的地方。
    Expect(!Parse(Bytes({0x21, 0x08, 0x34}), runs), "reject: fields cross the end");

    // 簇数为 0 的区间没有意义，且会让上层的区间推进原地打转。
    Expect(!Parse(Bytes({0x21, 0x00, 0x34, 0x12, 0x00}), runs), "reject: zero cluster count");

    // 负增量把 LCN 推到 0 以下：卷上不存在负簇号。
    Expect(!Parse(Bytes({0x21, 0x08, 0x34, 0x12, 0x21, 0x04, 0x00, 0xD0, 0x00}), runs),
           "reject: lcn driven negative");

    // 增量为 INT64_MIN：取负号本身就是有符号溢出，必须在取负之前拦掉。
    Expect(!Parse(Bytes({0x81, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00}), runs),
           "reject: int64 min delta");

    // 正向增量溢出 int64。
    Expect(!Parse(Bytes({0x81, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
                         0x81, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x00}), runs),
           "reject: positive overflow");

    // 每条失败路径都必须清空输出，调用方不能拿到半截结果当成有效区间。
    Expect(runs.empty(), "reject: output cleared on failure");
}

} // namespace

int main()
{
    TestReadSignedLittleEndian();
    TestRunListHappyPath();
    TestRunListSparse();
    TestRunListRejects();

    if (failures != 0) {
        std::cerr << "KswordFsDecodeTests: " << failures << " assertion(s) failed\n";
        return 1;
    }
    std::cout << "KswordFsDecodeTests: all assertions passed\n";
    return 0;
}
