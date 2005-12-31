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
#include "stdafx.h"
#include <olectl.h>
#include "Picture.h"
#include "helperfunctions.h"

#define HIMETRIC_INCH 2540

CPicture::CPicture()
{
	m_IPicture = NULL;
	m_Height = 0;
	m_Weight = 0;
	m_Width = 0;
}

CPicture::~CPicture()
{
	if(m_IPicture != NULL) FreePictureData(); // Important - Avoid Leaks...
}


void CPicture::FreePictureData()
{
	if(m_IPicture != NULL)
	{
		m_IPicture->Release();
		m_IPicture = NULL;
		m_Height = 0;
		m_Weight = 0;
		m_Width = 0;
	}
}

BOOL CPicture::Load(UINT ResourceName, LPCTSTR ResourceType)
{
	BOOL bResult = FALSE;

//	HGLOBAL		hGlobal = NULL;
	HRSRC		hSource = NULL;
	LPVOID		lpVoid  = NULL;
	int			nSize   = 0;

	if(m_IPicture != NULL) FreePictureData(); // Important - Avoid Leaks...

	hSource = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(ResourceName), ResourceType);

	if(hSource == NULL)
	{
		return(FALSE);
	}

	hGlobal = LoadResource(AfxGetResourceHandle(), hSource);
	if(hGlobal == NULL)
	{
		TRACE(_T("%s(%i) : LoadResource() Failed\n"),__FILE__,__LINE__);
		return(FALSE);
	}

	lpVoid = LockResource(hGlobal);
	if(lpVoid == NULL)
	{
		TRACE(_T("%s(%i) : LockResource() Failed\n"),__FILE__,__LINE__);
		return(FALSE);
	}
	nSize = (UINT)SizeofResource(AfxGetResourceHandle(), hSource);
	if(LoadPictureData((BYTE*)hGlobal, nSize)) bResult = TRUE;

	UnlockResource(hGlobal); // 16Bit Windows Needs This
	FreeResource(hGlobal); // 16Bit Windows Needs This (32Bit - Automatic Release)

	m_Weight = nSize; // Update Picture Size Info...

	if(m_IPicture != NULL) // Do Not Try To Read From Memory That Is Not Exist...
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
		bResult = FALSE;
	}

	return(bResult);
}

BOOL CPicture::Load(CString sFilePathName)
{
	BOOL bResult = FALSE;
	CFile PictureFile;
	CFileException e;
	int	nSize = 0;

	if(m_IPicture != NULL) FreePictureData(); // Important - Avoid Leaks...

	if(PictureFile.Open(sFilePathName, CFile::modeRead | CFile::typeBinary, &e))
	{
		nSize = (int)PictureFile.GetLength();
		BYTE* pBuffer = new BYTE[nSize];
	
		if(PictureFile.Read(pBuffer, nSize) > 0)
		{
			if(LoadPictureData(pBuffer, nSize))	bResult = TRUE;
		}

		PictureFile.Close();
		delete [] pBuffer;
	}
	else // Open Failed...
	{
		TRACE(_T("%s(%i) : could not open file %s. Systemerror: %s\n"),__FILE__,__LINE__,sFilePathName,GetLastErrorMessageString());
		bResult = FALSE;
	}

	m_Name = sFilePathName;
	m_Weight = nSize; // Update Picture Size Info...

	if(m_IPicture != NULL) // Do Not Try To Read From Memory That Is Not Exist...
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
		bResult = FALSE;
	}

	return(bResult);
}

