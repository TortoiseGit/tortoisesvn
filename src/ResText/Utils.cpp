#include "StdAfx.h"
#include ".\utils.h"

CUtils::CUtils(void)
{
}

CUtils::~CUtils(void)
{
}

void CUtils::StringExtend(LPTSTR str)
{
	TCHAR * cPos = str;
	do
	{
		cPos = _tcschr(cPos, '\n');
		if (cPos)
		{
			memmove(cPos+1, cPos, _tcslen(cPos)*sizeof(TCHAR));
			*cPos = '\\';
			*(cPos+1) = 'n';
		}
	} while (cPos != NULL);
	cPos = str;
	do
	{
		cPos = _tcschr(cPos, '\r');
		if (cPos)
		{
			memmove(cPos+1, cPos, _tcslen(cPos)*sizeof(TCHAR));
			*cPos = '\\';
			*(cPos+1) = 'r';
		}
	} while (cPos != NULL);
	cPos = str;
	do
	{
		cPos = _tcschr(cPos, '\t');
		if (cPos)
		{
			memmove(cPos+1, cPos, _tcslen(cPos)*sizeof(TCHAR));
			*cPos = '\\';
			*(cPos+1) = 't';
		}
	} while (cPos != NULL);
	cPos = str;
	do
	{
		cPos = _tcschr(cPos, '"');
		if (cPos)
		{
			memmove(cPos+1, cPos, _tcslen(cPos)*sizeof(TCHAR));
			*cPos = '\\';
			*(cPos+1) = '"';
			cPos++;
			cPos++;
		}
	} while (cPos != NULL);
}

void CUtils::StringCollapse(LPTSTR str)
{
	TCHAR * cPos = str;
	do
	{
		cPos = _tcschr(cPos, '\\');
		if (cPos)
		{
			if (*(cPos+1) == 'n')
			{
				*cPos = '\n';
				memmove(cPos+1, cPos+2, (_tcslen(cPos+2)+1)*sizeof(TCHAR));
			}
			else if (*(cPos+1) == 'r')
			{
				*cPos = '\r';
				memmove(cPos+1, cPos+2, (_tcslen(cPos+2)+1)*sizeof(TCHAR));
			}
			else if (*(cPos+1) == 't')
			{
				*cPos = '\t';
				memmove(cPos+1, cPos+2, (_tcslen(cPos+2)+1)*sizeof(TCHAR));
			}
			else if (*(cPos+1) == '"')
			{
				*cPos = '"';
				memmove(cPos+1, cPos+2, (_tcslen(cPos+2)+1)*sizeof(TCHAR));
			}
			else
			{
				cPos++;
			}
		}
	} while (cPos != NULL);
}

void CUtils::Error()
{
	LPVOID lpMsgBuf;
	if (!FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL ))
	{
		return;
	}

	// Display the string.
	_ftprintf(stderr, _T("%s\n"), (LPCTSTR)lpMsgBuf);

	// Free the buffer.
	LocalFree( lpMsgBuf );
}