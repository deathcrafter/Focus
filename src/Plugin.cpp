#include "Plugin.h"
#include "Measure.h"
#include <psapi.h>
#include <vector>

// Copied from Rainmeter.h
#define WM_RAINMETER_EXECUTE WM_APP + 2

HINSTANCE MODULE_INSTANCE;

std::mutex m_measures_mutex;
std::vector<Measure*> m_measures;
HWINEVENTHOOK m_hook = NULL;

std::mutex m_current_window_mutex;
static struct CurrentWindow {
	HWND currentWindow = NULL;
	std::wstring title = L"";
	std::wstring baseName = L"";
	std::wstring path = L"";
	std::wstring windowClass = L"";
} m_current_window;

/*
* Entry point to Dll, run once at dll load
* Use it to store the dll instance, in case you need it for hooks and other stuff
*/
BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpvReserved)  // reserved
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		MODULE_INSTANCE = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL); // disable thread library calls, for performance improvement
	default:
		break;
	}
	return TRUE;
}

void WinEventProc(
	HWINEVENTHOOK hWinEventHook,
	DWORD event,
	HWND hwnd,
	LONG idObject,
	LONG idChild,
	DWORD idEventThread,
	DWORD dwmsEventTime
) {
	if (hWinEventHook != m_hook || event != EVENT_SYSTEM_FOREGROUND) return;
	std::wstring windowClass;
	windowClass.resize(MAX_CLASS_NAME);
	GetClassName(hwnd, &windowClass[0], MAX_CLASS_NAME);
	std::wstring windowTitle;
	windowTitle.resize(MAX_PATH);
	GetWindowText(hwnd, &windowTitle[0], MAX_PATH);
	{
		std::unique_lock<std::mutex> lock(m_current_window_mutex);

		m_current_window.currentWindow = hwnd;

		m_current_window.baseName.clear();
		m_current_window.path.clear();
		m_current_window.baseName.resize(MAX_PATH);
		m_current_window.path.resize(MAX_PATH);
		DWORD processId = NULL;
		GetWindowThreadProcessId(hwnd, &processId);
		HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
		if (hProc) {
			GetModuleBaseName((HMODULE)hProc, NULL, &m_current_window.baseName[0], MAX_PATH);
			DWORD temp = MAX_PATH;
			QueryFullProcessImageName(hProc, NULL, &m_current_window.path[0], &temp);
			CloseHandle(hProc);
		}

		m_current_window.title = windowTitle;
		m_current_window.windowClass = windowClass;
	}
	std::unique_lock<std::mutex> lock(m_measures_mutex);
	for (auto measure : m_measures) {
		std::unique_lock<std::mutex> lock(measure->mutex);

		if (!measure->onForegroundChangeAction.empty())
			SendNotifyMessage(
				measure->rmWnd, WM_RAINMETER_EXECUTE, (WPARAM)measure->skin,
				(LPARAM)RmReplaceVariables(measure->rm, measure->onForegroundChangeAction.c_str())
			);

		if (!measure->enableFocusActions) continue;

		bool isConfigInGroup = false;

		if (NULL == _wcsicmp(windowClass.c_str(), L"RainmeterMeterWindow")) {
			if (windowTitle.find(measure->configGroup) != windowTitle.npos) {
				isConfigInGroup = true;
			}
		}

		if (measure->isFocused && !isConfigInGroup) {
			if (!measure->onUnfocusAction.empty())
				SendNotifyMessage(
					measure->rmWnd, WM_RAINMETER_EXECUTE, (WPARAM)measure->skin,
					(LPARAM)RmReplaceVariables(measure->rm, measure->onUnfocusAction.c_str())
				);
			measure->isFocused = false;
		}
		else if (!measure->isFocused && isConfigInGroup) {
			if (!measure->onFocusAction.empty())
				SendNotifyMessage(
					measure->rmWnd, WM_RAINMETER_EXECUTE, (WPARAM)measure->skin,
					(LPARAM)RmReplaceVariables(measure->rm, measure->onFocusAction.c_str())
				);
			measure->isFocused = true;
		}
	}
}

void Hook() {
	if (NULL == m_hook) {
		m_hook = SetWinEventHook(
			EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
			MODULE_INSTANCE, (WINEVENTPROC)WinEventProc,
			NULL, NULL,
			WINEVENT_OUTOFCONTEXT
		);
		if (NULL == m_hook && m_measures.size()) {
			RmLogF(m_measures[0]->rm, LOG_ERROR, L"Couldn't set windows event hook. Error: %x", GetLastError());
		}
	}
}

void Unhook() {
	if (NULL != m_hook) {
		UnhookWinEvent(m_hook);
		m_hook = NULL;
	}
}

