// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#include "StdAfx.h"
#include "TortoiseProc.h"
#include "UnicodeStrings.h"
#include "ProjectProperties.h"
#include "SVNProperties.h"
#include "TSVNPath.h"


ProjectProperties::ProjectProperties(void)
{
	bNumber = TRUE;
	bWarnIfNoIssue = FALSE;
	nLogWidthMarker = 0;
	nMinLogSize = 0;
	nMinLockMsgSize = 0;
	bFileListInEnglish = TRUE;
	bAppend = TRUE;
	lProjectLanguage = 0;
}

ProjectProperties::~ProjectProperties(void)
{
}


BOOL ProjectProperties::ReadPropsPathList(const CTSVNPathList& pathList)
{
	for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
	{
		if (ReadProps(pathList[nPath]))
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL ProjectProperties::ReadProps(CTSVNPath path)
{
	BOOL bFoundBugtraqLabel = FALSE;
	BOOL bFoundBugtraqMessage = FALSE;
	BOOL bFoundBugtraqNumber = FALSE;
	BOOL bFoundBugtraqLogRe = FALSE;
	BOOL bFoundBugtraqURL = FALSE;
	BOOL bFoundBugtraqWarnIssue = FALSE;
	BOOL bFoundBugtraqAppend = FALSE;
	BOOL bFoundLogWidth = FALSE;
	BOOL bFoundLogTemplate = FALSE;
	BOOL bFoundMinLogSize = FALSE;
	BOOL bFoundMinLockMsgSize = FALSE;
	BOOL bFoundFileListEnglish = FALSE;
	BOOL bFoundProjectLanguage = FALSE;

	if (!path.IsDirectory())
		path = path.GetContainingDirectory();
		
	for (;;)
	{
		SVNProperties props(path);
		for (int i=0; i<props.GetCount(); ++i)
		{
			CString sPropName = props.GetItemName(i).c_str();
			CString sPropVal = CString(((char *)props.GetItemValue(i).c_str()));
			if ((!bFoundBugtraqLabel)&&(sPropName.Compare(BUGTRAQPROPNAME_LABEL)==0))
			{
				sLabel = sPropVal;
				bFoundBugtraqLabel = TRUE;
			}
			if ((!bFoundBugtraqMessage)&&(sPropName.Compare(BUGTRAQPROPNAME_MESSAGE)==0))
			{
				sMessage = sPropVal;
				bFoundBugtraqMessage = TRUE;
			}
			if ((!bFoundBugtraqNumber)&&(sPropName.Compare(BUGTRAQPROPNAME_NUMBER)==0))
			{
				CString val;
				val = sPropVal;
				val = val.Trim(_T(" \n\r\t"));
				if ((val.CompareNoCase(_T("false"))==0)||(val.CompareNoCase(_T("no"))==0))
					bNumber = FALSE;
				else
					bNumber = TRUE;
				bFoundBugtraqNumber = TRUE;
			}
			if ((!bFoundBugtraqLogRe)&&(sPropName.Compare(BUGTRAQPROPNAME_LOGREGEX)==0))
			{
				sCheckRe = sPropVal;
				if (sCheckRe.Find('\n')>=0)
				{
					sBugIDRe = sCheckRe.Mid(sCheckRe.Find('\n')).Trim();
					sCheckRe = sCheckRe.Left(sCheckRe.Find('\n')).Trim();
					if (!sBugIDRe.IsEmpty())
					{
						try
						{
							patBugIDRe.init((LPCTSTR)sBugIDRe, MULTILINE);
						}
						catch (bad_alloc){sBugIDRe.Empty();}
						catch (bad_regexpr){sBugIDRe.Empty();}
					}
				}
				if (!sCheckRe.IsEmpty())
				{
					sCheckRe = sCheckRe.Trim();
					try
					{
						patCheckRe.init((LPCTSTR)sCheckRe, MULTILINE);
					}
					catch (bad_alloc){sCheckRe.Empty();}
					catch (bad_regexpr){sCheckRe.Empty();}
				}
				bFoundBugtraqLogRe = TRUE;
			}
			if ((!bFoundBugtraqURL)&&(sPropName.Compare(BUGTRAQPROPNAME_URL)==0))
			{
				sUrl = sPropVal;
				bFoundBugtraqURL = TRUE;
			}
			if ((!bFoundBugtraqWarnIssue)&&(sPropName.Compare(BUGTRAQPROPNAME_WARNIFNOISSUE)==0))
			{
				CString val;
				val = sPropVal;
				val = val.Trim(_T(" \n\r\t"));
				if ((val.CompareNoCase(_T("true"))==0)||(val.CompareNoCase(_T("yes"))==0))
					bWarnIfNoIssue = TRUE;
				else
					bWarnIfNoIssue = FALSE;
				bFoundBugtraqWarnIssue = TRUE;
			}
			if ((!bFoundBugtraqAppend)&&(sPropName.Compare(BUGTRAQPROPNAME_APPEND)==0))
			{
				CString val;
				val = sPropVal;
				val = val.Trim(_T(" \n\r\t"));
				if ((val.CompareNoCase(_T("true"))==0)||(val.CompareNoCase(_T("yes"))==0))
					bAppend = TRUE;
				else
					bAppend = FALSE;
				bFoundBugtraqAppend = TRUE;
			}
			if ((!bFoundLogWidth)&&(sPropName.Compare(PROJECTPROPNAME_LOGWIDTHLINE)==0))
			{
				CString val;
				val = sPropVal;
				if (!val.IsEmpty())
				{
					nLogWidthMarker = _ttoi(val);
				}
				bFoundLogWidth = TRUE;
			}
			if ((!bFoundLogTemplate)&&(sPropName.Compare(PROJECTPROPNAME_LOGTEMPLATE)==0))
			{
				sLogTemplate = sPropVal;
				sLogTemplate.Replace(_T("\r"), _T(""));
				sLogTemplate.Replace(_T("\n"), _T("\r\n"));
				bFoundLogTemplate = TRUE;
			}
			if ((!bFoundMinLogSize)&&(sPropName.Compare(PROJECTPROPNAME_LOGMINSIZE)==0))
			{
				CString val;
				val = sPropVal;
				if (!val.IsEmpty())
				{
					nMinLogSize = _ttoi(val);
				}
				bFoundMinLogSize = TRUE;
			}
			if ((!bFoundMinLockMsgSize)&&(sPropName.Compare(PROJECTPROPNAME_LOCKMSGMINSIZE)==0))
			{
				CString val;
				val = sPropVal;
				if (!val.IsEmpty())
				{
					nMinLockMsgSize = _ttoi(val);
				}
				bFoundMinLockMsgSize = TRUE;
			}
			if ((!bFoundFileListEnglish)&&(sPropName.Compare(PROJECTPROPNAME_LOGFILELISTLANG)==0))
			{
				CString val;
				val = sPropVal;
				val = val.Trim(_T(" \n\r\t"));
				if ((val.CompareNoCase(_T("false"))==0)||(val.CompareNoCase(_T("no"))==0))
					bFileListInEnglish = TRUE;
				else
					bFileListInEnglish = FALSE;
				bFoundFileListEnglish = TRUE;
			}
			if ((!bFoundProjectLanguage)&&(sPropName.Compare(PROJECTPROPNAME_PROJECTLANGUAGE)==0))
			{
				CString val;
				val = sPropVal;
				if (!val.IsEmpty())
				{
					LPTSTR strEnd;
					lProjectLanguage = _tcstol(val, &strEnd, 0);
				}
				bFoundProjectLanguage = TRUE;
			}
		}
		if (PathIsRoot(path.GetWinPath()))
			return FALSE;
		path = path.GetContainingDirectory();
		if ((!path.HasAdminDir())||(path.IsEmpty()))
		{
			if (bFoundBugtraqLabel | bFoundBugtraqMessage | bFoundBugtraqNumber
				| bFoundBugtraqURL | bFoundBugtraqWarnIssue | bFoundLogWidth
				| bFoundLogTemplate | bFoundBugtraqLogRe | bFoundMinLockMsgSize)
				return TRUE;
			return FALSE;
		}
	}
	//return FALSE;		//never reached
}

BOOL ProjectProperties::FindBugID(const CString& msg, CWnd * pWnd)
{
	size_t offset1 = 0;
	size_t offset2 = 0;

	if (sUrl.IsEmpty())
		return FALSE;

	// for use with GRETA, actually a basic_string<TCHAR>
	restring reMsg = (LPCTSTR)msg;

	// first use the checkre string to find bug ID's in the message
	if (!sCheckRe.IsEmpty())
	{
		if (!sBugIDRe.IsEmpty())
		{
			// match with two regex strings (without grouping!)
			try
			{
				match_results results;
				match_results::backref_type br;
				do 
				{
					br = patCheckRe.match( reMsg, results, offset1 );
					if( br.matched ) 
					{
						offset1 += results.rstart(0);
						offset2 = offset1 + results.rlength(0);
						ATLTRACE("matched %ws\n", results.backref(0).str().c_str());
						// now we have a full match. To create the links we need to extract the
						// bare bug ID's first.
						{
							size_t idoffset1=offset1;
							size_t idoffset2=offset2;
							match_results idresults;
							match_results::backref_type idbr;
							do 
							{
								idbr = patBugIDRe.match( reMsg, idresults, idoffset1, offset2-idoffset1);
								if (idbr.matched)
								{
									idoffset1 += idresults.rstart(0);
									idoffset2 = idoffset1 + idresults.rlength(0);
									ATLTRACE("matched id : %ws\n", idresults.backref(0).str().c_str());
									CHARRANGE range = {(LONG)idoffset1, (LONG)idoffset2};
									pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
									CHARFORMAT2 format;
									ZeroMemory(&format, sizeof(CHARFORMAT2));
									format.cbSize = sizeof(CHARFORMAT2);
									format.dwMask = CFM_LINK;
									format.dwEffects = CFE_LINK;
									pWnd->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
									idoffset1 = idoffset2;
								}
							} while(idbr.matched);
						}
						offset1 = offset2;
					}
				} while(br.matched);
			}
			catch (bad_alloc) {}
			catch (bad_regexpr){}
		}
		else
		{
			try
			{
				match_results results;
				match_results::backref_type br;

				do 
				{
					br = patCheckRe.match( reMsg, results, offset1 );
					if( br.matched ) 
					{
						for (size_t i=1; i<results.cbackrefs(); ++i)
						{
							if (results.rlength(i))
							{
								ATLTRACE("matched id : %ws\n", results.backref(i).str().c_str());
								CHARRANGE range = {(LONG)(offset1 + results.rstart(i))
									,(LONG)(offset1 + results.rstart(i) + results.rlength(i))};
								if (range.cpMin != range.cpMax)
								{
									pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
									CHARFORMAT2 format;
									ZeroMemory(&format, sizeof(CHARFORMAT2));
									format.cbSize = sizeof(CHARFORMAT2);
									format.dwMask = CFM_LINK;
									format.dwEffects = CFE_LINK;
									pWnd->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
								}
							}
						}
					}

					// that code looks broken:
					// we just iterated over all matching ranges but skip only the first one
					// => skip all

					// offset1 += results.rstart(0);
					// offset1 += results.rlength(0);
					if (br.matched)
					{
						offset1 += results.rstart(results.cbackrefs()-1);
						offset1 += results.rlength(results.cbackrefs()-1);
					}
				} while ((br.matched)&&(results.rlength(results.cbackrefs()-1)));
			}
			catch (bad_alloc) {}
			catch (bad_regexpr) {}
		}
	}

	if (!sMessage.IsEmpty())
	{
		CString sBugLine;
		CString sFirstPart;
		CString sLastPart;
		BOOL bTop = FALSE;
		if (sMessage.Find(_T("%BUGID%"))<0)
			return FALSE;
		sFirstPart = sMessage.Left(sMessage.Find(_T("%BUGID%")));
		sLastPart = sMessage.Mid(sMessage.Find(_T("%BUGID%"))+7);
		if (msg.ReverseFind('\n')>=0)
			sBugLine = msg.Mid(msg.ReverseFind('\n')+1);
		else
			sBugLine = msg;
		if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart)!=0)
			sBugLine.Empty();
		if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart)!=0)
			sBugLine.Empty();
		if (sBugLine.IsEmpty())
		{
			if (msg.Find('\n')>=0)
				sBugLine = msg.Left(msg.Find('\n'));
			if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart)!=0)
				sBugLine.Empty();
			if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart)!=0)
				sBugLine.Empty();
			bTop = TRUE;
		}
		if (sBugLine.IsEmpty())
			return FALSE;
		CString sBugIDPart = sBugLine.Mid(sFirstPart.GetLength(), sBugLine.GetLength() - sFirstPart.GetLength() - sLastPart.GetLength());
		if (sBugIDPart.IsEmpty())
			return FALSE;
		//the bug id part can contain several bug id's, separated by commas
		if (!bTop)
			offset1 = msg.GetLength() - sBugLine.GetLength() + sFirstPart.GetLength();
		else
			offset1 = sFirstPart.GetLength();
		sBugIDPart.Trim(_T(","));
		while (sBugIDPart.Find(',')>=0)
		{
			offset2 = offset1 + sBugIDPart.Find(',');
			CHARRANGE range = {(LONG)offset1, (LONG)offset2};
			pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
			CHARFORMAT2 format;
			ZeroMemory(&format, sizeof(CHARFORMAT2));
			format.cbSize = sizeof(CHARFORMAT2);
			format.dwMask = CFM_LINK;
			format.dwEffects = CFE_LINK;
			pWnd->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
			sBugIDPart = sBugIDPart.Mid(sBugIDPart.Find(',')+1);
			offset1 = offset2 + 1;
		}
		offset2 = offset1 + sBugIDPart.GetLength();
		CHARRANGE range = {(LONG)offset1, (LONG)offset2};
		pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
		CHARFORMAT2 format;
		ZeroMemory(&format, sizeof(CHARFORMAT2));
		format.cbSize = sizeof(CHARFORMAT2);
		format.dwMask = CFM_LINK;
		format.dwEffects = CFE_LINK;
		pWnd->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
		return TRUE;
	}
	return FALSE;
}

