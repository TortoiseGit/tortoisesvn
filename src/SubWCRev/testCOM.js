// test script for the SubWCRev COM/Automation-object

objScript = new ActiveXObject("Scripting.FileSystemObject");

objScript1 = new ActiveXObject("SubWCRev.object.1");
objScript2 = new ActiveXObject("SubWCRev.object.1");
objScript3 = new ActiveXObject("SubWCRev.object.1");

objScript1.GetWCInfo(objScript.GetAbsolutePathName("."), 1, 1);
objScript2.GetWCInfo(objScript.GetAbsolutePathName(".."), 1, 1);
objScript3.GetWCInfo(objScript.GetAbsolutePathName("SubWCRev.cpp"), 1, 1);

sInfo1 = "Revision = " + objScript1.Revision + "\nMin Revision = " + objScript1.MinRev + "\nMax Revision = " + objScript1.MaxRev + "\nDate = " + objScript1.Date + "\nURL = " + objScript1.Url + "\nAuthor = " + objScript1.Author + "\nHasMods = " + objScript1.HasModifications;
sInfo2 = "Revision = " + objScript2.Revision + "\nMin Revision = " + objScript2.MinRev + "\nMax Revision = " + objScript2.MaxRev + "\nDate = " + objScript2.Date + "\nURL = " + objScript2.Url + "\nAuthor = " + objScript2.Author + "\nHasMods = " + objScript2.HasModifications;
sInfo3 = "Revision = " + objScript3.Revision + "\nMin Revision = " + objScript3.MinRev + "\nMax Revision = " + objScript3.MaxRev + "\nDate = " + objScript3.Date + "\nURL = " + objScript3.Url + "\nAuthor = " + objScript3.Author + "\nHasMods = " + objScript3.HasModifications;

WScript.Echo(sInfo1);
WScript.Echo(sInfo2);
WScript.Echo(sInfo3);

