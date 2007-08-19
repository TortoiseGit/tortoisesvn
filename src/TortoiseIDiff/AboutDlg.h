#pragma once
#include "basedialog.h"
#include "hyperlink_base.h"

/**
 * about dialog.
 */
class CAboutDlg : public CDialog
{
public:
	CAboutDlg(HWND hParent);
	~CAboutDlg(void);

	void					SetHiddenWnd(HWND hWnd) {m_hHiddenWnd = hWnd;}
protected:
	LRESULT CALLBACK		DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT					DoCommand(int id);

private:
	HWND					m_hParent;
	HWND					m_hHiddenWnd;
	CHyperLink				m_link;
};
