#pragma once
#include <mutex>
#include <string>

// copied from rainmeter library
#define RAINMETER_CLASS_NAME				L"DummyRainWClass"
#define RAINMETER_WINDOW_NAME				L"Rainmeter control window"

class Measure
{
public:
	void* rm;
	void* skin;

	HWND hWnd;
	static const HWND rmWnd;

	std::wstring onFocusAction;
	std::wstring onUnfocusAction;
	std::wstring onForegroundChangeAction;

	bool isFocused;

	std::mutex mutex;

	void Reload();
	double Update();

	Measure(void* _rm);
	~Measure();
};