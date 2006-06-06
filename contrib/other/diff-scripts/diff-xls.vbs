dim objExcelApp, objArgs, objScript, objBaseDoc, objNewDoc

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
    MsgBox "File " + sBaseDoc +" does not exist.  Cannot compare the documents.", vbExclamation, "File not found"
    Wscript.Quit 1
End If
If objScript.FileExists(sNewDoc) = False Then
    MsgBox "File " + sNewDoc +" does not exist.  Cannot compare the documents.", vbExclamation, "File not found"
    Wscript.Quit 1
End If

Set objScript = Nothing

On Error Resume Next
Set objExcelApp = Wscript.CreateObject("Excel.Application")
If Err.Number <> 0 Then
   Wscript.Echo "You must have Excel installed to perform this operation."
   Wscript.Quit 1
End If

'Open base excel sheet
objExcelApp.Workbooks.Open sBaseDoc
'Open new excel sheet
objExcelApp.Workbooks.Open sNewDoc
'Show Excel window
objExcelApp.Visible = True
'Create a compare side by side view
objExcelApp.Windows.CompareSideBySideWith(objExcelApp.Windows(2).Caption)
If Err.Number <> 0 Then
   objExcelApp.DisplayAlerts = False
   objExcelApp.Quit()
   Wscript.Echo "You must have Excel 2003 or later installed to use compare side-by-side feature."
   Wscript.Quit 1
End If