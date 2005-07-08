dim objArgs,num,sBaseDoc,sNewDoc,objScript,word,destination

Set objArgs = WScript.Arguments
num = objArgs.Count
if num < 2 then
   MsgBox "Usage: [CScript | WScript] compare.vbs base.doc new.doc", vbExclamation, "Invalid arguments"
   WScript.Quit 1
end if

sBaseDoc=objArgs(0)
sNewDoc=objArgs(1)

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
set word = createobject("Word.Application")
If Err.Number <> 0 Then
   Wscript.Echo "You must have Microsoft Word installed to perform this operation."
   Wscript.Quit 1
End If

On Error Goto 0

word.visible=True

' Open the new document
set destination = word.Documents.Open(sNewDoc)
    
' Compare to the base document
destination.Compare(sBaseDoc)
    
' Show the comparison result
word.ActiveDocument.Windows(1).Visible = 1
    
' Mark the comparison document as saved to prevent the annoying
' "Save as" dialog from appearing.
word.ActiveDocument.Saved = 1
    
' Close the first document
destination.Close

