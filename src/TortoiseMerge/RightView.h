#pragma once
#include "BaseView.h"

class CRightView : public CBaseView
{
	DECLARE_DYNCREATE(CRightView)
public:
	CRightView(void);
	~CRightView(void);
protected:
	void	OnContextMenu(CPoint point, int nLine);
	BOOL	IsStateSelectable(CDiffData::DiffStates state);
	
};
