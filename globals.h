#pragma once
#include <Windows.h>
#include <thread>
#include <ctime>
#include <shellapi.h>
#include <string>
#include "resource.h"
#include <locale>
#include <random>
#include <iterator>
#include <curl/curl.h>

extern HINSTANCE HINST;
extern HWND MainWnd;
extern bool JPMode;

struct EventObject {
	bool Kill;
	std::thread* ThreadPtr;
};

template<typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
	std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
	std::advance(start, dis(g));
	return start;
}

template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	return select_randomly(start, end, gen);
}

extern size_t writeString(char* contents, size_t size, size_t nmemb, void* userp);
extern std::vector<char> HttpGet(std::string url);
extern std::vector<char> HttpGet(std::string url, curl_slist* chunk);