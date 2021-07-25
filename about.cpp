#include "globals.h"
#include "about.h"
#include <mmsystem.h>

//Variables
HICON mainIco;
HWND FormWnd;
std::thread* FormThread;
bool FormKill;

//Base Window Variables
unsigned int Width = 640;
unsigned int Height = 480;
MINMAXINFO* mmi;

//About Window
void AboutWndFn() {
	PlaySound(MAKEINTRESOURCE(IDR_WAVE1), NULL, SND_RESOURCE | SND_ASYNC);
	ShowWindow(FormWnd, SW_SHOW);
	
	HDC hdc = NULL;
	while (hdc == NULL) {
		hdc = GetDC(FormWnd);
		Sleep(100);
	}

	HICON thumb = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_THUMB));

	RECT clientArea;
	GetClientRect(FormWnd, &clientArea);
	int RectMiddleX = (clientArea.right - clientArea.left) / 2;
	FillRect(hdc, &clientArea, (HBRUSH)(COLOR_WINDOW + 1));
	DrawIconEx(hdc, RectMiddleX - 32, 32, thumb, 64, 64, 0, (HBRUSH)(COLOR_WINDOW + 1), DI_NORMAL);
	
	HFONT BigFont = CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Arial");
	HFONT SmallFont = CreateFont(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Arial");
	
	RECT TextRect = {};
	TextRect.left = RectMiddleX - 200;
	TextRect.right = RectMiddleX + 200;
	TextRect.top = 128;
	TextRect.bottom = 480;

	if (JPMode) {
		SetWindowText(FormWnd, L"情報");
		SelectObject(hdc, BigFont);
		DrawTextW(hdc, L"マリオ体験", -1, &TextRect, DT_CENTER);

		TextRect.top = 160;
		SelectObject(hdc, SmallFont);
		DrawTextW(hdc, L"このアプリが人生はマリオだらけ上げて目指します", -1, &TextRect, DT_CENTER);
		TextRect.top = 180;
		SetTextColor(hdc, (COLORREF)0x000000FF);
		DrawTextW(hdc, L"マリオだけ", -1, &TextRect, DT_CENTER);
		TextRect.top = 450;
		SetTextColor(hdc, (COLORREF)0x00FF8053);
		DrawTextW(hdc, L"crazy1755作", -1, &TextRect, DT_CENTER);
	}
	else {
		SelectObject(hdc, BigFont);
		DrawTextW(hdc, L"Mario Experience", -1, &TextRect, DT_CENTER);

		TextRect.top = 160;
		SelectObject(hdc, SmallFont);
		DrawTextW(hdc, L"This application aims to give you a life filled with Mario.", -1, &TextRect, DT_CENTER);
		TextRect.top = 180;
		SetTextColor(hdc, (COLORREF)0x000000FF);
		DrawTextW(hdc, L"Only Mario", -1, &TextRect, DT_CENTER);
		TextRect.top = 450;
		SetTextColor(hdc, (COLORREF)0x00FF8053);
		DrawTextW(hdc, L"Made by crazy1755", -1, &TextRect, DT_CENTER);
	}
	
	DeleteObject(BigFont);
	DeleteObject(SmallFont);
	
	while (!FormKill) {
		Sleep(1000);
	}
	
	ReleaseDC(FormWnd,hdc);
	FormThread = NULL;
}

LRESULT CALLBACK FormWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	case WM_DESTROY:
		FormKill = true;
		break;
	case WM_PAINT:
		break;
	case WM_GETMINMAXINFO:
		mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize.x = Width + 16;
		mmi->ptMinTrackSize.y = Height + 38;
		mmi->ptMaxTrackSize.x = Width;
		mmi->ptMaxTrackSize.y = Height;
		mmi->ptMaxSize.x = Width;
		mmi->ptMaxSize.y = Height;
		break;
	}
	return 0;
}

void InitAbout() {
	FormThread = NULL;
	FormKill = false;

	mainIco = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(MAINICON));

	//Base Form Window
	WNDCLASS nclass = {};
	nclass.lpfnWndProc = FormWndProc;
	nclass.hInstance = HINST;
	nclass.hIcon = mainIco;
	nclass.lpszClassName = L"MPAW";
	RegisterClass(&nclass);
}

void OpenAbout() {
	FormWnd = CreateWindowEx(
		WS_EX_DLGMODALFRAME,
		L"MPAW",
		L"About",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		NULL,
		NULL
	);
	FormKill = false;
	FormThread = new std::thread(AboutWndFn);
}

bool ActiveAbout() {
	return (FormThread != NULL);
}

void KillAbout() {
	if (FormThread != NULL) {
		FormKill = true;
		FormThread->join();
	}
}