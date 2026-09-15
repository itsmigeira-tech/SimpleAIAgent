#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <sstream>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winhttp.lib")

#define ID_INPUT 101
#define ID_OUTPUT 102
#define ID_SEND 103
#define ID_MODEL 104
#define ID_STATUS 105

HWND hInput, hOutput, hSend, hModel, hStatus;
std::wstring currentModel = L"llama3.2";

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    std::string s(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], size, NULL, NULL);
    return s;
}

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    std::wstring w(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], size);
    return w;
}

std::string HttpPost(const std::wstring& host, INTERNET_PORT port, const std::wstring& path, const std::string& body) {
    HINTERNET hSession = WinHttpOpen(L"SimpleAIAgent/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "";

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "";
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    std::wstring headers = L"Content-Type: application/json\r\n";
    BOOL bResults = WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)headers.length(), (LPVOID)body.c_str(), (DWORD)body.size(), (DWORD)body.size(), 0);
    if (bResults) bResults = WinHttpReceiveResponse(hRequest, NULL);

    std::string response;
    if (bResults) {
        DWORD dwSize = 0;
        do {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
            if (dwSize == 0) break;
            std::vector<char> buffer(dwSize + 1);
            DWORD dwDownloaded = 0;
            if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
                buffer[dwDownloaded] = 0;
                response += buffer.data();
            }
        } while (dwSize > 0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return response;
}

std::wstring ExtractResponse(const std::string& json) {
    size_t pos = json.find("\"response\":\"");
    if (pos == std::string::npos) {
        pos = json.find("\"content\":\"");
        if (pos == std::string::npos) return L"No response from model. Is Ollama running?";
        pos += 11;
    } else {
        pos += 12;
    }
    size_t end = pos;
    while (end < json.size()) {
        if (json[end] == '"' && json[end - 1] != '\\') break;
        end++;
    }
    std::string text = json.substr(pos, end - pos);
    size_t p = 0;
    while ((p = text.find("\\n", p)) != std::string::npos) {
        text.replace(p, 2, "\n");
        p += 1;
    }
    p = 0;
    while ((p = text.find("\\\"", p)) != std::string::npos) {
        text.replace(p, 2, "\"");
        p += 1;
    }
    return Utf8ToWide(text);
}

