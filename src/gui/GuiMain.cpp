#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shellapi.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include "app/Application.h"

namespace {

constexpr int kResultsPath = 1001;
constexpr int kBrowseButton = 1002;
constexpr int kSessionCombo = 1003;
constexpr int kMatricInput = 1004;
constexpr int kSearchButton = 1005;
constexpr int kStatusLabel = 1006;
constexpr int kSummaryLabel = 1007;
constexpr int kOpenResultButton = 1008;
constexpr UINT kExtractionComplete = WM_APP + 1;

HWND g_resultsPath = nullptr;
HWND g_sessionCombo = nullptr;
HWND g_matricInput = nullptr;
HWND g_searchButton = nullptr;
HWND g_statusLabel = nullptr;
HWND g_summaryLabel = nullptr;
HWND g_openResultButton = nullptr;
HFONT g_font = nullptr;
std::thread g_extractionWorker;
std::wstring g_outputPath;
bool g_extractionRunning = false;

void setControlFont(HWND control) {
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
}

void setStatus(const wchar_t* message) {
    SetWindowTextW(g_statusLabel, message);
}

std::wstring selectedSessionName() {
    const auto selectedIndex = SendMessageW(g_sessionCombo, CB_GETCURSEL, 0, 0);
    if (selectedIndex == CB_ERR) {
        return {};
    }

    const auto length = SendMessageW(g_sessionCombo, CB_GETLBTEXTLEN, selectedIndex, 0);
    if (length == CB_ERR) {
        return {};
    }

    std::wstring session(static_cast<size_t>(length) + 1, L'\0');
    SendMessageW(g_sessionCombo, CB_GETLBTEXT, selectedIndex,
                 reinterpret_cast<LPARAM>(session.data()));
    session.resize(static_cast<size_t>(length));
    return session;
}

void enableSearch(bool enabled) {
    EnableWindow(g_searchButton, enabled);
    EnableWindow(g_sessionCombo, enabled);
    EnableWindow(g_matricInput, enabled);
}

std::string wideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(),
                                         static_cast<int>(value.size()),
                                         nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(),
                        static_cast<int>(value.size()), result.data(), size,
                        nullptr, nullptr);
    return result;
}

void populateSessions(const std::filesystem::path& resultsFolder) {
    SendMessageW(g_sessionCombo, CB_RESETCONTENT, 0, 0);

    if (!std::filesystem::is_directory(resultsFolder)) {
        SendMessageW(g_sessionCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(L"No folder selected"));
        SendMessageW(g_sessionCombo, CB_SETCURSEL, 0, 0);
        return;
    }

    std::vector<std::wstring> sessions;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(resultsFolder, error)) {
        if (entry.is_directory(error)) {
            sessions.push_back(entry.path().filename().wstring());
        }
    }
    std::sort(sessions.begin(), sessions.end());

    for (const auto& session : sessions) {
        SendMessageW(g_sessionCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(session.c_str()));
    }

    if (sessions.empty()) {
        SendMessageW(g_sessionCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(L"No sessions found"));
    }
    SendMessageW(g_sessionCombo, CB_SETCURSEL, 0, 0);
}

std::filesystem::path chooseFolder(HWND owner) {
    BROWSEINFOW browseInfo{};
    browseInfo.hwndOwner = owner;
    browseInfo.lpszTitle = L"Select the Results folder";
    browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    PIDLIST_ABSOLUTE item = SHBrowseForFolderW(&browseInfo);
    if (item == nullptr) {
        return {};
    }

    wchar_t selectedPath[MAX_PATH]{};
    const bool resolved = SHGetPathFromIDListW(item, selectedPath) != FALSE;
    CoTaskMemFree(item);
    return resolved ? std::filesystem::path(selectedPath) : std::filesystem::path{};
}

void browseForResults(HWND window) {
    const auto selectedFolder = chooseFolder(window);
    if (selectedFolder.empty()) {
        return;
    }

    const auto pathText = selectedFolder.wstring();
    SetWindowTextW(g_resultsPath, pathText.c_str());
    populateSessions(selectedFolder);
    setStatus(L"Ready");
    SetWindowTextW(g_summaryLabel,
                   L"Choose a session and enter a matric number to begin.");
}

