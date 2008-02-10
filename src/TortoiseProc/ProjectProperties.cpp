// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "StdAfx.h"
#include "TortoiseProc.h"
#include "UnicodeUtils.h"
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
	bHasLogSummaryPattern = false;
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
	BOOL bFoundUserFileProps = FALSE;
	BOOL bFoundUserDirProps = FALSE;
	BOOL bFoundWebViewRev = FALSE;
	BOOL bFoundWebViewPathRev = FALSE;
	BOOL bFoundAutoProps = FALSE;
	BOOL bFoundLogSummary = FALSE;

	if (!path.IsDirectory())
		path = path.GetContainingDirectory();
		
	for (;;)
	{
		SVNProperties props(path, SVNRev::REV_WC, false);
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
			if ((!bFoundUserFileProps)&&(sPropName.Compare(PROJECTPROPNAME_USERFILEPROPERTY)==0))
			{
				sFPPath = sPropVal;
				sFPPath.Replace(_T("\r\n"), _T("\n"));
				bFoundUserFileProps = TRUE;
			}
			if ((!bFoundUserDirProps)&&(sPropName.Compare(PROJECTPROPNAME_USERDIRPROPERTY)==0))
			{
				sDPPath = sPropVal;
				sDPPath.Replace(_T("\r\n"), _T("\n"));
				bFoundUserDirProps = TRUE;
			}
			if ((!bFoundAutoProps)&&(sPropName.Compare(PROJECTPROPNAME_AUTOPROPS)==0))
			{
				sAutoProps = sPropVal;
				sAutoProps.Replace(_T("\r\n"), _T("\n"));
				bFoundAutoProps = TRUE;
			}
			if ((!bFoundWebViewRev)&&(sPropName.Compare(PROJECTPROPNAME_WEBVIEWER_REV)==0))
			{
				sWebViewerRev = sPropVal;
				bFoundWebViewRev = TRUE;
			}
			if ((!bFoundWebViewPathRev)&&(sPropName.Compare(PROJECTPROPNAME_WEBVIEWER_PATHREV)==0))
			{
				sWebViewerPathRev = sPropVal;
				bFoundWebViewPathRev = TRUE;
			}
			if ((!bFoundLogSummary)&&(sPropName.Compare(PROJECTPROPNAME_LOGSUMMARY)==0))
			{
				try
				{
					patLogSummary.init((LPCTSTR)sPropVal, MULTILINE);
					bFoundLogSummary = TRUE;
					bHasLogSummaryPattern = true;
				}
				catch (bad_alloc){}
				catch (bad_regexpr){}
			}
		}
		if (PathIsRoot(path.GetWinPath()))
			return FALSE;
		path = path.GetContainingDirectory();
		if ((!path.HasAdminDir())||(path.IsEmpty()))
		{
			if (bFoundBugtraqLabel | bFoundBugtraqMessage | bFoundBugtraqNumber
				| bFoundBugtraqURL | bFoundBugtraqWarnIssue | bFoundLogWidth
				| bFoundLogTemplate | bFoundBugtraqLogRe | bFoundMinLockMsgSize
				| bFoundUserFileProps | bFoundUserDirProps | bFoundAutoProps
				| bFoundWebViewRev | bFoundWebViewPathRev | bFoundLogSummary)
				return TRUE;
			return FALSE;
		}
	}
	//return FALSE;		//never reached
}

CString ProjectProperties::GetBugIDFromLog(CString& msg)
{
	CString sBugID;

	if (!sMessage.IsEmpty())
	{
		CString sBugLine;
		CString sFirstPart;
		CString sLastPart;
		BOOL bTop = FALSE;
		if (sMessage.Find(_T("%BUGID%"))<0)
			return sBugID;
		sFirstPart = sMessage.Left(sMessage.Find(_T("%BUGID%")));
		sLastPart = sMessage.Mid(sMessage.Find(_T("%BUGID%"))+7);
		msg.TrimRight('\n');
		if (msg.ReverseFind('\n')>=0)
			sBugLine = msg.Mid(msg.ReverseFind('\n')+1);
		else
		{
			if (bNumber)
			{
				// find out if the message consists only of numbers
				bool bOnlyNumbers = true;
				for (int i=0; i<msg.GetLength(); ++i)
				{
					if (!_istdigit(msg[i]))
					{
						bOnlyNumbers = false;
						break;
					}
				}
				if (bOnlyNumbers)
					sBugLine = msg;
			}
			else
				sBugLine = msg;
		}
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
			return sBugID;
		sBugID = sBugLine.Mid(sFirstPart.GetLength(), sBugLine.GetLength() - sFirstPart.GetLength() - sLastPart.GetLength());
		if (bTop)
		{
			msg = msg.Mid(sBugLine.GetLength());
			msg.TrimLeft('\n');
		}
		else
		{
			msg = msg.Left(msg.GetLength()-sBugLine.GetLength());
			msg.TrimRight('\n');
		}
	}
	return sBugID;
}

