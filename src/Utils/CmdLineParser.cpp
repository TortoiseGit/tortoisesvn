// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#include "stdafx.h"
#include "CmdLineParser.h"

const TCHAR CCmdLineParser::m_sDelims[] = _T("-/");
const TCHAR CCmdLineParser::m_sQuotes[] = _T("\"");
const TCHAR CCmdLineParser::m_sValueSep[] = _T(" :"); // don't forget space!!


CCmdLineParser::CCmdLineParser(LPCTSTR sCmdLine)
{
	if(sCmdLine) 
	{
		Parse(sCmdLine);
	}
}

CCmdLineParser::~CCmdLineParser()
{
	m_valueMap.clear();
}

BOOL CCmdLineParser::Parse(LPCTSTR sCmdLine) 
{
	const CString sEmpty = _T("");			//use this as a value if no actual value is given in commandline
	int nArgs = 0;

	if(!sCmdLine) 
		return false;
	
	m_valueMap.clear();
	m_sCmdLine = sCmdLine;

	LPCTSTR sCurrent = sCmdLine;

	for(;;)
	{
		//format is  -Key:"arg"
		
		if (_tcslen(sCurrent) == 0)
			break;		// no more data, leave loop

		LPCTSTR sArg = _tcspbrk(sCurrent, m_sDelims);
		if(!sArg) 
			break; // no (more) delimeters found
		sArg =  _tcsinc(sArg);

		if(_tcslen(sArg) == 0) 
			break; // ends with delim
			
		LPCTSTR sVal = _tcspbrk(sArg, m_sValueSep);
		if(sVal == NULL) 
		{ 
			CString Key(sArg);
			Key.MakeLower();
			m_valueMap.insert(CValsMap::value_type(Key, sEmpty));
			break;
		} 
		else if (sVal[0] == _T(' ') || _tcslen(sVal) == 1 ) 
		{ 
			// cmdline ends with /Key: or a key with no value 
			CString Key(sArg, (int)(sVal - sArg));
			if(!Key.IsEmpty()) 
			{ 
				Key.MakeLower();
				m_valueMap.insert(CValsMap::value_type(Key, sEmpty));
			}
			sCurrent = _tcsinc(sVal);
			continue;
		}
		else 
		{ 
			// key has value
			CString Key(sArg, (int)(sVal - sArg));
			Key.MakeLower();

			sVal = _tcsinc(sVal);

			LPCTSTR sQuote = _tcspbrk(sVal, m_sQuotes), sEndQuote(NULL);
			if(sQuote == sVal) 
			{ 
				// string with quotes (defined in m_sQuotes, e.g. '")
				sQuote = _tcsinc(sVal);
				sEndQuote = _tcspbrk(sQuote, m_sQuotes);
			} 
			else 
			{
				sQuote = sVal;
				sEndQuote = _tcschr(sQuote, _T(' '));
			}

			if(sEndQuote == NULL) 
			{ 
				// no end quotes or terminating space, take the rest of the string to its end
				CString csVal(sQuote);
				if(!Key.IsEmpty()) 
				{ 
					m_valueMap.insert(CValsMap::value_type(Key, csVal));
				}
				break;
			} 
			else 
			{ 
				// end quote
				if(!Key.IsEmpty()) 
				{	
					CString csVal(sQuote, (int)(sEndQuote - sQuote));
					m_valueMap.insert(CValsMap::value_type(Key, csVal));
				}
				sCurrent = _tcsinc(sEndQuote);
				continue;
			}
		}
	}
	
	return (nArgs > 0);		//TRUE if arguments were found
}

CCmdLineParser::CValsMap::const_iterator CCmdLineParser::findKey(LPCTSTR sKey) const 
{
	CString s(sKey);
	s.MakeLower();
	return m_valueMap.find(s);
}

BOOL CCmdLineParser::HasKey(LPCTSTR sKey) const 
{
	CValsMap::const_iterator it = findKey(sKey);
	if(it == m_valueMap.end()) 
		return false;
	return true;
}


BOOL CCmdLineParser::HasVal(LPCTSTR sKey) const 
{
	CValsMap::const_iterator it = findKey(sKey);
	if(it == m_valueMap.end()) 
		return false;
	if(it->second.IsEmpty()) 
		return false;
	return true;
}

LPCTSTR CCmdLineParser::GetVal(LPCTSTR sKey) const 
{
	CValsMap::const_iterator it = findKey(sKey);
	if (it == m_valueMap.end()) 
		return 0;
	return LPCTSTR(it->second);
}

LONG CCmdLineParser::GetLongVal(LPCTSTR sKey) const
{
	CValsMap::const_iterator it = findKey(sKey);
	if (it == m_valueMap.end())
		return 0;
	return _tstol(LPCTSTR(it->second));
}


CCmdLineParser::ITERPOS CCmdLineParser::begin() const 
{
	return m_valueMap.begin();
}

CCmdLineParser::ITERPOS CCmdLineParser::getNext(ITERPOS& pos, CString& sKey, CString& sValue) const 
{
	if (m_valueMap.end() == pos)
	{
		sKey.Empty();
		return pos;
	} 
	else 
	{
		sKey = pos->first;
		sValue = pos->second;
		pos ++;
		return pos;
	}
}

BOOL CCmdLineParser::isLast(const ITERPOS& pos) const 
{
	return (pos == m_valueMap.end());
}
