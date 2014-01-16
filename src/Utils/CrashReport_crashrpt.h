// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2012-2014 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once
#include "../../ext/crashrpt/include/CrashRpt.h"
#include "../version.h"
#include <time.h>
#include <string>
#include <tchar.h>


typedef int WINAPI crInstallFN(PCR_INSTALL_INFO pInfo);
typedef int WINAPI crUninstallFN(void);
typedef int WINAPI crInstallToCurrentThreadFN(void);
typedef int WINAPI crInstallToCurrentThread2FN(DWORD dwFlags);
typedef int WINAPI crUninstallFromCurrentThreadFN(void);
typedef int WINAPI crAddFile2FN(LPCTSTR pszFile,LPCTSTR pszDestFile,LPCTSTR pszDesc,DWORD dwFlags);
typedef int WINAPI crAddScreenshotFN(DWORD dwFlags);
typedef int WINAPI crAddScreenshot2FN(DWORD dwFlags,int nJpegQuality);
typedef int WINAPI crAddPropertyFN(LPCTSTR pszPropName,LPCTSTR pszPropValue);
typedef int WINAPI crAddRegKeyFN(LPCTSTR pszRegKey,LPCTSTR pszDstFileName,DWORD dwFlags);
typedef int WINAPI crGenerateErrorReportFN(CR_EXCEPTION_INFO* pExceptionInfo);
typedef int WINAPI crEmulateCrashFN(unsigned ExceptionType);
typedef int WINAPI crGetLastErrorMsgFN(LPTSTR pszBuffer,UINT uBuffSize);

/**
 * \ingroup Utils
 * This class wraps the most important functions the CrashRpt-library
 * offers. To learn more about the CrashRpt-library go to
 * http://code.google.com/p/crashrpt/ \n
 * \n
 *
 * \remark the dll is dynamically linked at runtime. So the main application
 * will still work even if the dll is not shipped.
 *
 */
class CCrashReport
{
private:
    CCrashReport(void)
        : m_hDll(NULL)
        , pfnInstall(nullptr)
        , pfnUninstall(nullptr)
        , pfnInstallToCurrentThread(nullptr)
        , pfnInstallToCurrentThread2(nullptr)
        , pfnUninstallFromCurrentThread(nullptr)
        , pfnAddFile2(nullptr)
        , pfnAddScreenshot(nullptr)
        , pfnAddScreenshot2(nullptr)
        , pfnAddProperty(nullptr)
        , pfnAddregKey(nullptr)
        , pfnGenerateErrorReport(nullptr)
        , pfnEmulateCrash(nullptr)
        , pfnGetLastErrorMsg(nullptr)
    {
        char s_month[6];
        int month, day, year;
        struct tm t = {0};
        static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
        sscanf_s(__DATE__, "%s %d %d", s_month, _countof(s_month)-1, &day, &year);
        month = (int)((strstr(month_names, s_month)-month_names))/3;

        t.tm_mon = month;
        t.tm_mday = day;
        t.tm_year = year - 1900;
        t.tm_isdst = -1;
        __time64_t compiletime = _mktime64(&t);

        __time64_t now;
        _time64(&now);

        if ((now - compiletime)<(60*60*24*31*3))
        {
            // less than three months since this version was compiled
            TCHAR szFileName[_MAX_FNAME];
            GetModuleFileName(NULL, szFileName, _MAX_FNAME);

            // C:\Programme\TortoiseSVN\bin\TortoiseProc.exe -> C:\Programme\TortoiseSVN\bin\CrashRpt.dll
            std::wstring strFilename = szFileName;
            strFilename = strFilename.substr(0, strFilename.find_last_of('\\')+1);
            strFilename += L"CrashRpt1300.dll";

            m_hDll = LoadLibrary(strFilename.c_str());
            if (m_hDll)
            {
#ifdef UNICODE
                pfnInstall = (crInstallFN*)GetProcAddress(m_hDll, "crInstallW");
#else
                pfnInstall = (crInstallFN*)GetProcAddress(m_hDll, "crInstallA");
#endif
                pfnUninstall = (crUninstallFN*)GetProcAddress(m_hDll, "crUninstall");
                pfnInstallToCurrentThread = (crInstallToCurrentThreadFN*)GetProcAddress(m_hDll, "crInstallToCurrentThread");
                pfnInstallToCurrentThread2 = (crInstallToCurrentThread2FN*)GetProcAddress(m_hDll, "crInstallToCurrentThread2");
                pfnUninstallFromCurrentThread = (crUninstallFromCurrentThreadFN*)GetProcAddress(m_hDll, "crUninstallFromCurrentThread");
#ifdef UNICODE
                pfnAddFile2 = (crAddFile2FN*)GetProcAddress(m_hDll, "crAddFile2W");
#else
                pfnAddFile2 = (crAddFile2FN*)GetProcAddress(m_hDll, "crAddFile2A");
#endif
                pfnAddScreenshot = (crAddScreenshotFN*)GetProcAddress(m_hDll, "crAddScreenshot");
                pfnAddScreenshot2 = (crAddScreenshot2FN*)GetProcAddress(m_hDll, "crAddScreenshot2");
#ifdef UNICODE
                pfnAddProperty = (crAddPropertyFN*)GetProcAddress(m_hDll, "crAddPropertyW");
                pfnAddregKey = (crAddRegKeyFN*)GetProcAddress(m_hDll, "crAddRegKeyW");
#else
                pfnAddProperty = (crAddPropertyFN*)GetProcAddress(m_hDll, "crAddPropertyA");
                pfnAddregKey = (crAddRegKeyFN*)GetProcAddress(m_hDll, "crAddRegKeyA");
#endif
                pfnGenerateErrorReport = (crGenerateErrorReportFN*)GetProcAddress(m_hDll, "crGenerateErrorReport");
                pfnEmulateCrash = (crEmulateCrashFN*)GetProcAddress(m_hDll, "crEmulateCrash");
#ifdef UNICODE
                pfnGetLastErrorMsg = (crGetLastErrorMsgFN*)GetProcAddress(m_hDll, "crGetLastErrorMsgW");
#else
                pfnGetLastErrorMsg = (crGetLastErrorMsgFN*)GetProcAddress(m_hDll, "crGetLastErrorMsgA");
#endif
            }
        }
    }

