// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseSVN

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

#include "stdafx.h"

#include "TestTempFile.h"
#include "PathDictionary.h"
#include "RootOutStream.h"
#include "RootInStream.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace LogCacheTests
{
    TEST_CLASS(PathDictonaryTests)
    {
    public:
        TEST_METHOD(EmptyTest)
        {
            CTestTempFile tmpFile;
            {
                CRootOutStream strm(tmpFile.GetFileName());
                LogCache::CPathDictionary dict;

                Assert::AreEqual((LogCache::index_t) 1, dict.size());

                // Save data to stream.
                strm << dict;
            }

            {
                CRootInStream strm(tmpFile.GetFileName());
                LogCache::CPathDictionary dict;

                // Load data to stream.
                strm >> dict;

                Assert::AreEqual((LogCache::index_t) 1, dict.size());
            }
        }

        TEST_METHOD(SimpleTest)
        {
            LogCache::index_t path1Idx;
            LogCache::index_t path2Idx;
            LogCache::index_t path3Idx;
            LogCache::index_t path4Idx;

            CTestTempFile tmpFile;
            {
                CRootOutStream strm(tmpFile.GetFileName());
                LogCache::CPathDictionary dict;

                LogCache::CDictionaryBasedPath path1(&dict, "/", false);
                path1Idx = path1.GetIndex();

                LogCache::CDictionaryBasedPath path2(&dict, "/trunk/test", false);
                path2Idx = path2.GetIndex();

                LogCache::CDictionaryBasedPath path3(&dict, "/branches", false);
                path3Idx = path3.GetIndex();

                LogCache::CDictionaryBasedPath path4(&dict, "/trunk", false);
                path4Idx = path4.GetIndex();

                // Save data to stream.
                strm << dict;
            }

            {
                CRootInStream strm(tmpFile.GetFileName());
                LogCache::CPathDictionary dict;

                // Load data to stream.
                strm >> dict;

                LogCache::CDictionaryBasedPath path1(&dict, path1Idx);
                Assert::AreEqual("/", path1.GetPath().c_str());

                LogCache::CDictionaryBasedPath path2(&dict, path2Idx);
                Assert::AreEqual("/trunk/test", path2.GetPath().c_str());

                LogCache::CDictionaryBasedPath path3(&dict, path3Idx);
                Assert::AreEqual("/branches", path3.GetPath().c_str());

                LogCache::CDictionaryBasedPath path4(&dict, path4Idx);
                Assert::AreEqual("/trunk", path4.GetPath().c_str());

                Assert::AreEqual((LogCache::index_t) 5, dict.size());
            }
        }

        TEST_METHOD(SingleByteCorruptionTest)
        {
            for (int i = 0; i < 266; i++)
            {
                CTestTempFile tmpFile;
                {
                    CRootOutStream strm(tmpFile.GetFileName());
                    LogCache::CPathDictionary dict;

                    LogCache::CDictionaryBasedPath path1(&dict, "/", false);
                    LogCache::CDictionaryBasedPath path2(&dict, "/trunk/test", false);
                    LogCache::CDictionaryBasedPath path3(&dict, "/branches", false);
                    LogCache::CDictionaryBasedPath path4(&dict, "/trunk", false);

                    // Save data to stream.
                    strm << dict;
                }

                // Modify single byte in file.
                {
                    std::fstream file;
                    file.open(tmpFile.GetFileName(), std::fstream::in | std::fstream::out | std::fstream::binary);
                    file.seekp(i, std::ios::beg);
                    char buf[1];
                    file.read(buf, 1);
                    buf[0] ^= 0x23;
                    file.seekp(i, std::ios::beg);
                    file.write(buf, 1);
                    file.close();
                }

                LogCache::CPathDictionary dict;
                try
                {
                    CRootInStream strm(tmpFile.GetFileName());

                    // Load data to stream.
                    strm >> dict;
                }
                catch(const std::exception & e)
                {
                    Logger::WriteMessage(e.what());
                    dict.Clear();
                }

                for (LogCache::index_t idx = 0; idx < dict.size(); idx++)
                {
                    LogCache::CDictionaryBasedPath dictPath(&dict, idx);
                    std::string path(dictPath.GetPath());
                }
            }
        }
    };
}
