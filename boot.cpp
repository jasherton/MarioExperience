#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <filesystem>
#include "globals.h"
#include "about.h"
#include "events.h"
#include "unzip.h"

#include <WinSDKVer.h>
#define _WIN32_WINNT 0x0601
#define WINVER 0x0601
#define NTDDI_VERSION 0x06010000
#include <SDKDDKVer.h>

#ifndef WINDEFINED
#define WINDEFINED
#include <Windows.h>
#endif

#define	WM_MARIO_CALLBACK WM_USER + 1

bool AppKill;
bool JPMode;
bool CycleKill;
HWND MainWnd;
HINSTANCE HINST;
HICON TrayIcon;
NOTIFYICONDATA TrayStruct = {};
static std::wstring ToolTip = L"Mario";
HMENU TrayMenu;

HANDLE AppMutex;
static std::string AppID = "v4.0";
static std::string BaseURL = "http://marioexperience.us.to/";
static std::string GitLatestURL = "https://api.github.com/repos/jasherton/MarioExperience/releases/latest";

static HICON NotifIcon1 = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_NOTIF1));
static HICON NotifAnnoy1 = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ANNOY1));
static HICON NotifAnnoy2 = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ANNOY2));
static HICON NotifThink = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_THINK));
static HICON NotifSad = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SAD));

static std::wstring EnableText = L"Enable";
static std::wstring DisableText = L"Disable";
static std::wstring EnableTextJP = L"続ける";
static std::wstring DisableTextJP = L"止める";

std::vector<std::wstring> NotifVector;
std::vector<std::wstring> NotifVectorJP;

