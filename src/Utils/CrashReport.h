// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2012-2014, 2021 - TortoiseSVN

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
#include "../../ext/CrashServer/CrashHandler/CrashHandler/CrashHandler.h"
#include "../version.h"

// dummy define, needed only when we use crashrpt instead of this.
#define CR_AF_MAKE_FILE_COPY 0

__forceinline HMODULE getMyModuleHandle()
{
    static int               sModuleMarker = 0;
    MEMORY_BASIC_INFORMATION memoryBasicInformation;
    if (!VirtualQuery(&sModuleMarker, &memoryBasicInformation, sizeof(memoryBasicInformation)))
    {
        return nullptr;
    }
    return static_cast<HMODULE>(memoryBasicInformation.AllocationBase);
}

/**
 * \ingroup Utils
 * helper class for the DoctorDumpSDK
 */
class CCrashReport
{
private:
    CCrashReport()
    {
        LoadDll();
    }

    ~CCrashReport()
    {
        if (!m_isReadyToExit)
            return;

        // If crash has happen not in main thread we should wait here until report will be sent
        // or else program will be terminated after return from main() and report sending will be halted.
        while (!m_isReadyToExit())
            ::Sleep(100);

        if (m_bSkipAssertsAdded)
            RemoveVectoredExceptionHandler(SkipAsserts);
    }

public:
    static CCrashReport& Instance()
    {
        static CCrashReport instance;
        return instance;
    }

    static int Uninstall() { return FALSE; }
    int        AddFile2(LPCWSTR pszFile, LPCWSTR pszDestFile, LPCWSTR /*pszDesc*/, DWORD /*dwFlags*/)
    {
        return AddFileToReport(pszFile, pszDestFile) ? 1 : 0;
    }

    //! Checks that crash handling was enabled.
    //! \return Return \b true if crash handling was enabled.
    bool IsCrashHandlingEnabled() const
    {
        return m_bWorking;
    }

    //! Initializes crash handler.
    //! \note You may call this function multiple times if some data has changed.
    //! \return Return \b true if crash handling was enabled.
    bool InitCrashHandler(
        ApplicationInfo* applicationInfo,  //!< [in] Pointer to the ApplicationInfo structure that identifies your application.
        HandlerSettings* handlerSettings,  //!< [in] Pointer to the HandlerSettings structure that customizes crash handling behavior. This parameter can be \b NULL.
        BOOL             ownProcess = TRUE //!< [in] If you own the process your code running in set this option to \b TRUE. If don't (for example you write
                                           //!<      a plugin to some external application) set this option to \b FALSE. In that case you need to explicitly
                                           //!<      catch exceptions. See \ref SendReport for more information.
        ) throw()
    {
        if (!m_initCrashHandler)
            return false;

        m_bWorking = m_initCrashHandler(applicationInfo, handlerSettings, ownProcess) != FALSE;

        return m_bWorking;
    }

    //! \note This function is experimental and may not be available and may not be support by Doctor Dump in the future.
    //! You may set custom information for your possible report.
    //! This text will be available on Doctor Dump dumps page.
    //! The text should not contain private information.
    //! \return If the function succeeds, the return value is \b true.
    bool SetCustomInfo(
        LPCWSTR text //!< [in] custom info for the report. The text will be cut to 100 characters.
    ) const
    {
        if (!m_setCustomInfo)
            return false;
        m_setCustomInfo(text);
        return true;
    }

    //! You may add any key/value pair to crash report.
    //! \return If the function succeeds, the return value is \b true.
    //! \note This function is thread safe.
    bool AddUserInfoToReport(
        LPCWSTR key,  //!< [in] key string that will be added to the report.
        LPCWSTR value //!< [in] value for the key.
    ) const throw()
    {
        if (!m_addUserInfoToReport)
            return false;
        m_addUserInfoToReport(key, value);
        return true;
    }

    //! You may remove any key that was added previously to crash report by \a AddUserInfoToReport.
    //! \return If the function succeeds, the return value is \b true.
    //! \note This function is thread safe.
    bool RemoveUserInfoFromReport(
        LPCWSTR key //!< [in] key string that will be removed from the report.
    ) const
    {
        if (!m_removeUserInfoFromReport)
            return false;
        m_removeUserInfoFromReport(key);
        return true;
    }

