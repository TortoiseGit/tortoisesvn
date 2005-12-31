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
 * \ingroup Utils
 * Helper class for setting mouse cursors.\n
 * There are two ways of using this class:
 * -# Just declare a CCursor object with the
 *    required cursor. As soon as the object
 *    goes out of scope the previous cursor
 *    is restored.
 *    \code
 *    someMethod()
 *    {
 *      CCursor(IDC_WAIT);
 *      //do something here
 *    }
 *    //now CCursor is out of scope and the default cursor is restored
 *    \endcode
 * -# use the object the usual way. Declare a CCursor object
 *    and use the methods to set the cursors.
 *
 * \remark the class can be used on Win95 and NT4 too, but the
 * hand cursor won't be available.
 *
 * \par requirements
 * win98 or later\n
 * win2k or later\n
 * MFC\n
 *
 * \version 1.0
 * first version
 *
 * \date 02-16-2003
 *
 * \author kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class CCursor
{
public:
	/**
	 * Constructs a CCursor object.
	 */
	CCursor(LPCTSTR CursorName)
	{
		ASSERT(this);
		m_bInitialized = FALSE;
		SetCursor(CursorName);
	}

	CCursor()
	{
		ASSERT(this);
		m_bInitialized = FALSE;
	}
	~CCursor(void)
	{
		ASSERT(this);
		Restore();
	}
	/**
	 * Sets a new cursor. If you previously set a new cursor then
	 * before setting a second cursor the old one is restored and
	 * then the new one is set.
	 */
	HCURSOR SetCursor(LPCTSTR CursorName)
	{
		//first restore possible old cursor before setting new one
		Restore();
		//try to load system cursor
		HCURSOR NewCursor = ::LoadCursor(NULL, CursorName);
		if(!NewCursor)
			//try to load application cursor
			NewCursor = ::LoadCursor(AfxGetResourceHandle(), CursorName);
		if(NewCursor)
		{
			m_hOldCursor = ::SetCursor(NewCursor);
			m_bInitialized = TRUE;
		}
		else
		{
			m_bInitialized = FALSE;
			TRACE("cursor not found!\n");
		}
		return m_hOldCursor;
	}
	/**
	 * Restores the cursor.
	 */
	void Restore()
	{
		ASSERT(this);
		if(m_bInitialized)
		{
			::SetCursor(m_hOldCursor);
			m_hOldCursor = NULL;
		}
		m_bInitialized = FALSE;
	}

private:
	HCURSOR m_hOldCursor;
	BOOL m_bInitialized;
};
