/*!*********************************************************************************************************************
\file         PVRCore\NativeLibrary.cpp
\author       PowerVR by Imagination, Developer Technology Team
\copyright    Copyright (c) Imagination Technologies Limited.
\brief         Implementations of methods of the NativeLibrary class.
***********************************************************************************************************************/
//!\cond NO_DOXYGEN
#include "PVRCore/NativeLibrary.h"
#include "PVRCore/Log.h"


#if defined( _WIN32 )
#include <windows.h>
typedef HINSTANCE LIBTYPE;

namespace pvr {
namespace native {
LIBTYPE OpenLibrary(const char* pszPath)
{
#if defined(_UNICODE) // UNDER_CE
	if (!pszPath)
	{ return NULL; }

	// Get full path of executable
	wchar_t pszPathW[_MAX_PATH];

	// Convert char to wchar
	DWORD i = 0;

	for (i = 0; i <= strlen(pszPath); ++i)
	{
		pszPathW[i] = static_cast<wchar_t>(pszPath[i]);
	}

	pszPathW[i] = '\0';
	return LoadLibraryW(pszPathW);
#else
	return LoadLibraryA(pszPath);
#endif
}

void CloseLibrary(LIBTYPE hLib)
{
	FreeLibrary(hLib);
}

void* GetLibFunction(LIBTYPE hLib, const char* pszName)
{
	if (hLib)
	{
#if defined(UNDER_CE)
		return GetProcAddressA(hLib, pszName);
#else
		return GetProcAddress(hLib, pszName);
#endif
	}
	return NULL;
}
}
}

#elif defined( __linux__ ) || defined(__QNXNTO__) || defined(__APPLE__)

#include <unistd.h>
#include <dlfcn.h>
typedef void* LIBTYPE;

#if defined(__APPLE__)
void* OpenFramework(const char* pszPath);

namespace pvr {
namespace native {
LIBTYPE OpenLibrary(const char* pszPath)
{
	return OpenFramework(pszPath); // An objective-C function that uses dlopen
}
}
}
#else
typedef void* LIBTYPE;

namespace pvr {
namespace native {
LIBTYPE OpenLibrary(const char* pszPath)
{
	LIBTYPE lt = dlopen(pszPath, RTLD_LAZY | RTLD_GLOBAL);

	if (!lt)
	{
		const char* err = dlerror();

		if (err)
		{
			Log(Log.Error, "dlopen failed with error: %s => %s", err, pszPath);
		}

		char pathMod[256];
		strcpy(pathMod, "./");
		strcat(pathMod, pszPath);

		lt = dlopen(pathMod, RTLD_LAZY | RTLD_GLOBAL);
		if (!lt)
		{
			const char* err = dlerror();

			if (err)
			{
				Log(Log.Error, "dlopen failed with error: %s => %s", err, pathMod);
			}
		}
		else
		{
			Log(Log.Information, "dlopen loaded (MOD PATH) %s", pathMod);
		}
	}

	return lt;
}
}
}
#endif

namespace pvr {
namespace native {
void CloseLibrary(LIBTYPE hLib) { dlclose(hLib); }

void* GetLibFunction(LIBTYPE hLib, const char* pszName)
{
	if (hLib)
	{
		void* fnct = dlsym(hLib, pszName);

		return fnct;
	}
	return NULL;
}
}
}
#elif defined (ANDROID)

#include <dlfcn.h>
namespace pvr {
namespace native {

LIBTYPE OpenLibrary(const char* pszPath)
{
	LIBTYPE lt = dlopen(pszPath, RTLD_LAZY | RTLD_GLOBAL);

	if (!lt)
	{
		const char* err = dlerror();

		if (err)
		{
			Log(Log.Error, "dlopen failed with error ");
			Log(Log.Error, err);
		}
	}

	return lt;
}

void CloseLibrary(LIBTYPE hLib) { dlclose(hLib); }

void* GetLibFunction(LIBTYPE hLib, const char* pszName)
{
	void* fnct = dlsym(hLib, pszName);
	return fnct;
}
}
}
#else
#error Unsupported platform
#endif

namespace pvr {
namespace native {

NativeLibrary::NativeLibrary(const std::string& LibPath) : m_disableErrorPrint(false), m_hHostLib(0), m_bError(false)
{
	size_t start = 0;
	std::string tmp;

	while (!m_hHostLib)
	{
		size_t end = LibPath.find_first_of(';', start);

		if (end == std::string::npos)
		{
			tmp = LibPath.substr(start, LibPath.length() - start);
		}
		else
		{
			tmp = LibPath.substr(start, end - start);
		}

		if (!tmp.empty())
		{
			m_hHostLib = OpenLibrary(tmp.c_str());

			if (!m_hHostLib)
			{
				// Remove the last character in case a new line character snuck in
				tmp = tmp.substr(0, tmp.size() - 1);
				m_hHostLib = OpenLibrary(tmp.c_str());
			}
		}

		if (end == std::string::npos)
		{
			break;
		}

		start = end + 1;
	}

	if (!m_hHostLib)
	{
		Log(Log.Critical, "Could not load host library '%s'", LibPath.c_str());

		this->m_bError = true;

	}
	Log(Log.Debug, "Host library '%s' loaded", LibPath.c_str());
}

NativeLibrary::~NativeLibrary()
{
	CloseLib();
}

void* NativeLibrary::getFunction(const char* pszName)
{

	void* pFn = GetLibFunction((LIBTYPE)m_hHostLib, pszName);

	if (pFn == NULL && m_disableErrorPrint == false)
	{
		m_bError |= (pFn == NULL);
		Log("Could not get function %s", pszName);
	}

	return pFn;
}

void NativeLibrary::CloseLib()
{
	if (m_hHostLib)
	{
		CloseLibrary((LIBTYPE)m_hHostLib);
		m_hHostLib = 0;
	}
}

bool NativeLibrary::LoadFailed()
{
	return m_bError;
}
}
}
//!\endcond
