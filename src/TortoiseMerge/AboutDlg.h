#pragma once

#include "Watereffect.h"
#include "Dib.h"
#include "HyperLink.h"
#include <afxctl.h>
#include <afxwin.h>
#include "afxwin.h"


#define ID_EFFECTTIMER 1111
#define ID_DROPTIMER 1112

/**
 * \ingroup TortoiseMerge
 * Class for showing an About box of TortoiseMerge. Contains a Picture
 * with the TortoiseMerge logo with a nice water effect. See CWaterEffect
 * for the implementation.
 */
class CAboutDlg : public CDialog
{
	DECLARE_DYNAMIC(CAboutDlg)

public:
	CAboutDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUT };

protected:
	HICON m_hIcon;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	DECLARE_MESSAGE_MAP()
public:

private:
	CWaterEffect m_waterEffect;
	CDib m_renderSrc;
	CDib m_renderDest;
	CHyperLink m_cWebLink;
	CHyperLink m_cSupportLink;
public:
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};
