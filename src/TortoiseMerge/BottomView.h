#pragma once
#include "BaseView.h"

class CBottomView : public CBaseView
{
	DECLARE_DYNCREATE(CBottomView)
public:
	CBottomView(void);
	~CBottomView(void);
protected:
	void	OnContextMenu(CPoint point, int nLine);
	BOOL	IsStateSelectable(CDiffData::DiffStates state);
	
};
