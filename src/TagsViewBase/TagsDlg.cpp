#include "TagsDlg.h"
#include "ConsoleOutputRedirector.h"
#include "resource.h"
#include "win32++/include/wxx_textconv.h"
#include <memory>
#include <set>
#include <algorithm>

namespace
{
    bool isFileExist(LPCTSTR pszFilePath)
    {
        if ( pszFilePath && pszFilePath[0] )
        {
            if ( ::GetFileAttributes(pszFilePath) != INVALID_FILE_ATTRIBUTES )
                return true;
        }
        return false;
    }

    LPCTSTR getFileName(LPCTSTR pszFilePathName)
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

    LPCTSTR getFileName(const tString& filePathName)
    {
        tString::size_type n = 0;
        if ( !filePathName.empty() )
        {
            n = filePathName.find_last_of(_T("\\/"));
            if ( n != tString::npos )
                ++n;
            else
                n = 0;
        }
        return (filePathName.c_str() + n);
    }

    LPCTSTR getFileExt(LPCTSTR pszFilePathName)
    {
        const TCHAR* pszExt = pszFilePathName + lstrlen(pszFilePathName);
        while ( --pszExt >= pszFilePathName )
        {
            if ( *pszExt == _T('.') )
                return pszExt;

            if ( *pszExt == _T('\\') || *pszExt == _T('/') )
                break;
        }
        return _T("");
    }

    tString getFileDirectory(const tString& filePathName)
    {
        tString::size_type n = filePathName.find_last_of(_T("\\/"));
        return ( (n != tString::npos) ? tString(filePathName.c_str(), n + 1) : tString() );
    }

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

    bool isSimilarPoint(const CPoint& pt1, const CPoint& pt2, LONG max_diff)
    {
        if ( max_diff < 0 )
            max_diff = - max_diff;

        LONG diff = pt1.x - pt2.x;
        if ( diff < 0 )
            diff = -diff;
        if ( diff > max_diff )
            return false;

        diff = pt1.y - pt2.y;
        if ( diff < 0 )
            diff = -diff;
        if ( diff > max_diff )
            return false;

        return true;
    }
}


tString CTagsDlgChild::getTooltip(const CTagsResultParser::tTagData* pTagData)
{
    tString s;

    s.reserve(200);
    s += pTagData->tagPattern;
    s += _T("\nscope: ");
    if ( !pTagData->tagScope.empty() )
        s += pTagData->tagScope;
    else
        s += _T("(global)");
    s += _T("\ntype: ");
    s += pTagData->tagType;

    if ( !pTagData->filePath.empty() )
    {
        TCHAR szNum[20];

        s += _T("\nfile: ");
        s += getFileName(pTagData->filePath);
        s += _T(":");
        ::wsprintf(szNum, _T("%d"), pTagData->line);
        s += szNum;
    }

    return s;
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
            LRESULT  lResult;
            TCHAR    szText[CTagsDlg::MAX_TAGNAME];
            tString  tagFilter;

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
                        LVITEM lvi;
                        TCHAR szItemText[CTagsDlg::MAX_TAGNAME];

                        szItemText[0] = 0;

                        ::ZeroMemory(&lvi, sizeof(lvi));
                        lvi.mask = LVIF_TEXT | LVIF_PARAM;
                        lvi.iItem = iItem;
                        lvi.pszText = szItemText;
                        lvi.cchTextMax = sizeof(szItemText)/sizeof(szItemText[0]) - 1;

                        m_pDlg->m_lvTags.GetItem(lvi);

                        m_pDlg->m_lvTags.SetItemState(iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                        m_pDlg->m_lvTags.SetSelectionMark(iItem);

                        const CTagsDlg::tTagData* pTagData = (const CTagsDlg::tTagData *) lvi.lParam;
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
                    if ( ScreenToClient(pt) /*&& !isSimilarPoint(pt, m_lastPoint, 2)*/ )
                    {
                        UINT uFlags = 0;
                        int nItem = HitTest(pt, &uFlags);
                        if ( nItem != -1 )
                        {
                            LPNMTTDISPINFO lpnmdi = (LPNMTTDISPINFO) lParam;
                            SendMessage(lpnmdi->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, 600);

                            const CTagsDlg::tTagData* pTagData = (const CTagsDlg::tTagData *) GetItemData(nItem);
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

                            //m_lastPoint = pt;
                            return 0;
                        }

                        //m_lastPoint.x = 0;
                        //m_lastPoint.y = 0;
                    }
                }
            }
        }
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
                    TVITEM tvi;
                    TCHAR  szItemText[CTagsDlg::MAX_TAGNAME];

                    szItemText[0] = 0;

                    ::ZeroMemory(&tvi, sizeof(tvi));
                    tvi.hItem = hItem;
                    tvi.mask = TVIF_TEXT | TVIF_PARAM;
                    tvi.pszText = szItemText;
                    tvi.cchTextMax = sizeof(szItemText)/sizeof(szItemText[0]) - 1;

                    m_pDlg->m_tvTags.GetItem(tvi);

                    const CTagsDlg::tTagData* pTagData = (const CTagsDlg::tTagData *) tvi.lParam;
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
                    if ( ScreenToClient(pt) /*&& !isSimilarPoint(pt, m_lastPoint, 2)*/ )
                    {
                        TVHITTESTINFO hitInfo = { 0 };

                        hitInfo.pt = pt;
                        HTREEITEM hItem = HitTest(hitInfo);
                        if ( hItem )
                        {
                            LPNMTTDISPINFO lpnmdi = (LPNMTTDISPINFO) lParam;
                            SendMessage(lpnmdi->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, 600);

                            const CTagsDlg::tTagData* pTagData = (const CTagsDlg::tTagData *) GetItemData(hItem);
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

                            //m_lastPoint = pt;
                            return 0;
                        }

                        //m_lastPoint.x = 0;
                        //m_lastPoint.y = 0;
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
    }

    return WndProcDefault(uMsg, wParam, lParam);
}

