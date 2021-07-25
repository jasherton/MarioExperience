#include "globals.h"

size_t writeString(char* contents, size_t size, size_t nmemb, void* userp)
{
	std::vector<char>* pt = ((std::vector<char>*)userp);
	size_t vecSize = pt->size();
	size_t conSize = size * nmemb;
	pt->resize(vecSize + conSize, 0);
	for (int i = 0; i < conSize; i++) {
		pt->at(vecSize + i) = contents[i];
	}
	return size * nmemb;
}

std::vector<char> CURL_Get(std::string url) {
	CURL* curl = curl_easy_init();
	std::vector<char> content = std::vector<char>();
	long status = 0;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeString);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
	CURLcode res = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
	curl_easy_cleanup(curl);
	if (status != 200) {
		content.clear();
	}
	return content;
}