BOOL ProjectProperties::FindBugID(const CString& msg, CWnd * pWnd)
{
	size_t offset1 = 0;
	size_t offset2 = 0;
	bool bFound = false;

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
						ATLTRACE(_T("matched %s\n"), results.backref(0).str().c_str());
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
									ATLTRACE(_T("matched id : %s\n"), idresults.backref(0).str().c_str());
									CHARRANGE range = {(LONG)idoffset1, (LONG)idoffset2};
									pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
									CHARFORMAT2 format;
									ZeroMemory(&format, sizeof(CHARFORMAT2));
									format.cbSize = sizeof(CHARFORMAT2);
									format.dwMask = CFM_LINK;
									format.dwEffects = CFE_LINK;
									pWnd->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
									idoffset1 = idoffset2;
									bFound = true;
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
						// clear the styles up to the match position
						ATLTRACE(_T("matched string : %s\n"), results.backref(0).str().c_str());
						offset1 += results.rstart(0);
						{
							ATLTRACE("matched results : %ld\n", results.cbackrefs());
							for (size_t test=0; test<results.cbackrefs(); ++test)
							{
								ATLTRACE(_T("matched (%d): %s\n"), test, results.backref(test).str().c_str());
							}
							// we define group 1 as the whole issue text
							// and group 2 as the issue number
							CHARRANGE range;
							if ((results.cbackrefs() > 2)&&(results.backref(2).matched))
							{
								if (results.backref(2).str().empty())
									range.cpMin = (LONG)(offset1 + results.rlength(1));
								else
									range.cpMin = (LONG)(offset1 + results.rlength(1)-results.rlength(2));
								range.cpMax =  (LONG)(offset1 + results.rlength(1));
							}
							else if ((results.cbackrefs() > 1)&&(results.backref(1).matched)&&(results.backref(1).str().size()>0))
							{
								range.cpMin = (LONG)(offset1 + results.rstart(1)-results.rstart(0));
								range.cpMax = (LONG)(range.cpMin + results.rlength(1));
							}
							else
							{
								range.cpMin = offset1;
								range.cpMax = offset1;
							}
							if (range.cpMin != range.cpMax)
							{
								pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
								CHARFORMAT2 format;
								ZeroMemory(&format, sizeof(CHARFORMAT2));
								format.cbSize = sizeof(CHARFORMAT2);
								format.dwMask = CFM_LINK;
								format.dwEffects = CFE_LINK;
								pWnd->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
								bFound = true;
							}
							offset1 += results.rlength(0);
						}
					}
				} while ((br.matched)&&((int)offset1<msg.GetLength()));
			}
			catch (bad_alloc) {}
			catch (bad_regexpr) {}
		}
	}
	else if ((!bFound)&&(!sMessage.IsEmpty()))
	{
		CString sBugLine;
		CString sFirstPart;
		CString sLastPart;
		BOOL bTop = FALSE;
		if (sMessage.Find(_T("%BUGID%"))<0)
			return FALSE;
		sFirstPart = sMessage.Left(sMessage.Find(_T("%BUGID%")));
		sLastPart = sMessage.Mid(sMessage.Find(_T("%BUGID%"))+7);
		CString sMsg = msg;
		sMsg.TrimRight('\n');
		if (sMsg.ReverseFind('\n')>=0)
			sBugLine = sMsg.Mid(sMsg.ReverseFind('\n')+1);
		else
			sBugLine = sMsg;
		if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart)!=0)
			sBugLine.Empty();
		if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart)!=0)
			sBugLine.Empty();
		if (sBugLine.IsEmpty())
		{
			if (sMsg.Find('\n')>=0)
				sBugLine = sMsg.Left(sMsg.Find('\n'));
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
			offset1 = sMsg.GetLength() - sBugLine.GetLength() + sFirstPart.GetLength();
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

CString ProjectProperties::FindBugID(const CString& msg)
{
	size_t offset1 = 0;
	size_t offset2 = 0;
	bool bFound = false;

	CString sRet;

	std::set<CString> bugIDs;

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
						ATLTRACE(_T("matched %s\n"), results.backref(0).str().c_str());
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
									ATLTRACE(_T("matched id : %s\n"), idresults.backref(0).str().c_str());
									bugIDs.insert(CString(idresults.backref(0).str().c_str()));
									idoffset1 = idoffset2;
									bFound = true;
								}
							} while ((idbr.matched)&&(offset2-idoffset1));
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
						// clear the styles up to the match position
						ATLTRACE(_T("matched string : %s\n"), results.backref(0).str().c_str());
						offset1 += results.rstart(0);
						{
							ATLTRACE("matched results : %ld\n", results.cbackrefs());
							for (size_t test=0; test<results.cbackrefs(); ++test)
							{
								ATLTRACE(_T("matched (%d): %s\n"), test, results.backref(test).str().c_str());
							}
							// we define group 1 as the whole issue text
							// and group 2 as the issue number
							CHARRANGE range;
							if ((results.cbackrefs() > 2)&&(results.backref(2).matched))
							{
								if (results.backref(2).str().empty())
									range.cpMin = (LONG)(offset1 + results.rlength(1));
								else
									range.cpMin = (LONG)(offset1 + results.rlength(1)-results.rlength(2));
								range.cpMax =  (LONG)(offset1 + results.rlength(1));
							}
							else if ((results.cbackrefs() > 1)&&(results.backref(1).matched)&&(results.backref(1).str().size()>0))
							{
								range.cpMin = (LONG)(offset1 + results.rstart(1)-results.rstart(0));
								range.cpMax = (LONG)(range.cpMin + results.rlength(1));
							}
							else
							{
								range.cpMin = range.cpMax = 0;
							}
							if (range.cpMin != range.cpMax)
							{
								bugIDs.insert(msg.Mid(range.cpMin, range.cpMax-range.cpMin));
								bFound = true;
							}
							offset1 += results.rlength(0);
						}
					}
				} while ((br.matched)&&((int)offset1<msg.GetLength()));
			}
			catch (bad_alloc) {}
			catch (bad_regexpr) {}
		}
	}
	else if ((!bFound)&&(!sMessage.IsEmpty()))
	{
		CString sBugLine;
		CString sFirstPart;
		CString sLastPart;
		BOOL bTop = FALSE;
		if (sMessage.Find(_T("%BUGID%"))<0)
			goto finish;
		sFirstPart = sMessage.Left(sMessage.Find(_T("%BUGID%")));
		sLastPart = sMessage.Mid(sMessage.Find(_T("%BUGID%"))+7);
		CString sMsg = msg;
		sMsg.TrimRight('\n');
		if (sMsg.ReverseFind('\n')>=0)
			sBugLine = sMsg.Mid(sMsg.ReverseFind('\n')+1);
		else
			sBugLine = sMsg;
		if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart)!=0)
			sBugLine.Empty();
		if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart)!=0)
			sBugLine.Empty();
		if (sBugLine.IsEmpty())
		{
			if (sMsg.Find('\n')>=0)
				sBugLine = sMsg.Left(sMsg.Find('\n'));
			if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart)!=0)
				sBugLine.Empty();
			if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart)!=0)
				sBugLine.Empty();
			bTop = TRUE;
		}
		if (sBugLine.IsEmpty())
			goto finish;
		CString sBugIDPart = sBugLine.Mid(sFirstPart.GetLength(), sBugLine.GetLength() - sFirstPart.GetLength() - sLastPart.GetLength());
		if (sBugIDPart.IsEmpty())
			goto finish;
		//the bug id part can contain several bug id's, separated by commas
		if (!bTop)
			offset1 = sMsg.GetLength() - sBugLine.GetLength() + sFirstPart.GetLength();
		else
			offset1 = sFirstPart.GetLength();
		sBugIDPart.Trim(_T(","));
		while (sBugIDPart.Find(',')>=0)
		{
			offset2 = offset1 + sBugIDPart.Find(',');
			CHARRANGE range = {(LONG)offset1, (LONG)offset2};
			bugIDs.insert(msg.Mid(range.cpMin, range.cpMax-range.cpMin));
			sBugIDPart = sBugIDPart.Mid(sBugIDPart.Find(',')+1);
			offset1 = offset2 + 1;
		}
		offset2 = offset1 + sBugIDPart.GetLength();
		CHARRANGE range = {(LONG)offset1, (LONG)offset2};
		bugIDs.insert(msg.Mid(range.cpMin, range.cpMax-range.cpMin));
	}
