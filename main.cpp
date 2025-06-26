//too many freaking includes
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "libxml2.lib")

struct Post {
    std::wstring date;
    std::wstring title;
    std::wstring url;
};
static std::vector<Post> posts;
static std::mutex postsMutex;
static HWND g_hWnd = nullptr;
static NOTIFYICONDATAW nid = {};
static bool isVisible = true;
static HRGN windowRegion = nullptr;
static int hoveredIndex = -1;
static const wchar_t* INI_PATH = L".\\widget.ini";
static const UINT WM_TRAYICON = WM_USER + 1;
static const UINT ID_TRAY_EXIT = 1001;
static const UINT ID_TRAY_TOGGLE = 1002;
static const UINT ID_TRAY_REFRESH = 1003;
std::wstring stringToWstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &result[0], size);
    return result;
}
std::string get_request(const std::string& url) {                                          //curl req
    CURL *curl = curl_easy_init();
    std::string result;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char *ptr, size_t size, size_t nmemb, void *userdata) -> size_t {
            std::string* response = static_cast<std::string*>(userdata);
            response->append(ptr, size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        CURLcode res = curl_easy_perform(curl);
       // if (res != CURLE_OK) {
         //   std::cerr << "curl_easy_perform failed: " << curl_easy_strerror(res) << std::endl;         //debug-shit
       // }
        curl_easy_cleanup(curl);
    }
    return result;
}
xmlXPathObjectPtr get_nodes(xmlDocPtr doc, const xmlChar* xpath) {                  //parsing
    xmlXPathContextPtr context = xmlXPathNewContext(doc);
   // if (!context) {
      //  std::cerr << "Failed to create XPath context" << std::endl;                                    //debug-shit
    //    return nullptr;
  //  }
    xmlXPathObjectPtr result = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);
    return result;
}

void updatePosts() {                                                                //scraping
    std::vector<Post> newPosts;
    try {
        std::string docStr = get_request("https://opensource.googleblog.com/search/label/gsoc");   
        htmlDocPtr htmldoc = htmlReadMemory(docStr.c_str(), docStr.size(), nullptr, nullptr, 
            HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
        if (!htmldoc) return;
        xmlXPathObjectPtr dateNodes = get_nodes(htmldoc, (const xmlChar*)"//h4[@class='date-header']/span");
        xmlXPathObjectPtr postNodes = get_nodes(htmldoc, (const xmlChar*)"//h3[@class='post-title entry-title']/a");
        if (dateNodes && postNodes) {
            int postcount = postNodes->nodesetval->nodeNr;
            int datecount = dateNodes->nodesetval->nodeNr;
           for (int i = 0; i < (postcount < 8 ? postcount : 8); i++) {
                xmlNodePtr postNode = postNodes->nodesetval->nodeTab[i];
                std::string title = (const char*)xmlNodeGetContent(postNode);
                std::string url = (const char*)xmlGetProp(postNode, (const xmlChar*)"href");
                std::string date = "";
                if (i < datecount) {
                    xmlNodePtr dateNode = dateNodes->nodesetval->nodeTab[i];
                    date = (const char*)xmlNodeGetContent(dateNode);
                }
                newPosts.push_back({
                    stringToWstring(date),
                    stringToWstring(title),
                    stringToWstring(url)
                });
            }
        }
        if (dateNodes) xmlXPathFreeObject(dateNodes);
        if (postNodes) xmlXPathFreeObject(postNodes);
        xmlFreeDoc(htmldoc);
    }
    catch (...) {
                                                                                         // Handling
    }

                                                                                        // Update 
    {
        std::lock_guard<std::mutex> lock(postsMutex);
        posts = std::move(newPosts);
    }
    if (g_hWnd && isVisible) 
    {
        int height = (int)posts.size() * 40 + 60; 
        RECT r;
        GetWindowRect(g_hWnd, &r);
        SetWindowPos(g_hWnd, nullptr, 0, 0, 450, height, SWP_NOMOVE | SWP_NOZORDER);
        if (windowRegion) DeleteObject(windowRegion);
        windowRegion = CreateRoundRectRgn(0, 0, 450, height, 20, 20);
        SetWindowRgn(g_hWnd, windowRegion, TRUE);
        InvalidateRect(g_hWnd, nullptr, TRUE);
    }
}
void refreshPostsAsync() {
    std::thread([]() {
        updatePosts();
    }).detach();
}
// System tray
void createTrayIcon(HWND hWnd) {
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"GSoC Widget");
    Shell_NotifyIconW(NIM_ADD, &nid);
}
void showTrayMenu(HWND hWnd) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_TOGGLE, isVisible ? L"Hide Widget" : L"Show Widget");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_REFRESH, L"Refresh Posts");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);
}
POINT LoadPosition() {
    wchar_t buf[32] = {};
    GetPrivateProfileStringW(L"Widget", L"Position", L"50,50", buf, _countof(buf), INI_PATH);
    POINT p{50,50};
    swscanf_s(buf, L"%d,%d", &p.x, &p.y);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    if (p.x < 0 || p.x > screenWidth - 100) p.x = 50;
    if (p.y < 0 || p.y > screenHeight - 100) p.y = 50;
    return p;
}
void SavePosition(POINT p) {
    wchar_t buf[32];
    swprintf_s(buf, L"%d,%d", p.x, p.y);
    WritePrivateProfileStringW(L"Widget", L"Position", buf, INI_PATH);
}
void AddToStartup() {
    wchar_t done[4] = {};
    GetPrivateProfileStringW(L"Widget", L"AutoStart", L"0", done, _countof(done), INI_PATH);
    if (done[0] == L'1') return;
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, _countof(path));
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"GSoCWidget", 0, REG_SZ,
            (BYTE*)path, (DWORD)((wcslen(path) + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
        WritePrivateProfileStringW(L"Widget", L"AutoStart", L"1", INI_PATH);
    }
}
HWND GetDesktopWorkerW() {
    HWND progman = FindWindowW(L"Progman", nullptr);
    if (!progman) return nullptr;
    SendMessageTimeoutW(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, nullptr);
    HWND worker = nullptr;
    EnumWindows([](HWND top, LPARAM lParam)->BOOL {
        HWND def = FindWindowExW(top, nullptr, L"SHELLDLL_DefView", nullptr);
        if (def) {
            HWND workerW = FindWindowExW(nullptr, top, L"WorkerW", nullptr);
            if (workerW) {
                *reinterpret_cast<HWND*>(lParam) = workerW;
                return FALSE;
            }
        }
        return TRUE;
    }, (LPARAM)&worker);
    
    return worker;
}

