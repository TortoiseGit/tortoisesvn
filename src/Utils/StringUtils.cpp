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
#include "StdAfx.h"
#include "stringutils.h"

int strwildcmp(const char *wild, const char *string)
{
	const char *cp = NULL;
	const char *mp = NULL;
	while ((*string) && (*wild != '*')) 
	{
		if ((*wild != *string) && (*wild != '?')) 
		{
			return 0; 
		}
		wild++; 
		string++; 
	} // while ((*string) && (*wild != '*')) 
	while (*string) 
	{
		if (*wild == '*') 
		{
			if (!*++wild) 
			{
				return 1; 
			} 
			mp = wild; 
			cp = string+1;
		} // if (*wild == '*') 
		else if ((*wild == *string) || (*wild == '?')) 
		{
			wild++;
			string++;
		} 
		else 
		{
			wild = mp;
			string = cp++;
		}
	} // while (*string)

	while (*wild == '*') 
	{
		wild++;
	}
	return !*wild;
}

int wcswildcmp(const wchar_t *wild, const wchar_t *string)
{
	const wchar_t *cp = NULL;
	const wchar_t *mp = NULL;
	while ((*string) && (*wild != '*')) 
	{
		if ((*wild != *string) && (*wild != '?')) 
		{
			return 0; 
		}
		wild++; 
		string++; 
	} // while ((*string) && (*wild != '*')) 
	while (*string) 
	{
		if (*wild == '*') 
		{
			if (!*++wild) 
			{
				return 1; 
			} 
			mp = wild; 
			cp = string+1;
		} // if (*wild == '*') 
		else if ((*wild == *string) || (*wild == '?')) 
		{
			wild++;
			string++;
		} 
		else 
		{
			wild = mp;
			string = cp++;
		}
	} // while (*string)

	while (*wild == '*') 
	{
		wild++;
	}
	return !*wild;
}

#ifdef _MFC_VER
BOOL CStringUtils::WildCardMatch(const CString& wildcard, const CString& string)
{
	return _tcswildcmp(wildcard, string);
}

CString CStringUtils::LinesWrap(const CString& longstring, int limit /* = 80 */, bool bCompactPaths /* = true */)
{
	CString retString;
	CStringArray arWords;
	if (longstring.GetLength() < limit)
		return longstring;	// no wrapping needed.
	// now start breaking the string into words

	int linepos = 0;
	int lineposold = 0;
	CString temp;
	while ((linepos = longstring.Find('\n', linepos)) >= 0)
	{
		temp = longstring.Mid(lineposold, linepos-lineposold);
		if (temp.IsEmpty())
			break;
		if ((linepos+1)<longstring.GetLength())
			linepos++;
		lineposold = linepos;
		if (!retString.IsEmpty())
			retString += _T("\n");
		retString += WordWrap(temp, limit, bCompactPaths);
	}
	temp = longstring.Mid(lineposold);
	if (!temp.IsEmpty())
		retString += _T("\n");
	retString += WordWrap(temp, limit, bCompactPaths);
	retString.Trim();
	retString.Replace(_T("\n\n"), _T("\n"));
	return retString;
}

CString CStringUtils::WordWrap(const CString& longstring, int limit /* = 80 */, bool bCompactPaths /* = true */)
{
	CString retString;
	if (longstring.GetLength() < limit)
		return longstring;	// no wrapping needed.
	CString temp = longstring;
	while (temp.GetLength() > limit)
	{
		int pos=0;
		int oldpos=0;
		while ((pos>=0)&&(temp.Find(' ', pos)<limit)&&(temp.Find(' ', pos)>0))
		{
			oldpos = pos;
			pos = temp.Find(' ', pos+1);
		}
		if (oldpos==0)
			oldpos = temp.Find(' ');
		if (pos<0)
		{
			retString += temp;
			temp.Empty();
		}
		else
		{
			CString longline = oldpos >= 0 ? temp.Left(oldpos+1) : temp;
			if ((bCompactPaths)&&(longline.GetLength() < MAX_PATH))
			{
				if (((!PathIsFileSpec(longline))&&longline.Find(':')<3)||(PathIsURL(longline)))
				{
					TCHAR buf[MAX_PATH];
					PathCompactPathEx(buf, longline, limit+1, 0);
					longline = buf;
				}				
			}

			retString += longline;
			if (oldpos >= 0)
				temp = temp.Mid(oldpos+1);
			else
				temp.Empty();
		}
		retString += _T("\n");
		pos = oldpos;
	}
	retString += temp;
	retString.Trim();
	return retString;
}

