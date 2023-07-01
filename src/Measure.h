#pragma once
#include <mutex>
#include <string>
#include <map>
#include <vector>

// copied from rainmeter library
#define RAINMETER_CLASS_NAME				L"DummyRainWClass"
#define RAINMETER_WINDOW_NAME				L"Rainmeter control window"

struct ConfigGroupCacheValue
{
	bool value = false;

	ConfigGroupCacheValue()
	{
		value = true;
	}
};

class Measure
{
public:
	void* rm;
	void* skin;

	HWND hWnd;
	static const HWND rmWnd;

	std::wstring configGroup;
	std::vector<std::wstring> configGroups;

	size_t skinsPathLength;

	bool enableFocusActions = false;
	std::wstring onFocusAction;
	std::wstring onUnfocusAction;

	std::wstring onForegroundChangeAction;

	bool isFocused;

	bool isUpdater;

	std::mutex mutex;

	void Reload();
	double Update();

	Measure(void* _rm);
	~Measure();
};