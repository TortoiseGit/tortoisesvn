// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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
 * \ingroup Utils
 * A simple wrapper class which loads the Crasher dll and frees
 * it when this object is destroyed.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 * dbghelp.dll version 5.1.2600.0 or newer (for the Crasher.dll)
 *
 * \version 1.0
 * first version
 *
 * \date 01-14-2003
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
 * \todo 
 *
 * \bug 
 *
 */
class CCrashHandler
{
public:
	CCrashHandler(void);
	~CCrashHandler(void);
private:
	HINSTANCE	m_hLib;
};
