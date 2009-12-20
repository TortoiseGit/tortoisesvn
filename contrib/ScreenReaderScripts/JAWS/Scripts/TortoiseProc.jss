;
;
;

Include "HJConst.jsh"
Include "HJGlobal.jsh"
Include "Common.jsm"

Const
	URLOfRepositoryControlID = 1029,
	URLOfRepositoryComboWrapperClassName = "ComboBoxEx32",
	RevisionLogListViewControlID = 1003

Void Function AutoStartEvent ()
let nSaySelectAfter = IniReadInteger (SECTION_OPTIONS, hKey_SaySelectedFirst, 0, file_default_jcf)
let nSaySelectAfter = IniReadInteger (SECTION_OPTIONS, hKey_SaySelectedFirst, nSaySelectAfter, GetActiveConfiguration () + cScPeriod + jcfFileExt)
let nSaySelectAfter = Not nSaySelectAfter
AutoStartEvent ()
EndFunction

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

Void Function SayObjectActiveItem (Int iPosition)
var
	Handle hFocus,
	Object oListView,
	String sHelp,
	Int iChildID,
	Int iControlID,
	Int iWindowType

let hFocus = GetFocus ()
let iWindowType = GetWindowSubtypeCode (hFocus, TRUE)
let iControlID = GetControlID (hFocus)
If iControlID == RevisionLogListViewControlID
&& iWindowType == WT_LISTVIEW then
	let oListView = GetFocusObject (iChildID)
	let sHelp = oListView.accHelp (iChildID)
	If (! nSaySelectAfter) then
		Say (sHelp, OT_ITEM_STATE)
	EndIf
EndIf
SayObjectActiveItem (iPosition)
If iControlID == RevisionLogListViewControlID
&& iWindowType == WT_LISTVIEW
&& nSaySelectAfter then
	Say (sHelp, OT_ITEM_STATE)
EndIf
EndFunction

Int Function HandleCustomWindows (Handle hWnd)
var
	Int iWindowType,
	Int iControlID

let iWindowType = GetWindowSubtypeCode (hWnd, TRUE)
let iControlID = GetControlID (hWnd)
If iControlID == RevisionLogListViewControlID
&& iWindowType == WT_LISTVIEW then
	IndicateControlType (iWindowType, cScNull, cScSpace)
	SayObjectActiveItem (TRUE)
	Return (TRUE)
EndIf
Return (HandleCustomWindows (hWnd))
EndFunction

Script Debug ()
SayString ("Debug.")
SayString (GetObjectHelp ())
SayString (GetObjectDescription (TRUE))
var Object o, Int i
let o = GetFocusObject (i)
SayInteger (i)
SayString (o.accHelp (i))
EndScript