BOOL CPicture::LoadPictureData(BYTE *pBuffer, int nSize)
{
	BOOL bResult = FALSE;

	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, nSize);

	if(hGlobal == NULL)
	{
		TRACE(_T("%s(%i) : Can not allocate enough memory\n"),__FILE__,__LINE__);
		return(FALSE);
	}

	void* pData = GlobalLock(hGlobal);
	memcpy(pData, pBuffer, nSize);
	GlobalUnlock(hGlobal);

	IStream* pStream = NULL;

	if(CreateStreamOnHGlobal(hGlobal, TRUE, &pStream) == S_OK)
	{
		HRESULT hr;
		if((hr = OleLoadPicture(pStream, nSize, FALSE, IID_IPicture, (LPVOID *)&m_IPicture)) == E_NOINTERFACE)
		{
			TRACE(_T("%s(%i) : IPicture interface is not supported\n"),__FILE__,__LINE__);
			return(FALSE);
		}
		else // S_OK
		{
			pStream->Release();
			pStream = NULL;
			bResult = TRUE;
		}
	}

	FreeResource(hGlobal); // 16Bit Windows Needs This (32Bit - Automatic Release)

	return(bResult);
}

BOOL CPicture::Show(CDC *pDC, CRect DrawRect)
{
    if (pDC == NULL || m_IPicture == NULL) return FALSE;
    
    long Width  = 0;
    long Height = 0;
    m_IPicture->get_Width(&Width);
    m_IPicture->get_Height(&Height);

    HRESULT hrP = NULL;
    
    hrP = m_IPicture->Render(pDC->m_hDC,
                      DrawRect.left,                  // Left
                      DrawRect.top,                   // Top
                      DrawRect.right - DrawRect.left, // Right
                      DrawRect.bottom - DrawRect.top, // Bottom
                      0,
                      Height,
                      Width,
                      -Height,
                      &DrawRect);

    if (SUCCEEDED(hrP)) return(TRUE);

	TRACE(_T("%s(%i) : can not allocate enough memory\n"),__FILE__,__LINE__);
    return(FALSE);
}

BOOL CPicture::Show(CDC *pDC, CPoint LeftTop, CPoint WidthHeight, int MagnifyX, int MagnifyY)
{
    if (pDC == NULL || m_IPicture == NULL) return FALSE;
    
    long Width  = 0;
    long Height = 0;
    m_IPicture->get_Width(&Width);
    m_IPicture->get_Height(&Height);
	if(MagnifyX == NULL) MagnifyX = 0;
	if(MagnifyY == NULL) MagnifyY = 0;
	MagnifyX = int(MulDiv(Width, pDC->GetDeviceCaps(LOGPIXELSX), HIMETRIC_INCH) * MagnifyX);
	MagnifyY = int(MulDiv(Height,pDC->GetDeviceCaps(LOGPIXELSY), HIMETRIC_INCH) * MagnifyY);

	CRect DrawRect(LeftTop.x, LeftTop.y, MagnifyX, MagnifyY);

    HRESULT hrP = NULL;

    hrP = m_IPicture->Render(pDC->m_hDC,
                      LeftTop.x,               // Left
                      LeftTop.y,               // Top
                      WidthHeight.x +MagnifyX, // Width
                      WidthHeight.y +MagnifyY, // Height
                      0,
                      Height,
                      Width,
                      -Height,
                      &DrawRect);

    if(SUCCEEDED(hrP)) return(TRUE);

	TRACE(_T("%s(%i) : can not allocate enough memory\n"),__FILE__,__LINE__);
    return(FALSE);
}

