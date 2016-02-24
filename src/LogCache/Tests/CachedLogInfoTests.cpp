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
                LogCache::CCachedLogInfo logInfo;

                logInfo.Insert(1, "author", "comment1", 1111);
                logInfo.AddChange(LogCache::CCachedLogInfo::TChangeAction::ACTION_ADDED,
                                  LogCache::node_dir, "/trunk", "",
                                  LogCache::NO_REVISION, FALSE, FALSE);

                logInfo.Save(tmpFile.GetFileName());
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

        TEST_METHOD(LoadPreCookedTest)
        {
            LogCache::CCachedLogInfo logInfo(GetTestDataDir() + L"src-LogCache-all");

            logInfo.Load(0);

            Assert::AreEqual(false, logInfo.IsEmpty());

            ValidateCachedLogInfo(logInfo);
        }

    private:
        void ValidateCachedLogInfo(const LogCache::CCachedLogInfo & logInfo)
        {
            const LogCache::CRevisionIndex & index = logInfo.GetRevisions();
            const LogCache::CRevisionInfoContainer & revInfo = logInfo.GetLogInfo();

            LogCache::revision_t firstRev = index.GetFirstCachedRevision();
            LogCache::revision_t lastRev = index.GetLastCachedRevision();

            for (LogCache::revision_t rev = firstRev; rev < lastRev; rev++)
            {
                LogCache::index_t idx = index[rev];
                if (idx != LogCache::NO_INDEX)
                {
                    std::string author(revInfo.GetAuthor(idx));
                    std::string comment(revInfo.GetComment(idx));
                    char presenceFlags = revInfo.GetPresenceFlags(idx);
                    __time64_t timeStamp = revInfo.GetTimeStamp(idx);

                    LogCache::CRevisionInfoContainer::CChangesIterator changeIt = revInfo.GetChangesBegin(idx);
                    LogCache::CRevisionInfoContainer::CChangesIterator changesEnd = revInfo.GetChangesEnd(idx);
                    for (; changeIt != changesEnd; ++changeIt)
                    {
                        LogCache::CRevisionInfoContainer::TChangeAction action = changeIt->GetAction();
                        std::string changePath(changeIt->GetPath().GetPath());
                        LogCache::node_kind_t nodeKind = changeIt->GetPathType();
                        unsigned char textModifies = changeIt->GetTextModifies();
                        unsigned char propsModifies = changeIt->GetPropsModifies();

                        if (changeIt->HasFromPath())
                        {
                            LogCache::revision_t fromRev(changeIt->GetFromRevision());
                            std::string fromPath(changeIt->GetFromPath().GetPath());
                        }
                    }
                }
            }
        }

        static std::wstring GetTestDataDir()
        {
            WCHAR moduleFileName[MAX_PATH] = { 0 };
            HMODULE hModule = NULL;
            GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCWSTR)GetTestDataDir,
                &hModule);
            ::GetModuleFileName(hModule, moduleFileName, _countof(moduleFileName));

            std::wstring moduleDir(moduleFileName);

            return moduleDir.substr(0, moduleDir.rfind(L'\\')) + L"\\..\\..\\..\\src\\LogCache\\Tests\\TestData\\";
        }
    };
}
