option explicit

dim objArgs,num,sTheirDoc,sMyDoc,sBaseDoc,sMergedDoc,objScript,word,baseDoc

Set objArgs = WScript.Arguments
num = objArgs.Count
if num < 4 then
   MsgBox "Usage: [CScript | WScript] merge.vbs %merged %theirs %mine %base", vbExclamation, "Invalid arguments"
   WScript.Quit 1
end if

sMergedDoc=objArgs(0)
sTheirDoc=objArgs(1)
sMyDoc=objArgs(2)
sBaseDoc=objArgs(3)

Set objScript = CreateObject("Scripting.FileSystemObject")
If objScript.FileExists(sTheirDoc) = False Then
    MsgBox "File " + sTheirDoc +" does not exist.  Cannot compare the documents.", vbExclamation, "File not found"
    Wscript.Quit 1
End If
If objScript.FileExists(sMergedDoc) = False Then
    MsgBox "File " + sMergedDoc +" does not exist.  Cannot compare the documents.", vbExclamation, "File not found"
    Wscript.Quit 1
End If

Set objScript = Nothing

On Error Resume Next
set word = createobject("Word.Application")
If Err.Number <> 0 Then
   Wscript.Echo "You must have Microsoft Word installed to perform this operation."
   Wscript.Quit 1
End If

On Error Goto 0

word.visible=True    

' Open the base document
set baseDoc = word.Documents.Open(sTheirDoc)
    
' Merge into the "My" document
baseDoc.Merge(sMergedDoc)
    
' Show the merge result
word.ActiveDocument.Windows(1).Visible = 1
   
' Close the first document
baseDoc.Close
    