void handleSearch(HWND window) {
    wchar_t resultsPath[MAX_PATH]{};
    wchar_t matricNumber[256]{};
    GetWindowTextW(g_resultsPath, resultsPath, MAX_PATH);
    GetWindowTextW(g_matricInput, matricNumber, 256);

    if (wcslen(resultsPath) == 0) {
        setStatus(L"Error");
        SetWindowTextW(g_summaryLabel, L"Please select a Results folder.");
        return;
    }
    const auto sessionName = selectedSessionName();
    const auto sessionPath = std::filesystem::path(resultsPath) / sessionName;
    if (sessionName.empty() || !std::filesystem::is_directory(sessionPath)) {
        setStatus(L"Error");
        SetWindowTextW(g_summaryLabel, L"Please select an academic session.");
        return;
    }
    if (wcslen(matricNumber) == 0) {
        setStatus(L"Error");
        SetWindowTextW(g_summaryLabel, L"Please enter a matric number.");
        return;
    }

    if (g_extractionRunning) {
        return;
    }

    g_extractionRunning = true;
    g_outputPath.clear();
    EnableWindow(g_openResultButton, FALSE);
    enableSearch(false);
    setStatus(L"Searching...");
    SetWindowTextW(g_summaryLabel, L"Searching workbooks. Please wait...");

    const auto matric = std::wstring(matricNumber);
    g_extractionWorker = std::thread([window, sessionPath, matric]() {
        auto* completion = new std::pair<bool, sde::ExtractionNotification>{};
        try {
            sde::Application application;
            completion->second = application.extractStudent(sessionPath, wideToUtf8(matric));
            completion->first = true;
        } catch (const std::exception&) {
            completion->first = false;
        }

        if (!PostMessageW(window, kExtractionComplete,
                          0, reinterpret_cast<LPARAM>(completion))) {
            delete completion;
        }
    });
}

void showExtractionResult(std::pair<bool, sde::ExtractionNotification>* completion) {
    if (g_extractionWorker.joinable()) {
        g_extractionWorker.join();
    }
    g_extractionRunning = false;
    enableSearch(true);

    if (!completion->first) {
        setStatus(L"Error");
        SetWindowTextW(g_summaryLabel,
                       L"Extraction could not be completed. Check the log for details.");
        delete completion;
        return;
    }

    const auto& result = completion->second;
    if (result.records.empty()) {
        setStatus(L"Completed");
        SetWindowTextW(g_summaryLabel, L"No matching records were found.");
        delete completion;
        return;
    }

    setStatus(L"Completed");
    const auto summary = L"Extraction Complete\r\n"
        L"Workbooks checked: " + std::to_wstring(result.processed) +
        L"\r\nMatching records: " + std::to_wstring(result.matches) +
        L"\r\nRecords extracted: " + std::to_wstring(result.records.size()) +
        L"\r\nFiles with no match: " + std::to_wstring(result.noMatch) +
        L"\r\nFiles with errors: " + std::to_wstring(result.errors.size());
    SetWindowTextW(g_summaryLabel, summary.c_str());

    if (!result.outputPath.empty()) {
        g_outputPath = std::filesystem::path(result.outputPath).wstring();
        EnableWindow(g_openResultButton, TRUE);
    }
    delete completion;
}

void openResult() {
    if (g_outputPath.empty()) {
        return;
    }
    const auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(
        nullptr, L"open", g_outputPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
    if (result <= 32) {
        setStatus(L"Error");
        SetWindowTextW(g_summaryLabel, L"The result file could not be opened.");
    }
}

HWND createLabel(HWND parent, const wchar_t* text, int x, int y, int width, int height) {
    HWND label = CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE,
                               x, y, width, height, parent, nullptr,
                               GetModuleHandleW(nullptr), nullptr);
    setControlFont(label);
    return label;
}

HWND createEdit(HWND parent, int id, int x, int y, int width, int height, DWORD style) {
    HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                WS_CHILD | WS_VISIBLE | style,
                                x, y, width, height, parent,
                                reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                GetModuleHandleW(nullptr), nullptr);
    setControlFont(edit);
    return edit;
}