    ~CCrashReport(void)
    {
        if (m_hDll)
            FreeLibrary(m_hDll);
    }
public:
    static CCrashReport&    Instance()
    {
        static CCrashReport instance;
        return instance;
    }

    int                     Install(PCR_INSTALL_INFO pInfo) { if (m_hDll && pfnInstall) return pfnInstall(pInfo); return FALSE; }
    int                     Uninstall(void) { if (m_hDll && pfnUninstall) return pfnUninstall(); return FALSE; }
    int                     InstallToCurrentThread(void) { if (m_hDll && pfnInstallToCurrentThread) return pfnInstallToCurrentThread(); return FALSE; }
    int                     InstallToCurrentThread2(DWORD dwFlags) { if (m_hDll && pfnInstallToCurrentThread2) return pfnInstallToCurrentThread2(dwFlags); return FALSE; }
    int                     UninstallFromCurrentThread(void) { if (m_hDll && pfnUninstallFromCurrentThread) return pfnUninstallFromCurrentThread(); return FALSE; }
    int                     AddFile2(LPCTSTR pszFile,LPCTSTR pszDestFile,LPCTSTR pszDesc,DWORD dwFlags) { if (m_hDll && pfnAddFile2) return pfnAddFile2(pszFile, pszDestFile, pszDesc, dwFlags); return FALSE; }
    int                     AddScreenshot(DWORD dwFlags) { if (m_hDll && pfnAddScreenshot) return pfnAddScreenshot(dwFlags); return FALSE; }
    int                     AddScreenshot2(DWORD dwFlags,int nJpegQuality) { if (m_hDll && pfnAddScreenshot2) return pfnAddScreenshot2(dwFlags, nJpegQuality); return FALSE; }
    int                     AddProperty(LPCTSTR pszPropName,LPCTSTR pszPropValue) { if (m_hDll && pfnAddProperty) return pfnAddProperty(pszPropName, pszPropValue); return FALSE; }
    int                     AddRegKey(LPCTSTR pszRegKey,LPCTSTR pszDstFileName,DWORD dwFlags) { if (m_hDll && pfnAddregKey) return pfnAddregKey(pszRegKey, pszDstFileName, dwFlags); return FALSE; }
    int                     GenerateErrorReport(CR_EXCEPTION_INFO* pExceptionInfo) { if (m_hDll && pfnGenerateErrorReport) return pfnGenerateErrorReport(pExceptionInfo); return FALSE; }
    int                     EmulateCrash(unsigned ExceptionType) { if (m_hDll && pfnEmulateCrash) return pfnEmulateCrash(ExceptionType); return FALSE; }
    int                     GetLastErrorMsg(LPTSTR pszBuffer,UINT uBuffSize) { if (m_hDll && pfnGetLastErrorMsg) return pfnGetLastErrorMsg(pszBuffer, uBuffSize); return FALSE; }

private:
    HMODULE                         m_hDll;

