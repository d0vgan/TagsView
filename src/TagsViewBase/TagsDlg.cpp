#include "TagsDlg.h"
#include "SysUniConv.h"
#include "ConsoleOutputRedirector.h"
#include "resource.h"

typedef struct sCTagsThreadParam {
    DWORD     dwThreadID;
    CTagsDlg* pDlg;
    tString   cmd_line;
    tString   source_file_name;
    tString   temp_file_name;
    BOOL      isUTF8;
} tCTagsThreadParam;

class string_cmp_less
{
    public:
        bool operator() (const string& s1, const string& s2) const
        {
            return (lstrcmpiA(s1.c_str(), s2.c_str()) < 0);
        }
};

static bool isFilePathExist(LPCTSTR pszFilePath, bool canBeDirectory = true)
{
    if ( pszFilePath && pszFilePath[0] )
    {
        WIN32_FIND_DATA ffd;
        HANDLE hFile = ::FindFirstFile( pszFilePath, &ffd );
        if ( hFile && (hFile != INVALID_HANDLE_VALUE) )
        {
            bool isNotDirectory = ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY);
            ::FindClose(hFile);
            return (canBeDirectory ? true : isNotDirectory);
        }
    }

    return false;
}

static LPCTSTR getFileName(LPCTSTR pszFilePathName)
{
    if ( pszFilePathName && *pszFilePathName )
    {
        int n = lstrlen(pszFilePathName);
        while ( --n >= 0 )
        {
            if ( (pszFilePathName[n] == _T('\\')) || (pszFilePathName[n] == _T('/')) )
            {
                break;
            }
        }

        if ( n >= 0 )
        {
            ++n;
            pszFilePathName += n;
        }
    }
    return pszFilePathName;
}

