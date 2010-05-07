/*****************************************************
	post update hook script for TSVN.
	automates compilation of WC of JAWS scripts just in place (user folder of appropriate version of JAWS).
	Intended to be used by blind or visually impaired developers using JAWS screen reader.
	Licensed under GNU GPL v2.
	(c) by Serge Tumanyan 2010.

	Installation:
		1. Open TSVN Settings.
		2. Go to 'Hooks' item in a treeview.
		3. Tab to the 'Add' button and press it.
		4. From the dialogue opened select post update hook, then set the path to the JAWS user folder 
		   (e. g. 'c:\documents and settings\Administrator\application data\freedom scientific\jaws\11.0\settings\enu').
		5. In the command line field type 'WScript ReCompileJAWSScriptsWC.js'.
		6. Select the checkboxes according to your choice.
		7. Issue an 'Ok' button.
		8. Save the changes.
		9. Enjoy the automatic compilation of your scripts after updating...
*****************************************************************/

// you can change the script name in the next line to compile the scripts more specific, not all the scripts in the folder...
var ScriptName = "*.jss";
// you can translate the following lines to your native language...
var LastRevision = "Revision: ";
var WorkingFolder = "Working directory: ";
var StartedIncorrectly = "The script was started either from the command line or from a wrong hook - the script is intended to be started only from the post update hook.";
var Finished = "Compiled...";
// End of lines to translate...
var ShellObject = WScript.CreateObject ("WScript.Shell");
var SAPI5Object = WScript.CreateObject ("SAPI.SpVoice");
var ArgumentsObject = WScript.Arguments;
var Version, PathParts, PathToSCompile;

if (ArgumentsObject.length != 5)
	{	SAPI5Object.Speak (StartedIncorrectly, 0);
		WScript.Quit(1);
	}
SAPI5Object.Speak (LastRevision + ArgumentsObject.Item (2), 0);
SAPI5Object.Speak (WorkingFolder + ArgumentsObject.Item (4), 0);
PathParts = WScript.ScriptFullName.split ("\\");
Version = PathParts[PathParts.length - 4];
PathToSCompile = "\"" + ShellObject.RegRead ("HKLM\\SOFTWARE\\Freedom Scientific\\JAWS\\" + Version + "\\Target") + "scompile.exe\"";
ShellObject.Run (PathToSCompile + ScriptName, 0, true);
SAPI5Object.Speak (Finished, 0);
