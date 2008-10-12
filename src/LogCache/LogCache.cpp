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
#include "./Streams/RootInStream.h"
#include "./Streams/RootOutStream.h"
#include "StringDictonary.h"
#include "CachedLogInfo.h"
#include "XMLLogReader.h"
#include "XMLLogWriter.h"
#include "./Streams/CompositeInStream.h"
#include "./Streams/CompositeOutStream.h"
#include "HighResClock.h"
#include "CopyFollowingLogIterator.h"

using namespace LogCache;

#ifdef _DEBUG
std::wstring path = L"E:\\temp\\tsvn";
#else
std::wstring path = L"E:\\temp\\kde";
#endif

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
	CCachedLogInfo logInfo (path + L".stream");

	CHighResClock clock1;
	CXMLLogReader::LoadFromXML (path + L".log.xml", logInfo);
	clock1.Stop();

	logInfo.Save();
	logInfo.Clear();

	CHighResClock clock2;
	logInfo.Load();
	clock2.Stop();

	logInfo.Save();
	Sleep(2000);

	CHighResClock clock4;
	logInfo.Save();
	clock4.Stop();

	CHighResClock clock3;
	CXMLLogWriter::SaveToXML (path + L".xml.out", logInfo, true);
	clock3.Stop();

	CStringA s;
	s.Format ("\nimport: %5.4f  load: %5.4f  export: %5.4f  save: %5.4f\n"
			 , clock1.GetMusecsTaken() / 1e+06
			 , clock2.GetMusecsTaken() / 1e+06
			 , clock3.GetMusecsTaken() / 1e+06
			 , clock4.GetMusecsTaken() / 1e+06);

	printf (s);
}

void TestIteration()
{
	CCachedLogInfo logInfo (path + L".stream");
	logInfo.Load();

	revision_t head = logInfo.GetRevisions().GetLastRevision()-1;

	CHighResClock clock1;
	CDictionaryBasedTempPath rootPath (&logInfo.GetLogInfo().GetPaths(), "");
	CCopyFollowingLogIterator rootIterator (&logInfo, head, rootPath);

	int revisionsForRoot = 0;
	while (!rootIterator.EndOfPath())
	{
		++revisionsForRoot;
		rootIterator.Advance();
	}
	clock1.Stop();

	CHighResClock clock2;
	CDictionaryBasedTempPath tagsPath (&logInfo.GetLogInfo().GetPaths(), "/tags");
	CCopyFollowingLogIterator tagsIterator (&logInfo, head, tagsPath);
	int revisionsForTags = 0;
	while (!tagsIterator.EndOfPath())
	{
		++revisionsForTags;
		tagsIterator.Advance();
	}
	clock2.Stop();

	CStringA s;
	s.Format ("found %d revisions on / in %5.4f secs\n"
			  "found %d revisions on /tags in %5.4f secs\n"
			 , revisionsForRoot
			 , clock1.GetMusecsTaken() / 1e+06
			 , revisionsForTags
			 , clock2.GetMusecsTaken() / 1e+06);

	printf (s);
}

void TestUpdate()
{
	CCachedLogInfo logInfo (path + L".stream");
	logInfo.Load();

	CCachedLogInfo copied (path + L".stream");
	copied.Load();

	CCachedLogInfo newData;
	newData.Insert (1234, "dummy", "", 0);

	CHighResClock clock1;
	logInfo.Update (copied);
	clock1.Stop();

	CHighResClock clock2;
	logInfo.Update (newData);
	clock2.Stop();

	CHighResClock clock3;
    logInfo.Update (newData, CRevisionInfoContainer::HAS_AUTHOR);
	clock3.Stop();

	CStringA s;
	s.Format ("updated all %d revisions in %5.4f secs\n"
			  "updated a single revision in %5.4f secs\n"
			  "updated an author in %5.4f secs\n"
			 , copied.GetLogInfo().size()
			 , clock1.GetMusecsTaken() / 1e+06
			 , clock2.GetMusecsTaken() / 1e+06
			 , clock3.GetMusecsTaken() / 1e+06);

	printf (s);
}

int _tmain(int argc, _TCHAR* argv[])
{
	TestXMLIO();
	TestIteration();
	TestUpdate();

	return 0;
}

