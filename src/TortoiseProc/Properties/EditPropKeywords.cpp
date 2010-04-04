// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010 - TortoiseSVN

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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "EditPropKeywords.h"
#include <cctype>


IMPLEMENT_DYNAMIC(CEditPropKeywords, CStandAloneDialog)

CEditPropKeywords::CEditPropKeywords(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CEditPropKeywords::IDD, pParent)
    , EditPropBase()
    , m_bAuthor(FALSE)
    , m_bDate(FALSE)
    , m_bID(FALSE)
    , m_bRevision(FALSE)
    , m_bURL(FALSE)
    , m_bHeader(FALSE)
{

}

CEditPropKeywords::~CEditPropKeywords()
{
}

void CEditPropKeywords::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_AUTHORKEY, m_bAuthor);
    DDX_Check(pDX, IDC_DATE, m_bDate);
    DDX_Check(pDX, IDC_ID, m_bID);
    DDX_Check(pDX, IDC_REV, m_bRevision);
    DDX_Check(pDX, IDC_URL, m_bURL);
    DDX_Check(pDX, IDC_HEADER, m_bHeader);
    DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
}


BEGIN_MESSAGE_MAP(CEditPropKeywords, CStandAloneDialog)
    ON_BN_CLICKED(IDC_PROPRECURSIVE, &CEditPropKeywords::OnBnClickedProprecursive)
END_MESSAGE_MAP()


// CEditPropKeywords message handlers

BOOL CEditPropKeywords::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();

    ExtendFrameIntoClientArea(IDC_DWM);
    m_aeroControls.SubclassControl(this, IDC_PROPRECURSIVE);
    m_aeroControls.SubclassOkCancel(this);

    std::transform(m_PropValue.begin(), m_PropValue.end(), m_PropValue.begin(), std::tolower);

    if ((m_PropValue.find("author") != std::string::npos) ||
        (m_PropValue.find("lastchangedby") != std::string::npos))
        m_bAuthor = true;
    if ((m_PropValue.find("date") != std::string::npos) ||
        (m_PropValue.find("lastchangeddate") != std::string::npos))
        m_bDate = true;
    if (m_PropValue.find("id") != std::string::npos)
        m_bID = true;
    if ((m_PropValue.find("revision") != std::string::npos) ||
        (m_PropValue.find("rev") != std::string::npos) ||
        (m_PropValue.find("lastchangedrevision") != std::string::npos))
        m_bRevision = true;
    if ((m_PropValue.find("url") != std::string::npos) ||
        (m_PropValue.find("headurl") != std::string::npos))
        m_bURL = true;
    if (m_PropValue.find("header") != std::string::npos)
        m_bHeader = true;

    AdjustControlSize(IDC_AUTHORKEY);
    AdjustControlSize(IDC_DATE);
    AdjustControlSize(IDC_ID);
    AdjustControlSize(IDC_REV);
    AdjustControlSize(IDC_URL);
    AdjustControlSize(IDC_HEADER);
    AdjustControlSize(IDC_PROPRECURSIVE);

    if (m_bFolder)
    {
        // for folders, the property can only be set recursively
        m_bRecursive = TRUE;
    }
    UpdateData(false);

    return TRUE;
}

void CEditPropKeywords::OnOK()
{
    UpdateData();

    std::string keywords;
    if (m_bAuthor)
        AddSpacedWord(keywords, "Author");
    if (m_bDate)
        AddSpacedWord(keywords, "Date");
    if (m_bID)
        AddSpacedWord(keywords, "Id");
    if (m_bRevision)
        AddSpacedWord(keywords, "Revision");
    if (m_bURL)
        AddSpacedWord(keywords, "URL");
    if (m_bHeader)
        AddSpacedWord(keywords, "Header");

    m_PropValue = keywords;
    m_bChanged = true;

    CStandAloneDialog::OnOK();
}

void CEditPropKeywords::OnBnClickedProprecursive()
{
    UpdateData();
    if (m_bFolder)
        m_bRecursive = TRUE;
    UpdateData(false);
}

void CEditPropKeywords::AddSpacedWord(std::string& str, const std::string& word)
{
    if (str.size())
        str += " ";
    str += word;
}

