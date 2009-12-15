;
;
;

Include "HJConst.jsh"
Include "Common.jsm"

Const
	URLOfRepositoryControlID = 1029,
	URLOfRepositoryComboWrapperClassName = "ComboBoxEx32"

String Function FindHotKey (String ByRef sPrompt)
var
	Handle hPrompt,
	Int iType,
	Int iControlID,
	String sHotKey

let hPrompt = GetFocus ()
let iType = GetWindowSubTypeCode (hPrompt)
let iControlID = GetControlID (hPrompt)
If iType == WT_EDITCOMBO
&& iControlID == URLOfRepositoryControlID then
	let hPrompt = GetParent (hPrompt)
	let iType = GetWindowSubTypeCode (hPrompt)
	let iControlID = GetControlID (hPrompt)
	If iType == WT_COMBOBOX
	&& iControlID == URLOfRepositoryControlID then
		let hPrompt = GetParent (hPrompt)
		let iControlID = GetControlID (hPrompt)
		If GetWindowClass (hPrompt) == URLOfRepositoryComboWrapperClassName
		&& iControlID == URLOfRepositoryControlID then
			let sHotKey = GetHotKey(GetPriorWindow (hPrompt))
			If sHotKey then
				let sPrompt = GetWindowName (GetPriorWindow (hPrompt))
				return (sHotKey)
			EndIf
		EndIf
	EndIf
EndIf
Return (FindHotKey(sPrompt))
EndFunction


Script Debug ()
SayString ("Debug.")
SayString (GetObjectHelp ())
var Object o, Int i
let o = GetFocusObject (i)
SayInteger (i)
SayString (o.accHelp (i))
EndScript