void AppendOutput(const std::wstring& text) {
    int len = GetWindowTextLengthW(hOutput);
    SendMessageW(hOutput, EM_SETSEL, len, len);
    SendMessageW(hOutput, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
    SendMessageW(hOutput, EM_SCROLLCARET, 0, 0);
}

void SetStatus(const std::wstring& text) {
    SetWindowTextW(hStatus, text.c_str());
}

void LoadModels() {
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE hRead, hWrite;
    CreatePipe(&hRead, &hWrite, &sa, 0);
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    PROCESS_INFORMATION pi = {};

    wchar_t cmd[] = L"ollama list";
    if (CreateProcessW(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hWrite);
        WaitForSingleObject(pi.hProcess, 3000);
        char buffer[4096] = {};
        DWORD read = 0;
        ReadFile(hRead, buffer, sizeof(buffer) - 1, &read, NULL);
        CloseHandle(hRead);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        std::string out(buffer, read);
        std::istringstream iss(out);
        std::string line;
        bool first = true;
        SendMessageW(hModel, CB_RESETCONTENT, 0, 0);
        while (std::getline(iss, line)) {
            if (first) { first = false; continue; }
            if (line.empty()) continue;
            size_t sp = line.find(' ');
            if (sp == std::string::npos) sp = line.find('\t');
            std::string name = (sp != std::string::npos) ? line.substr(0, sp) : line;
            if (!name.empty()) {
                std::wstring wname = Utf8ToWide(name);
                SendMessageW(hModel, CB_ADDSTRING, 0, (LPARAM)wname.c_str());
            }
        }
        if (SendMessageW(hModel, CB_GETCOUNT, 0, 0) > 0) {
            SendMessageW(hModel, CB_SETCURSEL, 0, 0);
            wchar_t buf[128] = {};
            SendMessageW(hModel, CB_GETLBTEXT, 0, (LPARAM)buf);
            currentModel = buf;
            SetStatus(L"Models loaded from Ollama");
        } else {
            SendMessageW(hModel, CB_ADDSTRING, 0, (LPARAM)L"llama3.2");
            SendMessageW(hModel, CB_SETCURSEL, 0, 0);
            SetStatus(L"Ollama not found or no models. Using default.");
        }
    } else {
        CloseHandle(hWrite);
        CloseHandle(hRead);
        SendMessageW(hModel, CB_ADDSTRING, 0, (LPARAM)L"llama3.2");
        SendMessageW(hModel, CB_SETCURSEL, 0, 0);
        SetStatus(L"Ollama not in PATH. Start Ollama and ensure models installed.");
    }
}

void SendPrompt() {
    wchar_t inputBuf[4096] = {};
    GetWindowTextW(hInput, inputBuf, 4096);
    std::wstring prompt = inputBuf;
    if (prompt.empty()) return;

    wchar_t modelBuf[128] = {};
    int sel = (int)SendMessageW(hModel, CB_GETCURSEL, 0, 0);
    if (sel != CB_ERR) {
        SendMessageW(hModel, CB_GETLBTEXT, sel, (LPARAM)modelBuf);
        currentModel = modelBuf;
    }

    AppendOutput(L"\r\nYou: " + prompt + L"\r\n");
    SetWindowTextW(hInput, L"");
    SetStatus(L"Thinking...");
    EnableWindow(hSend, FALSE);

    std::string modelUtf = WideToUtf8(currentModel);
    std::string promptUtf = WideToUtf8(prompt);
    auto escape = [](std::string s) {
        size_t p = 0;
        while ((p = s.find('"', p)) != std::string::npos) {
            s.replace(p, 1, "\\\"");
            p += 2;
        }
        return s;
    };
    std::string body = "{\"model\":\"" + escape(modelUtf) + "\",\"prompt\":\"" + escape(promptUtf) + "\",\"stream\":false}";

    std::string resp = HttpPost(L"localhost", 11434, L"/api/generate", body);
    std::wstring answer = ExtractResponse(resp);
    AppendOutput(L"Agent: " + answer + L"\r\n");
    SetStatus(L"Ready");
    EnableWindow(hSend, TRUE);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"SimpleAIAgentClass";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, L"SimpleAIAgentClass", L"Simple AI Agent",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 720, 520,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"Local AI Agent (Ollama)", WS_CHILD | WS_VISIBLE,
            20, 15, 300, 20, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"STATIC", L"Model:", WS_CHILD | WS_VISIBLE,
            20, 45, 50, 20, hwnd, NULL, NULL, NULL);
        hModel = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            80, 42, 200, 200, hwnd, (HMENU)ID_MODEL, NULL, NULL);

        hOutput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            20, 80, 660, 280, hwnd, (HMENU)ID_OUTPUT, NULL, NULL);

        hInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            20, 380, 540, 30, hwnd, (HMENU)ID_INPUT, NULL, NULL);

        hSend = CreateWindowW(L"BUTTON", L"Send", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            580, 378, 100, 34, hwnd, (HMENU)ID_SEND, NULL, NULL);

        hStatus = CreateWindowW(L"STATIC", L"Starting...", WS_CHILD | WS_VISIBLE,
            20, 430, 660, 20, hwnd, (HMENU)ID_STATUS, NULL, NULL);

        HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        SendMessageW(hOutput, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(hInput, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(hSend, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(hModel, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(hStatus, WM_SETFONT, (WPARAM)hFont, TRUE);

        LoadModels();
        AppendOutput(L"Welcome. Type a message and press Send.\r\nUses models already installed in Ollama.\r\n");
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_SEND) {
            SendPrompt();
        }
        break;
    case WM_KEYDOWN:
        if (wParam == VK_RETURN && GetFocus() == hInput) {
            SendPrompt();
            return 0;
        }
        break;
    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        if (hOutput) MoveWindow(hOutput, 20, 80, w - 40, h - 200, TRUE);
        if (hInput) MoveWindow(hInput, 20, h - 100, w - 160, 30, TRUE);
        if (hSend) MoveWindow(hSend, w - 120, h - 102, 100, 34, TRUE);
        if (hStatus) MoveWindow(hStatus, 20, h - 50, w - 40, 20, TRUE);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}
