
#include "stdafx.h"
#include "StandAloneDlg.h"
#include "ResizableDialog.h"

BEGIN_MESSAGE_MAP(CStandAloneDialog<CDialog>, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CStandAloneDialog<CResizableDialog>, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()
