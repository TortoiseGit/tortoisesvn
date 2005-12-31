// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#pragma once

/**
 * \ingroup CommonClasses
 * A simple wrapper class for the SHBrowseForFolder API.
 * Help-Link: ms-help://MS.VSCC/MS.MSDNVS/shellcc/platform/Shell/Functions/SHBrowseForFolder.htm
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 *
 * \version 1.0
 * first version
 *
 * \date 06-2002
 *
 * \author Stefan Kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class CBrowseFolder
{
public:
	enum retVal
	{
		CANCEL = 0,		///< the user has pressed cancel
		NOPATH,			///< no folder was selected
		OK				///< everything went just fine
	};
public:
	//constructor / deconstructor
	CBrowseFolder(void);
	~CBrowseFolder(void);
public:
	DWORD m_style;		///< styles of the dialog.
	/**
	 * Sets the infotext of the dialog. Call this method before calling Show().
	 */
	void SetInfo(LPCTSTR title);
	/*
	 * Sets the text to show for the checkbox. If this method is not called,
	 * then no checkbox is added.
	 */
	void SetCheckBoxText(LPCTSTR checktext);
	void SetCheckBoxText2(LPCTSTR checktext);
	/**
	 * Shows the Dialog. 
	 * \param parent [in] window handle of the parent window.
	 * \param path [out] the path to the folder which the user has selected 
	 * \return one of CANCEL, NOPATH or OK
	 */
	CBrowseFolder::retVal Show(HWND parent, CString& path, const CString& sDefaultPath = CString());
	CBrowseFolder::retVal Show(HWND parent, LPTSTR path, size_t pathlen, LPCTSTR szDefaultPath = NULL);
	static BOOL m_bCheck;		///< state of the checkbox on closing the dialog
	static BOOL m_bCheck2;
	TCHAR m_title[200];
protected:
	static void SetFont(HWND hwnd,LPTSTR FontName,int FontSize);

	static int CALLBACK BrowseCallBackProc(HWND  hwnd,UINT  uMsg,LPARAM  lParam,LPARAM  lpData);
	static LRESULT APIENTRY CheckBoxSubclassProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static LRESULT APIENTRY CheckBoxSubclassProc2(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
		
	static WNDPROC CBProc;
	static HWND checkbox;
	static HWND checkbox2;
	static HWND ListView;
	static CString m_sDefaultPath;
	TCHAR m_displayName[200];
	LPITEMIDLIST m_root;
	static TCHAR m_CheckText[200];
	static TCHAR m_CheckText2[200];
};
