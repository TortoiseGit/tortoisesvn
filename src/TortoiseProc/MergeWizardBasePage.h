// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once


/**
 * base class for the merge wizard property pages
 */
class CMergeWizardBasePage : public CPropertyPage
{
public:
	CMergeWizardBasePage() : CPropertyPage() {;}
	explicit CMergeWizardBasePage(UINT nIDTemplate, UINT nIDCaption = 0, DWORD dwSize = sizeof(PROPSHEETPAGE)) 
		: CPropertyPage(nIDTemplate, nIDCaption, dwSize) {;}
	explicit CMergeWizardBasePage(LPCTSTR lpszTemplateName, UINT nIDCaption = 0, DWORD dwSize = sizeof(PROPSHEETPAGE))
		: CPropertyPage(lpszTemplateName, nIDCaption, dwSize) {;}

	virtual ~CMergeWizardBasePage() {;}



protected:
	virtual void			SetButtonTexts()
	{
		CPropertySheet* psheet = (CPropertySheet*) GetParent();
		if (psheet)
		{
			psheet->GetDlgItem(ID_WIZFINISH)->SetWindowText(CString(MAKEINTRESOURCE(IDS_MERGE_MERGE)));
			psheet->GetDlgItem(ID_WIZBACK)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_BACK)));
			psheet->GetDlgItem(ID_WIZNEXT)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_NEXT)));
			psheet->GetDlgItem(IDHELP)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_HELP)));
			psheet->GetDlgItem(IDCANCEL)->SetWindowText(CString(MAKEINTRESOURCE(IDS_PROPPAGE_CANCEL)));
		}
	}
};