finish:
	for (std::set<CString>::iterator it = bugIDs.begin(); it != bugIDs.end(); ++it)
	{
		sRet += *it;
		sRet += _T(" ");
	}
	sRet.Trim();
	return sRet;
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

void ProjectProperties::InsertAutoProps(svn_config_t *cfg)
{
	// every line is an autoprop
	CString sPropsData = sAutoProps;
	bool bEnableAutoProps = false;
	while (!sPropsData.IsEmpty())
	{
		int pos = sPropsData.Find('\n');
		CString sLine = pos >= 0 ? sPropsData.Left(pos) : sPropsData;
		sLine.Trim();
		if (!sLine.IsEmpty())
		{
			// format is '*.something = property=value;property=value;....'
			// find first '=' char
			int equalpos = sLine.Find('=');
			if ((equalpos >= 0)&&(sLine[0] != '#'))
			{
				CString key = sLine.Left(equalpos);
				CString value = sLine.Mid(equalpos);
				key.Trim(_T(" ="));
				value.Trim(_T(" ="));
				svn_config_set(cfg, SVN_CONFIG_SECTION_AUTO_PROPS, (LPCSTR)CUnicodeUtils::GetUTF8(key), (LPCSTR)CUnicodeUtils::GetUTF8(value));
				bEnableAutoProps = true;
			}
		}
		if (pos >= 0)
			sPropsData = sPropsData.Mid(pos).Trim();
		else
			sPropsData.Empty();
	}
	if (bEnableAutoProps)
		svn_config_set(cfg, SVN_CONFIG_SECTION_MISCELLANY, SVN_CONFIG_OPTION_ENABLE_AUTO_PROPS, "yes");
}

