
#include "stdafx.h"
#include "StandAloneDlg.h"

BEGIN_MESSAGE_MAP(CStandAloneDialog, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CResizableStandAloneDialog, CResizableDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()
