#pragma once


/**
 * \ingroup TortoiseMerge
 * Extends the MFC CSplitterWnd with the functionality to
 * Show/Hide specific columns and rows, allows to lock
 * the splitter bars so the user can't move them
 * and also allows dynamic replacing of views with
 * other views.
 *
 * \par requirements
 * win98 or later\n
 * win2k or later\n
 * MFC\n
 *
 */
class CXSplitter : public CSplitterWnd
{
public:
	CXSplitter();
	virtual ~CXSplitter();

public:
	/**
	 * Checks if the splitter has its bars locked.
	 */
	BOOL		IsBarLocked() {return m_bBarLocked;}
	/**
	 * Locks/Unlocks the bar so the user can't move it.
	 * \param bState TRUE to lock, FALSE to unlock
	 */
	void		LockBar(BOOL bState=TRUE) {m_bBarLocked=bState;}
	/**
	 * Replaces a view in the Splitter with another view.
	 */
	BOOL		ReplaceView(int row, int col,CRuntimeClass * pViewClass, SIZE size);
    /**
     * Shows a splitter column which was previously hidden. Don't call
	 * this method if the column is already visible! Check it first
	 * with IsColumnHidden()
     */
    void		ShowColumn();
    /**
     * Hides the given splitter column. Don't call this method on already hidden columns!
	 * Check it first with IsColumnHidden()
     * \param nColHide The column to hide
     */
    void		HideColumn(int nColHide);
	/**
	 * Checks if a given column is hidden.
	 */
	BOOL		IsColumnHidden(int nCol){return (m_nHiddenCol == nCol);}
    /**
     * Shows a splitter row which was previously hidden. Don't call
	 * this method if the row is already visible! Check it first
	 * with IsRowHidden()
     */
    void		ShowRow();
    /**
     * Hides the given splitter row. Don't call this method on already hidden rows!
	 * Check it first with IsRowHidden()
     * \param nRowHide The row to hide
     */
    void		HideRow(int nRowHide);
	/**
	 * Checks if a given row is hidden.
	 */
	BOOL		IsRowHidden(int nRow){return (m_nHiddenRow == nRow);}

protected:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	DECLARE_MESSAGE_MAP()

private:
	BOOL		m_bBarLocked;	///< is the splitter bar locked?
    int			m_nHiddenCol;   ///< Index of the hidden column.
    int			m_nHiddenRow;   ///< Index of the hidden row.

};
