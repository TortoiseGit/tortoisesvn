'
' TortoiseSVN Diff script for Mathematica notebooks
'
' Last commit by:
' $Author$
' $Date$
' $Rev$
'
' Authors:
' Chris Rodgers http://rodgers.org.uk/, 2008
' (Based on diff-xlsx.vbs)
'

dim objArgs, objScript, objDiffNotebook

Set objArgs = WScript.Arguments
num = objArgs.Count
if num < 2 then
   MsgBox "Usage: [CScript | WScript] compare.vbs base.nb new.nb", vbExclamation, "Invalid arguments"
   WScript.Quit 1
end if

sBaseDoc = objArgs(0)
sNewDoc = objArgs(1)

Set objScript = CreateObject("Scripting.FileSystemObject")
If objScript.FileExists(sBaseDoc) = False Then
    MsgBox "File " + sBaseDoc +" does not exist.  Cannot compare the notebooks.", vbExclamation, "File not found"
    Wscript.Quit 1
End If
If objScript.FileExists(sNewDoc) = False Then
    MsgBox "File " + sNewDoc +" does not exist.  Cannot compare the notebooks.", vbExclamation, "File not found"
    Wscript.Quit 1
End If

On Error Resume Next
Dim tfolder, tname, tfile
Const TemporaryFolder = 2

Set tfolder = objScript.GetSpecialFolder(TemporaryFolder)

tname = objScript.GetTempName + ".nb"
Set objDiffNotebook = tfolder.CreateTextFile(tname)

'Output a Mathematica notebook that will do the diff for us
'We need a convoluted way of evaluating the NotebookDiff function to allow a single button press to load and run the package
objDiffNotebook.WriteLine "(* Content-type: application/mathematica *)" + vbCrLf + _
"(*** Wolfram Notebook File ***)" + vbCrLf + _
"(* http://www.wolfram.com/nb *)" + vbCrLf + _
"(* CreatedBy='Mathematica 6.0' *)" + vbCrLf + _
"(* Beginning of Notebook Content *)" + vbCrLf + _
"Notebook[{" + vbCrLf + _
"Cell[CellGroupData[{" + vbCrLf + _
"Cell[BoxData[" + vbCrLf + _
"ButtonBox[""\<\""Compare notebooks\""\>""," + vbCrLf + _
"Appearance->Automatic," + vbCrLf + _
"ButtonFrame->""DialogBox""," + vbCrLf + _
"ButtonFunction:>(Module[{x=ReplaceAll[Get[""AuthorTools`""]; " + vbCrLf + _
"nd[" + vbCrLf + _
"""" + Replace(sBaseDoc,"\","\\") + """," + vbCrLf + _
"""" + Replace(sNewDoc,"\","\\") + """], nd :> Symbol[""NotebookDiff""]]},CreateDocument[x]])," + vbCrLf + _
"Evaluator->Automatic," + vbCrLf + _
"Method->""Preemptive""]], ""Output""," + vbCrLf + _
"CellChangeTimes->{3.4277012139130297`*^9}]" + vbCrLf + _
"}, Open  ]]" + vbCrLf + _
"}]" + vbCrLf + _
"(* End of Notebook Content *)"

objDiffNotebook.Close

Set objShell = CreateObject("WScript.Shell")
objShell.Run tfolder + "\" + tname
