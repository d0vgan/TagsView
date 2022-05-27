#include "TagsListView.h"
#include "TagsDlg.h"

using namespace TagsCommon;

LRESULT CTagsListView::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( m_pDlg )
    {
        if ( (uMsg == WM_LBUTTONDBLCLK) ||
             (uMsg == WM_KEYDOWN && wParam == VK_RETURN) )
        {
            int iItem = GetSelectionMark();
            if ( iItem >= 0 )
            {
                int state = GetItemState(iItem, LVIS_FOCUSED | LVIS_SELECTED);
                if ( state & (LVIS_FOCUSED | LVIS_SELECTED) )
                {
                    LVITEM lvi;
                    TCHAR szItemText[CTagsDlg::MAX_TAGNAME];

                    szItemText[0] = 0;

                    ::ZeroMemory(&lvi, sizeof(lvi));
                    lvi.mask = LVIF_TEXT | LVIF_PARAM;
                    lvi.iItem = iItem;
                    lvi.pszText = szItemText;
                    lvi.cchTextMax = sizeof(szItemText)/sizeof(szItemText[0]) - 1;

                    GetItem(lvi);

                    SetItemState(iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                    SetSelectionMark(iItem);

                    const tTagData* pTagData = (const tTagData *) lvi.lParam;
                    if ( pTagData )
                    {
                        m_pDlg->PostMessage(CTagsDlg::WM_TAGDBLCLICKED, 0, (LPARAM) pTagData);

                        LRESULT lResult = 0;
                        if ( uMsg == WM_LBUTTONDBLCLK )
                        {
                            lResult = WndProcDefault(uMsg, wParam, lParam);
                        }
                        return lResult;
                    }
                }
            }
        }
        else if ( uMsg == WM_CHAR )
        {
            m_pDlg->m_edFilter.SetFocus();
            m_pDlg->m_edFilter.DirectMessage(uMsg, wParam, lParam);
            return 0;
        }
        else if ( uMsg == WM_KEYDOWN )
        {
            switch ( wParam )
            {
                case VK_ESCAPE:
                    m_pDlg->m_edFilter.DirectMessage(uMsg, wParam, lParam);
                    return 0;
            }
        }
        else if ( uMsg == WM_NOTIFY )
        {
            if ( ((LPNMHDR) lParam)->code == TTN_GETDISPINFO )
            {
                if ( m_pDlg->GetOptions().getBool(CTagsDlg::OPT_VIEW_SHOWTOOLTIPS) )
                {
                    CPoint pt = Win32xx::GetCursorPos();
                    if ( ScreenToClient(pt) )
                    {
                        UINT uFlags = 0;
                        int nItem = HitTest(pt, &uFlags);
                        if ( nItem != -1 )
                        {
                            LPNMTTDISPINFO lpnmdi = (LPNMTTDISPINFO) lParam;
                            SendMessage(lpnmdi->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, 600);

                            const tTagData* pTagData = (const tTagData *) GetItemData(nItem);
                            if ( pTagData )
                            {
                                m_lastTooltipText = getTooltip(pTagData);
                                lpnmdi->lpszText = const_cast<TCHAR*>(m_lastTooltipText.c_str());
                            }
                            else
                            {
                                m_lastTooltipText = GetItemText(nItem, 0);
                                lpnmdi->lpszText = const_cast<TCHAR*>(m_lastTooltipText.c_str());
                            }

                            return 0;
                        }
                    }
                }
            }
        }
        else if ( uMsg == WM_SETFOCUS )
        {
            bool isSelected = false;
            int iItem = GetSelectionMark();
            if ( iItem >= 0 )
            {
                int state = GetItemState(iItem, LVIS_FOCUSED | LVIS_SELECTED);
                if ( state & (LVIS_FOCUSED | LVIS_SELECTED) )
                {
                    isSelected = true;
                }
            }
            if ( !isSelected )
            {
                if ( GetItemCount() > 0 )
                {
                    SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                    SetSelectionMark(0);
                }
            }
        }
        else if ( uMsg == WM_RBUTTONDOWN )
        {
            HMENU hPopupMenu = ::GetSubMenu(m_pDlg->m_hMenu, 2);
            if ( hPopupMenu )
            {
                POINT pt = { 0, 0 };
                ::GetCursorPos(&pt);
                ::TrackPopupMenuEx(hPopupMenu, 0, pt.x, pt.y, m_pDlg->GetHwnd(), NULL);
            }
            return 0;
        }
        else if ( uMsg == WM_RBUTTONUP )
        {
            return 0;
        }
    }

    return WndProcDefault(uMsg, wParam, lParam);
}