const TCHAR* CTagsDlg::cszListViewColumnNames[LVC_TOTAL] = {
    _T("Name"),
    _T("Type"),
    _T("Line"),
    _T("File")
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
, m_tags(nullptr)
, m_optRdWr(NULL)
, m_pEdWr(NULL)
, m_prevSelStart(-1)
, m_isUpdatingSelToItem(false)
, m_nTagsThreadCount(0)
, m_hInstDll(NULL)
, m_crTextColor(0xFFFFFFFF)
, m_crBkgndColor(0xFFFFFFFF)
, m_hBkgndBrush(NULL)
{
    SetCTagsExePath();
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

    if ( m_opt.getInt(OPT_DEBUG_DELETETEMPINPUTFILE) != DTF_NEVERDELETE )
    {
        DeleteTempFile(m_ctagsTempInputFilePath);
    }
    if ( m_opt.getInt(OPT_DEBUG_DELETETEMPOUTPUTFILE) != DTF_NEVERDELETE )
    {
        DeleteTempFile(m_ctagsTempOutputFilePath);
    }
}

void CTagsDlg::DeleteTempFile(const tString& filePath)
{
    if ( !filePath.empty() )
    {
        if ( isFileExist(filePath.c_str()) )
        {
            ::DeleteFile( filePath.c_str() );
        }
    }
}

DWORD WINAPI CTagsDlg::CTagsThreadProc(LPVOID lpParam)
{
    CConsoleOutputRedirector cor;
    CProcess proc;
    tCTagsThreadParam* tt = (tCTagsThreadParam *) lpParam;
    bool bTagsAdded = false;

    if ( tt->pDlg->GetHwnd() )
    {
        std::unique_ptr<char[]> pTempTags;

        if ( tt->temp_output_file.empty() )
        {
            cor.Execute( tt->cmd_line.c_str() );
        }
        else
        {
            DeleteTempFile(tt->temp_output_file); // ctags will be outputting to this file

            proc.Execute( tt->cmd_line.c_str(), TRUE );

            HANDLE hFile = ::CreateFile(tt->temp_output_file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
            if ( hFile != INVALID_HANDLE_VALUE )
            {
                DWORD dwSize = 0;
                DWORD dwSizeLow = ::GetFileSize(hFile, &dwSize);

                dwSize = 0;
                pTempTags.reset(new char[dwSizeLow + 1]);
                if ( ::ReadFile(hFile, pTempTags.get(), dwSizeLow, &dwSize, NULL) && (dwSizeLow == dwSize) )
                {
                    pTempTags[dwSizeLow] = 0;
                }
                else
                {
                    char* ptr = pTempTags.release();
                    delete [] ptr;
                }

                ::CloseHandle(hFile);
            }
        }

        COptionsManager& opt = tt->pDlg->GetOptions();
        if ( opt.getInt(CTagsDlg::OPT_DEBUG_DELETETEMPINPUTFILE) == CTagsDlg::DTF_ALWAYSDELETE )
        {
            DeleteTempFile(tt->temp_input_file);
        }
        if ( opt.getInt(CTagsDlg::OPT_DEBUG_DELETETEMPOUTPUTFILE) == CTagsDlg::DTF_ALWAYSDELETE )
        {
            DeleteTempFile(tt->temp_output_file);
        }

        ::WaitForSingleObject(tt->pDlg->m_hTagsThreadEvent, 4000);
        if ( tt->dwThreadID == tt->pDlg->m_dwLastTagsThreadID )
        {
            if ( tt->pDlg->GetHwnd() /*&& tt->pDlg->IsWindowVisible()*/ )
            {
                const CEditorWrapper* pEdWr = tt->pDlg->m_pEdWr;
                if ( pEdWr && (pEdWr->ewGetFilePathName() == tt->source_file_name) )
                {
                    if ( tt->temp_output_file.empty() )
                        tt->pDlg->OnAddTags( cor.GetOutputString().c_str(), tt );
                    else
                        tt->pDlg->OnAddTags( pTempTags.get(), tt );

                    bTagsAdded = true;
                }
            }
        }
    }
    else
    {
        CTagsDlg* pDlg = tt->pDlg;
        if ( !pDlg || pDlg->GetOptions().getInt(OPT_DEBUG_DELETETEMPINPUTFILE) != DTF_NEVERDELETE )
        {
            DeleteTempFile(tt->temp_input_file);
        }
        if ( !pDlg || pDlg->GetOptions().getInt(OPT_DEBUG_DELETETEMPOUTPUTFILE) != DTF_NEVERDELETE )
        {
            DeleteTempFile(tt->temp_output_file);
        }
    }

    if ( !bTagsAdded )
    {
        CTagsDlg* pDlg = tt->pDlg;
        if ( !pDlg || pDlg->GetOptions().getInt(OPT_DEBUG_DELETETEMPOUTPUTFILE) != DTF_NEVERDELETE )
        {
            DeleteTempFile(tt->temp_output_file);
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
        case WM_TAGDBLCLICKED:
            OnTagDblClicked( (const tTagData *) lParam );
            return 0;

        case WM_FILECLOSED:
            OnFileClosed();
            return 0;

        case WM_UPDATETAGSVIEW:
            UpdateTagsView();
            return 0;

        case WM_CLOSETAGSVIEW:
            if ( m_opt.getInt(OPT_DEBUG_DELETETEMPINPUTFILE) != DTF_NEVERDELETE )
            {
                DeleteTempFile(m_ctagsTempInputFilePath);
            }
            if ( m_opt.getInt(OPT_DEBUG_DELETETEMPOUTPUTFILE) != DTF_NEVERDELETE )
            {
                DeleteTempFile(m_ctagsTempOutputFilePath);
            }
            if ( m_pEdWr )
            {
                m_pEdWr->ewCloseTagsView();
            }
            return 0;

        case WM_FOCUSTOEDITOR:
            if ( m_pEdWr )
            {
                m_pEdWr->ewDoSetFocus();
            }
            return 0;

        case WM_CTAGSPATHFAILED:
            if ( m_pEdWr )
            {
                tString err = _T("File \'ctags.exe\' was not found. The path is incorrect:\n");
                err += m_ctagsExeFilePath;
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
        m_pEdWr->ewCloseTagsView();
    }
}

void CTagsDlg::OnAddTags(const char* s, const tCTagsThreadParam* tt)
{
    unsigned int nParseFlags = 0;
    if ( tt->isUTF8 )
    {
        nParseFlags |= CTagsResultParser::PF_ISUTF8;
    }

    CTagsResultParser::Parse(
        s,
        nParseFlags,
        CTagsResultParser::tParseContext(
            *m_tags,
            getFileDirectory(tt->source_file_name),
            tt->temp_input_file.empty() ? tString() : tt->source_file_name
        ) 
    );

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
            this->PostMessage(WM_CLOSETAGSVIEW);
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

    //if ( ::GetFocus() != m_edFilter.GetHwnd() )
    {
        if ( m_opt.getBool(OPT_VIEW_ESCFOCUSTOEDITOR) )
        {
            PostMessage(WM_FOCUSTOEDITOR);
        }
    }
}

void CTagsDlg::createTooltips()
{
    /*
    if ( !m_ttHints.GetHwnd() || !m_ttHints.IsWindow() )
    {
        m_ttHints.CreateEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, GetHwnd(), NULL, NULL);
        m_ttHints.SetWindowPos( HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );

        m_ttHints.AddTool( m_lvTags, m_lvTags.GetClientRect(), IDC_LV_TAGS, LPSTR_TEXTCALLBACK );
        m_ttHints.AddTool( m_tvTags, m_tvTags.GetClientRect(), IDC_TV_TAGS, LPSTR_TEXTCALLBACK );

        //m_ttHints.SetDelayTime(TTDT_AUTOPOP, 10000);
        //m_ttHints.SetDelayTime(TTDT_INITIAL, 200);
        //m_ttHints.SetDelayTime(TTDT_RESHOW, 200);

        m_lvTags.SetToolTips(m_ttHints);
        m_tvTags.SetToolTips(m_ttHints);
    }
    */

    DWORD dwStyle = m_tvTags.GetStyle();
    if ( (dwStyle & TVS_NOTOOLTIPS) )
    {
        m_tvTags.SetStyle(dwStyle ^ TVS_NOTOOLTIPS);
    }
}

void CTagsDlg::destroyTooltips()
{
    DWORD dwStyle = m_tvTags.GetStyle();
    if ( !(dwStyle & TVS_NOTOOLTIPS) )
    {
        m_tvTags.SetStyle(dwStyle | TVS_NOTOOLTIPS);
    }

    /*
    m_lvTags.SendMessage( LVM_SETTOOLTIPS, 0, 0 );
    m_tvTags.SendMessage( TVM_SETTOOLTIPS, 0, 0 );

    if ( m_ttHints.GetHwnd() && m_ttHints.IsWindow() )
    {
        m_ttHints.DelTool( m_lvTags, IDC_LV_TAGS );
        m_ttHints.DelTool( m_tvTags, IDC_TV_TAGS );
        m_ttHints.Destroy();
    }
    */
}

BOOL CTagsDlg::OnInitDialog()
{
    INITCOMMONCONTROLSEX iccex;

    iccex.dwICC = ICC_WIN95_CLASSES;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    ::InitCommonControlsEx( &iccex );

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
    m_lvTags.InsertColumn( LVC_NAME, cszListViewColumnNames[LVC_NAME], LVCFMT_LEFT, m_opt.getInt(OPT_VIEW_NAMEWIDTH) );
    m_lvTags.InsertColumn( LVC_TYPE, cszListViewColumnNames[LVC_TYPE], LVCFMT_RIGHT, 60 );
    m_lvTags.InsertColumn( LVC_LINE, cszListViewColumnNames[LVC_LINE], LVCFMT_RIGHT, 60 );
    m_lvTags.InsertColumn( LVC_FILE, cszListViewColumnNames[LVC_FILE], LVCFMT_LEFT, 100 );

    //m_tbButtons.SetButtonState(IDM_PREVPOS, 0);
    //m_tbButtons.SetButtonState(IDM_NEXTPOS, 0);

    ApplyColors();

    if ( GetOptions().getBool(CTagsDlg::OPT_VIEW_SHOWTOOLTIPS) )
    {
        createTooltips();
    }

    eTagsViewMode viewMode = toViewMode( m_opt.getInt(OPT_VIEW_MODE) );
    eTagsSortMode sortMode = toSortMode( m_opt.getInt(OPT_VIEW_SORT) );
    SetViewMode(viewMode, sortMode);

    OnSize(true);

    checkCTagsExePath();

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

    // list-view window
    r = m_lvTags.GetWindowRect();
    ::MoveWindow( m_lvTags, left, top, width - 2*left, height - top - 2, (m_viewMode == TVM_LIST) );
    if ( GetOptions().getBool(CTagsDlg::OPT_VIEW_SHOWTOOLTIPS) )
    {
        if ( m_ttHints.GetHwnd() && m_ttHints.IsWindow() )
            m_ttHints.SetToolRect(m_lvTags.GetClientRect(), m_lvTags, IDC_LV_TAGS);
    }

    // tree-view window
    r = m_tvTags.GetWindowRect();
    ::MoveWindow( m_tvTags, left, top, width - 2*left, height - top - 2, (m_viewMode == TVM_TREE) );
    if ( GetOptions().getBool(CTagsDlg::OPT_VIEW_SHOWTOOLTIPS) )
    {
        if ( m_ttHints.GetHwnd() && m_ttHints.IsWindow() )
            m_ttHints.SetToolRect(m_tvTags.GetClientRect(), m_tvTags, IDC_TV_TAGS);
    }


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
        m_pEdWr->ewClearNavigationHistory(false);
        m_pEdWr->ewDoParseFile(true);
    }
}

bool CTagsDlg::GoToTag(const tString& filePath, const TCHAR* cszTagName)  // not implemented yet
{
    if ( cszTagName && cszTagName[0] && m_tags && !m_tags->empty() )
    {
        CTagsResultParser::file_tags& fileTags = (*m_tags)[filePath];
        CTagsResultParser::file_tags::iterator itr = getTagByName(fileTags, cszTagName);
        if ( itr != fileTags.end() )
        {
        }
    }

    return false;
}

namespace
{
    enum eUnicodeType {
        enc_NotUnicode = 0,
        enc_UCS2_LE,
        enc_UCS2_BE,
        enc_UTF8
    };

    eUnicodeType isUnicodeFile(HANDLE hFile)
    {
        if ( hFile )
        {
            unsigned char data[4];

            LONG nOffsetHigh = 0;
            ::SetFilePointer(hFile, 0, &nOffsetHigh, FILE_BEGIN);

            DWORD dwBytesRead = 0;
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

    void convertUcs2beToUcs2le(wchar_t* pszUCS2, int lenUCS2)
    {
        // swap high and low bytes
        wchar_t* p = pszUCS2;
        const wchar_t* pEnd = pszUCS2 + lenUCS2;
        while ( p < pEnd )
        {
            const wchar_t wch = *p;
            *p = ((wch >> 8) & 0x00FF) + (((wch & 0x00FF) << 8) & 0xFF00);
            ++p;
        }
    }

    std::unique_ptr<wchar_t[]> readFromFileAsUcs2(HANDLE hFile, int lenUCS2)
    {
        std::unique_ptr<wchar_t[]> pUCS2(new wchar_t[lenUCS2 + 1]);
        DWORD dwBytesRead = 0;
        bool isRead = false;

        if ( ::ReadFile(hFile, pUCS2.get(), 2*lenUCS2, &dwBytesRead, NULL) )
        {
            if ( dwBytesRead == 2*lenUCS2 )
            {
                pUCS2[lenUCS2] = 0;
                isRead = true;
            }
        }

        if ( !isRead )
        {
            wchar_t* ptr = pUCS2.release();
            delete [] ptr;
        }

        return pUCS2;
    }

    bool writeToFileAsUtf8(LPCTSTR pszFileName, const wchar_t* pszUCS2, int lenUCS2)
    {
        bool isFileWritten = false;

        int lenUTF8 = ::WideCharToMultiByte(CP_UTF8, 0, pszUCS2, lenUCS2, NULL, 0, NULL, NULL);
        if ( lenUTF8 > 0 )
        {
            std::unique_ptr<char[]> pUTF8(new char[lenUTF8 + 1]);

            pUTF8[0] = 0;
            ::WideCharToMultiByte(CP_UTF8, 0, pszUCS2, lenUCS2, pUTF8.get(), lenUTF8 + 1, NULL, NULL);
            pUTF8[lenUTF8] = 0;

            HANDLE hFile = ::CreateFile(pszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if ( hFile != INVALID_HANDLE_VALUE )
            {
                // UTF-8 BOM bytes
                // ::WriteFile(hTempFile, (LPCVOID) "\xEF\xBB\xBF", 3, &dwSize, NULL);

                DWORD dwBytesWritten = 0;
                if ( ::WriteFile(hFile, pUTF8.get(), lenUTF8, &dwBytesWritten, NULL) )
                {
                    if ( dwBytesWritten == lenUTF8 )
                    {
                        isFileWritten = true;
                    }
                }

                ::CloseHandle(hFile);

                if ( !isFileWritten )
                {
                    ::DeleteFile(pszFileName);
                }
            }
        }

        return isFileWritten;
    }

    void createUniqueTempFilesIfNeeded(LPCTSTR cszFileName, CTagsDlg::tCTagsThreadParam* tt)
    {
        TCHAR szUniqueId[32];
        TCHAR szTempPath[MAX_PATH + 1];

        szUniqueId[0] = 0;
        szTempPath[0] = 0;

        auto getTempPath = [](TCHAR pszTempPath[])
        {
            if ( pszTempPath[0] == 0 )
            {
                DWORD dwLen = ::GetTempPath(MAX_PATH, pszTempPath);
                if ( dwLen > 0 )
                {
                    --dwLen;
                    if ( pszTempPath[dwLen] != _T('\\') && pszTempPath[dwLen] != _T('/') )
                        pszTempPath[dwLen] = _T('\\');
                }
            }
        };

        auto getUniqueId = [](TCHAR pszUniqueId[])
        {
            if ( pszUniqueId[0] == 0 )
            {
                wsprintf(pszUniqueId, _T("%u_%X"), ::GetCurrentProcessId(), ::GetTickCount());
            }
        };

        HANDLE hFile = ::CreateFile(cszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if ( hFile == INVALID_HANDLE_VALUE )
        {
            hFile = ::CreateFile(cszFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        }
        if ( hFile != INVALID_HANDLE_VALUE )
        {
            DWORD dwFileSizeHigh = 0;
            DWORD dwFileSize = ::GetFileSize(hFile, &dwFileSizeHigh);

            eUnicodeType enc = isUnicodeFile(hFile);
            if ( enc == enc_NotUnicode )
            {
                tt->isUTF8 = false;
            }
            else if ( enc == enc_UTF8 )
            {
                tt->isUTF8 = true;
            }
            else if ( dwFileSize > 2 )
            {
                int lenUCS2 = (dwFileSize - 2)/2; // skip 2 leading BOM bytes
                std::unique_ptr<wchar_t[]> pUCS2 = readFromFileAsUcs2(hFile, lenUCS2);

                if ( pUCS2 )
                {
                    ::CloseHandle(hFile);
                    hFile = NULL;

                    if ( enc == enc_UCS2_BE )
                    {
                        convertUcs2beToUcs2le(pUCS2.get(), lenUCS2);
                    }

                    getTempPath(szTempPath);
                    getUniqueId(szUniqueId);

                    const TCHAR* pszExt = getFileExt(cszFileName);

                    // generating unique temp file name
                    auto& temp_file = tt->temp_input_file;
                    temp_file.clear();
                    temp_file.reserve(128);
                    temp_file += szTempPath;
                    temp_file += tt->pDlg->GetEditorShortName();
                    temp_file += _T("_inp_"); 
                    temp_file += szUniqueId;
                    if ( *pszExt )
                        temp_file += pszExt; // original file extension

                    if ( writeToFileAsUtf8(tt->temp_input_file.c_str(), pUCS2.get(), lenUCS2) )
                    {
                        tt->isUTF8 = true;
                    }
                    else
                    {
                        tt->temp_input_file.clear();
                    }
                }
            }

            if ( hFile != NULL )
                ::CloseHandle(hFile);
        }

        if ( !tt->pDlg->GetOptions().getBool(CTagsDlg::OPT_CTAGS_OUTPUTSTDOUT) )
        {
            getTempPath(szTempPath);
            getUniqueId(szUniqueId);

            // generating unique temp file name
            auto& temp_file = tt->temp_output_file;
            temp_file.clear();
            temp_file.reserve(128);
            temp_file += szTempPath;
            temp_file += tt->pDlg->GetEditorShortName();
            temp_file += _T("tags_"); 
            temp_file += szUniqueId;
            temp_file += _T(".txt");
        }
    }

    std::list<tString> getRelatedSourceFiles(const tString& fileName)
    {
        // C/C++ source files
        static const TCHAR* arrSourceCCpp[] = {
            _T(".c"),
            _T(".cc"),
            _T(".cpp"),
            _T(".cxx"),
            nullptr
        };

        // C/C++ header files
        static const TCHAR* arrHeaderCCpp[] = {
            _T(".h"),
            _T(".hh"),
            _T(".hpp"),
            _T(".hxx"),
            nullptr
        };

        auto processFileExtensions = [](const tString& fileName,
                                        const TCHAR* arrExts1[],
                                        const TCHAR* arrExts2[],
                                        std::list<tString>& relatedFiles) -> bool
        {
            int nMatchIndex = -1;

            const TCHAR* pszExt = getFileExt(fileName.c_str());
            if ( *pszExt )
            {
                for ( int i = 0; arrExts1[i] != nullptr && nMatchIndex == -1; ++i )
                {
                    if ( lstrcmpi(pszExt, arrExts1[i]) == 0 )
                        nMatchIndex = i;
                }

                if ( nMatchIndex != -1 )
                {
                    tString relatedFile = fileName;
                    for ( int i = 0; arrExts2[i] != nullptr; ++i )
                    {
                        relatedFile.erase(relatedFile.rfind(_T('.')));
                        relatedFile += arrExts2[i];
                        if ( isFileExist(relatedFile.c_str()) )
                        {
                            relatedFiles.push_back(relatedFile);
                        }
                    }
                }
            }

            return (nMatchIndex != -1);
        };

        std::list<tString> relatedFiles;

        if ( processFileExtensions(fileName, arrSourceCCpp, arrHeaderCCpp, relatedFiles) )
            return relatedFiles;

        if ( processFileExtensions(fileName, arrHeaderCCpp, arrSourceCCpp, relatedFiles) )
            return relatedFiles;

        return std::list<tString>();
    }

    tString getCtagsLangFamily(const tString& filePath)
    {
        struct tCtagsLangFamily
        {
            const TCHAR* cszLangName;
            std::vector<const TCHAR*> arrLangFiles;
        };

        // Note: this list can be obtained by running `ctags --list-maps`
        static const tCtagsLangFamily languages[] = {
            { _T("Abaqus"),          { _T(".inp") } },
            { _T("Abc"),             { _T(".abc") } },
            { _T("Ada"),             { _T(".adb"), _T(".ads"), _T(".ada") } },
            { _T("Ant"),             { _T("build.xml"), _T(".build.xml"), _T(".ant") } },
            { _T("Asciidoc"),        { _T(".asc"), _T(".adoc"), _T(".asciidoc") } },
            { _T("Asm"),             { _T(".a51"), _T(".29k"), _T(".x86"), _T(".asm"), _T(".s") } },
            { _T("Asp"),             { _T(".asp"), _T(".asa") } },
            { _T("Autoconf"),        { _T("configure.in"), _T(".in"), _T(".ac") } },
            { _T("AutoIt"),          { _T(".au3") } },
            { _T("Automake"),        { _T(".am") } },
            { _T("Awk"),             { _T(".awk"), _T(".gawk"), _T(".mawk") } },
            { _T("Basic"),           { _T(".bas"), _T(".bi"), _T(".bm"), _T(".bb"), _T(".pb") } },
            { _T("BETA"),            { _T(".bet") } },
            { _T("BibTeX"),          { _T(".bib") } },
            { _T("Clojure"),         { _T(".clj"), _T(".cljs"), _T(".cljc") } },
            { _T("CMake"),           { _T("CMakeLists.txt"), _T(".cmake") } },
            { _T("C"),               { _T(".c") } },
            { _T("C++"),             { _T(".c++"), _T(".cc"), _T(".cp"), _T(".cpp"), _T(".cxx"), _T(".h"), _T(".h++"), _T(".hh"), _T(".hp"), _T(".hpp"), _T(".hxx"), _T(".inl") } },
            { _T("CSS"),             { _T(".css") } },
            { _T("C#"),              { _T(".cs") } },
            { _T("Ctags"),           { _T(".ctags") } },
            { _T("Cobol"),           { _T(".cbl"), _T(".cob") } },
            { _T("CUDA"),            { _T(".cu"), _T(".cuh") } },
            { _T("D"),               { _T(".d"), _T(".di") } },
            { _T("Diff"),            { _T(".diff"), _T(".patch") } },
            { _T("DTD"),             { _T(".dtd"), _T(".mod") } },
            { _T("DTS"),             { _T(".dts"), _T(".dtsi") } },
            { _T("DosBatch"),        { _T(".bat"), _T(".cmd") } },
            { _T("Eiffel"),          { _T(".e") } },
            { _T("Elixir"),          { _T(".ex"), _T(".exs") } },
            { _T("EmacsLisp"),       { _T(".el") } },
            { _T("Erlang"),          { _T(".erl"), _T(".hrl") } },
            { _T("Falcon"),          { _T(".fal"), _T(".ftd") } },
            { _T("Flex"),            { _T(".as"), _T(".mxml") } },
            { _T("Fortran"),         { _T(".f"), _T(".for"), _T(".ftn"), _T(".f77"), _T(".f90"), _T(".f95"), _T(".f03"), _T(".f08"), _T(".f15") } },
            { _T("Fypp"),            { _T(".fy") } },
            { _T("Gdbinit"),         { _T(".gdbinit"), _T(".gdb") } },
            { _T("GDScript"),        { _T(".gd") } },
            { _T("GemSpec"),         { _T(".gemspec") } },
            { _T("Go"),              { _T(".go") } },
            { _T("Haskell"),         { _T(".hs") } },
            { _T("Haxe"),            { _T(".hx") } },
            { _T("HTML"),            { _T(".htm"), _T(".html") } },
            { _T("Iniconf"),         { _T(".ini"), _T(".conf") } },
            { _T("Inko"),            { _T(".inko") } },
            { _T("ITcl"),            { _T(".itcl") } },
            { _T("Java"),            { _T(".java") } },
            { _T("JavaProperties"),  { _T(".properties") } },
            { _T("JavaScript"),      { _T(".js"), _T(".jsx"), _T(".mjs") } },
            { _T("JSON"),            { _T(".json") } },
            { _T("Julia"),           { _T(".jl") } },
            { _T("LdScript"),        { _T(".lds.s"), _T("ld.script"), _T(".lds"), _T(".scr"), _T(".ld"), _T(".ldi") } },
            { _T("LEX"),             { _T(".lex"), _T(".l") } },
            { _T("Lisp"),            { _T(".cl"), _T(".clisp"), _T(".l"), _T(".lisp"), _T(".lsp") } },
            { _T("LiterateHaskell"), { _T(".lhs") } },
            { _T("Lua"),             { _T(".lua") } },
            { _T("M4"),              { _T(".m4"), _T(".spt") } },
            { _T("Man"),             { _T(".1"), _T(".2"), _T(".3"), _T(".4"), _T(".5"), _T(".6"), _T(".7"), _T(".8"), _T(".9"), _T(".3pm"), _T(".3stap"), _T(".7stap") } },
            { _T("Make"),            { _T("makefile"), _T("GNUmakefile"), _T(".mak"), _T(".mk") } },
            { _T("Markdown"),        { _T(".md"), _T(".markdown") } },
            { _T("MatLab"),          { _T(".m") } },
            { _T("Meson"),           { _T("meson.build") } },
            { _T("MesonOptions"),    { _T("meson_options.txt") } },
            { _T("Myrddin"),         { _T(".myr") } },
            { _T("NSIS"),            { _T(".nsi"), _T(".nsh") } },
            { _T("ObjectiveC"),      { _T(".mm"), _T(".m"), _T(".h") } },
            { _T("OCaml"),           { _T(".ml"), _T(".mli"), _T(".aug") } },
            { _T("Org"),             { _T(".org") } },
            { _T("Passwd"),          { _T("passwd") } },
            { _T("Pascal"),          { _T(".p"), _T(".pas") } },
            { _T("Perl"),            { _T(".pl"), _T(".pm"), _T(".ph"), _T(".plx"), _T(".perl") } },
            { _T("Perl6"),           { _T(".p6"), _T(".pm6"), _T(".pm"), _T(".pl6") } },
            { _T("PHP"),             { _T(".php"), _T(".php3"), _T(".php4"), _T(".php5"), _T(".php7"), _T(".phtml") } },
            { _T("Pod"),             { _T(".pod") } },
            { _T("PowerShell"),      { _T(".ps1"), _T(".psm1") } },
            { _T("Protobuf"),        { _T(".proto") } },
            { _T("PuppetManifest"),  { _T(".pp") } },
            { _T("Python"),          { _T(".py"), _T(".pyx"), _T(".pxd"), _T(".pxi"), _T(".scons"), _T(".wsgi") } },
            { _T("QemuHX"),          { _T(".hx") } },
            { _T("RMarkdown"),       { _T(".rmd") } },
            { _T("R"),               { _T(".r"), _T(".s"), _T(".q") } },
            { _T("Rake"),            { _T("Rakefile"), _T(".rake") } },
            { _T("REXX"),            { _T(".cmd"), _T(".rexx"), _T(".rx") } },
            { _T("Robot"),           { _T(".robot") } },
            { _T("RpmSpec"),         { _T(".spec") } },
            { _T("ReStructuredText"),{ _T(".rest"), _T(".rst") } },
            { _T("Ruby"),            { _T(".rb"), _T(".ruby") } },
            { _T("Rust"),            { _T(".rs") } },
            { _T("Scheme"),          { _T(".sch"), _T(".scheme"), _T(".scm"), _T(".sm"), _T(".rkt") } },
            { _T("SCSS"),            { _T(".scss") } },
            { _T("Sh"),              { _T(".sh"), _T(".bsh"), _T(".bash"), _T(".ksh"), _T(".zsh"), _T(".ash") } },
            { _T("SLang"),           { _T(".sl") } },
            { _T("SML"),             { _T(".sml"), _T(".sig") } },
            { _T("SQL"),             { _T(".sql") } },
            { _T("SystemdUnit"),     { _T(".service"), _T(".socket"), _T(".device"), _T(".mount"), _T(".automount"), _T(".swap"), _T(".target"), _T(".path"), _T(".timer"), _T(".snapshot"), _T(".slice") } },
            { _T("SystemTap"),       { _T(".stp"), _T(".stpm") } },
            { _T("Tcl"),             { _T(".tcl"), _T(".tk"), _T(".wish"), _T(".exp") } },
            { _T("Tex"),             { _T(".tex") } },
            { _T("TTCN"),            { _T(".ttcn"), _T(".ttcn3") } },
            { _T("Txt2tags"),        { _T(".t2t") } },
            { _T("TypeScript"),      { _T(".ts") } },
            { _T("Vera"),            { _T(".vr"), _T(".vri"), _T(".vrh") } },
            { _T("Verilog"),         { _T(".v") } },
            { _T("SystemVerilog"),   { _T(".sv"), _T(".svh"), _T(".svi") } },
            { _T("VHDL"),            { _T(".vhdl"), _T(".vhd") } },
            { _T("Vim"),             { _T("vimrc"), _T(".vimrc"), _T("_vimrc"), _T("gvimrc"), _T(".gvimrc"), _T("_gvimrc"), _T(".vim"), _T(".vba") } },
            { _T("WindRes"),         { _T(".rc") } },
            { _T("YACC"),            { _T(".y") } },
            { _T("YumRepo"),         { _T(".repo") } },
            { _T("Zephir"),          { _T(".zep") } },
            { _T("Glade"),           { _T(".glade") } },
            { _T("Maven2"),          { _T("pom.xml"), _T(".pom"), _T(".xml") } },
            { _T("PlistXML"),        { _T(".plist") } },
            { _T("RelaxNG"),         { _T(".rng") } },
            { _T("SVG"),             { _T(".svg") } },
            { _T("XML"),             { _T(".xml") } },
            { _T("XSLT"),            { _T(".xsl"), _T(".xslt") } },
            { _T("Yaml"),            { _T(".yml"), _T(".yaml") } },
            { _T("OpenAPI"),         { _T("openapi.yaml") } },
            { _T("Varlink"),         { _T(".varlink") } },
            { _T("Kotlin"),          { _T(".kt"), _T(".kts") } },
            { _T("Thrift"),          { _T(".thrift") } },
            { _T("Elm"),             { _T(".elm") } },
            { _T("RDoc"),            { _T(".rdoc") } }
        };

        auto ends_with = [](const tString& fileName, const TCHAR* cszEnd) -> bool
        {
            size_t nEndLen = lstrlen(cszEnd);
            if ( nEndLen <= fileName.length() )
            {
                if ( lstrcmpi(fileName.c_str() + fileName.length() - nEndLen, cszEnd) == 0 )
                    return true;
            }
            return false;
        };

        const TCHAR* pszExt = getFileExt(filePath.c_str());
        const TCHAR* pszFileName = getFileName(filePath);
        for ( const tCtagsLangFamily& lang : languages )
        {
            for ( const TCHAR* pattern : lang.arrLangFiles )
            {
                if ( pattern[0] == _T('.') )
                {
                    if ( *pszExt )
                    {
                        if ( ends_with(filePath, pattern) )
                        {
                            if ( lstrcmp(lang.cszLangName, _T("C")) == 0 || lstrcmp(lang.cszLangName, _T("C++")) == 0 )
                            {
                                return tString(_T("C,C++"));
                            }
                            return tString(lang.cszLangName);
                        }
                    }
                }
                else
                {
                    if ( lstrcmpi(pszFileName, pattern) == 0 )
                    {
                        return tString(lang.cszLangName);
                    }
                }
            }
        }

        return tString();
    }
}

void CTagsDlg::ParseFile(const TCHAR* const cszFileName, bool bReparsePhysicalFile)
{
    if ( !cszFileName )
    {
        ClearItems(true);
        m_tags = nullptr;
        return;
    }

    bool bJustAdded = false;
    tags_map* prev_tags = m_tags;
    m_tags = getCachedTagsMap(cszFileName, true, bJustAdded);

    if ( !bJustAdded && !bReparsePhysicalFile )
    {
        if ( m_tags != prev_tags )
        {
            ClearItems(true);
            ::SendNotifyMessage( this->GetHwnd(), WM_UPDATETAGSVIEW, 0, 0 );
        }
        else
        {
            UpdateCurrentItem();
        }
        return;
    }

    ClearItems(true);

    if ( !isFileExist(cszFileName) )
        return;

    TCHAR szExt[20];
    szExt[0] = 0;

    const TCHAR* pszExt = getFileExt(cszFileName);
    if ( *pszExt )
    {
        lstrcpyn(szExt, pszExt, 18);

        int nLen = 0;
        const TCHAR* pszSkipFileExts = m_opt.getStr(OPT_CTAGS_SKIPFILEEXTS, &nLen);
        if ( pszSkipFileExts && pszSkipFileExts[0] )
        {
            std::unique_ptr<TCHAR[]> pSkipExts(new TCHAR[nLen + 1]);
            lstrcpy(pSkipExts.get(), pszSkipFileExts);

            ::CharLower(pSkipExts.get());
            ::CharLower(szExt);

            if ( wcsstr(pSkipExts.get(), szExt) != NULL )
                return;
        }
    }

    tString ctagsOptPath = m_ctagsExeFilePath;
    if ( ctagsOptPath.length() > 3 )
    {
        tString::size_type len = ctagsOptPath.length();
        ctagsOptPath[len - 3] = _T('o');
        ctagsOptPath[len - 2] = _T('p');
        ctagsOptPath[len - 1] = _T('t');
        if ( !isFileExist(ctagsOptPath.c_str()) )
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

    tCTagsThreadParam* tt = new tCTagsThreadParam;
    tt->pDlg = this;
    tt->source_file_name = cszFileName;

    createUniqueTempFilesIfNeeded(cszFileName, tt);

    if ( tt->temp_input_file != m_ctagsTempInputFilePath )
    {
        if ( m_opt.getInt(OPT_DEBUG_DELETETEMPINPUTFILE) != DTF_NEVERDELETE )
        {
            DeleteTempFile(m_ctagsTempInputFilePath);
        }
        m_ctagsTempInputFilePath = tt->temp_input_file;
    }

    if ( tt->temp_output_file != m_ctagsTempOutputFilePath )
    {
        if ( m_opt.getInt(OPT_DEBUG_DELETETEMPOUTPUTFILE) != DTF_NEVERDELETE )
        {
            DeleteTempFile(m_ctagsTempOutputFilePath);
        }
        m_ctagsTempOutputFilePath = tt->temp_output_file;
    }

    // ctags command line:
    //
    //   --options=<pathname> 
    //       Read additional options from file or directory.
    //
    //   -f -
    //       tags are written to standard output.
    //
    //   --fields=fKnste
    //       file/f       Indicates that the tag has file-limited visibility.
    //       K            Kind of tag as long-name.
    //       line/n       The line number where name is defined or referenced in input.
    //       s            Scope of tag definition.
    //       typeref/t    Type and name of a variable, typedef, or return type of callable like function as typeref: field.
    //       end/e        Indicates the line number of the end lines of the language object.
    //
    tt->cmd_line.clear();
    tt->cmd_line.reserve(512);
    tt->cmd_line += _T("\"");
    tt->cmd_line += m_ctagsExeFilePath;
    if ( !ctagsOptPath.empty() )
    {
        tt->cmd_line += _T("\" --options=\"");
        tt->cmd_line += ctagsOptPath;
    }
    tt->cmd_line += _T("\" -f ");
    if ( tt->temp_output_file.empty() )
    {
        tt->cmd_line += _T("-");
    }
    else
    {
        tt->cmd_line += _T("\"");
        tt->cmd_line += tt->temp_output_file;
        tt->cmd_line += _T("\"");
    }
    tt->cmd_line += _T(" --fields=fKnste ");
    if ( tt->temp_input_file.empty() )
    {
        if ( !m_opt.getBool(OPT_CTAGS_SCANFOLDER) )
        {
            tt->cmd_line += _T("\"");
            tt->cmd_line += tt->source_file_name;
            tt->cmd_line += _T("\"");

            std::list<tString> relatedFiles = getRelatedSourceFiles(tt->source_file_name);
            if ( !relatedFiles.empty() )
            {
                for ( auto& relatedFile : relatedFiles )
                {
                    tt->cmd_line += _T(" \"");
                    tt->cmd_line += relatedFile;
                    tt->cmd_line += _T("\"");
                }
            }
        }
        else
        {
            if ( m_opt.getBool(OPT_CTAGS_SCANFOLDERRECURSIVELY) )
            {
                // Note: the recursive scanning may generate a tag file of several MB that takes toooooo long to visualize
                // TODO: consider virtual TreeView & ListView
                //tt->cmd_line += _T("--recurse=yes ");
            }

            tString ctagsLang = getCtagsLangFamily(tt->source_file_name);
            if ( !ctagsLang.empty() )
            {
                tt->cmd_line += _T("--languages=\"");
                tt->cmd_line += ctagsLang;
                tt->cmd_line += _T("\" ");
            }

            size_t n = tt->source_file_name.find_last_of(_T("\\/"));
            tString source_dir(tt->source_file_name.c_str(), n + 2); // ends with "\X" or "/X"
            source_dir.back() = _T('*'); // now ends with "\*" or "/*"

            tt->cmd_line += _T("\"");
            tt->cmd_line += source_dir;
            tt->cmd_line += _T("\"");
        }
    }
    else
    {
        tt->cmd_line += _T("\"");
        tt->cmd_line += tt->temp_input_file;
        tt->cmd_line += _T("\"");
    }

    ::ResetEvent(m_hTagsThreadEvent);
    HANDLE hThread = ::CreateThread(NULL, 0, CTagsThreadProc, tt, 0, &tt->dwThreadID);
    if ( hThread )
    {
        ::InterlockedIncrement(&m_nTagsThreadCount);
        m_dwLastTagsThreadID = tt->dwThreadID;
        ::CloseHandle(hThread);
    }
    ::SetEvent(m_hTagsThreadEvent);
}

void CTagsDlg::ReparseCurrentFile()
{
    OnParseClicked();
}

void CTagsDlg::SetSortMode(eTagsSortMode sortMode)
{
    if ( sortMode == TSM_FILE )
    {
        sortMode = TSM_LINE;
    }

    if ( (sortMode != TSM_NONE) && (sortMode != m_sortMode) )
    {
        m_opt.setInt( OPT_VIEW_SORT, (int) sortMode );

        m_prevSelStart = -1;

        m_csTagsItems.Lock();
            deleteAllItems(true);
        m_csTagsItems.Release();

        if ( m_viewMode == TVM_TREE )
        {
            m_tvTags.SetRedraw(FALSE);

            switch ( sortMode )
            {
                case TSM_LINE:
                case TSM_TYPE:
                    sortTagsTV(TSM_LINE);
                    m_tbButtons.SetButtonState(IDM_SORTNAME, TBSTATE_ENABLED);
                    m_tbButtons.SetButtonState(IDM_SORTLINE, TBSTATE_ENABLED | TBSTATE_CHECKED);
                    break;

                case TSM_NAME:
                    sortTagsTV(TSM_NAME);
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
                    sortTagsByLineLV();
                    m_tbButtons.SetButtonState(IDM_SORTNAME, TBSTATE_ENABLED);
                    m_tbButtons.SetButtonState(IDM_SORTLINE, TBSTATE_ENABLED | TBSTATE_CHECKED);
                    break;

                case TSM_TYPE:
                    sortTagsByNameOrTypeLV(TSM_TYPE);
                    break;

                case TSM_NAME:
                    sortTagsByNameOrTypeLV(TSM_NAME);
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
    if ( m_pEdWr && m_tags && !m_tags->empty() && !m_isUpdatingSelToItem )
    {
        int selEnd, selStart = m_pEdWr->ewGetSelectionPos(selEnd);

        if ( m_prevSelStart != selStart )
        {
            m_prevSelStart = selStart;

            int line = m_pEdWr->ewGetLineFromPos(selStart) + 1; // 1-based line

            const tString filePath = m_pEdWr->ewGetFilePathName();
            auto fileItr = m_tags->find(filePath);
            if ( fileItr == m_tags->end() )
            {
                return;
            }

            CTagsResultParser::file_tags& fileTags = fileItr->second;
            CTagsResultParser::file_tags::const_iterator itr = findTagByLine(fileTags, line);
            if ( itr == fileTags.end() )
            {
                if ( itr != fileTags.begin() )
                {
                    --itr;
                    if ( line < (*itr)->line )
                    {
                        auto itrBegin = fileTags.begin();
                        int diff_last = (*itr)->line - line;
                        int diff_first = (*itrBegin)->line - line;
                        if ( diff_first < 0 )
                            diff_first = -diff_first;
                        if ( diff_first < diff_last )
                            itr = itrBegin;
                    }
                }
                else
                    return;
            }

            if ( m_viewMode == TVM_TREE )
            {
                HTREEITEM hItem = (HTREEITEM) (*itr)->data.p;
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
                int iItem = (*itr)->data.i;
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
    if ( m_tags && m_tags->size() == 0 )
    {
        m_tags->insert( std::make_pair(m_pEdWr->ewGetFilePathName(), file_tags()) );
    }

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

LPCTSTR CTagsDlg::GetEditorShortName() const
{
    return ( m_pEdWr ? m_pEdWr->ewGetEditorShortName() : _T("") );
}

void CTagsDlg::deleteAllItems(bool bDelayedRedraw)
{
    if ( m_lvTags.IsWindow() )
    {
        if ( bDelayedRedraw )
            m_lvTags.SetRedraw(FALSE);

        m_lvTags.DeleteAllItems();

        if ( bDelayedRedraw )
            m_lvTags.SetRedraw(TRUE);
    }

    if ( m_tvTags.IsWindow() )
    {
        if ( bDelayedRedraw )
            m_tvTags.SetRedraw(FALSE);

        m_tvTags.DeleteAllItems();

        if ( bDelayedRedraw )
            m_tvTags.SetRedraw(TRUE);
    }
}

int CTagsDlg::addListViewItem(int nItem, const tTagData* pTag)
{
    LVITEM lvi;
    tString tagName = pTag->getFullTagName();

    ::ZeroMemory(&lvi, sizeof(lvi));
    lvi.iItem = nItem;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText = const_cast<TCHAR*>(tagName.c_str());
    lvi.cchTextMax = static_cast<int>(tagName.length());
    lvi.lParam = (LPARAM) pTag;
    int nRet = m_lvTags.InsertItem(lvi);

    m_lvTags.SetItemText(nItem, LVC_TYPE, pTag->tagType.c_str());

    TCHAR ts[20];
    wsprintf(ts, _T("%d"), pTag->line);
    m_lvTags.SetItemText(nItem, LVC_LINE, ts);

    m_lvTags.SetItemText(nItem, LVC_FILE, getFileName(pTag->filePath));

    return nRet;
}

HTREEITEM CTagsDlg::addTreeViewItem(HTREEITEM hParent, const tString& tagName, tTagData* pTag)
{
    TVINSERTSTRUCT tvis;

    ::ZeroMemory(&tvis, sizeof(tvis));
    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvis.item.pszText = const_cast<TCHAR*>(tagName.c_str());
    tvis.item.cchTextMax = static_cast<int>(tagName.length());
    tvis.item.lParam = (LPARAM) pTag;

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

void CTagsDlg::ClearItems(bool bDelayedRedraw )
{
    m_csTagsItems.Lock();
        deleteAllItems(bDelayedRedraw);
    m_csTagsItems.Release();

    if ( m_tbButtons.IsWindow() )
    {
        m_tbButtons.SetButtonState(IDM_PREVPOS, 0);
        m_tbButtons.SetButtonState(IDM_NEXTPOS, 0);
    }

    m_prevSelStart = -1;
}

void CTagsDlg::ClearCachedTags()
{
    m_cachedTags.clear();
    m_tags = nullptr;
}

void CTagsDlg::PurifyCachedTags()
{
    IEditorWrapper::file_set openedFiles = m_pEdWr->ewGetOpenedFilePaths();
    std::list<std::list<tags_map>::const_iterator> itemsToDelete;

    std::list<tags_map>::const_iterator itrEnd = m_cachedTags.end();
    std::list<tags_map>::const_iterator itrTags = m_cachedTags.begin();
    for ( ; itrTags != itrEnd; ++itrTags )
    {
        const tags_map& tagsMap = *itrTags;
        auto itrFile = std::find_if(tagsMap.begin(), tagsMap.end(),
            [&openedFiles](const tags_map::value_type& fileItem){ return (openedFiles.find(fileItem.first) != openedFiles.end()); }
        );
        if ( itrFile == tagsMap.end() )
        {
            itemsToDelete.push_back(itrTags);
        }
    }

    for ( auto& itr : itemsToDelete )
    {
        m_cachedTags.erase(itr);
    }

    if ( m_cachedTags.empty() )
    {
        m_tags = nullptr;
    }

    if ( openedFiles.empty() )
    {
        ClearItems(true);
    }
}

void CTagsDlg::OnSettingsChanged()
{
    if ( GetHwnd() )
    {
        if ( IsWindowVisible() )
        {
            ApplyColors();
        }

        if ( IsWindow() )
        {
            if ( GetOptions().getBool(CTagsDlg::OPT_VIEW_SHOWTOOLTIPS) )
                createTooltips();
            else
                destroyTooltips();
        }
    }
}

void CTagsDlg::OnFileClosed()
{
    PurifyCachedTags();
}

void CTagsDlg::OnTagDblClicked(const tTagData* pTagData)
{
    if ( pTagData )
    {
        if ( !pTagData->filePath.empty() )
        {
            const tString& filePath = pTagData->filePath;
            tString currFilePath = m_pEdWr->ewGetFilePathName();
            if ( lstrcmpi(filePath.c_str(), currFilePath.c_str()) != 0 )
            {
                m_isUpdatingSelToItem = true;
                m_pEdWr->ewDoOpenFile(filePath.c_str());
            }
        }

        const int line = pTagData->line;
        if ( line >= 0 )
        {
            m_isUpdatingSelToItem = true;
            m_pEdWr->ewSetNavigationPoint( _T("") );
            //UpdateNavigationButtons();

            int pos = m_pEdWr->ewGetPosFromLine(line - 1);
            m_prevSelStart = pos;
            m_pEdWr->ewDoSetSelection(pos, pos);

            m_pEdWr->ewSetNavigationPoint( _T("") );
            UpdateNavigationButtons();

            m_pEdWr->ewDoSetFocus();
            m_isUpdatingSelToItem = false;
        }
    }
}

void CTagsDlg::checkCTagsExePath()
{
    if ( !isFileExist(m_ctagsExeFilePath.c_str()) )
    {
        this->PostMessage(WM_CTAGSPATHFAILED, 0, 0);
    }
}

CTagsResultParser::file_tags::iterator CTagsDlg::getTagByLine(CTagsResultParser::file_tags& fileTags, const int line)
{
    CTagsResultParser::file_tags::iterator itr = std::upper_bound(
        fileTags.begin(),
        fileTags.end(),
        line,
        [](const int line, const std::unique_ptr<tTagData>& pTag){ return (line < pTag->line); }
    ); // returns an item with itr->second.line > line

    if ( itr == fileTags.end() && itr != fileTags.begin() )
    {
        // there is no item with itr->second.line > line, so should be itr->second.line == line
        --itr;
    }

    if ( itr != fileTags.end() )
    {
        auto itr2 = itr;

        itr = fileTags.end(); // will return end() if there is no itr2 does not succeed

        for ( ; ; --itr2 )
        {
            if ( (*itr2)->line <= line )
            {
                itr = itr2; // success!
                break;
            }

            if ( itr2 == fileTags.begin() )
            {
                // We should not get here, but let's have it to be able to set a breakpoint
                break;
            }
        }
    }

    return itr;
}

CTagsResultParser::file_tags::iterator CTagsDlg::findTagByLine(CTagsResultParser::file_tags& fileTags, const int line)
{
    CTagsResultParser::file_tags::iterator itr = getTagByLine(fileTags, line);

    if ( itr != fileTags.end() )
    {
        for ( auto itr2 = itr; ; --itr2 )
        {
            if ( (*itr2)->end_line >= line )
            {
                itr = itr2;
                break;
            }

            if ( itr2 == fileTags.begin() )
                break;

            if ( (*itr2)->tagScope.empty() )
                break;
        }
    }

    return itr;
}

CTagsResultParser::file_tags::iterator CTagsDlg::getTagByName(CTagsResultParser::file_tags& fileTags, const tString& tagName)
{
    return std::find_if(fileTags.begin(), fileTags.end(),
        [&tagName](const std::unique_ptr<tTagData>& tag){ return (tag->tagName == tagName); }
    );
}

std::list<CTagsResultParser::tags_map>::iterator CTagsDlg::getCachedTagsMapItr(const TCHAR* cszFileName, bool bAddIfNotExist, bool& bJustAdded)
{
    bJustAdded = false;

    auto itrTags = std::find_if(m_cachedTags.begin(), m_cachedTags.end(),
        [&cszFileName](const tags_map& tagsMap){ return (tagsMap.find(cszFileName) != tagsMap.end()); }
    );
    if ( itrTags != m_cachedTags.end() )
        return itrTags;

    if ( bAddIfNotExist )
    {
        bJustAdded = true;
        m_cachedTags.push_back(tags_map());
        itrTags = m_cachedTags.end();
        --itrTags; // iterator to the last element
        itrTags->insert( std::make_pair(tString(cszFileName), file_tags()) );
    }

    return itrTags;
}

CTagsResultParser::tags_map* CTagsDlg::getCachedTagsMap(const TCHAR* cszFileName, bool bAddIfNotExist, bool& bJustAdded)
{
    auto itrTags = getCachedTagsMapItr(cszFileName, bAddIfNotExist, bJustAdded);
    return ( (itrTags != m_cachedTags.end()) ? &(*itrTags) : nullptr );
}

void CTagsDlg::initOptions()
{
    if ( m_opt.HasOptions() )
        return;

    const TCHAR cszView[]     = _T("View");
    const TCHAR cszColors[]   = _T("Colors");
    const TCHAR cszBehavior[] = _T("Behavior");
    const TCHAR cszCtags[]    = _T("Ctags");
    const TCHAR cszDebug[]    = _T("Debug");

    m_opt.ReserveMemory(OPT_COUNT);

    // View section
    m_opt.AddInt( OPT_VIEW_MODE,              cszView,  _T("Mode"),             TVM_LIST );
    m_opt.AddInt( OPT_VIEW_SORT,              cszView,  _T("Sort"),             TSM_LINE );
    m_opt.AddInt( OPT_VIEW_WIDTH,             cszView,  _T("Width"),            220      );
    m_opt.AddInt( OPT_VIEW_NAMEWIDTH,         cszView,  _T("NameWidth"),        220      );
    m_opt.AddBool( OPT_VIEW_SHOWTOOLTIPS,     cszView,  _T("ShowTooltips"),     true     );
    m_opt.AddBool( OPT_VIEW_ESCFOCUSTOEDITOR, cszView,  _T("EscFocusToEditor"), false    );

    // Colors section
    m_opt.AddBool( OPT_COLORS_USEEDITORCOLORS,  cszColors, _T("UseEditorColors"), true );
    //m_opt.AddHexInt( OPT_COLORS_BKGND,   cszColor, _T("BkGnd"),   0 );
    //m_opt.AddHexInt( OPT_COLORS_TEXT,    cszColor, _T("Text"),    0 );
    //m_opt.AddHexInt( OPT_COLORS_TEXTSEL, cszColor, _T("TextSel"), 0 );
    //m_opt.AddHexInt( OPT_COLORS_SELA,    cszColor, _T("SelA"),    0 );
    //m_opt.AddHexInt( OPT_COLORS_SELN,    cszColor, _T("SelN"),    0 );

    // Behavior section
    m_opt.AddBool( OPT_BEHAVIOR_PARSEONSAVE, cszBehavior, _T("ParseOnSave"), true );

    // Ctags section
    m_opt.AddBool( OPT_CTAGS_OUTPUTSTDOUT,          cszCtags, _T("OutputToStdout"), false );
    m_opt.AddBool( OPT_CTAGS_SCANFOLDER,            cszCtags, _T("ScanFolder"), false );
    m_opt.AddBool( OPT_CTAGS_SCANFOLDERRECURSIVELY, cszCtags, _T("ScanFolderRecursively"), false );
    m_opt.AddStr( OPT_CTAGS_SKIPFILEEXTS,           cszCtags, _T("SkipFileExts"), _T(".txt") );

    // Debug section
    m_opt.AddInt(OPT_DEBUG_DELETETEMPINPUTFILE, cszDebug, _T("DeleteTempInputFiles"), DTF_ALWAYSDELETE);
    m_opt.AddInt(OPT_DEBUG_DELETETEMPOUTPUTFILE, cszDebug, _T("DeleteTempOutputFiles"), DTF_ALWAYSDELETE);
}

bool CTagsDlg::isTagMatchFilter(const tString& tagName) const
{
    const int len_filt = static_cast<int>(m_tagFilter.length());
    const int len_name = static_cast<int>(tagName.length());
    if ( len_name < len_filt )
        return false;

    const int max_pos = len_name - len_filt;
    int pos = 0;
    while ( pos <= max_pos )
    {
        if ( ::CompareString(
                 LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                 tagName.c_str() + pos, len_filt,
                 m_tagFilter.c_str(), len_filt) == CSTR_EQUAL )
        {
            return true;
        }
        else
            ++pos;
    }

    return false;
}

size_t CTagsDlg::getNumTotalItemsForSorting() const
{
    size_t nTotalItems = 0;

    if ( m_tags )
    {
        for ( const auto& fileItem : *m_tags )
        {
            nTotalItems += fileItem.second.size();
        }
    }

    size_t nItemsToReserve = nTotalItems;
    if ( nTotalItems != 0 && !m_tagFilter.empty() )
    {
        if ( m_tagFilter.length() < 10 )
        {
            size_t nDivider = 1;
            for ( size_t i = m_tagFilter.length(); i != 0; --i )
            {
                nDivider *= 4;
            }
            nItemsToReserve /= nDivider;
            nItemsToReserve = 16*(1 + nItemsToReserve/16);
        }
        else
        {
            nItemsToReserve = 0;
        }
    }

    return nItemsToReserve;
}

size_t  CTagsDlg::getNumItemsForSorting(const CTagsDlg::file_tags& fileTags) const
{
    size_t nItemsToReserve = fileTags.size();
    if ( nItemsToReserve != 0 && !m_tagFilter.empty() )
    {
        if ( m_tagFilter.length() < 10 )
        {
            size_t nDivider = 1;
            for ( size_t i = m_tagFilter.length(); i != 0; --i )
            {
                nDivider *= 4;
            }
            nItemsToReserve /= nDivider;
        }
        else
        {
            nItemsToReserve = 0;
        }
    }
    return nItemsToReserve;
}

void CTagsDlg::sortTagsByLineLV()
{
    if ( !m_tags )
        return;

    int nItem = 0;
    for ( auto& fileItem : *m_tags )
    {
        CTagsResultParser::file_tags& fileTags = fileItem.second;
        for ( std::unique_ptr<tTagData>& pTag : fileTags )
        {
            if ( m_tagFilter.empty() || isTagMatchFilter(pTag->getFullTagName()) )
            {
                pTag->data.i = addListViewItem(nItem++, pTag.get());
            }
            else
            {
                pTag->data.i = -1;
            }
        }
    }
}

void CTagsDlg::sortTagsByNameOrTypeLV(eTagsSortMode sortMode)
{
    if ( !m_tags )
        return;

    std::vector<tTagData*> tags;
    tags.reserve(getNumTotalItemsForSorting());

    for ( auto& fileItem : *m_tags )
    {
        CTagsResultParser::file_tags& fileTags = fileItem.second;
        for ( std::unique_ptr<tTagData>& pTag : fileTags )
        {
            if ( m_tagFilter.empty() || isTagMatchFilter(pTag->getFullTagName()) )
            {
                tags.push_back(pTag.get());
            }
            else
            {
                pTag->data.i = -1;
            }
        }
    }

    if ( sortMode == TSM_NAME )
    {
        std::sort(
            tags.begin(),
            tags.end(),
            [](tTagData* pTag1, tTagData* pTag2){ return (pTag1->getFullTagName() < pTag2->getFullTagName()); }
        );
    }
    else // if ( sortMode == TSM_TYPE )
    {
        if ( m_sortMode == TSM_NAME )
        {
            // the previous sorting was by name, re-applying it first
            std::sort(
                tags.begin(),
                tags.end(),
                [](tTagData* pTag1, tTagData* pTag2){ return (pTag1->getFullTagName() < pTag2->getFullTagName()); }
            );
        }

        std::stable_sort(
            tags.begin(),
            tags.end(),
            [](tTagData* pTag1, tTagData* pTag2){ return (pTag1->tagType < pTag2->tagType); }
        );
    }

    int nItem = 0;
    for ( tTagData* pTag : tags )
    {
        int n = addListViewItem(nItem++, pTag);
        pTag->data.i = n;
    }
}

void CTagsDlg::sortTagsTV(eTagsSortMode sortMode)
{
    if ( !m_tags )
        return;

    struct tTagsByFile
    {
        tString filePath;
        std::vector<tTagData*> fileTags;
    };

    std::list<tTagsByFile> tags;

    for ( auto& fileItem : *m_tags )
    {
        CTagsResultParser::file_tags& fileTags = fileItem.second;

        tTagsByFile tagsByFile;
        tagsByFile.filePath = fileItem.first;
        tagsByFile.fileTags.reserve(getNumItemsForSorting(fileTags));

        for ( std::unique_ptr<tTagData>& pTag : fileTags )
        {
            if ( m_tagFilter.empty() || isTagMatchFilter(pTag->getFullTagName()) )
            {
                tagsByFile.fileTags.push_back(pTag.get());
            }
            else
            {
                pTag->data.p = nullptr;
            }
        }

        if ( !tagsByFile.fileTags.empty() )
        {
            if ( sortMode == TSM_LINE )
            {
                std::sort(
                    tagsByFile.fileTags.begin(),
                    tagsByFile.fileTags.end(),
                    [](tTagData* pTag1, tTagData* pTag2){ return (pTag1->line < pTag2->line); }
                );
            }
            else // if ( sortMode == TSM_NAME )
            {
                std::sort(
                    tagsByFile.fileTags.begin(),
                    tagsByFile.fileTags.end(),
                    [](tTagData* pTag1, tTagData* pTag2){ return (pTag1->getFullTagName() < pTag2->getFullTagName()); }
                );
            }

            tags.push_back(tagsByFile);
        }
    }

    auto setNodeItemExpanded = [](CTagsTreeView& tvTags, HTREEITEM hItem)
    {
        TVITEM tvi;

        ::ZeroMemory(&tvi, sizeof(tvi));
        tvi.hItem = hItem;
        tvi.mask = TVIF_HANDLE | TVIF_STATE;
        tvi.state = TVIS_EXPANDED;
        tvi.stateMask = TVIS_EXPANDED;

        tvTags.SetItem(tvi);
    };

    for ( tTagsByFile& tagsByFile : tags )
    {
        // adding a root file item
        HTREEITEM hFileItem = addTreeViewItem( TVI_ROOT, getFileName(tagsByFile.filePath), nullptr );
        setNodeItemExpanded(m_tvTags, hFileItem);

        std::map<tString, HTREEITEM> scopeMap;

        for ( tTagData* pTag : tagsByFile.fileTags )
        {
            auto scopeIt = scopeMap.find(!pTag->tagScope.empty() ? pTag->tagScope : pTag->tagName);
            if ( scopeIt != scopeMap.end() )
            {
                HTREEITEM hScopeItem = scopeIt->second;

                if ( !pTag->tagScope.empty() )
                {
                    HTREEITEM hItem = addTreeViewItem(hScopeItem, pTag->tagName, pTag);
                    pTag->data.p = (void *) hItem;
                }
            }
            else
            {
                HTREEITEM hScopeItem = hFileItem;

                if ( !pTag->tagScope.empty() )
                {
                    // adding a scope item
                    hScopeItem = addTreeViewItem(hFileItem, pTag->tagScope, nullptr);
                    scopeMap[pTag->tagScope] = hScopeItem;

                    if ( !m_tagFilter.empty() )
                        setNodeItemExpanded(m_tvTags, hScopeItem);
                }

                HTREEITEM hItem = addTreeViewItem(hScopeItem, pTag->tagName, pTag);
                if ( pTag->tagScope.empty() )
                {
                    // a scope item has been added
                    scopeMap[pTag->tagName] = hItem;

                    if ( !m_tagFilter.empty() )
                        setNodeItemExpanded(m_tvTags, hItem);
                }

                pTag->data.p = (void *) hItem;
            }
        }
    }
}
