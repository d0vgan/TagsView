#include "TagsFilterEdit.h"
#include "TagsDlg.h"

using namespace TagsCommon;

namespace
{
    void cutEditText(HWND hEdit, BOOL bCutAfterCaret)
    {
        DWORD   dwSelPos1 = 0;
        DWORD   dwSelPos2 = 0;
        UINT    len = 0;

        ::SendMessage( hEdit, EM_GETSEL, (WPARAM) &dwSelPos1, (LPARAM) &dwSelPos2 );

        len = (UINT) ::GetWindowTextLength(hEdit);

        if ( bCutAfterCaret )
        {
            if ( dwSelPos1 < len )
            {
                ::SendMessage( hEdit, EM_SETSEL, dwSelPos1, -1 );
                ::SendMessage( hEdit, EM_REPLACESEL, TRUE, (LPARAM) _T("\0") );
                ::SendMessage( hEdit, EM_SETSEL, dwSelPos1, dwSelPos1 );
            }
        }
        else
        {
            if ( dwSelPos2 > 0 )
            {
                if ( dwSelPos2 > len )
                    dwSelPos2 = len;

                ::SendMessage( hEdit, EM_SETSEL, 0, dwSelPos2 );
                ::SendMessage( hEdit, EM_REPLACESEL, TRUE, (LPARAM) _T("\0") );
                ::SendMessage( hEdit, EM_SETSEL, 0, 0 );
            }
        }
    }
}

LRESULT CTagsFilterEdit::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( m_pDlg )
    {
        bool bProcessLocal = false;

        if ( uMsg == WM_CHAR )
        {
            if ( wParam == 0x7F ) // 0x7F is Ctrl+BS, that's we all love M$ for!
            {
                return 0;
            }
        }

        if ( uMsg == WM_KEYDOWN )
        {
            switch ( wParam )
            {
                case VK_DELETE:
                case VK_BACK:
                    if ( ::GetKeyState(VK_CONTROL) & 0x80 ) // Ctrl+Del, Ctrl+BS
                    {
                        cutEditText( GetHwnd(), (wParam == VK_DELETE) );
                        bProcessLocal = true;
                    }
                    break;

                case VK_ESCAPE:
                    ::SetWindowText( GetHwnd(), _T("") );
                    bProcessLocal = true;
                    break;
            }
        }

        if ( bProcessLocal || (uMsg == WM_CHAR) ||
             ((uMsg == WM_KEYDOWN) && (wParam == VK_DELETE)) )
        {
            LRESULT  lResult;
            TCHAR    szText[CTagsDlg::MAX_TAGNAME];
            t_string tagFilter;

            lResult = bProcessLocal ? 0 : WndProcDefault(uMsg, wParam, lParam);
            szText[0] = 0;
            ::GetWindowText(GetHwnd(), szText, CTagsDlg::MAX_TAGNAME - 1);
            tagFilter = szText;
            if ( tagFilter != m_pDlg->m_tagFilter )
            {
                m_pDlg->m_tagFilter = tagFilter;
                m_pDlg->UpdateTagsView();
            }

            return lResult;
        }

        /*
        if ( uMsg == WM_PAINT )
        {
            m_pDlg->ApplyColors();
        }
        */
    }

    return WndProcDefault(uMsg, wParam, lParam);
}
