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

#include "stdafx.h"
#include <olectl.h>
#include "shlwapi.h"
#include "Picture.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdiplus.lib")

#define HIMETRIC_INCH 2540

CPicture::CPicture()
{
	m_IPicture = NULL;
	m_Height = 0;
	m_Weight = 0;
	m_Width = 0;
	pBitmap = NULL;
}

CPicture::~CPicture()
{
	if (m_IPicture != NULL) 
		FreePictureData(); // Important - Avoid Leaks...
	GdiplusShutdown(gdiplusToken);
}


void CPicture::FreePictureData()
{
	if (m_IPicture != NULL)
	{
		m_IPicture->Release();
		m_IPicture = NULL;
		m_Height = 0;
		m_Weight = 0;
		m_Width = 0;
	}
}

bool CPicture::Load(stdstring sFilePathName)
{
	bool bResult = false;
	//CFile PictureFile;
	//CFileException e;
	int	nSize = 0;
	m_FileSize.clear();
	if (m_IPicture != NULL) 
		FreePictureData(); // Important - Avoid Leaks...

	HANDLE hFile = CreateFile(sFilePathName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		BY_HANDLE_FILE_INFORMATION fileinfo;
		if (GetFileInformationByHandle(hFile, &fileinfo))
		{
			BYTE * buffer = new BYTE[fileinfo.nFileSizeLow];
			DWORD readbytes;
			if (ReadFile(hFile, buffer, fileinfo.nFileSizeLow, &readbytes, NULL))
			{
				if (LoadPictureData(buffer, readbytes))
				{
					TCHAR buf[100];
					_stprintf_s(buf, _T("%ld kBytes"), fileinfo.nFileSizeLow/1024);
					m_FileSize = stdstring(buf);
					bResult = true;
				}
			}
			delete buffer;
		}
		CloseHandle(hFile);
	}
	else
		return bResult;

	m_Name = sFilePathName;
	m_Weight = nSize; // Update Picture Size Info...

	if(m_IPicture != NULL) // Do Not Try To Read From Memory That Does Not Exist...
	{ 
		m_IPicture->get_Height(&m_Height);
		m_IPicture->get_Width(&m_Width);
		// Calculate Its Size On a "Standard" (96 DPI) Device Context
		m_Height = MulDiv(m_Height, 96, HIMETRIC_INCH);
		m_Width  = MulDiv(m_Width,  96, HIMETRIC_INCH);
	}
	else // Picture Data Is Not a Known Picture Type
	{
		m_Height = 0;
		m_Width = 0;
		bResult = false;
	}
	if (bResult == false)
	{
		TCHAR gdifindbuf[MAX_PATH];
		_tcscpy_s(gdifindbuf, MAX_PATH, _T("gdiplus.dll"));
		if (PathFindOnPath(gdifindbuf, NULL))
		{
			// we have gdiplus, so try loading the picture with that one
			if (GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, NULL )==Ok)
			{   
				{
					pBitmap = new Bitmap(sFilePathName.c_str(), FALSE);
					m_Height = pBitmap->GetHeight();
					m_Width = pBitmap->GetWidth();
					bResult = true;
				}
			}
			else
				pBitmap = NULL;
		}
		else
		{
			pBitmap = NULL;
		}

	}

	return(bResult);
}

bool CPicture::LoadPictureData(BYTE *pBuffer, int nSize)
{
	bool bResult = false;

	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, nSize);

	if(hGlobal == NULL)
	{
		return(false);
	}

	void* pData = GlobalLock(hGlobal);
	if (pData == NULL)
		return false;
	memcpy(pData, pBuffer, nSize);
	GlobalUnlock(hGlobal);

	IStream* pStream = NULL;

	if(CreateStreamOnHGlobal(hGlobal, true, &pStream) == S_OK)
	{
		HRESULT hr;
		if((hr = OleLoadPicture(pStream, nSize, false, IID_IPicture, (LPVOID *)&m_IPicture)) == S_OK)
		{
			pStream->Release();
			pStream = NULL;
			bResult = true;
		}
		else
		{
			return false;
		}
	}

	FreeResource(hGlobal); // 16Bit Windows Needs This (32Bit - Automatic Release)

	return(bResult);
}

bool CPicture::Show(HDC hDC, RECT DrawRect)
{
	if (hDC == NULL) 
		return false;
	if ((m_IPicture == NULL)&&(pBitmap == NULL))
		return false;

	if (m_IPicture)
	{
		long Width  = 0;
		long Height = 0;
		m_IPicture->get_Width(&Width);
		m_IPicture->get_Height(&Height);

		HRESULT hrP = NULL;

		hrP = m_IPicture->Render(hDC,
			DrawRect.left,                  // Left
			DrawRect.top,                   // Top
			DrawRect.right - DrawRect.left, // Right
			DrawRect.bottom - DrawRect.top, // Bottom
			0,
			Height,
			Width,
			-Height,
			&DrawRect);

		if (SUCCEEDED(hrP)) 
			return(true);
	}
	else if (pBitmap)
	{
		Graphics graphics(hDC);
		Rect rect(DrawRect.left, DrawRect.top, DrawRect.right-DrawRect.left, DrawRect.bottom-DrawRect.top);
		graphics.DrawImage(pBitmap, rect);
	}

	return(false);
}

bool CPicture::Show(HDC hDC, POINT LeftTop, POINT WidthHeight, int MagnifyX, int MagnifyY)
{
	if (hDC == NULL || m_IPicture == NULL) return false;
	
	long Width  = 0;
	long Height = 0;
	m_IPicture->get_Width(&Width);
	m_IPicture->get_Height(&Height);
	if(MagnifyX == NULL) MagnifyX = 0;
	if(MagnifyY == NULL) MagnifyY = 0;
	MagnifyX = int(MulDiv(Width, GetDeviceCaps(hDC, LOGPIXELSX), HIMETRIC_INCH) * MagnifyX);
	MagnifyY = int(MulDiv(Height,GetDeviceCaps(hDC, LOGPIXELSY), HIMETRIC_INCH) * MagnifyY);

	RECT DrawRect;
	DrawRect.left = LeftTop.x;
	DrawRect.top = LeftTop.y;
	DrawRect.right = MagnifyX;
	DrawRect.bottom = MagnifyY;

	HRESULT hrP = NULL;

	hrP = m_IPicture->Render(hDC,
					  LeftTop.x,               // Left
					  LeftTop.y,               // Top
					  WidthHeight.x +MagnifyX, // Width
					  WidthHeight.y +MagnifyY, // Height
					  0,
					  Height,
					  Width,
					  -Height,
					  &DrawRect);

	if(SUCCEEDED(hrP)) return(true);

	return(false);
}



bool CPicture::UpdateSizeOnDC(HDC hDC)
{
	if(hDC == NULL || m_IPicture == NULL) { m_Height = 0; m_Width = 0; return(false); };

	m_IPicture->get_Height(&m_Height);
	m_IPicture->get_Width(&m_Width);

	// Get Current DPI - Dot Per Inch
	int CurrentDPI_X = GetDeviceCaps(hDC, LOGPIXELSX);
	int CurrentDPI_Y = GetDeviceCaps(hDC, LOGPIXELSY);

	m_Height = MulDiv(m_Height, CurrentDPI_Y, HIMETRIC_INCH);
	m_Width  = MulDiv(m_Width,  CurrentDPI_X, HIMETRIC_INCH);

	return(true);
}
