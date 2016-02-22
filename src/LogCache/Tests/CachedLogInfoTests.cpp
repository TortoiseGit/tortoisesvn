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
#include "CachedLogInfo.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace LogCacheTests
{
    TEST_CLASS(CachedLogInfoTests)
    {
    public:
        TEST_METHOD(SimpleTest)
        {
            CTestTempFile tmpFile;
            {
                LogCache::CCachedLogInfo logInfo(tmpFile.GetFileName());

                logInfo.Insert(1, "author", "comment1", 1111);
                logInfo.AddChange(LogCache::CCachedLogInfo::TChangeAction::ACTION_ADDED,
                                  LogCache::node_dir, "/trunk", "",
                                  LogCache::NO_REVISION, FALSE, FALSE);

                logInfo.Save();
            }

            {
                LogCache::CCachedLogInfo logInfo(tmpFile.GetFileName());
                logInfo.Load(0);

                LogCache::index_t idx;

                // Test access to non-existing revision.
                idx = logInfo.GetRevisions()[105];
                Assert::AreEqual((LogCache::index_t) LogCache::NO_INDEX, idx);

                // Access to revision that we stored in DB
                idx = logInfo.GetRevisions()[1];
                Assert::AreNotEqual((LogCache::index_t) LogCache::NO_INDEX, idx);

                Assert::AreEqual("comment1", logInfo.GetLogInfo().GetComment(idx).c_str());
                Assert::AreEqual("author", logInfo.GetLogInfo().GetAuthor(idx));
            }
        }
    };
}