bool ProjectProperties::AddAutoProps(const CTSVNPath& path)
{
	if (!path.IsDirectory())
		return true;	// no error, but nothing to do

	bool bRet = true;

	char buf[1024] = {0};
	SVNProperties props(path, SVNRev::REV_WC, false);
	if (!sLabel.IsEmpty())
		bRet = props.Add(BUGTRAQPROPNAME_LABEL, CUnicodeUtils::StdGetUTF8((LPCTSTR)sLabel)) && bRet;
	if (!sMessage.IsEmpty())
		bRet = props.Add(BUGTRAQPROPNAME_MESSAGE, CUnicodeUtils::StdGetUTF8((LPCTSTR)sMessage)) && bRet;
	if (!bNumber)
		bRet = props.Add(BUGTRAQPROPNAME_NUMBER, "false") && bRet;
	if (!sCheckRe.IsEmpty())
		bRet = props.Add(BUGTRAQPROPNAME_LOGREGEX, CUnicodeUtils::StdGetUTF8((LPCTSTR)(sCheckRe + _T("\n") + sBugIDRe))) && bRet;
	if (!sUrl.IsEmpty())
		bRet = props.Add(BUGTRAQPROPNAME_URL, CUnicodeUtils::StdGetUTF8((LPCTSTR)sUrl)) && bRet;
	if (bWarnIfNoIssue)
		bRet = props.Add(BUGTRAQPROPNAME_WARNIFNOISSUE, "true") && bRet;
	if (!bAppend)
		bRet = props.Add(BUGTRAQPROPNAME_APPEND, "false") && bRet;
	if (nLogWidthMarker)
	{
		sprintf_s(buf, sizeof(buf), "%ld", nLogWidthMarker);
		bRet = props.Add(PROJECTPROPNAME_LOGWIDTHLINE, buf) && bRet;
	}
	if (!sLogTemplate.IsEmpty())
		bRet = props.Add(PROJECTPROPNAME_LOGTEMPLATE, CUnicodeUtils::StdGetUTF8((LPCTSTR)sLogTemplate)) && bRet;
	if (nMinLogSize)
	{
		sprintf_s(buf, sizeof(buf), "%ld", nMinLogSize);
		bRet = props.Add(PROJECTPROPNAME_LOGMINSIZE, buf) && bRet;
	}
	if (nMinLockMsgSize)
	{
		sprintf_s(buf, sizeof(buf), "%ld", nMinLockMsgSize);
		bRet = props.Add(PROJECTPROPNAME_LOCKMSGMINSIZE, buf) && bRet;
	}
	if (!bFileListInEnglish)
		bRet = props.Add(PROJECTPROPNAME_LOGFILELISTLANG, "false") && bRet;
	if (lProjectLanguage)
	{
		sprintf_s(buf, sizeof(buf), "%ld", lProjectLanguage);
		bRet = props.Add(PROJECTPROPNAME_PROJECTLANGUAGE, buf) && bRet;
	}
	if (!sFPPath.IsEmpty())
		bRet = props.Add(PROJECTPROPNAME_USERFILEPROPERTY, CUnicodeUtils::StdGetUTF8((LPCTSTR)sFPPath)) && bRet;
	if (!sDPPath.IsEmpty())
		bRet = props.Add(PROJECTPROPNAME_USERDIRPROPERTY, CUnicodeUtils::StdGetUTF8((LPCTSTR)sDPPath)) && bRet;
	if (!sWebViewerRev.IsEmpty())
		bRet = props.Add(PROJECTPROPNAME_WEBVIEWER_REV, CUnicodeUtils::StdGetUTF8((LPCTSTR)sWebViewerRev)) && bRet;
	if (!sWebViewerPathRev.IsEmpty())
		bRet = props.Add(PROJECTPROPNAME_WEBVIEWER_PATHREV, CUnicodeUtils::StdGetUTF8((LPCTSTR)sWebViewerPathRev)) && bRet;
	if (!sAutoProps.IsEmpty())
		bRet = props.Add(PROJECTPROPNAME_AUTOPROPS, CUnicodeUtils::StdGetUTF8((LPCTSTR)sAutoProps)) && bRet;
	return bRet;
}

