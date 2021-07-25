#include "events.h"
#include "globals.h"
#include <vector>
#include <thread>

std::vector<EventObject*> EventList;
std::vector<void*> EventFunc;

void InitEvents() {
	EventList = std::vector<EventObject*>();
	EventFunc = std::vector<void*>();
	EventFunc.push_back(&Piss);
	EventFunc.push_back(&Facts);
}

void DoEvent() {
	EventObject* obj = new EventObject();
	obj->Kill = false;
	obj->ThreadPtr = NULL;
	void* fn = *select_randomly(EventFunc.begin(), EventFunc.end());
	bool res = ((bool(*)(void*))fn)(obj);
	if (res) {
		EventList.push_back(obj);
	}
}

void EventGarbage() {
	if (EventList.size() == 0) return;
	std::vector<EventObject*> evt = std::vector<EventObject*>();
	for (int i = 0; i < EventList.size(); i++) {
		EventObject* obj = EventList[i];
		if (obj->ThreadPtr != NULL) {
			evt.push_back(obj);
		}
		else {
			delete obj;
		}
	}
	EventList.swap(evt);
}

void KillEvents() {
	if (EventList.size() == 0) return;
	for (int i = EventList.size()-1; i >= 0; i--) {
		EventObject* obj = EventList[i];
		if (obj->ThreadPtr != NULL) {
			obj->Kill = true;
			obj->ThreadPtr->join();
		}
		EventList.pop_back();
	}
}