CString ProjectProperties::GetBugIDUrl(const CString& sBugID)
{
	CString ret;
	if (sUrl.IsEmpty())
		return ret;
	if (!sMessage.IsEmpty() || !sCheckRe.IsEmpty())
	{
		ret = sUrl;
		ret.Replace(_T("%BUGID%"), sBugID);
	}
	return ret;
}

BOOL ProjectProperties::CheckBugID(const CString& sID)
{
	if (!sCheckRe.IsEmpty()&&(!bNumber)&&!sID.IsEmpty())
	{
		CString sBugID = sID;
		sBugID.Replace(_T(", "), _T(","));
		sBugID.Replace(_T(" ,"), _T(","));
		CString sMsg = sMessage;
		sMsg.Replace(_T("%BUGID%"), sBugID);
		return HasBugID(sMsg);
	}
	if (bNumber)
	{
		// check if the revision actually _is_ a number
		// or a list of numbers separated by colons
		TCHAR c = 0;
		int len = sID.GetLength();
		for (int i=0; i<len; ++i)
		{
			c = sID.GetAt(i);
			if ((c < '0')&&(c != ','))
			{
				return FALSE;
			}
			if (c > '9')
				return FALSE;
		}
	}
	return TRUE;
}

BOOL ProjectProperties::HasBugID(const CString& sMessage)
{
	if (!sCheckRe.IsEmpty())
	{
		try
		{
			match_results results;
			match_results::backref_type br;

			br = patCheckRe.match( restring ((LPCTSTR)sMessage), results );
			return br.matched;
		}
		catch (bad_alloc) {}
		catch (bad_regexpr) {}
	}
	return FALSE;
}

