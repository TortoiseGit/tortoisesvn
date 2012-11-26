// testCOMMemory.js - javascript file
// test script for the SubWCRev COM/Automation-object

var i, svn;

svn = new ActiveXObject("SubWCRev.object");

for (i = 0; i < 10000; ++i) {
    svn.GetWCInfo("C:\\projects\\trunk\\common\\common.cpp", false, false);
}
