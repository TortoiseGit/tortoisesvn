#pragma once

#include <windows.h>
#include "ShellExt.h"

/**
 * \ingroup TortoiseShell
 * Displays and updates all controls on the property page. The property
 * page itself is shown by explorer.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 *
 * \version 1.0
 * first version
 *
 * \date 10-11-2002
 *
 * \author kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 * 
 * \todo use the themed controls for Windows XP
 * \todo better handling of SVN properties
 *
 * \bug 
 *
 */
class CSVNPropertyPage
{
public:
	CSVNPropertyPage(const stdstring &filename);
	virtual ~CSVNPropertyPage();

	/**
	 * Sets the window handle.
	 * \param hwnd the handle.
	 */
	virtual void SetHwnd(HWND hwnd);
	/**
	 * Callback function which receives the window messages of the
	 * property page. See the Win32 API for PropertySheets for details.
	 * Or go directly to ms-help://MS.VSCC/MS.MSDNVS/winui/dlgboxes_7ycz.htm
	 * in the VS.NET help.
	 */
	virtual BOOL PageProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	virtual const stdstring &GetFilename() const { return filename; }

protected:
	/**
	 * Initializes the property page.
	 */
	virtual void InitWorkfileView();

	HWND m_hwnd;
	const stdstring filename;
	TCHAR stringtablebuffer[255];

//#define MAKESTRING(ID) LoadString(g_hmodThisDll, ID, stringtablebuffer, sizeof(stringtablebuffer))
};


