' start this script like this
' wscript.exe "doc.vbs" %base %mine

option explicit

dim sPath1,sPath2,objArgs,word,destination

Set objArgs = WScript.Arguments
sPath1=objArgs(0)
sPath2=objArgs(1)

    set word = createobject("Word.Application")
    word.visible=1
    
    ' Open the first document
    set destination = word.Documents.Open(sPath2)
    ' Hide it
    destination.Windows(1).Visible=0
    
    ' Compare to the second document
    destination.Compare(sPath1)
    
    ' Show the comparison result
    word.ActiveDocument.Windows(1).Visible = 1
    
    ' Mark the comparison document as saved to prevent the annoying
    ' "Save as" dialog from appearing.
    word.ActiveDocument.Saved = 1
    
    ' Close the first document
    destination.Close
    