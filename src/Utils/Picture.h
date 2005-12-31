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

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

/**
 * \ingroup CommonClasses
 * Class for showing picture files. 
 * Use this class to show pictures of different file formats: BMP, DIB, EMF, GIF, ICO, JPG, WMF
 * The class uses the IPicture interface, the same way as internet explorer does.
 * 
 * Example of usage:
 * \code
 * CPicture m_picture;
 * //load picture data into the IPicture interface
 * m_picture.Load("Test.jpg");	//load from a file
 * m_picture.Load(IDR_TEST, "JPG");	//load from a resource
 * 
 * //when using in a dialog based application (CPaintDC dc(this);)
 * m_picture.UpdateSizeOnDC(&dc);	//get picture dimensions in pixels
 * m_picture.Show(&dc, CPoint(0,0), CPoint(m_picture.m_Width, m_picture.m_Height), 0, 0);
 * m_picture.Show(&dc, CRect(0,0,100,100)); //change original dimensions
 * m_picture.ShowBitmapResource(&dc, IDB_TEST, CPoint(0,0));	//show bitmap resource
 * 
 * //when using in a regular mfc application (CDC* pDC)
 * m_picture.UpdateSizeOnDC(pDC);	//get picture dimensions in pixels
 * m_picture.Show(pDC, CPoint(0,0), CPoint(m_picture.m_Width, m_picture.m_Height), 0, 0);
 * m_picture.Show(pDC, CRect(0,0,100,100)); //change original dimensions
 * m_picture.ShowBitmapResource(pDC, IDB_TEST, CPoint(0,0));	//show bitmap resource
 *
 * //to show picture information
 * CString s;
 * s.Format("Size = %4d\nWidth = %4d\nHeight = %4d\nWeight = %4d\n",
 *				m_picture.m_Weigth, m_picture.m_Width, m_picture.m_Height, m_picture.m_Weight);
 * AfxMessageBox(s);
 * \endcode
 *
 * \par requirements 
 * win95 or later, winNT4 or later
 *
 * \author Stefan Kueng
 *
 * \par license 
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness 
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 * 
 * \version 1.1	
 * added feature to load picture from resources
 * \version 1.0 
 * \date 2001
 * \todo
 * \bug
 * \warning
 *
 */
class CPicture
{
public:
	/**
	 * frees the allocated memory that holds the IPicture interface data and
	 * clear picture information
	 */
	void FreePictureData();
	/**
	 * open a picture file and load it (BMP, DIB, EMF, GIF, ICO, JPG, WMF).
	 *
	 * \param sFilePathName the path of the picture file
	 * \return TRUE if succeeded.
	 */
	BOOL Load(CString sFilePathName);
	/**
	 * open a resource and load it (BMP, DIB, EMF, GIF, ICO, JPG, WMF).
	 * \note
	 * when adding a bitmap resource in visual studio it would automatically show
	 * on "Bitmap". This is NOT good because we need to load it from a custom
	 * resource "BMP".
	 * to add a custom resource: import resource -> open as -> custom
	 *
	 * \param ResourceName the defined name of the resource (UINT). e.g IDR_PICTURE)
	 * \param ResourceType the name of the custom resource type (e.g. "JPG")
	 * \return TRUE if succeeded.
	 */
	BOOL Load(UINT ResourceName, LPCTSTR ResourceType);
	/**
	 * reads the picture data from a source and loads it into the current IPicture object in use.
	 * \param pBuffer buffer of data source
	 * \param nSize the size of the buffer
	 * \return TRUE if succeeded
	 */
	BOOL LoadPictureData(BYTE* pBuffer, int nSize);
	/**
	 * saves the picture as a bitmap file.
	 * \param sFilePathName path of the file to save
	 * \return TRUE if succeeded
	 */
	BOOL SaveAsBitmap(CString sFilePathName);
	/**
	 * draws the loaded picture direct to the given device context.
	 * \note
	 * if the given size is not the actual picture size, then the picture will
	 * be drawn streched to the given dimensions.
	 * \param pDC the device context to draw on
	 * \param LeftTop where to start drawing, i.e. the left/top point of the picture
	 * \param WidthHeight size of the picture to draw
	 * \param MagnifyX magnify pixel width, 0 = default (No Magnify)
	 * \param MagnifyY magnify pixel height, 0 = default (No Magnify)
	 * \return TRUE if succeeded
	 */
	BOOL Show(CDC* pDC, CPoint LeftTop, CPoint WidthHeight, int MagnifyX = 0, int MagnifyY = 0);
	/**
	 * draws the loaded picture direct to the given device context.
	 * \note
	 * if the given size is not the actual picture size, then the picture will
	 * be drawn streched to the given dimensions.
	 * \param pDC the device context to draw on
	 * \param DrawRect the dimensions to draw the picture on
	 * \return TRUE if succeeded
	 */
	BOOL Show(CDC* pDC, CRect DrawRect);
	/**
	 * draw a bitmap resource to the given device context (using Bitblt()).
	 * It will use the bitmap resource original size (only BMP, DIB)
	 * \note
	 * this method is just another simple way of displaying a bitmap resource,
	 * it is not connected with the IPicture interface and can be used
	 * as a standalone method.
	 * \param pDC the device context to draw on
	 * \param BMPResource resource name as defined in the resources
	 * \param LeftTop upper left point to draw the picture
	 * \return TRUE if succeeded
	 */
	static BOOL ShowBitmapResource(CDC* pDC, const int BMPResource, CPoint LeftTop);
	/**
	 * get the original picture pixel size. A pointer to a device context is needed
	 * for the pixel calculation (DPI). Also updates the classes height and width
	 * members.
	 * \param pDC the device context to perform the calculations on
	 * \return TRUE if succeeded
	 */
	BOOL UpdateSizeOnDC(CDC* pDC);

	CPicture();
	virtual ~CPicture();


	HGLOBAL		hGlobal;

	IPicture* m_IPicture; ///< Same As LPPICTURE (typedef IPicture __RPC_FAR *LPPICTURE)

	LONG      m_Height; ///< Height (In Pixels Ignor What Current Device Context Uses)
	LONG      m_Weight; ///< Size Of The Image Object In Bytes (File OR Resource)
	LONG      m_Width;  ///< Width (In Pixels Ignor What Current Device Context Uses)
	CString	  m_Name;	///< The FileName of the Picture as used in Load()
};

