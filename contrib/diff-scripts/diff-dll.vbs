' extensions: dll;exe
'
' TortoiseSVN Diff script for dll and exe files
'
' Copyright (C) 2010 the TortoiseSVN team
' This file is distributed under the same license as TortoiseSVN
'
' Last commit by:
' $Author$
' $Date$
' $Rev$
'
' Authors:
' Casey Barton, 2010
'
dim objArgs, objScript, sBaseVer, sNewVer, sMessage

Set objArgs = WScript.Arguments
num = objArgs.Count
if num < 2 then
   MsgBox "Usage: [CScript | WScript] compare.vbs base.doc new.doc", vbExclamation, "Invalid arguments"
   WScript.Quit 1
end if

sBaseDoc = objArgs(0)
sNewDoc = objArgs(1)

Set objScript = CreateObject("Scripting.FileSystemObject")
If objScript.FileExists(sBaseDoc) = False Then
    MsgBox "File " + sBaseDoc +" does not exist.  Cannot compare the files.", vbExclamation, "File not found"
    Wscript.Quit 1
End If
If objScript.FileExists(sNewDoc) = False Then
    MsgBox "File " + sNewDoc +" does not exist.  Cannot compare the files.", vbExclamation, "File not found"
    Wscript.Quit 1
End If

sBaseVer = objScript.GetFileVersion(sBaseDoc)
sNewVer = objScript.GetFileVersion(sNewDoc)

if sBaseVer = sNewVer then
    sMessage = "Versions are identical: " + sBaseVer
else
    sMessage = "Versions differ:" + vbCrLf + vbCrLf _
        + "Base" + vbCrLf _
        + "  File: " + sBaseDoc + vbCrLf _
        + "  Version: " + sBaseVer + vbCrLf + vbCrLf _
        + "New" + vbCrLf _
        + "  File: " + sNewDoc + vbCrLf _
        + "  Version: " + sNewVer
        
end if

MsgBox sMessage, vbInformation, "DLL Version Comparison"

Wscript.Quit
