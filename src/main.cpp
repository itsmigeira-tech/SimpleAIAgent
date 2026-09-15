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
#define ID_LABEL 106

HWND hInput, hOutput, hSend, hModel, hLabel;
HBRUSH hBrushWindow = NULL;

struct Endpoint {
    const char* id;
    const wchar_t* host;
    INTERNET_PORT port;
    const wchar_t* path;
};

struct ModelChoice {
    const wchar_t* display;
    int preferred[8];
    int preferredCount;
};

Endpoint g_endpoints[] = {
    { "gpt-oss:20b", L"api.llm7.io", 443, L"/v1/chat/completions" },
    { "default", L"api.llm7.io", 443, L"/v1/chat/completions" },
    { "fast", L"api.llm7.io", 443, L"/v1/chat/completions" },
    { "mistral-Nemo-Instruct-2407", L"api.llm7.io", 443, L"/v1/chat/completions" },
    { "kilo-auto/free", L"api.kilo.ai", 443, L"/api/gateway/chat/completions" },
    { "Qwen3-Coder-30B-A3B-Instruct", L"oai.endpoints.kepler.ai.cloud.ovh.net", 443, L"/v1/chat/completions" },
    { "Qwen3-32B", L"oai.endpoints.kepler.ai.cloud.ovh.net", 443, L"/v1/chat/completions" },
    { "Qwen3.6-27B", L"oai.endpoints.kepler.ai.cloud.ovh.net", 443, L"/v1/chat/completions" },
    { "DeepSeek-R1-Distill-Llama-70B", L"oai.endpoints.kepler.ai.cloud.ovh.net", 443, L"/v1/chat/completions" },
    { "gpt-oss-20b", L"oai.endpoints.kepler.ai.cloud.ovh.net", 443, L"/v1/chat/completions" },
    { "Meta-Llama-3_3-70B-Instruct", L"oai.endpoints.kepler.ai.cloud.ovh.net", 443, L"/v1/chat/completions" },
    { "openai", L"text.pollinations.ai", 443, L"/openai" },
};
const int g_endpointCount = sizeof(g_endpoints) / sizeof(g_endpoints[0]);

ModelChoice g_models[] = {
    { L"GPT-OSS", { 0, 9, 1, 4, 11 }, 5 },
    { L"Qwen Coder", { 5, 1, 0, 4, 11 }, 5 },
    { L"DeepSeek", { 8, 1, 0, 4, 11 }, 5 },
    { L"Qwen", { 6, 7, 1, 0, 4, 11 }, 6 },
};
const int g_modelCount = sizeof(g_models) / sizeof(g_models[0]);

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

