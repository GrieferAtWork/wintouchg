/* Copyright (c) 2020-2021 Griefer@Work                                       *
 *                                                                            *
 * This software is provided 'as-is', without any express or implied          *
 * warranty. In no event will the authors be held liable for any damages      *
 * arising from the use of this software.                                     *
 *                                                                            *
 * Permission is granted to anyone to use this software for any purpose,      *
 * including commercial applications, and to alter it and redistribute it     *
 * freely, subject to the following restrictions:                             *
 *                                                                            *
 * 1. The origin of this software must not be misrepresented; you must not    *
 *    claim that you wrote the original software. If you use this software    *
 *    in a product, an acknowledgement (see the following) in the product     *
 *    documentation is required:                                              *
 *    Portions Copyright (c) 2020-2021 Griefer@Work                           *
 * 2. Altered source versions must be plainly marked as such, and must not be *
 *    misrepresented as being the original software.                          *
 * 3. This notice may not be removed or altered from any source distribution. *
 */
#ifndef GUARD_WINTOUCHG_MAIN_C
#define GUARD_WINTOUCHG_MAIN_C 1

#define WINVER 0x0602
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_SECURE_NO_WARNINGS 1

#include <Windows.h>
#include <Windowsx.h>
#include <dwmapi.h>
#include <endpointvolume.h>
#include <math.h>
#include <mmdeviceapi.h>
#include <psapi.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
//#include <wlanapi.h>
//#include <bluetoothapis.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
;

#undef LONG_MIN
#undef LONG_MAX
#define LONG_MIN (-2147483647L - 1)
#define LONG_MAX 2147483647L

#define RC_WIDTH(rc)  ((rc).right - (rc).left)
#define RC_HEIGHT(rc) ((rc).bottom - (rc).top)

#undef THIS_
#undef THIS
#define THIS_ INTERFACE *This,
#define THIS  INTERFACE *This

#undef DECLARE_INTERFACE
#define DECLARE_INTERFACE(iface)            \
	typedef struct iface iface;             \
	typedef struct iface##Vtbl iface##Vtbl; \
	struct iface {                          \
		struct iface##Vtbl const *lpVtbl;   \
	};                                      \
	struct iface##Vtbl

#undef STDMETHOD
#undef STDMETHOD_
#undef STDMETHODV
#undef STDMETHODV_
#define STDMETHOD(method)       HRESULT (STDMETHODCALLTYPE * method)
#define STDMETHOD_(type,method) type (STDMETHODCALLTYPE * method)
#define STDMETHODV(method)       HRESULT (STDMETHODVCALLTYPE * method)
#define STDMETHODV_(type,method) type (STDMETHODVCALLTYPE * method)


/************************************************************************/
/* Error handling and logging                                           */
/************************************************************************/
#if 0
#define HAVE_DEBUG_PRINTF
#define debug_printf(...) printf(__VA_ARGS__)
#else
#define debug_printf(...) (void)0
#endif

#define err(code, ...)                      \
	do {                                    \
		fprintf(stderr, "wintouchg: ");     \
		fprintf(stderr, __VA_ARGS__);       \
		fprintf(stderr, "\n");              \
		/* Sleep, because wintouchg is      \
		 * always launched in a separate,   \
		 * so if we crash, any error info   \
		 * would only blink for a second */ \
		for (;;)                            \
			Sleep(1000);                    \
		exit(code);                         \
	} while (0)