    crInstallFN *                   pfnInstall;
    crUninstallFN *                 pfnUninstall;
    crInstallToCurrentThreadFN *    pfnInstallToCurrentThread;
    crInstallToCurrentThread2FN *   pfnInstallToCurrentThread2;
    crUninstallFromCurrentThreadFN *pfnUninstallFromCurrentThread;
    crAddFile2FN *                  pfnAddFile2;
    crAddScreenshotFN *             pfnAddScreenshot;
    crAddScreenshot2FN *            pfnAddScreenshot2;
    crAddPropertyFN *               pfnAddProperty;
    crAddRegKeyFN *                 pfnAddregKey;
    crGenerateErrorReportFN *       pfnGenerateErrorReport;
    crEmulateCrashFN *              pfnEmulateCrash;
    crGetLastErrorMsgFN *           pfnGetLastErrorMsg;
};


class CCrashReportTSVN
{
public:

    //! Installs exception handlers to the caller process
    CCrashReportTSVN(LPCTSTR appname)
    {
        CR_INSTALL_INFO info;
        SecureZeroMemory(&info, sizeof(CR_INSTALL_INFO));
        info.cb = sizeof(CR_INSTALL_INFO);
        info.pszAppName = appname;
        info.pszAppVersion = _T(STRPRODUCTVER);
        TCHAR subject[MAX_PATH] = {0};
        _stprintf_s(subject, L"Crash report for %s, Version %d.%d.%d.%d", appname, TSVN_VERMAJOR, TSVN_VERMINOR, TSVN_VERMICRO, TSVN_VERBUILD);
        info.pszEmailSubject = subject;
        info.pszEmailTo = L"tortoisesvn@gmail.com";
        info.pszUrl = L"http://tortoisesvn.net/scripts/crashrpt.php";
        info.uPriorities[CR_HTTP] = 1;  // First (and only) try send report over HTTP
        info.uPriorities[CR_SMTP] = CR_NEGATIVE_PRIORITY;  // don't send report over SMTP
        info.uPriorities[CR_SMAPI] = CR_NEGATIVE_PRIORITY; // don't send report over Simple MAPI
        // Install all available exception handlers, use HTTP binary transfer encoding (recommended).
        info.dwFlags |= CR_INST_ALL_POSSIBLE_HANDLERS;
        info.dwFlags |= CR_INST_HTTP_BINARY_ENCODING;
        info.dwFlags |= CR_INST_APP_RESTART;
        info.dwFlags |= CR_INST_SEND_QUEUED_REPORTS;
        // Define the Privacy Policy URL
        info.pszPrivacyPolicyURL = L"http://tortoisesvn.net/crashreport_privacy.html";

        TCHAR module[MAX_PATH] = {0};
        GetModuleFileName(NULL, module, MAX_PATH);
        info.pszCustomSenderIcon = module;

        m_nInstallStatus = CCrashReport::Instance().Install(&info);
        CCrashReport::Instance().AddRegKey(L"HKEY_CURRENT_USER\\Software\\TortoiseSVN", L"regkey1.xml", 0);
        CCrashReport::Instance().AddRegKey(L"HKEY_CURRENT_USER\\Software\\TortoiseMerge", L"regkey2.xml", 0);
    }


    //! Deinstalls exception handlers from the caller process
    ~CCrashReportTSVN()
    {
        CCrashReport::Instance().Uninstall();
    }

    //! Install status
    int m_nInstallStatus;
};

class CCrashReportThread
{
public:

    /// Installs exception handlers to the caller thread
    CCrashReportThread(DWORD dwFlags=0)
    {
        m_nInstallStatus = CCrashReport::Instance().InstallToCurrentThread2(dwFlags);
    }

    /// Deinstalls exception handlers from the caller thread
    ~CCrashReportThread()
    {
        CCrashReport::Instance().UninstallFromCurrentThread();
    }

    /// Install status
    int m_nInstallStatus;
};

