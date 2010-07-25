// testCOM.js - javascript file
// test script for the SubWCRev COM/Automation-object

filesystem = new ActiveXObject("Scripting.FileSystemObject");

subwcrevObject1 = new ActiveXObject("SubWCRev.object");
subwcrevObject2 = new ActiveXObject("SubWCRev.object");
subwcrevObject3 = new ActiveXObject("SubWCRev.object");
subwcrevObject4 = new ActiveXObject("SubWCRev.object");

subwcrevObject1.GetWCInfo(filesystem.GetAbsolutePathName("."), 1, 1);
subwcrevObject2.GetWCInfo(filesystem.GetAbsolutePathName(".."), 1, 1);
subwcrevObject3.GetWCInfo(filesystem.GetAbsolutePathName("SubWCRev.cpp"), 1, 1);
subwcrevObject4.GetWCInfo(filesystem.GetAbsolutePathName("..\\.."), 1, 1);

wcInfoString1 = "Revision = " + subwcrevObject1.Revision + "\nMin Revision = " + subwcrevObject1.MinRev + "\nMax Revision = " + subwcrevObject1.MaxRev + "\nDate = " + subwcrevObject1.Date + "\nURL = " + subwcrevObject1.Url + "\nAuthor = " + subwcrevObject1.Author + "\nHasMods = " + subwcrevObject1.HasModifications + "\nIsSvnItem = " + subwcrevObject1.IsSvnItem + "\nNeedsLocking = " + subwcrevObject1.NeedsLocking + "\nIsLocked = " + subwcrevObject1.IsLocked + "\nLockCreationDate = " + subwcrevObject1.LockCreationDate + "\nLockOwner = " + subwcrevObject1.LockOwner + "\nLockComment = " + subwcrevObject1.LockComment;
wcInfoString2 = "Revision = " + subwcrevObject2.Revision + "\nMin Revision = " + subwcrevObject2.MinRev + "\nMax Revision = " + subwcrevObject2.MaxRev + "\nDate = " + subwcrevObject2.Date + "\nURL = " + subwcrevObject2.Url + "\nAuthor = " + subwcrevObject2.Author + "\nHasMods = " + subwcrevObject2.HasModifications + "\nIsSvnItem = " + subwcrevObject2.IsSvnItem + "\nNeedsLocking = " + subwcrevObject2.NeedsLocking + "\nIsLocked = " + subwcrevObject2.IsLocked + "\nLockCreationDate = " + subwcrevObject2.LockCreationDate + "\nLockOwner = " + subwcrevObject2.LockOwner + "\nLockComment = " + subwcrevObject2.LockComment;
wcInfoString3 = "Revision = " + subwcrevObject3.Revision + "\nMin Revision = " + subwcrevObject3.MinRev + "\nMax Revision = " + subwcrevObject3.MaxRev + "\nDate = " + subwcrevObject3.Date + "\nURL = " + subwcrevObject3.Url + "\nAuthor = " + subwcrevObject3.Author + "\nHasMods = " + subwcrevObject3.HasModifications + "\nIsSvnItem = " + subwcrevObject3.IsSvnItem + "\nNeedsLocking = " + subwcrevObject3.NeedsLocking + "\nIsLocked = " + subwcrevObject3.IsLocked + "\nLockCreationDate = " + subwcrevObject3.LockCreationDate + "\nLockOwner = " + subwcrevObject3.LockOwner + "\nLockComment = " + subwcrevObject3.LockComment;
wcInfoString4 = "Revision = " + subwcrevObject4.Revision + "\nMin Revision = " + subwcrevObject4.MinRev + "\nMax Revision = " + subwcrevObject4.MaxRev + "\nDate = " + subwcrevObject4.Date + "\nURL = " + subwcrevObject4.Url + "\nAuthor = " + subwcrevObject4.Author + "\nHasMods = " + subwcrevObject4.HasModifications + "\nIsSvnItem = " + subwcrevObject4.IsSvnItem + "\nNeedsLocking = " + subwcrevObject4.NeedsLocking + "\nIsLocked = " + subwcrevObject4.IsLocked + "\nLockCreationDate = " + subwcrevObject4.LockCreationDate + "\nLockOwner = " + subwcrevObject4.LockOwner + "\nLockComment = " + subwcrevObject4.LockComment;

WScript.Echo(wcInfoString1);
WScript.Echo(wcInfoString2);
WScript.Echo(wcInfoString3);
WScript.Echo(wcInfoString4);

