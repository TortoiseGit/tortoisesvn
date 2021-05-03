// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2013, 2020-2021 - TortoiseSVN

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
#pragma once

#include "StandAloneDlg.h"
#include "MyGraph.h"
#include "TSVNPath.h"
#include "UnicodeUtils.h"
#include "MiscUI\ThemeControls.h"

#include <map>
#include <list>

/**
 * \ingroup TortoiseProc
 * Helper class for drawing and then saving the drawing to a meta file (wmf)
 */
class CMyMetaFileDC : public CMetaFileDC
{
public:
    HGDIOBJ SelectObject(HGDIOBJ hObject) const
    {
        return (hObject != nullptr) ? ::SelectObject(m_hDC, hObject) : nullptr;
    }
};

/**
 * \ingroup TortoiseProc
 * Helper dialog showing statistics gathered from the log messages shown in the
 * log dialog.
 *
 * The function GatherData() collects statistical information and stores it
 * in the corresponding member variables. You can access the data as shown in
 * the following examples:
 * @code
 *    commits = m_commitsPerWeekAndAuthor[week_nr][author_name];
 *    filechanges = m_filechangesPerWeekAndAuthor[week_nr][author_name];
 *    commits = m_commitsPerAuthor[author_name];
 * @endcode

 */
class CStatGraphDlg : public CResizableStandAloneDialog //CResizableStandAloneDialog
{
    DECLARE_DYNAMIC(CStatGraphDlg)

public:
    CStatGraphDlg(CWnd* pParent = nullptr);
    ~CStatGraphDlg() override;

    enum
    {
        IDD = IDD_STATGRAPH
    };

    // Data passed from the caller of the dialog.
    CDWordArray*  m_parDates;
    CDWordArray*  m_parFileChanges;
    CStringArray* m_parAuthors;
    CTSVNPath     m_path;

protected:
    // ** Constants **
    static const long int SECONDS_IN_WEEK   = 604800; // ... a week has 604800 seconds
    static const long int SECONDS_IN_DAY    = 86400;  // ... a day has 86400.0 seconds
    static const int      COEFF_AUTHOR_SHIP = 2;

    // ** Data types **

    /// The types of units used in the various graphs.
    enum UnitType
    {
        Days,
        Weeks,
        Months,
        Quarters,
        Years
    };

    // Available next metrics
    enum Metrics
    {
        TextStatStart,
        AllStat,
        TextStatEnd,
        GraphicStatStart,
        PercentageOfAuthorship,
        CommitsByAuthor,
        CommitsByDate,
        GraphicStatEnd,
    };

    // *** Re-implemented member functions from CDialog
    void         OnOK() override;
    void         OnCancel() override;
    void         DoDataExchange(CDataExchange* pDX) override;
    BOOL         OnInitDialog() override;
    BOOL         PreTranslateMessage(MSG* pMsg) override;
    void         ShowLabels(BOOL bShow);
    afx_msg void OnCbnSelchangeGraphcombo();
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnNeedText(NMHDR* pnmh, LRESULT* pResult);
    afx_msg void OnBnClickedGraphbarbutton();
    afx_msg void OnBnClickedGraphbarstackedbutton();
    afx_msg void OnBnClickedGraphlinebutton();
    afx_msg void OnBnClickedGraphlinestackedbutton();
    afx_msg void OnBnClickedGraphpiebutton();
    afx_msg void OnFileSavestatgraphas();
    DECLARE_MESSAGE_MAP()

    // ** Member functions **

    /// Updates the variables m_nWeeks, m_nDays and m_minDate
    void UpdateWeekCount();
    /// Returns the week-of-the-year for the given time.
    static int GetCalendarWeek(const CTime& time);
    /// Parses the data given to the dialog and generates mappings with statistical data.
    void GatherData();
    /// Populates the lists passed as arguments based on the commit threshold set with the skipper.
    void FilterSkippedAuthors(std::list<std::wstring>& includedAuthors, std::list<std::wstring>& skippedAuthors);
    /// Shows the graph Percentage Of Authorship
    void ShowPercentageOfAuthorship();
    /// Shows the graph with commit counts per author.
    void ShowCommitsByAuthor();
    /// Shows the graph with commit counts per author and date.
    void ShowCommitsByDate();
    /// Shows the initial statistics page.
    void ShowStats();