void CStringUtils::RemoveAccelerators(CString& text)
{
	int pos = 0;
	while ((pos=text.Find('&',pos))>=0)
	{
		if (text.GetLength() > (pos-1))
		{
			if (text.GetAt(pos+1)!=' ')
				text.Delete(pos);
		}
		pos++;
	}
}


bool CStringUtils::WriteAsciiStringToClipboard(const CStringA& sClipdata, HWND hOwningWnd)
{
	if (OpenClipboard(hOwningWnd))
	{
		EmptyClipboard();
		HGLOBAL hClipboardData;
		hClipboardData = GlobalAlloc(GMEM_DDESHARE, sClipdata.GetLength()+1);
		if (hClipboardData)
		{
			char * pchData;
			pchData = (char*)GlobalLock(hClipboardData);
			if (pchData)
			{
				strcpy_s(pchData, sClipdata.GetLength()+1, (LPCSTR)sClipdata);
				if (GlobalUnlock(hClipboardData))
				{
					if (SetClipboardData(CF_TEXT,hClipboardData)==NULL)
					{
						CloseClipboard();
						return false;
					}
				}
				else
				{
					CloseClipboard();
					return false;
				}
			}
			else
			{
				CloseClipboard();
				return false;
			}
		}
		else
		{
			CloseClipboard();
			return false;
		}
		CloseClipboard();
		return true;
	}
	return false;
}

#endif // #ifdef _MFC_VER


#define IsCharNumeric(C) (!IsCharAlpha(C) && IsCharAlphaNumeric(C))

int CStringUtils::CompareNumerical(LPCTSTR x_str, LPCTSTR y_str)
{
	LPCTSTR num_x_begin = x_str, num_y_begin = y_str;
	DWORD num_x_cnt = 0, num_y_cnt = 0;
	int cs_cmp_result = 2;

	// skip same chars and remember last numeric part of strings
	while ((*x_str || *y_str) && CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_IGNOREWIDTH, x_str, 1, y_str, 1) == 2 /* equal */ )
	{
		if (IsCharNumeric(*x_str))
		{
			++num_x_cnt;
			++num_y_cnt;
		}
		else
		{
			num_x_begin = CharNext(x_str);
			num_y_begin = CharNext(y_str);
			num_x_cnt = 0;
			num_y_cnt = 0;
			if (cs_cmp_result == 2)
				cs_cmp_result = CompareString(LOCALE_USER_DEFAULT, NORM_IGNOREWIDTH, x_str, 1, y_str, 1);
		}
		x_str = CharNext(x_str);
		y_str = CharNext(y_str);
	}

	// parse numeric part of first arg
	if (num_x_cnt || IsCharNumeric(*x_str))
	{
		LPCTSTR x_str_tmp = x_str;
		while (IsCharNumeric(*x_str_tmp))
		{
			++num_x_cnt;
			x_str_tmp = CharNext(x_str_tmp);
		}

		// parse numeric part of second arg
		if (num_y_cnt || IsCharNumeric(*y_str))
		{
			LPCTSTR y_str_tmp = y_str;
			while (IsCharNumeric(*y_str_tmp))
			{
				++num_y_cnt;
				y_str_tmp = CharNext(y_str_tmp);
			}

			DWORD num_x_cnt_with_zeros = num_x_cnt, num_y_cnt_with_zeros = num_y_cnt;

			while (num_x_cnt < num_y_cnt)
			{
				if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNOREWIDTH, num_y_begin, 1, TEXT("0"), 1) != 2 /* not equal to '0' */ )
					return -1;
				num_y_begin = CharNext(num_y_begin);
				--num_y_cnt;
			}

			while (num_x_cnt > num_y_cnt)
			{
				if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNOREWIDTH, num_x_begin, 1, TEXT("0"), 1) != 2 /* not equal to '0' */ )
					return 1;
				num_x_begin = CharNext(num_x_begin);
				--num_x_cnt;
			}

			// here num_x_cnt == num_y_cnt
			int cmp_result = CompareString(LOCALE_USER_DEFAULT, NORM_IGNOREWIDTH, num_x_begin, num_x_cnt, num_y_begin, num_y_cnt);

			if (cmp_result != 2)
				return cmp_result - 2;
			if (num_x_cnt_with_zeros != num_y_cnt_with_zeros)
				return num_x_cnt_with_zeros < num_y_cnt_with_zeros ? -1 : 1;
			if (cs_cmp_result != 2)
				return cs_cmp_result - 2;
		}
	}

	// otherwise, compare literally
	int cmp_result = CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_IGNOREWIDTH, x_str, -1, y_str, -1);
	if (cmp_result != 2)
		return cmp_result - 2;
	if (cs_cmp_result == 2)
		cs_cmp_result = CompareString(LOCALE_USER_DEFAULT, NORM_IGNOREWIDTH, x_str, -1, y_str, -1);
	return cs_cmp_result - 2;
}