#ifdef DEBUG
static class PropTest
{
public:
	PropTest()
	{
		size_t offset1 = 0;
		size_t offset2 = 0;
		CString sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
		CString sCheckRe = _T("[Ii]ssue #?(\\d+)(,? ?#?(\\d+))+");
		CString sBugIDRe = _T("(\\d+)");
		CString msg = _T("this is a test logmessage: issue 222\nIssue #456, #678, 901  #456");
		match_results results;
		rpattern pat( (LPCTSTR)sCheckRe, MULTILINE ); 
		match_results::backref_type br;

		// for use with GRETA
		restring reMsg = (LPCTSTR)msg;
		do 
		{
			br = pat.match( reMsg, results, offset1);

			if( br.matched ) 
			{
				offset1 += results.rstart(0);
				offset2 = offset1 + results.rlength(0);
				ATLTRACE("matched : %ws\n", results.backref(0).str().c_str());
				{
					size_t idoffset1=offset1;
					match_results idresults;
					rpattern idpat( (LPCTSTR)sBugIDRe, MULTILINE );
					match_results::backref_type idbr;
					do 
					{
						idbr = idpat.match( reMsg, idresults, idoffset1, offset2-idoffset1);
						if (idbr.matched)
						{
							idoffset1 += idresults.rstart(0);
							idoffset1 += idresults.rlength(0);
							ATLTRACE("matched id : %ws\n", idresults.backref(0).str().c_str());
						}
					} while(idbr.matched);
				}
				offset1 = offset2;
			}
		} while(br.matched);

		try
		{
			match_results results;
			match_results::backref_type br;
			CString sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
			CString sCheckRe = _T("[Ii]ssue #?(\\d+),? ?#?(\\d+)?,? ?#?(\\d+)?,? ?#?(\\d+)?,? ?#?(\\d+)?,? ?#?(\\d+)?");
			CString msg = _T("this is a test logmessage: issue 2222\nIssue #456, 678, #901, #100  #456");
			offset1 = 0;

			restring reMsg = (LPCTSTR)msg;
			do 
			{
				rpattern pat( (LPCTSTR)sCheckRe, MULTILINE ); 

				br = pat.match( reMsg, results, offset1);
				ATLTRACE("matched : %ws\n", results.backref(0).str().c_str());
				if( br.matched ) 
				{
					for (size_t i=1; i<results.cbackrefs(); ++i)
					{
						ATLTRACE("matched id : %ws\n", results.backref(i).str().c_str());
					}
				}
				if (br.matched)
				{
					offset1 += results.rstart(0);
					offset1 += results.rlength(0);
				}
			} while(br.matched);
			msg.ReleaseBuffer();
		}
		catch (bad_alloc) {}
		catch (bad_regexpr) {}

	}
} PropTest;
#endif