static void cutEditText(HWND hEdit, BOOL bCutAfterCaret)
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
                        cutEditText( this->GetHwnd(), (wParam == VK_DELETE) );
                        bProcessLocal = true;
                    }
                    break;

                case VK_ESCAPE:
                    ::SetWindowText(GetHwnd(), _T(""));
                    bProcessLocal = true;
                    break;
            }
        }

        if ( bProcessLocal || (uMsg == WM_CHAR) ||
             ((uMsg == WM_KEYDOWN) && (wParam == VK_DELETE)) )
        {
            LRESULT lResult;
            TCHAR   szText[CTagsDlg::MAX_TAGNAME];
            string  tagFilter;

            lResult = bProcessLocal ? 0 : WndProcDefault(uMsg, wParam, lParam);
            szText[0] = 0;
            ::GetWindowText(GetHwnd(), szText, CTagsDlg::MAX_TAGNAME - 1);
            #ifdef UNICODE
                char szTextA[CTagsDlg::MAX_TAGNAME];
                int len = SysUniConv::UnicodeToMultiByte(szTextA, CTagsDlg::MAX_TAGNAME - 1, szText);
                szTextA[len] = 0;
                tagFilter = szTextA;
            #else
                tagFilter = szText;
            #endif
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

LRESULT CTagsListView::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( m_pDlg )
    {
        if ( (uMsg == WM_LBUTTONDBLCLK) ||
             (uMsg == WM_KEYDOWN && wParam == VK_RETURN) )
        {
            if ( m_pDlg->m_pEdWr )
            {
                int iItem = m_pDlg->m_lvTags.GetSelectionMark();
                if ( iItem >= 0 )
                {
                    int state = m_pDlg->m_lvTags.GetItemState(iItem, LVIS_FOCUSED | LVIS_SELECTED);
                    if ( state & (LVIS_FOCUSED | LVIS_SELECTED) )
                    {
                        LVITEM lvi = { 0 };
                        TCHAR szItemText[CTagsDlg::MAX_TAGNAME];

                        szItemText[0] = 0;

                        lvi.mask = LVIF_TEXT | LVIF_PARAM;
                        lvi.iItem = iItem;
                        lvi.pszText = szItemText;
                        lvi.cchTextMax = sizeof(szItemText)/sizeof(szItemText[0]) - 1;

                        m_pDlg->m_lvTags.GetItem(lvi);

                        m_pDlg->m_lvTags.SetItemState(iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                        m_pDlg->m_lvTags.SetSelectionMark(iItem);

                        const CTagsResultParser::tTagData* pTagData =
                          (const CTagsResultParser::tTagData *) lvi.lParam;
                        if ( pTagData )
                        {
                            const int line = pTagData->line;
                            if ( line >= 0 )
                            {
                                m_pDlg->m_pEdWr->ewSetNavigationPoint( _T("") );
                                //m_pDlg->UpdateNavigationButtons();

                                int pos = m_pDlg->m_pEdWr->ewGetPosFromLine(line - 1);
                                m_pDlg->m_prevSelStart = pos;
                                m_pDlg->m_pEdWr->ewDoSetSelection(pos, pos);

                                m_pDlg->m_pEdWr->ewSetNavigationPoint( _T("") );
                                m_pDlg->UpdateNavigationButtons();

                                LRESULT lResult = 0;
                                if ( uMsg == WM_LBUTTONDBLCLK )
                                {
                                    lResult = WndProcDefault(uMsg, wParam, lParam);
                                }
                                m_pDlg->m_pEdWr->ewDoSetFocus();
                                return lResult;
                            }
                        }
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
        /*else if ( uMsg == WM_NOTIFY )
        {
            if ( ((LPNMHDR) lParam)->code == TTN_GETDISPINFO )
            {
                LPNMTTDISPINFO lpnmdi = (LPNMTTDISPINFO) lParam;
                SendMessage(lpnmdi->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, 300);
                lpnmdi->lpszText = (LPTSTR) str;



                   1. Call CWnd::EnableToolTips(TRUE) during the control initialization, which for the CListCtrl means over-defining CWnd::PreSubclassWindow().
   2. Over-define CWnd::OnToolHitTest(...) for the CListCtrl. It will be called every time the mouse moves over the control, and the return value of the function specifies whether a tooltip should be displayed.
   3. Implement a TTN_NEEDTEXT message handler. It will be called when the mouse is over the control and the previous method has returned that a tooltip is available.

The third step is only necessary if the second step specifies that the tooltip text should be retrieved using the callback (LPSTR_TEXTCALLBACK). The reason for doing a callback is only speed consideration, in case the lookup of the tooltip text is slow. Instead of specifying a callback, we can just return the tooltip text directly by performing a malloc() to hold the text (it will be automatically deallocated) and enables tooltip text beyond 80 characters.

If using the TTN_NEEDTEXT message handler and one wants to display tooltip longer than 80 characters, then one must allocate the wanted text buffer and set the TOOLTIPTEXT::lpszText pointer to this text-buffer in the message handler (one has to deallocate this text buffer manually)



                return 0;
            }
        }*/
        /*
        else if ( uMsg == WM_PAINT )
        {
            m_pDlg->ApplyColors();
        }
        */
        else if ( uMsg == WM_SETFOCUS )
        {
            bool isSelected = false;
            int iItem = m_pDlg->m_lvTags.GetSelectionMark();
            if ( iItem >= 0 )
            {
                int state = m_pDlg->m_lvTags.GetItemState(iItem, LVIS_FOCUSED | LVIS_SELECTED);
                if ( state & (LVIS_FOCUSED | LVIS_SELECTED) )
                {
                    isSelected = true;
                }
            }
            if ( !isSelected )
            {
                if ( m_pDlg->m_lvTags.GetItemCount() > 0 )
                {
                    m_pDlg->m_lvTags.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                    m_pDlg->m_lvTags.SetSelectionMark(0);
                }
            }
        }
    }

    return WndProcDefault(uMsg, wParam, lParam);
}


LRESULT CTagsTreeView::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( m_pDlg )
    {
        if ( (uMsg == WM_LBUTTONDBLCLK) ||
             (uMsg == WM_KEYDOWN && wParam == VK_RETURN) )
        {
            if ( m_pDlg->m_pEdWr )
            {
                HTREEITEM hItem = m_pDlg->m_tvTags.GetSelection();
                if ( hItem )
                {
                    TVITEM tvi = { 0 };
                    TCHAR szItemText[CTagsDlg::MAX_TAGNAME];

                    szItemText[0] = 0;

                    tvi.hItem = hItem;
                    tvi.mask = TVIF_TEXT | TVIF_PARAM;
                    tvi.pszText = szItemText;
                    tvi.cchTextMax = sizeof(szItemText)/sizeof(szItemText[0]) - 1;

                    m_pDlg->m_tvTags.GetItem(tvi);

                    const CTagsResultParser::tTagData* pTagData =
                      (const CTagsResultParser::tTagData *) tvi.lParam;
                    if ( pTagData )
                    {
                        const int line = pTagData->line;
                        if ( line >= 0 )
                        {
                            m_pDlg->m_pEdWr->ewSetNavigationPoint( _T("") );
                            //m_pDlg->UpdateNavigationButtons();

                            int pos = m_pDlg->m_pEdWr->ewGetPosFromLine(line - 1);
                            m_pDlg->m_prevSelStart = pos;
                            m_pDlg->m_pEdWr->ewDoSetSelection(pos, pos);

                            m_pDlg->m_pEdWr->ewSetNavigationPoint( _T("") );
                            m_pDlg->UpdateNavigationButtons();

                            LRESULT lResult = 0;
                            if ( uMsg == WM_LBUTTONDBLCLK )
                            {
                                lResult = WndProcDefault(uMsg, wParam, lParam);
                            }
                            m_pDlg->m_pEdWr->ewDoSetFocus();
                            return lResult;
                        }
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
        /*
        else if ( uMsg == WM_PAINT )
        {
            m_pDlg->ApplyColors();
        }
        */
    }

    return WndProcDefault(uMsg, wParam, lParam);
}

void CTagsToolTips::AddTool(CWnd& wnd)
{
    static unsigned int uId = 0;
    RECT rect;
    TOOLINFO ti;

    ::GetClientRect(wnd.GetHwnd(), &rect);

    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_SUBCLASS;
    ti.hwnd = wnd.GetHwnd();
    ti.hinst = m_pDlg->GetInstDll();
    ti.uId = uId;
    ti.lpszText = LPSTR_TEXTCALLBACK;
    // ToolTip control will cover the whole window
    ti.rect.left = rect.left;
    ti.rect.top = rect.top;
    ti.rect.right = rect.right;
    ti.rect.bottom = rect.bottom;
    ti.lParam = 0;

    ::SendMessage( GetHwnd(), TTM_ADDTOOL, 0, (LPARAM) &ti );

    ++uId;
}

const TCHAR* CTagsDlg::cszListViewColumnNames[LVC_TOTAL] = {
    _T("Name"),
    _T("Type"),
    _T("Line")
};

/*static int CALLBACK ListViewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    return ::CompareString(
        LOCALE_USER_DEFAULT, NORM_IGNORECASE,
        (LPCTSTR) lParam1, -1,
        (LPCTSTR) lParam2, -1 );
}*/

CTagsDlg::CTagsDlg() : CDialog(IDD_MAIN)
, m_viewMode(TVM_NONE)
, m_sortMode(TSM_NONE)
, m_dwLastTagsThreadID(0)
, m_isUTF8tags(false)
, m_optRdWr(NULL)
, m_pEdWr(NULL)
, m_prevSelStart(-1)
, m_nTagsThreadCount(0)
, m_hInstDll(NULL)
, m_crTextColor(0xFFFFFFFF)
, m_crBkgndColor(0xFFFFFFFF)
, m_hBkgndBrush(NULL)
{
    SetCTagsPath();
    m_hTagsThreadEvent = ::CreateEvent(NULL, TRUE, TRUE, NULL);
    ::SetEvent(m_hTagsThreadEvent);
}

CTagsDlg::~CTagsDlg()
{
    Destroy(); // to be sure GetHwnd() returns NULL

    while ( m_nTagsThreadCount > 0 )
    {
        // "freeze" until all threads are over
        ::Sleep(40);
    }

    ::CloseHandle(m_hTagsThreadEvent);

    if ( m_hBkgndBrush != NULL )
    {
        ::DeleteObject(m_hBkgndBrush);
    }
}

DWORD WINAPI CTagsDlg::CTagsThreadProc(LPVOID lpParam)
{
    CConsoleOutputRedirector cor;
    tCTagsThreadParam* tt = (tCTagsThreadParam *) lpParam;

    if ( tt->pDlg->GetHwnd() )
    {
        cor.Execute( tt->cmd_line.c_str() );
        if ( !tt->temp_file_name.empty() )
        {
            ::DeleteFile( tt->temp_file_name.c_str() );
        }

        ::WaitForSingleObject(tt->pDlg->m_hTagsThreadEvent, 4000);
        if ( tt->dwThreadID == tt->pDlg->m_dwLastTagsThreadID )
        {
            if ( tt->pDlg->GetHwnd() /*&& tt->pDlg->IsWindowVisible()*/ )
            {
                const CEditorWrapper* pEdWr = tt->pDlg->m_pEdWr;
                if ( pEdWr && (pEdWr->ewGetFilePathName() == tt->source_file_name) )
                    tt->pDlg->OnAddTags( cor.GetDataString(), tt->isUTF8 ? true : false );
            }
        }
    }

    ::InterlockedDecrement(&tt->pDlg->m_nTagsThreadCount);
    delete tt;

    return 0;
}

INT_PTR CTagsDlg::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        /*case WM_ADDTAGS:
            OnAddTags( *((const string *) lParam) );
            return 0;*/

        case WM_UPDATETAGSVIEW:
            UpdateTagsView();
            return 0;

        case WM_CTAGSPATHFAILED:
            if ( m_pEdWr )
            {
                tString err = _T("File \'ctags.exe\' was not found. The path is incorrect:\n");
                err += m_ctagsPath;
                ::MessageBox( m_pEdWr->ewGetMainHwnd(), err.c_str(), _T("TagsView Error"), MB_OK | MB_ICONERROR );
            }
            return 0;

        case WM_CTLCOLOREDIT:
            if ( GetOptions().getBool(OPT_COLORS_USEEDITORCOLORS) )
            {
                return OnCtlColorEdit(wParam, lParam);
            }
            break;

        case WM_SIZE:
            OnSize();
            break;

        case WM_INITDIALOG:
            return OnInitDialog();
    }

    return DialogProcDefault(uMsg, wParam, lParam);
}

void CTagsDlg::EndDialog(INT_PTR nResult)
{
    CDialog::EndDialog(nResult);

    if ( m_pEdWr )
    {
        m_pEdWr->ewOnTagsViewClose();
    }
}

void CTagsDlg::OnAddTags(const string& s, bool isUTF8)
{
    CTagsResultParser::Parse( s, m_tags );
    m_isUTF8tags = isUTF8;
    // let's update tags view in the primary thread
    ::SendNotifyMessage( this->GetHwnd(), WM_UPDATETAGSVIEW, 0, 0 );
}

BOOL CTagsDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch ( LOWORD(wParam) )
    {
        case IDM_PREVPOS:
            OnPrevPosClicked();
            break;

        case IDM_NEXTPOS:
            OnNextPosClicked();
            break;

        case IDM_SORTLINE:
            SetSortMode(TSM_LINE);
            break;

        case IDM_SORTNAME:
            SetSortMode(TSM_NAME);
            break;

        case IDM_VIEWLIST:
            SetViewMode(TVM_LIST, m_sortMode);
            break;

        case IDM_VIEWTREE:
            SetViewMode(TVM_TREE, m_sortMode);
            break;

        case IDM_PARSE:
            OnParseClicked();
            break;

        case IDM_CLOSE:
            if ( m_pEdWr )
            {
                m_pEdWr->ewOnTagsViewClose();
            }
            break;
    }

    return FALSE;
}

void CTagsDlg::OnCancel()
{
    // don't close the dialog

    // we also get here when Esc is pressed ANYWHERE in the dialog
    // that's we all love M$ for!
    m_edFilter.DirectMessage(WM_KEYDOWN, VK_ESCAPE, 0);
}

static void fillToolInfo(
  TOOLINFO* lpti,
  LPTSTR    lpToolTipText,
  HWND      hWnd,
  HINSTANCE hInst)
{
    static unsigned int uId = 0;
    RECT rect;

    ::GetClientRect(hWnd, &rect);

    lpti->cbSize = sizeof(TOOLINFO);
    lpti->uFlags = TTF_SUBCLASS;
    lpti->hwnd = hWnd;
    lpti->hinst = hInst;
    lpti->uId = uId;
    lpti->lpszText = lpToolTipText;
    // ToolTip control will cover the whole window
    lpti->rect.left = rect.left;
    lpti->rect.top = rect.top;
    lpti->rect.right = rect.right;
    lpti->rect.bottom = rect.bottom;
    lpti->lParam = 0;

    ++uId;
}

void CTagsDlg::initTooltips()
{
    INITCOMMONCONTROLSEX iccex;

    iccex.dwICC = ICC_WIN95_CLASSES;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    ::InitCommonControlsEx( &iccex );

    m_ttHints.SetTagsDlg(this);
    m_ttHints.Create(GetHwnd());

    m_ttHints.SetWindowPos( HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );

    m_ttHints.AddTool( m_lvTags );

}

BOOL CTagsDlg::OnInitDialog()
{
    //initOptions();

    m_edFilter.SetTagsDlg(this);
    m_lvTags.SetTagsDlg(this);
    m_tvTags.SetTagsDlg(this);

    m_edFilter.Attach( ::GetDlgItem(GetHwnd(), IDC_ED_FILTER) );
    m_lvTags.Attach( ::GetDlgItem(GetHwnd(), IDC_LV_TAGS) );
    m_tvTags.Attach( ::GetDlgItem(GetHwnd(), IDC_TV_TAGS) );

    m_tbButtons.CreateEx(
        0, //WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE,
        TOOLBARCLASSNAME, _T(""),
        /*WS_TABSTOP | */ WS_VISIBLE | WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
        1, 1, 0, 0, GetHwnd(), NULL);

    const int nTbButtons = 8;
    const int nTbSeparators = 4;

    CBitmap tbBitmap(IDB_TOOLBAR);
    CSize tbBitmapSize = tbBitmap.GetSize();
    COLORREF mask = RGB(192, 192, 192);
    m_tbImageList.Create(tbBitmapSize.cx/nTbButtons, tbBitmapSize.cy, ILC_COLOR32 | ILC_MASK, nTbButtons, 0);
    m_tbImageList.Add(tbBitmap, mask);
    m_tbButtons.SetImageList(m_tbImageList);
    m_tbButtons.SetHotImageList(m_tbImageList);

    TBBUTTON tbButtonArray[nTbButtons + nTbSeparators];
    ::ZeroMemory(tbButtonArray, sizeof(tbButtonArray));

    auto setTbButton = [&tbButtonArray](int idx, int iBitmap, int idCommand, BYTE fsState, BYTE fsStyle)
    {
        tbButtonArray[idx].iBitmap = iBitmap;
        tbButtonArray[idx].idCommand = idCommand;
        tbButtonArray[idx].fsState = fsState;
        tbButtonArray[idx].fsStyle = fsStyle;
    };

    setTbButton(0,  0, IDM_PREVPOS,  0,               TBSTYLE_BUTTON);
    setTbButton(1,  1, IDM_NEXTPOS,  0,               TBSTYLE_BUTTON);
    setTbButton(2,  0, 0,            0,               TBSTYLE_SEP);
    setTbButton(3,  2, IDM_SORTLINE, TBSTATE_ENABLED, TBSTYLE_BUTTON);
    setTbButton(4,  3, IDM_SORTNAME, TBSTATE_ENABLED, TBSTYLE_BUTTON);
    setTbButton(5,  0, 0,            0,               TBSTYLE_SEP);
    setTbButton(6,  4, IDM_VIEWLIST, TBSTATE_ENABLED, TBSTYLE_BUTTON);
    setTbButton(7,  5, IDM_VIEWTREE, TBSTATE_ENABLED, TBSTYLE_BUTTON);
    setTbButton(8,  0, 0,            0,               TBSTYLE_SEP);
    setTbButton(9,  6, IDM_PARSE,    TBSTATE_ENABLED, TBSTYLE_BUTTON);
    setTbButton(10, 0, 0,            0,               TBSTYLE_SEP);
    setTbButton(11, 7, IDM_CLOSE,    TBSTATE_ENABLED, TBSTYLE_BUTTON);

    m_tbButtons.SendMessage(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    m_tbButtons.AddButtons(nTbButtons + nTbSeparators, tbButtonArray);

    m_lvTags.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_lvTags.InsertColumn( 0, cszListViewColumnNames[0], LVCFMT_LEFT, m_opt.getInt(OPT_VIEW_NAMEWIDTH) );
    m_lvTags.InsertColumn( 1, cszListViewColumnNames[1], LVCFMT_RIGHT, 60 );
    m_lvTags.InsertColumn( 2, cszListViewColumnNames[2], LVCFMT_RIGHT, 60 );

    //m_tbButtons.SetButtonState(IDM_PREVPOS, 0);
    //m_tbButtons.SetButtonState(IDM_NEXTPOS, 0);

    ApplyColors();

    initTooltips();

    eTagsViewMode viewMode = toViewMode( m_opt.getInt(OPT_VIEW_MODE) );
    eTagsSortMode sortMode = toSortMode( m_opt.getInt(OPT_VIEW_SORT) );
    SetViewMode(viewMode, sortMode);

    OnSize(true);

    checkCTagsPath();

    return TRUE;
}

void CTagsDlg::OnOK()
{
    // don't close the dialog
    HWND hFocusedWnd = ::GetFocus();

    if ( hFocusedWnd == m_tvTags.GetHwnd() )
    {
        m_tvTags.DirectMessage(WM_KEYDOWN, VK_RETURN, 0);
    }
    else if ( hFocusedWnd == m_lvTags.GetHwnd() )
    {
        m_lvTags.DirectMessage(WM_KEYDOWN, VK_RETURN, 0);
    }
}

LRESULT CTagsDlg::OnNotify(WPARAM wParam, LPARAM lParam)
{
    switch ( ((LPNMHDR) lParam)->code )
    {
        case TTN_GETDISPINFO:
            {
                // code from Win32++
                int iIndex = m_tbButtons.HitTest();
                if ( iIndex >= 0 )
                {
                    int nID = m_tbButtons.GetCommandID(iIndex);
                    if ( nID > 0 )
                    {
                        if ( m_DispInfoText.find(nID) == m_DispInfoText.end() )
                        {
                            m_DispInfoText[nID] = Win32xx::LoadString(nID);
                        }
                        LPNMTTDISPINFO lpDispInfo = (LPNMTTDISPINFO) lParam;
                        lpDispInfo->lpszText = const_cast<TCHAR*>(m_DispInfoText[nID].c_str());
                    }
                }
            }
            break;

        case LVN_COLUMNCLICK:
            SetSortMode( (eTagsSortMode) (TSM_NAME + ((LPNMLISTVIEW) lParam)->iSubItem) );
            break;

        case LVN_ITEMACTIVATE:
            break;

        case TVN_SELCHANGED:
            break;
    }

    return 0;
}

void CTagsDlg::OnSize(bool bInitial )
{
    if ( bInitial )
    {
        CRect r = GetWindowRect();
        ::MoveWindow( GetHwnd(), 0, 0, m_opt.getInt(OPT_VIEW_WIDTH), r.Height(), TRUE );
        return;
    }

    CRect r = GetClientRect();
    const int width = r.Width();
    const int height = r.Height();
    const int left = 2;
    int top = 1;

/*#ifdef _AKEL_TAGSVIEW
    HWND hStCaption = GetDlgItem(IDC_ST_CAPTION);
    if ( hStCaption )
    {
        ::GetWindowRect(hStCaption, &r);
        ::MoveWindow(hStCaption, left, 0, width - 2*left, r.Height(), TRUE);
        top = r.Height() + 1;
    }
#endif*/

    // toolbar
    r = m_tbButtons.GetWindowRect();
    ::MoveWindow( m_tbButtons, left, top, width - 2*left, r.Height(), TRUE );
    top += (r.Height() + 1);

    // edit window
    r = m_edFilter.GetWindowRect();
    ::MoveWindow( m_edFilter, left, top, width - 2*left, r.Height(), TRUE );
    top += (r.Height() + 1);

    /*RECT edRect;
    m_edFilter.DirectMessage(EM_GETRECT, 0, (LPARAM)&edRect);
    edRect.right -= r.Height();
    m_edFilter.DirectMessage(EM_SETRECT, 0, (LPARAM)&edRect);*/

    // list-view window
    r = m_lvTags.GetWindowRect();
    ::MoveWindow( m_lvTags, left, top, width - 2*left, height - top - 2, (m_viewMode == TVM_LIST) );

    // tree-view window
    r = m_tvTags.GetWindowRect();
    ::MoveWindow( m_tvTags, left, top, width - 2*left, height - top - 2, (m_viewMode == TVM_TREE) );


    // saving options...

    r = GetWindowRect();
    if ( r.Width() > 0 )
    {
        m_opt.setInt( OPT_VIEW_WIDTH, r.Width() );
    }

    int iNameWidth = m_lvTags.GetColumnWidth(0);
    if ( iNameWidth > 0 )
    {
        m_opt.setInt( OPT_VIEW_NAMEWIDTH, iNameWidth );
    }
}

INT_PTR CTagsDlg::OnCtlColorEdit(WPARAM wParam, LPARAM lParam)
{
    if ( reinterpret_cast<HWND>(lParam) == m_edFilter.GetHwnd() )
    {
        ::SetTextColor(reinterpret_cast<HDC>(wParam), m_crTextColor);
        ::SetBkColor(reinterpret_cast<HDC>(wParam), m_crBkgndColor);
        return reinterpret_cast<INT_PTR>(m_hBkgndBrush);
    }
    else
    {
        ::SetBkColor(reinterpret_cast<HDC>(wParam), ::GetSysColor(COLOR_WINDOW));
        return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_WINDOW));
    }
}

void CTagsDlg::OnPrevPosClicked()
{
    if ( m_pEdWr )
    {
        HWND hFocusedWnd = ::GetFocus();

        m_pEdWr->ewSetNavigationPoint( _T(""), false );
        m_pEdWr->ewDoNavigateBackward();

        UpdateNavigationButtons();

        if ( hFocusedWnd == m_pEdWr->ewGetEditHwnd() )
            m_pEdWr->ewDoSetFocus();
    }
}

void CTagsDlg::OnNextPosClicked()
{
    if ( m_pEdWr )
    {
        HWND hFocusedWnd = ::GetFocus();

        m_pEdWr->ewDoNavigateForward();

        UpdateNavigationButtons();

        if ( hFocusedWnd == m_pEdWr->ewGetEditHwnd() )
            m_pEdWr->ewDoSetFocus();
    }
}

void CTagsDlg::OnParseClicked()
{
    if ( m_pEdWr )
    {
        //m_pEdWr->ewClearNavigationHistory(false);
        m_pEdWr->ewDoParseFile();
    }
}

bool CTagsDlg::GoToTag(const TCHAR* cszTagName)  // not implemented yet
{
    if ( cszTagName && cszTagName[0] && !m_tags.empty() )
    {
        string tagName;

        #ifdef UNICODE
            char szTagNameA[CTagsDlg::MAX_TAGNAME];
            int len = SysUniConv::UnicodeToMultiByte(szTagNameA, CTagsDlg::MAX_TAGNAME - 1, cszTagName);
            szTagNameA[len] = 0;
            tagName = szTagNameA;
        #else
            tagName = cszTagName;
        #endif

        CTagsResultParser::tags_map::iterator itr = getTagByName(tagName);
        if ( itr != m_tags.end() )
        {
        }
    }

    return false;
}

enum eUnicodeType {
    enc_NotUnicode = 0,
    enc_UCS2_LE,
    enc_UCS2_BE,
    enc_UTF8
};

static eUnicodeType isUnicodeFile(HANDLE hFile)
{
    if ( hFile )
    {
        LONG nOffsetHigh;
        DWORD dwBytesRead;
        unsigned char data[4];

        nOffsetHigh = 0;
        ::SetFilePointer(hFile, 0, &nOffsetHigh, FILE_BEGIN);
        dwBytesRead = 0;
        if ( ::ReadFile(hFile, data, 2, &dwBytesRead, NULL) )
        {
            if ( dwBytesRead == 2 )
            {
                if ( data[0] == 0xFF && data[1] == 0xFE )
                    return enc_UCS2_LE;

                if ( data[0] == 0xFE && data[1] == 0xFF )
                    return enc_UCS2_BE;

                if ( data[0] == 0xEF && data[1] == 0xBB )
                {
                    if ( ::ReadFile(hFile, data, 1, &dwBytesRead, NULL) )
                    {
                        if ( dwBytesRead == 1 && data[0] == 0xBF )
                            return enc_UTF8;
                    }
                }
            }
        }
    }

    return enc_NotUnicode;
}

static bool createTempFileIfNeeded(LPCTSTR cszFileName, BOOL* pIsUTF8, TCHAR pszTempFileName[2*MAX_PATH])
{
    bool bResult = false;
    HANDLE hFile = ::CreateFile(cszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        DWORD dwSize = 0;
        DWORD dwSizeLow = ::GetFileSize(hFile, &dwSize);
        eUnicodeType enc = isUnicodeFile(hFile);
        if ( enc == enc_NotUnicode )
        {
            pszTempFileName[0] = 0;
            *pIsUTF8 = FALSE;
            bResult = true;
        }
        else if ( enc == enc_UTF8 )
        {
            pszTempFileName[0] = 0;
            *pIsUTF8 = TRUE;
            bResult = true;
        }
        else if ( dwSizeLow > 2 )
        {
            int lenUCS2 = (dwSizeLow - 2)/2; // skip 2 leading BOM bytes
            wchar_t* pUCS2 = new wchar_t[lenUCS2 + 1];
            if ( pUCS2 )
            {
                dwSize = 0;
                if ( ::ReadFile(hFile, pUCS2, 2*lenUCS2, &dwSize, NULL) )
                {
                    if ( dwSize == 2*lenUCS2 )
                    {
                        ::CloseHandle(hFile);
                        hFile = NULL;

                        if ( enc == enc_UCS2_BE )
                        {
                            // swap high and low bytes
                            wchar_t* p = pUCS2;
                            const wchar_t* pEnd = pUCS2 + lenUCS2;
                            while ( p < pEnd )
                            {
                                const wchar_t wch = *p;
                                *p = ((wch >> 8) & 0x00FF) + (((wch & 0x00FF) << 8) & 0xFF00);
                                ++p;
                            }
                        }
                        pUCS2[lenUCS2] = 0;

                        int lenUTF8 = ::WideCharToMultiByte(CP_UTF8, 0, pUCS2, lenUCS2, NULL, 0, NULL, NULL);
                        if ( (lenUTF8 > 0) && (lenUTF8 != lenUCS2) )
                        {
                            char* pUTF8 = new char[lenUTF8 + 1];
                            if ( pUTF8 )
                            {
                                pUTF8[0] = 0;
                                ::WideCharToMultiByte(CP_UTF8, 0, pUCS2, lenUCS2, pUTF8, lenUTF8 + 1, NULL, NULL);
                                pUTF8[lenUTF8] = 0;

                                delete [] pUCS2;
                                pUCS2 = NULL;

                                pszTempFileName[0] = 0;
                                dwSize = ::GetTempPath(2*MAX_PATH - 1, pszTempFileName);
                                if ( dwSize > 0 )
                                {
                                    --dwSize;
                                    if ( pszTempFileName[dwSize] != _T('\\') && pszTempFileName[dwSize] != _T('/') )
                                        pszTempFileName[dwSize] = _T('\\');
                                }
                                // generating unique temp file name
                                TCHAR szNum[64];
                                szNum[0] = 0;
                                wsprintf( szNum, _T("%lu_"), ::GetTickCount() );
                                lstrcat( pszTempFileName, szNum );
                                lstrcat( pszTempFileName, getFileName(cszFileName) );

                                HANDLE hTempFile = ::CreateFile(pszTempFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                                if ( hTempFile != INVALID_HANDLE_VALUE )
                                {
                                    /*
                                    // UTF-8 BOM bytes
                                    ::WriteFile(hTempFile, (LPCVOID) "\xEF\xBB\xBF", 3, &dwSize, NULL);
                                    */

                                    dwSize = 0;
                                    if ( ::WriteFile(hTempFile, pUTF8, lenUTF8, &dwSize, NULL) )
                                    {
                                        if ( dwSize == lenUTF8 )
                                        {
                                            *pIsUTF8 = TRUE;
                                            bResult = true;
                                        }
                                    }

                                    ::CloseHandle(hTempFile);
                                }

                                delete [] pUTF8;
                            }
                        }
                    }
                }

                if ( pUCS2 != NULL )
                    delete [] pUCS2;
            }
        }

        if ( hFile != NULL )
            ::CloseHandle(hFile);
    }

    return bResult;
}

void CTagsDlg::ParseFile(const TCHAR* const cszFileName)
{
    m_csTagsItems.Lock();

        m_lvTags.SetRedraw(FALSE);
        m_lvTags.DeleteAllItems();
        m_lvTags.SetRedraw(TRUE);

        m_tvTags.SetRedraw(FALSE);
        m_tvTags.DeleteAllItems();
        m_tvTags.SetRedraw(TRUE);

    m_csTagsItems.Release();

    m_tbButtons.SetButtonState(IDM_PREVPOS, 0);
    m_tbButtons.SetButtonState(IDM_NEXTPOS, 0);

    m_prevSelStart = -1;

    if ( cszFileName && cszFileName[0] )
    {
        BOOL isUTF8;
        const TCHAR* pszWorkFileName;
        TCHAR szTempFile[2*MAX_PATH];

        isUTF8 = FALSE;
        pszWorkFileName = cszFileName;
        szTempFile[0] = 0;
        if ( createTempFileIfNeeded(cszFileName, &isUTF8, szTempFile) )
        {
            if ( szTempFile[0] != 0 )
                pszWorkFileName = szTempFile;
        }

        tCTagsThreadParam* tt = new tCTagsThreadParam;
        if ( tt )
        {
            HANDLE  hThread;
            tString ctagsOptPath = m_ctagsPath;

            if ( ctagsOptPath.length() > 3 )
            {
                tString::size_type len = ctagsOptPath.length();
                ctagsOptPath[len - 3] = _T('o');
                ctagsOptPath[len - 2] = _T('p');
                ctagsOptPath[len - 1] = _T('t');
                if ( !isFilePathExist(ctagsOptPath.c_str(), false) )
                {
                    // file does not exist - so don't use it
                    ctagsOptPath.clear();
                }
            }
            else
            {
                // shit happened? ;)
                ctagsOptPath.clear();
            }

            tt->dwThreadID = 0;
            tt->pDlg = this;
            tt->cmd_line = _T("\"");
            tt->cmd_line += m_ctagsPath;
            if ( !ctagsOptPath.empty() )
            {
                tt->cmd_line += _T("\" --options=\"");
                tt->cmd_line += ctagsOptPath;
            }
            tt->cmd_line += _T("\" -f - --fields=fKnste \"");
            tt->cmd_line += pszWorkFileName;
            tt->cmd_line += _T("\"");
            tt->source_file_name = cszFileName;
            if ( pszWorkFileName == szTempFile )
                tt->temp_file_name = szTempFile;
            else
                tt->temp_file_name.clear();
            tt->isUTF8 = isUTF8;

            ::ResetEvent(m_hTagsThreadEvent);
            hThread = ::CreateThread(NULL, 0, CTagsThreadProc, tt, 0, &tt->dwThreadID);
            if ( hThread )
            {
                ::InterlockedIncrement(&m_nTagsThreadCount);
                m_dwLastTagsThreadID = tt->dwThreadID;
                ::CloseHandle(hThread);
            }
            ::SetEvent(m_hTagsThreadEvent);
        }
    }
}

void CTagsDlg::ReparseCurrentFile()
{
    OnParseClicked();
}

void CTagsDlg::SetSortMode(eTagsSortMode sortMode)
{
    if ( (sortMode != TSM_NONE) && (sortMode != m_sortMode) )
    {
        m_opt.setInt( OPT_VIEW_SORT, (int) sortMode );

        m_prevSelStart = -1;

        if ( m_viewMode == TVM_TREE )
        {
            m_tvTags.SetRedraw(FALSE);

            switch ( sortMode )
            {
                case TSM_LINE:
                case TSM_TYPE:
                    m_sortMode = TSM_LINE;
                    sortTagsByType();
                    m_tbButtons.SetButtonState(IDM_SORTNAME, TBSTATE_ENABLED);
                    m_tbButtons.SetButtonState(IDM_SORTLINE, TBSTATE_ENABLED | TBSTATE_CHECKED);
                    break;

                case TSM_NAME:
                    m_sortMode = TSM_NAME;
                    sortTagsByType();
                    m_tbButtons.SetButtonState(IDM_SORTLINE, TBSTATE_ENABLED);
                    m_tbButtons.SetButtonState(IDM_SORTNAME, TBSTATE_ENABLED | TBSTATE_CHECKED);
                    break;
            }

            UpdateCurrentItem();

            m_tvTags.SetRedraw(TRUE);
            m_tvTags.InvalidateRect(TRUE);
        }
        else
        {
            m_lvTags.SetRedraw(FALSE);

            switch ( sortMode )
            {
                case TSM_LINE:
                    sortTagsByLine();
                    m_tbButtons.SetButtonState(IDM_SORTNAME, TBSTATE_ENABLED);
                    m_tbButtons.SetButtonState(IDM_SORTLINE, TBSTATE_ENABLED | TBSTATE_CHECKED);
                    break;

                case TSM_TYPE:
                    sortTagsByType();
                    break;

                case TSM_NAME:
                    sortTagsByName();
                    m_tbButtons.SetButtonState(IDM_SORTLINE, TBSTATE_ENABLED);
                    m_tbButtons.SetButtonState(IDM_SORTNAME, TBSTATE_ENABLED | TBSTATE_CHECKED);
                    break;
            }

            UpdateCurrentItem();

            m_lvTags.SetRedraw(TRUE);
            m_lvTags.InvalidateRect(TRUE);
        }

        m_sortMode = sortMode;
    }
}

void CTagsDlg::SetViewMode(eTagsViewMode viewMode, eTagsSortMode sortMode)
{
    if ( (viewMode != TVM_NONE) && (viewMode != m_viewMode) )
    {
        m_opt.setInt( OPT_VIEW_MODE, (int) viewMode );

        m_viewMode = viewMode;
        m_sortMode = TSM_NONE;
        m_lvTags.ShowWindow(SW_HIDE);
        m_tvTags.ShowWindow(SW_HIDE);
    }

    SetSortMode(sortMode);

    switch ( m_viewMode )
    {
        case TVM_LIST:
            if ( !m_lvTags.IsWindowVisible() )
                m_lvTags.ShowWindow(SW_SHOWNORMAL);
            m_tbButtons.SetButtonState(IDM_VIEWTREE, TBSTATE_ENABLED);
            m_tbButtons.SetButtonState(IDM_VIEWLIST, TBSTATE_ENABLED | TBSTATE_CHECKED);
            break;

        case TVM_TREE:
            if ( !m_tvTags.IsWindowVisible() )
                m_tvTags.ShowWindow(SW_SHOWNORMAL);
            m_tbButtons.SetButtonState(IDM_VIEWLIST, TBSTATE_ENABLED);
            m_tbButtons.SetButtonState(IDM_VIEWTREE, TBSTATE_ENABLED | TBSTATE_CHECKED);
            break;
    }
}

void CTagsDlg::UpdateCurrentItem()
{
    if ( m_pEdWr && !m_tags.empty() )
    {
        int selEnd, selStart = m_pEdWr->ewGetSelectionPos(selEnd);

        if ( m_prevSelStart != selStart )
        {
            m_prevSelStart = selStart;

            int line = m_pEdWr->ewGetLineFromPos(selStart) + 1; // 1-based line
            CTagsResultParser::tags_map::const_iterator itr = m_tags.lower_bound(line);
            if ( itr == m_tags.end() )
            {
                if ( itr != m_tags.begin() )
                    --itr;
                else
                    return;
            }
            else
            {
                if ( (itr != m_tags.begin()) && (itr->second.line > line) )
                {
                    --itr;
                }

                for ( auto itr2 = itr; itr2->second.line <= line; --itr2 )
                {
                    if ( itr2->second.end_line >= line )
                    {
                        itr = itr2;
                        break;
                    }

                    if ( itr2 == m_tags.begin() )
                        break;
                }
            }

            if ( m_viewMode == TVM_TREE )
            {
                HTREEITEM hItem = (HTREEITEM) itr->second.data.p;
                if ( hItem )
                {
                    m_csTagsItems.Lock();
                    if ( m_tvTags.GetCount() > 0 )
                    {
                        m_tvTags.SelectItem(hItem);
                        m_tvTags.EnsureVisible(hItem);
                    }
                    m_csTagsItems.Release();
                }
            }
            else
            {
                int iItem = itr->second.data.i;
                if ( iItem >= 0 )
                {
                    m_csTagsItems.Lock();
                    if ( m_lvTags.GetItemCount() > 0 )
                    {
                        m_lvTags.SetItemState(iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                        m_lvTags.SetSelectionMark(iItem);
                        m_lvTags.EnsureVisible(iItem, FALSE);
                    }
                    m_csTagsItems.Release();
                }
            }
        }
    }
}

void CTagsDlg::UpdateTagsView()
{
    m_prevSelStart = -1;

    eTagsViewMode viewMode = m_viewMode;
    eTagsSortMode sortMode = m_sortMode;
    //disabled to avoid the background blink: m_viewMode = TVM_NONE;
    m_sortMode = TSM_NONE;
    SetViewMode(viewMode, sortMode);
    UpdateNavigationButtons();
}

void CTagsDlg::UpdateNavigationButtons()
{
    if ( m_pEdWr )
    {
        if ( m_tbButtons.GetHwnd() )
        {
            m_tbButtons.SetButtonState(IDM_PREVPOS,
              m_pEdWr->ewCanNavigateBackward() ? TBSTATE_ENABLED : 0);
            m_tbButtons.SetButtonState(IDM_NEXTPOS,
              m_pEdWr->ewCanNavigateForward() ? TBSTATE_ENABLED : 0);
        }
    }
}

int CTagsDlg::addListViewItem(int nItem, const CTagsResultParser::tTagData& tagData)
{
    int    n = -1;
    LVITEM lvi = { 0 };
    TCHAR  ts[MAX_TAGNAME];

    const UINT cp = m_isUTF8tags ? CP_UTF8 : CP_ACP;

#ifdef UNICODE
    const int len = SysUniConv::MultiByteToUnicode( ts, MAX_TAGNAME - 1, tagData.getFullTagName().c_str(), -1, cp );
#else
    const int len = (int) tagData.getFullTagName().copy(ts, MAX_TAGNAME - 1);
#endif
    ts[len] = 0;

    lvi.iItem = nItem;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText = ts;
    lvi.cchTextMax = len;
    lvi.lParam = (LPARAM) tagData.pTagData;
    n = m_lvTags.InsertItem(lvi);

#ifdef UNICODE
    SysUniConv::MultiByteToUnicode(ts, MAX_TAGTYPE, tagData.tagType.c_str(), MAX_TAGTYPE - 1, cp);
#else
    tagData.tagType.copy(ts, MAX_TAGTYPE - 1);
#endif
    m_lvTags.SetItemText(nItem, LVC_TYPE, ts);

    wsprintf(ts, _T("%d"), tagData.line);
    m_lvTags.SetItemText(nItem, LVC_LINE, ts);

    return n;
}

HTREEITEM CTagsDlg::addTreeViewItem(HTREEITEM hParent, const CTagsResultParser::tTagData& tagData)
{
    TVINSERTSTRUCT tvis = { 0 };
    TCHAR          ts[MAX_TAGNAME];

    const UINT cp = m_isUTF8tags ? CP_UTF8 : CP_ACP;

#ifdef UNICODE
    const int len = SysUniConv::MultiByteToUnicode( ts, MAX_TAGNAME - 1, tagData.tagName.c_str(), -1, cp );
#else
    const int len = (int) tagData.tagName.copy(ts, MAX_TAGNAME - 1);
#endif
    ts[len] = 0;

    if ( hParent == TVI_ROOT )
    {
        ::CharUpper(ts);
    }

    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvis.item.pszText = ts;
    tvis.item.cchTextMax = len;
    tvis.item.lParam = (LPARAM) tagData.pTagData;

    return m_tvTags.InsertItem(tvis);
}

void CTagsDlg::ApplyColors()
{
    bool bApplyEditorColors = false;
    bool bColorChanged = false;
    IEditorWrapper::sEditorColors colors;

    if ( m_opt.getBool(CTagsDlg::OPT_COLORS_USEEDITORCOLORS) &&
         m_pEdWr && m_pEdWr->ewGetEditorColors(colors) )
    {
        bApplyEditorColors = true;
    }
    else
    {
        colors.crTextColor = ::GetSysColor(COLOR_WINDOWTEXT);
        colors.crBkgndColor = ::GetSysColor(COLOR_WINDOW);
    }

    if ( m_crBkgndColor != colors.crBkgndColor )
    {
        m_lvTags.SetTextBkColor(colors.crBkgndColor);
        m_lvTags.SetBkColor(colors.crBkgndColor);
        m_tvTags.SetBkColor(colors.crBkgndColor);

        m_crBkgndColor = colors.crBkgndColor;
        if ( m_hBkgndBrush != NULL )
        {
            ::DeleteObject(m_hBkgndBrush);
        }
        m_hBkgndBrush = ::CreateSolidBrush(colors.crBkgndColor);
        bColorChanged = true;
    }

    if ( m_crTextColor != colors.crTextColor )
    {
        m_lvTags.SetTextColor(colors.crTextColor);
        m_tvTags.SetTextColor(colors.crTextColor);

        m_crTextColor = colors.crTextColor;
        bColorChanged = true;
    }

    if ( bColorChanged )
    {
        m_edFilter.Invalidate(TRUE);
        m_lvTags.Invalidate(TRUE);
        m_tvTags.Invalidate(TRUE);
    }
}

void CTagsDlg::checkCTagsPath()
{
    if ( !isFilePathExist(m_ctagsPath.c_str(), false) )
    {
        this->PostMessage(WM_CTAGSPATHFAILED, 0, 0);
    }
}

CTagsResultParser::tags_map::iterator CTagsDlg::getTagByLine(const int line)
{
    CTagsResultParser::tags_map::iterator itr = m_tags.lower_bound(line);
    if ( itr != m_tags.end() )
    {
        if ( (itr != m_tags.begin()) && (itr->second.line > line) )
        {
            --itr;
        }
    }
    return itr;
}

CTagsResultParser::tags_map::iterator CTagsDlg::getTagByName(const string& tagName)
{
    CTagsResultParser::tags_map::iterator itr = m_tags.begin();
    while ( itr != m_tags.end() )
    {
        if ( itr->second.tagName == tagName )
            break;
        else
            ++itr;
    }
    return itr;
}

void CTagsDlg::initOptions()
{
    if ( m_opt.HasOptions() )
        return;

    const TCHAR cszView[]     = _T("View");
    const TCHAR cszColors[]   = _T("Colors");
    const TCHAR cszBehavior[] = _T("Behavior");

    m_opt.ReserveMemory(OPT_COUNT);

    m_opt.AddInt( OPT_VIEW_MODE,      cszView,  _T("Mode"),       TVM_LIST );
    m_opt.AddInt( OPT_VIEW_SORT,      cszView,  _T("Sort"),       TSM_LINE );
    m_opt.AddInt( OPT_VIEW_WIDTH,     cszView,  _T("Width"),      220      );
    m_opt.AddInt( OPT_VIEW_NAMEWIDTH, cszView,  _T("NameWidth"),  220      );

    m_opt.AddBool( OPT_COLORS_USEEDITORCOLORS,  cszColors, _T("UseEditorColors"), true );
    //m_opt.AddHexInt( OPT_COLORS_BKGND,   cszColor, _T("BkGnd"),   0 );
    //m_opt.AddHexInt( OPT_COLORS_TEXT,    cszColor, _T("Text"),    0 );
    //m_opt.AddHexInt( OPT_COLORS_TEXTSEL, cszColor, _T("TextSel"), 0 );
    //m_opt.AddHexInt( OPT_COLORS_SELA,    cszColor, _T("SelA"),    0 );
    //m_opt.AddHexInt( OPT_COLORS_SELN,    cszColor, _T("SelN"),    0 );

    m_opt.AddBool( OPT_BEHAVIOR_PARSEONSAVE, cszBehavior, _T("ParseOnSave"), true );
}

bool CTagsDlg::isTagMatchFilter(const string& tagName)
{
    if ( m_tagFilter.empty() )
        return true;

    const string::size_type len1 = m_tagFilter.length();
    const string::size_type len = tagName.length();
    if ( len < len1 )
        return false;

    const string::size_type max_pos = len - len1;
    string::size_type pos = 0;
    while ( pos <= max_pos )
    {
        if ( ::CompareStringA(
                 LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                 tagName.c_str() + pos, (int)len1,
                 m_tagFilter.c_str(), (int)len1) == CSTR_EQUAL )
        {
            return true;
        }
        else
            ++pos;
    }

    return false;
}

void CTagsDlg::sortTagsByLine()
{
    m_csTagsItems.Lock();

        m_lvTags.DeleteAllItems();
        m_tvTags.DeleteAllItems();

    m_csTagsItems.Release();

    int nItem = 0;
    CTagsResultParser::tags_map::iterator itr = m_tags.begin();
    for ( ; itr != m_tags.end(); ++itr )
    {
        CTagsResultParser::tTagData& tagData = itr->second;
        if ( !isTagMatchFilter(tagData.getFullTagName()) )
        {
            tagData.data.i = -1;
            continue;
        }

        tagData.data.i = addListViewItem(nItem++, tagData);
    }
}

void CTagsDlg::sortTagsByName()
{
    m_csTagsItems.Lock();

        m_lvTags.DeleteAllItems();
        m_tvTags.DeleteAllItems();

    m_csTagsItems.Release();

    string s;
    multimap<string, CTagsResultParser::tTagData, string_cmp_less> tagsMap;
    multimap<string, CTagsResultParser::tTagData, string_cmp_less>::iterator i;
    CTagsResultParser::tags_map::iterator itr = m_tags.begin();
    for ( ; itr != m_tags.end(); ++itr )
    {
        bool isAdded = false;
        const CTagsResultParser::tTagData& tagData = itr->second;
        const string tagFullName = tagData.getFullTagName();

        if ( !isTagMatchFilter(tagFullName) )
        {
            itr->second.data.i = -1;
            continue;
        }

        if ( m_sortMode != TSM_NAME )
        {
            i = tagsMap.find(tagFullName);
            while ( (i != tagsMap.end()) && (i->second.tagName == tagFullName) )
            {
                if ( m_sortMode == TSM_LINE )
                {
                    if ( i->second.line >= tagData.line )
                    {
                        tagsMap.insert( i, std::make_pair(tagFullName, tagData) );
                        isAdded = true;
                        break;
                    }
                }
                else if ( m_sortMode == TSM_TYPE )
                {
                    if ( i->second.tagType >= tagData.tagType )
                    {
                        tagsMap.insert( i, std::make_pair(tagFullName, tagData) );
                        isAdded = true;
                        break;
                    }
                }
                ++i;
            }
        }

        if ( !isAdded )
        {
            tagsMap.insert( std::make_pair(tagFullName, tagData) );
        }
    }

    int nItem = 0;
    i = tagsMap.begin();
    for ( ; i != tagsMap.end(); ++i )
    {
        int n = addListViewItem(nItem++, i->second);
        CTagsResultParser::tags_map::iterator tagIt = getTagByLine(i->second.line);
        if ( tagIt != m_tags.end() )
        {
            tagIt->second.data.i = n;
        }
    }
}

void CTagsDlg::sortTagsByType()
{
    m_csTagsItems.Lock();

        m_lvTags.DeleteAllItems();
        m_tvTags.DeleteAllItems();

    m_csTagsItems.Release();

    multimap<string, CTagsResultParser::tTagData> tagsMap;
    multimap<string, CTagsResultParser::tTagData>::iterator i;
    CTagsResultParser::tags_map::iterator itr = m_tags.begin();
    for ( ; itr != m_tags.end(); ++itr )
    {
        bool isAdded = false;
        const CTagsResultParser::tTagData& tagData = itr->second;
        const string tagFullName = tagData.getFullTagName();

        if ( !isTagMatchFilter(tagFullName) )
        {
            if ( m_viewMode == TVM_TREE )
                itr->second.data.p = NULL;
            else
                itr->second.data.i = -1;
            continue;
        }

        if ( m_sortMode != TSM_TYPE )
        {
            i = tagsMap.find(tagData.tagType);
            while ( (i != tagsMap.end()) && (i->second.tagType == tagData.tagType) )
            {
                if ( m_sortMode == TSM_LINE )
                {
                    if ( i->second.line >= tagData.line )
                    {
                        tagsMap.insert( i, std::make_pair(tagData.tagType, tagData) );
                        isAdded = true;
                        break;
                    }
                }
                else if ( m_sortMode == TSM_NAME )
                {
                    if ( lstrcmpiA(i->second.getFullTagName().c_str(), tagFullName.c_str()) >= 0 )
                    {
                        tagsMap.insert( i, std::make_pair(tagData.tagType, tagData) );
                        isAdded = true;
                        break;
                    }
                }
                ++i;
            }
        }

        if ( !isAdded )
        {
            tagsMap.insert( std::make_pair(tagData.tagType, tagData) );
        }
    }

    map<string, HTREEITEM> scopeParentMap;
    HTREEITEM hTypeParent = TVI_ROOT;
    int nItem = 0;
    string type = "";
    i = tagsMap.begin();
    for ( ; i != tagsMap.end(); ++i )
    {
        if ( m_viewMode == TVM_TREE )
        {
            if ( type != i->second.tagType )
            {
                CTagsResultParser::tTagData tagData;

                type = i->second.tagType;
                tagData.tagName = i->second.tagType;
                tagData.line = -3;
                tagData.pTagData = (void *) 0;

                hTypeParent = addTreeViewItem(TVI_ROOT, tagData);

                scopeParentMap.clear();
                scopeParentMap[""] = hTypeParent;

                if ( !m_tagFilter.empty() )
                {
                    TVITEM tvi = { 0 };

                    tvi.hItem = hTypeParent;
                    tvi.mask = TVIF_HANDLE | TVIF_STATE;
                    tvi.state = TVIS_EXPANDED;
                    tvi.stateMask = TVIS_EXPANDED;

                    m_tvTags.SetItem(tvi);
                }
            }

            HTREEITEM hScopeParent;
            map<string, HTREEITEM>::iterator scopeIt = scopeParentMap.find(i->second.tagScope);
            if ( scopeIt != scopeParentMap.end() )
            {
                hScopeParent = scopeIt->second;
            }
            else
            {
                CTagsResultParser::tTagData tagData;

                tagData.tagName = i->second.tagScope;
                tagData.line = -2;
                tagData.pTagData = (void *) 0;

                hScopeParent = addTreeViewItem(hTypeParent, tagData);
                scopeParentMap[tagData.tagName] = hScopeParent;

                if ( !m_tagFilter.empty() )
                {
                    TVITEM tvi = { 0 };

                    tvi.hItem = hScopeParent;
                    tvi.mask = TVIF_HANDLE | TVIF_STATE;
                    tvi.state = TVIS_EXPANDED;
                    tvi.stateMask = TVIS_EXPANDED;

                    m_tvTags.SetItem(tvi);
                }
            }
            HTREEITEM hItem = addTreeViewItem(hScopeParent, i->second);
            CTagsResultParser::tags_map::iterator tagIt = getTagByLine(i->second.line);
            if ( tagIt != m_tags.end() )
            {
                tagIt->second.data.p = (void *) hItem;
            }
        }
        else
        {
            int n = addListViewItem(nItem++, i->second);
            CTagsResultParser::tags_map::iterator tagIt = getTagByLine(i->second.line);
            if ( tagIt != m_tags.end() )
            {
                tagIt->second.data.i = n;
            }
        }
    }
}

