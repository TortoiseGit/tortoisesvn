option explicit

dim objArgs,num,sBaseDoc,sTheirDoc,sMyDoc,objScript,word,baseDoc

Set objArgs = WScript.Arguments
num = objArgs.Count
if num < 3 then
   MsgBox "Usage: [CScript | WScript] compare.vbs base.doc theirs.doc merged.doc", vbExclamation, "Invalid arguments"
   WScript.Quit 1
end if

sBaseDoc=objArgs(0)
sTheirDoc=objArgs(1)
sMyDoc=objArgs(2)

Set objScript = CreateObject("Scripting.FileSystemObject")
If objScript.FileExists(sBaseDoc) = False Then
    MsgBox "File " + sBaseDoc +" does not exist.  Cannot compare the documents.", vbExclamation, "File not found"
    Wscript.Quit 1
End If
If objScript.FileExists(sTheirDoc) = False Then
    MsgBox "File " + sTheirDoc +" does not exist.  Cannot compare the documents.", vbExclamation, "File not found"
    Wscript.Quit 1
End If
If objScript.FileExists(sMyDoc) = False Then
    MsgBox "File " + sMyDoc +" does not exist.  Cannot compare the documents.", vbExclamation, "File not found"
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
set baseDoc = word.Documents.Open(sBaseDoc)
    
' Merge in the "theirs" document
baseDoc.Merge(sTheirDoc)

' Merge in the "My" document
baseDoc.Merge(sMyDoc)
    
' Show the merge result
word.ActiveDocument.Windows(1).Visible = 1
   
' Close the first document
baseDoc.Close
    
