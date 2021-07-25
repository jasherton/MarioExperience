#include "events.h"
#include "globals.h"
#include "image.h"

bool PW_INIT = false;
unsigned int PW_Width = 640;
unsigned int PW_Height = 480;
MINMAXINFO* PW_MMI;
IMAGE piss;

void MainFunc(EventObject* obj, HWND FormWnd) {
	ShowWindow(FormWnd, SW_SHOW);

	HDC hdc = NULL;
	while (hdc == NULL) {
		hdc = GetDC(FormWnd);
		Sleep(100);
	}
	
	if (piss.height > 0) {
		unsigned int totalSize = PW_Width * PW_Height;
		unsigned int basePos = (100 * PW_Width) + (PW_Width / 2) - (piss.width / 2);
		std::vector<COLORREF> Background(PW_Width * PW_Height);
		for (unsigned int i = 0; i < Background.size(); i++) {
			Background[i] = 0xFFFFFF;
		}
		for (unsigned int i = 0; i < piss.data.size(); i++) {
			if ((piss.data[i] & 0xFF) > 0) {
				unsigned int newPos = basePos + ((i / piss.width) * PW_Width + i % piss.width);
				if (newPos < totalSize) {
					Background[newPos] = piss.data[i] >> 8;
				}
			}
		}
		HDC src = CreateCompatibleDC(hdc);
		if (src != NULL) {
			HBITMAP image = CreateBitmap(PW_Width, PW_Height, 1, sizeof(COLORREF) * 8, Background.data());
			SelectObject(src, image);
			BitBlt(hdc, 0, 0, PW_Width, PW_Height, src, 0, 0, SRCCOPY);
			DeleteObject(image);
			DeleteDC(src);
		}
	}
	else {
		RECT clientArea;
		GetClientRect(FormWnd, &clientArea);
		FillRect(hdc, &clientArea, (HBRUSH)(COLOR_WINDOW + 1));
	}

	HFONT BigFont = CreateFont(32, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Arial");

	RECT TextRect = {};
	TextRect.left = 0;
	TextRect.right = 640;
	TextRect.top = 300;
	TextRect.bottom = 480;
	
	if (JPMode) {
		SetWindowText(FormWnd, L"マリオ小便中");
		SelectObject(hdc, BigFont);
		DrawTextW(hdc, L"マリオ小便中", -1, &TextRect, DT_CENTER);
	}
	else {
		SelectObject(hdc, BigFont);
		DrawTextW(hdc, L"Mario Pissing", -1, &TextRect, DT_CENTER);
	}

	DeleteObject(BigFont);

	while (!obj->Kill) {
		Sleep(1000);
		if (!IsWindowVisible(FormWnd)) { obj->Kill = true; }
	}

	ReleaseDC(FormWnd, hdc);
	obj->ThreadPtr = NULL;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	case WM_PAINT:
		break;
	case WM_GETMINMAXINFO:
		PW_MMI = (MINMAXINFO*)lParam;
		PW_MMI->ptMinTrackSize.x = PW_Width + 16;
		PW_MMI->ptMinTrackSize.y = PW_Height + 38;
		PW_MMI->ptMaxTrackSize.x = PW_Width;
		PW_MMI->ptMaxTrackSize.y = PW_Height;
		PW_MMI->ptMaxSize.x = PW_Width;
		PW_MMI->ptMaxSize.y = PW_Height;
		break;
	}
	return 0;
}

bool Piss(void* ptr) {
	if (!PW_INIT) {
		WNDCLASS nclass = {};
		nclass.lpfnWndProc = WndProc;
		nclass.hInstance = HINST;
		nclass.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(MAINICON));
		nclass.lpszClassName = L"MPEV1";
		RegisterClass(&nclass);
		
		HRSRC pissResource = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_PISS), L"PNG");
		if (pissResource != NULL) {
			DWORD imgSize = SizeofResource(GetModuleHandle(NULL), pissResource);
			HGLOBAL img = LoadResource(GetModuleHandle(NULL), pissResource);
			if (img != NULL) {
				LPVOID data = LockResource(img);
				std::vector<unsigned char> datVec(imgSize, 0);
				memcpy(datVec.data(), data, imgSize);
				PNG pissImage = PNG(READER(datVec));
				pissImage.ReadChunks();
				piss = pissImage.Process();
			}
		}

		PW_INIT = true;
	}

	HWND FormWnd = CreateWindowEx(
		WS_EX_DLGMODALFRAME,
		L"MPEV1",
		L"Mario Pissing",
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

	EventObject* obj = (EventObject*)ptr;
	std::thread* man = new std::thread(MainFunc,obj,FormWnd);
	obj->ThreadPtr = man;
	return true;
}