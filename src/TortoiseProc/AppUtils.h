// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010 - TortoiseSVN

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
#include "HistoryCombo.h"
#include "SVNRev.h"
#include "CommonAppUtils.h"

class CTSVNPath;

/**
 * \ingroup TortoiseProc
 * An utility class with static functions.
 */
class CAppUtils : public CCommonAppUtils
{
public:
    /**
    * Flags for StartExtDiff function.
    */
    struct DiffFlags
    {
        bool bWait;
        bool bBlame;
        bool bReadOnly;
        bool bAlternativeTool; // If true, invert selection of TortoiseMerge vs. external diff tool

        DiffFlags(): bWait(false), bBlame(false), bReadOnly(false), bAlternativeTool(false) {}
        DiffFlags& Wait(bool b = true) { bWait = b; return *this; }
        DiffFlags& Blame(bool b = true) { bBlame = b; return *this; }
        DiffFlags& ReadOnly(bool b = true) { bReadOnly = b; return *this; }
        DiffFlags& AlternativeTool(bool b = true) { bAlternativeTool = b; return *this; }
    };

    struct MergeFlags
    {
        bool bWait;
        bool bReadOnly;
        bool bAlternativeTool; // If true, invert selection of TortoiseMerge vs. external merge tool

        MergeFlags(): bWait(false), bReadOnly(false), bAlternativeTool(false)   {}
        MergeFlags& Wait(bool b = true) { bWait = b; return *this; }
        MergeFlags& ReadOnly(bool b = true) { bReadOnly = b; return *this; }
        MergeFlags& AlternativeTool(bool b = true) { bAlternativeTool = b; return *this; }
    };

    /**
     * Launches the external merge program if there is one.
     * \return TRUE if the program could be started
     */
    static BOOL StartExtMerge(const MergeFlags& flags,
        const CTSVNPath& basefile, const CTSVNPath& theirfile, const CTSVNPath& yourfile, const CTSVNPath& mergedfile,
        const CString& basename = CString(), const CString& theirname = CString(), const CString& yourname = CString(),
        const CString& mergedname = CString());

    /**
     * Starts the external patch program (currently always TortoiseMerge)
     */
    static BOOL StartExtPatch(const CTSVNPath& patchfile, const CTSVNPath& dir,
            const CString& sOriginalDescription = CString(), const CString& sPatchedDescription = CString(),
            BOOL bReversed = FALSE, BOOL bWait = FALSE);

    /**
     * Starts the external diff application
     */
    static bool StartExtDiff(
        const CTSVNPath& file1, const CTSVNPath& file2,
        const CString& sName1, const CString& sName2, const DiffFlags& flags,
        int line);

    /**
     * Starts the external diff application for properties
     */
    static BOOL StartExtDiffProps(const CTSVNPath& file1, const CTSVNPath& file2,
            const CString& sName1 = CString(), const CString& sName2 = CString(),
            BOOL bWait = FALSE, BOOL bReadOnly = FALSE);

    /**
     * Checks if the given file has a size of less than four, which means
     * an 'empty' file or just newlines, i.e. an empty diff.
     */
    static BOOL CheckForEmptyDiff(const CTSVNPath& sDiffPath);

    /**
     * Create a font which can is used for log messages, etc
     */
    static void CreateFontForLogs(CFont& fontToCreate);

    /**
    * Launch the external blame viewer
    */
    static bool LaunchTortoiseBlame(
        const CString& sBlameFile, const CString& sOriginalFile, const CString& sParams = CString(),
        const SVNRev& startrev = SVNRev(), const SVNRev& endrev = SVNRev());

    /**
     * Formats text in a rich edit control (version 2).
     * text in between * chars is formatted bold
     * text in between ^ chars is formatted italic
     * text in between _ chars is underlined
     */
    static bool FormatTextInRichEditControl(CWnd * pWnd);

    static std::vector<CHARRANGE> FindRegexMatches (const std::wstring& text, const CString& matchstring, const CString& matchsubstring = _T(".*"));

    static bool FindStyleChars(const CString& sText, TCHAR stylechar, int& start, int& end);

    static bool BrowseRepository(CHistoryCombo& combo, CWnd * pParent, SVNRev& rev, bool multiSelection = false);
    static bool BrowseRepository(const CString& repoRoot, CHistoryCombo& combo, CWnd * pParent, SVNRev& rev);

    /**
     * guesses a name of the project from a repository URL
     */
    static  CString GetProjectNameFromURL(CString url);

    /**
     * Replacement for SVNDiff::ShowUnifiedDiff(), but started as a separate process.
     */
    static bool StartShowUnifiedDiff(HWND hWnd, const CTSVNPath& url1, const SVNRev& rev1,
                                    const CTSVNPath& url2, const SVNRev& rev2,
                                    const SVNRev& peg = SVNRev(), const SVNRev& headpeg = SVNRev(),
                                    bool bAlternateDiff = false,
                                    bool bIgnoreAncestry = false,
                                    bool /* blame */ = false);

    /**
     * Replacement for SVNDiff::ShowCompare(), but started as a separate process.
     */
    static bool StartShowCompare(HWND hWnd, const CTSVNPath& url1, const SVNRev& rev1,
                                const CTSVNPath& url2, const SVNRev& rev2,
                                const SVNRev& peg = SVNRev(), const SVNRev& headpeg = SVNRev(),
                                bool bAlternateDiff = false, bool ignoreancestry = false,
                                bool blame = false, svn_node_kind_t nodekind = svn_node_unknown,
                                int line = 0);

    /**
     * Sets up all the default diff and merge scripts.
     * \param force if true, overwrite all existing entries
     * \param either "Diff", "Merge" or an empty string
     */
    static bool SetupDiffScripts(bool force, const CString& type);

    /**
     * Apply the @a effects or color (depending on @a mask)
     * for all char ranges given in @a positions to the
     * @a window text.
     */
    static void SetCharFormat ( CWnd* window
                              , DWORD mask
                              , DWORD effects 
                              , const std::vector<CHARRANGE>& positions);

private:
    static CString PickDiffTool(const CTSVNPath& file1, const CTSVNPath& file2);
    static bool GetMimeType(const CTSVNPath& file, CString& mimetype);
    static void SetCharFormat(CWnd* window, DWORD mask, DWORD effects );
    CAppUtils(void);
    ~CAppUtils(void);
};
