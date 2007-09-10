// TortoiseMerge - a Diff/Patch program

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
#include "DiffStates.h"
#include "FileTextLines.h"

#include <vector>

typedef struct
{
	CString						sLine;
	DiffStates					state;
	int							linenumber; 
	// TODO: move the lineendings enum to a separate header file to get rid
	// of the dependency to CFileTextLines
	CFileTextLines::LineEndings	ending;
} viewdata;

class CViewData
{
public:
	CViewData(void);
	~CViewData(void);

	void						AddData(const CString& sLine, DiffStates state, int linenumber, CFileTextLines::LineEndings ending);
	void						AddData(const viewdata& data);
	void						InsertData(int index, const CString& sLine, DiffStates state, int linenumber, CFileTextLines::LineEndings ending);
	void						InsertData(int index, const viewdata& data);
	void						RemoveData(int index) {m_data.erase(m_data.begin() + index);}

	const viewdata&				GetData(int index) {return m_data[index];}
	const CString&				GetLine(int index) {return m_data[index].sLine;}
	DiffStates					GetState(int index) {return m_data[index].state;}
	int							GetLineNumber(int index) {return m_data[index].linenumber;}
	CFileTextLines::LineEndings	GetLineEnding(int index) {return m_data[index].ending;}

	int							GetCount() {return m_data.size();}

	void						SetState(int index, DiffStates state) {m_data[index].state = state;}
	void						SetLine(int index, const CString& sLine) {m_data[index].sLine = sLine;}
	void						SetLineNumber(int index, int linenumber) {m_data[index].linenumber = linenumber;}

	void						Clear() {m_data.clear();}
	void						Reserve(int length) {m_data.reserve(length);}

protected:
	std::vector<viewdata>		m_data;
};
