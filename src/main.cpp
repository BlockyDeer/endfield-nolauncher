#include "launch.h"

#include <exception>
#include <iostream>
#include <string>

namespace {

std::wstring Timestamp() {
    SYSTEMTIME now{};
    GetLocalTime(&now);
    wchar_t text[32]{};
    _snwprintf_s(text, _countof(text), _TRUNCATE, L"%04u-%02u-%02u %02u:%02u:%02u",
        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);
    return text;
}

std::filesystem::path ExecutablePath() {
    std::wstring value(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, value.data(),
        static_cast<DWORD>(value.size()));
    if (length == 0 || length >= value.size()) return {};
    value.resize(length);
    return value;
}

std::wstring DescribeCommandLine(int argc, wchar_t* argv[]) {
    std::wstring text;
    for (int index = 0; index < argc; ++index) {
        if (index) text += L' ';
        text += nolauncher::QuoteArgument(argv[index]);
    }
    return text;
}

// Mirrors std::wcerr into a UTF-8 log file, one timestamped line at a time.
//
// A run started by Steam, a shortcut, or another launcher has no visible
// console, so its diagnostics would otherwise have nowhere to go. The launch
// core writes to std::wcerr, so teeing that stream captures the whole run
// without the core knowing a log exists.
class LogTee : public std::wstreambuf {
public:
    LogTee(std::wstreambuf* console, HANDLE file) : console_(console), file_(file) {}

    ~LogTee() override { Flush(); }

protected:
    int_type overflow(int_type character) override {
        if (traits_type::eq_int_type(character, traits_type::eof())) {
            return traits_type::not_eof(character);
        }
        const wchar_t text = traits_type::to_char_type(character);
        if (text == L'\n') Flush();
        else if (text != L'\r') pending_ += text;
        console_->sputc(text);
        return character;
    }

    std::streamsize xsputn(const wchar_t* text, std::streamsize count) override {
        for (std::streamsize index = 0; index < count; ++index) {
            overflow(traits_type::to_int_type(text[index]));
        }
        return count;
    }

private:
    void Flush() {
        if (pending_.empty() || !file_ || file_ == INVALID_HANDLE_VALUE) return;
        const std::wstring line = Timestamp() + L" " + pending_ + L"\r\n";
        pending_.clear();
        const int characters = static_cast<int>(line.size());
        const int bytes = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), characters,
            nullptr, 0, nullptr, nullptr);
        if (bytes <= 0) return;
        std::string utf8(static_cast<size_t>(bytes), '\0');
        WideCharToMultiByte(CP_UTF8, 0, line.c_str(), characters, utf8.data(), bytes,
            nullptr, nullptr);
        DWORD written = 0;
        WriteFile(file_, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
    }

    std::wstreambuf* console_;
    HANDLE file_;
    std::wstring pending_;
};

class ScopedRedirect {
public:
    ScopedRedirect(std::wostream& stream, std::wstreambuf* replacement)
        : stream_(stream), previous_(stream.rdbuf(replacement)) {}
    ~ScopedRedirect() { stream_.rdbuf(previous_); }
    ScopedRedirect(const ScopedRedirect&) = delete;
    ScopedRedirect& operator=(const ScopedRedirect&) = delete;
private:
    std::wostream& stream_;
    std::wstreambuf* previous_;
};

// Appends to a log beside the executable. Failing to open it is not fatal: the
// run continues with console and dialog reporting only.
nolauncher::Handle OpenLog() {
    const auto executable = ExecutablePath();
    if (executable.empty()) return nolauncher::Handle();
    const auto path = executable.parent_path() / L"EndfieldNoLauncher.log";
    return nolauncher::Handle(CreateFileW(path.c_str(), FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr));
}

// A detached console has nowhere to print, so failures from that point on must
// reach the user through a dialog instead.
class Reporter {
public:
    explicit Reporter(bool use_dialog) : use_dialog_(use_dialog) {}

    void Report(const std::wstring& message) const {
        if (use_dialog_) nolauncher::ShowError(message);
        else std::wcerr << message << L'\n';
    }

private:
    bool use_dialog_;
};

DWORD Run(int argc, wchar_t* argv[]) {
    using namespace nolauncher;

    Options options;
    std::wstring error;
    if (!ParseOptions(argc, argv, options, error)) {
        std::wcerr << error << L'\n' << Usage();
        return 2;
    }
    if (options.help) {
        std::wcout << Usage();
        return 0;
    }
    std::wcerr << L"游戏 / Game: " << options.game.wstring() << L'\n';

    ConfigureConsole(options);
    const Reporter report(options.hide_console);

    VerifiedLauncher launcher;
    if (!FindOfficialLauncher(options.parent, launcher, error)) { report.Report(error); return 7; }
    std::wcerr << L"官方启动器 / Official launcher: " << launcher.executable.wstring() << L'\n';

    OfficialParent parent;
    if (!parent.Start(launcher, error)) { report.Report(error); return 4; }
    std::wcerr << L"已创建挂起的官方父进程 / Suspended official parent created.\n";
    // The parent image is mapped and its signature has been read. Releasing the
    // file lets the official updater replace the launcher while the game runs.
    launcher.file.reset();

    // The game is created without CREATE_SUSPENDED because nothing is injected:
    // it starts running as soon as it has the official parent.
    PROCESS_INFORMATION game{};
    if (!parent.CreateGame(options.game, options.arguments, 0, game, error)) {
        report.Report(error);
        return 4;
    }
    std::wcerr << L"游戏进程已创建，PID / Game created, pid " << game.dwProcessId << L'\n';
    // A created game may already be running. Its handle is closed when the game
    // exits, and nothing here terminates a game this process did not launch.
    Handle process(game.hProcess);
    CloseHandle(game.hThread);

    std::wcerr << L"等待游戏退出… / Waiting for the game to exit…\n";
    DWORD exit_code = 0;
    if (!WaitForGame(process.get(), exit_code, error)) { report.Report(error); return 8; }
    return exit_code;
}

} // namespace

int wmain(int argc, wchar_t* argv[]) {
    // Opened before parsing so that a rejected command line is recorded too,
    // which is the only evidence available for a launcher-driven run.
    const nolauncher::Handle log = OpenLog();
    LogTee tee(std::wcerr.rdbuf(), log.get());
    ScopedRedirect redirect(std::wcerr, &tee);

    std::wcerr << L"启动 / Launch: " << DescribeCommandLine(argc, argv) << L'\n';

    DWORD code = 4;
    try {
        code = Run(argc, argv);
    } catch (const std::exception&) {
        nolauncher::ShowError(L"启动时发生异常。 / Launch failed with an unexpected exception.");
    }
    std::wcerr << L"退出码 / Exit code: " << code << L'\n';
    return static_cast<int>(code);
}
