#pragma once
#include "BaseView.h"

class CLeftView : public CBaseView
{
	DECLARE_DYNCREATE(CLeftView)
public:
	CLeftView(void);
	~CLeftView(void);
protected:
	void	OnContextMenu(CPoint point, int nLine);
	BOOL	IsStateSelectable(CDiffData::DiffStates state);
	
};