#undef assert
#define assert(expr)                                 \
	do {                                             \
		if (!(expr))                                 \
			err(1, "%s:%d: Assertion failed ('%s')", \
			    __FILE__, __LINE__, #expr);          \
	} while (0)

#define LOGERROR_PTR(function, ptr) \
	Wtg_LogErrorPtr(__func__, __LINE__, function, (void *)(uintptr_t)(ptr))
static void Wtg_LogErrorPtr(char const *caller, int line,
                            char const *function, void *ptr) {
	fprintf(stderr, "[error] In '%s:%d': Function '%s' returned error: %p\n",
	        caller, line, function, ptr);
}
#define LOGERROR_GLE(function) \
	Wtg_LogErrorGetLastError(__func__, __LINE__, function)
static void Wtg_LogErrorGetLastError(char const *caller, int line,
                                     char const *function) {
	fprintf(stderr, "[error] In '%s:%d': Function '%s' returned error: %lu\n",
	        caller, line, function, (unsigned long)GetLastError());
}
#define LOGERROR(...) \
	Wtg_LogError(__func__, __LINE__, __VA_ARGS__)
#define LOGERROR_CONTINUE(...) \
	fprintf(stderr, __VA_ARGS__)
static void Wtg_LogError(char const *caller, int line,
                         char const *format, ...) {
	va_list args;
	va_start(args, format);
	fprintf(stderr, "[error] In '%s:%d': ", caller, line);
	vfprintf(stderr, format, args);
	va_end(args);
}
static void Wtg_WarnMissingShLibFunction(HMODULE hModule, LPCSTR lpProcName) {
#ifndef MAX_PATH
#define MAX_PATH 260
#endif /* !MAX_PATH */
	char modName[MAX_PATH];
	DWORD dwError = GetLastError();
	if (!GetModuleFileNameA(hModule, modName, sizeof(modName)))
		strcpy(modName, "?");
	fprintf(stderr, "[warn] Shlib '%s': function '%s' not found (%lu)\n",
	        lpProcName, modName, (unsigned long)dwError);
}
/************************************************************************/




/* Max # of touch points to support */
#define WTG_MAX_TOUCH_INPUTS 256

/* Pixel distance threshold for gestures. */
#define WTG_GESTURE_SWIPE_THRESHOLD        50.0  /* Average distance traveled */
#define WTG_GESTURE_SWIPE_COMMIT_THRESHOLD 200.0 /* Min. distance for commit */
#define WTG_GESTURE_ZOOM_THRESHOLD         150.0 /* Average distance traveled */
#define WTG_GESTURE_ZOOM_COMMIT_THRESHOLD  200.0 /* Min. distance for commit */

/* Multi-touch gesture recognition bounds. */
#define WTG_GESTURE_STRT_TOUCH_COUNT 4 /* Begin a multi-touch gesture if >= this # of touch inputs are present */
#define WTG_GESTURE_STOP_TOUCH_COUNT 1 /* Stop a multi-touch gesture if <= this # of touch inputs remain */

static HINSTANCE hApplicationInstance;






/************************************************************************/
/* Dynamic system library hooks                                         */
/************************************************************************/
#define DEFINE_DYNAMIC_FUNCTION(return, cc, name, args) \
	typedef return (cc * LP##name) args;                \
	static LP##name pdyn_##name = NULL

/* Ole32 API. */
#undef CO_MTA_USAGE_COOKIE
#define CO_MTA_USAGE_COOKIE real_CO_MTA_USAGE_COOKIE
DECLARE_HANDLE(CO_MTA_USAGE_COOKIE);
DEFINE_DYNAMIC_FUNCTION(HRESULT, STDAPICALLTYPE, CoInitialize, (LPVOID pvReserved));
DEFINE_DYNAMIC_FUNCTION(HRESULT, STDAPICALLTYPE, CoCreateInstance, (REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv));
DEFINE_DYNAMIC_FUNCTION(void, STDAPICALLTYPE, CoUninitialize, (void));
DEFINE_DYNAMIC_FUNCTION(HRESULT, STDAPICALLTYPE, CoIncrementMTAUsage, (CO_MTA_USAGE_COOKIE * pCookie));
#define CoInitialize        (*pdyn_CoInitialize)
#define CoCreateInstance    (*pdyn_CoCreateInstance)
#define CoUninitialize      (*pdyn_CoUninitialize)
#define CoIncrementMTAUsage (*pdyn_CoIncrementMTAUsage)


/* User32 API. */
typedef struct {
	DWORD dwAttr;
	PVOID pData;
	ULONG dataSize;
} WINCOMPATTRDATA;
DEFINE_DYNAMIC_FUNCTION(BOOL, WINAPI, RegisterPointerInputTarget, (HWND hwnd, POINTER_INPUT_TYPE pointerType));
DEFINE_DYNAMIC_FUNCTION(BOOL, WINAPI, RegisterTouchWindow, (HWND hwnd, ULONG ulFlags));
DEFINE_DYNAMIC_FUNCTION(BOOL, WINAPI, InjectTouchInput, (UINT32 count, POINTER_TOUCH_INFO const *contacts));
DEFINE_DYNAMIC_FUNCTION(BOOL, WINAPI, InitializeTouchInjection, (UINT32 maxCount, DWORD dwMode));
DEFINE_DYNAMIC_FUNCTION(BOOL, WINAPI, GetPointerTouchInfo, (UINT32 pointerId, POINTER_TOUCH_INFO *touchInfo));
DEFINE_DYNAMIC_FUNCTION(BOOL, WINAPI, SetWindowCompositionAttribute, (HWND hwnd, WINCOMPATTRDATA *pAttrData));
#define RegisterPointerInputTarget    (*pdyn_RegisterPointerInputTarget)
#define RegisterTouchWindow           (*pdyn_RegisterTouchWindow)
#define InjectTouchInput              (*pdyn_InjectTouchInput)
#define InitializeTouchInjection      (*pdyn_InitializeTouchInjection)
#define GetPointerTouchInfo           (*pdyn_GetPointerTouchInfo)
#define SetWindowCompositionAttribute (*pdyn_SetWindowCompositionAttribute)

/* DWM API. */
DEFINE_DYNAMIC_FUNCTION(HRESULT, STDAPICALLTYPE, DwmRegisterThumbnail, (HWND hwndDestination, HWND hwndSource, PHTHUMBNAIL phThumbnailId));
DEFINE_DYNAMIC_FUNCTION(HRESULT, STDAPICALLTYPE, DwmUnregisterThumbnail, (HTHUMBNAIL hThumbnailId));
DEFINE_DYNAMIC_FUNCTION(HRESULT, STDAPICALLTYPE, DwmUpdateThumbnailProperties, (HTHUMBNAIL hThumbnailId, DWM_THUMBNAIL_PROPERTIES const *ptnProperties));
DEFINE_DYNAMIC_FUNCTION(HRESULT, STDAPICALLTYPE, DwmQueryThumbnailSourceSize, (HTHUMBNAIL hThumbnailId, PSIZE pSize));
DEFINE_DYNAMIC_FUNCTION(HRESULT, STDAPICALLTYPE, DwmGetWindowAttribute, (HWND hwnd, DWORD dwAttribute, PVOID pvAttribute, DWORD cbAttribute));
DEFINE_DYNAMIC_FUNCTION(HRESULT, STDAPICALLTYPE, DwmEnableBlurBehindWindow, (HWND hWnd, DWM_BLURBEHIND const *pBlurBehind));
DEFINE_DYNAMIC_FUNCTION(HRESULT, STDAPICALLTYPE, DwmExtendFrameIntoClientArea, (HWND hWnd, MARGINS const *pMarInset));
#define DwmRegisterThumbnail         (*pdyn_DwmRegisterThumbnail)
#define DwmUnregisterThumbnail       (*pdyn_DwmUnregisterThumbnail)
#define DwmUpdateThumbnailProperties (*pdyn_DwmUpdateThumbnailProperties)
#define DwmQueryThumbnailSourceSize  (*pdyn_DwmQueryThumbnailSourceSize)
#define DwmGetWindowAttribute        (*pdyn_DwmGetWindowAttribute)
#define DwmEnableBlurBehindWindow    (*pdyn_DwmEnableBlurBehindWindow)
#define DwmExtendFrameIntoClientArea (*pdyn_DwmExtendFrameIntoClientArea)

#define DEFINE_LAZY_LIBRARY_LOADER(pdyn_LibraryAPI, GetLibraryHandle, LIBNAME, LibDllName) \
	static HMODULE pdyn_LibraryAPI = NULL;                                                 \
	static HMODULE GetLibraryHandle(void) {                                                \
		HMODULE hResult = pdyn_LibraryAPI;                                                 \
		if (hResult) {                                                                     \
			if (hResult == (HMODULE)-1)                                                    \
				hResult = NULL;                                                            \
		} else {                                                                           \
			hResult = GetModuleHandleW(LIBNAME);                                           \
			if (!hResult) {                                                                \
				hResult = LoadLibraryW(LibDllName);                                        \
				if (!hResult)                                                              \
					hResult = (HMODULE)-1;                                                 \
			}                                                                              \
			pdyn_LibraryAPI = hResult;                                                     \
		}                                                                                  \
		return hResult;                                                                    \
	}
DEFINE_LAZY_LIBRARY_LOADER(pdyn_Ole32, DynApi_GetOld32Handle, L"OLE32", L"Ole32.dll");
DEFINE_LAZY_LIBRARY_LOADER(pdyn_User32, DynApi_GetUser32Handle, L"USER32", L"User32.dll");
DEFINE_LAZY_LIBRARY_LOADER(pdyn_DwmAPI, DynApi_GetDwmApiHandle, L"DWMAPI", L"dwmapi.dll");
#undef DEFINE_LAZY_LIBRARY_LOADER

static FARPROC DynApi_GetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
	FARPROC result = GetProcAddress(hModule, lpProcName);
	if (!result)
		Wtg_WarnMissingShLibFunction(hModule, lpProcName);
	return result;
}

#define DynApi_TryLoadFunction(hLibrary, Name) \
	(pdyn_##Name = (LP##Name)DynApi_GetProcAddress(hLibrary, #Name))
#define DynApi_LoadFunction(hLibrary, Name)                             \
	do {                                                                \
		pdyn_##Name = (LP##Name)DynApi_GetProcAddress(hLibrary, #Name); \
		if (!pdyn_##Name)                                               \
			goto fail;                                                  \
	} while (0)


/* Ensure that the given API is available (returning `true' if it is, and `false' otherwise) */
static bool DynApi_InitializeOld32(void) {
	HMODULE hOle32 = DynApi_GetOld32Handle();
	if (!hOle32)
		goto fail;
	DynApi_LoadFunction(hOle32, CoInitialize);
	DynApi_LoadFunction(hOle32, CoCreateInstance);
	DynApi_LoadFunction(hOle32, CoUninitialize);
	DynApi_TryLoadFunction(hOle32, CoIncrementMTAUsage);
	return true;
fail:
	return false;
}

static bool DynApi_InitializeUser32(void) {
	HMODULE hUser32 = DynApi_GetUser32Handle();
	if (!hUser32)
		goto fail;
	DynApi_LoadFunction(hUser32, RegisterPointerInputTarget);
	DynApi_LoadFunction(hUser32, RegisterTouchWindow);
	DynApi_LoadFunction(hUser32, InjectTouchInput);
	DynApi_LoadFunction(hUser32, InitializeTouchInjection);
	DynApi_LoadFunction(hUser32, GetPointerTouchInfo);
	DynApi_TryLoadFunction(hUser32, SetWindowCompositionAttribute);
	return true;
fail:
	return false;
}

static bool DynApi_InitializeDwm(void) {
	HMODULE hDwmApi = DynApi_GetDwmApiHandle();
	if (!hDwmApi)
		goto fail;
	/* Load API hooks. */
	DynApi_LoadFunction(hDwmApi, DwmRegisterThumbnail);
	DynApi_LoadFunction(hDwmApi, DwmUnregisterThumbnail);
	DynApi_LoadFunction(hDwmApi, DwmUpdateThumbnailProperties);
	DynApi_LoadFunction(hDwmApi, DwmQueryThumbnailSourceSize);
	/* Optional functions. */
	DynApi_TryLoadFunction(hDwmApi, DwmGetWindowAttribute);
	DynApi_TryLoadFunction(hDwmApi, DwmEnableBlurBehindWindow);
	DynApi_TryLoadFunction(hDwmApi, DwmExtendFrameIntoClientArea);
	return true;
fail:
	return false;
}
/************************************************************************/







/************************************************************************/
/* System settings helper functions                                     */
/************************************************************************/
#define _DISPLAY_BRIGHTNESS _real__DISPLAY_BRIGHTNESS
#define DISPLAY_BRIGHTNESS  _real_DISPLAY_BRIGHTNESS
#define PDISPLAY_BRIGHTNESS _real_PDISPLAY_BRIGHTNESS
typedef struct _DISPLAY_BRIGHTNESS {
	UCHAR ucDisplayPolicy;
	UCHAR ucACBrightness;
	UCHAR ucDCBrightness;
} DISPLAY_BRIGHTNESS, *PDISPLAY_BRIGHTNESS;
/* XXX: Microsoft says that this API is deprecated, but the replacement
 *      uses yet another one of those really weird APIs, and somehow even
 *      seems related to powershell... */
#ifndef IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS
#define IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS \
	CTL_CODE(FILE_DEVICE_VIDEO, 0x126, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif /* !IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS */
#ifndef IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS
#define IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS \
	CTL_CODE(FILE_DEVICE_VIDEO, 0x127, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif /* !IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS */
#ifndef DISPLAYPOLICY_AC
#define DISPLAYPOLICY_AC   0x00000001
#endif /* !DISPLAYPOLICY_AC */
#ifndef DISPLAYPOLICY_DC
#define DISPLAYPOLICY_DC   0x00000002
#endif /* !DISPLAYPOLICY_DC */
#ifndef DISPLAYPOLICY_BOTH
#define DISPLAYPOLICY_BOTH 0x00000003
#endif /* !DISPLAYPOLICY_BOTH */
static HANDLE SysIntern_OpenLCD(void) {
	HANDLE hResult;
	hResult = CreateFileW(L"\\\\.\\LCD", GENERIC_READ | GENERIC_WRITE,
	                      0, NULL, OPEN_EXISTING, 0, NULL);
	if (hResult == NULL || hResult == INVALID_HANDLE_VALUE)
		LOGERROR_GLE("CreateFileW(\"\\\\.\\LCD\")");
	return hResult;
}
static BOOL SysIntern_GetDisplayBrightness(DISPLAY_BRIGHTNESS *__restrict pDb) {
	BOOL bResult;
	DWORD ret = 0;
	HANDLE hLCD = SysIntern_OpenLCD();
	if (hLCD == NULL || hLCD == INVALID_HANDLE_VALUE)
		return FALSE;
	memset(pDb, 0, sizeof(*pDb));
	bResult = DeviceIoControl(hLCD, IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS,
	                          NULL, 0, (LPVOID)pDb, sizeof(*pDb), &ret, NULL);
	if (!bResult)
		LOGERROR_GLE("DeviceIoControl()");
	CloseHandle(hLCD);
	return bResult;
}
static BOOL SysIntern_SetDisplayBrightness(DISPLAY_BRIGHTNESS const *__restrict pDb) {
	BOOL bResult;
	DWORD ret = 0;
	HANDLE hLCD = SysIntern_OpenLCD();
	if (hLCD == NULL || hLCD == INVALID_HANDLE_VALUE)
		return FALSE;
	bResult = DeviceIoControl(hLCD, IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS,
	                          (LPVOID)pDb, sizeof(*pDb), NULL, 0, &ret, NULL);
	if (!bResult)
		LOGERROR_GLE("DeviceIoControl()");
	CloseHandle(hLCD);
	return bResult;
}
static LONG SysIntern_GetBrightnessPercent(void) {
	DISPLAY_BRIGHTNESS db;
	if (!SysIntern_GetDisplayBrightness(&db))
		return -1;
	if (db.ucDisplayPolicy & DISPLAYPOLICY_AC)
		return db.ucACBrightness;
	if (db.ucDisplayPolicy & DISPLAYPOLICY_DC)
		return db.ucDCBrightness;
	LOGERROR("Unsupported db.ucDisplayPolicy: %#lx\n", db.ucDisplayPolicy);
	return -1;
}
static bool SysIntern_SetBrightnessPercent(LONG newval) {
	DISPLAY_BRIGHTNESS db;
	if (!SysIntern_GetDisplayBrightness(&db))
		return false;
	if (newval < 0)
		newval = 0;
	if (newval > 100)
		newval = 100;
	if (db.ucDisplayPolicy & DISPLAYPOLICY_AC)
		db.ucACBrightness = newval;
	else if (db.ucDisplayPolicy & DISPLAYPOLICY_DC)
		db.ucDCBrightness = newval;
	else {
		LOGERROR("Unsupported db.ucDisplayPolicy: %#lx\n", db.ucDisplayPolicy);
		return false;
	}
	return SysIntern_SetDisplayBrightness(&db);
}

/* Get/Set the current brightness as a float-value between 0.0 and 1.0 */
static double last_system_brightness = -1.0;
static double Sys_GetBrightness(void) {
	if (last_system_brightness < 0.0)
		last_system_brightness = (double)SysIntern_GetBrightnessPercent() / 100.0;
	return last_system_brightness;
}
static void Sys_SetBrightness(double value) {
	if (value < 0.0)
		value = 0.0;
	if (value > 1.0)
		value = 1.0;
	if (value != last_system_brightness) {
		SysIntern_SetBrightnessPercent((DWORD)(value * 100.0));
		last_system_brightness = value;
	}
}


#define DEFINE_UUID(name, a, b, c, d, e)          \
	static GUID const name = {                    \
		0x##a##u, 0x##b##u, 0x##c##u,             \
		{ (0x##d##u & 0xff00u) >> 8,              \
		  (0x##d##u & 0xffu),                     \
		  (0x##e##ull & 0xff0000000000ull) >> 40, \
		  (0x##e##ull & 0xff00000000ull) >> 32,   \
		  (0x##e##ull & 0xff000000ull) >> 24,     \
		  (0x##e##ull & 0xff0000ull) >> 16,       \
		  (0x##e##ull & 0xff00ull) >> 8,          \
		  (0x##e##ull & 0xffull) }                \
	}

DEFINE_UUID(UUID_MMDeviceEnumerator, BCDE0395, E52F, 467C, 8E3D, C4579291692E);
DEFINE_UUID(UUID_IMMDeviceEnumerator, A95664D2, 9614, 4F35, A746, DE8DB63617E6);
DEFINE_UUID(UUID_IAudioEndpointVolume, 5CDF2C82, 841E, 4546, 9722, 0CF74078229A);

static IAudioEndpointVolume *SysIntern_VolumeControllerAcquire(void) {
	HRESULT hr;
	IMMDevice *defaultDevice;
	IMMDeviceEnumerator *deviceEnumerator;
	IAudioEndpointVolume *endpointVolume;
	if (!pdyn_CoInitialize || !pdyn_CoCreateInstance) {
		if (!DynApi_InitializeOld32()) {
			LOGERROR_GLE("DynApi_InitializeOld32()");
			goto err;
		}
	}

	deviceEnumerator = NULL;
	defaultDevice    = NULL;
	endpointVolume   = NULL;

	hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		LOGERROR_PTR("CoInitialize()", hr);
		goto err;
	}
	hr = CoCreateInstance(&UUID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER,
	                      &UUID_IMMDeviceEnumerator, (LPVOID *)&deviceEnumerator);
	if (FAILED(hr) || !deviceEnumerator) {
		LOGERROR_PTR("CoCreateInstance(UUID_MMDeviceEnumerator, UUID_IMMDeviceEnumerator)", hr);
		goto err2;
	}
	hr = deviceEnumerator->lpVtbl->GetDefaultAudioEndpoint(deviceEnumerator, eRender,
	                                                       eConsole, &defaultDevice);
	deviceEnumerator->lpVtbl->Release(deviceEnumerator);
	if (FAILED(hr) || !defaultDevice) {
		LOGERROR_PTR("IMMDeviceEnumerator::GetDefaultAudioEndpoint()", hr);
		goto err2;
	}
	hr = defaultDevice->lpVtbl->Activate(defaultDevice, &UUID_IAudioEndpointVolume,
	                                     CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
	defaultDevice->lpVtbl->Release(defaultDevice);
	if (FAILED(hr) || !endpointVolume) {
		LOGERROR_PTR("IMMDevice::Activate()", hr);
		goto err2;
	}
	return endpointVolume;
err2:
	if (pdyn_CoUninitialize)
		CoUninitialize();
err:
	return NULL;
}

static void SysIntern_VolumeControllerRelease(IAudioEndpointVolume *self) {
	if (!self)
		return;
	self->lpVtbl->Release(self);
	if (pdyn_CoUninitialize)
		CoUninitialize();
}


static float last_system_volume = -1.0;
static float Sys_GetVolume(void) {
	if (last_system_volume < 0.0f) {
		IAudioEndpointVolume *endpointVolume;
		endpointVolume = SysIntern_VolumeControllerAcquire();
		if (endpointVolume) {
			HRESULT hr;
			hr = endpointVolume->lpVtbl->GetMasterVolumeLevelScalar(endpointVolume, &last_system_volume);
			SysIntern_VolumeControllerRelease(endpointVolume);
			if (FAILED(hr))
				LOGERROR_PTR("IAudioEndpointVolume::GetMasterVolumeLevelScalar()", hr);
		}
	}
	return last_system_volume;
}

static void Sys_SetVolume(float value) {
	IAudioEndpointVolume *endpointVolume;
	if (value < 0.0f)
		value = 0.0f;
	if (value > 1.0f)
		value = 1.0f;
	if (value == last_system_volume)
		return;
	endpointVolume = SysIntern_VolumeControllerAcquire();
	if (endpointVolume) {
		HRESULT hr;
		hr = endpointVolume->lpVtbl->SetMasterVolumeLevelScalar(endpointVolume, value, NULL);
		SysIntern_VolumeControllerRelease(endpointVolume);
		if (FAILED(hr))
			LOGERROR_PTR("IAudioEndpointVolume::SetMasterVolumeLevelScalar()", hr);
	}
	last_system_volume = value;
}


/* RADIO Access (Bluetooth / Wifi) */
typedef DWORD RADIOACCESSSTATUS;
#define RADIOACCESSSTATUS_UNSPECIFIED      0
#define RADIOACCESSSTATUS_ALLOWED          1
#define RADIOACCESSSTATUS_DENIED_BY_USER   2
#define RADIOACCESSSTATUS_DENIED_BY_SYSTEM 3
typedef DWORD RADIOKIND;
#define RADIOKIND_OTHER            0
#define RADIOKIND_WIFI             1
#define RADIOKIND_MOBILE_BROADBAND 2
#define RADIOKIND_BLUETOOTH        3
#define RADIOKIND_FM               4
typedef DWORD RADIOSTATE;
#define RADIOSTATE_UNKNOWN  0
#define RADIOSTATE_ON       1
#define RADIOSTATE_OFF      2
#define RADIOSTATE_DISABLED 3

#undef IInspectable
#undef IID_IUnknown
#undef IID_IAgileObject
#undef IID_IRadioStatics
#undef HSTRING
#undef HSTRING_HEADER
#define IInspectable      real_IInspectable
#define IID_IUnknown      real_IID_IUnknown
#define IID_IAgileObject  real_IID_IAgileObject
#define IID_IRadioStatics real_IID_IRadioStatics
#define HSTRING           real_HSTRING
#define HSTRING_HEADER    real_HSTRING_HEADER

DECLARE_HANDLE(HSTRING);

typedef struct {
	union {
		PVOID Reserved1;
#ifdef _WIN64
		char Reserved2[24];
#else /* _WIN64 */
		char Reserved2[20];
#endif /* !_WIN64 */
	} Reserved;
} HSTRING_HEADER;

static GUID const IID_IUnknown                                         = { 0x00000000, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static GUID const IID_IAgileObject                                     = { 0x94ea2b94, 0xe9cc, 0x49e0, { 0xc0, 0xff, 0xee, 0x64, 0xca, 0x8f, 0x5b, 0x90 } };
static GUID const IID_IRadioStatics                                    = { 0x5fb6a12e, 0x67cb, 0x46ae, { 0xaa, 0xe9, 0x65, 0x91, 0x9f, 0x86, 0xef, 0xf4 } };
static GUID const IID_AsyncOperationCompletedHandler_IVectorView_Radio = { 0xd30691e6, 0x60a0, 0x59c9, { 0x89, 0x65, 0x5b, 0xbe, 0x28, 0x2e, 0x82, 0x08 } };
static GUID const IID_AsyncOperationCompletedHandler_RadioAccessStatus = { 0xbd248e73, 0xf05f, 0x574c, { 0xae, 0x3d, 0x9b, 0x95, 0xc4, 0xbf, 0x28, 0x2a } };

/* From "api-ms-win-core-winrt-string-l1-1-0.dll": */
DEFINE_DYNAMIC_FUNCTION(HRESULT, STDAPICALLTYPE, WindowsCreateStringReference, (PCWSTR sourceString, UINT32 length, HSTRING_HEADER *hstringHeader, HSTRING *string));
DEFINE_DYNAMIC_FUNCTION(HRESULT, STDAPICALLTYPE, WindowsDeleteString, (HSTRING string));
#define WindowsDeleteString          (*pdyn_WindowsDeleteString)
#define WindowsCreateStringReference (*pdyn_WindowsCreateStringReference)

/* From "api-ms-win-core-winrt-l1-1-0.dll": */
DEFINE_DYNAMIC_FUNCTION(HRESULT, WINAPI, RoGetActivationFactory, (HSTRING activatableClassId, GUID const *iid, void** factory));
#define RoGetActivationFactory (*pdyn_RoGetActivationFactory)

static HMODULE hModule_api_ms_win_core_winrt_string_l1_1_0; /* "api-ms-win-core-winrt-string-l1-1-0.dll" */
static HMODULE hModule_api_ms_win_core_winrt_l1_1_0;        /* "api-ms-win-core-winrt-l1-1-0.dll" */

static void SysIntern_FreeMsWinCoreWinRtAPIS(void) {
#define X_FreeLibrary(x) ((x) && (FreeLibrary(x), (x) = NULL, 1))
	X_FreeLibrary(hModule_api_ms_win_core_winrt_string_l1_1_0);
	X_FreeLibrary(hModule_api_ms_win_core_winrt_l1_1_0);
#undef X_FreeLibrary
}

static bool SysIntern_LoadMsWinCoreWinRtAPIS(void) {
	hModule_api_ms_win_core_winrt_string_l1_1_0 = LoadLibraryW(L"api-ms-win-core-winrt-string-l1-1-0.dll");
	if (!hModule_api_ms_win_core_winrt_string_l1_1_0) {
		LOGERROR_GLE("LoadLibraryW(L\"api-ms-win-core-winrt-string-l1-1-0.dll\")");
		goto fail;
	}
	hModule_api_ms_win_core_winrt_l1_1_0 = LoadLibraryW(L"api-ms-win-core-winrt-l1-1-0.dll");
	if (!hModule_api_ms_win_core_winrt_l1_1_0) {
		LOGERROR_GLE("LoadLibraryW(L\"api-ms-win-core-winrt-l1-1-0.dll\")");
		goto fail;
	}
	DynApi_LoadFunction(hModule_api_ms_win_core_winrt_string_l1_1_0, WindowsDeleteString);
	DynApi_LoadFunction(hModule_api_ms_win_core_winrt_string_l1_1_0, WindowsCreateStringReference);
	DynApi_LoadFunction(hModule_api_ms_win_core_winrt_l1_1_0, RoGetActivationFactory);
	return true;
fail:
	SysIntern_FreeMsWinCoreWinRtAPIS();
	return false;
}

#undef INTERFACE
#define INTERFACE IInspectable
DECLARE_INTERFACE(IInspectable) {
	/* IUnknown */
	STDMETHOD(QueryInterface)(THIS_ GUID const *riid, LPVOID FAR * ppvObj);
	STDMETHOD_(ULONG, AddRef)(THIS);
	STDMETHOD_(ULONG, Release)(THIS);
	/* IInspectable */
	STDMETHOD(GetIids)(THIS_ DWORD *count, GUID **ids);
	STDMETHOD(GetRuntimeClassName)(THIS_ void **name);
	STDMETHOD(GetTrustLevel)(THIS_ /*Windows::Foundation::TrustLevel*/ int *level);
};

#undef INTERFACE
#define INTERFACE IRadioStatics
DECLARE_INTERFACE(IRadioStatics) {
	/* IUnknown */
	STDMETHOD(QueryInterface)(THIS_ GUID const *riid, LPVOID FAR * ppvObj);
	STDMETHOD_(ULONG, AddRef)(THIS);
	STDMETHOD_(ULONG, Release)(THIS);
	/* IInspectable */
	STDMETHOD(GetIids)(THIS_ DWORD *count, GUID **ids);
	STDMETHOD(GetRuntimeClassName)(THIS_ void **name);
	STDMETHOD(GetTrustLevel)(THIS_ /*Windows::Foundation::TrustLevel*/ int *level);
	/* IRadioStatics */
	STDMETHOD(GetRadiosAsync)(THIS_ void **value);
	STDMETHOD(GetDeviceSelector)(THIS_ void **deviceSelector);
	STDMETHOD(FromIdAsync)(THIS_ void *deviceId, void **value);
	STDMETHOD(RequestAccessAsync)(THIS_ void **value);
};

#undef INTERFACE
#define INTERFACE IAsyncOperation
DECLARE_INTERFACE(IAsyncOperation) {
	/* IUnknown */
	STDMETHOD(QueryInterface)(THIS_ GUID const *riid, LPVOID FAR * ppvObj);
	STDMETHOD_(ULONG, AddRef)(THIS);
	STDMETHOD_(ULONG, Release)(THIS);
	/* IInspectable */
	STDMETHOD(GetIids)(THIS_ DWORD *count, GUID **ids);
	STDMETHOD(GetRuntimeClassName)(THIS_ void **name);
	STDMETHOD(GetTrustLevel)(THIS_ /*Windows::Foundation::TrustLevel*/ int *level);
	/* IAsyncOperation */
	STDMETHOD(put_Completed)(THIS_ void *handler);
	STDMETHOD(get_Completed)(THIS_ void **handler);
	STDMETHOD(GetResults)(THIS_ void **results);
};

#undef INTERFACE
#define INTERFACE IAsyncCompletionHandler
DECLARE_INTERFACE(IAsyncCompletionHandler) {
	/* IUnknown */
	STDMETHOD(QueryInterface)(THIS_ GUID const *riid, LPVOID FAR * ppvObj);
	STDMETHOD_(ULONG, AddRef)(THIS);
	STDMETHOD_(ULONG, Release)(THIS);
	/* IAsyncCompletionHandler */
	STDMETHOD(Invoke)(THIS_ void *asyncInfo, /*winrt::Windows::Foundation::AsyncStatus*/ int status);
};

#undef INTERFACE
#define INTERFACE IVectorView
DECLARE_INTERFACE(IVectorView) {
	/* IUnknown */
	STDMETHOD(QueryInterface)(THIS_ GUID const *riid, LPVOID FAR * ppvObj);
	STDMETHOD_(ULONG, AddRef)(THIS);
	STDMETHOD_(ULONG, Release)(THIS);
	/* IInspectable */
	STDMETHOD(GetIids)(THIS_ DWORD *count, GUID **ids);
	STDMETHOD(GetRuntimeClassName)(THIS_ void **name);
	STDMETHOD(GetTrustLevel)(THIS_ /*Windows::Foundation::TrustLevel*/ int *level);
	/* IVectorView */
	STDMETHOD(GetAt)(THIS_ DWORD index, void **item);
	STDMETHOD(get_Size)(THIS_ DWORD * size);
	STDMETHOD(IndexOf)(THIS_ void *value, DWORD *index, bool *found);
	STDMETHOD(GetMany)(THIS_ DWORD startIndex, DWORD capacity, void **value, DWORD *actual);
};

#undef INTERFACE
#define INTERFACE IRadio
DECLARE_INTERFACE(IRadio) {
	/* IUnknown */
	STDMETHOD(QueryInterface)(THIS_ GUID const *riid, LPVOID FAR * ppvObj);
	STDMETHOD_(ULONG, AddRef)(THIS);
	STDMETHOD_(ULONG, Release)(THIS);
	/* IInspectable */
	STDMETHOD(GetIids)(THIS_ DWORD *count, GUID **ids);
	STDMETHOD(GetRuntimeClassName)(THIS_ void **name);
	STDMETHOD(GetTrustLevel)(THIS_ /*Windows::Foundation::TrustLevel*/ int *level);
	/* IRadio */
	STDMETHOD(SetStateAsync)(THIS_ RADIOSTATE value, void **retval);
	STDMETHOD(add_StateChanged)(THIS_ void *handler, INT64 *eventCookie);
	STDMETHOD(remove_StateChanged)(THIS_ INT64 eventCookie);
	STDMETHOD(get_State)(THIS_ RADIOSTATE *value);
	STDMETHOD(get_Name)(THIS_ void **value);
	STDMETHOD(get_Kind)(THIS_ RADIOKIND *value);
};

static IRadioStatics *SysIntern_GetRadioStaticsActivationFactory(void) {
	IRadioStatics *result = NULL;
	HSTRING_HEADER hsHdr;
	HSTRING hstr;
	HRESULT hr;
	hr = pdyn_WindowsCreateStringReference(L"Windows.Devices.Radios.Radio",
	                                       28, &hsHdr, &hstr);
	if (FAILED(hr)) {
		LOGERROR_PTR("WindowsCreateStringReference()", hr);
		goto done;
	}
	hr = pdyn_RoGetActivationFactory(hstr, &IID_IRadioStatics, (void **)&result);
	if (hr == CO_E_NOTINITIALIZED) {
		if (!pdyn_CoIncrementMTAUsage)
			DynApi_InitializeOld32();
		if (pdyn_CoIncrementMTAUsage) {
			CO_MTA_USAGE_COOKIE cookie;
			CoIncrementMTAUsage(&cookie);
			hr = pdyn_RoGetActivationFactory(hstr, &IID_IRadioStatics, (void **)&result);
		}
	}
	pdyn_WindowsDeleteString(hstr);
	if (FAILED(hr) || !result) {
		LOGERROR_PTR("RoGetActivationFactory()", hr);
		result = NULL;
	}
done:
	return result;
}

typedef struct {
	struct IAsyncCompletionHandlerVtbl const *lpVtbl;          /* [1..1] V-table. */
	LONG                                      achc_refcnt;     /* Reference counter */
	GUID const                               *achc_intf_guid;  /* Interface GUID */
	HANDLE                                    achc_hsem;       /* Semaphore */
} SysIntern_IAsyncCompletionHandlerCallback;

static ULONG STDMETHODCALLTYPE
SysIntern_IAsyncCompletionHandlerCallback_AddRef(IAsyncCompletionHandler *self) {
	SysIntern_IAsyncCompletionHandlerCallback *me;
	me = (SysIntern_IAsyncCompletionHandlerCallback *)self;
	return InterlockedIncrement(&me->achc_refcnt);
}

static ULONG STDMETHODCALLTYPE
SysIntern_IAsyncCompletionHandlerCallback_Release(IAsyncCompletionHandler *self) {
	ULONG ulResult;
	SysIntern_IAsyncCompletionHandlerCallback *me;
	me = (SysIntern_IAsyncCompletionHandlerCallback *)self;
	ulResult = InterlockedDecrement(&me->achc_refcnt);
	if (ulResult == 0) {
		CloseHandle(me->achc_hsem);
		free(me);
	}
	return ulResult;
}

static HRESULT STDMETHODCALLTYPE
SysIntern_IAsyncCompletionHandlerCallback_QueryInterface(IAsyncCompletionHandler *self,
                                                         GUID const *riid, LPVOID *ppvObj) {
	SysIntern_IAsyncCompletionHandlerCallback *me;
	me = (SysIntern_IAsyncCompletionHandlerCallback *)self;
	if (memcmp(riid, me->achc_intf_guid, sizeof(GUID)) == 0 ||
	    memcmp(riid, &IID_IUnknown, sizeof(GUID)) == 0 ||
	    memcmp(riid, &IID_IAgileObject, sizeof(GUID)) == 0) {
		*ppvObj = me;
		SysIntern_IAsyncCompletionHandlerCallback_AddRef(self);
		return S_OK;
	}
	*ppvObj = NULL;
	return E_NOINTERFACE;
}

static HRESULT STDMETHODCALLTYPE
SysIntern_IAsyncCompletionHandlerCallback_Invoke(IAsyncCompletionHandler *self,
                                                 void *asyncInfo, int status) {
	SysIntern_IAsyncCompletionHandlerCallback *me;
	(void)asyncInfo;
	(void)status;
	me = (SysIntern_IAsyncCompletionHandlerCallback *)self;
	ReleaseSemaphore(me->achc_hsem, 1, NULL);
	return S_OK;
}

static struct IAsyncCompletionHandlerVtbl const SysIntern_IAsyncCompletionHandlerCallback_VT = {
	&SysIntern_IAsyncCompletionHandlerCallback_QueryInterface,
	&SysIntern_IAsyncCompletionHandlerCallback_AddRef,
	&SysIntern_IAsyncCompletionHandlerCallback_Release,
	&SysIntern_IAsyncCompletionHandlerCallback_Invoke,
};

static HRESULT
SysIntern_WaitForIAsyncOperation(IAsyncOperation *__restrict iaop,
                                 GUID const *__restrict intf_guid) {
	DWORD dwError;
	HRESULT hr;
	SysIntern_IAsyncCompletionHandlerCallback *cb;
	cb = (SysIntern_IAsyncCompletionHandlerCallback *)malloc(
	/* */ sizeof(SysIntern_IAsyncCompletionHandlerCallback));
	if (!cb) {
		LOGERROR("malloc(SysIntern_IAsyncCompletionHandlerCallback) failed\n");
		return E_OUTOFMEMORY;
	}
	cb->achc_hsem = CreateSemaphoreW(NULL, 0, 1, NULL);
	if (!cb->achc_hsem) {
		LOGERROR_GLE("CreateSemaphoreW()");
		hr = E_OUTOFMEMORY;
		goto err_cb;
	}
	cb->achc_intf_guid = intf_guid;
	cb->achc_refcnt    = 1;
	cb->lpVtbl         = &SysIntern_IAsyncCompletionHandlerCallback_VT;
	/* Register the completion callback. */
	hr = iaop->lpVtbl->put_Completed(iaop, cb);
	if (FAILED(hr)) {
		LOGERROR_PTR("IAsyncOperation::put_Completed()", hr);
		goto err_cb2;
	}
	/* Wait for the completion callback to be invoked. */
	dwError = WaitForSingleObject(cb->achc_hsem, INFINITE);
	if (dwError == WAIT_FAILED) {
		LOGERROR_GLE("WaitForSingleObject()");
		hr = E_UNEXPECTED;
	}
	cb->lpVtbl->Release((IAsyncCompletionHandler *)cb);
	return hr;
err_cb2:
	CloseHandle(cb->achc_hsem);
err_cb:
	free(cb);
	return hr;
}

static IVectorView *SysIntern_GetRadioList(bool requestAccess) {
	IVectorView *result = NULL;
	IRadioStatics *iRst;
	IAsyncOperation *iAsync = NULL;
	HRESULT hr;
	iRst = SysIntern_GetRadioStaticsActivationFactory();
	if (!iRst)
		goto done;
	if (requestAccess) {
		hr = iRst->lpVtbl->RequestAccessAsync(iRst, (void **)&iAsync);
		if (FAILED(hr) || !iAsync) {
			LOGERROR_PTR("IRadioStatics::RequestAccessAsync()", hr);
		} else {
			RADIOACCESSSTATUS acc;
			SysIntern_WaitForIAsyncOperation(iAsync, &IID_AsyncOperationCompletedHandler_RadioAccessStatus);
			acc = RADIOACCESSSTATUS_UNSPECIFIED;
			/* Check operation status and log an error if it failed. */
			hr = iAsync->lpVtbl->GetResults(iAsync, (void **)&acc);
			if (FAILED(hr)) {
				LOGERROR_PTR("IRadioStatics::RequestAccessAsync()::GetResults()", hr);
			} else if (acc != RADIOACCESSSTATUS_ALLOWED) {
				LOGERROR("IRadioStatics::RequestAccessAsync() returned access error %u\n", acc);
			}
	 		iAsync->lpVtbl->Release(iAsync);
		}
		iAsync = NULL;
	}
	hr = iRst->lpVtbl->GetRadiosAsync(iRst, (void **)&iAsync);
	iRst->lpVtbl->Release(iRst);;
	if (FAILED(hr) || !iAsync) {
		LOGERROR_PTR("IRadioStatics::GetRadiosAsync()", hr);
		goto done;
	}
	hr = SysIntern_WaitForIAsyncOperation(iAsync, &IID_AsyncOperationCompletedHandler_IVectorView_Radio);
	if (FAILED(hr))
		goto done;
	hr = iAsync->lpVtbl->GetResults(iAsync, (void **)&result);
	iAsync->lpVtbl->Release(iAsync);
	if (FAILED(hr) || !result) {
		LOGERROR_PTR("IAsyncOperation::GetResults()", hr);
		result = NULL;
	}
done:
	return result;
}

/* High-level does-everything functions */
static RADIOSTATE SysIntern_GetRadioState(RADIOKIND rkKind) {
	DWORD i, dwSize = 0;
	IVectorView *rdList;
	HRESULT hr;
	RADIOSTATE result = RADIOSTATE_UNKNOWN;
	if (!SysIntern_LoadMsWinCoreWinRtAPIS())
		goto done;
	rdList = SysIntern_GetRadioList(false);
	if (!rdList)
		goto done;
	hr = rdList->lpVtbl->get_Size(rdList, &dwSize);
	if (FAILED(hr)) {
		LOGERROR_PTR("IVectorView::get_Size()", hr);
		goto done;
	}
	for (i = 0; i < dwSize; ++i) {
		IRadio *rdo = NULL;
		RADIOKIND rdoKind;
		hr = rdList->lpVtbl->GetAt(rdList, i, (void **)&rdo);
		if (FAILED(hr) || !rdo) {
			LOGERROR_PTR("IVectorView::GetAt()", hr);
			continue;
		}
		rdoKind = RADIOKIND_OTHER;
		hr = rdo->lpVtbl->get_Kind(rdo, &rdoKind);
		if (FAILED(hr)) {
			LOGERROR_PTR("IRadio::get_Kind()", hr);
			rdoKind = RADIOKIND_OTHER;
		}
		if (rdoKind == rkKind) {
			hr = rdo->lpVtbl->get_State(rdo, &result);
			if (FAILED(hr)) {
				LOGERROR_PTR("IRadio::get_State()", hr);
				result = RADIOSTATE_UNKNOWN;
			}
		}
		rdo->lpVtbl->Release(rdo);
		if (rdoKind == rkKind)
			break;
	}
	rdList->lpVtbl->Release(rdList);
done:
	SysIntern_FreeMsWinCoreWinRtAPIS();
	return result;
}

static void SysIntern_SetRadioState(RADIOKIND rkKind, RADIOSTATE rsState) {
	DWORD i, dwSize = 0;
	IVectorView *rdList;
	HRESULT hr;
	if (!SysIntern_LoadMsWinCoreWinRtAPIS())
		goto done;
	rdList = SysIntern_GetRadioList(true);
	if (!rdList)
		goto done;
	hr = rdList->lpVtbl->get_Size(rdList, &dwSize);
	if (FAILED(hr)) {
		LOGERROR_PTR("IVectorView::get_Size()", hr);
		goto done;
	}
	for (i = 0; i < dwSize; ++i) {
		IRadio *rdo = NULL;
		RADIOKIND rdoKind;
		hr = rdList->lpVtbl->GetAt(rdList, i, (void **)&rdo);
		if (FAILED(hr) || !rdo) {
			LOGERROR_PTR("IVectorView::GetAt()", hr);
			continue;
		}
		rdoKind = RADIOKIND_OTHER;
		hr = rdo->lpVtbl->get_Kind(rdo, &rdoKind);
		if (FAILED(hr)) {
			LOGERROR_PTR("IRadio::get_Kind()", hr);
			rdoKind = RADIOKIND_OTHER;
		}
		if (rdoKind == rkKind) {
			IAsyncOperation *ssAsync = NULL;
			hr = rdo->lpVtbl->SetStateAsync(rdo, rsState, (void **)&ssAsync);
			if (FAILED(hr) || !ssAsync) {
				LOGERROR_PTR("IRadio::SetStateAsync()", hr);
			} else {
				hr = SysIntern_WaitForIAsyncOperation(ssAsync, &IID_AsyncOperationCompletedHandler_RadioAccessStatus);
				if (!FAILED(hr)) {
					RADIOACCESSSTATUS acc = RADIOACCESSSTATUS_UNSPECIFIED;
					/* Check operation status and log an error if it failed. */
					hr = ssAsync->lpVtbl->GetResults(ssAsync, (void **)&acc);
					if (FAILED(hr)) {
						LOGERROR_PTR("IRadio::SetStateAsync()::GetResults()", hr);
					} else if (acc != RADIOACCESSSTATUS_ALLOWED) {
						LOGERROR("IRadio::SetStateAsync() returned access error %u\n", acc);
						if (acc == RADIOACCESSSTATUS_DENIED_BY_USER) {
							static bool didnotice = false;
							if (!didnotice) {
								printf("[notice] Wintouchg isn't allowed to control radio devices\n");
								printf("[notice] You must enable app access to radio devices in SystemSettings/Privacy\n");
								didnotice = true;
							}
						}
					}
				}
				ssAsync->lpVtbl->Release(ssAsync);
			}
		}
		rdo->lpVtbl->Release(rdo);
		if (rdoKind == rkKind)
			break;
	}
	rdList->lpVtbl->Release(rdList);
done:
	SysIntern_FreeMsWinCoreWinRtAPIS();
}

static int last_system_bluetooth = -1;
static int last_system_wifi      = -1;

/* High-level System radio getter/setter */
static bool Sys_GetBluetoothEnabled(void) {
	if (last_system_bluetooth < 0)
		last_system_bluetooth = SysIntern_GetRadioState(RADIOKIND_BLUETOOTH) == RADIOSTATE_ON;
	return last_system_bluetooth > 0;
}
static void Sys_SetBluetoothEnabled(bool enabled) {
	if (last_system_bluetooth == enabled)
		return;
	SysIntern_SetRadioState(RADIOKIND_BLUETOOTH,
	                        enabled ? RADIOSTATE_ON
	                                : RADIOSTATE_OFF);
	last_system_bluetooth = enabled;
}
static bool Sys_GetWifiEnabled(void) {
	if (last_system_wifi < 0)
		last_system_wifi = SysIntern_GetRadioState(RADIOKIND_WIFI) == RADIOSTATE_ON;
	return last_system_wifi > 0;
}
static void Sys_SetWifiEnabled(bool enabled) {
	if (last_system_wifi == enabled)
		return;
	SysIntern_SetRadioState(RADIOKIND_WIFI,
	                        enabled ? RADIOSTATE_ON
	                                : RADIOSTATE_OFF);
	last_system_wifi = enabled;
}


/* TODO: CP_ELEM_TGL_ROTLOCK: Rotation lock
 *  - HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\AutoRotation\Enable
 *  - [DllImport("user32.dll", EntryPoint = "#2507")]
 *    extern static bool SetAutoRotationState(bool bEnable);
 * Source: https://social.msdn.microsoft.com/Forums/en-US/f9cd061b-ce33-4376-9114-f4f783a3bde3/how-do-i-turn-onoff-autorotation-by-code */

/* TODO: CP_ELEM_TGL_NIGHTMODE: Night mode
 *  - HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\CloudStore\Store\Cache\DefaultAccount\$$windows.data.bluelightreduction.bluelightreductionstate\Current
 * Source: Own reverse engineering */

/* TODO: CP_ELEM_TGL_FLIGHTMODE: Flight mode enable/disable
 * Can be read using: HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\RadioManagement\SystemRadioState
 * Source: https://stackoverflow.com/questions/42273812/how-to-detect-airplane-mode-programmatically-in-laptop-with-windows-10-using-uwp
 */
/* TODO: CP_ELEM_TGL_PWRSAVE:    Powersaving mode enable/disable */
/* TODO: Battery state/levels:   powrprof.dll? (maybe)  */
/************************************************************************/







/************************************************************************/
/* Windowing helper functions                                           */
/************************************************************************/
/* From: https://stackoverflow.com/questions/210504/enumerate-windows-like-alt-tab-does */
#define LONGEST_ILLEGAL_CLASS_NAME 32
static WCHAR const non_taskbar_window_class_names[][LONGEST_ILLEGAL_CLASS_NAME + 1] = {
	L"Shell_TrayWnd", L"DV2ControlHost",
	L"MsgrIMEWindowClass", L"SysShadow", L"Button",
	L"Windows.UI.Core.CoreWindow"
};
static bool Sys_IsAltTabWindow(HWND hWnd) {
	WCHAR cName[LONGEST_ILLEGAL_CLASS_NAME + 1];
	HWND hWndWalk, hWndTry, hWndParent;
	HWND hShell = GetShellWindow();
	int cNameLen;
	LONG style;
	size_t i;
	DWORD dwCloaked;
	if (hWnd == hShell)
		return false;
#ifndef GWL_HWNDPARENT
#define GWL_HWNDPARENT (-8)
#endif /* !GWL_HWNDPARENT */
	hWndParent = (HWND)(uintptr_t)GetWindowLongW(hWnd, GWL_HWNDPARENT);
	if (hWndParent != NULL && hWndParent != hShell)
		return false;
	style = GetWindowLong(hWnd, GWL_EXSTYLE);
	if ((style & WS_EX_TOOLWINDOW) && !(style & WS_EX_APPWINDOW))
		return false;
	if (!IsWindowVisible(hWnd))
		return false;
	if (pdyn_DwmGetWindowAttribute &&
	    DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &dwCloaked, sizeof(dwCloaked))) {
		if (dwCloaked != 0)
			return false;
	}
	hWndTry  = NULL;
	hWndWalk = GetAncestor(hWnd, GA_ROOTOWNER);
	for (;;) {
		HWND hNext;
		hNext = GetLastActivePopup(hWndWalk);
		if (!hNext || hNext == hWndTry)
			break;
		hWndTry = hNext;
		if (IsWindowVisible(hWndTry))
			break;
		hWndWalk = hWndTry;
	}
	if (hWndWalk != hWnd)
		return false;
	cNameLen = GetClassNameW(hWnd, cName, LONGEST_ILLEGAL_CLASS_NAME);
	if (cNameLen <= 0)
		return false;
	if (cNameLen <= LONGEST_ILLEGAL_CLASS_NAME) {
		cName[cNameLen] = 0;
		for (i = 0; i < sizeof(non_taskbar_window_class_names) / sizeof(*non_taskbar_window_class_names); ++i)
			if (wcscmp(cName, non_taskbar_window_class_names[i]) == 0)
				return false;
		if (cNameLen >= 18 &&
		    memcmp(cName, L"WMP9MediaBarFlyout", 18 * sizeof(WCHAR)) == 0)
			return false;
	}
	cName[0] = 0;
	if (GetWindowTextW(hWnd, cName, LONGEST_ILLEGAL_CLASS_NAME) <= 0 || cName[0] == 0)
		return false;
	return true;
}

/* Determine window features and special behavior. */
#define SYS_WINDOW_FEAT_RESIZABLE 0x0001 /* FLAG: Resizable window */
#define SYS_WINDOW_FEAT_MOVABLE   0x0002 /* FLAG: Movable window */
#define SYS_WINDOW_FEAT_MINIMIZE  0x0004 /* FLAG: May minimize */
#define SYS_WINDOW_FEAT_MAXIMIZE  0x0008 /* FLAG: May maximize */
#define SYS_WINDOW_FEAT_DESKTOP   0x8000 /* Desktop-window (special handling) */
static unsigned int Sys_GetWindowFeatures(HWND hWnd) {
	unsigned int result = 0;
	WCHAR cName[65];
	int cNameLen;
	DWORD dwStyle;
	if (hWnd == GetShellWindow())
		return SYS_WINDOW_FEAT_DESKTOP;
	dwStyle = GetWindowLong(hWnd, GWL_STYLE);
	if (!(dwStyle & WS_VISIBLE))
		goto done;
	if (dwStyle & (WS_SYSMENU | WS_BORDER | WS_DLGFRAME))
		result |= SYS_WINDOW_FEAT_MOVABLE;
	if (dwStyle & WS_MINIMIZEBOX)
		result |= (SYS_WINDOW_FEAT_MINIMIZE | SYS_WINDOW_FEAT_MOVABLE);
	if (dwStyle & WS_MAXIMIZEBOX)
		result |= (SYS_WINDOW_FEAT_MAXIMIZE | SYS_WINDOW_FEAT_MOVABLE);
	if (dwStyle & WS_SIZEBOX)
		result |= (SYS_WINDOW_FEAT_RESIZABLE | SYS_WINDOW_FEAT_MOVABLE);
	cNameLen = GetClassNameW(hWnd, cName, 64);
	if (cNameLen > 0 && cNameLen < 64) {
		cName[cNameLen] = 0;
		if (wcscmp(cName, L"Progman") == 0 || wcscmp(cName, L"WorkerW") == 0) {
			if ((dwStyle & (WS_POPUP | WS_VISIBLE | WS_MINIMIZE | WS_DISABLED |
			                WS_CAPTION | WS_BORDER | WS_DLGFRAME | WS_SYSMENU |
			                WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)) ==
			    /*      */ (WS_POPUP | WS_VISIBLE)) {
				DWORD shellPid, winPid;
				shellPid = winPid = 0;
				GetWindowThreadProcessId(GetShellWindow(), &shellPid);
				GetWindowThreadProcessId(hWnd, &winPid);
				if (shellPid && winPid && shellPid == winPid)
					return SYS_WINDOW_FEAT_DESKTOP; /* Special case: Desktop window */
			}
		}
	}
done:
	return result;
}
/************************************************************************/







/************************************************************************/
/* Touch point information                                              */
/************************************************************************/
typedef struct {
	UINT32 tm_pid; /* Real (hardware) pointer ID. */
} WtgTouchInfoMeta;
static POINTER_TOUCH_INFO touch_info[WTG_MAX_TOUCH_INPUTS];
static WtgTouchInfoMeta touch_meta[WTG_MAX_TOUCH_INPUTS];
static size_t touch_count = 0;
/* Set of touch IDs that have been canceled. */
static UINT32 touch_canceled_ids[WTG_MAX_TOUCH_INPUTS];
static size_t touch_canceled_cnt = 0;

/* Inject touch information. */
static void WtgTouchInfo_Inject(void) {
	if (!touch_count)
		return;
	if (!InjectTouchInput(touch_count, touch_info)) {
		LOGERROR("InjectTouchInput() failed: %lu (pointerIds: {", GetLastError());
		for (size_t i = 0; i < touch_count; ++i) {
			LOGERROR_CONTINUE("%s%u", i == 0 ? "" : ",",
			                  touch_info[i].pointerInfo.pointerId);
		}
		LOGERROR_CONTINUE("})\n");
	}
}
static void WtgTouchInfo_InjectEx(size_t changed_index) {
	POINTER_TOUCH_INFO hwinfo;
	if (GetPointerTouchInfo(touch_meta[changed_index].tm_pid, &hwinfo)) {
		POINTER_TOUCH_INFO *info;
		info = &touch_info[changed_index];
		info->rcContact.left   = info->pointerInfo.ptPixelLocation.x - (hwinfo.pointerInfo.ptPixelLocation.x - hwinfo.rcContact.left);
		info->rcContact.top    = info->pointerInfo.ptPixelLocation.y - (hwinfo.pointerInfo.ptPixelLocation.y - hwinfo.rcContact.top);
		info->rcContact.right  = info->pointerInfo.ptPixelLocation.x + (hwinfo.rcContact.right - hwinfo.pointerInfo.ptPixelLocation.x);
		info->rcContact.bottom = info->pointerInfo.ptPixelLocation.y + (hwinfo.rcContact.bottom - hwinfo.pointerInfo.ptPixelLocation.y);
		info->touchFlags       = hwinfo.touchFlags;
		info->touchMask        = hwinfo.touchMask;
		info->rcContact        = hwinfo.rcContact;
		info->orientation      = hwinfo.orientation;
		info->pressure         = hwinfo.pressure;
	}
	WtgTouchInfo_Inject();
}

/* Inject touch input to cancel currently in-progress touch inputs. */
static void WtgTouchInfo_CancelInputs(void) {
	size_t i;
	for (i = 0; i < touch_count; ++i)
		touch_info[i].pointerInfo.pointerFlags = POINTER_FLAG_CANCELED | POINTER_FLAG_UP;
	WtgTouchInfo_Inject();
	touch_count = 0;
}
/************************************************************************/





/************************************************************************/
/* Gesture control                                                      */
/************************************************************************/
typedef struct {
	UINT32 gt_hwid;  /* Hardware pointer ID */
	POINT  gt_start; /* Starting position */
	POINT  gt_now;   /* Current position */
	POINT  gt_delta; /* Delta movement */
} WtgGestureTouchPoint;

/* Touch information when there are >= WTG_GESTURE_STRT_TOUCH_COUNT different points.
 * Note that all of these points may be assumed to be held down! */
static WtgGestureTouchPoint gesture_info[WTG_MAX_TOUCH_INPUTS];
/* The actual # of elements in `gesture_info', or `0' when not doing a
 * multi-touch gesture. */
static size_t gesture_count = 0;


static double Wtg_DistanceBetween(POINT a, POINT b) {
	LONG h = a.x - b.x;
	LONG v = a.y - b.y;
	LONG t = (h * h) + (v * v);
	return sqrt(abs(t));
}

#define WTG_DEFINE_GESTURE_DISTANCE_CALCULATOR(name, field)           \
	static double name(void) {                                        \
		size_t i, j, count = 0;                                       \
		double result = 0.0;                                          \
		for (i = 0; i < gesture_count; ++i) {                         \
			for (j = i + 1; j < gesture_count; ++j) {                 \
				result += Wtg_DistanceBetween(gesture_info[i].field,  \
				                              gesture_info[j].field); \
				++count;                                              \
			}                                                         \
		}                                                             \
		return (result / count) * gesture_count;                      \
	}
WTG_DEFINE_GESTURE_DISTANCE_CALCULATOR(WtgGesture_CalculateStartDistance, gt_start);
WTG_DEFINE_GESTURE_DISTANCE_CALCULATOR(WtgGesture_CalculateCurrDistance, gt_now);
#undef WTG_DEFINE_GESTURE_DISTANCE_CALCULATOR

static double WtgGesture_CalculatePrevDistance(void) {
	size_t i, j, count = 0;
	double result = 0.0;
	for (i = 0; i < gesture_count; ++i) {
		POINT prevI;
		prevI = gesture_info[i].gt_now;
		prevI.x -= gesture_info[i].gt_delta.x;
		prevI.y -= gesture_info[i].gt_delta.y;
		for (j = i + 1; j < gesture_count; ++j) {
			POINT prevJ;
			prevJ = gesture_info[j].gt_now;
			prevJ.x -= gesture_info[j].gt_delta.x;
			prevJ.y -= gesture_info[j].gt_delta.y;
			result += Wtg_DistanceBetween(prevI, prevJ);
			++count;
		}
	}
	return (result / count) * gesture_count;
}

#define WTG_DEFINE_GESTURE_TOTAL_DELTA_CALCULATOR(name, xy) \
	static double name(void) {                              \
		size_t i;                                           \
		LONG result = 0;                                    \
		for (i = 0; i < gesture_count; ++i) {               \
			result += gesture_info[i].gt_now.xy -           \
			          gesture_info[i].gt_start.xy;          \
		}                                                   \
		return (double)result / gesture_count;              \
	}
WTG_DEFINE_GESTURE_TOTAL_DELTA_CALCULATOR(WtgGesture_CalculateHDeltaTotal, x);
WTG_DEFINE_GESTURE_TOTAL_DELTA_CALCULATOR(WtgGesture_CalculateVDeltaTotal, y);
#undef WTG_DEFINE_GESTURE_TOTAL_DELTA_CALCULATOR

#define WTG_DEFINE_GESTURE_DELTA_CALCULATOR(name, xy) \
	static double name(void) {                        \
		size_t i;                                     \
		LONG result = 0;                              \
		for (i = 0; i < gesture_count; ++i)           \
			result += gesture_info[i].gt_delta.xy;    \
		return (double)result / gesture_count;        \
	}
WTG_DEFINE_GESTURE_DELTA_CALCULATOR(WtgGesture_CalculateHDelta, x);
WTG_DEFINE_GESTURE_DELTA_CALCULATOR(WtgGesture_CalculateVDelta, y);
#undef WTG_DEFINE_GESTURE_DELTA_CALCULATOR
/************************************************************************/










/************************************************************************/
/* Application window Handling (app switcher)                           */
/************************************************************************/
typedef struct {
	HWND aw_window; /* Application window. */
} WtgAppWindow;
typedef struct {
	WtgAppWindow *awl_vec; /* [0..awl_cnt][owned] Vector of app windows. */
	size_t        awl_cnt; /* # of app windows. */
	size_t        awl_alc; /* Allocated # of app windows. */
} WtgAppWindowList;

/* [0..appwin_cnt] Vector of application windows.
 * The order of windows in this vector represents the switch-order
 * when cycling to the prev/next window. Additionally, the first
 * entry in this vector (if such an entry exists) points at the
 * currently active window, but is only updated when needed (i.e.
 * whenever the app switching is opened, or one switching between
 * active windows. */
static WtgAppWindowList appwin = { NULL, 0, 0 };

#define WtgAppWindowList_Fini(self) free((self)->awl_vec)
static BOOL CALLBACK _WtgAppWindowList_EnumCallback(HWND window, LPARAM param) {
	if (Sys_IsAltTabWindow(window)) {
		WtgAppWindowList *self;
		self = (WtgAppWindowList *)(uintptr_t)param;
		if (self->awl_cnt >= self->awl_alc) {
			WtgAppWindow *new_vec;
			size_t new_alc = self->awl_alc * 2;
			if (new_alc < self->awl_cnt + 8)
				new_alc = self->awl_cnt + 8;
			new_vec = (WtgAppWindow *)realloc(self->awl_vec,
			                                      new_alc *
			                                      sizeof(WtgAppWindow));
			if (!new_vec) {
				new_alc = self->awl_cnt + 1;
				new_vec = (WtgAppWindow *)realloc(self->awl_vec,
				                                      new_alc *
				                                      sizeof(WtgAppWindow));
				if (!new_vec)
					return FALSE;
			}
			self->awl_vec = new_vec;
			self->awl_alc = new_alc;
		}
		self->awl_vec[self->awl_cnt].aw_window = window;
		++self->awl_cnt;
	}
	return TRUE;
}
static WtgAppWindowList WtgAppWindowList_New(void) {
	WtgAppWindowList result;
	result.awl_vec = NULL;
	result.awl_cnt = 0;
	result.awl_alc = 0;
	EnumWindows(&_WtgAppWindowList_EnumCallback, (LPARAM)(uintptr_t)&result);
	if (result.awl_cnt > result.awl_alc) {
		WtgAppWindow *new_vec;
		new_vec = (WtgAppWindow *)realloc(result.awl_vec,
		                                  result.awl_cnt *
		                                  sizeof(WtgAppWindow));
		if (new_vec) {
			result.awl_vec = new_vec;
			result.awl_alc = result.awl_cnt;
		}
	}
	return result;
}
static size_t
WtgAppWindowList_IndexOf(WtgAppWindowList *__restrict self, HWND hWnd) {
	size_t i;
	for (i = 0; i < self->awl_cnt; ++i) {
		if (self->awl_vec[i].aw_window == hWnd)
			break;
	}
	return i;
}
static void
WtgAppWindowList_ApplyOrder(WtgAppWindowList *__restrict self,
                            WtgAppWindowList const *__restrict order) {
	size_t order_i, dst_i = 0;
	for (order_i = 0; order_i < order->awl_cnt; ++order_i) {
		size_t self_index;
		self_index = WtgAppWindowList_IndexOf(self, order->awl_vec[order_i].aw_window);
		if (self_index < self->awl_cnt && self_index >= dst_i) {
			if (self_index > dst_i) {
				WtgAppWindow ent;
				/* Move `self->awl_vec[self_index]' to `self->awl_vec[dst_i]' */
				memcpy(&ent, &self->awl_vec[self_index], sizeof(ent));
				memmove(&self->awl_vec[dst_i + 1],
				        &self->awl_vec[dst_i],
				        (self_index - dst_i) * sizeof(ent));
				memcpy(&self->awl_vec[dst_i], &ent, sizeof(ent));
			}
			++dst_i;
		}
	}
}
static void
WtgAppWindowList_LeftRotate(WtgAppWindowList *__restrict self) {
	WtgAppWindow ent;
	memcpy(&ent, &self->awl_vec[0], sizeof(ent));
	memmove(&self->awl_vec[0], &self->awl_vec[1],
	        (self->awl_cnt - 1) * sizeof(ent));
	memcpy(&self->awl_vec[self->awl_cnt - 1], &ent, sizeof(ent));
}
static void
WtgAppWindowList_BringToFront(WtgAppWindowList *__restrict self,
                              HWND hForegroundWindow) {
	size_t index;
	index = WtgAppWindowList_IndexOf(self, hForegroundWindow);
	if (index < self->awl_cnt && index > 0) {
		do {
			WtgAppWindowList_LeftRotate(self);
		} while (--index);
	}
}

/* (Re-)load all visible application windows, whilst trying to preserve
 * the same ordering as was being used by `appwin_vec'. Then, rotate the
 * vector of application windows until `hForegroundWindow' points at the
 * first entry (that is: `appwin_vec[0]')
 * If the given `hForegroundWindow' isn't an application window, then
 * the application window vector isn't rotated. */
static void
WtgAppWindowList_Reload(HWND hForegroundWindow) {
	WtgAppWindowList newlist, oldlist;
	newlist = WtgAppWindowList_New();
	oldlist = appwin;
	WtgAppWindowList_ApplyOrder(&newlist, &oldlist);
	WtgAppWindowList_BringToFront(&newlist, hForegroundWindow);
#ifdef HAVE_DEBUG_PRINTF
	debug_printf("Reload: %lu -> %lu windows\n", oldlist.awl_cnt, newlist.awl_cnt);
	for (size_t i = 0; i < newlist.awl_cnt; ++i) {
		char tbuf[256], cbuf[256];
		debug_printf("\tappwin[%lu] = %p ('", i, newlist.awl_vec[i].aw_window);
		int tlen = GetWindowTextA(newlist.awl_vec[i].aw_window, tbuf, 256);
		int clen = GetClassNameA(newlist.awl_vec[i].aw_window, cbuf, 256);
		if (tlen < 0)
			tlen = 0;
		if (clen < 0)
			clen = 0;
		debug_printf("%.*s', '%.*s", clen, cbuf, tlen, tbuf);
		debug_printf("')\n");
	}
#endif /* HAVE_DEBUG_PRINTF */
	WtgAppWindowList_Fini(&oldlist);
	appwin = newlist;
}

/* Lookup the predecessor/successor of `hWnd'
 * If no such element exists, or if there are too
 * few windows, then NULL is returned instead. */
static HWND WtgAppList_Prev(HWND hWnd) {
	size_t index;
	if (appwin.awl_cnt <= 1)
		return NULL;
	index = WtgAppWindowList_IndexOf(&appwin, hWnd);
	if (index >= appwin.awl_cnt)
		return NULL;
	--index;
	if (index >= appwin.awl_cnt)
		index = appwin.awl_cnt - 1;
	return appwin.awl_vec[index].aw_window;
}

static HWND WtgAppList_Next(HWND hWnd) {
	size_t index;
	if (appwin.awl_cnt <= 1)
		return NULL;
	index = WtgAppWindowList_IndexOf(&appwin, hWnd);
	if (index >= appwin.awl_cnt)
		return NULL;
	++index;
	if (index >= appwin.awl_cnt)
		index = 0;
	return appwin.awl_vec[index].aw_window;
}
/************************************************************************/










/************************************************************************/
/* Gesture animation / Recognition                                      */
/************************************************************************/
#define WTG_GESTURE_KIND_NONE       0 /* None / not determined */
#define WTG_GESTURE_KIND_IGNORE     1 /* Ignored gesture */
#define WTG_GESTURE_KIND_VSWIPE_APP 2 /* Vertical swiping for applets */
#define WTG_GESTURE_KIND_ZOOM       3 /* Zoom or pinch */
#define WTG_GESTURE_KIND_ZOOM_RSTOR 4 /* Zoom or pinch (for window restore) */
#define WTG_GESTURE_KIND_VSWIPE     5 /* Vertical swiping */
#define WTG_GESTURE_KIND_HSWIPE     6 /* Horizontal swiping */
#define WTG_GESTURE_KIND_VALID(x)   ((x) >= 2)
#define WTG_GESTURE_KIND_HASDATA(x) ((x) >= 3)

#define WTG_WINDOW_KIND_NORMAL    0 /* Normal, non-maximized window. */
#define WTG_WINDOW_KIND_MAXIMIZED 1 /* Maximized window. */
#define WTG_WINDOW_KIND_DESKTOP   2 /* The desktop itself. */
static unsigned int Wtg_GetWindowKind(HWND window) {
	WINDOWPLACEMENT placement;
	unsigned int result;
	result = WTG_WINDOW_KIND_NORMAL;
	if (GetWindowPlacement(window, &placement)) {
		if (placement.showCmd == SW_MAXIMIZE ||
			placement.showCmd == SW_SHOWMAXIMIZED)
			result = WTG_WINDOW_KIND_MAXIMIZED;
	}
	return result;
}

typedef struct {
	LONG      gwd_gwl_exstyle; /* Original `GWL_EXSTYLE' */
	COLORREF  gwd_crkey;       /* ... */
	BYTE      gwd_balpha;      /* ... */
	DWORD     gwd_dwflags;     /* ... */
} WtgHiddenWindowData;

static void
WtgHiddenWindowData_Hide(WtgHiddenWindowData *__restrict self, HWND window) {
	self->gwd_gwl_exstyle = GetWindowLongW(window, GWL_EXSTYLE);
	if (!(self->gwd_gwl_exstyle & WS_EX_LAYERED))
		SetWindowLongW(window, GWL_EXSTYLE, self->gwd_gwl_exstyle | WS_EX_LAYERED);
	if (!GetLayeredWindowAttributes(window, &self->gwd_crkey, &self->gwd_balpha, &self->gwd_dwflags)) {
		LOGERROR_GLE("GetLayeredWindowAttributes()");
		self->gwd_crkey   = 0;
		self->gwd_balpha  = 255;
		self->gwd_dwflags = 0;
	}
	if (!SetLayeredWindowAttributes(window, 0, 0, LWA_ALPHA))
		LOGERROR_GLE("SetLayeredWindowAttributes()");
}

static void
WtgHiddenWindowData_Show(WtgHiddenWindowData const *__restrict self, HWND window) {
	if (!SetLayeredWindowAttributes(window, self->gwd_crkey, self->gwd_balpha, self->gwd_dwflags))
		LOGERROR_GLE("SetLayeredWindowAttributes()");
	if (!(self->gwd_gwl_exstyle & WS_EX_LAYERED))
		SetWindowLongW(window, GWL_EXSTYLE, self->gwd_gwl_exstyle);
}


typedef struct {
	unsigned int        wa_animkind; /* The kind of window that `wa_animwin' is. (one of `GESTURE_WINDOW_KIND_*') */
	HWND                wa_animwin;  /* The window that is being animated. */
	WtgHiddenWindowData wa_windat;   /* Original data for `wa_animwin' */
	HTHUMBNAIL          wa_winthumb; /* DWM Thumbnail for `wa_animwin'. */
	RECT                wa_rect;     /* Original on-screen frame rect for `wa_animwin'. */
} WtgAnimatedWindow;

static bool
WtgAnimatedWindow_InitEx(WtgAnimatedWindow *__restrict self,
                         HWND overlay_window, BOOL visible) {
	HRESULT hr;
	DWM_THUMBNAIL_PROPERTIES tnProperties;
	/* Create the thumbnail */
	hr = DwmRegisterThumbnail(overlay_window, self->wa_animwin, &self->wa_winthumb);
	if (FAILED(hr)) {
		LOGERROR_PTR("DwmRegisterThumbnail()", hr);
		goto fail;
	}

	/* Set properties */
	tnProperties.dwFlags = DWM_TNP_SOURCECLIENTAREAONLY |
	                       DWM_TNP_RECTDESTINATION |
	                       DWM_TNP_VISIBLE;
	tnProperties.fSourceClientAreaOnly = FALSE;
	tnProperties.rcDestination         = self->wa_rect;
	tnProperties.fVisible              = visible;
	hr = DwmUpdateThumbnailProperties(self->wa_winthumb, &tnProperties);
	if (FAILED(hr)) {
		LOGERROR_PTR("DwmUpdateThumbnailProperties()", hr);
		goto fail_overlay_thumb;
	}

	/* Hide the original window. */
	WtgHiddenWindowData_Hide(&self->wa_windat, self->wa_animwin);
	return true;
fail_overlay_thumb:
	DwmUnregisterThumbnail(self->wa_winthumb);
fail:
	return false;
}

static bool
WtgAnimatedWindow_Init(WtgAnimatedWindow *__restrict self,
                       HWND animwin, HWND overlay_window,
                       unsigned int anim_winkind,
                       BOOL visible) {
	self->wa_animwin  = animwin;
	self->wa_animkind = anim_winkind;
	if (!pdyn_DwmGetWindowAttribute ||
	    FAILED(DwmGetWindowAttribute(animwin, DWMWA_EXTENDED_FRAME_BOUNDS,
	                                 &self->wa_rect, sizeof(RECT)))) {
		if (!GetWindowRect(animwin, &self->wa_rect)) {
			LOGERROR_GLE("GetWindowRect()");
			return false;
		}
	}
	return WtgAnimatedWindow_InitEx(self, overlay_window, visible);
}

static void
WtgAnimatedWindow_Fini(WtgAnimatedWindow *__restrict self) {
	WtgHiddenWindowData_Show(&self->wa_windat, self->wa_animwin);
	DwmUnregisterThumbnail(self->wa_winthumb);
}




typedef struct {
	unsigned int              ga_kind;    /* Animation kind. (one of `WTG_GESTURE_KIND_*') */
	double                    ga_dist;    /* Animation distance (in pixels).
	                                       *  - WTG_GESTURE_KIND_ZOOM:   `> 0' Zoom
	                                       *    WTG_GESTURE_KIND_ZOOM:   `< 0' Pinch
	                                       *  - WTG_GESTURE_KIND_HSWIPE: `> 0' Swipe (to the) right
	                                       *    WTG_GESTURE_KIND_HSWIPE: `< 0' Swipe (to the) left
	                                       *  - WTG_GESTURE_KIND_VSWIPE: `> 0' Swipe (to the) bottom
	                                       *    WTG_GESTURE_KIND_VSWIPE: `< 0' Swipe (to the) top */
	double                    ga_dist2;   /* Secondary distance (the opposite V/H for SWIPE gestures) */
	HWND                      ga_fgwin;   /* The foreground window at the time that the gesture was started. */
	unsigned int              ga_fgfeat;  /* Special features of `ga_fgwin' (set of `SYS_WINDOW_FEAT_*') */
	unsigned int              ga_fgkind;  /* The kind of window that `ga_fgwin' is. (one of `GESTURE_WINDOW_KIND_*') */
	HWND                      ga_overlay; /* [owned][1..1] Overlay window (for animation rendering) */
	POINT                     ga_overpos; /* Screen position of `ga_overlay' */
	POINT                     ga_oversiz; /* Screen size of `ga_overlay' */
	HWND                      ga_blurlay; /* [owned][0..1] Blur overlay window (for animation rendering) */
	double                    ga_blurold; /* Previous blur value. */
	/* All of the below are only valid when `WTG_GESTURE_KIND_HASDATA(ga_kind)' */
	WtgAnimatedWindow         ga_anim;    /* Window animation */
	WtgAnimatedWindow         ga_prev;    /* [valid_if(ga_kind             == WTG_GESTURE_KIND_HSWIPE &&
	                                       *           ga_anim.wa_animkind == WTG_WINDOW_KIND_MAXIMIZED)]
	                                       * Window animation for the next window */
	WtgAnimatedWindow         ga_next;    /* [valid_if(ga_kind             == WTG_GESTURE_KIND_HSWIPE &&
	                                       *           ga_anim.wa_animkind == WTG_WINDOW_KIND_MAXIMIZED)]
	                                       * Window animation for the next window */
} WtgGestureAnimation;

static HWND WtgGestureAnimation_CreateOverlayWindow(WtgGestureAnimation const *__restrict self) {
	HWND hResult;
	hResult = CreateWindowExW(WS_EX_NOACTIVATE | WS_EX_LAYERED |
	                          WS_EX_TRANSPARENT | WS_EX_TOPMOST,
	                          L"ShadowWindowClass", L"", WS_POPUP,
	                          self->ga_overpos.x, self->ga_overpos.y,
	                          self->ga_oversiz.x, self->ga_oversiz.y,
	                          HWND_DESKTOP, NULL, hApplicationInstance, NULL);
	if (hResult == NULL)
		LOGERROR_GLE("CreateWindowExW()");
	return hResult;
}

static bool
WtgGestureAnimation_Init(WtgGestureAnimation *__restrict self,
                         HWND foreground_window) {
	MONITORINFO mi;
	HMONITOR hMon;
	memset(self, 0, sizeof(*self));
	self->ga_kind   = WTG_GESTURE_KIND_NONE;
	self->ga_dist   = 0.0;
	self->ga_dist2  = 0.0;
	self->ga_fgwin  = foreground_window;
	self->ga_fgfeat = Sys_GetWindowFeatures(foreground_window);
	if (self->ga_fgfeat == SYS_WINDOW_FEAT_DESKTOP) {
		self->ga_fgkind = WTG_WINDOW_KIND_DESKTOP;
	} else {
		WINDOWPLACEMENT placement;
		self->ga_fgkind = WTG_WINDOW_KIND_NORMAL;
		if (GetWindowPlacement(foreground_window, &placement)) {
			if (placement.showCmd == SW_MAXIMIZE ||
			    placement.showCmd == SW_SHOWMAXIMIZED)
				self->ga_fgkind = WTG_WINDOW_KIND_MAXIMIZED;
		}
	}
	hMon = MonitorFromWindow(foreground_window, MONITOR_DEFAULTTOPRIMARY);
	if (!hMon) {
		LOGERROR_GLE("MonitorFromWindow()");
		goto err;
	}
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfoW(hMon, &mi))
		LOGERROR_GLE("GetMonitorInfoW()");
	/* Construct the gesture overlay window. */
	self->ga_overpos.x = mi.rcWork.left;
	self->ga_overpos.y = mi.rcWork.top;
	self->ga_oversiz.x = RC_WIDTH(mi.rcWork);
	self->ga_oversiz.y = RC_HEIGHT(mi.rcWork);
	self->ga_overlay   = WtgGestureAnimation_CreateOverlayWindow(self);
	if (!self->ga_overlay)
		goto err;
	ShowWindow(self->ga_overlay, SW_SHOWNOACTIVATE);
	return true;
err:
	return false;
}

static void
WtgGestureAnimation_Fini(WtgGestureAnimation *__restrict self) {
	if (WTG_GESTURE_KIND_HASDATA(self->ga_kind)) {
		if (self->ga_kind == WTG_GESTURE_KIND_HSWIPE &&
		    self->ga_anim.wa_animkind == WTG_WINDOW_KIND_MAXIMIZED) {
			if (self->ga_prev.wa_animwin != NULL)
				WtgAnimatedWindow_Fini(&self->ga_prev);
			if (self->ga_next.wa_animwin != NULL)
				WtgAnimatedWindow_Fini(&self->ga_next);
		}
		WtgAnimatedWindow_Fini(&self->ga_anim);
	}
	if (self->ga_blurlay != NULL)
		DestroyWindow(self->ga_blurlay);
	if (self->ga_overlay != NULL)
		DestroyWindow(self->ga_overlay);
}

static void Wtg_EnableBlur(HWND hWnd) {
	if (!pdyn_SetWindowCompositionAttribute &&
	    (!pdyn_DwmEnableBlurBehindWindow || !pdyn_DwmExtendFrameIntoClientArea))
		return; /* Not possible... */
	if (pdyn_SetWindowCompositionAttribute != NULL) {
		/* From: https://github.com/jdmansour/mintty/blob/glass/src/winmain.c */
#define WCA_ACCENT_POLICY 19
		typedef enum {
			ACCENT_DISABLED                   = 0,
			ACCENT_ENABLE_GRADIENT            = 1,
			ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
			ACCENT_ENABLE_BLURBEHIND          = 3,
			ACCENT_INVALID_STATE              = 4
		} ACCENT_STATE;
		typedef struct {
			ACCENT_STATE accentState;
			int accentFlags;
			int gradientColor;
			int invalidState;
		} DWMACCENTPOLICY;
		DWMACCENTPOLICY policy;
		WINCOMPATTRDATA data = { WCA_ACCENT_POLICY, &policy, sizeof(policy) };
		memset(&policy, 0, sizeof(policy));
		policy.accentState = ACCENT_ENABLE_BLURBEHIND;
		if (!SetWindowCompositionAttribute(hWnd, &data))
			LOGERROR_GLE("SetWindowCompositionAttribute()");
	} else {
		HRESULT hr;
		{
			DWM_BLURBEHIND bb;
			bb.dwFlags = DWM_BB_ENABLE;
			bb.fEnable = TRUE;
			hr = DwmEnableBlurBehindWindow(hWnd, &bb);
			if (FAILED(hr))
				LOGERROR_PTR("DwmEnableBlurBehindWindow()", hr);
		}
		{
			MARGINS margins;
			margins.cxLeftWidth    = -1;
			margins.cxRightWidth   = 0;
			margins.cyTopHeight    = 0;
			margins.cyBottomHeight = 0;
			hr = DwmExtendFrameIntoClientArea(hWnd, &margins);
			if (FAILED(hr))
				LOGERROR_PTR("DwmExtendFrameIntoClientArea()", hr);
		}
	}
}

static HWND
WtgGestureAnimation_CreateBlurWindow(WtgGestureAnimation const *__restrict self) {
	HWND result;
	if (!pdyn_SetWindowCompositionAttribute &&
	    (!pdyn_DwmEnableBlurBehindWindow || !pdyn_DwmExtendFrameIntoClientArea))
		return NULL; /* Not possible... */
	result = WtgGestureAnimation_CreateOverlayWindow(self);
	if (result) {
		SetWindowPos(result, self->ga_overlay, 0, 0, 0, 0,
		             SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE |
		             SWP_NOACTIVATE | SWP_NOSENDCHANGING);
		Wtg_EnableBlur(result);
	}
	return result;
}

static void
WtgGestureAnimation_SetBlur(WtgGestureAnimation *__restrict self,
                            double strength);
/************************************************************************/









/************************************************************************/
/* The current gesture animation                                        */
/************************************************************************/
static WtgGestureAnimation ga;
/************************************************************************/










/************************************************************************/
/* Application switcher and control panel                               */
/************************************************************************/
typedef struct {
	bool  as_active; /* [lock(WRITE_ONCE)] Applet is active */
	HWND  as_window; /* [lock(WRITE_ONCE)][0..1] Application switcher window. */
	/* All of the below are only valid when `as_window != NULL' */
	HWND  as_oldwin; /* [const][0..1] The previously active window (if the
	                  * switcher was entered from within a maximized application,
	                  * else NULL if entered from the desktop). */
} WtgAppSwitcherApplet;

static void WtgAppSwitcherApplet_Fini(WtgAppSwitcherApplet *__restrict self) {
	if (self->as_window != NULL)
		DestroyWindow(self->as_window);
	memset(self, 0, sizeof(*self));
}

#define CP_ELEM_NONE           ((unsigned int)-1) /* No element */
#define CP_ELEM_BRIGHTNESS     0 /* Brightness slider */
#define CP_ELEM_VOLUME         1 /* Volume slider */
#define CP_ELEM_CLK_BATTERY    2 /* Battery & Clock information */
#define CP_ELEM_BUTTONS_FIRST  CP_ELEM_TGL_WIFI /* First button */
#define CP_ELEM_TGL_WIFI       3 /* Toggle wifi */
#define CP_ELEM_TGL_BLUETOOTH  4 /* Toggle bluetooth */
#define CP_ELEM_TGL_ROTLOCK    5 /* Toggle rotation lock */
#define CP_ELEM_TGL_FLIGHTMODE 6 /* Toggle flight mode */
#define CP_ELEM_TGL_NIGHTMODE  7 /* Toggle night mode */
#define CP_ELEM_TGL_PWRSAVE    8 /* Toggle powersaving mode */
#define CP_ELEM_BUTTON_USER_LO 9                                /* User-defined buttons */
#define CP_ELEM_BUTTON_USER_HI (CP_ELEM_BUTTONS_FIRST + 14 - 1) /* *ditto* */
#define CP_ELEM_BUTTONS_LAST   CP_ELEM_BUTTON_USER_HI           /* Last button */
#define CP_ELEM_COUNT          (CP_ELEM_BUTTON_USER_HI + 1)     /* # of elements */
/* ... */

#define CP_BUTTON_WIDTH  115 /* TODO: DPI scaling? */
#define CP_BUTTON_HEIGHT 85  /* TODO: DPI scaling? */

typedef struct {
	UINT32       cat_pointerId; /* Hardware pointer ID. */
	unsigned int cat_element;   /* Control element grabbed by this pointer. (one of `CP_ELEM_*') */
	unsigned int cat_hover;     /* Control element being hovered over. (if any) */
} WtgAppletTouchPoint;

typedef struct {
	bool                cp_active;  /* [lock(WRITE_ONCE)] Applet is active */
	HWND                cp_window;  /* [lock(WRITE_ONCE)][owned][0..1] Control panel window. */
	/* All of the below are only valid when `cp_window != NULL' */
	HWND                cp_wrapper; /* [const][owned][1..1] Wrapper for `cp_window' (to keep it off of the task bar) */
	POINT               cp_pos;     /* [const] (final) position of the `cp_window' window */
	POINT               cp_size;    /* [const] Size of the `cp_window' window */
	RECT                cp_client;  /* [const] Usable client area for `cp_window' */
	HWND                cp_blur;    /* [const][lock(WRITE_ONCE)][0..1][valid_if(cp_active)]
	                                 * Screen blur overlay . */
	UINT_PTR            cp_timer;   /* A timer that fired every second */
	SYSTEMTIME          cp_now;     /* The current system time. (for rendering; updated every second) */
	WtgAppletTouchPoint cp_tpvec[WTG_MAX_TOUCH_INPUTS];
	size_t              cp_tpcnt;   /* # of valid touch points. */
	RECT                cp_elems[CP_ELEM_COUNT]; /* Display rects for elements. */
} WtgControlPanelApplet;

static void WtgControlPanelApplet_Fini(WtgControlPanelApplet *__restrict self) {
	if (self->cp_active && self->cp_blur != NULL)
		DestroyWindow(self->cp_blur);
	if (self->cp_window != NULL) {
		if (self->cp_timer != 0)
			KillTimer(self->cp_window, self->cp_timer);
		DestroyWindow(self->cp_window);
	}
	if (self->cp_wrapper != NULL)
		DestroyWindow(self->cp_wrapper);
	memset(self, 0, sizeof(*self));
}

static size_t /* Find the index of `pointerId', or return `cp_tpcnt' */
WtgControlPanelApplet_FindTouch(WtgControlPanelApplet *__restrict self, UINT32 pointerId) {
	size_t result;
	for (result = 0; result < self->cp_tpcnt; ++result) {
		if (self->cp_tpvec[result].cat_pointerId == pointerId)
			break;
	}
	return result;
}
static bool /* Delete the touch-point for `pointerId' */
WtgControlPanelApplet_DelTouch(WtgControlPanelApplet *__restrict self, UINT32 pointerId) {
	size_t index = WtgControlPanelApplet_FindTouch(self, pointerId);
	if (index < self->cp_tpcnt) {
		--self->cp_tpcnt;
		memmove(&self->cp_tpvec[index],
		        &self->cp_tpvec[index + 1],
		        (self->cp_tpcnt - index) *
		        sizeof(*self->cp_tpvec));
		return true;
	}
	return false;
}
static void /* Set the element touched by `pointerId' (one of `CP_ELEM_*') */
WtgControlPanelApplet_SetTouch(WtgControlPanelApplet *__restrict self,
                               UINT32 pointerId, unsigned int elem,
                               unsigned int hover) {
	size_t index = WtgControlPanelApplet_FindTouch(self, pointerId);
	if (index >= WTG_MAX_TOUCH_INPUTS)
		return;
	self->cp_tpvec[index].cat_pointerId = pointerId;
	self->cp_tpvec[index].cat_element   = elem;
	self->cp_tpvec[index].cat_hover     = hover;
	if (self->cp_tpcnt <= index)
		self->cp_tpcnt = index + 1;
}
static unsigned int /* Get the element touched by `pointerId' (one of `CP_ELEM_*') */
WtgControlPanelApplet_GetTouch(WtgControlPanelApplet *__restrict self,
                               UINT32 pointerId) {
	unsigned int result = CP_ELEM_NONE;
	size_t index = WtgControlPanelApplet_FindTouch(self, pointerId);
	if (index < self->cp_tpcnt)
		result = self->cp_tpvec[index].cat_element;
	return result;
}
static unsigned int /* Get the element hovered by `pointerId' (one of `CP_ELEM_*') */
WtgControlPanelApplet_GetHover(WtgControlPanelApplet *__restrict self,
                               UINT32 pointerId) {
	unsigned int result = CP_ELEM_NONE;
	size_t index = WtgControlPanelApplet_FindTouch(self, pointerId);
	if (index < self->cp_tpcnt)
		result = self->cp_tpvec[index].cat_hover;
	return result;
}
static bool /* Check if `elem' is being held */
WtgControlPanelApplet_IsHeld(WtgControlPanelApplet *__restrict self,
                             unsigned int elem) {
	size_t i;
	for (i = 0; i < self->cp_tpcnt; ++i) {
		if (self->cp_tpvec[i].cat_element == elem)
			return true;
	}
	return false;
}
static bool /* Check if `elem' is being hovered */
WtgControlPanelApplet_IsHover(WtgControlPanelApplet *__restrict self,
                              unsigned int elem) {
	size_t i;
	for (i = 0; i < self->cp_tpcnt; ++i) {
		if (self->cp_tpvec[i].cat_hover == elem)
			return true;
	}
	return false;
}

/* Applet instances */
static WtgAppSwitcherApplet as; /* Application switcher. */
static WtgControlPanelApplet cp; /* Control panel. */


/* GUI elements (from gui/gui.c)
 * NOTE: All of these using BGRA byte order! */
extern uint8_t const gui_cp_l[32][1][4];
extern uint8_t const gui_cp_c[1][1][4];
extern uint8_t const gui_cp_r[32][1][4];
extern uint8_t const gui_cp_bl[32][32][4];
extern uint8_t const gui_cp_b[1][32][4];
extern uint8_t const gui_cp_br[32][32][4];
extern uint8_t const gui_cp_wifi[64][64][4];
extern uint8_t const gui_cp_bth[32][64][4];
#define GUI_RGBA(r, g, b, a) /* XXX: Byteorder??? */ \
	((UINT32)(b) | ((UINT32)(g) << 8) | ((UINT32)(r) << 16) | ((UINT32)(a) << 24))

#define CP_BORDER 32 /* Width (in pixels) of the control panel border. */

#define GUI_WIDTH(elem)  (sizeof(elem) / sizeof(*(elem)))
#define GUI_HEIGHT(elem) (sizeof(*(elem)) / sizeof(**(elem)))


static void Wtg_PerPixelCopy(void *dst, void const *src,
                             size_t w, size_t h,
                             size_t dst_stride,
                             size_t src_stride) {
	while (h) {
		memcpy(dst, src, w * 4);
		dst = (BYTE *)dst + (dst_stride * 4);
		src = (BYTE *)src + (src_stride * 4);
		--h;
	}
}

static void Wtg_PerPixelBlit(UINT32 *image, size_t x, size_t y,
                             void const *src, size_t w, size_t h) {
	Wtg_PerPixelCopy(image + x + (y * cp.cp_size.x),
	                 src, w, h, cp.cp_size.x, w);
}

#if 0 /*< Define as 1 to have the blender round upwards. */
#define div256(x) (((x) + 0xfe) / 0xff)
#else
#define div256(x) ((x) / 0xff)
#endif

static UINT32 Wtg_BlendPixels(UINT32 dst, UINT32 src) {
	union bgra_pixel {
		UINT32 pixel;
		struct {
			UINT8 b;
			UINT8 g;
			UINT8 r;
			UINT8 a;
		};
	};
	union bgra_pixel dst_pixel;
	union bgra_pixel src_pixel;
	union bgra_pixel lhs_pixel;
	union bgra_pixel rhs_pixel;
	union bgra_pixel res_pixel;
	dst_pixel.pixel = dst;
	src_pixel.pixel = src;

	lhs_pixel.r = (UINT8)div256((uint32_t)src_pixel.r * src_pixel.a);
	lhs_pixel.g = (UINT8)div256((uint32_t)src_pixel.g * src_pixel.a);
	lhs_pixel.b = (UINT8)div256((uint32_t)src_pixel.b * src_pixel.a);
	lhs_pixel.a = (UINT8)div256((uint32_t)src_pixel.a * src_pixel.a);

	rhs_pixel.r = (UINT8)div256((uint32_t)dst_pixel.r * (0xff - src_pixel.a));
	rhs_pixel.g = (UINT8)div256((uint32_t)dst_pixel.g * (0xff - src_pixel.a));
	rhs_pixel.b = (UINT8)div256((uint32_t)dst_pixel.b * (0xff - src_pixel.a));
	rhs_pixel.a = (UINT8)div256((uint32_t)dst_pixel.a * (0xff - src_pixel.a));

	res_pixel.r = lhs_pixel.r + rhs_pixel.r;
	res_pixel.g = lhs_pixel.g + rhs_pixel.g;
	res_pixel.b = lhs_pixel.b + rhs_pixel.b;
	res_pixel.a = lhs_pixel.a + rhs_pixel.a;

	return res_pixel.pixel;
}

static void Wtg_PerPixelBlend(void *dst, void const *src,
                              size_t w, size_t h,
                              size_t dst_stride,
                              size_t src_stride) {
	size_t x;
	while (h) {
		for (x = 0; x < w; ++x) {
			((UINT32 *)dst)[x] = Wtg_BlendPixels(((UINT32 const *)dst)[x],
			                                     ((UINT32 const *)src)[x]);
		}
		dst = (BYTE *)dst + (dst_stride * 4);
		src = (BYTE *)src + (src_stride * 4);
		--h;
	}
}


static bool WtgControlPanel_IsSliderVertical(unsigned int elem) {
	LONG w, h;
	w = RC_WIDTH(cp.cp_elems[elem]);
	h = RC_HEIGHT(cp.cp_elems[elem]);
	return h > w;
}

static double
WtgControlPanel_GetSliderPositionAt(unsigned int elem, LONG x, LONG y) {
	double result;
	if (WtgControlPanel_IsSliderVertical(elem)) {
		LONG h = RC_HEIGHT(cp.cp_elems[elem]);
		result = (double)(h - y) / (double)h;
	} else {
		result = (double)x / (double)RC_WIDTH(cp.cp_elems[elem]);
	}
	return result;
}

static void /* @param: mode: One of `ELEM_MODE_*' */
WtgControlPanel_DrawClockHandle(HDC hdc, POINT const *__restrict center,
                                double angle, double length) {
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif /* !M_PI */
	double to_x, to_y;
	angle *= 2.0 * M_PI;
	to_x = sin(angle) * length;
	to_y = -cos(angle) * length;
	MoveToEx(hdc, center->x, center->y, NULL);
	LineTo(hdc, center->x + (LONG)to_x, center->y + (LONG)to_y);
}

#define WTG_CONTROL_PANEL_ELEM_MODE_NORMAL 0 /* Normal element */
#define WTG_CONTROL_PANEL_ELEM_MODE_HELD   1 /* Element is being held down */
#define WTG_CONTROL_PANEL_ELEM_MODE_HOVER  2 /* Element is being hovered over */
static void /* @param: mode: One of `ELEM_MODE_*' */
WtgControlPanel_DrawElement(HDC hdc, UINT32 *base, unsigned int elem,
                            size_t w, size_t h, size_t stride,
                            unsigned int mode) {
#define PIXEL(x, y) base[(x) + ((y) * stride)]
	UINT32 color;
	size_t x, y;
	color = GUI_RGBA(0, 0, 0, 0xff);
	if (mode == WTG_CONTROL_PANEL_ELEM_MODE_HOVER)
		color = GUI_RGBA(0, 0, 0xff, 0xff);
	else if (mode == WTG_CONTROL_PANEL_ELEM_MODE_HELD) {
		color = GUI_RGBA(0xff, 0, 0, 0xff);
	}
	for (x = 0; x < w; ++x) {
		PIXEL(x, 0)     = color;
		PIXEL(x, h - 1) = color;
	}
	for (y = 1; y < h - 1; ++y) {
		PIXEL(0, y)     = color;
		PIXEL(w - 1, y) = color;
	}
	switch (elem) {

	case CP_ELEM_BRIGHTNESS:
	case CP_ELEM_VOLUME: {
		UINT32 fillColor;
		double pct;
		if (w <= 2 || h <= 2)
			break;
		w -= 2;
		h -= 2;
		base += stride;
		base += 1;
		if (elem == CP_ELEM_BRIGHTNESS) {
			pct       = Sys_GetBrightness();
			fillColor = GUI_RGBA(0x78, 0xdb, 0x61, 0xff);
		} else {
			pct       = (double)Sys_GetVolume();
			fillColor = GUI_RGBA(0x26, 0x92, 0xAD, 0xff);
		}
		if (WtgControlPanel_IsSliderVertical(elem)) {
			y = (size_t)(pct * (double)h);
			while (y) {
				UINT32 *scanline;
				--y;
				scanline = &PIXEL(0, h - y);
				for (x = 0; x < w; ++x)
					scanline[x] = fillColor;
			}
		} else {
			w = (size_t)(pct * (double)w);
			for (y = 0; y < h; ++y) {
				UINT32 *scanline = &PIXEL(0, y);
				for (x = 0; x < w; ++x)
					scanline[x] = fillColor;
			}
		}
		{
			char text[64];
			int len = sprintf(text, "%u%%", (unsigned int)(100 * pct));
			DrawTextA(hdc, text, len, &cp.cp_elems[elem],
			          DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
	}	break;

	case CP_ELEM_CLK_BATTERY: {
		LONG w, h;
		RECT clkRc = cp.cp_elems[elem];
		POINT clkCenter;
		GetLocalTime(&cp.cp_now);
		w = RC_WIDTH(clkRc);
		h = RC_HEIGHT(clkRc);
		if (w < h) {
			clkRc.bottom = clkRc.top + w;
			h = w;
		} else {
			clkRc.right = clkRc.left + h;
			w = h;
		}
		clkCenter.x = clkRc.left + w / 2;
		clkCenter.y = clkRc.top + h / 2;
		Ellipse(hdc, clkRc.left, clkRc.top, clkRc.right, clkRc.bottom);
		{
			char nowbuf[64];
			int len;
			LONG halfHeight = h / 2;

			len = sprintf(nowbuf, "%.2u:%.2u:%.2u", cp.cp_now.wHour, cp.cp_now.wMinute, cp.cp_now.wSecond);
			clkRc.bottom -= halfHeight;
			DrawTextA(hdc, nowbuf, len, &clkRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			clkRc.bottom += halfHeight;
			clkRc.top += halfHeight;
			len = sprintf(nowbuf, "%.2u.%.2u.%u",
			              cp.cp_now.wDay, cp.cp_now.wMonth,
			              cp.cp_now.wYear);
			DrawTextA(hdc, nowbuf, len, &clkRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
		WtgControlPanel_DrawClockHandle(hdc, &clkCenter, (double)cp.cp_now.wSecond / 60.0, (double)(clkCenter.x - clkRc.left));
		WtgControlPanel_DrawClockHandle(hdc, &clkCenter, (double)cp.cp_now.wMinute / 60.0, (double)(clkCenter.x - clkRc.left) * (2.0 / 3.0));
		WtgControlPanel_DrawClockHandle(hdc, &clkCenter, (double)(cp.cp_now.wHour % 12) / 12.0, (double)(clkCenter.x - clkRc.left) * (1.0 / 3.0));
	}	break;

	case CP_ELEM_TGL_WIFI: {
		if (Sys_GetWifiEnabled()) {
			for (y = 1; y < h - 1; ++y) {
				for (x = 1; x < w - 1; ++x) {
					PIXEL(x, y) = GUI_RGBA(0x78, 0xdb, 0x61, 0xff);
				}
			}
		}
		if (w >= GUI_WIDTH(gui_cp_wifi) &&
		    h >= GUI_HEIGHT(gui_cp_wifi)) {
			LONG x, y;
			x = (w - GUI_WIDTH(gui_cp_wifi)) / 2;
			y = (h - GUI_HEIGHT(gui_cp_wifi)) / 2;
			Wtg_PerPixelBlend(&PIXEL(x, y), gui_cp_wifi,
			                  GUI_WIDTH(gui_cp_wifi),
			                  GUI_HEIGHT(gui_cp_wifi), stride,
			                  GUI_WIDTH(gui_cp_wifi));
		}
	}	break;

	case CP_ELEM_TGL_BLUETOOTH: {
		if (Sys_GetBluetoothEnabled()) {
			for (y = 1; y < h - 1; ++y) {
				for (x = 1; x < w - 1; ++x) {
					PIXEL(x, y) = GUI_RGBA(0x78, 0xdb, 0x61, 0xff);
				}
			}
		}
		if (w >= GUI_WIDTH(gui_cp_bth) &&
		    h >= GUI_HEIGHT(gui_cp_bth)) {
			LONG x, y;
			x = (w - GUI_WIDTH(gui_cp_bth)) / 2;
			y = (h - GUI_HEIGHT(gui_cp_bth)) / 2;
			Wtg_PerPixelBlend(&PIXEL(x, y), gui_cp_bth,
			                  GUI_WIDTH(gui_cp_bth),
			                  GUI_HEIGHT(gui_cp_bth), stride,
			                  GUI_WIDTH(gui_cp_bth));
		}
	}	break;

	default:
		break;
	}
#undef PIXEL
}

static void WtgControlPanel_Draw(HDC hdc, UINT32 *image) {
	size_t x, y, w, h;
	w = cp.cp_size.x;
	h = cp.cp_size.y;
	if (w <= ((CP_BORDER * 2) + 1) ||
	    h <= (CP_BORDER + 1))
		return; /* Too small (shouldn't happen) */
	{
		UINT32 center;
		memcpy(&center, gui_cp_c, 4);
		for (y = 0; y < h - CP_BORDER; ++y) {
			UINT32 *scanline = &image[y * w];
			for (x = CP_BORDER; x < w - CP_BORDER; ++x)
				scanline[x] = center;
			memcpy((BYTE *)&scanline[0], gui_cp_l, sizeof(gui_cp_l));
			memcpy((BYTE *)&scanline[w] - sizeof(gui_cp_r), gui_cp_r, sizeof(gui_cp_r));
		}
	}
	Wtg_PerPixelBlit(image, 0, h - CP_BORDER, gui_cp_bl, CP_BORDER, CP_BORDER);
	Wtg_PerPixelBlit(image, w - CP_BORDER, h - CP_BORDER, gui_cp_br, CP_BORDER, CP_BORDER);
	for (y = 0; y < CP_BORDER; ++y) {
		UINT32 color, *scanline = &image[((h - CP_BORDER) + y) * w];
		memcpy(&color, gui_cp_b[0][y], 4);
		for (x = CP_BORDER; x < w - CP_BORDER; ++x)
			scanline[x] = color;
	}
	/* Render control elements. */
	{
		unsigned int i;
		for (i = 0; i < CP_ELEM_COUNT; ++i) {
			unsigned int mode;
			if (cp.cp_elems[i].left >= cp.cp_elems[i].right ||
			    cp.cp_elems[i].top >= cp.cp_elems[i].bottom)
				continue; /* Not visible... */
			mode = WTG_CONTROL_PANEL_ELEM_MODE_NORMAL;
			if (WtgControlPanelApplet_IsHeld(&cp, i))
				mode = WTG_CONTROL_PANEL_ELEM_MODE_HELD;
			else if (WtgControlPanelApplet_IsHover(&cp, i)) {
				mode = WTG_CONTROL_PANEL_ELEM_MODE_HOVER;
			}
			WtgControlPanel_DrawElement(hdc, image + cp.cp_elems[i].left + (cp.cp_elems[i].top * w),
			                            i, RC_WIDTH(cp.cp_elems[i]), RC_HEIGHT(cp.cp_elems[i]),
			                            w, mode);
		}
	}
}

void WtgControlPanel_Redraw(void) {
	BLENDFUNCTION blend;
	POINT ptZero;
	SIZE sSize;
	HDC hdcScreen, hdcMem;
	HBITMAP hBackground;
	HBITMAP hOldMemDcObject;
	void *dibBackgroundBits;
	BITMAPINFO bmInfo;
	ZeroMemory(&bmInfo, sizeof(bmInfo));
	hdcScreen = GetDC(NULL);
	if (!hdcScreen)
		LOGERROR_GLE("GetDC(NULL)");

	/* Describe the background image. */
	bmInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bmInfo.bmiHeader.biWidth       = cp.cp_size.x;
	bmInfo.bmiHeader.biHeight      = -((LONG)cp.cp_size.y);
	bmInfo.bmiHeader.biPlanes      = 1;
	bmInfo.bmiHeader.biBitCount    = 32;
	bmInfo.bmiHeader.biCompression = BI_RGB;

	/* Allocate the pixel data buffer. */
	dibBackgroundBits = NULL;
	hBackground = CreateDIBSection(hdcScreen, &bmInfo, DIB_RGB_COLORS, &dibBackgroundBits, NULL, 0);
	if (hBackground == NULL)
		LOGERROR_GLE("CreateDIBSection()");

	/* Get the background image into the window! */
	hdcMem = CreateCompatibleDC(hdcScreen);
	if (!hdcMem)
		LOGERROR_GLE("CreateCompatibleDC()");
	hOldMemDcObject = (HBITMAP)SelectObject(hdcMem, hBackground);
	if (hOldMemDcObject == NULL || hOldMemDcObject == HGDI_ERROR)
		LOGERROR_GLE("SelectObject()");

	SelectObject(hdcMem, GetStockObject(DC_BRUSH));
	SelectObject(hdcMem, GetStockObject(DC_PEN));
	SetDCBrushColor(hdcMem, RGB(0x63, 0x63, 0x63));
	SetDCPenColor(hdcMem, RGB(0, 0, 0));
	SetTextColor(hdcMem, RGB(0xff, 0xff, 0xff));
	SetBkMode(hdcMem, TRANSPARENT);

	/* Actually render the control panel. */
	WtgControlPanel_Draw(hdcMem, (UINT32 *)dibBackgroundBits);

	/* Make sure that nothing within the client area is transparent. */
	{
		LONG y, x, w;
		UINT32 *scanline;
		scanline = (UINT32 *)dibBackgroundBits + (cp.cp_client.top * cp.cp_size.x);
		scanline += cp.cp_client.left;
		w = RC_WIDTH(cp.cp_client);
		for (y = cp.cp_client.top; y < cp.cp_client.bottom; ++y) {
			for (x = 0; x < w; ++x)
				scanline[x] |= 0xff000000;
			scanline += cp.cp_size.x;
		}
	}

	blend.BlendOp             = AC_SRC_OVER;
	blend.BlendFlags          = 0;
	blend.SourceConstantAlpha = 255;
	blend.AlphaFormat         = AC_SRC_ALPHA;
	sSize.cx                  = cp.cp_size.x;
	sSize.cy                  = cp.cp_size.y;
	ptZero.x                  = 0;
	ptZero.y                  = 0;
	if (!UpdateLayeredWindow(cp.cp_window, hdcScreen, NULL, &sSize,
	                         hdcMem, &ptZero, RGB(0, 0, 0), &blend, ULW_ALPHA))
		LOGERROR_GLE("UpdateLayeredWindow()");
	SelectObject(hdcMem, hOldMemDcObject);
	DeleteDC(hdcMem);
	DeleteObject(hBackground);
	ReleaseDC(NULL, hdcScreen);
}

/* Lookup & return the index (one of CP_ELEM_*)
 * of the element that contains the given coords. */
static unsigned int WtgControlPanel_ElemAt(LONG x, LONG y) {
	unsigned int i;
	for (i = 0; i < CP_ELEM_COUNT; ++i) {
		if (x >= cp.cp_elems[i].left && x < cp.cp_elems[i].right &&
		    y >= cp.cp_elems[i].top && y < cp.cp_elems[i].bottom)
			return i; /* Found it! */
	}
	return CP_ELEM_NONE;
}


/* Handle the user clicking on the index'd `elem'
 * Called after a release event if still within the
 * bounds of the original element.
 * @return: true: Need to do a re-draw */
static bool WtgControlPanel_ClickElem(unsigned int elem) {
	switch (elem) {

	case CP_ELEM_TGL_WIFI:
		Sys_SetWifiEnabled(!Sys_GetWifiEnabled());
		return true;

	case CP_ELEM_TGL_BLUETOOTH:
		Sys_SetBluetoothEnabled(!Sys_GetBluetoothEnabled());
		return true;

	default:
		break;
	}

	// TODO: CP_ELEM_TGL_ROTLOCK
	// TODO: CP_ELEM_TGL_FLIGHTMODE
	// TODO: CP_ELEM_TGL_NIGHTMODE
	// TODO: CP_ELEM_TGL_PWRSAVE
	return false;
}

/* Called whenever moving the pointer after an initial
 * down-event on-top of `elem' (including immediately
 * after the initial down-event)
 * Coords are relative to the top-left corner of the element. */
static void WtgControlPanel_DragElem(unsigned int elem, LONG x, LONG y) {
	debug_printf("WtgControlPanel_DragElem(%u, %d, %d)\n", elem, x, y);
	switch (elem) {

	case CP_ELEM_BRIGHTNESS: {
		double position;
		position = WtgControlPanel_GetSliderPositionAt(elem, x, y);
		Sys_SetBrightness(position);
		WtgControlPanel_Redraw();
	}	break;

	case CP_ELEM_VOLUME: {
		float position;
		position = (float)WtgControlPanel_GetSliderPositionAt(elem, x, y);
		Sys_SetVolume(position);
		WtgControlPanel_Redraw();
	}	break;

	default:
		break;
	}
}

/* Same as `WtgControlPanel_DragElem()', but coords are relative to the control panel. */
static void WtgControlPanel_DragElemWrapper(unsigned int elem, LONG x, LONG y) {
	x -= cp.cp_elems[elem].left;
	y -= cp.cp_elems[elem].top;
	WtgControlPanel_DragElem(elem, x, y);
}

static bool WtgControlPanel_OnPointerDown(UINT32 pointerId, LONG x, LONG y) {
	unsigned int elem = WtgControlPanel_ElemAt(x, y);
	WtgControlPanelApplet_SetTouch(&cp, pointerId, elem, elem);
	if (elem != CP_ELEM_NONE) {
		WtgControlPanel_DragElemWrapper(elem, x, y);
		WtgControlPanel_Redraw();
	}
	return true;
}

static bool WtgControlPanel_OnPointerUp(UINT32 pointerId, LONG x, LONG y, BOOL hover) {
	bool mustRedraw = false;
	unsigned int origElem, currElem;
	origElem = WtgControlPanelApplet_GetTouch(&cp, pointerId);
	currElem = WtgControlPanel_ElemAt(x, y);
	if (hover && currElem != CP_ELEM_NONE) {
		WtgControlPanelApplet_SetTouch(&cp, pointerId, CP_ELEM_NONE, currElem);
	} else {
		WtgControlPanelApplet_DelTouch(&cp, pointerId);
	}
	if (origElem != CP_ELEM_NONE) {
		WtgControlPanel_DragElemWrapper(origElem, x, y);
		if (origElem == currElem) {
			mustRedraw = WtgControlPanel_ClickElem(origElem);
		}
	}
	if (origElem != CP_ELEM_NONE)
		mustRedraw = true;
	if (mustRedraw)
		WtgControlPanel_Redraw();
	return true;
}

static bool WtgControlPanel_OnPointerMove(UINT32 pointerId, LONG x, LONG y, BOOL held) {
	if (held) {
		/* Drag */
		unsigned int elem;
		elem = WtgControlPanelApplet_GetTouch(&cp, pointerId);
		if (elem != CP_ELEM_NONE)
			WtgControlPanel_DragElemWrapper(elem, x, y);
	} else {
		/* Hover */
		unsigned int oldElem, newElem;
		oldElem = WtgControlPanelApplet_GetHover(&cp, pointerId);
		newElem = WtgControlPanel_ElemAt(x, y);
		if (oldElem != newElem) {
			WtgControlPanelApplet_SetTouch(&cp, pointerId, CP_ELEM_NONE, newElem);
			WtgControlPanel_Redraw();
		}
	}
	return true;
}

static LRESULT WINAPI /* For the control-panel window */
WtgControlPanel_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {

	case WM_TIMER: {
		SYSTEMTIME now;
		GetLocalTime(&now);
		SetTimer(cp.cp_window, cp.cp_timer,
		         1000 - now.wMilliseconds,
		         NULL);
		if (memcmp(&cp.cp_now, &now, sizeof(now)) != 0) {
			cp.cp_now = now;
			WtgControlPanel_Redraw();
		}
	}	break;

	case WM_KILLFOCUS: {
		if (cp.cp_active) {
			WtgControlPanelApplet_Fini(&cp);
			return 0;
		}
	}	break;

	case WM_POINTERDOWN:
		if (IS_POINTER_INCONTACT_WPARAM(wParam)) {
			POINT pt;
			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);
			if (ScreenToClient(hWnd, &pt)) {
				if (WtgControlPanel_OnPointerDown(GET_POINTERID_WPARAM(wParam), pt.x, pt.y))
					return 0;
			}
		}
		break;

	case WM_POINTERUPDATE:
		if (IS_POINTER_INRANGE_WPARAM(wParam) || IS_POINTER_INCONTACT_WPARAM(wParam)) {
			POINT pt;
			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);
			if (ScreenToClient(hWnd, &pt)) {
				if (WtgControlPanel_OnPointerMove(GET_POINTERID_WPARAM(wParam), pt.x, pt.y,
				                                  IS_POINTER_INCONTACT_WPARAM(wParam)))
					return 0;
			}
		}
		break;

	case WM_POINTERUP: {
		if (IS_POINTER_CANCELED_WPARAM(wParam)) {
			if (WtgControlPanelApplet_DelTouch(&cp, GET_POINTERID_WPARAM(wParam)))
				WtgControlPanel_Redraw();
		} else if (!IS_POINTER_INCONTACT_WPARAM(wParam)) {
			POINT pt;
			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);
			if (ScreenToClient(hWnd, &pt)) {
				if (WtgControlPanel_OnPointerUp(GET_POINTERID_WPARAM(wParam), pt.x, pt.y,
				                                IS_POINTER_INRANGE_WPARAM(wParam)))
					return 0;
			}
		}
	}	break;

#if 0
	case WM_POINTERLEAVE: {
		UINT32 id = GET_POINTERID_WPARAM(wParam);
		if (IS_POINTER_INCONTACT_WPARAM(wParam)) {
			POINT pt;
			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);
			if (ScreenToClient(hWnd, &pt))
				WtgControlPanel_OnPointerUp(id, pt.x, pt.y);
		}
		if (WtgControlPanelApplet_DelTouch(&cp, id))
			WtgControlPanel_Redraw();
		return 0;
	}	break;
#endif

	case WM_LBUTTONDOWN:
		if (WtgControlPanel_OnPointerDown((UINT32)-1,
		                                  GET_X_LPARAM(lParam),
		                                  GET_Y_LPARAM(lParam)))
			return 0;
		break;

	case WM_LBUTTONUP:
		if (WtgControlPanel_OnPointerUp((UINT32)-1,
		                                GET_X_LPARAM(lParam),
		                                GET_Y_LPARAM(lParam),
		                                TRUE))
			return 0;
		break;

	case WM_MOUSEMOVE:
		if (WtgControlPanel_OnPointerMove((UINT32)-1,
		                                  GET_X_LPARAM(lParam),
		                                  GET_Y_LPARAM(lParam),
		                                  (wParam & MK_LBUTTON) != 0))
			return 0;
		break;

	case WM_MOUSELEAVE: {
		if (WtgControlPanelApplet_DelTouch(&cp, (UINT32)-1))
			WtgControlPanel_Redraw();
		return 0;
	}	break;

	default: break;
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static void WtgControlPanel_PlaceElements(void) {
	RECT lQuarter, mHalf, rQuarter;
	LONG buttonRows, buttonCols;
	unsigned int i;
	double button_w, button_h;

	lQuarter.top    = cp.cp_client.top;
	lQuarter.left   = cp.cp_client.left;
	lQuarter.right  = cp.cp_client.left + (RC_WIDTH(cp.cp_client) / 4);
	lQuarter.bottom = cp.cp_client.bottom;
	rQuarter.top    = cp.cp_client.top;
	rQuarter.left   = cp.cp_client.right - (RC_WIDTH(cp.cp_client) / 4);
	rQuarter.right  = cp.cp_client.right;
	rQuarter.bottom = cp.cp_client.bottom;
	mHalf.top       = cp.cp_client.top;
	mHalf.left      = lQuarter.right;
	mHalf.right     = rQuarter.left;
	mHalf.bottom    = cp.cp_client.bottom;

	{
		LONG rQuarterW, rQuarterH;
		rQuarterW = RC_WIDTH(rQuarter);
		rQuarterH = RC_HEIGHT(rQuarter);
		if (rQuarterW >= rQuarterH) {
			/* Horizontal sliders */
			cp.cp_elems[CP_ELEM_BRIGHTNESS]        = rQuarter;
			cp.cp_elems[CP_ELEM_BRIGHTNESS].bottom = rQuarter.top + (rQuarterH / 2);
			cp.cp_elems[CP_ELEM_VOLUME]            = rQuarter;
			cp.cp_elems[CP_ELEM_VOLUME].top        = cp.cp_elems[CP_ELEM_BRIGHTNESS].bottom;
		} else {
			/* Vertical sliders */
			cp.cp_elems[CP_ELEM_BRIGHTNESS]       = rQuarter;
			cp.cp_elems[CP_ELEM_BRIGHTNESS].right = rQuarter.left + (rQuarterW / 2);
			cp.cp_elems[CP_ELEM_VOLUME]           = rQuarter;
			cp.cp_elems[CP_ELEM_VOLUME].left      = cp.cp_elems[CP_ELEM_BRIGHTNESS].right;
		}
	}
	cp.cp_elems[CP_ELEM_CLK_BATTERY] = lQuarter;

	buttonCols = RC_WIDTH(mHalf) / CP_BUTTON_WIDTH;
	buttonRows = RC_HEIGHT(mHalf) / CP_BUTTON_HEIGHT;

	if (buttonCols <= 0 || buttonRows <= 0)
		return; /* Cannot place any buttons... :( */
	button_w = (double)RC_WIDTH(mHalf) / (double)buttonCols;
	button_h = (double)RC_HEIGHT(mHalf) / (double)buttonRows;
	for (i = CP_ELEM_BUTTONS_FIRST; i <= CP_ELEM_BUTTONS_LAST; ++i) {
		LONG grid_x, grid_y;
		LONG button_center_x, button_center_y;
		grid_x = (i - CP_ELEM_BUTTONS_FIRST) % buttonCols;
		grid_y = (i - CP_ELEM_BUTTONS_FIRST) / buttonCols;
		if (grid_y >= buttonRows) {
			/* XXX: If this every becomes an issue, maybe implement scrolling? */
			break; /* Out of bounds... :( */
		}
		grid_x = (buttonCols - 1) - grid_x; /* Start from the right */
		button_center_x = mHalf.left + (LONG)((button_w * grid_x) + (button_w / 2.0));
		button_center_y = mHalf.top  + (LONG)((button_h * grid_y) + (button_h / 2.0));
		cp.cp_elems[i].left   = button_center_x - (CP_BUTTON_WIDTH / 2);
		cp.cp_elems[i].top    = button_center_y - (CP_BUTTON_HEIGHT / 2);
		cp.cp_elems[i].right  = cp.cp_elems[i].left + CP_BUTTON_WIDTH;
		cp.cp_elems[i].bottom = cp.cp_elems[i].top + CP_BUTTON_HEIGHT;
	}
}

#define APPLET_CP_CLASSNAME L"ControlPanel"
static bool WtgControlPanel_RegisterWindowClass(void) {
	static ATOM applet_cp_class = 0;
	WNDCLASSEXW WindowClassEx;
	if (applet_cp_class)
		return true;
	ZeroMemory(&WindowClassEx, sizeof(WNDCLASSEXW));
	WindowClassEx.cbSize        = sizeof(WNDCLASSEXW);
	WindowClassEx.lpfnWndProc   = &WtgControlPanel_WndProc;
	WindowClassEx.hInstance     = hApplicationInstance;
	WindowClassEx.lpszClassName = APPLET_CP_CLASSNAME;
	WindowClassEx.hCursor       = LoadCursorA(NULL, IDC_ARROW);
	applet_cp_class             = RegisterClassExW(&WindowClassEx);
	return applet_cp_class != 0;
}

static HWND WtgControlPanel_GetOrCreateWindow(void) {
	if (!cp.cp_window) {
		if (!WtgControlPanel_RegisterWindowClass())
			return NULL;
		memset(&cp, 0, sizeof(cp));
		cp.cp_size.x = ga.ga_oversiz.x - (ga.ga_oversiz.x / 10);
		cp.cp_size.y = ga.ga_oversiz.y / 5;
		cp.cp_pos.x  = ga.ga_overpos.x + ((ga.ga_oversiz.x - cp.cp_size.x) / 2);
		cp.cp_pos.y  = ga.ga_overpos.y;
		cp.cp_client.left   = CP_BORDER;
		cp.cp_client.right  = cp.cp_size.x - CP_BORDER;
		cp.cp_client.top    = cp.cp_size.y;
		cp.cp_size.y        = cp.cp_size.y * 2;
		cp.cp_client.bottom = cp.cp_size.y - CP_BORDER;

		/* Use a wrapper to keep the window off of the task bar! */
		cp.cp_wrapper = WtgGestureAnimation_CreateOverlayWindow(&ga);
		if (cp.cp_wrapper) {
			/* Create the applet window. */
			cp.cp_window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED,
			                               APPLET_CP_CLASSNAME,
			                               NULL, WS_POPUP | WS_VISIBLE,
			                               cp.cp_pos.x,
			                               cp.cp_pos.y - cp.cp_size.y,
			                               cp.cp_size.x, cp.cp_size.y,
			                               cp.cp_wrapper, NULL,
			                               hApplicationInstance, NULL);
			if (cp.cp_window) {
				RegisterTouchWindow(cp.cp_window, 0);
				WtgControlPanel_PlaceElements();
				WtgControlPanel_Redraw();
				if (!SetForegroundWindow(cp.cp_window))
					LOGERROR_GLE("SetForegroundWindow()");
				GetLocalTime(&cp.cp_now);
				cp.cp_timer = SetTimer(cp.cp_window, 0,
				                       1000 - cp.cp_now.wMilliseconds,
				                       NULL);
			} else {
				LOGERROR_GLE("CreateWindowExW()");
				DestroyWindow(cp.cp_wrapper);
				cp.cp_wrapper = NULL;
			}
		}
	}
	return cp.cp_window;
}


/* Called when a vertical swipe gesture is started
 * @return: true:  Init ok
 * @return: false: Init failed (ignore the gesture) */
static bool WtgApplet_Init(void) {
	debug_printf("WtgApplet_Init()\n");
	return true;
}

static void WtgControlPanel_SetAnim(double visibility) {
	double yMax;
	LONG yPos; /* Y-position of bottom window border */
	if (visibility > 1.0)
		visibility = sqrt(sqrt(sqrt(sqrt(visibility))));
	else if (visibility < 0.0) {
		visibility = -(sqrt(sqrt(sqrt(sqrt(1.0 + -visibility)))) - 1.0);
	}
	yMax = (double)(cp.cp_size.y - cp.cp_client.top);
	yPos = (LONG)(visibility * yMax);
	if (visibility < 0.0)
		visibility = 0.0;
	if (visibility > 1.0)
		visibility = 1.0;
	SetWindowPos(cp.cp_window, NULL,
	             cp.cp_pos.x,
	             cp.cp_pos.y + (yPos - cp.cp_size.y), 0 ,0 ,
	             SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER |
	             SWP_NOCOPYBITS | SWP_NOOWNERZORDER |
	             SWP_NOSENDCHANGING);
	if (cp.cp_blur) {
		if (!SetLayeredWindowAttributes(cp.cp_blur, 0, (BYTE)(visibility * 255), LWA_ALPHA))
			LOGERROR_GLE("SetLayeredWindowAttributes()");
	} else {
		WtgGestureAnimation_SetBlur(&ga, visibility);
	}
}


/* Called to update the enter/exit animation of applets.
 * Called every time from `WtgGesture_Update()' */
static void WtgApplet_Show(void) {
	if (cp.cp_active || ga.ga_dist > 0.0) {
		if (WtgControlPanel_GetOrCreateWindow() != NULL) {
			double yMax, visibility;
			yMax = (double)(cp.cp_size.y - cp.cp_client.top);
			if (cp.cp_active) {
				/* Was already active before. */
				visibility = (yMax + ga.ga_dist) / yMax;
			} else {
				visibility = ga.ga_dist / yMax;
			}
			WtgControlPanel_SetAnim(visibility);
		}
		if (!as.as_active && as.as_window != NULL) {
			if (!SetLayeredWindowAttributes(as.as_window, 0, 0, LWA_ALPHA))
				LOGERROR_GLE("SetLayeredWindowAttributes()");
		}
		return;
	}
	if (cp.cp_window != NULL)
		WtgControlPanel_SetAnim(0.0);
	/* TODO: Application switcher animation */
	WtgGestureAnimation_SetBlur(&ga, 0.0);
}

/* Called when a vertical swipe gesture is finished
 * This function should make use of `ga' to determine if the
 * the applet currently visible should stay up.
 * @return: true:  The applet is staying up.
 * @return: false: The applet was destroyed. */
static bool WtgApplet_Done(void) {
	bool result = false;
	debug_printf("WtgApplet_Done(%f) [dist2:%f]\n", ga.ga_dist, ga.ga_dist2);

	/* Control panel... */
	{
		double yMax, dist = ga.ga_dist;
		yMax = (double)(cp.cp_size.y - cp.cp_client.top);
		if (!cp.cp_active) {
			if (dist >= yMax / 2.0) {
				/* Activate the control panel. */
				debug_printf("Activate CP\n");
				WtgControlPanel_SetAnim(1.0);
				cp.cp_active  = true;
				cp.cp_blur    = ga.ga_blurlay; /* Steal! */
				ga.ga_blurlay = NULL;
				result        = true;
				goto done;
			}
		} else {
			if (dist <= -(yMax / 3.0)) {
				/* Deactivate the control panel. */
				debug_printf("Deactivate CP\n");
				WtgControlPanelApplet_Fini(&cp);
				result = true;
				goto done_after_cp;
			}
			/* The control panel overlays the app switcher,
			 * so if the panel is active, don't given the
			 * switcher a chance at doing anything! */
			WtgControlPanel_SetAnim(1.0);
			goto done;
		}
	}
	/* TODO: Application switcher */

done:
	if (!cp.cp_active)
		WtgControlPanelApplet_Fini(&cp);
done_after_cp:
	if (!as.as_active)
		WtgAppSwitcherApplet_Fini(&as);
	return result;
}
/************************************************************************/












/************************************************************************/
/* Gesture animation information.                                       */
/************************************************************************/
static void
WtgGestureAnimation_SetBlur(WtgGestureAnimation *__restrict self,
                            double strength) {
	/* Clamp blur strength */
	if (strength < 0.0)
		strength = 0.0;
	if (strength > 1.0)
		strength = 1.0;
	if (self->ga_blurold == strength)
		return; /* Unchanged */
	/* Don't include additional blur effects when one of these is active! */
	if (cp.cp_active || as.as_active)
		return;
	if (!self->ga_blurlay) {
		self->ga_blurlay = WtgGestureAnimation_CreateBlurWindow(self);
		if (!self->ga_blurlay)
			return;
	}
	if (!SetLayeredWindowAttributes(self->ga_blurlay, 0,
	                                (BYTE)(strength * 255),
	                                LWA_ALPHA))
		LOGERROR_GLE("SetLayeredWindowAttributes()");
	self->ga_blurold = strength;
}

static double Wtg_ExpLowerNumber(double n) {
	return copysign(sqrt(fabs(n) + 1.0) - 1.0, n);
}

static POINT WtgGesture_GetSwipeOffset(bool movable) {
	POINT result;
	double hdelta = ga.ga_dist;
	double vdelta = ga.ga_dist2;
	if (ga.ga_kind == WTG_GESTURE_KIND_VSWIPE) {
		hdelta = ga.ga_dist2;
		vdelta = ga.ga_dist;
	}
	if (!movable) {
		hdelta = Wtg_ExpLowerNumber(hdelta);
		vdelta = Wtg_ExpLowerNumber(vdelta);
	}
	result.x = (LONG)hdelta;
	result.y = (LONG)vdelta;
	return result;
}

static HWND Wtg_FindWorkerWForShellDllDefView(void) {
	HWND hDesktop = GetDesktopWindow();
	HWND hWorkerW = NULL;
	HWND hShellViewWin;
	for (;;) {
		hWorkerW = FindWindowExW(hDesktop, hWorkerW, L"WorkerW", NULL);
		if (hWorkerW == NULL)
			break;
		hShellViewWin = FindWindowExW(hWorkerW, NULL, L"SHELLDLL_DefView", NULL);
		if (hShellViewWin != NULL)
			break;
	}
	return hWorkerW;
}


/* (Re-)Initialize globals used for tracking gestures. */
static void WtgGesture_Start(void) {
	HWND fw, shell;
	debug_printf("WtgGesture_Start()\n");
	fw    = GetForegroundWindow();
	shell = GetShellWindow();
	if (!fw)
		fw = GetActiveWindow();
	if (!fw)
		fw = shell;
	for (;;) {
		HWND hParent;
		hParent = GetParent(fw);
		if (!hParent || hParent == shell)
			break;
		fw = hParent;
	}
	if (fw == shell) {
		fw = GetWindow(fw, GW_CHILD);
		if (fw != NULL) {
		    if (GetWindow(fw, GW_HWNDNEXT) != NULL ||
			    GetWindow(fw, GW_HWNDPREV) != NULL)
				fw = NULL;
			else {
				/* Undocumented command. Used to move `SHELLDLL_DefView'
				 * into a separate `WorkerW' root-window, which we can
				 * then hook into to only move desktop icons.
				 * >> https://www.codeproject.com/questions/1036180/can-i-somehow-set-the-parent-of-shelldll-defview-h
				 */
				unsigned int attempt = 0;
				printf("[trace] Send message 0x052c to shell window (%p)\n", shell);
				SendMessageW(shell, 0x052c, 0, 0);
				for (attempt = 0; attempt < 64; ++attempt) {
					fw = Wtg_FindWorkerWForShellDllDefView();
					if (fw != NULL)
						break;
					Sleep(1);
				}
			}
		}
		if (fw == NULL) {
			/* Find a `WorkerW' window that has a `SHELLDLL_DefView' child. */
			fw = Wtg_FindWorkerWForShellDllDefView();
		}
	}
	if (!WtgGestureAnimation_Init(&ga, fw)) {
		memset(&ga, 0, sizeof(ga));
		ga.ga_kind = WTG_GESTURE_KIND_IGNORE;
	}
}


static double
Wtg_CalculateVisibilityOpacity(LONG total, LONG lo, LONG hi) {
	if (lo < 0)
		lo = 0;
	if (hi > total)
		hi = total;
	if (lo >= hi)
		return 0;
	return (double)(hi - lo) / total;
}

static double
Wtg_CalculateVisibilityOpacityEx(LONG total, LONG lo, LONG hi,
                                 double divisor) {
	double result;
	result = Wtg_CalculateVisibilityOpacity(total, lo, hi);
	result = (result * divisor) - (divisor - 1.0);
	if (result < 0.0)
		result = 0.0;
	if (result > 1.0)
		result = 1.0;
	return result;
}

static HWND last_minimized_window = NULL;

/* Called when one of the touch-points moves inside of a gesture.
 * Note called when a touch-point is added/removed */
static void WtgGesture_Update(void) {
	if (ga.ga_kind == WTG_GESTURE_KIND_NONE) {
		/* Check if we can recognize the gesture
		 * For this purpose, we will commit to one of the possible
		 * gestures by looking at the average delta between the
		 * current touch positions, and their counterparts when
		 * the gesture began. */
		double start_distance = WtgGesture_CalculateStartDistance();
		double now_distance   = WtgGesture_CalculateCurrDistance();
		double distance_diff  = now_distance - start_distance;
		/* Recognize zoom/pinch by comparing `start_distance' and `now_distance' */
		if (distance_diff >= WTG_GESTURE_ZOOM_THRESHOLD ||
		    distance_diff <= -WTG_GESTURE_ZOOM_THRESHOLD) {
			ga.ga_dist = distance_diff;
			ga.ga_kind = WTG_GESTURE_KIND_ZOOM;
		} else {
			/* Recognize H/V swiping by looking at the delta between start and now. */
			double hdelta = WtgGesture_CalculateHDeltaTotal();
			double vdelta = WtgGesture_CalculateVDeltaTotal();
			if (hdelta >= WTG_GESTURE_SWIPE_THRESHOLD ||
			    hdelta <= -WTG_GESTURE_SWIPE_THRESHOLD) {
				ga.ga_dist  = hdelta;
				ga.ga_dist2 = vdelta;
				ga.ga_kind  = WTG_GESTURE_KIND_HSWIPE;
			} else {
				if (vdelta >= WTG_GESTURE_SWIPE_THRESHOLD ||
				    vdelta <= -WTG_GESTURE_SWIPE_THRESHOLD) {
					ga.ga_dist  = vdelta;
					ga.ga_dist2 = hdelta;
					ga.ga_kind  = WTG_GESTURE_KIND_VSWIPE;
				}
			}
		}
		if (ga.ga_kind != WTG_GESTURE_KIND_NONE) {
			HWND animwin;
			unsigned int animwin_kind;
			/* Gesture started. */
			animwin      = ga.ga_fgwin;
			animwin_kind = ga.ga_fgkind;
			if (animwin != NULL && (animwin == as.as_window ||
			                        animwin == cp.cp_window ||
			                        animwin == cp.cp_wrapper)) {
				if (ga.ga_kind != WTG_GESTURE_KIND_VSWIPE)
					goto ignore_gesture;
				ga.ga_kind = WTG_GESTURE_KIND_VSWIPE_APP;
				goto after_anim_init;
			}
			if (animwin_kind == WTG_WINDOW_KIND_DESKTOP &&
			    ga.ga_kind == WTG_GESTURE_KIND_ZOOM &&
			    last_minimized_window != NULL) {
				unsigned int lastmin_feat;
				lastmin_feat = Sys_GetWindowFeatures(last_minimized_window);
				if (lastmin_feat & SYS_WINDOW_FEAT_MINIMIZE) {
					WINDOWPLACEMENT placement;
					if (GetWindowPlacement(last_minimized_window, &placement)) {
						if (placement.showCmd == SW_MINIMIZE ||
						    placement.showCmd == SW_SHOWMINIMIZED ||
						    placement.showCmd == SW_SHOWMINNOACTIVE) {
							animwin                = last_minimized_window;
							animwin_kind           = lastmin_feat;
							ga.ga_anim.wa_animwin  = animwin;
							ga.ga_anim.wa_animkind = animwin_kind;
							ga.ga_anim.wa_rect     = placement.rcNormalPosition;
							if (!WtgAnimatedWindow_InitEx(&ga.ga_anim, ga.ga_overlay, TRUE))
								goto ignore_gesture;
							ga.ga_kind = WTG_GESTURE_KIND_ZOOM_RSTOR;
							goto after_anim_init;
						}
					}
				}
			}
			if (ga.ga_kind == WTG_GESTURE_KIND_VSWIPE &&
			    (animwin_kind == WTG_WINDOW_KIND_DESKTOP ||
			     animwin_kind == WTG_WINDOW_KIND_MAXIMIZED)) {
				/* Do an applet-based swipe animation. */
				ga.ga_kind = WTG_GESTURE_KIND_VSWIPE_APP;
				if (!WtgApplet_Init())
					goto ignore_gesture;
				goto do_update_gesture_initial;
			}
			if (animwin != NULL &&
			    WtgAnimatedWindow_Init(&ga.ga_anim, animwin, ga.ga_overlay,
			                     animwin_kind, TRUE)) {
				/* OK! */
			} else {
ignore_gesture:
				ga.ga_kind = WTG_GESTURE_KIND_IGNORE;
			}
after_anim_init:
			if (animwin != NULL && ga.ga_kind == WTG_GESTURE_KIND_HSWIPE &&
			    ga.ga_anim.wa_animkind == WTG_WINDOW_KIND_MAXIMIZED) {
				HWND prev, next;

				/* Reload the list of application windows. */
				WtgAppWindowList_Reload(ga.ga_fgwin);

				/* Load the predecessor/successor of the current application window. */
				prev = WtgAppList_Prev(animwin);
				next = WtgAppList_Next(animwin);

				ga.ga_prev.wa_animwin = prev;
				ga.ga_next.wa_animwin = next;
				if (next != NULL &&
				    !WtgAnimatedWindow_Init(&ga.ga_next, next, ga.ga_overlay,
				                      Wtg_GetWindowKind(next), FALSE)) {
					WtgAnimatedWindow_Fini(&ga.ga_anim);
					ga.ga_kind = WTG_GESTURE_KIND_IGNORE;
				} else {
					if (prev != NULL &&
					    !WtgAnimatedWindow_Init(&ga.ga_prev, prev, ga.ga_overlay,
					                      Wtg_GetWindowKind(prev), FALSE)) {
						WtgAnimatedWindow_Fini(&ga.ga_next);
						WtgAnimatedWindow_Fini(&ga.ga_anim);
						ga.ga_kind = WTG_GESTURE_KIND_IGNORE;
					}
				}
			}
			/* Update the gesture for the first time. */
do_update_gesture_initial:
			if (WTG_GESTURE_KIND_VALID(ga.ga_kind)) {
				size_t i;
				for (i = 0; i < gesture_count; ++i) {
					gesture_info[i].gt_delta.x = 0;
					gesture_info[i].gt_delta.y = 0;
				}
				goto do_update_gesture;
			}
		}
	} else if (ga.ga_kind != WTG_GESTURE_KIND_IGNORE) {
		DWM_THUMBNAIL_PROPERTIES tnProperties;
		/* Using the DWM Thumbnail API, play some nice animations
		 * with the contents of active windows (e.g. shrink/enlarge
		 * for pinch/zoom) */
do_update_gesture:
		switch (ga.ga_kind) {

		case WTG_GESTURE_KIND_VSWIPE_APP:
			ga.ga_dist  += WtgGesture_CalculateVDelta();
			ga.ga_dist2 += WtgGesture_CalculateHDelta();
			WtgApplet_Show();
			return;

		case WTG_GESTURE_KIND_ZOOM:
		case WTG_GESTURE_KIND_ZOOM_RSTOR: {
			double delta, prev, now;
			prev  = WtgGesture_CalculatePrevDistance();
			now   = WtgGesture_CalculateCurrDistance();
			delta = now - prev;
			ga.ga_dist += delta;
		}	break;

		case WTG_GESTURE_KIND_HSWIPE:
			ga.ga_dist  += WtgGesture_CalculateHDelta();
			ga.ga_dist2 += WtgGesture_CalculateVDelta();
			break;

		case WTG_GESTURE_KIND_VSWIPE:
			ga.ga_dist  += WtgGesture_CalculateVDelta();
			ga.ga_dist2 += WtgGesture_CalculateHDelta();
			break;

		default:
			break;
		}

		/* Animate the current window. */
		tnProperties.dwFlags = DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE;
		tnProperties.rcDestination = ga.ga_anim.wa_rect;
		switch (ga.ga_kind) {

		case WTG_GESTURE_KIND_ZOOM:
		case WTG_GESTURE_KIND_ZOOM_RSTOR: {
			double scale, radius, distance, max_scale;
			RECT src, dst, scrollspace;
			WINDOWPLACEMENT wp;
			distance  = ga.ga_dist;
			src       = ga.ga_anim.wa_rect;
			max_scale = 1.0;
			if (ga.ga_kind == WTG_GESTURE_KIND_ZOOM_RSTOR) {
				/* Zoom out towards restore */
				dst        = src;
				src.left   = dst.left + RC_WIDTH(dst) / 2;
				src.top    = dst.top + RC_HEIGHT(dst) / 2;
				src.right  = src.left;
				src.bottom = src.top;
			} else if (ga.ga_anim.wa_animkind == WTG_WINDOW_KIND_NORMAL) {
				if (distance < 0) {
					/* Zoom out towards minimize */
					dst.left   = src.left + RC_WIDTH(src) / 2;
					dst.top    = src.top + RC_HEIGHT(src) / 2;
					dst.right  = dst.left;
					dst.bottom = dst.top;
					distance   = -distance;
					if (!(ga.ga_fgfeat & SYS_WINDOW_FEAT_MINIMIZE))
						max_scale = 0.0;
				} else {
					/* Zoom in towards maximize */
					dst.left   = ga.ga_overpos.x;
					dst.top    = ga.ga_overpos.y;
					dst.right  = ga.ga_overpos.x + ga.ga_oversiz.x;
					dst.bottom = ga.ga_overpos.y + ga.ga_oversiz.y;
					if (!(ga.ga_fgfeat & SYS_WINDOW_FEAT_MAXIMIZE)) {
						max_scale = 0.0;
						if (dst.left == src.left && dst.top == src.top &&
						    dst.right == src.right && dst.bottom == src.bottom) {
							/* Special case: Full-screen window without the maximize flag set!
							 * Maximization isn't allowed, but we should still allow the user
							 * to zoom in a little bit, only to snap back afterwards. */
							dst.left -= 32;
							dst.top -= 18;
							dst.right += 32;
							dst.bottom += 18;
						}
					}
				}
			} else if (ga.ga_anim.wa_animkind == WTG_WINDOW_KIND_MAXIMIZED) {
				if (!(ga.ga_fgfeat & SYS_WINDOW_FEAT_MAXIMIZE)) {
					if (!(ga.ga_fgfeat & SYS_WINDOW_FEAT_MINIMIZE))
						max_scale = 0.0;
					goto set_fallback_dst_position;
				}
				if (GetWindowPlacement(ga.ga_anim.wa_animwin, &wp))
					dst = wp.rcNormalPosition;
				else {
set_fallback_dst_position:
					dst.left = src.left;
					dst.left += RC_WIDTH(src) / 2;
					dst.top = src.top;
					dst.top += RC_HEIGHT(src) / 2;
					dst.right  = dst.left;
					dst.bottom = dst.top;
				}
				distance = -distance;
			} else {
				max_scale = 0.0;
				goto set_fallback_dst_position;
			}
			/* Figure out how much scroll-space there is all around. */
			scrollspace.left   = dst.left - src.left;
			scrollspace.top    = dst.top - src.top;
			scrollspace.right  = dst.right - src.right;
			scrollspace.bottom = dst.bottom - src.bottom;

			/* Figure out the average travel-radius on all sides */
			{
				double w, h;
				w      = (double)(abs(scrollspace.left) + abs(scrollspace.right)) / 2.0;
				h      = (double)(abs(scrollspace.top) + abs(scrollspace.bottom)) / 2.0;
				radius = sqrt((w * w) + (h * h));
			}
			if (radius == 0.0)
				radius = 1.0;

			/* Calculate the scale at which we should do the
			 * render, where 0.0 equals src and 1.0 equals dst */
			scale = distance / radius;

			/* Indicate scaling bounds to the user. */
			if (scale < 0.0)
				scale = -(sqrt(sqrt(sqrt(1.0 + -scale))) - 1.0);
			if (scale > max_scale)
				scale = sqrt(sqrt(sqrt(scale + (1.0 - max_scale)))) - (1.0 - max_scale);

			/* Update the animation rect area. */
			tnProperties.rcDestination = src;
			tnProperties.rcDestination.left += (LONG)(scale * scrollspace.left);
			tnProperties.rcDestination.top += (LONG)(scale * scrollspace.top);
			tnProperties.rcDestination.right += (LONG)(scale * scrollspace.right);
			tnProperties.rcDestination.bottom += (LONG)(scale * scrollspace.bottom);
			if (ga.ga_kind == WTG_GESTURE_KIND_ZOOM_RSTOR) {
				tnProperties.dwFlags |= DWM_TNP_OPACITY;
				tnProperties.opacity = 255;
				if (ga.ga_dist > 0) {
					if (scale <= 0.0) {
						tnProperties.opacity = 0;
					} else if (scale < 1.0) {
						tnProperties.opacity = (BYTE)(scale * 255);
					}
				} else {
					if (!(ga.ga_fgfeat & SYS_WINDOW_FEAT_MAXIMIZE))
						goto set_illegal_zoom_blur;
					WtgGestureAnimation_SetBlur(&ga, scale * 2.0);
				}
				WtgGestureAnimation_SetBlur(&ga, 0.0);
			} else if (ga.ga_anim.wa_animkind == WTG_WINDOW_KIND_MAXIMIZED) {
				WtgGestureAnimation_SetBlur(&ga, (1.0 - scale) * 2.0);
			} else if (ga.ga_anim.wa_animkind == WTG_WINDOW_KIND_NORMAL) {
				tnProperties.dwFlags |= DWM_TNP_OPACITY;
				tnProperties.opacity = 255;
				if (ga.ga_dist < 0) {
					if (scale >= 1.0)
						tnProperties.opacity = 0;
					else if (scale > 0.0) {
						tnProperties.opacity = (BYTE)((1.0 - scale) * 255);
					}
					if (!(ga.ga_fgfeat & SYS_WINDOW_FEAT_MINIMIZE))
						goto set_illegal_zoom_blur;
					WtgGestureAnimation_SetBlur(&ga, 0.0);
				} else {
					if (!(ga.ga_fgfeat & SYS_WINDOW_FEAT_MAXIMIZE))
						goto set_illegal_zoom_blur;
					WtgGestureAnimation_SetBlur(&ga, scale * 2.0);
				}
			} else {
set_illegal_zoom_blur:
				WtgGestureAnimation_SetBlur(&ga, fabs(scale) * 15.0);
			}
		}	break;

		case WTG_GESTURE_KIND_HSWIPE:
		case WTG_GESTURE_KIND_VSWIPE: {
			if (ga.ga_anim.wa_animkind == WTG_WINDOW_KIND_NORMAL ||
			    ga.ga_anim.wa_animkind == WTG_WINDOW_KIND_DESKTOP) {
				double hdelta = ga.ga_dist;
				double vdelta = ga.ga_dist2;
				if (ga.ga_kind == WTG_GESTURE_KIND_VSWIPE) {
					hdelta = ga.ga_dist2;
					vdelta = ga.ga_dist;
				}
				if (!(ga.ga_fgfeat & SYS_WINDOW_FEAT_MOVABLE)) {
					hdelta = Wtg_ExpLowerNumber(hdelta);
					vdelta = Wtg_ExpLowerNumber(vdelta);
				}
				tnProperties.rcDestination.left   += (LONG)hdelta;
				tnProperties.rcDestination.top    += (LONG)vdelta;
				tnProperties.rcDestination.right  += (LONG)hdelta;
				tnProperties.rcDestination.bottom += (LONG)vdelta;
				if (!(ga.ga_fgfeat & SYS_WINDOW_FEAT_MOVABLE)) {
					double distance;
					distance = sqrt((hdelta * hdelta) + (vdelta * vdelta));
					distance -= 8.0;
					if (distance < 0.0)
						distance = 0.0;
					WtgGestureAnimation_SetBlur(&ga, fabs(distance) / 10.0);
				}
			} else {
				LONG total, lo, hi;
				if (ga.ga_kind == WTG_GESTURE_KIND_HSWIPE) {
					double dist = ga.ga_dist;
					if (ga.ga_anim.wa_animkind == WTG_WINDOW_KIND_MAXIMIZED &&
					    ga.ga_kind == WTG_GESTURE_KIND_HSWIPE) {
						if (dist < 0.0 && ga.ga_prev.wa_animwin == NULL) {
							dist = -(sqrt(1.0 + -dist) - 1.0);
						} else if (dist > 0.0 && ga.ga_next.wa_animwin == NULL) {
							dist = (sqrt(1.0 + dist) - 1.0);
						}
					}
					tnProperties.rcDestination.left += (LONG)dist;
					tnProperties.rcDestination.right += (LONG)dist;
					total = RC_WIDTH(tnProperties.rcDestination);
					lo    = tnProperties.rcDestination.left;
					hi    = tnProperties.rcDestination.right;
				} else {
					tnProperties.rcDestination.top += (LONG)ga.ga_dist;
					tnProperties.rcDestination.bottom += (LONG)ga.ga_dist;
					total = RC_HEIGHT(tnProperties.rcDestination);
					lo    = tnProperties.rcDestination.top;
					hi    = tnProperties.rcDestination.bottom;
				}
				tnProperties.dwFlags |= DWM_TNP_OPACITY;
				tnProperties.opacity = (BYTE)(255 * Wtg_CalculateVisibilityOpacity(total, lo, hi));
				if (ga.ga_anim.wa_animkind == WTG_WINDOW_KIND_MAXIMIZED) {
					if (ga.ga_kind == WTG_GESTURE_KIND_HSWIPE) {
						DWM_THUMBNAIL_PROPERTIES pProp, nProp;
						double blur_strength;
						blur_strength = fabs(ga.ga_dist);
						if (blur_strength <= WTG_GESTURE_SWIPE_THRESHOLD)
							blur_strength = 0.0;
						else {
							blur_strength = (blur_strength - WTG_GESTURE_SWIPE_THRESHOLD);
						}
						WtgGestureAnimation_SetBlur(&ga, blur_strength / 100.0);
						pProp.dwFlags       = DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE;
						nProp.dwFlags       = DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE;
						pProp.rcDestination = tnProperties.rcDestination;
						nProp.rcDestination = tnProperties.rcDestination;
	
						pProp.rcDestination.left -= total / 2;
						pProp.rcDestination.right -= total / 2;
						nProp.rcDestination.left += total / 2;
						nProp.rcDestination.right += total / 2;
						pProp.fVisible = pProp.rcDestination.right > 0;
						nProp.fVisible = nProp.rcDestination.left < ga.ga_oversiz.x;
						tnProperties.opacity = (BYTE)(255 * Wtg_CalculateVisibilityOpacityEx(total, lo, hi, 2.0));
						if (pProp.fVisible) {
							pProp.dwFlags |= DWM_TNP_OPACITY;
							pProp.opacity = (BYTE)(255 * Wtg_CalculateVisibilityOpacityEx(total,
							                                                              pProp.rcDestination.left,
							                                                              pProp.rcDestination.right,
							                                                              2.0));
						}
						if (nProp.fVisible) {
							nProp.dwFlags |= DWM_TNP_OPACITY;
							nProp.opacity = (BYTE)(255 * Wtg_CalculateVisibilityOpacityEx(total,
							                                                              nProp.rcDestination.left,
							                                                              nProp.rcDestination.right,
							                                                              2.0));
						}
						{
							HRESULT hr;
							if (ga.ga_prev.wa_animwin != NULL) {
								hr = DwmUpdateThumbnailProperties(ga.ga_prev.wa_winthumb, &pProp);
								if (FAILED(hr))
									LOGERROR_PTR("DwmUpdateThumbnailProperties(ga_prev)", hr);
							}
							if (ga.ga_next.wa_animwin != NULL) {
								hr = DwmUpdateThumbnailProperties(ga.ga_next.wa_winthumb, &nProp);
								if (FAILED(hr))
									LOGERROR_PTR("DwmUpdateThumbnailProperties(ga_next)", hr);
							}
						}
					} else {
						double blur_strength;
						blur_strength = fabs(ga.ga_dist);
						if (blur_strength <= WTG_GESTURE_SWIPE_THRESHOLD)
							blur_strength = 0.0;
						else {
							blur_strength = (blur_strength - WTG_GESTURE_SWIPE_THRESHOLD);
						}
						WtgGestureAnimation_SetBlur(&ga, blur_strength / 100.0);
					}
				}
			}
		}	break;

		default:
			break;
		}
		tnProperties.fVisible = (tnProperties.rcDestination.left < tnProperties.rcDestination.right) &&
		                        (tnProperties.rcDestination.top < tnProperties.rcDestination.bottom);
		if (!tnProperties.fVisible) {
			memset(&tnProperties, 0, sizeof(tnProperties));
			tnProperties.dwFlags = DWM_TNP_VISIBLE;
		}
		{
			HRESULT hr;
			hr = DwmUpdateThumbnailProperties(ga.ga_anim.wa_winthumb,
			                                  &tnProperties);
			if (FAILED(hr))
				LOGERROR_PTR("DwmUpdateThumbnailProperties()", hr);
		}
	}
}

/* Finish the current gesture as the result of the # of touch-points
 * dropping down to `<= WTG_GESTURE_STOP_TOUCH_COUNT'
 * @return: true:  Gesture was successful.
 *                 In this case, this function will have
 *                 already called `WtgTouchInfo_CancelInputs()'
 * @return: false: Gesture failed.
 *                 In this case, no touch inputs will have
 *                 been canceled! */
static bool WtgGesture_Finish(void) {
	debug_printf("WtgGesture_Finish()\n");
	if (WTG_GESTURE_KIND_VALID(ga.ga_kind)) {
		/* Perform the action indicated by the current motion */
		switch (ga.ga_kind) {

		case WTG_GESTURE_KIND_VSWIPE_APP:
			if (WtgApplet_Done()) {
				WtgTouchInfo_CancelInputs();
				goto success;
			}
			goto failed;

		case WTG_GESTURE_KIND_ZOOM_RSTOR:
			if (ga.ga_dist >= WTG_GESTURE_ZOOM_COMMIT_THRESHOLD) {
				/* Restore the last minimized window. */
				WtgTouchInfo_CancelInputs();
				ShowWindow(ga.ga_anim.wa_animwin, SW_RESTORE);
				goto success;
			}
			break;

		case WTG_GESTURE_KIND_ZOOM:
			if (ga.ga_dist >= WTG_GESTURE_ZOOM_COMMIT_THRESHOLD) {
				/* Zoom in */
				if (ga.ga_fgkind == WTG_WINDOW_KIND_NORMAL) {
					if (ga.ga_fgfeat & SYS_WINDOW_FEAT_MAXIMIZE) {
						/* Zoom into a non-maximized window -> Maximize that window! */
						WtgTouchInfo_CancelInputs();
						ShowWindow(ga.ga_anim.wa_animwin, SW_MAXIMIZE);
						goto success;
					}
				} else {
					/* Zoom in on maximized window -> no-op */
				}
			} else if (ga.ga_dist <= -WTG_GESTURE_ZOOM_COMMIT_THRESHOLD) {
				/* Zoom out (pinch)  -> Minimize the current window */
				if (ga.ga_fgkind == WTG_WINDOW_KIND_NORMAL) {
					if (ga.ga_fgfeat & SYS_WINDOW_FEAT_MINIMIZE) {
						WtgTouchInfo_CancelInputs();
						ShowWindow(ga.ga_anim.wa_animwin, SW_MINIMIZE);
						last_minimized_window = ga.ga_anim.wa_animwin;
						goto success;
					}
				} else {
					if (ga.ga_fgfeat & SYS_WINDOW_FEAT_MAXIMIZE) {
						WtgTouchInfo_CancelInputs();
						ShowWindow(ga.ga_anim.wa_animwin, SW_RESTORE);
						goto success;
					}
				}
			}
			break;

		case WTG_GESTURE_KIND_HSWIPE:
		case WTG_GESTURE_KIND_VSWIPE:
			if (ga.ga_anim.wa_animkind == WTG_WINDOW_KIND_NORMAL) {
				if (ga.ga_fgfeat & SYS_WINDOW_FEAT_MOVABLE) {
					/* Move a window */
					RECT rc;
					if (GetWindowRect(ga.ga_anim.wa_animwin, &rc)) {
						POINT offset;
						WtgTouchInfo_CancelInputs();
						offset = WtgGesture_GetSwipeOffset(true);
						rc.left += offset.x;
						rc.top += offset.y;
						rc.right += offset.x;
						rc.bottom += offset.y;
						if (ga.ga_fgfeat & SYS_WINDOW_FEAT_RESIZABLE) {
							/* TODO: There should be drop-off boxes near the screen
							 *       borders, where letting go of a window inside of
							 *       those areas will result in the window to be sized
							 *       according to that box. (just like moving+placing
							 *       windows via their title-bars already does)
							 */
						}
						SetWindowPos(ga.ga_anim.wa_animwin,
						             HWND_TOP, rc.left, rc.top,
						             RC_WIDTH(rc),
						             RC_HEIGHT(rc),
						             SWP_SHOWWINDOW);
						goto success;
					}
				}
			} else {
				if (ga.ga_kind == WTG_GESTURE_KIND_HSWIPE) {
					if (ga.ga_anim.wa_animkind == WTG_WINDOW_KIND_MAXIMIZED) {
						HWND dest = NULL;
						if (ga.ga_dist >= WTG_GESTURE_SWIPE_COMMIT_THRESHOLD) {
							dest = ga.ga_prev.wa_animwin;
						} else if (ga.ga_dist <= -WTG_GESTURE_SWIPE_COMMIT_THRESHOLD) {
							dest = ga.ga_next.wa_animwin;
						}
						if (dest != NULL) {
							unsigned int dest_features;
							/* Switch to prev/next application. */
							WtgTouchInfo_CancelInputs();
							dest_features = Sys_GetWindowFeatures(dest);
							if (dest_features & SYS_WINDOW_FEAT_MAXIMIZE)
								ShowWindow(dest, SW_MAXIMIZE);
							if (dest != ga.ga_anim.wa_animwin && (ga.ga_fgfeat & SYS_WINDOW_FEAT_MINIMIZE))
								ShowWindow(ga.ga_anim.wa_animwin, SW_MINIMIZE);
							SetForegroundWindow(dest);
							SetActiveWindow(dest);
							goto success;
						} else {
							debug_printf("No swipe destination\n");
						}
					} else {
						/* Swipe left/right on desktop -> no-op */
					}
				} else {
#if 0 /* Shouldn't get here because of `WTG_GESTURE_KIND_VSWIPE_APP' */
					if (ga.ga_dist >= WTG_GESTURE_SWIPE_COMMIT_THRESHOLD) {
						/* Swipe (to the) bottom */
					} else if (ga.ga_dist <= -WTG_GESTURE_SWIPE_COMMIT_THRESHOLD) {
						/* Swipe (to the) top */
					}
#endif
				}
			}
			break;

		default:
			break;
		}
		if (ga.ga_kind == WTG_GESTURE_KIND_HSWIPE &&
		    ga.ga_anim.wa_animkind == WTG_WINDOW_KIND_MAXIMIZED) {
			if (ga.ga_prev.wa_animwin != NULL)
				WtgAnimatedWindow_Fini(&ga.ga_prev);
			if (ga.ga_next.wa_animwin != NULL)
				WtgAnimatedWindow_Fini(&ga.ga_next);
		}
		WtgAnimatedWindow_Fini(&ga.ga_anim);
		SetForegroundWindow(ga.ga_anim.wa_animwin);
		SetActiveWindow(ga.ga_anim.wa_animwin);
		ga.ga_kind = WTG_GESTURE_KIND_IGNORE;
	}
failed:
	WtgGestureAnimation_Fini(&ga);
	return false;
success:
	WtgGestureAnimation_Fini(&ga);
	return true;
}
/************************************************************************/











/************************************************************************/
/* Touch information processing and injection                           */
/************************************************************************/
#define WtgTouch_EraseInfo(i)                            \
	(--touch_count,                                      \
	 memmove(&touch_info[i], &touch_info[(i) + 1],       \
	         (touch_count - (i)) * sizeof(*touch_info)), \
	 memmove(&touch_meta[i], &touch_meta[(i) + 1],       \
	         (touch_count - (i)) * sizeof(*touch_meta)))

static size_t WtgGesture_FindInfoIndex(UINT32 pId) {
	size_t result;
	for (result = 0; result < gesture_count; ++result) {
		if (gesture_info[result].gt_hwid == pId)
			break;
	}
	return result;
}

static size_t WtgTouch_FindInfoIndex(UINT32 pId) {
	size_t result;
	for (result = 0; result < touch_count; ++result) {
		if (touch_meta[result].tm_pid == pId)
			break;
	}
	return result;
}

static bool Wtg_IsPointerIdInUse(UINT32 pId) {
	size_t i;
	for (i = 0; i < touch_count; ++i) {
		if (touch_info[i].pointerInfo.pointerId == pId)
			return true;
	}
	for (i = 0; i < touch_canceled_cnt; ++i) {
		if (touch_canceled_ids[i] == pId)
			return true;
	}
	return false;
}

static UINT32 Wtg_FindUnusedPointerId(void) {
	UINT32 result = 1;
	while (Wtg_IsPointerIdInUse(result))
		++result;
	return result;
}

static size_t Wtg_FindCancledTouchIdIndex(UINT32 pointerId) {
	size_t result;
	for (result = 0; result < touch_canceled_cnt; ++result) {
		if (touch_canceled_ids[result] == pointerId)
			break;
	}
	return result;
}

static LRESULT CALLBACK
WtgTouchGrabber_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {

	case WM_POINTERDOWN: {
create_new_touch_pointer:
		if (gesture_count != 0) {
			if (gesture_count >= WTG_MAX_TOUCH_INPUTS)
				return 0;
			/* Add another touch-point to the gesture */
			gesture_info[gesture_count].gt_hwid    = GET_POINTERID_WPARAM(wParam);
			gesture_info[gesture_count].gt_start.x = GET_X_LPARAM(lParam);
			gesture_info[gesture_count].gt_start.y = GET_Y_LPARAM(lParam);
			gesture_info[gesture_count].gt_now     = gesture_info[gesture_count].gt_start;
			++gesture_count;
		} else {
			if (touch_count >= WTG_MAX_TOUCH_INPUTS)
				return 0;
			if (touch_count + 1 >= WTG_GESTURE_STRT_TOUCH_COUNT && touch_canceled_cnt == 0) {
				/* Begin a gesture (can only be done when there are no more canceled touch-points) */
				size_t i;
				for (i = 0; i < touch_count; ++i) {
					gesture_info[i].gt_hwid  = touch_meta[i].tm_pid;
					gesture_info[i].gt_start = touch_info[i].pointerInfo.ptPixelLocation;
					gesture_info[i].gt_now   = gesture_info[i].gt_start;
				}
				gesture_count = touch_count + 1;
				gesture_info[touch_count].gt_hwid    = GET_POINTERID_WPARAM(wParam);
				gesture_info[touch_count].gt_start.x = GET_X_LPARAM(lParam);
				gesture_info[touch_count].gt_start.y = GET_Y_LPARAM(lParam);
				gesture_info[touch_count].gt_now     = gesture_info[touch_count].gt_start;
				/* Initialize gesture tracking globals. */
				WtgGesture_Start();
			} else {
				/* Normal input outside of a gesture */
				POINTER_TOUCH_INFO *info;
				touch_meta[touch_count].tm_pid = GET_POINTERID_WPARAM(wParam);
				info = &touch_info[touch_count];
				memset(info, 0, sizeof(*info));
				info->pointerInfo.pointerType = PT_TOUCH;
				info->pointerInfo.pointerId   = Wtg_FindUnusedPointerId();
				info->touchFlags              = TOUCH_FLAG_NONE;
				info->touchMask               = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
				info->orientation             = 90;
				info->pressure                = 32000;
		
				info->pointerInfo.ptPixelLocation.x = GET_X_LPARAM(lParam);
				info->pointerInfo.ptPixelLocation.y = GET_Y_LPARAM(lParam);
				info->rcContact.top                 = info->pointerInfo.ptPixelLocation.y - 2;
				info->rcContact.bottom              = info->pointerInfo.ptPixelLocation.y + 2;
				info->rcContact.left                = info->pointerInfo.ptPixelLocation.x - 2;
				info->rcContact.right               = info->pointerInfo.ptPixelLocation.x + 2;
				++touch_count;
				info->pointerInfo.pointerFlags = POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_DOWN;
				WtgTouchInfo_InjectEx(touch_count);
				info->pointerInfo.pointerFlags = POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_UPDATE;
			}
		}
		return 0;
	}	break;

	case WM_POINTERUPDATE: {
		size_t index;
		UINT32 id = GET_POINTERID_WPARAM(wParam);
		/* Check if this touch input had been canceled. */
		index = Wtg_FindCancledTouchIdIndex(id);
		if (index < touch_canceled_cnt) {
			/* Canceled touch input... */
			WtgTouchInfo_Inject(); /* Always (re-)inject for heart-beat */
		} else if (gesture_count != 0) {
			/* Inside of a gesture */
			index = WtgGesture_FindInfoIndex(id);
			if (index >= gesture_count)
				goto create_new_touch_pointer; /* Shouldn't happen... */
			gesture_info[index].gt_delta.x = GET_X_LPARAM(lParam) - gesture_info[index].gt_now.x;
			gesture_info[index].gt_delta.y = GET_Y_LPARAM(lParam) - gesture_info[index].gt_now.y;
			gesture_info[index].gt_now.x   = GET_X_LPARAM(lParam);
			gesture_info[index].gt_now.y   = GET_Y_LPARAM(lParam);
			WtgGesture_Update();
			gesture_info[index].gt_delta.x = 0;
			gesture_info[index].gt_delta.y = 0;
			WtgTouchInfo_Inject(); /* Always (re-)inject for heart-beat */
		} else {
			/* Normal touch movement outside of a gesture */
			POINTER_TOUCH_INFO *info;
			index = WtgTouch_FindInfoIndex(id);
			if (index >= touch_count)
				goto create_new_touch_pointer; /* Shouldn't happen... */
			info = &touch_info[index];

			info->pointerInfo.ptPixelLocation.x = GET_X_LPARAM(lParam);
			info->pointerInfo.ptPixelLocation.y = GET_Y_LPARAM(lParam);
			info->rcContact.top                 = info->pointerInfo.ptPixelLocation.y - 2;
			info->rcContact.bottom              = info->pointerInfo.ptPixelLocation.y + 2;
			info->rcContact.left                = info->pointerInfo.ptPixelLocation.x - 2;
			info->rcContact.right               = info->pointerInfo.ptPixelLocation.x + 2;
			WtgTouchInfo_InjectEx(index);
		}
		return 0;
	}	break;

	case WM_POINTERUP: {
		size_t index;
		UINT32 id = GET_POINTERID_WPARAM(wParam);
		/* Check if this touch input had been canceled. */
		index = Wtg_FindCancledTouchIdIndex(id);
		if (index < touch_canceled_cnt) {
			/* Remove the ID */
			--touch_canceled_cnt;
			memmove(&touch_canceled_ids[index],
			        &touch_canceled_ids[index + 1],
			        (touch_canceled_cnt - index) *
			        sizeof(touch_canceled_ids[index]));
			WtgTouchInfo_Inject(); /* Always (re-)inject for heart-beat */
		} else if (gesture_count != 0) {
			assert(touch_canceled_cnt == 0);
			index = WtgGesture_FindInfoIndex(id);
			if (index >= gesture_count)
				return 0; /* Shouldn't happen... */
			/* Remove 1 touch-point from the gesture. */
			--gesture_count;
			memmove(&gesture_info[index],
			        &gesture_info[index + 1],
			        (gesture_count - index) *
			        sizeof(gesture_info[index]));
			if (gesture_count <= WTG_GESTURE_STOP_TOUCH_COUNT) {
				/* Finish the current gesture. */
				if (WtgGesture_Finish()) {
					size_t i;
					/* Cancel all remaining gesture-specific inputs. */
					for (i = 0; i < gesture_count; ++i)
						touch_canceled_ids[i] = gesture_info[i].gt_hwid;
					touch_canceled_cnt = gesture_count;
					gesture_count      = 0;
				} else {
					size_t i;
					/* Gesture failed.
					 * -> Cancel touch-points that have gone away in the
					 *    mean time, and update touch-points that are still
					 *    there. */

					/* Step #1: Mark all touch points as canceled. */
					for (i = 0; i < touch_count; ++i) {
						touch_info[i].pointerInfo.pointerFlags = POINTER_FLAG_CANCELED |
						                                         POINTER_FLAG_UP;
					}

					/* Step #2: Update information for all touch points that are
					 *          still there (leaving those that have gone away
					 *          set to their canceled state). */
					touch_canceled_cnt = 0;
					for (i = 0; i < gesture_count; ++i) {
						size_t touch_index;
						UINT32 hwId = gesture_info[i].gt_hwid;
						touch_index = WtgTouch_FindInfoIndex(hwId);
						if (touch_index < touch_count) {
							POINTER_TOUCH_INFO *info;
							/* This one stayed! */
							info = &touch_info[touch_index];
							info->pointerInfo.pointerFlags = POINTER_FLAG_INRANGE |
							                                 POINTER_FLAG_INCONTACT |
							                                 POINTER_FLAG_UPDATE;
							info->pointerInfo.ptPixelLocation.x = gesture_info[i].gt_now.x;
							info->pointerInfo.ptPixelLocation.y = gesture_info[i].gt_now.y;
							info->rcContact.top    = info->pointerInfo.ptPixelLocation.y - 2;
							info->rcContact.bottom = info->pointerInfo.ptPixelLocation.y + 2;
							info->rcContact.left   = info->pointerInfo.ptPixelLocation.x - 2;
							info->rcContact.right  = info->pointerInfo.ptPixelLocation.x + 2;
						} else {
							/* Cancel this touch-point! */
							touch_canceled_ids[touch_canceled_cnt] = hwId;
							++touch_canceled_cnt;
						}
					}
					gesture_count = 0;
					WtgTouchInfo_Inject();
					/* Now remove all `POINTER_FLAG_CANCELED' touch points proper */
					i = 0;
					while (i < touch_count) {
						if (touch_info[i].pointerInfo.pointerFlags ==
						    (POINTER_FLAG_CANCELED | POINTER_FLAG_UP)) {
							/* Remove this touch-point. */
							WtgTouch_EraseInfo(i);
						} else {
							/* Keep this touch-point. */
							++i;
						}
					}
				}
			} else {
				/* Always (re-)inject for heart-beat */
				WtgTouchInfo_Inject();
			}
		} else {
			/* Normal case: Regular touch input outside of multi-touch gesture. */
			POINTER_TOUCH_INFO *info;
			index = WtgTouch_FindInfoIndex(GET_POINTERID_WPARAM(wParam));
			if (index >= touch_count)
				return 0; /* Shouldn't happen... */
			info = &touch_info[index];
			if (info->pointerInfo.ptPixelLocation.x != GET_X_LPARAM(lParam) ||
			    info->pointerInfo.ptPixelLocation.y != GET_Y_LPARAM(lParam)) {
				info->pointerInfo.ptPixelLocation.x = GET_X_LPARAM(lParam);
				info->pointerInfo.ptPixelLocation.y = GET_Y_LPARAM(lParam);
				info->rcContact.top                 = info->pointerInfo.ptPixelLocation.y - 2;
				info->rcContact.bottom              = info->pointerInfo.ptPixelLocation.y + 2;
				info->rcContact.left                = info->pointerInfo.ptPixelLocation.x - 2;
				info->rcContact.right               = info->pointerInfo.ptPixelLocation.x + 2;
				WtgTouchInfo_InjectEx(index);
				info->pointerInfo.pointerFlags = POINTER_FLAG_UP;
				WtgTouchInfo_Inject();
			} else {
				info->pointerInfo.pointerFlags = POINTER_FLAG_UP;
				WtgTouchInfo_InjectEx(index);
			}
			WtgTouch_EraseInfo(index);
		}
		return 0;
	}	break;

	default:
		break;
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

static HWND WtgTouchGrabber_CreateWindow(void) {
	HWND hResult = NULL;
	WNDCLASSEXW wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEXW));
	wc.cbSize        = sizeof(WNDCLASSEXW);
	wc.lpfnWndProc   = &WtgTouchGrabber_WndProc;
	wc.hInstance     = hApplicationInstance;
	wc.lpszClassName = L"TouchGrabberWindowClass";
	if (RegisterClassExW(&wc) == 0)
		LOGERROR_GLE("RegisterClassExW()");
	hResult = CreateWindowExW(0, wc.lpszClassName, NULL,
	                          0, 0, 0, 0, 0, HWND_MESSAGE,
	                          NULL, hApplicationInstance, NULL);
	if (hResult == NULL)
		LOGERROR_GLE("CreateWindowExW()");
	return hResult;
}

static LRESULT WINAPI
WtgShadow_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {

	case WM_NCHITTEST:
		return HTTRANSPARENT;

	default: break;
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}


static bool WtgShadow_RegisterWindowClass(void) {
	WNDCLASSEXW WindowClassEx;
	ZeroMemory(&WindowClassEx, sizeof(WNDCLASSEXW));
	WindowClassEx.cbSize        = sizeof(WNDCLASSEXW);
	WindowClassEx.lpfnWndProc   = &WtgShadow_WndProc;
	WindowClassEx.hInstance     = hApplicationInstance;
	WindowClassEx.lpszClassName = L"ShadowWindowClass";
	if (RegisterClassExW(&WindowClassEx) == 0) {
		LOGERROR_GLE("RegisterClassExW()");
		return false;
	}
	return true;
}

#ifndef MAX_TOUCH_COUNT
#define MAX_TOUCH_COUNT 256
#endif /* !MAX_TOUCH_COUNT */



/* Try everything in the book to make us as DPI-aware as possible, and stop
 * windows from screwing with our scaling (else window animations will look
 * all wrong) */
static void Wtg_MakeProcessDPIAware(void) {
	{
		typedef BOOL (WINAPI *LPSetProcessDpiAwarenessContext)(HANDLE value);
		LPSetProcessDpiAwarenessContext pdyn_SetProcessDpiAwarenessContext;
		DynApi_TryLoadFunction(pdyn_User32, SetProcessDpiAwarenessContext);
		if (pdyn_SetProcessDpiAwarenessContext) {
#undef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE
#undef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    ((HANDLE)-3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)
			if ((*pdyn_SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
				return;
			if ((*pdyn_SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
				return;
		}
	}
	{
		HMODULE hShcore = LoadLibraryW(L"Shcore.dll");
		if (hShcore != NULL) {
			typedef HRESULT (STDAPICALLTYPE *LPSetProcessDpiAwareness)(int value);
			LPSetProcessDpiAwareness pdyn_SetProcessDpiAwareness;
			DynApi_TryLoadFunction(hShcore, SetProcessDpiAwareness);
			if (pdyn_SetProcessDpiAwareness) {
#define PROCESS_SYSTEM_DPI_AWARE      1
#define PROCESS_PER_MONITOR_DPI_AWARE 2
				if (!FAILED((*pdyn_SetProcessDpiAwareness)(PROCESS_PER_MONITOR_DPI_AWARE)))
					return;
				if (!FAILED((*pdyn_SetProcessDpiAwareness)(PROCESS_SYSTEM_DPI_AWARE)))
					return;
			}
		}
	}
	{
		typedef BOOL (WINAPI *LPSetProcessDpiAware)(void);
		LPSetProcessDpiAware pdyn_SetProcessDpiAware;
		DynApi_TryLoadFunction(pdyn_User32, SetProcessDpiAware);
		if (pdyn_SetProcessDpiAware) {
			(*pdyn_SetProcessDpiAware)();
		}
	}
}


int main(int argc, char *argv[]) {
	HWND hTouchGrabberWindow;
	(void)argc;
	(void)argv;
	hApplicationInstance = GetModuleHandle(NULL);
	if (!DynApi_InitializeUser32())
		err(1, "DynApi_InitializeUser32() failed: %lu", (unsigned long)GetLastError());
	if (!DynApi_InitializeDwm())
		err(1, "DynApi_InitializeDwm() failed: %lu", (unsigned long)GetLastError());
	Wtg_MakeProcessDPIAware();
	if (!InitializeTouchInjection(MAX_TOUCH_COUNT, TOUCH_FEEDBACK_NONE))
		err(1, "InitializeTouchInjection() failed: %lu\n", (unsigned long)GetLastError());
	if (!WtgShadow_RegisterWindowClass())
		err(1, "Failed to register shadow window class\n");
	if ((hTouchGrabberWindow = WtgTouchGrabber_CreateWindow()) == NULL)
		err(1, "Failed to create touch grabber window\n");
//	if (!RegisterTouchWindow(myWindow, 0))
//		err(1, "RegisterTouchWindow() failed: %lu\n", GetLastError());
	if (!RegisterPointerInputTarget(hTouchGrabberWindow, PT_TOUCH))
		err(1, "RegisterPointerInputTarget() failed: %lu\n", (unsigned long)GetLastError());
	for (;;) {
		MSG msg;
		BOOL bRet;
		bRet = GetMessageW(&msg, NULL, 0, 0);
		if (bRet > 0) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		} else if (bRet < 0) {
			/* ... */
		} else {
			break;
		}
	}
	return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !GUARD_WINTOUCHG_MAIN_C */