void SaveData() {
	std::ofstream setOut;
	setOut.exceptions(0);
	setOut.open("user.bin", std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	setOut.write((char*)&JPMode, sizeof(bool));
	setOut.write((char*)&CycleKill, sizeof(bool));
	setOut.close();
}

std::vector<char> UpdateInfo;
std::vector<char> UpdateData;
bool UpdateValid;

void GetUpdateInfo() {
	UpdateValid = false;
	curl_slist* headers = NULL;
	headers = curl_slist_append(headers, "User-Agent: MarioExperience");
	UpdateInfo = HttpGet(GitLatestURL, headers);
	if (UpdateInfo.size() > 0) {
		std::string updtxt(UpdateInfo.data());
		size_t idx = updtxt.find("\"tag_name\"");
		if (idx != std::string::npos) {
			size_t idx2 = updtxt.find("\"", idx + 10);
			if (idx2 != std::string::npos) {
				size_t idx3 = updtxt.find("\"", idx2 + 1);
				if (idx3 != std::string::npos) {
					std::string tagName = updtxt.substr(idx2+1, idx3 - idx2 - 1);
					if (tagName != AppID) {
						size_t downIdx = updtxt.find("\"browser_download_url\"");
						if (downIdx != std::string::npos) {
							size_t downIdx2 = updtxt.find("\"", downIdx + 22);
							if (downIdx2 != std::string::npos) {
								size_t downIdx3 = updtxt.find("\"", downIdx2 + 1);
								if (downIdx3 != std::string::npos) {
									std::string downURL = updtxt.substr(downIdx2+1, downIdx3 - downIdx2 - 1);
									UpdateData = HttpGet(downURL, headers);
									if (UpdateData.size() > 0) {
										UpdateValid = true;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void DoUpdate() {
	if (UpdateValid) {
		std::ofstream zipOut;
		zipOut.exceptions(0);
		zipOut.open("latest.zip", std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
		zipOut.write(UpdateData.data(), UpdateData.size());
		zipOut.close();

		if (TrayStruct.cbSize > 0) {
			Shell_NotifyIcon(NIM_DELETE, &TrayStruct);
			SaveData();
		}

		TCHAR szExeFileName[MAX_PATH];
		GetModuleFileName(NULL, szExeFileName, MAX_PATH);
		std::filesystem::path filePath(szExeFileName);
		std::filesystem::path parent = filePath.parent_path();
		std::filesystem::rename(filePath, parent.string() + "\\temp.exe");

		HZIP hz = OpenZip(L"latest.zip", 0);
		SetUnzipBaseDir(hz, parent.wstring().data());
		ZIPENTRY ze;
		GetZipItem(hz, -1, &ze);
		for (int i = 0; i < ze.index; i++)
		{
			GetZipItem(hz, i, &ze);
			UnzipItem(hz, i, ze.name);
		}
		CloseZip(hz);
		
		std::filesystem::remove("latest.zip");
		std::filesystem::rename(parent.string() + "\\MarioPissing.exe", filePath);

		ReleaseMutex(AppMutex);
		CloseHandle(AppMutex);

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		ZeroMemory(&pi, sizeof(pi));
		si.cb = sizeof(si);
		if (CreateProcess(NULL, (LPWSTR)(filePath.wstring() + L" -up").c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		};
		HANDLE hnd;
		hnd = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, TRUE, GetCurrentProcessId());
		TerminateProcess(hnd, 0);
	}
}

unsigned int IgnoreCount;
unsigned int MarioTimer;
unsigned int IgnoreSwitch;
HICON DispIco;
std::thread* CycleThread;
void MarioCycle() {
	MarioTimer = (rand() % 60 + rand() % 60 + rand() % 60) * 60;
	while (!CycleKill) {
		Sleep(1000);
		MarioTimer -= 1;
		if (MarioTimer == 0) {
			if (!AppKill) {
				std::wstring NotifTitle = L"Mario Alert";
				std::wstring NotifText = L"Test";
				
				IgnoreSwitch = (IgnoreCount < 5) ? 0 : (IgnoreCount < 7) ? 1 : (IgnoreCount < 9) ? 2 : (IgnoreCount == 9) ? 3 : (IgnoreCount == 10) ? 4 : (IgnoreCount == 11) ? 5 : 6;
				
				switch (IgnoreSwitch) {
				case 0:
					DispIco = NotifIcon1;
					break;
				case 1:
					DispIco = NotifAnnoy1;
					break;
				case 2:
					DispIco = NotifAnnoy2;
					break;
				case 3:
					DispIco = NotifThink;
					break;
				case 4:
					DispIco = NotifThink;
					break;
				case 5:
					DispIco = NotifSad;
					break;
				case 6:
					DispIco = NotifSad;
					break;
				}

				if (IgnoreSwitch == 0) {
					int randText = rand() % 3000;
					if (randText > 1000 && randText < 2000) {
						NotifText = (JPMode) ? std::wstring(L"マリオ小便中") : std::wstring(L"Mario Pissing");
					}
					else {
						NotifText = (JPMode) ? NotifVectorJP[0] : NotifVector[0];
					}
				}
				else {
					switch (IgnoreSwitch) {
					default:
						NotifText = (JPMode) ? NotifVectorJP[0] : NotifVector[0];
						break;
					case 3:
						NotifText = (JPMode) ? NotifVectorJP[1] : NotifVector[1];
						break;
					case 4:
						NotifText = (JPMode) ? NotifVectorJP[2] : NotifVector[2];
						break;
					case 5:
						NotifText = (JPMode) ? NotifVectorJP[3] : NotifVector[3];
						break;
					case 6:
						NotifText = (JPMode) ? NotifVectorJP[4] : NotifVector[4];
						break;
					}
				}

				NotifTitle = (JPMode) ? std::wstring(L"マリオ通知") : std::wstring(L"Mario Alert");

				NOTIFYICONDATA NotifStruct = {};
				NotifStruct.cbSize = sizeof(NOTIFYICONDATA);
				NotifStruct.hWnd = MainWnd;
				NotifStruct.uID = 0;
				NotifStruct.uFlags = NIF_INFO;
				NotifStruct.hIcon = DispIco;
				NotifStruct.hBalloonIcon = DispIco;
				NotifStruct.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON | NIIF_NOSOUND;
				NotifStruct.uTimeout = (UINT)10000;
				memcpy(NotifStruct.szInfo, NotifText.c_str(), NotifText.length() * sizeof(wchar_t));
				memcpy(NotifStruct.szInfoTitle, NotifTitle.c_str(), NotifTitle.length() * sizeof(wchar_t));
				Shell_NotifyIcon(NIM_MODIFY, &NotifStruct);
			}
			MarioTimer = (rand() % 60 + rand() % 60 + rand() % 60) * 60;
		}
	}
	CycleThread = NULL;
}

std::thread* IdleThread;
bool IdleKill;
void IdleThreadFn() {
	unsigned int GarbageCounter = 0;
	unsigned int UpdateCounter = 0;
	UpdateValid = false;
	while (!IdleKill) {
		Sleep(500);
		GarbageCounter++;
		UpdateCounter++;
		if (GarbageCounter >= 10) {
			GarbageCounter = 0;
			EventGarbage();
		}
		if (UpdateCounter >= 1200) {
			UpdateCounter = 0;
			GetUpdateInfo();
		}
	}
}

bool BalloonVis;
int wmId, wmEvent;
POINT lpClickPoint;
MENUITEMINFO menuItem;

//Main Handler Window Procedure
LRESULT CALLBACK MainProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	case WM_DESTROY:
		AppKill = true;
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		break;
	case WM_MARIO_CALLBACK: {
		switch (LOWORD(lParam)) {
		case WM_LBUTTONUP: {
			if (BalloonVis) { BalloonVis = false; return 1; }
			if (!CycleKill) { return 1; }
			DoEvent();
			return 1;
		}
		case WM_RBUTTONUP:
		{
			if (BalloonVis) { BalloonVis = false; return 1; }
			GetCursorPos(&lpClickPoint);
			SetForegroundWindow(MainWnd);
			HMENU subMenu = GetSubMenu(TrayMenu, JPMode ? 1 : 0);
			if (!CycleKill) {
				menuItem.cbSize = sizeof(MENUITEMINFO);
				menuItem.fMask = MIIM_TYPE | MIIM_DATA;
				if (!JPMode) {
					menuItem.dwTypeData = (wchar_t*)DisableText.c_str();
					menuItem.cch = DisableText.length() * sizeof(wchar_t);
				}
				else {
					menuItem.dwTypeData = (wchar_t*)DisableTextJP.c_str();
					menuItem.cch = DisableTextJP.length() * sizeof(wchar_t);
				}
				SetMenuItemInfo(subMenu, 1, true, &menuItem);
			}
			else {
				menuItem.cbSize = sizeof(MENUITEMINFO);
				menuItem.fMask = MIIM_TYPE | MIIM_DATA;
				if (!JPMode) {
					menuItem.dwTypeData = (wchar_t*)EnableText.c_str();
					menuItem.cch = EnableText.length() * sizeof(wchar_t);
				}
				else {
					menuItem.dwTypeData = (wchar_t*)EnableTextJP.c_str();
					menuItem.cch = EnableTextJP.length() * sizeof(wchar_t);
				}
				SetMenuItemInfo(subMenu, 1, true, &menuItem);
			}
			EnableMenuItem(subMenu, ID_UPDATE, (!UpdateValid) ? MF_GRAYED : MF_BYCOMMAND);
			TrackPopupMenu(subMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, lpClickPoint.x, lpClickPoint.y, 0, MainWnd, NULL);
			return 1;
		}
		case NIN_BALLOONSHOW:
			BalloonVis = true;
			break;
		case NIN_BALLOONTIMEOUT: {
			IgnoreCount = (IgnoreCount < 12) ? IgnoreCount + 1 : IgnoreCount;
			BalloonVis = false;
			return 1;
		}
		case NIN_BALLOONUSERCLICK: {
			IgnoreCount = 0;
			DoEvent();
			return 1;
		}
		}
		break;
	}
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId) {
		case ID_ABOUT:
			if (!ActiveAbout()) { OpenAbout(); }
			break;
		case ID_EXIT:
			PostQuitMessage(0);
			break;
		case ID_LANGUAGE:
			switch (JPMode) {
			case true:
				JPMode = false;
				break;
			case false:
				JPMode = true;
				break;
			}
			break;
		case ID_DISABLE:
			if (!CycleKill) {
				CycleKill = true;
			}
			else {
				CycleKill = false;
				CycleThread = new std::thread(MarioCycle);
			}
			break;
		case ID_UPDATE: {
			DoUpdate();
			break;
		}
		case ID_EVENT: {
			DoEvent();
			break;
		}
		}
		break;
	}
	return 0;
}

//Entry Point
int _stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	WNDCLASS nclass = {};
	nclass.lpfnWndProc = MainProc;
	nclass.hInstance = hInstance;
	nclass.lpszClassName = L"MPMHW";
	RegisterClass(&nclass);
	
	AppMutex = CreateMutex(NULL, TRUE, L"MARIOEXPERIENCEMUTEX");
	if (ERROR_ALREADY_EXISTS == GetLastError()) { return 0; }

	CycleThread = NULL;
	TrayMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MENU1));
	AppKill = false;
	HINST = hInstance;

	//Settings File
	std::ifstream setIn;
	setIn.exceptions(0);
	std::vector<char> settings(16,0);
	
	setIn.open("user.bin", std::ios_base::in | std::ios_base::binary);
	if (!(setIn.fail() || setIn.bad())) {
		setIn.read(settings.data(), settings.size());
		setIn.close();
		char* dataBuffer = settings.data();
		JPMode = (bool)*dataBuffer;
		dataBuffer += sizeof(bool);
		CycleKill = (bool)*dataBuffer;
	}
	else {
		setIn.close();
		wchar_t LC[85] = { 0 };
		GetUserDefaultLocaleName(LC, LOCALE_NAME_MAX_LENGTH);
		std::wstring LocaleName = std::wstring(LC);
		JPMode = LocaleName.substr(0, 2) == L"ja" ? true : false;
		CycleKill = false;
	}

	bool UpdatedApp = false;
	if (__argc > 1) {
		UpdatedApp = (strcmp(__argv[1], "-up") == 0);
	}

	if (UpdatedApp) {
		if (std::filesystem::exists("temp.exe")) {
			std::filesystem::remove("temp.exe");
		}
	}
	else {
		#if !_DEBUG
		GetUpdateInfo();
		DoUpdate();
		#endif
	}
	
	InitAbout();
	InitEvents();

	MainWnd = CreateWindowEx(
		NULL,
		L"MPMHW",
		L"Mario Experience",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		hInstance,
		NULL
	);
	
	NotifVector.push_back(std::wstring(L"Mario has something fun for you"));
	NotifVector.push_back(std::wstring(L"something fun"));
	NotifVector.push_back(std::wstring(L"something fun?"));
	NotifVector.push_back(std::wstring(L"fun?"));
	NotifVector.push_back(std::wstring(L"..."));
	NotifVectorJP.push_back(std::wstring(L"マリオは何か楽しいものがありますよ"));
	NotifVectorJP.push_back(std::wstring(L"何か楽しいもの"));
	NotifVectorJP.push_back(std::wstring(L"何か楽しいもの？"));
	NotifVectorJP.push_back(std::wstring(L"楽しい？"));
	NotifVectorJP.push_back(std::wstring(L"..."));
	
	//Mario Init
	TrayIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_TRAYDEFAULT));
	TrayStruct.cbSize = sizeof(NOTIFYICONDATA);
	TrayStruct.hWnd = MainWnd;
	TrayStruct.uID = 0;
	TrayStruct.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	TrayStruct.hIcon = TrayIcon;
	TrayStruct.uCallbackMessage = WM_MARIO_CALLBACK;
	memcpy(TrayStruct.szTip, ToolTip.c_str(), ToolTip.length()*sizeof(wchar_t));
	Shell_NotifyIcon(NIM_ADD, &TrayStruct);

	MarioTimer = 0;
	IgnoreCount = 0;
	if (!CycleKill) {
		CycleThread = new std::thread(MarioCycle);
	}

	IdleKill = false;
	IdleThread = new std::thread(IdleThreadFn);

	ShowWindow(MainWnd, SW_HIDE);
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Shell_NotifyIcon(NIM_DELETE, &TrayStruct);

	SaveData();

	if (CycleThread != NULL) {
		CycleKill = true;
		CycleThread->join();
	}

	if (IdleThread != NULL) {
		IdleKill = true;
		IdleThread->join();
	}

	KillEvents();
	ReleaseMutex(AppMutex);
	CloseHandle(AppMutex);
	
	return 1;
}