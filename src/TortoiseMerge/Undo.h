// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

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
#include <map>
#include <list>

class CBaseView;

typedef struct viewstate
{
	std::map<int, CString> difflines;
	std::map<int, DWORD> linestates;
	std::map<int, DWORD> linelines;
	std::list<int> addedlines;
} viewstate;

class CUndo
{
public:
	static CUndo& GetInstance();

	bool Undo(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom);
	void AddState(const viewstate& leftstate, const viewstate& rightstate, const viewstate& bottomstate);
	bool CanUndo() {return (m_viewstates.size() > 0);}
protected:
	void Undo(const viewstate& state, CBaseView * pView);
	std::list<viewstate> m_viewstates;
private:
	CUndo();
	~CUndo();
};