std::string HttpPostHttps(const std::wstring& host, INTERNET_PORT port, const std::wstring& path, const std::string& body) {
    HINTERNET hSession = WinHttpOpen(L"SimpleAIAgent/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "";
    WinHttpSetTimeouts(hSession, 5000, 5000, 15000, 20000);

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

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

bool LooksLikeFail(const std::string& json) {
    if (json.empty()) return true;
    if (json.find("429") != std::string::npos) return true;
    if (json.find("rate limit") != std::string::npos) return true;
    if (json.find("Rate limit") != std::string::npos) return true;
    if (json.find("too many requests") != std::string::npos) return true;
    if (json.find("\"error\"") != std::string::npos && json.find("\"content\"") == std::string::npos) return true;
    return false;
}

std::wstring StripEmojis(const std::wstring& in) {
    std::wstring out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ) {
        wchar_t c = in[i];
        if (c >= 0xD800 && c <= 0xDBFF && i + 1 < in.size()) {
            wchar_t c2 = in[i + 1];
            if (c2 >= 0xDC00 && c2 <= 0xDFFF) { i += 2; continue; }
        }
        if ((c >= 0x2600 && c <= 0x27BF) || (c >= 0x2300 && c <= 0x23FF) ||
            (c >= 0x2B00 && c <= 0x2BFF) || (c >= 0xFE00 && c <= 0xFE0F) ||
            c == 0x200D || c == 0xFE0F) { i++; continue; }
        out.push_back(c);
        i++;
    }
    return out;
}

void ReplaceAll(std::wstring& s, const std::wstring& from, const std::wstring& to) {
    size_t p = 0;
    while ((p = s.find(from, p)) != std::wstring::npos) {
        s.replace(p, from.size(), to);
        p += to.size();
    }
}

std::wstring CleanReply(std::wstring s) {
    // Remove think / reasoning tags and leftovers
    ReplaceAll(s, L"</think>", L"");
    ReplaceAll(s, L"<think>", L"");
    ReplaceAll(s, L"</redacted_thinking>", L"");
    ReplaceAll(s, L"<redacted_thinking>", L"");
    ReplaceAll(s, L"</reasoning>", L"");
    ReplaceAll(s, L"<reasoning>", L"");

    // If multiple answers were glued, keep the last non-empty segment
    // Split on repeated greeting patterns by taking text after last tag residue
    size_t lastBreak = s.find_last_of(L"\n");
    // Prefer the final paragraph if the model dumped several replies
    std::vector<std::wstring> parts;
    std::wstring cur;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == L'\n') {
            // keep newlines inside one block
            cur.push_back(L'\n');
        } else {
            cur.push_back(s[i]);
        }
    }
    // Collapse runs of blank lines
    std::wstring out;
    bool prevBlank = false;
    for (size_t i = 0; i < s.size(); i++) {
        wchar_t c = s[i];
        if (c == L'\r') continue;
        if (c == L'\n') {
            if (!prevBlank) out.push_back(c);
            prevBlank = true;
        } else {
            out.push_back(c);
            prevBlank = false;
        }
    }
    s = out;

    // Trim
    while (!s.empty() && (s.front() == L' ' || s.front() == L'\n' || s.front() == L'\t')) s.erase(s.begin());
    while (!s.empty() && (s.back() == L' ' || s.back() == L'\n' || s.back() == L'\t')) s.pop_back();

    return StripEmojis(s);
}

std::wstring ExtractChatContent(const std::string& json) {
    size_t keyPos = std::string::npos;
    for (size_t i = 0; i + 9 < json.size(); i++) {
        if (json[i] == '"' &&
            json[i + 1] == 'c' && json[i + 2] == 'o' && json[i + 3] == 'n' &&
            json[i + 4] == 't' && json[i + 5] == 'e' && json[i + 6] == 'n' &&
            json[i + 7] == 't' && json[i + 8] == '"') {
            keyPos = i;
        }
    }
    if (keyPos == std::string::npos) return L"";

    size_t pos = keyPos + 9;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos >= json.size() || json[pos] != ':') return L"";
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) pos++;
    if (pos >= json.size() || json[pos] != '"') return L"";
    pos++;

    std::string text;
    for (size_t i = pos; i < json.size(); i++) {
        if (json[i] == '\\' && i + 1 < json.size()) {
            char n = json[i + 1];
            if (n == 'n') { text.push_back('\n'); i++; continue; }
            if (n == 'r') { text.push_back('\r'); i++; continue; }
            if (n == 't') { text.push_back('\t'); i++; continue; }
            if (n == '"') { text.push_back('"'); i++; continue; }
            if (n == '\\') { text.push_back('\\'); i++; continue; }
            if (n == '/') { text.push_back('/'); i++; continue; }
            text.push_back(json[i]);
            continue;
        }
        if (json[i] == '"') break;
        text.push_back(json[i]);
    }

    return CleanReply(Utf8ToWide(text));
}