    //! You may add any file to crash report. This file will be read when crash appears and will be sent within the report.
    //! Multiple files may be added. Filename of the file in the report may be changed to any name.
    //! \return If the function succeeds, the return value is \b true.
    //! \note This function is thread safe.
    bool AddFileToReport(
        LPCWSTR path,                       //!< [in] Path to the file, that will be added to the report.
        LPCWSTR reportFileName /* = NULL */ //!< [in] Filename that will be used in report for this file. If parameter is \b NULL, original name from path will be used.
    ) const throw()
    {
        if (!m_addFileToReport)
            return false;
        m_addFileToReport(path, reportFileName);
        return true;
    }

    //! Remove from report the file that was registered earlier to be sent within report.
    //! \return If the function succeeds, the return value is \b true.
    //! \note This function is thread safe.
    bool RemoveFileFromReport(
        LPCWSTR path //!< [in] Path to the file, that will be removed from the report.
    ) const throw()
    {
        if (!m_removeFileFromReport)
            return false;
        m_removeFileFromReport(path);
        return true;
    }

    //! Fills version field (V) of ApplicationInfo with product version
    //! found in the executable file of the current process.
    //! \return If the function succeeds, the return value is \b true.
    bool GetVersionFromApp(
        ApplicationInfo* appInfo //!< [out] Pointer to ApplicationInfo structure. Its version field (V) will be set to product version.
    ) const throw()
    {
        if (!m_getVersionFromApp)
            return false;
        return m_getVersionFromApp(appInfo) != FALSE;
    }

    //! Fill version field (V) of ApplicationInfo with product version found in the file specified.
    //! \return If the function succeeds, the return value is \b true.
    bool GetVersionFromFile(
        LPCWSTR          path,   //!< [in] Path to the file product version will be extracted from.
        ApplicationInfo* appInfo //!< [out] Pointer to ApplicationInfo structure. Its version field (V) will be set to product version.
    ) const throw()
    {
        if (!m_getVersionFromFile)
            return false;
        return m_getVersionFromFile(path, appInfo) != FALSE;
    }

    //! If you do not own the process your code running in (for example you write a plugin to some
    //! external application) you need to properly initialize CrashHandler using \b ownProcess option.
    //! Also you need to explicitly catch all exceptions in all entry points to your code and in all
    //! threads you create. To do so use this construction:
    //! \code
    //! bool SomeEntryPoint(PARAM p)
    //! {
    //!     __try
    //!     {
    //!         return YouCode(p);
    //!     }
    //!     __except (CrashHandler::SendReport(GetExceptionInformation()))
    //!     {
    //!         ::ExitProcess(0); // It is better to stop the process here or else corrupted data may incomprehensibly crash it later.
    //!         return false;
    //!     }
    //! }
    //! \endcode
    LONG SendReport(
        EXCEPTION_POINTERS* exceptionPointers //!< [in] Pointer to EXCEPTION_POINTERS structure. You should get it using GetExceptionInformation()
                                              //!<      function inside __except keyword.
    ) const
    {
        if (!m_sendReport)
            return EXCEPTION_CONTINUE_SEARCH;
        // There is no crash handler but asserts should continue anyway
        if (exceptionPointers->ExceptionRecord->ExceptionCode == EXCEPTION_ASSERTION_VIOLATED)
            return EXCEPTION_CONTINUE_EXECUTION;
        return m_sendReport(exceptionPointers);
    }

    //! To send a report about violated assertion you can throw exception with this exception code
    //! using: \code RaiseException(CrashHandler::ExceptionAssertionViolated, 0, 0, NULL); \endcode
    //! Execution will continue after report will be sent (EXCEPTION_CONTINUE_EXECUTION would be used).
    //! You may pass grouping string as first parameter (see \a SkipDoctorDump_SendAssertionViolated).
    //! \note If you called CrashHandler constructor and crshhndl.dll was missing you still may using this exception.
    //!       It will be caught, ignored and execution will continue. \ref SendReport function also works safely
    //!       when crshhndl.dll was missing.
    static const DWORD EXCEPTION_ASSERTION_VIOLATED = static_cast<DWORD>(0xCCE17000);

