#pragma once


/**
 * \ingroup TortoiseProc
 * Settings page for the icon overlays.
 *
 * \par requirements
 * win95 or later\n
 * winNT4 or later\n
 * MFC\n
 *
 * \version 1.0
 * first version
 *
 * \date 04-14-2003
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
 * \todo 
 *
 * \bug 
 *
 */
class CSetOverlayPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetOverlayPage)

public:
	CSetOverlayPage();   // standard constructor
	virtual ~CSetOverlayPage();
	/**
	 * Saves the changed settings to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

// Dialog Data
	enum { IDD = IDD_SETTINGSOVERLAY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();

private:
	BOOL			m_bShowChangedDirs;
	BOOL			m_bRemovable;
	BOOL			m_bNetwork;
	BOOL			m_bFixed;
	BOOL			m_bCDROM;
	CRegDWORD		m_regShowChangedDirs;
	CRegDWORD		m_regDriveMaskRemovable;
	CRegDWORD		m_regDriveMaskRemote;
	CRegDWORD		m_regDriveMaskFixed;
	CRegDWORD		m_regDriveMaskCDROM;
	CBalloon		m_tooltips;
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedChangeddirs();
	afx_msg void OnBnClickedRemovable();
	afx_msg void OnBnClickedNetwork();
	afx_msg void OnBnClickedFixed();
	afx_msg void OnBnClickedCdrom();
	virtual BOOL OnApply();
};
