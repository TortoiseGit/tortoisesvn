#include "StdAfx.h"
#include "ViewData.h"

CViewData::CViewData(void)
{
}

CViewData::~CViewData(void)
{
}

void CViewData::AddData(const CString& sLine, DiffStates state, int linenumber, CFileTextLines::LineEndings ending)
{
	viewdata data;
	data.sLine = sLine;
	data.state = state;
	data.linenumber = linenumber;
	data.ending = ending;
	return AddData(data);
}

void CViewData::AddData(const viewdata& data)
{
	return m_data.push_back(data);
}

void CViewData::InsertData(int index, const CString& sLine, DiffStates state, int linenumber, CFileTextLines::LineEndings ending)
{
	viewdata data;
	data.sLine = sLine;
	data.state = state;
	data.linenumber = linenumber;
	data.ending = ending;
	return InsertData(index, data);
}

void CViewData::InsertData(int index, const viewdata& data)
{
	m_data.insert(m_data.begin()+index, data);
}