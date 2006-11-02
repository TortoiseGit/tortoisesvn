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
    WScript.Quit(1);
}
if ( ! objScript.FileExists(sNewDoc))
{
    WScript.Echo("File " + sNewDoc +" does not exist.  Cannot compare the documents.");
    WScript.Quit(1);
}

objScript = null;

try
{
   word = WScript.CreateObject("Word.Application");
}
catch(e)
{
   WScript.Echo("You must have Microsoft Word installed to perform this operation.");
   WScript.Quit(1);
}

word.visible = true;

// Open the new document
destination = word.Documents.Open(sNewDoc);
    
// Compare to the base document
try
{
   destination.Compare(sBaseDoc, "", 2);
// "" preserves the AuthorName Variant.
// 2 is the CompareTarget Variant.  I do not know how to invoke
// Word.WdCompareTarget.wdCompareTargetNew, so I use the value
// instead.
// AuthorName and CompareTarget do not need to be specified for this
// function (already places comparison in a new document).
 
   WScript.Echo("Compare:\n" + sBaseDoc + "\n" + sNewDoc);

// Let the user know Compare was used and on what files (not needed).
// If this line is removed, and AuthorName and CompareTarget are
// removed from Compare, there is no change to existing Compare code.

}
catch(e)
{
   destination.Merge(sBaseDoc, 2);
 
// 2 is the MergeTarget Variant.  I do not know how to invoke
// Word.WdMergeTarget.wdMergeTargetNew, so I use the value instead.
// MergeTarget does need to be specified for this function (places
// merge in a new document).

   WScript.Echo("Merge:\n" + sBaseDoc + "\n" + sNewDoc);
 
// Let the user know Merge was used and on what files (needed IMO).

}
    
// Show the comparison result
if (Number(word.Version) < 12)
{
	word.ActiveDocument.Windows(1).Visible = 1;
}
    
// Mark the comparison document as saved to prevent the annoying
// "Save as" dialog from appearing.
word.ActiveDocument.Saved = 1;
    
// Close the first document
if (Number(word.Version) >= 10)
{
    destination.Close();
}
