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
#include "./Containers/StringDictonary.h"
#include "./Containers/CachedLogInfo.h"
#include "./Access/XMLLogReader.h"
#include "./Access/XMLLogWriter.h"
#include "./Streams/CompositeInStream.h"
#include "./Streams/CompositeOutStream.h"
#include "./Streams/HuffmanEncoder.h"
#include "./Streams/HuffmanDecoder.h"
#include "HighResClock.h"
#include "./Access/CopyFollowingLogIterator.h"

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
	logInfo.Load(0);
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
	logInfo.Load(0);

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
	logInfo.Load(0);

	CCachedLogInfo copied (path + L".stream");
	copied.Load(0);

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

void BenchmarkHuffman()
{
	enum {DATA_SIZE = 0x8000, RUN_COUNT = 0x8000, _1MB = 0x100000};

	// encoder speed

	BYTE data [DATA_SIZE];
	memset (data, 'x', DATA_SIZE);
	for (BYTE* target = data; target+10 < data + DATA_SIZE; target += 10)
		memcpy (target, "0123456789", 10);

	CHighResClock clock1;
	for (int i = 0; i < RUN_COUNT; ++i)
	{
		delete CHuffmanEncoder().Encode (data, DATA_SIZE).first;
	}
	clock1.Stop();

	// decoder speed

	std::pair<BYTE*, DWORD> compressed = CHuffmanEncoder().Encode (data, DATA_SIZE);

	CHighResClock clock2;
	for (int i = 0; i < RUN_COUNT; ++i)
	{
		CHuffmanDecoder decoder;

		const BYTE* input = compressed.first;
		BYTE* output = data;
		decoder.Decode (input, output);
	}
	clock2.Stop();

	delete compressed.first;

	CStringA s;
	s.Format ("compressed %d MB in %5.3f secs = %5.2f MB/sec (%2.1f ticks / byte)\n"
			  "decompressed %d / %d MB in %5.3f secs = %5.2f / %5.2f MB/sec\n"
			 , RUN_COUNT * DATA_SIZE / _1MB
			 , clock1.GetMusecsTaken() / 1e+06
			 , (RUN_COUNT * DATA_SIZE * 1e+06) / clock1.GetMusecsTaken() / _1MB
			 , (2.4e+03 * clock1.GetMusecsTaken()) / (RUN_COUNT * DATA_SIZE)
			 , RUN_COUNT * compressed.second / _1MB
			 , RUN_COUNT * DATA_SIZE / _1MB
			 , clock2.GetMusecsTaken() / 1e+06
			 , (RUN_COUNT * compressed.second * 1e+06) / clock2.GetMusecsTaken() / _1MB
			 , (RUN_COUNT * DATA_SIZE * 1e+06) / clock2.GetMusecsTaken() / _1MB);

	printf (s);
}

int _tmain(int argc, _TCHAR* argv[])
{
/*	TestXMLIO();
	TestIteration();
	TestUpdate();
*/
	BenchmarkHuffman();

	return 0;
}