    /// Rolling Percentage Of Authorship of author to integer
    static int RollPercentageOfAuthorship(double it);

    /// Load list of drawing authors
    template <class Map>
    void LoadListOfAuthors(Map& map, bool reloadSkiper = false, bool compare = false);

    // If we have other authors, count them and their commits.
    template <class Map>
    void DrawOthers(const std::list<std::wstring>& others, MyGraphSeries* graphData, Map& map);

    /// Called when user checks/unchecks the "Authors case sensitive" checkbox.
    /// Recalculates statistical data because the number and names of authors
    /// can have changed. Also calls RedrawGraph().
    void AuthorsCaseSensitiveChanged();
    /// Called when user checks/unchecks the "Sort by commit count" checkbox.
    /// Calls RedrawGraph().
    void SortModeChanged();
    /// Clears the current graph and frees all data series.
    void ClearGraph();
    /// Updates the currently shown statistics page.
    void RedrawGraph();

    /// PreShow Statistic function
    bool PreViewStat(bool fShowLabels);
    /// PreShow Graphic function
    MyGraphSeries* PreViewGraph(__in UINT graphTitle, __in UINT yAxisLabel, __in UINT xAxisLabel = NULL);
    /// Show Selected Static metric
    void ShowSelectStat(Metrics selectedMetric, bool reloadSkiper = false);

    int                     GetUnitCount() const;
    int                     GetUnit(const CTime& time) const;
    CStatGraphDlg::UnitType GetUnitType() const;
    CString                 GetUnitString() const;
    CString                 GetUnitsString() const;
    CString                 GetUnitLabel(int unit, CTime& lasttime) const;

    void EnableDisableMenu() const;

    void SaveGraph(CString sFilename);
    int  GetEncoderClsid(const WCHAR* format, CLSID* pClsid) const;

    void StoreCurrentGraphType();
    void ShowErrorMessage();

    // init ruler & limit its range
    void SetSkipper(bool reloadSkiper);

    //Load statistical queries
    void LoadStatQueries(__in UINT curStr, Metrics loadMetric, bool setDef = false);

    //Considers coefficient contribution author
    static double CoeffContribution(int distFromEnd);

    CPtrArray   m_graphDataArray;
    MyGraph     m_graph;
    CComboBox   m_cGraphType;
    CSliderCtrl m_skipper;
    BOOL        m_bAuthorsCaseSensitive;
    BOOL        m_bSortByCommitCount;

    CThemeMFCButton m_btnGraphBar;
    CThemeMFCButton m_btnGraphBarStacked;
    CThemeMFCButton m_btnGraphLine;
    CThemeMFCButton m_btnGraphLineStacked;
    CThemeMFCButton m_btnGraphPie;

    MyGraph::GraphType m_graphType;
    bool               m_bStacked;

    CToolTipCtrl* m_pToolTip;

    int m_langOrder;

    // ** Member variables holding the statistical data **

    /// Number of days in the revision interval.
    int m_nDays;
    /// Number of weeks in the revision interval.
    int m_nWeeks;
    /// The starting date/time for the revision interval.
    __time64_t m_minDate;
    /// The ending date/time for the revision interval.
    __time64_t m_maxDate;
    /// The total number of commits (equals size of the m_parXXX arrays).
    INT_PTR m_nTotalCommits;
    /// The total number of file changes.
    LONG m_nTotalFileChanges;
    /// Holds the number of commits per unit and author.
    std::map<int, std::map<std::wstring, LONG>> m_commitsPerUnitAndAuthor;
    /// Holds the number of file changes per unit and author.
    std::map<int, std::map<std::wstring, LONG>> m_filechangesPerUnitAndAuthor;
    /// First interval number (key) in the mappings.
    int m_firstInterval;
    /// Last interval number (key) in the mappings.
    int m_lastInterval;
    /// Mapping of total commits per author, access data via
    std::map<std::wstring, LONG> m_commitsPerAuthor;
    /// Mapping of Percentage Of Authorship per author
    std::map<std::wstring, double> m_percentageOfAuthorship;

    /// The list of author names sorted based on commit count
    /// (author with most commits is first in list).
    std::list<std::wstring> m_authorNames;
    /// unit names by week/month/quarter
    std::map<LONG, std::wstring> m_unitNames;
};