CString ProjectProperties::GetLogSummary(const CString& sMessage)
{
	restring reMsg = restring(sMessage);
	CString sRet;
	if (bHasLogSummaryPattern)
	{
		try
		{
			match_results results;
			match_results::backref_type br;
			br = patLogSummary.match( reMsg, results, 0 );
			if( br.matched ) 
			{
				if (results.cbackrefs() > 0)
				{
					ATLTRACE(_T("matched id : %s\n"), results.backref(1).str().c_str());
					sRet = results.backref(1).str().c_str();
					sRet.Trim();
				}
			}
		}
		catch (bad_alloc) {}
		catch (bad_regexpr){}
	}
	return sRet;
}

#ifdef DEBUG
static class PropTest
{
public:
	PropTest()
	{
		CString msg = _T("this is a test logmessage: issue 222\nIssue #456, #678, 901  #456");
		size_t offset1 = 0;
		size_t offset2 = 0;
		CString sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
		CString sCheckRe = _T("[Ii]ssue #?(\\d+)(,? ?#?(\\d+))+");
		CString sBugIDRe = _T("(\\d+)");
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
				ATLTRACE(_T("matched : %s\n"), results.backref(0).str().c_str());
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
							ATLTRACE(_T("matched id : %s\n"), idresults.backref(0).str().c_str());
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
				ATLTRACE(_T("matched : %s\n"), results.backref(0).str().c_str());
				if( br.matched ) 
				{
					for (size_t i=1; i<results.cbackrefs(); ++i)
					{
						ATLTRACE(_T("matched id : %s\n"), results.backref(i).str().c_str());
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
		ProjectProperties props;
		props.sCheckRe = _T("PAF-[0-9]+");
		props.patCheckRe.init((LPCTSTR)props.sCheckRe, MULTILINE);
		props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
		CString sRet = props.FindBugID(_T("This is a test for PAF-88"));
		ATLASSERT(sRet.IsEmpty());
		props.sCheckRe = _T("[Ii]ssue #?(\\d+)");
		props.patCheckRe.init((LPCTSTR)props.sCheckRe, MULTILINE);
		sRet = props.FindBugID(_T("Testing issue #99"));
		sRet.Trim();
		ATLASSERT(sRet.Compare(_T("99"))==0);
		props.sCheckRe = _T("[Ii]ssues?:?(\\s*(,|and)?\\s*#\\d+)+");
		props.patCheckRe.init((LPCTSTR)props.sCheckRe, MULTILINE);
		props.sBugIDRe = _T("(\\d+)");
		props.patBugIDRe.init((LPCTSTR)props.sBugIDRe, MULTILINE);
		props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
		sRet = props.FindBugID(_T("This is a test for Issue #7463"));
		sRet.Trim();
		ATLASSERT(sRet.Compare(_T("7463"))==0);
		props.sCheckRe = _T("^\\[(\\d+)\\].*");
		props.patCheckRe.init((LPCTSTR)props.sCheckRe, MULTILINE);
		props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
		sRet = props.FindBugID(_T("[000815] some stupid programming error fixed"));
		sRet.Trim();
		ATLASSERT(sRet.Compare(_T("000815"))==0);
		props.sCheckRe = _T("\\[\\[(\\d+)\\]\\]\\]");
		props.patCheckRe.init((LPCTSTR)props.sCheckRe, MULTILINE);
		props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
		sRet = props.FindBugID(_T("test test [[000815]]] some stupid programming error fixed"));
		sRet.Trim();
		ATLASSERT(sRet.Compare(_T("000815"))==0);
	}
} PropTest;
#endif



