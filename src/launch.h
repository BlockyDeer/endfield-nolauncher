#pragma once

// Launches the game under a signed, suspended official launcher process.
//
// The game validates its *direct* parent process — name, Authenticode
// signature, and signer certificate fingerprint — and refuses to start when
// the parent is not the official Hypergryph launcher. This module satisfies
// that check without running the launcher UI: it creates the official
// Games.exe suspended, never resumes it, and assigns it as the parent of the
// game process through PROC_THREAD_ATTRIBUTE_PARENT_PROCESS.
//
// No code is injected into the launcher or the game, and no game file is
// modified. Every failure path reports through `error`; the caller owns the
// VerifiedLauncher file handle until it releases it.

#include <windows.h>

#include <filesystem>
#include <string>
#include <vector>

namespace nolauncher {

struct Options {
    std::filesystem::path game;          // --game, must be named Endfield.exe
    std::filesystem::path parent;        // --parent, empty selects discovery
    std::vector<std::wstring> arguments; // arguments after --
    bool hide_console = false;           // --hide-console
    bool help = false;                   // --help, -h, /?
};

// Parses argv. Any invalid option returns false with `error` set. A help
// request sets options.help, returns true, and skips every other check.
bool ParseOptions(int argc, wchar_t* argv[], Options& options, std::wstring& error);

std::wstring Usage();

// Quotes one argument for a CreateProcessW command line: wraps in double
// quotes and escapes backslashes that precede a quote or the closing quote.
std::wstring QuoteArgument(const std::wstring& argument);
std::wstring WindowsError(const wchar_t* operation, DWORD code);

// Reports a failure with a modal dialog. Callers use this only when no console
// is attached to print to.
void ShowError(const std::wstring& message);

// Detaches the console for --hide-console. A command-line launch keeps the
// console it inherited.
void ConfigureConsole(const Options& options);

class Handle {
public:
    explicit Handle(HANDLE value = nullptr) : value_(value) {}
    ~Handle();
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    Handle(Handle&& other) noexcept;
    Handle& operator=(Handle&& other) noexcept;
    HANDLE get() const { return value_; }
    void reset(HANDLE value = nullptr);
private:
    HANDLE value_ = nullptr;
};

struct VerifiedLauncher {
    std::filesystem::path executable;
    // Keeps the executable open from signature verification through process
    // creation, so it cannot be replaced in that interval.
    Handle file;
};

bool IsOfficialPublisher(const std::wstring& organization);

// Requires a file named Games.exe with a trusted signature whose signer
// organization is the official publisher. On success `result.file` is open.
bool VerifyOfficialLauncher(const std::filesystem::path& path, VerifiedLauncher& result,
    std::wstring& error);

// Verifies `requested` when it is non-empty. Otherwise enumerates the
// registered launcher installations and returns the highest version that
// verifies.
bool FindOfficialLauncher(const std::filesystem::path& requested, VerifiedLauncher& result,
    std::wstring& error);

// Owns the suspended launcher process that serves as the game's parent.
class OfficialParent {
public:
    OfficialParent() = default;
    ~OfficialParent();
    OfficialParent(const OfficialParent&) = delete;
    OfficialParent& operator=(const OfficialParent&) = delete;

    // Requires a verified launcher whose file handle is still open.
    bool Start(const VerifiedLauncher& launcher, std::wstring& error);

    // Creates the game with the suspended launcher as its direct parent.
    // The caller owns `process.hProcess` and `process.hThread`.
    bool CreateGame(const std::filesystem::path& game,
        const std::vector<std::wstring>& arguments, DWORD creation_flags,
        PROCESS_INFORMATION& process, std::wstring& error) const;

private:
    Handle process_;
};

// Blocks until the process exits, then reads its exit code.
bool WaitForGame(HANDLE process, DWORD& exit_code, std::wstring& error);

} // namespace nolauncher
