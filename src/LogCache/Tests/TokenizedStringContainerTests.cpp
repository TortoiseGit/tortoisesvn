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
#include "TokenizedStringContainer.h"
#include "RootOutStream.h"
#include "RootInStream.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace LogCacheTests
{
    TEST_CLASS(TokenizedStringContainerTests)
    {
    public:
        TEST_METHOD(SimpleTest)
        {
            CTestTempFile tmpFile;
            {
                CRootOutStream strm(tmpFile.GetFileName());
                LogCache::CTokenizedStringContainer tsc;

                Assert::AreEqual((LogCache::index_t) 0, tsc.Insert("This is test string."));
                Assert::AreEqual((LogCache::index_t) 1, tsc.Insert("Another test string."));
                Assert::AreEqual((LogCache::index_t) 2, tsc.Insert("LogMessage"));
                Assert::AreEqual((LogCache::index_t) 3, tsc.Insert(""));

                Assert::AreEqual((LogCache::index_t) 4, tsc.size());

                tsc.Compress();
                // Save data to stream.
                strm << tsc;
            }

            {
                CRootInStream strm(tmpFile.GetFileName());
                LogCache::CTokenizedStringContainer tsc;

                // Load data to stream.
                strm >> tsc;

                Assert::AreEqual("This is test string.", tsc[0].c_str());
                Assert::AreEqual("Another test string.", tsc[1].c_str());
                Assert::AreEqual("LogMessage", tsc[2].c_str());
                Assert::AreEqual("", tsc[3].c_str());

                Assert::AreEqual((LogCache::index_t) 4, tsc.size());
            }
        }

        TEST_METHOD(SingleByteCorruptionTest)
        {
            for (int i = 0; i < 368; i++)
            {
                CTestTempFile tmpFile;
                {
                    CRootOutStream strm(tmpFile.GetFileName());
                    LogCache::CTokenizedStringContainer tsc;

                    Assert::AreEqual((LogCache::index_t) 0, tsc.Insert("This is test string."));
                    Assert::AreEqual((LogCache::index_t) 1, tsc.Insert("Another test string."));
                    Assert::AreEqual((LogCache::index_t) 2, tsc.Insert("LogMessage"));
                    Assert::AreEqual((LogCache::index_t) 3, tsc.Insert(""));

                    Assert::AreEqual((LogCache::index_t) 4, tsc.size());

                    tsc.Compress();
                    // Save data to stream.
                    strm << tsc;
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

                LogCache::CTokenizedStringContainer tsc;
                try
                {
                    CRootInStream strm(tmpFile.GetFileName());

                    // Load data to stream.
                    strm >> tsc;
                }
                catch (const std::exception & e)
                {
                    // Exceptions are excpected when reading data from corrupted storage.
                    Logger::WriteMessage(e.what());
                    tsc.Clear();
                }

                // Access all strings in the container. We cannot check their content
                // since storage is corrupted.
                for (LogCache::index_t idx = 0; idx < tsc.size(); idx++)
                {
                    std::string str(tsc[idx]);
                }
            }
        }
    };
}