    //! Sends assertion violation report from this point and continue execution.
    //! \sa ExceptionAssertionViolated
    //! \note Functions containing "SkipDoctorDump" will be ignored in stack parsing.
    void SkipDoctorDump_SendAssertionViolated(
        LPCSTR dumpGroup = nullptr //!< [in] All dumps with that group will be separated from dumps with same stack but another group. Set parameter to \b NULL if no grouping is required.
    ) const
    {
        if (!m_bWorking)
            return;
        if (dumpGroup)
            ::RaiseException(CrashHandler::ExceptionAssertionViolated, 0, 1, reinterpret_cast<ULONG_PTR*>(&dumpGroup));
        else
            ::RaiseException(CrashHandler::ExceptionAssertionViolated, 0, 0, nullptr);
    }

private:
    bool LoadDll(LPCWSTR crashHandlerPath = nullptr) throw()
    {
        m_bLoaded                  = false;
        m_bWorking                 = false;
        m_bSkipAssertsAdded        = false;
        m_initCrashHandler         = nullptr;
        m_sendReport               = nullptr;
        m_isReadyToExit            = nullptr;
        m_setCustomInfo            = nullptr;
        m_addUserInfoToReport      = nullptr;
        m_removeUserInfoFromReport = nullptr;
        m_addFileToReport          = nullptr;
        m_removeFileFromReport     = nullptr;
        m_getVersionFromApp        = nullptr;
        m_getVersionFromFile       = nullptr;

        // hCrashHandlerDll should not be unloaded, crash may appear even after return from main().
        // So hCrashHandlerDll is not saved after construction.
        BOOL bIsWow = FALSE;
        IsWow64Process(GetCurrentProcess(), &bIsWow);
        HMODULE hCrashHandlerDll = nullptr;
        if (bIsWow == FALSE)
        {
            if (crashHandlerPath == nullptr)
            {
                HMODULE hDll          = getMyModuleHandle();
                wchar_t modPath[1024] = {0};
                if (GetModuleFileName(hDll, modPath, _countof(modPath)))
                {
                    wchar_t* dirPoint = wcsrchr(modPath, '\\');
                    if (dirPoint)
                    {
                        *dirPoint = 0;
                        wcscat_s(modPath, L"\\crshhndl.dll");
                        hCrashHandlerDll = ::LoadLibraryW(modPath);
                    }
                    else
                        hCrashHandlerDll = ::LoadLibraryW(L"crshhndl.dll");
                }
                else
                    hCrashHandlerDll = ::LoadLibraryW(L"crshhndl.dll");
            }
            else
                hCrashHandlerDll = ::LoadLibraryW(crashHandlerPath);
        }
        if (hCrashHandlerDll != nullptr)
        {
            m_initCrashHandler         = reinterpret_cast<PfnInitCrashHandler>(GetProcAddress(hCrashHandlerDll, "InitCrashHandler"));
            m_sendReport               = reinterpret_cast<PfnSendReport>(GetProcAddress(hCrashHandlerDll, "SendReport"));
            m_isReadyToExit            = reinterpret_cast<PfnIsReadyToExit>(GetProcAddress(hCrashHandlerDll, "IsReadyToExit"));
            m_setCustomInfo            = reinterpret_cast<PfnSetCustomInfo>(GetProcAddress(hCrashHandlerDll, "SetCustomInfo"));
            m_addUserInfoToReport      = reinterpret_cast<PfnAddUserInfoToReport>(GetProcAddress(hCrashHandlerDll, "AddUserInfoToReport"));
            m_removeUserInfoFromReport = reinterpret_cast<PfnRemoveUserInfoFromReport>(GetProcAddress(hCrashHandlerDll, "RemoveUserInfoFromReport"));
            m_addFileToReport          = reinterpret_cast<PfnAddFileToReport>(GetProcAddress(hCrashHandlerDll, "AddFileToReport"));
            m_removeFileFromReport     = reinterpret_cast<PfnRemoveFileFromReport>(GetProcAddress(hCrashHandlerDll, "RemoveFileFromReport"));
            m_getVersionFromApp        = reinterpret_cast<PfnGetVersionFromApp>(GetProcAddress(hCrashHandlerDll, "GetVersionFromApp"));
            m_getVersionFromFile       = reinterpret_cast<PfnGetVersionFromFile>(GetProcAddress(hCrashHandlerDll, "GetVersionFromFile"));

            m_bLoaded = m_initCrashHandler && m_sendReport && m_isReadyToExit && m_setCustomInfo && m_addUserInfoToReport && m_removeUserInfoFromReport && m_addFileToReport && m_removeFileFromReport && m_getVersionFromApp && m_getVersionFromFile;
        }

#if 0 // TSVN don't use it
#    if _WIN32_WINNT >= 0x0501 /*_WIN32_WINNT_WINXP*/
        // if no crash processing was started, we need to ignore ExceptionAssertionViolated exceptions.
        if (!m_bLoaded)
        {
            ::AddVectoredExceptionHandler(TRUE, SkipAsserts);
            m_bSkipAssertsAdded = true;
        }
#    endif
#endif

        return m_bLoaded;
    }

