// testCOM.js - javascript file
// test script for the SubWCRev COM/Automation-object

filesystem = new ActiveXObject("Scripting.FileSystemObject");

subwcrevObject1 = new ActiveXObject("SubWCRev.object");
subwcrevObject2 = new ActiveXObject("SubWCRev.object");
subwcrevObject3 = new ActiveXObject("SubWCRev.object");

subwcrevObject1.GetWCInfo(filesystem.GetAbsolutePathName("."), 1, 1);
subwcrevObject2.GetWCInfo(filesystem.GetAbsolutePathName(".."), 1, 1);
subwcrevObject3.GetWCInfo(filesystem.GetAbsolutePathName("SubWCRev.cpp"), 1, 1);

wcInfoString1 = "Revision = " + subwcrevObject1.Revision + "\nMin Revision = " + subwcrevObject1.MinRev + "\nMax Revision = " + subwcrevObject1.MaxRev + "\nDate = " + subwcrevObject1.Date + "\nURL = " + subwcrevObject1.Url + "\nAuthor = " + subwcrevObject1.Author + "\nHasMods = " + subwcrevObject1.HasModifications;
wcInfoString2 = "Revision = " + subwcrevObject2.Revision + "\nMin Revision = " + subwcrevObject2.MinRev + "\nMax Revision = " + subwcrevObject2.MaxRev + "\nDate = " + subwcrevObject2.Date + "\nURL = " + subwcrevObject2.Url + "\nAuthor = " + subwcrevObject2.Author + "\nHasMods = " + subwcrevObject2.HasModifications;
wcInfoString3 = "Revision = " + subwcrevObject3.Revision + "\nMin Revision = " + subwcrevObject3.MinRev + "\nMax Revision = " + subwcrevObject3.MaxRev + "\nDate = " + subwcrevObject3.Date + "\nURL = " + subwcrevObject3.Url + "\nAuthor = " + subwcrevObject3.Author + "\nHasMods = " + subwcrevObject3.HasModifications;

WScript.Echo(wcInfoString1);
WScript.Echo(wcInfoString2);
WScript.Echo(wcInfoString3);

