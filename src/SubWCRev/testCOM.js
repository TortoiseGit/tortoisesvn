// testCOM.js - javascript file
// test script for the SubWCRev COM/Automation-object

filesystem = new ActiveXObject("Scripting.FileSystemObject");

subwcrevObject1 = new ActiveXObject("SubWCRev.object");
subwcrevObject2 = new ActiveXObject("SubWCRev.object");
subwcrevObject3 = new ActiveXObject("SubWCRev.object");
subwcrevObject4 = new ActiveXObject("SubWCRev.object");

subwcrevObject2_1 = new ActiveXObject("SubWCRev.object");
subwcrevObject2_2 = new ActiveXObject("SubWCRev.object");
subwcrevObject2_3 = new ActiveXObject("SubWCRev.object");
subwcrevObject2_4 = new ActiveXObject("SubWCRev.object");

subwcrevObject1.GetWCInfo(filesystem.GetAbsolutePathName("."), 1, 1);
subwcrevObject2.GetWCInfo(filesystem.GetAbsolutePathName(".."), 1, 1);
subwcrevObject3.GetWCInfo(filesystem.GetAbsolutePathName("SubWCRev.cpp"), 1, 1);
subwcrevObject4.GetWCInfo(filesystem.GetAbsolutePathName("..\\.."), 1, 1);

subwcrevObject2_1.GetWCInfo2(filesystem.GetAbsolutePathName("."), 1, 1, 1);
subwcrevObject2_2.GetWCInfo2(filesystem.GetAbsolutePathName(".."), 1, 1, 1);
subwcrevObject2_3.GetWCInfo2(filesystem.GetAbsolutePathName("SubWCRev.cpp"), 1, 1, 1);
subwcrevObject2_4.GetWCInfo2(filesystem.GetAbsolutePathName("..\\.."), 1, 1, 1);

wcInfoString1 = "Revision = " + subwcrevObject1.Revision + "\nMin Revision = " + subwcrevObject1.MinRev + "\nMax Revision = " + subwcrevObject1.MaxRev + "\nDate = " + subwcrevObject1.Date + "\nURL = " + subwcrevObject1.Url + "\nAuthor = " + subwcrevObject1.Author + "\nHasMods = " + subwcrevObject1.HasModifications + "\nHasMixed = " + subwcrevObject1.HasMixedRevisions + "\nExtAllFixed = " + subwcrevObject1.HaveExternalsAllFixedRevision + "\nIsTagged = " + subwcrevObject1.IsWcTagged + "\nIsSvnItem = " + subwcrevObject1.IsSvnItem + "\nNeedsLocking = " + subwcrevObject1.NeedsLocking + "\nIsLocked = " + subwcrevObject1.IsLocked + "\nLockCreationDate = " + subwcrevObject1.LockCreationDate + "\nLockOwner = " + subwcrevObject1.LockOwner + "\nLockComment = " + subwcrevObject1.LockComment;
wcInfoString2 = "Revision = " + subwcrevObject2.Revision + "\nMin Revision = " + subwcrevObject2.MinRev + "\nMax Revision = " + subwcrevObject2.MaxRev + "\nDate = " + subwcrevObject2.Date + "\nURL = " + subwcrevObject2.Url + "\nAuthor = " + subwcrevObject2.Author + "\nHasMods = " + subwcrevObject2.HasModifications + "\nHasMixed = " + subwcrevObject2.HasMixedRevisions + "\nExtAllFixed = " + subwcrevObject2.HaveExternalsAllFixedRevision + "\nIsTagged = " + subwcrevObject2.IsWcTagged + "\nIsSvnItem = " + subwcrevObject2.IsSvnItem + "\nNeedsLocking = " + subwcrevObject2.NeedsLocking + "\nIsLocked = " + subwcrevObject2.IsLocked + "\nLockCreationDate = " + subwcrevObject2.LockCreationDate + "\nLockOwner = " + subwcrevObject2.LockOwner + "\nLockComment = " + subwcrevObject2.LockComment;
wcInfoString3 = "Revision = " + subwcrevObject3.Revision + "\nMin Revision = " + subwcrevObject3.MinRev + "\nMax Revision = " + subwcrevObject3.MaxRev + "\nDate = " + subwcrevObject3.Date + "\nURL = " + subwcrevObject3.Url + "\nAuthor = " + subwcrevObject3.Author + "\nHasMods = " + subwcrevObject3.HasModifications + "\nHasMixed = " + subwcrevObject3.HasMixedRevisions + "\nExtAllFixed = " + subwcrevObject3.HaveExternalsAllFixedRevision + "\nIsTagged = " + subwcrevObject3.IsWcTagged + "\nIsSvnItem = " + subwcrevObject3.IsSvnItem + "\nNeedsLocking = " + subwcrevObject3.NeedsLocking + "\nIsLocked = " + subwcrevObject3.IsLocked + "\nLockCreationDate = " + subwcrevObject3.LockCreationDate + "\nLockOwner = " + subwcrevObject3.LockOwner + "\nLockComment = " + subwcrevObject3.LockComment;
wcInfoString4 = "Revision = " + subwcrevObject4.Revision + "\nMin Revision = " + subwcrevObject4.MinRev + "\nMax Revision = " + subwcrevObject4.MaxRev + "\nDate = " + subwcrevObject4.Date + "\nURL = " + subwcrevObject4.Url + "\nAuthor = " + subwcrevObject4.Author + "\nHasMods = " + subwcrevObject4.HasModifications + "\nHasMixed = " + subwcrevObject4.HasMixedRevisions + "\nExtAllFixed = " + subwcrevObject4.HaveExternalsAllFixedRevision + "\nIsTagged = " + subwcrevObject4.IsWcTagged + "\nIsSvnItem = " + subwcrevObject4.IsSvnItem + "\nNeedsLocking = " + subwcrevObject4.NeedsLocking + "\nIsLocked = " + subwcrevObject4.IsLocked + "\nLockCreationDate = " + subwcrevObject4.LockCreationDate + "\nLockOwner = " + subwcrevObject4.LockOwner + "\nLockComment = " + subwcrevObject4.LockComment;

