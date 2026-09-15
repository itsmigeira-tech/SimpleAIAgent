#include <windows.h>
#include <windowsx.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <commctrl.h>
#include <cctype>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

#ifndef EM_SETCUEBANNER
#define EM_SETCUEBANNER 0x1501
#endif

#define ID_INPUT 101
#define ID_OUTPUT 102
#define ID_SEND 103
#define ID_MODEL 104
#define ID_LABEL 106

HWND hInput, hOutput, hSend, hModel, hLabel;
HBRUSH hBrushWindow = NULL;
HBRUSH hBrushEdit = NULL;
HFONT hFontUI = NULL;
HFONT hFontCode = NULL;

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
LRESULT CALLBACK RoundEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR);

void ApplyRoundRegion(HWND hwnd, int radius) {
    RECT rc;
    GetWindowRect(hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return;
    HRGN rgn = CreateRoundRectRgn(0, 0, w + 1, h + 1, radius, radius);
    SetWindowRgn(hwnd, rgn, TRUE);
}

void DrawVerticalGradient(HDC hdc, RECT rc, COLORREF top, COLORREF bottom) {
    TRIVERTEX vertex[2];
    vertex[0].x = rc.left;
    vertex[0].y = rc.top;
    vertex[0].Red = (COLOR16)(GetRValue(top) << 8);
    vertex[0].Green = (COLOR16)(GetGValue(top) << 8);
    vertex[0].Blue = (COLOR16)(GetBValue(top) << 8);
    vertex[0].Alpha = 0;

    vertex[1].x = rc.right;
    vertex[1].y = rc.bottom;
    vertex[1].Red = (COLOR16)(GetRValue(bottom) << 8);
    vertex[1].Green = (COLOR16)(GetGValue(bottom) << 8);
    vertex[1].Blue = (COLOR16)(GetBValue(bottom) << 8);
    vertex[1].Alpha = 0;

    GRADIENT_RECT gRect = { 0, 1 };
    GradientFill(hdc, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
}

void DrawRoundedRect(HDC hdc, RECT rc, int radius, COLORREF fill, COLORREF border) {
    HBRUSH br = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldBr = SelectObject(hdc, br);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);
    SelectObject(hdc, oldBr);
    SelectObject(hdc, oldPen);
    DeleteObject(br);
    DeleteObject(pen);
}

void DrawRoundedGradientButton(HDC hdc, RECT rc, bool pressed, bool hot) {
    COLORREF top = hot ? RGB(250, 250, 252) : RGB(248, 248, 250);
    COLORREF bottom = hot ? RGB(232, 232, 236) : RGB(236, 236, 240);
    if (pressed) {
        top = RGB(228, 228, 232);
        bottom = RGB(242, 242, 246);
    }

    HRGN rgn = CreateRoundRectRgn(rc.left, rc.top, rc.right, rc.bottom, 14, 14);
    SelectClipRgn(hdc, rgn);
    DrawVerticalGradient(hdc, rc, top, bottom);
    SelectClipRgn(hdc, NULL);
    DeleteObject(rgn);

    HPEN pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 205));
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBr = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    RoundRect(hdc, rc.left, rc.top, rc.right - 1, rc.bottom - 1, 14, 14);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBr);
    DeleteObject(pen);
}

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

std::string TrimDigits(std::string s) {
    size_t i = 0;
    while (i + 1 < s.size() && s[i] == '0') i++;
    return s.substr(i);
}

std::string AddBig(const std::string& a, const std::string& b) {
    int i = (int)a.size() - 1, j = (int)b.size() - 1, carry = 0;
    std::string r;
    while (i >= 0 || j >= 0 || carry) {
        int sum = carry;
        if (i >= 0) sum += a[i--] - '0';
        if (j >= 0) sum += b[j--] - '0';
        r.push_back(char('0' + (sum % 10)));
        carry = sum / 10;
    }
    std::reverse(r.begin(), r.end());
    return TrimDigits(r);
}

std::string SubBig(std::string a, std::string b) {
    int i = (int)a.size() - 1, j = (int)b.size() - 1, borrow = 0;
    std::string r;
    while (i >= 0) {
        int d = (a[i] - '0') - borrow - (j >= 0 ? (b[j] - '0') : 0);
        if (d < 0) { d += 10; borrow = 1; } else borrow = 0;
        r.push_back(char('0' + d));
        i--; j--;
    }
    std::reverse(r.begin(), r.end());
    return TrimDigits(r);
}

bool GreaterEq(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return a.size() > b.size();
    return a >= b;
}

