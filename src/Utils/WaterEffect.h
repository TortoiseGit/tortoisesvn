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

#define random( min, max ) (( rand() % (int)((( max ) + 1 ) - ( min ))) + ( min ))

/**
 * \ingroup Utils
 * 
 * Provides a water effect on a picture. To do that the class needs
 * the picture as an array of pixels. See CDIB class for information
 * of that format.
 * The formulas used in this class I found on a website: http://freespace.virgin.net/hugo.elias/graphics/x_water.htm \n
 * An example of how to use this class and produce a water effect:
 * \code
 * //in your initialization code (e.g. OnInitDialog() )
 * CPictureHolder tmpPic;
 * tmpPic.CreateFromBitmap(IDB_LOGOFLIPPED);
 * m_renderSrc.Create32BitFromPicture(&tmpPic,301,167);
 * m_renderDest.Create32BitFromPicture(&tmpPic,301,167);
 * m_waterEffect.Create(301,167);
 * \endcode
 * In a timer function / method you need to render the picture:
 * \code
 * m_waterEffect.Render((DWORD*)m_renderSrc.GetDIBits(), (DWORD*)m_renderDest.GetDIBits());
 * CClientDC dc(this);
 * CPoint ptOrigin(15,20);
 * m_renderDest.Draw(&dc,ptOrigin);
 * \endcode
 * To add blobs you can do that either in a timer too
 * \code
 * CRect r;
 * r.left = 15;
 * r.top = 20;
 * r.right = r.left + m_renderSrc.GetWidth();
 * r.bottom = r.top + m_renderSrc.GetHeight();
 * m_waterEffect.Blob(random(r.left,r.right), random(r.top, r.bottom), 2, 200, m_waterEffect.m_iHpage);
 * \endcode
 * or/and in a mouse-event handler
 * \code
 * void CTestDlg::OnMouseMove(UINT nFlags, CPoint point)
 * {
 * 	CRect r;
 * 	r.left = 15;
 * 	r.top = 20;
 * 	r.right = r.left + m_renderSrc.GetWidth();
 * 	r.bottom = r.top + m_renderSrc.GetHeight();
 * 
 * 	if(r.PtInRect(point) == TRUE)
 * 	{
 * 		// dibs are drawn upside down...
 * 		point.y -= 20;
 * 		point.y = 167-point.y;
 * 
 * 		if (nFlags & MK_LBUTTON)
 * 			m_waterEffect.Blob(point.x -15,point.y,5,80,m_waterEffect.m_iHpage);
 * 		else
 * 			m_waterEffect.Blob(point.x -15,point.y,2,30,m_waterEffect.m_iHpage);
 * 	}
 * 	CDialog::OnMouseMove(nFlags, point);
 * }
 * \endcode
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.1 
 * made the water density changeable (i.e. added a public variable for that)
 * \version 1.0
 * first version
 *
 * \date 5-12-2001
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
 * \todo add a picture showing the effect for the Doxygen documentation of this class
 *
 * \bug 
 *
 */
class CWaterEffect  
{
public:
	CWaterEffect();
	virtual ~CWaterEffect();


	/**
	 * Creates the CWaterEffect object used for a picture with a width of \a iWidth and a height of \a iHeight
	 * \param iWidth the width of the picture in pixels
	 * \param iHeight the height of the picture in pixels
	 */
	void Create(int iWidth,int iHeight);
	/**
	 * Renders the picture, i.e. perform the required calculations on \a pSrcImage and store the result in
	 * \a pTargetImage
	 * \param pSrcImage the image to perform the rendering on
	 * \param pTargetImage the resulting image
	 */
	void Render(DWORD* pSrcImage,DWORD* pTargetImage);
	/**
	 * Adds a 'Blob' to the picture, i.e. the effect of a drop falling in the water.
	 * \param x the x coordinate of the blob position
	 * \param y the y coordinate of the blob position
	 * \param radius the radius in pixels the blob (or drop) should have
	 * \param height the height of the blob, i.e. how deep it will enter the water
	 * \param page which of the two buffers to use. 
	 * \remark since DIB's are drawn upside down the y coordinate has to be 'flipped', i.e. subtract the
	 * height of the picture from the real y coordinate first.
	 */
	void Blob(int x, int y, int radius, int height, int page);

	int			m_iDensity;	///< The water density, higher values lead to slower water motion
	int			m_iHpage;	///< the buffer which is in use
private:
	/**
	 * Clears both buffers. The result is that all effects are cleared.
	 */
	void ClearWater();
	/**
	 * performs the calculations.
	 * \param npage which buffer to use
	 * \param density the water density
	 */
	void CalcWater(int npage, int density);
	/**
	 * Smoothes the waves of the water so that they dissapear after a while
	 * \param npage the buffer to use
	 */
	void SmoothWater(int npage);

	/**
	 * Draws the watereffect to the resulting image buffer
	 * \param page the internal buffer to use
	 * \param LightModifier how much light to use. Higher values give more 'contrast/shadows' of the waves.
	 * \param pSrcImage the image to use
	 * \param pTargetImage the resulting image
	 */
	void DrawWater(int page, int LightModifier,DWORD* pSrcImage,DWORD* pTargetImage);
	/**
	 * Converts the colors of the source picture (perhaps with color tables) to true color values.
	 */
	COLORREF GetShiftedColor(COLORREF color,int shift);

	int			m_iLightModifier;
	int			m_iWidth;
	int			m_iHeight;

	int*		m_iBuffer1;
	int*		m_iBuffer2;

};

