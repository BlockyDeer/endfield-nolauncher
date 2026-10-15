// Exercises the two pure parts of the launcher: command-line parsing and
// CreateProcessW argument quoting. The process-creation path needs a signed
// official launcher and the real game, so it is out of scope here.

#include "../src/launch.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void Check(bool condition, const wchar_t* what) {
    if (condition) return;
    std::wcout << L"FAIL: " << what << L'\n';
    ++g_failures;
}

void CheckEqual(const std::wstring& actual, const std::wstring& expected, const wchar_t* what) {
    if (actual == expected) return;
    std::wcout << L"FAIL: " << what << L"\n  expected: " << expected
               << L"\n  actual:   " << actual << L'\n';
    ++g_failures;
}

// Keeps the argument storage stable so the pointers handed to ParseOptions stay
// valid for the whole call.
struct Argv {
    std::vector<std::wstring> storage;
    std::vector<wchar_t*> pointers;

    explicit Argv(std::vector<std::wstring> values) : storage(std::move(values)) {
        pointers.reserve(storage.size());
        for (auto& value : storage) pointers.push_back(value.data());
    }
    int argc() const { return static_cast<int>(pointers.size()); }
    wchar_t** argv() { return pointers.data(); }
};

std::filesystem::path MakeGameFile() {
    const auto directory = std::filesystem::temp_directory_path() / L"endfield-nolauncher-tests";
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    const auto game = directory / L"Endfield.exe";
    std::ofstream(game, std::ios::binary) << "test stub";
    return game;
}

bool Parse(std::vector<std::wstring> values, nolauncher::Options& options, std::wstring& error) {
    Argv argv(std::move(values));
    return nolauncher::ParseOptions(argv.argc(), argv.argv(), options, error);
}

void TestQuoteArgument() {
    using nolauncher::QuoteArgument;
    CheckEqual(QuoteArgument(L""), L"\"\"", L"empty argument is quoted");
    CheckEqual(QuoteArgument(L"plain"), L"\"plain\"", L"plain argument is quoted");
    CheckEqual(QuoteArgument(L"C:\\Program Files\\a.exe"), L"\"C:\\Program Files\\a.exe\"",
        L"spaces are contained by the quotes");
    // A trailing backslash must be doubled, otherwise it escapes the closing quote.
    CheckEqual(QuoteArgument(L"C:\\dir\\"), L"\"C:\\dir\\\\\"",
        L"trailing backslash is doubled");
    // Only backslashes directly before a quote are doubled.
    CheckEqual(QuoteArgument(L"a\\b"), L"\"a\\b\"", L"interior backslash is preserved");
    CheckEqual(QuoteArgument(L"a\"b"), L"\"a\\\"b\"", L"interior quote is escaped");
    CheckEqual(QuoteArgument(L"a\\\"b"), L"\"a\\\\\\\"b\"",
        L"backslash before a quote is doubled first");
}

void TestArgumentSeparator() {
    const auto game = MakeGameFile();
    nolauncher::Options options;
    std::wstring error;
    const bool parsed = Parse({L"tool", L"--game", game.wstring(), L"--",
        L"-logFile", L"C:\\logs\\a b.txt", L"--steam-command"}, options, error);
    Check(parsed, L"a launch with game arguments parses");
    Check(options.arguments.size() == 3, L"every argument after -- is forwarded");
    if (options.arguments.size() == 3) {
        CheckEqual(options.arguments[0], L"-logFile", L"first forwarded argument keeps its spacing");
        CheckEqual(options.arguments[1], L"C:\\logs\\a b.txt", L"forwarded argument keeps embedded spaces");
        CheckEqual(options.arguments[2], L"--steam-command", L"option-shaped game arguments are not re-parsed");
    }

    // Without the separator the same token is an option, because the tool's own
    // options and the game's arguments share one command line.
    nolauncher::Options rejected;
    Check(!Parse({L"tool", L"--game", game.wstring(), L"--steam-command"}, rejected, error),
        L"an option-shaped argument needs the -- separator");
}

void TestHelp() {
    nolauncher::Options options;
    std::wstring error;
    Check(Parse({L"tool", L"--help"}, options, error), L"--help parses without --game");
    Check(options.help, L"--help sets help");
    Check(Parse({L"tool", L"-h"}, options, error), L"-h parses");
    Check(options.help, L"-h sets help");
}

void TestGameValidation() {
    const auto game = MakeGameFile();
    nolauncher::Options options;
    std::wstring error;

    Check(Parse({L"tool", L"--game", game.wstring()}, options, error), L"a real Endfield.exe parses");
    Check(options.game.is_absolute(), L"the game path becomes absolute");
    Check(!options.hide_console, L"hide_console defaults to false");
    Check(options.arguments.empty(), L"arguments default to empty");

    Check(!Parse({L"tool"}, options, error), L"a missing --game is rejected");
    Check(!Parse({L"tool", L"--game", L"C:\\tmp\\notepad.exe"}, options, error),
        L"an executable that is not Endfield.exe is rejected");
    Check(!Parse({L"tool", L"--game", L"C:\\missing\\Endfield.exe"}, options, error),
        L"a missing game file is rejected");
    Check(!Parse({L"tool", L"--game"}, options, error), L"a dangling --game is rejected");
    Check(!Parse({L"tool", L"--game", game.wstring(), L"--game", game.wstring()}, options, error),
        L"a duplicate --game is rejected");
}

void TestRemovedInjectorOptions() {
    const auto game = MakeGameFile();
    nolauncher::Options options;
    std::wstring error;
    // The launcher-only build must not accept the injector's options.
    for (const wchar_t* option : {L"--manual-map", L"--manual-map-late"}) {
        Check(!Parse({L"tool", L"--game", game.wstring(), option}, options, error),
            L"an injector option is rejected");
    }
}

void TestParentAndConsole() {
    const auto game = MakeGameFile();
    nolauncher::Options options;
    std::wstring error;
    const bool parsed = Parse({L"tool", L"--game", game.wstring(),
        L"--parent", L"C:\\Launcher\\Games.exe", L"--hide-console"}, options, error);
    Check(parsed, L"--parent and --hide-console parse");
    CheckEqual(options.parent.wstring(), L"C:\\Launcher\\Games.exe", L"--parent is stored verbatim");
    Check(options.hide_console, L"--hide-console is stored");

    // An empty --parent selects discovery, which is the documented default.
    Check(Parse({L"tool", L"--game", game.wstring()}, options, error), L"discovery mode parses");
    Check(options.parent.empty(), L"an omitted --parent stays empty");
}

} // namespace

int wmain() {
    TestQuoteArgument();
    TestArgumentSeparator();
    TestHelp();
    TestGameValidation();
    TestRemovedInjectorOptions();
    TestParentAndConsole();

    if (g_failures == 0) {
        std::wcout << L"All option and quoting checks passed.\n";
        return 0;
    }
    std::wcout << g_failures << L" check(s) failed.\n";
    return 1;
}