std::string MulBig(const std::string& a, const std::string& b) {
    if (a == "0" || b == "0") return "0";
    std::vector<int> res(a.size() + b.size(), 0);
    for (int i = (int)a.size() - 1; i >= 0; i--) {
        for (int j = (int)b.size() - 1; j >= 0; j--) {
            int mul = (a[i] - '0') * (b[j] - '0') + res[i + j + 1];
            res[i + j + 1] = mul % 10;
            res[i + j] += mul / 10;
        }
    }
    std::string r;
    for (int d : res) r.push_back(char('0' + d));
    return TrimDigits(r);
}

bool TryLocalMath(const std::wstring& prompt, std::wstring& out) {
    std::string s;
    for (wchar_t c : prompt) {
        if (c >= L'0' && c <= L'9') s.push_back((char)c);
        else if (c == L'+' || c == L'*') s.push_back((char)c);
        else if (c == L'x' || c == L'X') s.push_back('*');
        else if (c == L'-') s.push_back('-');
        else if (c == L' ' || c == L'\t' || c == L'?' || c == L'=' || c == L',') continue;
        else if ((c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z')) continue;
        else return false;
    }
    if (s.empty()) return false;

    char op = 0;
    size_t opPos = std::string::npos;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '+' || s[i] == '*') {
            if (op != 0) return false;
            op = s[i];
            opPos = i;
        } else if (s[i] == '-' && i > 0) {
            if (op != 0) return false;
            op = '-';
            opPos = i;
        }
    }
    if (op == 0 || opPos == 0 || opPos + 1 >= s.size()) return false;

    std::string left = TrimDigits(s.substr(0, opPos));
    std::string right = TrimDigits(s.substr(opPos + 1));
    if (left.empty() || right.empty()) return false;
    for (char c : left) if (c < '0' || c > '9') return false;
    for (char c : right) if (c < '0' || c > '9') return false;

    std::string result;
    if (op == '+') result = AddBig(left, right);
    else if (op == '-') {
        if (GreaterEq(left, right)) result = SubBig(left, right);
        else result = "-" + SubBig(right, left);
    } else if (op == '*') {
        if (left.size() + right.size() > 200) return false;
        result = MulBig(left, right);
    } else return false;

    out = Utf8ToWide(result);
    return true;
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

std::wstring FormatCodeBlocks(std::wstring s) {
    const std::wstring fence = L"```";
    const std::wstring openBar = L"\r\n----- CODE -----\r\n";
    const std::wstring closeBar = L"\r\n----- END CODE -----\r\n";
    size_t pos = 0;
    bool open = true;
    while ((pos = s.find(fence, pos)) != std::wstring::npos) {
        size_t endLine = s.find(L'\n', pos);
        size_t replaceEnd = pos + fence.size();
        if (open) {
            if (endLine != std::wstring::npos && endLine < pos + 20) replaceEnd = endLine + 1;
            s.replace(pos, replaceEnd - pos, openBar);
            pos += openBar.size();
        } else {
            s.replace(pos, fence.size(), closeBar);
            pos += closeBar.size();
        }
        open = !open;
    }
    return s;
}

std::wstring CleanReply(std::wstring s) {
    ReplaceAll(s, L"</think>", L"");
    ReplaceAll(s, L"<think>", L"");
    ReplaceAll(s, L"</redacted_thinking>", L"");
    ReplaceAll(s, L"<redacted_thinking>", L"");
    ReplaceAll(s, L"</reasoning>", L"");
    ReplaceAll(s, L"<reasoning>", L"");

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
    while (!s.empty() && (s.front() == L' ' || s.front() == L'\n' || s.front() == L'\t')) s.erase(s.begin());
    while (!s.empty() && (s.back() == L' ' || s.back() == L'\n' || s.back() == L'\t')) s.pop_back();
    s = FormatCodeBlocks(s);
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
        "Keep answers short and exact. One answer only. "
        "Never invent or pad digits in math. "
        "When you show code, wrap it in markdown fences. "
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

    std::wstring answer;

    if (TryLocalMath(prompt, answer)) {
        AppendOutput(L"Agent: " + answer + L"\r\n");
        EnableWindow(hSend, TRUE);
        InvalidateRect(hSend, NULL, TRUE);
        return;
    }

    std::string promptUtf = WideToUtf8(prompt);
    std::string modelLabel = WideToUtf8(g_models[sel].display);

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
    InvalidateRect(hSend, NULL, TRUE);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icc);

    hBrushWindow = CreateSolidBrush(RGB(245, 245, 247));
    hBrushEdit = CreateSolidBrush(RGB(255, 255, 255));

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
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 740, 540,
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
        hFontUI = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        hFontCode = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            FIXED_PITCH | FF_MODERN, L"Consolas");

        hLabel = CreateWindowW(L"STATIC", L"Model:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            24, 18, 50, 22, hwnd, (HMENU)ID_LABEL, NULL, NULL);

        hModel = CreateWindowW(L"COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            80, 14, 190, 220, hwnd, (HMENU)ID_MODEL, NULL, NULL);

        hOutput = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_CLIPSIBLINGS,
            24, 56, 676, 360, hwnd, (HMENU)ID_OUTPUT, NULL, NULL);

        hInput = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_CLIPSIBLINGS,
            24, 432, 540, 36, hwnd, (HMENU)ID_INPUT, NULL, NULL);

        SendMessageW(hInput, EM_SETCUEBANNER, TRUE, (LPARAM)L"Ask Anything...");

        hSend = CreateWindowW(L"BUTTON", L"Send",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_CLIPSIBLINGS,
            576, 430, 124, 40, hwnd, (HMENU)ID_SEND, NULL, NULL);

        SendMessageW(hOutput, WM_SETFONT, (WPARAM)hFontCode, TRUE);
        SendMessageW(hInput, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(hSend, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(hModel, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(hLabel, WM_SETFONT, (WPARAM)hFontUI, TRUE);

        SetWindowTheme = NULL; // keep native where possible
        (void)0;

        ApplyRoundRegion(hOutput, 16);
        ApplyRoundRegion(hInput, 14);
        ApplyRoundRegion(hSend, 14);

        LoadModels();
        break;
    }
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        // Subtle top-to-bottom gradient, same light family
        DrawVerticalGradient(hdc, rc, RGB(252, 252, 253), RGB(240, 240, 243));
        return 1;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        DrawVerticalGradient(hdc, rc, RGB(252, 252, 253), RGB(240, 240, 243));

        // Soft rounded outline behind chat + input cluster
        if (hOutput && hInput) {
            RECT ro, ri;
            GetWindowRect(hOutput, &ro);
            GetWindowRect(hInput, &ri);
            MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&ro, 2);
            MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&ri, 2);
            RECT cluster = { ro.left - 6, ro.top - 6, ro.right + 6, ri.bottom + 6 };
            DrawRoundedRect(hdc, cluster, 18, RGB(255, 255, 255), RGB(220, 220, 225));
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlID == ID_SEND) {
            bool pressed = (dis->itemState & ODS_SELECTED) != 0;
            bool hot = (dis->itemState & ODS_HOTLIGHT) != 0 || (dis->itemState & ODS_FOCUS) != 0;
            DrawRoundedGradientButton(dis->hDC, dis->rcItem, pressed, hot);

            SetBkMode(dis->hDC, TRANSPARENT);
            SetTextColor(dis->hDC, RGB(40, 40, 45));
            HFONT old = (HFONT)SelectObject(dis->hDC, hFontUI);
            DrawTextW(dis->hDC, L"Send", -1, &dis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dis->hDC, old);
            return TRUE;
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(50, 50, 55));
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, RGB(255, 255, 255));
        SetTextColor(hdc, RGB(30, 30, 35));
        return (LRESULT)hBrushEdit;
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

        int left = 24;
        int top = 56;
        int rightPad = 24;
        int bottomPad = 20;
        int inputH = 36;
        int sendW = 124;
        int sendH = 40;
        int gap = 10;

        int panelW = w - left - rightPad;
        if (panelW < 260) panelW = 260;

        int inputY = h - bottomPad - inputH;
        if (inputY < top + 90) inputY = top + 90;

        int outputH = inputY - gap - top;
        if (outputH < 70) outputH = 70;

        if (hOutput) {
            MoveWindow(hOutput, left, top, panelW, outputH, TRUE);
            ApplyRoundRegion(hOutput, 16);
        }
        if (hInput) {
            MoveWindow(hInput, left, inputY, panelW - sendW - gap, inputH, TRUE);
            ApplyRoundRegion(hInput, 14);
        }
        if (hSend) {
            MoveWindow(hSend, left + panelW - sendW, inputY - 2, sendW, sendH, TRUE);
            ApplyRoundRegion(hSend, 14);
            InvalidateRect(hSend, NULL, TRUE);
        }

        if (hInput) SetWindowPos(hInput, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        if (hSend) SetWindowPos(hSend, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }
    case WM_DESTROY:
        if (hBrushWindow) DeleteObject(hBrushWindow);
        if (hBrushEdit) DeleteObject(hBrushEdit);
        if (hFontUI) DeleteObject(hFontUI);
        if (hFontCode) DeleteObject(hFontCode);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}
