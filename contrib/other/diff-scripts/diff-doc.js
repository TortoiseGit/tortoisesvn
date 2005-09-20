var objArgs,num,sBaseDoc,sNewDoc,objScript,word,destination;

objArgs = WScript.Arguments;
num = objArgs.length;
if (num < 2)
{
   WScript.Echo("Usage: [CScript | WScript] diff-doc.js base.doc new.doc");
   WScript.Quit(1);
}

sBaseDoc = objArgs(0);
sNewDoc = objArgs(1);

objScript = new ActiveXObject("Scripting.FileSystemObject");
if ( ! objScript.FileExists(sBaseDoc))
{
    WScript.Echo("File " + sBaseDoc + " does not exist.  Cannot compare the documents.");
    Wscript.Quit(1);
}
if ( ! objScript.FileExists(sNewDoc))
{
    WScript.Echo("File " + sNewDoc +" does not exist.  Cannot compare the documents.");
    Wscript.Quit(1);
}

objScript = null;

try
{
   word = WScript.CreateObject("Word.Application");
}
catch(e)
{
   Wscript.Echo("You must have Microsoft Word installed to perform this operation.");
   Wscript.Quit(1);
}

word.visible = true;

// Open the new document
destination = word.Documents.Open(sNewDoc);
    
// Compare to the base document
destination.Compare(sBaseDoc);
    
// Show the comparison result
word.ActiveDocument.Windows(1).Visible = 1;
    
// Mark the comparison document as saved to prevent the annoying
// "Save as" dialog from appearing.
word.ActiveDocument.Saved = 1;
    
// Close the first document
if (Number(word.Version) >= 10)
{
    destination.Close();
}