void AddMeasure(Measure* measure) {
	std::unique_lock<std::mutex> lock(m_measures_mutex);
	auto f = std::find(m_measures.begin(), m_measures.end(), measure);
	if (f == m_measures.end()) {
		if (!m_measures.size()) Hook();
		m_measures.push_back(measure);
	}
}

void RemoveMeasure(Measure* measure) {
	std::unique_lock<std::mutex> lock(m_measures_mutex);
	auto f = std::find(m_measures.begin(), m_measures.end(), measure);
	if (f != m_measures.end()) {
		m_measures.erase(f);
		if (!m_measures.size()) Unhook();
	}
}

/*
* Called once, at skin load or refresh
* Read any options that need to be constant here
*/
PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure(rm);
	*data = measure;
	AddMeasure(measure);
}

/*
* Called once after Initialize
* Called before every Update if DynamicVariables=1 is defined
* Read options that can require dynamic variables here
*/
PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	Measure* measure = (Measure*)data;
	measure->Reload();
}

/*
* Called at every measure update
* Update your values here
*/
PLUGIN_EXPORT double Update(void* data)
{
	Measure* measure = (Measure*)data;
	return 0.0;
}


/* 
* Called everytime a [MeasureThisPlugin] is resolved
* DO NOT do any lengthy operations here, use Update for that
* Should only be used if you want the string value to be different than the numeric value
*/
PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	Measure* measure = (Measure*)data;
	std::unique_lock<std::mutex> lock(m_current_window_mutex);
	return m_current_window.title.c_str();
	// return nullptr;
}


/*
* Called once, at skin unload (a skin is unloaded when you Refresh it)
* Perform any necessary cleanups here
*/
PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;
	RemoveMeasure(measure);
	delete measure;
}

std::vector<std::wstring> Tokenize(std::wstring args) {
	std::vector<std::wstring> ret;
	size_t start = args.find_first_not_of(L' ');
	size_t end = args.find_first_of(L' ', start);
	while (end != args.npos) {
		ret.push_back(args.substr(start, end - 1));
		start = args.find_first_not_of(L' ', end);
		end = args.find_first_of(L' ', start);
	}
	if (start != args.npos) ret.push_back(args.substr(start));
	return ret;
}

std::wstring Join(std::vector<std::wstring> tokens) {
	std::wstring ret;
	for (auto token : tokens) {
		ret.append(token);
		ret.append(L" ");
	}
	return ret;
}

HWND GetSkinWindow(const wchar_t* configName)
{
	HWND trayWnd = FindWindow(L"RainmeterTrayClass", NULL);
	if (trayWnd)
	{
		COPYDATASTRUCT cds;
		cds.dwData = 5101;
		cds.cbData = (DWORD)(wcslen(configName) + 1) * sizeof(wchar_t);
		cds.lpData = (void*)configName;
		return (HWND)SendMessage(trayWnd, WM_COPYDATA, 0, (LPARAM)&cds);
	}

	return NULL;
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	Measure* measure = (Measure*)data;

	std::vector<std::wstring> tokens = Tokenize(args);
	HWND window = NULL;

	if (tokens.size() >= 2) {
		window = FindWindow(tokens[0].c_str(), tokens[1].c_str());
	}
	else if (tokens.size() == 1) {
		window = GetSkinWindow(tokens[0].c_str());
	}

	if (window) {
		int tries = 0;
		while (tries < 5) {
			if (SetForegroundWindow(window) && GetForegroundWindow() == window) break;
			++tries;
		}
	}
}

PLUGIN_EXPORT LPCWSTR CurrentTitle(void* data, const int argc, const WCHAR** argv)
{
	Measure* measure = (Measure*)data;
	std::unique_lock<std::mutex> lock(m_current_window_mutex);
	return m_current_window.title.c_str();
}

PLUGIN_EXPORT LPCWSTR CurrentBaseName(void* data, const int argc, const WCHAR** argv)
{
	Measure* measure = (Measure*)data;
	std::unique_lock<std::mutex> lock(m_current_window_mutex);
	return m_current_window.baseName.c_str();
}

PLUGIN_EXPORT LPCWSTR CurrentPath(void* data, const int argc, const WCHAR** argv)
{
	Measure* measure = (Measure*)data;
	std::unique_lock<std::mutex> lock(m_current_window_mutex);
	return m_current_window.path.c_str();
}

PLUGIN_EXPORT LPCWSTR CurrentWindowClass(void* data, const int argc, const WCHAR** argv)
{
	Measure* measure = (Measure*)data;
	std::unique_lock<std::mutex> lock(m_current_window_mutex);
	return m_current_window.windowClass.c_str();
}