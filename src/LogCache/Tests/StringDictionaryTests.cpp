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
#include "StringDictonary.h"
#include "RootOutStream.h"
#include "RootInStream.h"

#include <fstream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace LogCacheTests
{
    TEST_CLASS(StringDictonaryTests)
    {
    public:
        TEST_METHOD(EmptyTest)
        {
            CTestTempFile tmpFile;
            {
                CRootOutStream strm(tmpFile.GetFileName());
                LogCache::CStringDictionary dict;

                Assert::AreEqual((LogCache::index_t)1, dict.size());

                // Save data to stream.
                strm << dict;
            }

            {
                CRootInStream strm(tmpFile.GetFileName());
                LogCache::CStringDictionary dict;

                // Load data to stream.
                strm >> dict;

                Assert::AreEqual((LogCache::index_t) 1, dict.size());
            }
        }

        TEST_METHOD(SimpleTest)
        {
            CTestTempFile tmpFile;
            {
                CRootOutStream strm(tmpFile.GetFileName());
                LogCache::CStringDictionary dict;

                Assert::AreEqual((LogCache::index_t) 1, dict.AutoInsert("dict1"));
                Assert::AreEqual((LogCache::index_t) 2, dict.AutoInsert("dict2"));
                Assert::AreEqual((LogCache::index_t) 3, dict.AutoInsert("dict3"));
                Assert::AreEqual((LogCache::index_t) 4, dict.AutoInsert("dict4"));

                Assert::AreEqual((LogCache::index_t) 2, dict.AutoInsert("dict2"));
                Assert::AreEqual((LogCache::index_t) 3, dict.AutoInsert("dict3"));

                Assert::AreEqual((LogCache::index_t) 5, dict.size());

                // Save data to stream.
                strm << dict;
            }

            {
                CRootInStream strm(tmpFile.GetFileName());
                LogCache::CStringDictionary dict;

                // Load data to stream.
                strm >> dict;

                Assert::AreEqual((LogCache::index_t) 1, dict.Find("dict1"));
                Assert::AreEqual((LogCache::index_t) 2, dict.Find("dict2"));
                Assert::AreEqual((LogCache::index_t) 3, dict.Find("dict3"));
                Assert::AreEqual((LogCache::index_t) 4, dict.Find("dict4"));

                Assert::AreEqual((LogCache::index_t) 5, dict.size());
            }
        }

        TEST_METHOD(SingleByteCorruptionTest)
        {
            for (int i = 0; i < 125; i++)
            {
                CTestTempFile tmpFile;
                {
                    CRootOutStream strm(tmpFile.GetFileName());
                    LogCache::CStringDictionary dict;

                    Assert::AreEqual((LogCache::index_t) 1, dict.AutoInsert("dict1"));
                    Assert::AreEqual((LogCache::index_t) 2, dict.AutoInsert("dict2"));
                    Assert::AreEqual((LogCache::index_t) 3, dict.AutoInsert("dict3"));
                    Assert::AreEqual((LogCache::index_t) 4, dict.AutoInsert("dict4"));

                    Assert::AreEqual((LogCache::index_t) 5, dict.size());

                    // Save data to stream.
                    strm << dict;
                }

                // Corrupt single byte in file.
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

                try
                {
                    CRootInStream strm(tmpFile.GetFileName());
                    LogCache::CStringDictionary dict;

                    // Load data from stream.
                    strm >> dict;

                    LogCache::index_t idx;
                    idx = dict.Find("dict1");
                    idx = dict.Find("dict2");
                    idx = dict.Find("dict3");
                    idx = dict.Find("dict4");
                }
                catch(const std::exception & e)
                {
                    // Exceptions are excepcted when reading data from corrupted storage.
                    Logger::WriteMessage(e.what());
                }
            }
        }
    };
}
