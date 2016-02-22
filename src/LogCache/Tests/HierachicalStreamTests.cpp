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
#include "RootOutStream.h"
#include "RootInStream.h"
#include "BLOBOutStream.h"
#include "BLOBInStream.h"

#include <fstream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace LogCacheTests
{
    TEST_CLASS(HierarchicalStreamTests)
    {
    public:
        TEST_METHOD(SimpleTest)
        {
            CTestTempFile tmpFile;
            {
                CRootOutStream strm(tmpFile.GetFileName());

                CBLOBOutStream *subStream;
                subStream = strm.OpenSubStream<CBLOBOutStream>(1);

                subStream->Add((BYTE*) "1234567890", 10);

                subStream = strm.OpenSubStream<CBLOBOutStream>(2);

                std::string data2(1024, 'A');
                subStream->Add((BYTE*) &data2.front(), data2.length());

                // Save data to stream.
                strm.AutoClose();
            }

            {
                CRootInStream strm(tmpFile.GetFileName());

                CBLOBInStream *subStream;
                subStream  = strm.GetSubStream<CBLOBInStream>(1);

                std::string actual;
                actual = std::string((const char *) subStream->GetData(), subStream->GetSize());

                Assert::AreEqual("1234567890", actual.c_str());

                subStream = strm.GetSubStream<CBLOBInStream>(2);
                Assert::AreEqual((size_t)1024, subStream->GetSize());
            }
        }
    };
}
