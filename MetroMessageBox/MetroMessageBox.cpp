#include <dwmapi.h>
#include <iostream>
#include <vector>
#include <Windows.h>
#pragma comment(lib, "dwmapi.lib")
using namespace std;
HWND hwndMsg, hwndParent;
DWORD color, lastanswer;
int Create(LPCWSTR text, LPCWSTR title, LPCWSTR b1name = L"", LPCWSTR b2name = L"", LPCWSTR b3name = L"", LPCWSTR b4name = L"", DWORD timeoutMs = 0) {
	auto CalculateTextHeightWithWordWrap = [](HFONT hFont, LPCWSTR text) -> int {
		RECT rect = { 0, 0, 680, 0 };
		HDC hdc = GetDC(GetDesktopWindow());
		if (!hdc) return 0;
		HFONT hOld = (HFONT)SelectObject(hdc, hFont);
		DrawTextW(hdc, text, -1, &rect, DT_CALCRECT | DT_WORDBREAK | DT_LEFT | DT_TOP);
		SelectObject(hdc, hOld);
		ReleaseDC(GetDesktopWindow(), hdc);
		return rect.bottom - rect.top;
		};
	struct ButtonData { bool isHover, isDisabled; };
	typedef struct _RTL_OSVERSIONINFOEXW { ULONG dwOSVersionInfoSize; ULONG dwMajorVersion; ULONG dwMinorVersion; ULONG dwBuildNumber; ULONG dwPlatformId; WCHAR szCSDVersion[128]; USHORT wServicePackMajor; USHORT wServicePackMinor; USHORT wSuiteMask; BYTE wProductType; BYTE wReserved; } RTL_OSVERSIONINFOEXW, * PRTL_OSVERSIONINFOEXW;
	typedef NTSTATUS(WINAPI* PRtlGetVersion)(PRTL_OSVERSIONINFOEXW);
	PRtlGetVersion RtlGetVersion = (PRtlGetVersion)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
	RTL_OSVERSIONINFOEXW osvi = { sizeof(RTL_OSVERSIONINFOEXW) };
	RtlGetVersion(&osvi);
	if (osvi.dwMajorVersion >= 10) {
		BOOL ob;
		DwmGetColorizationColor(&color, &ob);
		if (!color) {
			HKEY hKey;
			DWORD type = REG_DWORD, size = sizeof(color);
			RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\DWM", 0, KEY_READ, &hKey);
			RegQueryValueExW(hKey, L"ColorizationColor", 0, &type, (LPBYTE)&color, &size);
			RegCloseKey(hKey);
			color -= 0x170000;
			if (!color) {
				color = 0x0067b3;
			}
		}
	}
	else {
		color = 0x0067b3;
	}
	WNDCLASSEX wc{};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = [](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
		ButtonData* data;
		BOOL isPressed = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
		if (msg == WM_NCCREATE) {
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
			data = (ButtonData*)LocalAlloc(LPTR, sizeof(ButtonData));
			if (!data) return FALSE;
			data->isHover = false;
			data->isDisabled = false;
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)data);
		}
		data = (ButtonData*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
		switch (msg) {
		case WM_MOUSEMOVE: {
			if (!data->isHover) {
				data->isHover = true;
				TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
				TrackMouseEvent(&tme);
				InvalidateRect(hWnd, NULL, true);
			}
			return 0;
		}
		case WM_MOUSELEAVE: {
			data->isHover = false;
			InvalidateRect(hWnd, 0, true);
			return 0;
		}
		case WM_LBUTTONDOWN: {
			InvalidateRect(hWnd, 0, true);
			return 0;
		}
		case WM_LBUTTONUP: {
			SendMessageW(GetParent(hWnd), WM_COMMAND, GetDlgCtrlID(hWnd), LPARAM(hWnd));
			InvalidateRect(hWnd, 0, true);
			return 0;
		}
		case WM_ENABLE: {
			data->isDisabled = !wParam;
			InvalidateRect(hWnd, 0, true);
			return 0;
		}
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			RECT rc;
			GetClientRect(hWnd, &rc);
			COLORREF bgColor;
			COLORREF textColor;
			if (data->isHover) {
				if (isPressed) {
					bgColor = 0xFFFFFF;
					textColor = 0x000000;
				}
				else {
					bgColor = RGB(GetBValue(color) + 11, GetGValue(color) + 0x27, GetRValue(color) + 0x27);
					textColor = 0xFFFFFF;
				}
			}
			else {
				bgColor = RGB(GetBValue(color), GetGValue(color), GetRValue(color));
				textColor = 0xFFFFFF;
			}
			FillRect(hdc, &rc, CreateSolidBrush(bgColor));
			SelectObject(hdc, CreatePen(PS_SOLID, 3, 0xFFFFFF));
			SelectObject(hdc, GetStockObject(NULL_BRUSH));
			SelectObject(hdc, CreateFontA(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "Segoe UI"));
			Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
			wchar_t text[256];
			GetWindowTextW(hWnd, text, 256);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, textColor);
			DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			EndPaint(hWnd, &ps);
			return 0;
		}
		case WM_NCDESTROY: {
			ButtonData* pdata = (ButtonData*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
			if (pdata) {
				LocalFree(pdata);
			}
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
			return 0;
		}
		}
		return DefWindowProcW(hWnd, msg, wParam, lParam); };
	wc.hInstance = GetModuleHandleW(0);
	wc.hCursor = LoadCursorW(0, IDC_ARROW);
	wc.lpszClassName = L"FButton";
	RegisterClassExW(&wc);
	wc.hCursor = LoadCursorW(0, IDC_ARROW);
	wc.lpfnWndProc = [](HWND hWnd, unsigned int msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
		switch (msg) {
		case WM_CREATE: {
			HANDLE hThread = CreateThread(0, 0, [](LPVOID lparam) -> ULONG {
				for (int i = 0; i <= 111; i += 96 / 6) {
					SetLayeredWindowAttributes(hwndParent, 0, i, LWA_ALPHA);
					SetLayeredWindowAttributes(HWND(lparam), 0, i * (256 / 111), LWA_ALPHA);
					SetWindowLongPtrW(HWND(lparam), -16, WS_VISIBLE & ~WS_TILEDWINDOW & ~WS_CAPTION);
					Sleep(1);
				}
				SetLayeredWindowAttributes(HWND(lparam), 0, 255, LWA_ALPHA);
				return TRUE; }, hWnd, 0, 0);
			if (hThread) CloseHandle(hThread);
			break;
		}
		case WM_CTLCOLORSTATIC: {
			SetBkMode(HDC(wparam), TRANSPARENT);
			SetTextColor(HDC(wparam), 0xFFFFFF);
			return LRESULT(GetStockObject(5));
		}
		case WM_KEYDOWN: {
			if (wparam == VK_RETURN || wparam == VK_ESCAPE) {
				SendMessageW(hwndMsg, WM_COMMAND, 1004, 0);
				SendMessageW(hwndMsg, WM_COMMAND, 1003, 0);
				SendMessageW(hwndMsg, WM_COMMAND, 1002, 0);
				SendMessageW(hwndMsg, WM_COMMAND, 1001, 0);
			}
			break;
		}
		case WM_COMMAND: {
			EnableWindow(hwndMsg, false);
			EnableWindow(hwndParent, false);
			KillTimer(hWnd, 1);
			for (int i = 112; i >= 0; i -= 96 / 6) {
				SetLayeredWindowAttributes(hwndParent, 0, i, LWA_ALPHA);
				SetLayeredWindowAttributes(hwndMsg, 0, i * (256 / 111), LWA_ALPHA);
				Sleep(1);
			}
			DestroyWindow(hwndMsg);
			DestroyWindow(hwndParent);
			lastanswer = wparam;
			return 0;
		}
		case WM_TIMER: {
			KillTimer(hWnd, (UINT_PTR)wparam);
			PostMessageW(hWnd, WM_COMMAND, 0, 0);
			return 0;
		}
		}
		return DefWindowProcW(hWnd, msg, wparam, lparam); };
	wc.hbrBackground = CreateSolidBrush(RGB(GetBValue(color), GetGValue(color), GetRValue(color)));
	wc.lpszClassName = L"MsgBoxClass";
	RegisterClassExW(&wc);
	HFONT hFont = CreateFontA(20, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "Segoe UI");
	HFONT hTitleFont = CreateFontA(37, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "Segoe UI");
	HDC hdcDesktop = GetDC(GetDesktopWindow());
	HFONT hOld = (HFONT)SelectObject(hdcDesktop, hTitleFont);
	RECT tr{};
	DrawTextW(hdcDesktop, title, -1, &tr, DT_CALCRECT | DT_SINGLELINE);
	SelectObject(hdcDesktop, hOld);
	ReleaseDC(GetDesktopWindow(), hdcDesktop);
	int leftMargin = 24;
	int computedWidth = tr.right - tr.left + leftMargin * 3;
	int winWidth = (computedWidth > 680) ? computedWidth : 680;
	hwndParent = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | 0, L"#32770", title, WS_POPUP | WS_VISIBLE, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0, 0, 0, 0);
	ShowWindow(hwndParent, SW_HIDE);
	SetWindowLongPtrW(hwndParent, -16, WS_VISIBLE | WS_DISABLED);
	SetWindowLongPtrW(hwndParent, -4, LONG_PTR(WNDPROC([](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
		switch (msg) {
		case WM_ERASEBKGND: {
			RECT rect;
			GetClientRect(hwnd, &rect);
			FillRect((HDC)wparam, &rect, CreateSolidBrush(0));
			return 1;
		}
		}
		return DefWindowProcW(hwnd, msg, wparam, lparam); })));
	ShowWindow(hwndParent, SW_SHOW);
	int contentWidth = winWidth - leftMargin * 2;
	hwndMsg = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | 0, L"MsgBoxClass", title, WS_POPUP | WS_VISIBLE, (GetSystemMetrics(SM_CXSCREEN) - winWidth) / 2, (GetSystemMetrics(SM_CYSCREEN) - 141 - CalculateTextHeightWithWordWrap(hFont, text)) / 2 - 24, winWidth, 141 + CalculateTextHeightWithWordWrap(hFont, text), hwndParent, 0, 0, 0);
	SetWindowLongPtrW(hwndMsg, -16, 0 & ~WS_TILEDWINDOW);
	ShowWindow(hwndMsg, SW_HIDE);
	HWND tmph = CreateWindowExW(0, L"Static", title, WS_CHILD | WS_VISIBLE | SS_LEFT, leftMargin, 18, contentWidth, 40, hwndMsg, 0, 0, 0);
	SendMessageW(tmph, WM_SETFONT, WPARAM(hTitleFont), 1);
	tmph = CreateWindowExW(0, L"Static", text, WS_CHILD | WS_VISIBLE | SS_LEFT, leftMargin + 3, 64, contentWidth - 6, 22 + CalculateTextHeightWithWordWrap(hFont, text), hwndMsg, 0, 0, 0);
	SendMessageW(tmph, WM_SETFONT, WPARAM(hFont), 1);
	HDC hdcBtn = GetDC(hwndMsg);
	HFONT hBtnFont = CreateFontA(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "Segoe UI"), hOldBtnFont = (HFONT)SelectObject(hdcBtn, hBtnFont);
	int defaultBtnWidth = 90, btnHeight = 30, gap = 24, padding = 32;
	vector<LPCWSTR> names;
	if (b1name != L"") names.push_back(b1name);
	if (b2name != L"") names.push_back(b2name);
	if (b3name != L"") names.push_back(b3name);
	if (b4name != L"") names.push_back(b4name);
	vector<int> widths(names.size());
	for (int i = 0; i < names.size(); ++i) {
		RECT r = { 0, 0, 0, 0 };
		DrawTextW(hdcBtn, names[i], -1, &r, DT_CALCRECT | DT_SINGLELINE);
		int w = (r.right - r.left) + padding;
		if (w < defaultBtnWidth) w = defaultBtnWidth;
		widths[i] = w;
	}
	int originalRightEdge = winWidth - leftMargin;
	int totalWidth = 0;
	for (int i = 0; i < widths.size(); ++i) totalWidth += widths[i];
	if (widths.size() > 1) totalWidth += gap * (int(widths.size()) - 1);
	int groupLeft = originalRightEdge - totalWidth;
	int top = 85 + CalculateTextHeightWithWordWrap(hFont, text);
	int cur = groupLeft;
	for (int i = 0; i < names.size(); ++i) {
		CreateWindowExW(0, L"FButton", names[i], WS_CHILD | WS_VISIBLE | WS_TABSTOP, cur, top, widths[i], btnHeight, hwndMsg, HMENU(1001 + i), 0, 0);
		cur += widths[i] + gap;
	}
	SelectObject(hdcBtn, hOldBtnFont);
	ReleaseDC(hwndMsg, hdcBtn);
	DeleteObject(hBtnFont);
	SetWindowLongPtrW(hwndMsg, -16, 0 & ~WS_TILEDWINDOW);
	ShowWindow(hwndMsg, SW_SHOW);
	if (timeoutMs > 0) {
		SetTimer(hwndMsg, 1, timeoutMs, NULL);
	}
	SetForegroundWindow(hwndParent);
	SetFocus(hwndMsg);
	MSG msg;
	while (IsWindow(hwndMsg)) {
		GetMessageW(&msg, 0, 0, 0);
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	if (hTitleFont) DeleteObject(hTitleFont);
	if (hFont) DeleteObject(hFont);
	UnregisterClassW(L"FButton", wc.hInstance);
	UnregisterClassW(L"MsgBoxClass", wc.hInstance);
	return lastanswer;
}
int main() {
	MessageBeep(64); //发出一个简单的提示音（可选）
	int result = Create(L"这是一个自定义消息框示例，支持多行文本显示和自动换行。\n此对话框会在10秒后自动关闭。", L"自定义消息框", L"确定", L"取消", L"重试", L"", 10000);
	cout << "用户选择了按钮 ID: " << result << endl;
	return 0;
}
int WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	return main();
}