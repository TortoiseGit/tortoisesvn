#pragma once
#include "Balloon.h"

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

namespace LogCache
{
    struct CLogCacheStatisticsData;
}


class CLogCacheStatisticsDlg : public CDialog
{
	DECLARE_DYNAMIC(CLogCacheStatisticsDlg)

public:
    CLogCacheStatisticsDlg (const LogCache::CLogCacheStatisticsData& data, CWnd * pParentWnd = NULL); 
	virtual ~CLogCacheStatisticsDlg();

	enum { IDD = IDD_LOGCACHESTATISTICS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX); 
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()

private:

    CString sizeRAM;
    CString sizeDisk;
    CString connectionState;
    CString lastRead;
    CString lastWrite;
    CString lastHeadUpdate;
    CString authors;
    CString paths;
    CString pathElements;
    CString skipRanges;
    CString wordTokens;
    CString pairTokens;
    CString textSize;
    CString uncompressedSize;
    CString maxRevision;
    CString revisionCount;
    CString changesTotal;
    CString changedRevisions;
    CString changesMissing;
    CString mergesTotal;
    CString mergesRevisions;
    CString mergesMissing;
    CString userRevpropsTotal;
    CString userRevpropsRevisions;
    CString userRevpropsMissing;

    CString DateToString (__time64_t time);
    CString ToString (__int64 value);

	CBalloon m_tooltips;
};
