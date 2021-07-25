#include "events.h"
#include "globals.h"
#include "image.h"

bool FW_INIT = false;
unsigned int FW_Width = 640;
unsigned int FW_Height = 480;
MINMAXINFO* FW_MMI;
IMAGE factsImage;

std::vector<std::wstring> FactVec;
std::vector<std::wstring> FactVecJP;

void FactThread(EventObject* obj, HWND FormWnd) {
	ShowWindow(FormWnd, SW_SHOW);

	HDC hdc = NULL;
	while (hdc == NULL) {
		hdc = GetDC(FormWnd);
		Sleep(100);
	}

	if (factsImage.height > 0) {
		unsigned int totalSize = FW_Width * FW_Height;
		std::vector<COLORREF> Background(FW_Width * FW_Height);
		for (unsigned int i = 0; i < Background.size(); i++) {
			Background[i] = factsImage.data[i] >> 8;
		}
		HDC src = CreateCompatibleDC(hdc);
		if (src != NULL) {
			HBITMAP image = CreateBitmap(FW_Width, FW_Height, 1, sizeof(COLORREF) * 8, Background.data());
			SelectObject(src, image);
			BitBlt(hdc, 0, 0, FW_Width, FW_Height, src, 0, 0, SRCCOPY);
			DeleteObject(image);
			DeleteDC(src);
		}
	}
	else {
		RECT clientArea;
		GetClientRect(FormWnd, &clientArea);
		FillRect(hdc, &clientArea, (HBRUSH)(COLOR_WINDOW + 1));
	}

	HFONT BigFont = CreateFont(36, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Arial");

	RECT TextRect = {};
	TextRect.left = 17;
	TextRect.right = 17+306;
	TextRect.top = 68;
	TextRect.bottom = 68+50;

	int index = select_randomly(FactVec.begin(), FactVec.end()) - FactVec.begin();

	if (JPMode) {
		SetWindowText(FormWnd, L"マリオ事実");
		SelectObject(hdc, BigFont);
		SetTextColor(hdc, (COLORREF)0x000000E3);
		DrawTextW(hdc, L"マリオ事実", -1, &TextRect, DT_CENTER);
		HFONT SmallFont = CreateFont(24, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Arial");
		SelectObject(hdc, SmallFont);
		SetTextColor(hdc, (COLORREF)0x00000000);
		TextRect.left = 340;
		TextRect.right = 340 + 275;
		TextRect.top = 90;
		TextRect.bottom = 90 + 365;
		DrawTextW(hdc, FactVecJP[index].c_str(), -1, &TextRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
		DeleteObject(SmallFont);
	}
	else {
		SelectObject(hdc, BigFont);
		SetTextColor(hdc, (COLORREF)0x000000E3);
		DrawTextW(hdc, L"Mario Fact", -1, &TextRect, DT_CENTER);
		HFONT SmallFont = CreateFont(24, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Arial");
		SelectObject(hdc, SmallFont);
		SetTextColor(hdc, (COLORREF)0x00000000);
		TextRect.left = 340;
		TextRect.right = 340 + 275;
		TextRect.top = 90;
		TextRect.bottom = 90 + 365;
		DrawTextW(hdc, FactVec[index].c_str(), -1, &TextRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
		DeleteObject(SmallFont);
	}

	DeleteObject(BigFont);

	while (!obj->Kill) {
		Sleep(1000);
		if (!IsWindowVisible(FormWnd)) { obj->Kill = true; }
	}

	ReleaseDC(FormWnd, hdc);
	obj->ThreadPtr = NULL;
}

LRESULT CALLBACK FactProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	case WM_PAINT:
		break;
	case WM_GETMINMAXINFO:
		FW_MMI = (MINMAXINFO*)lParam;
		FW_MMI->ptMinTrackSize.x = FW_Width + 16;
		FW_MMI->ptMinTrackSize.y = FW_Height + 38;
		FW_MMI->ptMaxTrackSize.x = FW_Width;
		FW_MMI->ptMaxTrackSize.y = FW_Height;
		FW_MMI->ptMaxSize.x = FW_Width;
		FW_MMI->ptMaxSize.y = FW_Height;
		break;
	}
	return 0;
}

bool Facts(void* ptr) {
	if (!FW_INIT) {
		WNDCLASS nclass = {};
		nclass.lpfnWndProc = FactProc;
		nclass.hInstance = HINST;
		nclass.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(MAINICON));
		nclass.lpszClassName = L"MPEV2";
		RegisterClass(&nclass);
		
		HRSRC pissResource = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_FACTS), L"PNG");
		if (pissResource != NULL) {
			DWORD imgSize = SizeofResource(GetModuleHandle(NULL), pissResource);
			HGLOBAL img = LoadResource(GetModuleHandle(NULL), pissResource);
			if (img != NULL) {
				LPVOID data = LockResource(img);
				std::vector<unsigned char> datVec(imgSize, 0);
				memcpy(datVec.data(), data, imgSize);
				PNG pissImage = PNG(READER(datVec));
				pissImage.ReadChunks();
				factsImage = pissImage.Process();
			}
		}

		FactVec = std::vector<std::wstring>();
		FactVecJP = std::vector<std::wstring>();

		FactVec.push_back(std::wstring(
			L"Did you know?\n\n"
			"Every brick you break in Super Mario Bros. is actually a toad."
		));
		FactVec.push_back(std::wstring(L"Mario was created by Shigeru Miyamoto."));
		FactVec.push_back(std::wstring(L"Mario is very popular."));
		FactVec.push_back(std::wstring(L"Mario has a brother named Luigi."));
		FactVec.push_back(std::wstring(L"Mario likes pasta."));
		FactVec.push_back(std::wstring(L"Mario is watching you."));
		FactVec.push_back(std::wstring(L"Mario has played tennis."));
		FactVec.push_back(std::wstring(L"Mario has gone to hell before."));
		FactVec.push_back(std::wstring(L"Mario has gone to heaven before."));
		FactVec.push_back(std::wstring(L"Mario has appeared in many games."));
		FactVec.push_back(std::wstring(L"Mario has been in prison before."));
		FactVec.push_back(std::wstring(L"Mario sleeps sometimes."));
		FactVec.push_back(std::wstring(L"Mario eats mushrooms."));
		FactVec.push_back(std::wstring(L"Mario can jump."));
		FactVec.push_back(std::wstring(L"Mario can die."));
		FactVec.push_back(std::wstring(L"Mario has infinite lives."));
		FactVec.push_back(std::wstring(L"Mario has been on TV before."));
		
		FactVecJP.push_back(std::wstring(
			L"知りましたの？\n\n"
			"スーパーマリオブラザーズにはそれぞれブロック壊すのは実際にキノピオですよ"
		));
		FactVecJP.push_back(std::wstring(L"マリオは作られて宮本茂にです"));
		FactVecJP.push_back(std::wstring(L"マリオはとっても人気があります"));
		FactVecJP.push_back(std::wstring(L"マリオは弟がありますで、ルイージ！"));
		FactVecJP.push_back(std::wstring(L"マリオはパスタ好きだよ"));
		FactVecJP.push_back(std::wstring(L"マリオは見ている。"));
		FactVecJP.push_back(std::wstring(L"マリオはテニス遊びました"));
		FactVecJP.push_back(std::wstring(L"マリオは前に地獄に行った"));
		FactVecJP.push_back(std::wstring(L"マリオは前に天国に行った"));
		FactVecJP.push_back(std::wstring(L"マリオが多いゲームは登場しましたよ"));
		FactVecJP.push_back(std::wstring(L"マリオは前に刑務所にいましたよ"));
		FactVecJP.push_back(std::wstring(L"マリオはたまに眠る。"));
		FactVecJP.push_back(std::wstring(L"マリオはキノコを食べます"));
		FactVecJP.push_back(std::wstring(L"マリオはジャンプ出来る"));
		FactVecJP.push_back(std::wstring(L"マリオは死ねるよ"));
		FactVecJP.push_back(std::wstring(L"マリオは無限なライフがありますよ"));
		FactVecJP.push_back(std::wstring(L"マリオはテレビに出て前にですよ"));
		
		FW_INIT = true;
	}

	HWND FormWnd = CreateWindowEx(
		WS_EX_DLGMODALFRAME,
		L"MPEV2",
		L"Mario Fact",
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
	std::thread* man = new std::thread(FactThread, obj, FormWnd);
	obj->ThreadPtr = man;
	return true;
}