void AppendOutput(const std::wstring& text) {
    int len = GetWindowTextLengthW(hOutput);
    SendMessageW(hOutput, EM_SETSEL, len, len);
    SendMessageW(hOutput, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
    SendMessageW(hOutput, EM_SCROLLCARET, 0, 0);
}

void LoadModels() {
    SendMessageW(hModel, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i < g_modelCount; i++) {
        SendMessageW(hModel, CB_ADDSTRING, 0, (LPARAM)g_models[i].display);
    }
    SendMessageW(hModel, CB_SETCURSEL, 0, 0);
}

std::string BuildBody(const char* modelId, const std::string& promptUtf, const std::string& modelLabel) {
    auto escape = [](std::string s) {
        size_t p = 0;
        while ((p = s.find('\\', p)) != std::string::npos) { s.replace(p, 1, "\\\\"); p += 2; }
        p = 0;
        while ((p = s.find('"', p)) != std::string::npos) { s.replace(p, 1, "\\\""); p += 2; }
        p = 0;
        while ((p = s.find('\n', p)) != std::string::npos) { s.replace(p, 1, "\\n"); p += 2; }
        return s;
    };

    std::string system =
        "You are a helpful assistant in Simple AI Agent. "
        "Reply in plain text only. Never use emojis. "
        "Never output tags like </think> or <think>. "
        "Give one short answer only. Do not repeat yourself. "
        "For simple math, answer with the number and one short sentence. "
        "If asked what model you are, answer with: " + modelLabel + ".";

    return std::string("{\"model\":\"") + modelId +
        "\",\"messages\":["
        "{\"role\":\"system\",\"content\":\"" + escape(system) + "\"},"
        "{\"role\":\"user\",\"content\":\"" + escape(promptUtf) + "\"}"
        "],\"stream\":false}";
}

void SendPrompt() {
    wchar_t inputBuf[4096] = {};
    GetWindowTextW(hInput, inputBuf, 4096);
    std::wstring prompt = inputBuf;
    if (prompt.empty()) return;

    int sel = (int)SendMessageW(hModel, CB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= g_modelCount) sel = 0;

    AppendOutput(L"\r\nYou: " + prompt + L"\r\n");
    SetWindowTextW(hInput, L"");
    EnableWindow(hSend, FALSE);

    std::string promptUtf = WideToUtf8(prompt);
    std::string modelLabel = WideToUtf8(g_models[sel].display);
    std::wstring answer;

    bool used[32] = {};
    int order[32];
    int orderCount = 0;

    ModelChoice& choice = g_models[sel];
    for (int i = 0; i < choice.preferredCount; i++) {
        int idx = choice.preferred[i];
        if (idx >= 0 && idx < g_endpointCount && !used[idx]) {
            used[idx] = true;
            order[orderCount++] = idx;
        }
    }
    for (int i = 0; i < g_endpointCount; i++) {
        if (!used[i]) order[orderCount++] = i;
    }

    for (int pass = 0; pass < 2 && answer.empty(); pass++) {
        if (pass > 0) Sleep(1200);
        for (int i = 0; i < orderCount; i++) {
            Endpoint& ep = g_endpoints[order[i]];
            std::string body = BuildBody(ep.id, promptUtf, modelLabel);
            std::string resp = HttpPostHttps(ep.host, ep.port, ep.path, body);
            if (LooksLikeFail(resp)) continue;
            answer = ExtractChatContent(resp);
            if (!answer.empty()) break;
        }
    }

    if (answer.empty()) answer = L"All backends busy. Wait 20s and send again.";

    AppendOutput(L"Agent: " + answer + L"\r\n");
    EnableWindow(hSend, TRUE);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icc);

    hBrushWindow = CreateSolidBrush(GetSysColor(COLOR_WINDOW));

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = hBrushWindow;
    wc.lpszClassName = L"SimpleAIAgentClass";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, L"SimpleAIAgentClass", L"Simple AI Agent",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 720, 500,
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
        hLabel = CreateWindowW(L"STATIC", L"Model:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 18, 50, 20, hwnd, (HMENU)ID_LABEL, NULL, NULL);

        hModel = CreateWindowW(L"COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            75, 14, 180, 200, hwnd, (HMENU)ID_MODEL, NULL, NULL);

        // No client edge border on chat area
        hOutput = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_BORDER,
            20, 48, 660, 310, hwnd, (HMENU)ID_OUTPUT, NULL, NULL);

        hInput = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_BORDER,
            20, 370, 540, 28, hwnd, (HMENU)ID_INPUT, NULL, NULL);

        hSend = CreateWindowW(L"BUTTON", L"Send",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            580, 368, 100, 32, hwnd, (HMENU)ID_SEND, NULL, NULL);

        HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        SendMessageW(hOutput, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(hInput, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(hSend, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(hModel, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

        LoadModels();
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        return (LRESULT)hBrushWindow;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_SEND) SendPrompt();
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
        if (hOutput) MoveWindow(hOutput, 20, 48, w - 40, h - 120, TRUE);
        if (hInput) MoveWindow(hInput, 20, h - 60, w - 160, 28, TRUE);
        if (hSend) MoveWindow(hSend, w - 120, h - 62, 100, 32, TRUE);
        break;
    }
    case WM_DESTROY:
        if (hBrushWindow) DeleteObject(hBrushWindow);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}
