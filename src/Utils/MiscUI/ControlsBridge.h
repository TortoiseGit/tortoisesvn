#pragma once
#include "afxwin.h"

class CControlsBridge {
public:
	static void AlignHorizontally(CWnd* parent, int labelId, int controlId);

private:
	CControlsBridge();
	~CControlsBridge();
};

inline void CControlsBridge::AlignHorizontally(CWnd* parent, int labelId, int controlId)
{
	CString labelText;
	parent->GetDlgItemText(labelId, labelText);

    CDC* dc = parent->GetDC();
	dc->SelectObject(parent->GetDlgItem(labelId )->GetFont());
	CSize textSize(dc->GetTextExtent( labelText ));

	CRect labelRect;
	parent->GetDlgItem(labelId )->GetWindowRect(labelRect);
	parent->ScreenToClient(labelRect);
	labelRect.right = labelRect.left + textSize.cx + 2;
	parent->GetDlgItem(labelId)->MoveWindow(labelRect);

	CRect controlRect;
	parent->GetDlgItem(controlId)->GetWindowRect(controlRect);
	parent->ScreenToClient(controlRect);
	controlRect.left = labelRect.right;
	parent->GetDlgItem(controlId)->MoveWindow(controlRect);
}