// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseSVN

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

#ifndef MB_CANCELTRYCONTINUE
#define MB_CANCELTRYCONTINUE        0x00000006L // adds three buttons, "Cancel", "Try Again", "Continue"
#endif

#define MB_CONTINUEABORT            0x00000007L // adds two buttons, "Continue", "Abort"
#define MB_SKIPSKIPALLCANCEL        0x00000008L // adds three buttons, "Skip", "Skip All", "Cancel"
#define MB_IGNOREIGNOREALLCANCEL    0x00000009L // adds three buttons, "Ignore", "Ignore All", "Cancel"

#define MB_DONOTASKAGAIN            0x01000000L // add checkbox "Do not ask me again"
#define MB_DONOTTELLAGAIN           0x02000000L // add checkbox "Do not tell me again"
#define MB_DONOTSHOWAGAIN           0x04000000L // add checkbox "Do not show again"
#define MB_YESTOALL                 0x08000000L // must be used with either MB_YESNO or MB_YESNOCANCEL
#define MB_NOTOALL                  0x10000000L // must be used with either MB_YESNO or MB_YESNOCANCEL
#define MB_NORESOURCE               0x20000000L // do not try to load button strings from resources
#define MB_NOSOUND                  0x40000000L // do not play sound when mb is displayed
#define MB_TIMEOUT                  0x80000000L // returned if timeout expired

#define MB_DEFBUTTON5               0x00000400L
#define MB_DEFBUTTON6               0x00000500L

#ifndef IDTRYAGAIN
#define IDTRYAGAIN          10
#endif

#ifndef IDCONTINUE
#define IDCONTINUE          11
#endif

#define IDSKIP              14
#define IDSKIPALL           15
#define IDIGNOREALL         16

#define IDYESTOALL          19
#define IDNOTOALL           20

// following 4 ids MUST be sequential
#define IDCUSTOM1           23
#define IDCUSTOM2           24
#define IDCUSTOM3           25
#define IDCUSTOM4           26


UINT TSVNMessageBox(HWND hWnd, LPCTSTR lpMessage, LPCTSTR lpCaption, UINT uType, LPCTSTR sHelpPath = NULL);
UINT TSVNMessageBox(HWND hWnd, UINT nMessage, UINT nCaption, UINT uType, LPCTSTR sHelpPath = NULL);

UINT TSVNMessageBox(HWND hWnd, LPCTSTR lpMessage, LPCTSTR lpCaption, int nDef, LPCTSTR lpButton1, LPCTSTR lpButton2, LPCTSTR lpButton3 = NULL);
//UINT TSVNMessageBox(HWND hWnd, UINT nMessage, UINT nCaption, int nDef, LPCTSTR icon, UINT nButton1, UINT nButton2 = NULL, UINT nButton3 = NULL);