#if defined(_DEBUG)
// Some test cases for these classes
static class StringUtilsTest
{
public:
	StringUtilsTest()
	{
		CString longline = _T("this is a test of how a string can be splitted into several lines");
		CString splittedline = CStringUtils::WordWrap(longline, 10);
		ATLTRACE("WordWrap:\n%ws\n", splittedline);
		splittedline = CStringUtils::LinesWrap(longline, 10);
		ATLTRACE("LinesWrap:\n%ws\n", splittedline);
		longline = _T("c:\\this_is_a_very_long\\path_on_windows and of course some other words added to make the line longer");
		splittedline = CStringUtils::WordWrap(longline, 10);
		ATLTRACE("WordWrap:\n%ws\n", splittedline);
		splittedline = CStringUtils::LinesWrap(longline, 10);
		ATLTRACE("LinesWrap:\n%ws\n", splittedline);
		longline = _T("Forced failure in https://myserver.com/a_long_url_to_split PROPFIND error");
		splittedline = CStringUtils::WordWrap(longline, 20);
		ATLTRACE("WordWrap:\n%ws\n", splittedline);
		splittedline = CStringUtils::LinesWrap(longline, 20);
		ATLTRACE("LinesWrap:\n%ws\n", splittedline);
		longline = _T("Forced\nfailure in https://myserver.com/a_long_url_to_split PROPFIND\nerror");
		splittedline = CStringUtils::WordWrap(longline, 40);
		ATLTRACE("WordWrap:\n%ws\n", splittedline);
		splittedline = CStringUtils::LinesWrap(longline, 40);
		ATLTRACE("LinesWrap:\n%ws\n", splittedline);
		longline = _T("Failed to add file\nc:\\export\\spare\\Devl-JBoss\\development\\head\\src\\something\\CoreApplication\\somethingelse\\src\\com\\yetsomthingelse\\shipper\\DAO\\ShipmentInfoDAO1.java\nc:\\export\\spare\\Devl-JBoss\\development\\head\\src\\something\\CoreApplication\\somethingelse\\src\\com\\yetsomthingelse\\shipper\\DAO\\ShipmentInfoDAO2.java");
		splittedline = CStringUtils::WordWrap(longline);
		ATLTRACE("WordWrap:\n%ws\n", splittedline);
		splittedline = CStringUtils::LinesWrap(longline);
		ATLTRACE("LinesWrap:\n%ws\n", splittedline);
	}
} StringUtilsTest;

#endif
