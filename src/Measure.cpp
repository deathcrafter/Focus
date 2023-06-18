#include "Plugin.h"
#include "Measure.h"

const HWND Measure::rmWnd = FindWindow(RAINMETER_CLASS_NAME, RAINMETER_WINDOW_NAME);

Measure::Measure(void* _rm) :
	rm(_rm),
	skin(RmGetSkin(_rm)),
	hWnd(RmGetSkinWindow(_rm))
{
	configGroup.append(RmReadString(rm, L"RootConfig", L""));
	enableFocusActions = !configGroup.empty() && configGroup.find_first_not_of(L" ") != configGroup.npos;
	configGroup = RmReplaceVariables(rm, L"#SKINSPATH#") + configGroup + L"\\";

	HWND hwnd = GetForegroundWindow();
	std::wstring windowClass;
	windowClass.resize(MAX_CLASS_NAME);
	GetClassName(hwnd, &windowClass[0], MAX_CLASS_NAME);
	std::wstring windowTitle;
	windowTitle.resize(MAX_PATH);

	if (NULL == _wcsicmp(windowClass.c_str(), L"RainmeterMeterWindow")) {
		if (windowTitle.find(configGroup) != windowTitle.npos) {
			isFocused = true;
		}
	}
}

void Measure::Reload() {
	std::unique_lock<std::mutex> lock(mutex);
	if (enableFocusActions) {
		onFocusAction = RmReadString(rm, L"OnFocusAction", L"", FALSE);
		onUnfocusAction = RmReadString(rm, L"OnUnfocusAction", L"", FALSE);
	}
	onForegroundChangeAction = RmReadString(rm, L"OnForegroundChangeAction", L"", FALSE);
}

double Measure::Update() {
	return 0.0;
}

Measure::~Measure()
{
}
