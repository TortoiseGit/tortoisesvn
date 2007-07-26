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
	objExcelApp.Application.WindowState = xlMaximized
	objExcelApp.Windows.Arrange(-4128)
End If

'Mark differences in sNewDoc red
objExcelApp.Workbooks(2).Sheets(1).Cells.FormatConditions.Delete
objExcelApp.Workbooks(1).Sheets(1).Copy ,objExcelApp.Workbooks(2).Sheets(objExcelApp.Workbooks(2).Sheets.Count)
objExcelApp.Workbooks(2).Sheets(objExcelApp.Workbooks(2).Sheets.Count).Name = "Dummy_for_Comparison"
objExcelApp.Workbooks(2).Sheets(1).Activate
'To create a local formula the cell A1 is used
original_content = objExcelApp.Workbooks(2).Sheets(1).Cells(1,1).Formula
String sFormula
objExcelApp.Workbooks(2).Sheets(1).Cells(1,1).Formula = "=INDIRECT(""Dummy_for_Comparison" & "!""&ADDRESS(ROW(),COLUMN()))"
sFormula = objExcelApp.Workbooks(2).Sheets(1).Cells(1,1).FormulaLocal
objExcelApp.Workbooks(2).Sheets(1).Cells(1,1).Formula = original_content
'with the local formula the conditional formatting is used to mark the cells that are different
const xlCellValue = 1
const xlNotEqual = 4
objExcelApp.Workbooks(2).Sheets(1).Cells.FormatConditions.Add xlCellValue, xlNotEqual, sFormula
objExcelApp.Workbooks(2).Sheets(1).Cells.FormatConditions(1).Interior.ColorIndex = 3
