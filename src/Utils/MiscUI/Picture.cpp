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
#include <locale>
#include <algorithm>
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
	bHaveGDIPlus = false;
	m_ip = InterpolationModeDefault;
	hIcons = NULL;
	lpIcons = NULL;
	nCurrentIcon = 0;
	bIsIcon = false;
}

CPicture::~CPicture()
{
	FreePictureData(); // Important - Avoid Leaks...
	if (pBitmap)
		delete (pBitmap);
	if (bHaveGDIPlus)
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
	if (hIcons)
	{
		LPICONDIR lpIconDir = (LPICONDIR)lpIcons;
		if (lpIconDir)
		{
			for (int i=0; i<lpIconDir->idCount; ++i)
			{
				DestroyIcon(hIcons[i]);
			}
		}
		delete hIcons;
		hIcons = NULL;
	}
	if (lpIcons)
		delete lpIcons;
}

bool CPicture::Load(stdstring sFilePathName)
{
	bool bResult = false;
	bIsIcon = false;
	lpIcons = NULL;
	//CFile PictureFile;
	//CFileException e;
	int	nSize = 0;
	m_FileSize.clear();
	FreePictureData(); // Important - Avoid Leaks...

	HMODULE hLib = LoadLibrary(_T("gdiplus.dll"));
	if (hLib)
	{
		// we have gdiplus, so try loading the picture with that one
		if (GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, NULL )==Ok)
		{   
			{
				pBitmap = new Bitmap(sFilePathName.c_str(), FALSE);
				GUID guid;
				pBitmap->GetRawFormat(&guid);

				// gdiplus only loads the first icon found in an icon file
				// so we have to handle icon files ourselves :(

				// Even though gdiplus can load icons, it can't load the new
				// icons from Vista - in Vista, the icon format changed slightly.
				// But the LoadIcon/LoadImage API still can load those icons,
				// at least those dimensions which are also used on pre-Vista
				// systems.
				// For that reason, we don't rely on gdiplus telling us if
				// the image format is "icon" or not, we also check the
				// file extension for ".ico".
				std::transform(sFilePathName.begin(), sFilePathName.end(), sFilePathName.begin(), ::tolower);
				bool bIco = _tcsstr(sFilePathName.c_str(), _T(".ico"))!=NULL;
				if ((guid == ImageFormatIcon)||((pBitmap->GetLastStatus() != Ok)&&(bIco)))
				{
					delete (pBitmap);
					pBitmap = NULL;
					bIsIcon = true;

					HANDLE hFile = CreateFile(sFilePathName.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hFile != INVALID_HANDLE_VALUE)
					{
						BY_HANDLE_FILE_INFORMATION fileinfo;
						if (GetFileInformationByHandle(hFile, &fileinfo))
						{
							lpIcons = new BYTE[fileinfo.nFileSizeLow];
							DWORD readbytes;
							if (ReadFile(hFile, lpIcons, fileinfo.nFileSizeLow, &readbytes, NULL))
							{
								// we have the icon. Now gather the information we need later
								CloseHandle(hFile);
								nCurrentIcon = 0;
								LPICONDIR lpIconDir = (LPICONDIR)lpIcons;
								hIcons = new HICON[lpIconDir->idCount];
								m_Width = lpIconDir->idEntries[0].bWidth;
								m_Height = lpIconDir->idEntries[0].bHeight;
								for (int i=0; i<lpIconDir->idCount; ++i)
								{
									hIcons[i] = (HICON)LoadImage(NULL, sFilePathName.c_str(), IMAGE_ICON, 
																lpIconDir->idEntries[i].bWidth,
																lpIconDir->idEntries[i].bHeight,
																LR_LOADFROMFILE);
								}
								bResult = true;
							}
							else
							{
								delete lpIcons;
								lpIcons = NULL;
								CloseHandle(hFile);
							}
						}
						else
							CloseHandle(hFile);
					}
				}
				else
				{
					m_Height = pBitmap->GetHeight();
					m_Width = pBitmap->GetWidth();
					bResult = true;
				}
			}
			bHaveGDIPlus = true;
		}
		else
			pBitmap = NULL;
		FreeLibrary(hLib);
	}
	else
	{
		pBitmap = NULL;
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

	if ((CreateStreamOnHGlobal(hGlobal, true, &pStream) == S_OK)&&(pStream))
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
	if (bIsIcon && lpIcons)
	{
		::DrawIconEx(hDC, DrawRect.left, DrawRect.top, hIcons[nCurrentIcon], DrawRect.right-DrawRect.left, DrawRect.bottom-DrawRect.top, 0, NULL, DI_NORMAL);
		return true;
	}
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
		graphics.SetInterpolationMode(m_ip);
		Rect rect(DrawRect.left, DrawRect.top, DrawRect.right-DrawRect.left, DrawRect.bottom-DrawRect.top);
		graphics.DrawImage(pBitmap, rect);
	}

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

