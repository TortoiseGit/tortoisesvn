// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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
// LogCache.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "RootInStream.h"
#include "RootOutStream.h"
#include "StringDictonary.h"
#include "CachedLogInfo.h"
#include "XMLLogReader.h"
#include "XMLLogWriter.h"

void ReadStream (const std::wstring& fileName)
{
	CRootInStream stream (fileName);

	CStringDictionary dictionary;

	stream >> dictionary;
}

void WriteStream (const std::wstring& fileName)
{
	CRootOutStream stream (fileName);

	CStringDictionary dictionary;
	dictionary.Insert ("test");
	dictionary.Insert ("hugo");
	dictionary.Insert ("otto");

	stream << dictionary;
}

void TestXMLIO()
{
//	CCachedLogInfo logInfo (L"C:\\temp\\kde.stream");
	CCachedLogInfo logInfo (L"C:\\temp\\tsvntrunk.stream");
//	logInfo.Load();
//	logInfo.Clear();

//	CXMLLogReader::LoadFromXML (L"C:\\temp\\kde.log.xml", logInfo);
	CXMLLogReader::LoadFromXML (L"C:\\temp\\tsvntrunk.xml", logInfo);
	CXMLLogWriter::SaveToXML (L"C:\\temp\\tsvntrunk.xml.out", logInfo);
	logInfo.Save();
}

int _tmain(int argc, _TCHAR* argv[])
{
	WriteStream (L"C:\\temp\\test.stream");
	ReadStream (L"C:\\temp\\test.stream");

	TestXMLIO();

	return 0;
}

