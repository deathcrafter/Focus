#include "Plugin.h"
#include "Measure.h"

const HWND Measure::rmWnd = FindWindow(RAINMETER_CLASS_NAME, RAINMETER_WINDOW_NAME);

Measure::Measure(void* _rm) :
	rm(_rm),
	skin(RmGetSkin(_rm)),
	hWnd(RmGetSkinWindow(_rm))
{
	isFocused = GetForegroundWindow() == hWnd;
}

void Measure::Reload() {
	std::unique_lock<std::mutex> lock(mutex);
	onFocusAction = RmReadString(rm, L"OnFocusAction", L"", FALSE);
	onUnfocusAction = RmReadString(rm, L"OnUnfocusAction", L"", FALSE);
	onForegroundChangeAction = RmReadString(rm, L"OnForegroundChangeAction", L"", FALSE);
}

double Measure::Update() {
	return 0.0;
}

Measure::~Measure()
{
}