    static LONG CALLBACK SkipAsserts(EXCEPTION_POINTERS* pExceptionInfo)
    {
        if (pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ASSERTION_VIOLATED)
            return EXCEPTION_CONTINUE_EXECUTION;
        return EXCEPTION_CONTINUE_SEARCH;
    }

    bool m_bLoaded;
    bool m_bWorking;
    bool m_bSkipAssertsAdded;

    using PfnInitCrashHandler         = BOOL (*)(ApplicationInfo* applicationInfo, HandlerSettings* handlerSettings, BOOL ownProcess);
    using PfnSendReport               = LONG (*)(EXCEPTION_POINTERS* exceptionPointers);
    using PfnIsReadyToExit            = BOOL (*)();
    using PfnSetCustomInfo            = void (*)(LPCWSTR text);
    using PfnAddUserInfoToReport      = void (*)(LPCWSTR key, LPCWSTR value);
    using PfnRemoveUserInfoFromReport = void (*)(LPCWSTR key);
    using PfnAddFileToReport          = void (*)(LPCWSTR path, LPCWSTR reportFileName /* = NULL */);
    using PfnRemoveFileFromReport     = void (*)(LPCWSTR path);
    using PfnGetVersionFromApp        = BOOL (*)(ApplicationInfo* appInfo);
    using PfnGetVersionFromFile       = BOOL (*)(LPCWSTR path, ApplicationInfo* appInfo);

    PfnInitCrashHandler         m_initCrashHandler;
    PfnSendReport               m_sendReport;
    PfnIsReadyToExit            m_isReadyToExit;
    PfnSetCustomInfo            m_setCustomInfo;
    PfnAddUserInfoToReport      m_addUserInfoToReport;
    PfnRemoveUserInfoFromReport m_removeUserInfoFromReport;
    PfnAddFileToReport          m_addFileToReport;
    PfnRemoveFileFromReport     m_removeFileFromReport;
    PfnGetVersionFromApp        m_getVersionFromApp;
    PfnGetVersionFromFile       m_getVersionFromFile;
};

class CCrashReportTSVN
{
public:
    //! Installs exception handlers to the caller process
    CCrashReportTSVN(LPCWSTR appname, bool bOwnProcess = true)
        : m_nInstallStatus(0)
    {
        ApplicationInfo appInfo     = {0};
        appInfo.ApplicationInfoSize = sizeof(ApplicationInfo);
        appInfo.ApplicationGUID     = "71040f62-f78a-4953-b5b3-5c148349fed7";
        appInfo.Prefix              = "tsvn";
        appInfo.AppName             = appname;
        appInfo.Company             = L"TortoiseSVN";

        appInfo.Hotfix = 0;
        appInfo.V[0]   = TSVN_VERMAJOR;
        appInfo.V[1]   = TSVN_VERMINOR;
        appInfo.V[2]   = TSVN_VERMICRO;
        appInfo.V[3]   = TSVN_VERBUILD;

        HandlerSettings handlerSettings            = {0};
        handlerSettings.HandlerSettingsSize        = sizeof(handlerSettings);
        handlerSettings.LeaveDumpFilesInTempFolder = FALSE;
        handlerSettings.UseWER                     = FALSE;
        handlerSettings.OpenProblemInBrowser       = TRUE;
        handlerSettings.SubmitterID                = 0;

        CCrashReport::Instance().InitCrashHandler(&appInfo, &handlerSettings, bOwnProcess);
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
    CCrashReportThread(DWORD dwFlags = 0)
    {
        UNREFERENCED_PARAMETER(dwFlags);
    }

    /// Deinstalls exception handlers from the caller thread
    ~CCrashReportThread()
    {
    }
};