BOOL CPicture::SaveAsBitmap(CString sFilePathName)
{
	BOOL bResult = FALSE;
	ILockBytes *Buffer = 0;
	IStorage   *pStorage = 0;
	IStream    *FileStream = 0;
	BYTE	   *BufferBytes;
	STATSTG		BytesStatistics;
	DWORD		OutData;
	long		OutStream;
	CFile		BitmapFile;	CFileException e;
	double		SkipFloat = 0;
	DWORD		ByteSkip = 0;
	_ULARGE_INTEGER RealData;

	CreateILockBytesOnHGlobal(NULL, TRUE, &Buffer); // Create ILockBytes Buffer

	HRESULT hr = ::StgCreateDocfileOnILockBytes(Buffer,
				 STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_READWRITE, 0, &pStorage);

	hr = pStorage->CreateStream(L"PICTURE",
		 STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_READWRITE, 0, 0, &FileStream);

	m_IPicture->SaveAsFile(FileStream, TRUE, &OutStream); // Copy Data Stream
	FileStream->Release();
	pStorage->Release();
	Buffer->Flush(); 

	// Get Statistics For Final Size Of Byte Array
	Buffer->Stat(&BytesStatistics, STATFLAG_NONAME);

	// Cut UnNeeded Data Coming From SaveAsFile() (Leave Only "Pure" Picture Data)
	SkipFloat = (double(OutStream) / 512); // Must Be In a 512 Blocks...
	if(SkipFloat > DWORD(SkipFloat)) ByteSkip = (DWORD)SkipFloat + 1;
	else ByteSkip = (DWORD)SkipFloat;
	ByteSkip = ByteSkip * 512; // Must Be In a 512 Blocks...
	
	// Find Difference Between The Two Values
	ByteSkip = (DWORD)(BytesStatistics.cbSize.QuadPart - ByteSkip);

	// Allocate Only The "Pure" Picture Data
	RealData.LowPart = 0;
	RealData.HighPart = 0;
	RealData.QuadPart = ByteSkip;
	BufferBytes = (BYTE*)malloc(OutStream);
	if(BufferBytes == NULL)
	{
		Buffer->Release();
		TRACE(_T("%s(%i) : can not allocate enough memory\n"),__FILE__,__LINE__);
	}

	Buffer->ReadAt(RealData, BufferBytes, OutStream, &OutData);

	if(BitmapFile.Open(sFilePathName, CFile::typeBinary | CFile::modeCreate | CFile::modeWrite, &e))
	{
		BitmapFile.Write(BufferBytes, OutData);
		BitmapFile.Close();
		bResult = TRUE;
	}
	else // Write File Failed...
	{
		TRACE(_T("%s(%i) : can not write file %s. Systemerror: %s\n"),__FILE__,__LINE__,sFilePathName,GetLastErrorMessageString());
		bResult = FALSE;
	}
	
	Buffer->Release();
	free(BufferBytes);

	return(bResult);
}

BOOL CPicture::ShowBitmapResource(CDC *pDC, const int BMPResource, CPoint LeftTop)
{
	if (pDC == NULL) return(FALSE);

	CBitmap BMP;
	if(BMP.LoadBitmap(BMPResource))
	{
		// Get Bitmap Details
		BITMAP BMPInfo;
		BMP.GetBitmap(&BMPInfo);

		// Create An In-Memory DC Compatible With The Display DC We R Gonna Paint On
		CDC DCMemory;
		DCMemory.CreateCompatibleDC(pDC);

		// Select The Bitmap Into The In-Memory DC
		CBitmap* pOldBitmap = DCMemory.SelectObject(&BMP);

		// Copy Bits From The In-Memory DC Into The On-Screen DC
		pDC->BitBlt(LeftTop.x, LeftTop.y, BMPInfo.bmWidth, BMPInfo.bmHeight, &DCMemory, 0, 0, SRCCOPY);

		DCMemory.SelectObject(pOldBitmap); // (As Shown In MSDN Example...)
	}
	else 
	{
		TRACE(_T("%s(%i) : can not find the bitmap resource\n"),__FILE__,__LINE__);
		return(FALSE);
	}

	return(TRUE);
}

BOOL CPicture::UpdateSizeOnDC(CDC *pDC)
{
	if(pDC == NULL || m_IPicture == NULL) { m_Height = 0; m_Width = 0; return(FALSE); };

    m_IPicture->get_Height(&m_Height);
    m_IPicture->get_Width(&m_Width);

	// Get Current DPI - Dot Per Inch
    int CurrentDPI_X = pDC->GetDeviceCaps(LOGPIXELSX);
    int CurrentDPI_Y = pDC->GetDeviceCaps(LOGPIXELSY);

	// Use a "Standard" Print (When Printing)
    if(pDC->IsPrinting())
	{
		CurrentDPI_X = 96;
        CurrentDPI_Y = 96;
	}

    m_Height = MulDiv(m_Height, CurrentDPI_Y, HIMETRIC_INCH);
    m_Width  = MulDiv(m_Width,  CurrentDPI_X, HIMETRIC_INCH);

    return(TRUE);
}
