#pragma once

// Client crash callback
typedef BOOL (CALLBACK *LPGETLOGFILE) (LPVOID lpvState);
// Stack trace callback
typedef void (*TraceCallbackFunction)(DWORD address, const char *ImageName,
									  const char *FunctionName, DWORD functionDisp,
									  const char *Filename, DWORD LineNumber, DWORD lineDisp,
									  void *data);

typedef LPVOID (*InstallEx)(LPGETLOGFILE pfn, LPCSTR lpcszTo, LPCSTR lpcszSubject, BOOL bUseUI);
typedef void (*UninstallEx)(LPVOID lpState);
typedef void (*EnableUI)(void);
typedef void (*DisableUI)(void);
typedef void (*AddFileEx)(LPVOID lpState, LPCSTR lpFile, LPCSTR lpDesc);
typedef void (*AddRegistryEx)(LPVOID lpState, LPCSTR lpRegistry, LPCSTR lpDesc);
typedef void (*AddEventLogEx)(LPVOID lpState, LPCSTR lpEventLog, LPCSTR lpDesc);

/**
 * \ingroup CrashRpt
 * This class wraps the most important functions the CrashRpt-library
 * offers. To learn more about the CrashRpt-library go to
 * http://www.codeproject.com/debug/crash_report.asp \n
 * To compile the library you need the WTL. You can get the WTL
 * directly from microsoft: 
 * http://www.microsoft.com/downloads/details.aspx?FamilyID=128e26ee-2112-4cf7-b28e-7727d9a1f288&DisplayLang=en \n
 * \n
 * Many changes were made to the library so if you read the
 * article on CodeProject also read the changelog in the source
 * folder.\n
 * The most important changes are:
 * - stacktrace is included in the report, with symbols/linenumbers if available
 * - "save" button so the user can save the report instead of directly send it
 * - can be used by multiple applications
 * - zlib linked statically, so no need to ship the zlib.dll separately
 * \n
 * To use the library just include the header file "CrashReport.h"
 * \code
 * #include "CrashReport.h"
 * \endcode
 * Then you can either declare an instance of the class CCrashReport
 * somewhere globally in your application like this:
 * \code
 * CCrashReport g_crasher("report@mycompany.com", "Crashreport for MyApplication");
 * \endcode
 * that way you can't add registry keys or additional files to the report, but
 * it's the fastest and easiest way to use the library.
 * Another way is to declare a global variable and initialize it in e.g. InitInstance()
 * \code
 * CCrashReport g_crasher;
 * //then somewhere in InitInstance.
 * g_crasher.AddFile("mylogfile.log", "this is a log file");
 * g_crasher.AddRegistry("HKCU\\Software\\MyCompany\\MyProgram");
 * \endcode
 *
 *
 * \par requirements
 * WTL\n
 *
 * \remark the dll is dynamically linked at runtime. So the main application
 * will still work even if the dll is not shipped. 
 *
 */
class CCrashReport
{
public:
	/**
	 * Construct the CrashReport-Object. This loads the dll
	 * and initializes it.
	 * \param lpTo the mail address the crash report should be sent to
	 * \param lpSubject the mail subject
	 */
	CCrashReport(LPCSTR lpTo = NULL, LPCSTR lpSubject = NULL, BOOL bUseUI = TRUE)
	{
		InstallEx pfnInstallEx;
		m_hDll = LoadLibrary(_T("CrashRpt"));
		if (m_hDll)
		{
			pfnInstallEx = (InstallEx)GetProcAddress(m_hDll, "InstallEx");
			m_lpvState = pfnInstallEx(NULL, lpTo, lpSubject, bUseUI);
		}
	}
	~CCrashReport()
	{
		UninstallEx pfnUninstallEx;
		if ((m_hDll)&&(m_lpvState))
		{
			pfnUninstallEx = (UninstallEx)GetProcAddress(m_hDll, "UninstallEx");
			pfnUninstallEx(m_lpvState);
		}
		FreeLibrary(m_hDll);
	}
	/**
	 * Adds a file which will be included in the crash report. Use this
	 * if your application generates log-files or the like.
	 * \param lpFile the full path to the file
	 * \param lpDesc a description of the file, used in the crash report dialog
	 */
	void AddFile(LPCSTR lpFile, LPCSTR lpDesc)
	{
		AddFileEx pfnAddFileEx;
		if ((m_hDll)&&(m_lpvState))
		{
			pfnAddFileEx = (AddFileEx)GetProcAddress(m_hDll, "AddFileEx");
			(pfnAddFileEx)(m_lpvState, lpFile, lpDesc);
		}
	}
	/**
	 * Adds a whole registry tree to the crash report. 
	 * \param lpFile the full registry path, e.g. "HKLM\\Software\\MyApplication"
	 * \param lpDesc a description of the generated registry file, used in the crash report dialog
	 */
	void AddRegistry(LPCSTR lpFile, LPCSTR lpDesc)
	{
		AddRegistryEx pfnAddRegistryEx;
		if ((m_hDll)&&(m_lpvState))
		{
			pfnAddRegistryEx = (AddRegistryEx)GetProcAddress(m_hDll, "AddRegistryHiveEx");
			(pfnAddRegistryEx)(m_lpvState, lpFile, lpDesc);
		}
	}
	/**
	 * Adds a system Event Log to the crash report.
	 * \param lpFile 
	 * \param lpDesc 
	 */
	void AddEventLog(LPCSTR lpFile, LPCSTR lpDesc)
	{
		AddEventLogEx pfnAddEventLogEx;
		if ((m_hDll)&&(m_lpvState))
		{
			pfnAddEventLogEx = (AddEventLogEx)GetProcAddress(m_hDll, "AddEventLogEx");
			(pfnAddEventLogEx)(m_lpvState, lpFile, lpDesc);
		}
	}

	
private:
	HMODULE			m_hDll;
	LPVOID			m_lpvState;
};