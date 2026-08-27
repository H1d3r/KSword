#include "../KswordARKLight/Core/DriverLeasePolicy.h"
#include "../KswordARKLight/Core/EntityRef.h"
#include "../KswordARKLight/Features/Memory/MemorySnapshot.h"
#include "../KswordARKLight/Features/Memory/MemoryInspection.h"
#include "../KswordARKLight/Ui/EvidenceSession.h"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void Expect(const bool condition, const wchar_t* label) {
    if (!condition) {
        ++failures;
        std::wcerr << L"FAIL: " << label << L'\n';
    }
}

} // namespace

int wmain() {
    using Ksword::Core::CommandInputKind;
    using Ksword::Core::DriverLeasePolicy;
    using Ksword::Core::EntityKind;
    using Ksword::Core::NavigationTarget;
    using Ksword::Core::ParseCommandInput;
    using Ksword::Features::Memory::MemorySnapshotHistory;
    using Ksword::Features::Memory::MemoryReadSnapshot;

    Expect(DriverLeasePolicy::OwnsStartTransition(false, true), L"new driver start is owned");
    Expect(!DriverLeasePolicy::OwnsStartTransition(true, true), L"pre-existing driver is not owned");
    Expect(DriverLeasePolicy::ShouldStopOnLastRelease(true, 0), L"owned driver stops on last lease");
    Expect(!DriverLeasePolicy::ShouldStopOnLastRelease(true, 1), L"owned driver stays for peer lease");
    Expect(!DriverLeasePolicy::ShouldStopOnLastRelease(false, 0), L"pre-existing driver stays running");

    const auto process = ParseCommandInput(L" pid 1234 ");
    Expect(process.kind == CommandInputKind::Navigation, L"pid command navigates");
    Expect(process.navigation.target == NavigationTarget::ProcessDetails, L"pid target");
    Expect(process.navigation.entity.kind == EntityKind::Process && process.navigation.entity.id == 1234,
        L"pid entity identity");

    const auto window = ParseCommandInput(L"hwnd 0xABC");
    Expect(window.navigation.target == NavigationTarget::WindowManager && window.navigation.entity.id == 0xABCU,
        L"hex HWND command");

    const auto file = ParseCommandInput(L"file C:\\Windows\\System32\\ntdll.dll");
    Expect(file.navigation.target == NavigationTarget::FileBrowser &&
        file.navigation.entity.text == L"C:\\Windows\\System32\\ntdll.dll", L"file command");

    const auto module = ParseCommandInput(L"网络");
    Expect(module.navigation.entity.kind == EntityKind::Module && module.navigation.entity.text == L"网络",
        L"plain module title");

    const auto shell = ParseCommandInput(L"! whoami /all");
    Expect(shell.kind == CommandInputKind::Shell && shell.shellCommand == L"whoami /all", L"explicit shell escape");
    Expect(ParseCommandInput(L"pid zero").kind == CommandInputKind::Invalid, L"invalid pid rejected");

    MemorySnapshotHistory snapshots(2U);
    Expect(!snapshots.record(0U, 0x1000U, 4U, { 1U }, L"bad"), L"snapshot rejects missing pid");
    Expect(snapshots.record(42U, 0x1000U, 4U, { 1U, 2U, 3U, 4U }, L"first"), L"first snapshot recorded");
    Expect(snapshots.record(42U, 0x2000U, 2U, { 5U, 6U }, L"second"), L"second snapshot recorded");
    Expect(snapshots.canMovePrevious() && !snapshots.canMoveNext(), L"snapshot back navigation available");
    Expect(snapshots.movePrevious() && snapshots.current() && snapshots.current()->address == 0x1000U,
        L"snapshot previous selects first bytes");
    Expect(snapshots.record(42U, 0x3000U, 1U, { 7U }, L"branch"), L"snapshot branch recorded");
    Expect(snapshots.size() == 2U && !snapshots.canMoveNext() && snapshots.current() && snapshots.current()->address == 0x3000U,
        L"snapshot branch truncates forward history");

    MemoryReadSnapshot inspect{};
    inspect.sequence = 7U;
    inspect.processId = 42U;
    inspect.address = 0x1000U;
    inspect.requestedBytes = 13U;
    inspect.bytes = { 'T', 'e', 's', 't', 0U, 'W', 0U, 'i', 0U, 'd', 0U, 'e', 0U };
    inspect.statusText = L"partial read";
    const std::wstring hexAscii = Ksword::Features::Memory::RenderMemorySnapshotHexAscii(inspect);
    const std::wstring textRuns = Ksword::Features::Memory::ExtractMemorySnapshotText(inspect);
    Expect(hexAscii.find(L"0x0000000000001000") != std::wstring::npos && hexAscii.find(L"Test") != std::wstring::npos,
        L"memory hex ascii view includes address and printable bytes");
    Expect(textRuns.find(L"ASCII") != std::wstring::npos && textRuns.find(L"UTF-16LE") != std::wstring::npos,
        L"memory inspection extracts ascii and utf16 text runs");
    Expect(Ksword::Features::Memory::BuildMemorySnapshotTextReport(inspect).find(L"ReturnedBytes") != std::wstring::npos,
        L"memory inspection report includes snapshot metadata");

    const std::wstring redacted = Ksword::Ui::RedactEvidenceText(
        L"C:\\Users\\Felix\\Desktop\\sample.txt", Ksword::Ui::EvidenceRedaction::Privacy);
    Expect(redacted == L"C:\\Users\\<redacted>\\Desktop\\sample.txt", L"privacy path redaction");

    const Ksword::Ui::EvidenceDiff diff = Ksword::Ui::BuildEvidenceDiff(L"one\r\ntwo\r\n", L"two\r\nthree\r\n");
    Expect(diff.added.size() == 1U && diff.added.front() == L"three", L"evidence added line");
    Expect(diff.removed.size() == 1U && diff.removed.front() == L"one", L"evidence removed line");
    Expect(diff.unchanged.size() == 1U && diff.unchanged.front() == L"two", L"evidence unchanged line");

    Ksword::Ui::EvidenceSession session;
    Expect(session.record(L"process", L"tsv", L"pid\tname") == 1U, L"first evidence sequence");
    session.record(L"process", L"tsv", L"pid\tname\r\n4\tSystem");
    Expect(session.size() == 2U, L"evidence session size");
    Expect(session.exportJson(Ksword::Ui::EvidenceRedaction::Privacy).find(L"ksword-arklight-evidence-v1") !=
        std::wstring::npos, L"evidence JSON schema");
    Expect(Ksword::Ui::RenderEvidenceDiff(session.latestDiff()).find(L"+ 4\tSystem") != std::wstring::npos,
        L"latest evidence diff");

    if (failures == 0) {
        std::wcout << L"KswordARKLightTests: PASS\n";
    }
    return failures == 0 ? 0 : 1;
}