WScript.Echo(wcInfoString1);
WScript.Echo(wcInfoString2);
WScript.Echo(wcInfoString3);
WScript.Echo(wcInfoString4);

wcInfoString1 = "Revision = " + subwcrevObject2_1.Revision + "\nMin Revision = " + subwcrevObject2_1.MinRev + "\nMax Revision = " + subwcrevObject2_1.MaxRev + "\nDate = " + subwcrevObject2_1.Date + "\nURL = " + subwcrevObject2_1.Url + "\nAuthor = " + subwcrevObject2_1.Author + "\nHasMods = " + subwcrevObject2_1.HasModifications + "\nHasMixed = " + subwcrevObject2_1.HasMixedRevisions + "\nExtAllFixed = " + subwcrevObject2_1.HaveExternalsAllFixedRevision + "\nIsTagged = " + subwcrevObject2_1.IsWcTagged + "\nIsSvnItem = " + subwcrevObject2_1.IsSvnItem + "\nNeedsLocking = " + subwcrevObject2_1.NeedsLocking + "\nIsLocked = " + subwcrevObject2_1.IsLocked + "\nLockCreationDate = " + subwcrevObject2_1.LockCreationDate + "\nLockOwner = " + subwcrevObject2_1.LockOwner + "\nLockComment = " + subwcrevObject2_1.LockComment;
wcInfoString2 = "Revision = " + subwcrevObject2_2.Revision + "\nMin Revision = " + subwcrevObject2_2.MinRev + "\nMax Revision = " + subwcrevObject2_2.MaxRev + "\nDate = " + subwcrevObject2_2.Date + "\nURL = " + subwcrevObject2_2.Url + "\nAuthor = " + subwcrevObject2_2.Author + "\nHasMods = " + subwcrevObject2_2.HasModifications + "\nHasMixed = " + subwcrevObject2_2.HasMixedRevisions + "\nExtAllFixed = " + subwcrevObject2_2.HaveExternalsAllFixedRevision + "\nIsTagged = " + subwcrevObject2_2.IsWcTagged + "\nIsSvnItem = " + subwcrevObject2_2.IsSvnItem + "\nNeedsLocking = " + subwcrevObject2_2.NeedsLocking + "\nIsLocked = " + subwcrevObject2_2.IsLocked + "\nLockCreationDate = " + subwcrevObject2_2.LockCreationDate + "\nLockOwner = " + subwcrevObject2_2.LockOwner + "\nLockComment = " + subwcrevObject2_2.LockComment;
wcInfoString3 = "Revision = " + subwcrevObject2_3.Revision + "\nMin Revision = " + subwcrevObject2_3.MinRev + "\nMax Revision = " + subwcrevObject2_3.MaxRev + "\nDate = " + subwcrevObject2_3.Date + "\nURL = " + subwcrevObject2_3.Url + "\nAuthor = " + subwcrevObject2_3.Author + "\nHasMods = " + subwcrevObject2_3.HasModifications + "\nHasMixed = " + subwcrevObject2_3.HasMixedRevisions + "\nExtAllFixed = " + subwcrevObject2_3.HaveExternalsAllFixedRevision + "\nIsTagged = " + subwcrevObject2_3.IsWcTagged + "\nIsSvnItem = " + subwcrevObject2_3.IsSvnItem + "\nNeedsLocking = " + subwcrevObject2_3.NeedsLocking + "\nIsLocked = " + subwcrevObject2_3.IsLocked + "\nLockCreationDate = " + subwcrevObject2_3.LockCreationDate + "\nLockOwner = " + subwcrevObject2_3.LockOwner + "\nLockComment = " + subwcrevObject2_3.LockComment;
wcInfoString4 = "Revision = " + subwcrevObject2_4.Revision + "\nMin Revision = " + subwcrevObject2_4.MinRev + "\nMax Revision = " + subwcrevObject2_4.MaxRev + "\nDate = " + subwcrevObject2_4.Date + "\nURL = " + subwcrevObject2_4.Url + "\nAuthor = " + subwcrevObject2_4.Author + "\nHasMods = " + subwcrevObject2_4.HasModifications + "\nHasMixed = " + subwcrevObject2_4.HasMixedRevisions + "\nExtAllFixed = " + subwcrevObject2_4.HaveExternalsAllFixedRevision + "\nIsTagged = " + subwcrevObject2_4.IsWcTagged + "\nIsSvnItem = " + subwcrevObject2_4.IsSvnItem + "\nNeedsLocking = " + subwcrevObject2_4.NeedsLocking + "\nIsLocked = " + subwcrevObject2_4.IsLocked + "\nLockCreationDate = " + subwcrevObject2_4.LockCreationDate + "\nLockOwner = " + subwcrevObject2_4.LockOwner + "\nLockComment = " + subwcrevObject2_4.LockComment;

WScript.Echo(wcInfoString1);
WScript.Echo(wcInfoString2);
WScript.Echo(wcInfoString3);
WScript.Echo(wcInfoString4);