void createControls(HWND window) {
    createLabel(window, L"Results Folder", 24, 24, 140, 24);
    g_resultsPath = createEdit(window, kResultsPath, 24, 50, 470, 30,
                               ES_AUTOHSCROLL | ES_READONLY);
    HWND browse = CreateWindowW(L"BUTTON", L"Browse...",
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                504, 50, 100, 30, window,
                                reinterpret_cast<HMENU>(static_cast<INT_PTR>(kBrowseButton)),
                                GetModuleHandleW(nullptr), nullptr);
    setControlFont(browse);

    createLabel(window, L"Academic Session", 24, 96, 160, 24);
    g_sessionCombo = CreateWindowW(WC_COMBOBOXW, L"",
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
                                   24, 122, 280, 180, window,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSessionCombo)),
                                   GetModuleHandleW(nullptr), nullptr);
    setControlFont(g_sessionCombo);
    SendMessageW(g_sessionCombo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(L"No folder selected"));
    SendMessageW(g_sessionCombo, CB_SETCURSEL, 0, 0);

    createLabel(window, L"Matric Number", 24, 168, 160, 24);
    g_matricInput = createEdit(window, kMatricInput, 24, 194, 280, 30,
                               ES_AUTOHSCROLL | ES_LEFT);
    SendMessageW(g_matricInput, EM_SETCUEBANNER, TRUE,
                 reinterpret_cast<LPARAM>(L"Example: DE.2024/1755"));

    g_searchButton = CreateWindowW(L"BUTTON", L"Search",
                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                24, 244, 130, 36, window,
                                reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSearchButton)),
                                GetModuleHandleW(nullptr), nullptr);
    setControlFont(g_searchButton);

    createLabel(window, L"Status", 24, 310, 100, 24);
    g_statusLabel = createLabel(window, L"Ready", 120, 310, 180, 24);
    createLabel(window, L"Results Summary", 24, 350, 180, 24);
    g_summaryLabel = createLabel(window,
                                 L"Choose a Results folder to discover sessions.",
                                 24, 378, 580, 48);

    g_openResultButton = CreateWindowW(L"BUTTON", L"Open Result",
                                    WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_PUSHBUTTON,
                                    24, 442, 130, 34, window,
                                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kOpenResultButton)),
                                    GetModuleHandleW(nullptr), nullptr);
    setControlFont(g_openResultButton);
}

void resizeControls(HWND window) {
    RECT client{};
    GetClientRect(window, &client);
    const int width = std::max(640, static_cast<int>(client.right - client.left));
    MoveWindow(g_resultsPath, 24, 50, width - 160, 30, TRUE);
    MoveWindow(GetDlgItem(window, kBrowseButton), width - 130, 50, 106, 30, TRUE);
    MoveWindow(g_summaryLabel, 24, 378, width - 48, 48, TRUE);
}

LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        createControls(window);
        return 0;
    case WM_SIZE:
        resizeControls(window);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == kBrowseButton) {
            browseForResults(window);
        } else if (LOWORD(wParam) == kSearchButton) {
            handleSearch(window);
        } else if (LOWORD(wParam) == kOpenResultButton) {
            openResult();
        }
        return 0;
    case kExtractionComplete:
        showExtractionResult(reinterpret_cast<std::pair<bool, sde::ExtractionNotification>*>(lParam));
        return 0;
    case WM_DESTROY:
        if (g_extractionWorker.joinable()) {
            g_extractionWorker.join();
        }
        DeleteObject(g_font);
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    INITCOMMONCONTROLSEX commonControls{sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&commonControls);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    g_font = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                         L"Segoe UI");

    const wchar_t className[] = L"StudentDataExtractorGuiWindow";
    WNDCLASSW windowClass{};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = windowProcedure;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&windowClass);

    HWND window = CreateWindowExW(0, className, L"Student Data Extractor",
                                  WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 680, 550,
                                  nullptr, nullptr, instance, nullptr);
    if (window == nullptr) {
        DeleteObject(g_font);
        CoUninitialize();
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    CoUninitialize();
    return static_cast<int>(message.wParam);
}
