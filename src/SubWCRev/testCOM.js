// test script for the SubWCRev COM/Automation-object

objScript = new ActiveXObject("Scripting.FileSystemObject");

objScript1 = new ActiveXObject("SubWCRev.object.1");
objScript2 = new ActiveXObject("SubWCRev.object.1");

objScript1.GetWCInfo(objScript.GetAbsolutePathName("."), 1, 1);
objScript2.GetWCInfo(objScript.GetAbsolutePathName(".."), 1, 1);

sInfo1 = "Revision = " + objScript1.Revision + "\nMin Revision = " + objScript1.MinRev + "\nMax Revision = " + objScript1.MaxRev + "\nDate = " + objScript1.Date + "\nURL = " + objScript1.Url + "\nHasMods = " + objScript1.HasModifications;
sInfo2 = "Revision = " + objScript2.Revision + "\nMin Revision = " + objScript2.MinRev + "\nMax Revision = " + objScript2.MaxRev + "\nDate = " + objScript2.Date + "\nURL = " + objScript2.Url + "\nHasMods = " + objScript2.HasModifications;

WScript.Echo(sInfo1);
WScript.Echo(sInfo2);

