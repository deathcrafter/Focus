#include "Plugin.h"
#include "Measure.h"

const HWND Measure::rmWnd = FindWindow(RAINMETER_CLASS_NAME, RAINMETER_WINDOW_NAME);

std::vector<std::wstring> SplitString(const std::wstring& s, const std::wstring& delimiter) {
	std::vector<std::wstring> tokens;

	auto trim = [](std::wstring& s) {
		s = s.substr(s.find_first_not_of(L" "));
		s = s.substr(0, s.find_last_not_of(L" ") + 1);
	};

	auto add_token = [&tokens, &trim](std::wstring& token) {
		trim(token);
		if (!token.empty()) tokens.push_back(token);
	};

	size_t last = 0;
	size_t next = 0;
	while ((next = s.find(delimiter, last)) != std::wstring::npos) {
		std::wstring token = s.substr(last, next - last);
		add_token(token);
		last = next + 1;
	}
	if (last < s.size())
	{
		std::wstring token = s.substr(last);
		add_token(token);
	}

	return tokens;
}

Measure::Measure(void* _rm) :
	rm(_rm),
	skin(RmGetSkin(_rm)),
	hWnd(RmGetSkinWindow(_rm))
{
	configGroup.append(RmReadString(rm, L"ConfigGroups", L""));
	enableFocusActions = !configGroup.empty() && configGroup.find_first_not_of(L" ") != configGroup.npos;
	configGroups = SplitString(configGroup, L"|");
	skinsPathLength = wcslen(RmReplaceVariables(rm, L"#SKINSPATH#"));

	HWND hwnd = GetForegroundWindow();
	std::wstring windowClass;
	windowClass.resize(MAX_CLASS_NAME);
	GetClassName(hwnd, &windowClass[0], MAX_CLASS_NAME);
	std::wstring windowTitle;
	windowTitle.resize(MAX_PATH);

	if (NULL == _wcsicmp(windowClass.c_str(), L"RainmeterMeterWindow")) {
		using namespace std::regex_constants;
		for (auto configGroup : configGroups) {
			std::wstring skinTitle = windowTitle.substr(skinsPathLength); // remove the skins path from the title
			if (std::regex_search(skinTitle, std::wregex(configGroup, ECMAScript | icase))) {
				isFocused = true;
				break;
			}
		}
	}

	// isUpdater = NULL != RmReadInt(rm, L"Updater", 0);
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
