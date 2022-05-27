#include "TagsTreeView.h"
#include "TagsDlg.h"

using namespace TagsCommon;

LRESULT CTagsTreeView::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( m_pDlg )
    {
        if ( (uMsg == WM_LBUTTONDBLCLK) ||
             (uMsg == WM_KEYDOWN && wParam == VK_RETURN) )
        {
            HTREEITEM hItem = GetSelection();
            if ( hItem )
            {
                TVITEM tvi;
                TCHAR  szItemText[CTagsDlg::MAX_TAGNAME];

                szItemText[0] = 0;

                ::ZeroMemory(&tvi, sizeof(tvi));
                tvi.hItem = hItem;
                tvi.mask = TVIF_TEXT | TVIF_PARAM;
                tvi.pszText = szItemText;
                tvi.cchTextMax = sizeof(szItemText)/sizeof(szItemText[0]) - 1;

                GetItem(tvi);

                const tTagData* pTagData = (const tTagData *) tvi.lParam;
                if ( pTagData )
                {
                    m_pDlg->PostMessage(CTagsDlg::WM_TAGDBLCLICKED, 0, (LPARAM) pTagData);
                }

                if ( (uMsg != WM_LBUTTONDBLCLK) || !m_pDlg->GetOptions().getBool(CTagsDlg::OPT_VIEW_DBLCLICKTREE) )
                    return 0;
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
                        TVHITTESTINFO hitInfo = { 0 };

                        hitInfo.pt = pt;
                        HTREEITEM hItem = HitTest(hitInfo);
                        if ( hItem )
                        {
                            LPNMTTDISPINFO lpnmdi = (LPNMTTDISPINFO) lParam;
                            SendMessage(lpnmdi->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, 600);

                            const tTagData* pTagData = (const tTagData *) GetItemData(hItem);
                            if ( pTagData )
                            {
                                m_lastTooltipText = getTooltip(pTagData);
                                lpnmdi->lpszText = const_cast<TCHAR*>(m_lastTooltipText.c_str());
                            }
                            else
                            {
                                m_lastTooltipText = GetItemText(hItem);
                                lpnmdi->lpszText = const_cast<TCHAR*>(m_lastTooltipText.c_str());
                            }

                            return 0;
                        }
                    }
                }
            }
            /*
            else if ( ((LPNMHDR) lParam)->code == TTN_SHOW )
            {
                HWND hToolTips = GetToolTips();
                if ( hToolTips )
                {
                    CPoint pt = Win32xx::GetCursorPos();

                    if ( ScreenToClient(pt) )
                    {
                        TVHITTESTINFO hitInfo = { 0 };
                
                        hitInfo.pt = pt;
                        HTREEITEM hItem = HitTest(hitInfo);
                        if ( hItem )
                        {
                            RECT rc;
                            GetItemRect(hItem, rc, FALSE);

                            LONG height = rc.bottom - rc.top;
                            rc.top += height;
                            rc.bottom += height;
                            rc.right = rc.left + 600;

                            RECT rc2 = GetClientRect();
                            ::SendMessage( hToolTips, TTM_ADJUSTRECT, FALSE, (LPARAM) &rc );
                        }
                    }
                }
            }
            */
        }
        else if ( uMsg == WM_RBUTTONDOWN )
        {
            BOOL bItemHasChildren = FALSE;
            HTREEITEM hItem = GetSelection();
            if ( hItem )
            {
                bItemHasChildren = ItemHasChildren(hItem);
            }

            HMENU hPopupMenu = ::GetSubMenu(m_pDlg->m_hMenu, bItemHasChildren ? 1 : 0);
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