UINT CPicture::GetColorDepth()
{
	if (bIsIcon && lpIcons)
	{
		LPICONDIR lpIconDir = (LPICONDIR)lpIcons;
		return lpIconDir->idEntries[nCurrentIcon].wBitCount;
	}
	switch (GetPixelFormat())
	{
	case PixelFormat1bppIndexed:
		return 1;
	case PixelFormat4bppIndexed:
		return 4;
	case PixelFormat8bppIndexed:
		return 8;
	case PixelFormat16bppARGB1555:
	case PixelFormat16bppGrayScale:
	case PixelFormat16bppRGB555:
	case PixelFormat16bppRGB565:
		return 16;
	case PixelFormat24bppRGB:
		return 24;
	case PixelFormat32bppARGB:
	case PixelFormat32bppPARGB:
	case PixelFormat32bppRGB:
		return 32;
	case PixelFormat48bppRGB:
		return 48;
	case PixelFormat64bppARGB:
	case PixelFormat64bppPARGB:
		return 64;
	}
	return 0;
}

UINT CPicture::GetNumberOfFrames(int dimension)
{
	if (bIsIcon && lpIcons)
	{
		return 1;
	}
	if (pBitmap == NULL)
		return 0;
	UINT count = 0;
	count = pBitmap->GetFrameDimensionsCount();
	GUID* pDimensionIDs = (GUID*)malloc(sizeof(GUID)*count);

	pBitmap->GetFrameDimensionsList(pDimensionIDs, count);

	UINT frameCount = pBitmap->GetFrameCount(&pDimensionIDs[dimension]);

	free(pDimensionIDs);
	return frameCount;
}

UINT CPicture::GetNumberOfDimensions()
{
	if (bIsIcon && lpIcons)
	{
		LPICONDIR lpIconDir = (LPICONDIR)lpIcons;
		return lpIconDir->idCount;
	}
	return pBitmap ? pBitmap->GetFrameDimensionsCount() : 0;
}

long CPicture::SetActiveFrame(UINT frame)
{
	if (bIsIcon && lpIcons)
	{
		nCurrentIcon = frame-1;
		m_Height = GetHeight();
		m_Width = GetWidth();
		return 0;
	}
	if (pBitmap == NULL)
		return 0;
	UINT count = 0;
	count = pBitmap->GetFrameDimensionsCount();
	GUID* pDimensionIDs = (GUID*)malloc(sizeof(GUID)*count);

	pBitmap->GetFrameDimensionsList(pDimensionIDs, count);

	UINT frameCount = pBitmap->GetFrameCount(&pDimensionIDs[0]);

	free(pDimensionIDs);

	if (frame > frameCount)
		return 0;

	GUID pageGuid = FrameDimensionTime;
	pBitmap->SelectActiveFrame(&pageGuid, frame);

	// Assume that the image has a property item of type PropertyItemEquipMake.
	// Get the size of that property item.
	int nSize = pBitmap->GetPropertyItemSize(PropertyTagFrameDelay);

	// Allocate a buffer to receive the property item.
	PropertyItem* pPropertyItem = (PropertyItem*) malloc(nSize);

	pBitmap->GetPropertyItem(PropertyTagFrameDelay, nSize, pPropertyItem);

	UINT prevframe = frame;
	prevframe--;
	if (prevframe < 0)
		prevframe = 0;
	long delay = ((long*)pPropertyItem->value)[prevframe] * 10;
	free(pPropertyItem);
	m_Height = GetHeight();
	m_Width = GetWidth();
	return delay;
}

UINT CPicture::GetHeight()
{
	if ((bIsIcon)&&(lpIcons))
	{
		LPICONDIR lpIconDir = (LPICONDIR)lpIcons;
		return lpIconDir->idEntries[nCurrentIcon].bHeight;
	}
	return pBitmap ? pBitmap->GetHeight() : 0;
}

UINT CPicture::GetWidth()
{
	if ((bIsIcon)&&(lpIcons))
	{
		LPICONDIR lpIconDir = (LPICONDIR)lpIcons;
		return lpIconDir->idEntries[nCurrentIcon].bWidth;
	}
	return pBitmap ? pBitmap->GetWidth() : 0;
}

PixelFormat CPicture::GetPixelFormat()
{
	if ((bIsIcon)&&(lpIcons))
	{
		LPICONDIR lpIconDir = (LPICONDIR)lpIcons;
		if (lpIconDir->idEntries[nCurrentIcon].wPlanes == 1)
		{
			if (lpIconDir->idEntries[nCurrentIcon].wBitCount == 1)
				return PixelFormat1bppIndexed;
			if (lpIconDir->idEntries[nCurrentIcon].wBitCount == 4)
				return PixelFormat4bppIndexed;
			if (lpIconDir->idEntries[nCurrentIcon].wBitCount == 8)
				return PixelFormat8bppIndexed;
		}
		if (lpIconDir->idEntries[nCurrentIcon].wBitCount == 32)
		{
			return PixelFormat32bppARGB;
		}
	}
	return pBitmap ? pBitmap->GetPixelFormat() : 0;
}