void DrawRoundedRect(HDC hdc, RECT rect, int radius, COLORREF fillColor, COLORREF borderColor = RGB(40, 40, 40)) {
    HRGN region = CreateRoundRectRgn(rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectClipRgn(hdc, region);
    HBRUSH brush = CreateSolidBrush(fillColor);
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
    HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    RoundRect(hdc, rect.left, rect.top, rect.right-1, rect.bottom-1, radius, radius);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    SelectClipRgn(hdc, nullptr);
    DeleteObject(region);
}
int GetPostIndexAtPoint(POINT pt, const RECT& clientRect) {
    const int lineH = 35;
    const int startY = 45;   
    if (pt.y < startY) return -1;
    int idx = (pt.y - startY) / lineH;
    std::lock_guard<std::mutex> lock(postsMutex);
    if (idx >= 0 && idx < (int)posts.size()) {
        return idx;
    }
    return -1;
}
void SetDPIAwareness() {//new stuff i learned, basically your gui could be blurry because of dpi, and this fixes it well
    typedef HRESULT(WINAPI* SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
    HMODULE user32 = GetModuleHandle(L"user32.dll");
    if (user32) {
        SetProcessDpiAwarenessContextProc setDpiAwarenessContext = 
            (SetProcessDpiAwarenessContextProc)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
        if (setDpiAwarenessContext) {
            setDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            return;
        }
    }
    // Fallback to older API version(8.1+ afaik)
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
}
float GetDPIScale(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hwnd, hdc);
    return dpiX / 96.0f;
}
HFONT CreateScaledFont(const wchar_t* fontName, int baseSize, int weight, HWND hwnd) {
    float dpiScale = GetDPIScale(hwnd);
    int scaledSize = (int)(baseSize * dpiScale);
    
    return CreateFontW(
        -scaledSize,
        0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, 
        fontName
    );
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l) {
    static bool tracking = false;
    static POINT lastPoint = {0, 0};
    switch (msg) {
    case WM_CREATE: {
        g_hWnd = hWnd;
        createTrayIcon(hWnd);
        LONG exStyle = GetWindowLongW(hWnd, GWL_EXSTYLE);
        SetWindowLongW(hWnd, GWL_EXSTYLE, 
            exStyle | WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW);
        SetLayeredWindowAttributes(hWnd, 0, (BYTE)240, LWA_ALPHA);
        windowRegion = CreateRoundRectRgn(0, 0, 450, 210, 20, 20);
        SetWindowRgn(hWnd, windowRegion, TRUE);
        refreshPostsAsync();
        SetTimer(hWnd, 1, 30 * 60 * 1000, nullptr);
        
        InvalidateRect(hWnd, nullptr, TRUE);
        return 0;
    }
    case WM_TIMER:
        if (w == 1) {
            refreshPostsAsync();
        }
        return 0;
    case WM_TRAYICON:
        switch (l) {
        case WM_RBUTTONUP:
            showTrayMenu(hWnd);
            break;
        case WM_LBUTTONDBLCLK:
            isVisible = !isVisible;
            ShowWindow(hWnd, isVisible ? SW_SHOW : SW_HIDE);
            break;
        }
        return 0;
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case ID_TRAY_EXIT:
            PostQuitMessage(0);
            break;
        case ID_TRAY_TOGGLE:
            isVisible = !isVisible;
            ShowWindow(hWnd, isVisible ? SW_SHOW : SW_HIDE);
            break;
        case ID_TRAY_REFRESH:
            refreshPostsAsync();
            break;
        }
        return 0;
    case WM_LBUTTONDOWN: {
        POINT pt = { GET_X_LPARAM(l), GET_Y_LPARAM(l) };
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        
        int postIndex = GetPostIndexAtPoint(pt, clientRect);
        if (postIndex >= 0) {
            // hyperlink title
            std::lock_guard<std::mutex> lock(postsMutex);
            if (postIndex < (int)posts.size()) {
                ShellExecuteW(nullptr, L"open", posts[postIndex].url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            }
        } else {
            // empty area click = drag mode on
            SetCapture(hWnd);
            tracking = true;
            lastPoint.x = GET_X_LPARAM(l);
            lastPoint.y = GET_Y_LPARAM(l);
        }
        return 0;
    }
    case WM_MOUSEMOVE: {
        POINT pt = { GET_X_LPARAM(l), GET_Y_LPARAM(l) };
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        
        if (tracking) {
            // dragging
            POINT currentPoint;
            currentPoint.x = GET_X_LPARAM(l);
            currentPoint.y = GET_Y_LPARAM(l);
            
            int dx = currentPoint.x - lastPoint.x;
            int dy = currentPoint.y - lastPoint.y;
            
            RECT r;
            GetWindowRect(hWnd, &r);
            SetWindowPos(hWnd, nullptr, 
                r.left + dx, r.top + dy, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER);
        } else {
            // hover effects
            int newHoveredIndex = GetPostIndexAtPoint(pt, clientRect);
            if (newHoveredIndex != hoveredIndex) {
                hoveredIndex = newHoveredIndex;
                InvalidateRect(hWnd, nullptr, TRUE);
                SetCursor(LoadCursor(nullptr, hoveredIndex >= 0 ? IDC_HAND : IDC_ARROW));
            }
        }
        return 0;
    }

    case WM_LBUTTONUP:
        if (tracking) {
            ReleaseCapture();
            tracking = false;
            RECT r; 
            GetWindowRect(hWnd, &r);
            SavePosition({ r.left, r.top });
        }
        return 0;

    case WM_MOUSELEAVE:
        hoveredIndex = -1;
        InvalidateRect(hWnd, nullptr, TRUE);
        return 0;

    case WM_RBUTTONUP:
        showTrayMenu(hWnd);
        return 0;

case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hWnd, &ps);
    RECT r; 
    GetClientRect(hWnd, &r);
    HDC memDC = CreateCompatibleDC(dc);
    HBITMAP memBitmap = CreateCompatibleBitmap(dc, r.right, r.bottom);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
    SetBkMode(memDC, TRANSPARENT);
    SetTextAlign(memDC, TA_LEFT | TA_TOP);
    DrawRoundedRect(memDC, r, 20, RGB(18, 18, 18), RGB(45, 45, 45));
    RECT headerRect = {10, 10, r.right - 10, 35};
    DrawRoundedRect(memDC, headerRect, 8, RGB(30, 30, 30), RGB(60, 60, 60));
    HFONT headerFont = CreateScaledFont(L"Segoe UI", 13, FW_SEMIBOLD, hWnd);
    HFONT oldHeaderFont = (HFONT)SelectObject(memDC, headerFont);
    SetTextColor(memDC, RGB(240, 240, 240));
    SIZE headerTextSize;
    GetTextExtentPoint32W(memDC, L"GSoC Updates", 
                         wcslen(L"GSoC Updates"), &headerTextSize);
    int headerX = (headerRect.right - headerRect.left - headerTextSize.cx) / 2 + headerRect.left;
    int headerY = (headerRect.bottom - headerRect.top - headerTextSize.cy) / 2 + headerRect.top;
    TextOutW(memDC, headerX, headerY, L"GSoC Updates", 
             wcslen(L"GSoC Updates"));
    
    SelectObject(memDC, oldHeaderFont);
    DeleteObject(headerFont);
    HFONT postFont = CreateScaledFont(L"Segoe UI", 10, FW_NORMAL, hWnd);
    HFONT postTitleFont = CreateScaledFont(L"Segoe UI", 11, FW_MEDIUM, hWnd);
    
    const int lineH = 40;
    const int startY = 50;
    
    {
        std::lock_guard<std::mutex> lock(postsMutex);
        if (posts.empty()) {
            HFONT oldFont = (HFONT)SelectObject(memDC, postFont);
            SetTextColor(memDC, RGB(160, 160, 160));
            TextOutW(memDC, 20, startY, L"Loading GSoC posts...", 
                    wcslen(L"Loading GSoC posts..."));
            SelectObject(memDC, oldFont);
        } else {
            for (int i = 0; i < (int)posts.size(); i++) {
    int yPos = startY + i * lineH;
    
    //hover effect
    RECT postRect = {12, yPos - 3, r.right - 12, yPos + lineH - 5};
    COLORREF bgColor = (i == hoveredIndex) ? RGB(40, 40, 45) : RGB(25, 25, 28);
    COLORREF borderColor = (i == hoveredIndex) ? RGB(80, 140, 255) : RGB(45, 45, 50);
    DrawRoundedRect(memDC, postRect, 6, bgColor, borderColor);
    
    // Date
    if (!posts[i].date.empty()) {
        HFONT oldFont = (HFONT)SelectObject(memDC, postFont);
        SetTextColor(memDC, RGB(120, 170, 255));
        TextOutW(memDC, 22, yPos + 3,
                 posts[i].date.c_str(),
                 (int)posts[i].date.length());
        SelectObject(memDC, oldFont);
    }

    // Title/description of blog
    HFONT oldTitleFont = (HFONT)SelectObject(memDC, postTitleFont);
    COLORREF titleColor = (i == hoveredIndex) ? RGB(255, 255, 255) : RGB(235, 235, 235);
SetTextColor(memDC, titleColor);
TEXTMETRIC tm;
GetTextMetrics(memDC, &tm);
int textHeight = tm.tmHeight;

RECT titleRect = {
    postRect.left + 10,           
    yPos + 18,                  
    postRect.right - 10,          
    yPos + 18 + textHeight + 4   
};

DrawTextW(
    memDC,
    posts[i].title.c_str(),
    -1,
    &titleRect,
    DT_SINGLELINE |        
    DT_VCENTER |             
    DT_END_ELLIPSIS |       
    DT_NOPREFIX             
);

SelectObject(memDC, oldTitleFont);

        }
    }
}
    BitBlt(dc, 0, 0, r.right, r.bottom, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
    DeleteObject(postFont);
    DeleteObject(postTitleFont);
    EndPaint(hWnd, &ps);
    return 0;
}
    case WM_DESTROY:
        Shell_NotifyIconW(NIM_DELETE, &nid);
        if (windowRegion) DeleteObject(windowRegion);
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, w, l);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
    SetDPIAwareness();
    curl_global_init(CURL_GLOBAL_ALL);
    
    POINT pos = LoadPosition();
    int width = 150, height = 50; 
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"GSoCWidget";
    wc.hbrBackground = nullptr;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);
    HWND parent = GetDesktopWorkerW();
    DWORD style = parent ? (WS_CHILD | WS_VISIBLE) : (WS_POPUP | WS_VISIBLE);
    HWND wnd = CreateWindowExW(
        0, wc.lpszClassName, L"GSoC Widget",
        style,
        pos.x, pos.y, width, height,
        parent, nullptr, hInst, nullptr);
    if (!wnd) {
        MessageBoxW(nullptr, L"Failed to create window!", L"Error", MB_OK | MB_ICONERROR);
        curl_global_cleanup();
        return 1;
    }
    ShowWindow(wnd, nCmdShow);
    UpdateWindow(wnd);
    AddToStartup();
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    curl_global_cleanup();
    return 0;
}