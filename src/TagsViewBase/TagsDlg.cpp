#include "TagsDlg.h"
#include "ConsoleOutputRedirector.h"
#include "resource.h"
#include "win32++/include/wxx_textconv.h"
#include <memory>
#include <set>
#include <list>

namespace
{
    typedef CTagsResultParser::tTagData tTagData;

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

                        const tTagData* pTagData = (const tTagData *) lvi.lParam;
                        if ( pTagData )
                        {
                            if ( !pTagData->filePath.empty() )
                            {
                                const tString& filePath = pTagData->filePath;
                                tString currFilePath = m_pDlg->m_pEdWr->ewGetFilePathName();
                                if ( lstrcmpi(filePath.c_str(), currFilePath.c_str()) != 0 )
                                {
                                    m_pDlg->m_isUpdatingSelToItem = true;
                                    m_pDlg->m_pEdWr->ewDoOpenFile(filePath.c_str());
                                }
                            }

                            const int line = pTagData->line;
                            if ( line >= 0 )
                            {
                                m_pDlg->m_isUpdatingSelToItem = true;
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
                                m_pDlg->m_isUpdatingSelToItem = false;
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

                    const tTagData* pTagData = (const tTagData *) tvi.lParam;
                    if ( pTagData )
                    {
                        if ( !pTagData->filePath.empty() )
                        {
                            const tString& filePath = pTagData->filePath;
                            tString currFilePath = m_pDlg->m_pEdWr->ewGetFilePathName();
                            if ( lstrcmpi(filePath.c_str(), currFilePath.c_str()) != 0 )
                            {
                                m_pDlg->m_isUpdatingSelToItem = true;
                                m_pDlg->m_pEdWr->ewDoOpenFile(filePath.c_str());
                            }
                        }

                        const int line = pTagData->line;
                        if ( line >= 0 )
                        {
                            m_pDlg->m_isUpdatingSelToItem = true;
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
                            m_pDlg->m_isUpdatingSelToItem = false;
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
        /*case WM_ADDTAGS:
            OnAddTags( (const char *) lParam) );
            return 0;*/

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
    std::set<tString> relatedFiles;

    unsigned int nParseFlags = 0;
    if ( tt->isUTF8 )
    {
        nParseFlags |= CTagsResultParser::PF_ISUTF8;
    }

    CTagsResultParser::Parse(
        s,
        nParseFlags,
        CTagsResultParser::tParseContext(
            m_tags,
            relatedFiles,
            getFileDirectory(tt->source_file_name),
            tt->temp_input_file.empty() ? tString() : tt->source_file_name
        ) 
    );

    if ( !relatedFiles.empty() )
    {
        for ( auto& relatedFile : relatedFiles )
        {
            m_pEdWr->ewAddRelatedFile(relatedFile);
        }
    }

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
            PostMessage(WM_CLOSETAGSVIEW);
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
        m_pEdWr->ewDoParseFile();
    }
}

bool CTagsDlg::GoToTag(const tString& filePath, const TCHAR* cszTagName)  // not implemented yet
{
    if ( cszTagName && cszTagName[0] && !m_tags.empty() )
    {
        CTagsResultParser::file_tags_map& fileTags = m_tags[filePath].tagsMap;
        CTagsResultParser::file_tags_map::iterator itr = getTagByName(fileTags, cszTagName);
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

}

void CTagsDlg::ParseFile(const TCHAR* const cszFileName)
{
    ClearItems(true);

    if ( isFileExist(cszFileName) )
    {
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
                        m_pEdWr->ewAddRelatedFile(relatedFile);
                    }
                    m_pEdWr->ewAddRelatedFile(tt->source_file_name);
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

                size_t n = tt->source_file_name.find_last_of(_T("\\/"));
                tString source_dir(tt->source_file_name.c_str(), n + 2); // ends with "\X" or "/X"
                source_dir.back() = _T('*'); // now ends with "\*" or "/*"

                tt->cmd_line += _T("\"");
                tt->cmd_line += source_dir;
                tt->cmd_line += _T("\"");

                m_pEdWr->ewAddRelatedFile(tt->source_file_name);
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
                    m_sortMode = TSM_TYPE;
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
    if ( m_pEdWr && !m_tags.empty() && !m_isUpdatingSelToItem )
    {
        int selEnd, selStart = m_pEdWr->ewGetSelectionPos(selEnd);

        if ( m_prevSelStart != selStart )
        {
            m_prevSelStart = selStart;

            int line = m_pEdWr->ewGetLineFromPos(selStart) + 1; // 1-based line

            const tString filePath = m_pEdWr->ewGetFilePathName();
            auto fileItr = m_tags.find(filePath);
            if ( fileItr == m_tags.end() )
            {
                return;
            }

            CTagsResultParser::file_tags_map& fileTags = fileItr->second.tagsMap;
            CTagsResultParser::file_tags_map::const_iterator itr = findTagByLine(fileTags, line);
            if ( itr == fileTags.end() )
            {
                if ( itr != fileTags.begin() )
                    --itr;
                else
                    return;
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

int CTagsDlg::addListViewItem(int nItem, const tTagData& tagData)
{
    LVITEM lvi;
    tString tagName = tagData.getFullTagName();

    ::ZeroMemory(&lvi, sizeof(lvi));
    lvi.iItem = nItem;
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText = const_cast<TCHAR*>(tagName.c_str());
    lvi.cchTextMax = static_cast<int>(tagName.length());
    lvi.lParam = (LPARAM) tagData.pTagData;
    int nRet = m_lvTags.InsertItem(lvi);

    m_lvTags.SetItemText(nItem, LVC_TYPE, tagData.tagType.c_str());

    TCHAR ts[20];
    wsprintf(ts, _T("%d"), tagData.line);
    m_lvTags.SetItemText(nItem, LVC_LINE, ts);

    m_lvTags.SetItemText(nItem, LVC_FILE, getFileName(tagData.filePath));

    return nRet;
}

HTREEITEM CTagsDlg::addTreeViewItem(HTREEITEM hParent, const tTagData& tagData)
{
    TVINSERTSTRUCT tvis;
    const tString& tagName = tagData.tagName;

    ::ZeroMemory(&tvis, sizeof(tvis));
    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvis.item.pszText = const_cast<TCHAR*>(tagName.c_str());
    tvis.item.cchTextMax = static_cast<int>(tagName.length());
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

void CTagsDlg::OnFileActivated()
{
    m_prevSelStart = -1;
    UpdateNavigationButtons();
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

void CTagsDlg::checkCTagsExePath()
{
    if ( !isFileExist(m_ctagsExeFilePath.c_str()) )
    {
        this->PostMessage(WM_CTAGSPATHFAILED, 0, 0);
    }
}

CTagsResultParser::file_tags_map::iterator CTagsDlg::getTagByLine(CTagsResultParser::file_tags_map& fileTags, const int line)
{
    CTagsResultParser::file_tags_map::iterator itr = fileTags.upper_bound(line); // returns an item with itr->second.line > line

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
            if ( itr2->second.line <= line )
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

CTagsResultParser::file_tags_map::iterator CTagsDlg::findTagByLine(CTagsResultParser::file_tags_map& fileTags, const int line)
{
    CTagsResultParser::file_tags_map::iterator itr = getTagByLine(fileTags, line);

    if ( itr != fileTags.end() )
    {
        for ( auto itr2 = itr; ; --itr2 )
        {
            if ( itr2->second.end_line >= line )
            {
                itr = itr2;
                break;
            }

            if ( itr2 == fileTags.begin() )
                break;

            if ( itr2->second.tagScope.empty() )
                break;
        }
    }

    return itr;
}

CTagsResultParser::file_tags_map::iterator CTagsDlg::getTagByName(CTagsResultParser::file_tags_map& fileTags, const tString& tagName)
{
    CTagsResultParser::file_tags_map::iterator itr = fileTags.begin();
    while ( itr != fileTags.end() )
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
    const TCHAR cszCtags[]    = _T("Ctags");
    const TCHAR cszDebug[]    = _T("Debug");

    m_opt.ReserveMemory(OPT_COUNT);

    // View section
    m_opt.AddInt( OPT_VIEW_MODE,          cszView,  _T("Mode"),         TVM_LIST );
    m_opt.AddInt( OPT_VIEW_SORT,          cszView,  _T("Sort"),         TSM_LINE );
    m_opt.AddInt( OPT_VIEW_WIDTH,         cszView,  _T("Width"),        220      );
    m_opt.AddInt( OPT_VIEW_NAMEWIDTH,     cszView,  _T("NameWidth"),    220      );
    m_opt.AddBool( OPT_VIEW_SHOWTOOLTIPS, cszView,  _T("ShowTooltips"), true     );

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
    if ( m_tagFilter.empty() )
        return true;

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

bool CTagsDlg::isTagFilePathSame(const tString& filePath1, const tString& filePath2)
{
    return ( filePath1.empty() || filePath2.empty() || lstrcmpi(filePath1.c_str(), filePath2.c_str()) == 0 );
}

void CTagsDlg::sortTagsByLine()
{
    int nItem = 0;
    for ( auto& fileItem : m_tags )
    {
        CTagsResultParser::file_tags_vec& fileTags = fileItem.second.tagsVec;
        for ( tTagData* tagPtr : fileTags )
        {
            if ( isTagMatchFilter(tagPtr->getFullTagName()) )
            {
                tagPtr->data.i = addListViewItem(nItem++, *tagPtr);
            }
            else
            {
                tagPtr->data.i = -1;
            }
        }
    }
}

void CTagsDlg::sortTagsByName()
{
    std::list<tTagData*> tagsList;

    for ( auto& fileItem : m_tags )
    {
        CTagsResultParser::file_tags_vec& fileTags = fileItem.second.tagsVec;
        for ( tTagData* tagPtr : fileTags )
        {
            if ( isTagMatchFilter(tagPtr->getFullTagName()) )
            {
                tagsList.push_back(tagPtr);
            }
            else
            {
                tagPtr->data.i = -1;
            }
        }
    }

    tagsList.sort( [](tTagData* tagPtr1, tTagData* tagPtr2) { return (tagPtr1->getFullTagName() < tagPtr2->getFullTagName()); } );

    int nItem = 0;
    for ( tTagData* tagPtr : tagsList )
    {
        int n = addListViewItem(nItem++, *tagPtr);
        tagPtr->data.i = n;
    }
}

void CTagsDlg::sortTagsByType()
{
    std::list<tTagData*> tagsList;

    for ( auto& fileItem : m_tags )
    {
        CTagsResultParser::file_tags_vec& fileTags = fileItem.second.tagsVec;
        for ( tTagData* tagPtr : fileTags )
        {
            if ( isTagMatchFilter(tagPtr->getFullTagName()) )
            {
                tagsList.push_back(tagPtr);
            }
            else
            {
                if ( m_viewMode == TVM_TREE )
                    tagPtr->data.p = nullptr;
                else
                    tagPtr->data.i = -1;
            }
        }
    }

    if ( m_sortMode == TSM_LINE )
    {
        tagsList.sort( [](tTagData* tagPtr1, tTagData* tagPtr2) { return (lstrcmpi(tagPtr1->filePath.c_str(), tagPtr2->filePath.c_str()) <= 0 && tagPtr1->line < tagPtr2->line); } );
    }
    else if ( m_sortMode == TSM_NAME )
    {
        tagsList.sort( [](tTagData* tagPtr1, tTagData* tagPtr2) { return (lstrcmpi(tagPtr1->filePath.c_str(), tagPtr2->filePath.c_str()) <= 0 && tagPtr1->getFullTagName() < tagPtr2->getFullTagName()); } );
    }
    else // TSM_TYPE
    {
        tagsList.sort( [](tTagData* tagPtr1, tTagData* tagPtr2) { return (tagPtr1->tagType < tagPtr2->tagType); } );
    }

    struct sScopeItem
    {
        HTREEITEM hFileItem;
        std::map<tString, HTREEITEM> scopeMap;
    };

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

    auto createTagData = [](const tString& tagName, int line) -> tTagData
    {
        tTagData tagData;

        tagData.tagName = tagName;
        tagData.line = line;
        tagData.end_line = line;
        tagData.data.p = nullptr;
        tagData.pTagData = nullptr;

        return tagData;
    };

    std::map<tString, sScopeItem> fileScopeMap;
    int nItem = 0;

    for ( tTagData* tagPtr : tagsList )
    {
        if ( m_viewMode == TVM_TREE )
        {
            bool bAddFileRootNode = true;

            HTREEITEM hFileItem = TVI_ROOT;
            std::map<tString, sScopeItem>::iterator itrFileScope;

            if ( bAddFileRootNode)
            {
                itrFileScope = fileScopeMap.find(tagPtr->filePath);
                if ( itrFileScope == fileScopeMap.end() )
                {
                    // adding a root file item
                    hFileItem = addTreeViewItem( TVI_ROOT, createTagData(getFileName(tagPtr->filePath), -3) );
                    itrFileScope = fileScopeMap.insert( std::make_pair(tagPtr->filePath, sScopeItem()) ).first;
                    itrFileScope->second.hFileItem = hFileItem;

                    setNodeItemExpanded(m_tvTags, hFileItem);
                }
                else
                    hFileItem = itrFileScope->second.hFileItem;
            }
            else
            {
                if ( fileScopeMap.empty() )
                {
                    itrFileScope = fileScopeMap.insert( std::make_pair(tString(), sScopeItem()) ).first;
                    itrFileScope->second.hFileItem = TVI_ROOT;
                }
                else
                    itrFileScope = fileScopeMap.begin();
            }

            std::map<tString, HTREEITEM>& scopeMap = itrFileScope->second.scopeMap;
            auto scopeIt = scopeMap.find(!tagPtr->tagScope.empty() ? tagPtr->tagScope : tagPtr->tagName);
            if ( scopeIt != scopeMap.end() && !tagPtr->tagScope.empty() )
            {
                HTREEITEM hScopeItem = scopeIt->second;

                if ( !tagPtr->tagScope.empty() )
                {
                    HTREEITEM hItem = addTreeViewItem(hScopeItem, *tagPtr);
                    tagPtr->data.p = (void *) hItem;
                }
            }
            else
            {
                HTREEITEM hScopeItem = hFileItem;

                if ( !tagPtr->tagScope.empty() )
                {
                    // adding a scope item
                    hScopeItem = addTreeViewItem( hFileItem, createTagData(tagPtr->tagScope, -2) );
                    scopeMap[tagPtr->tagScope] = hScopeItem;

                    if ( !m_tagFilter.empty() )
                        setNodeItemExpanded(m_tvTags, hScopeItem);
                }

                HTREEITEM hItem = addTreeViewItem(hScopeItem, *tagPtr);
                if ( tagPtr->tagScope.empty() )
                {
                    // a scope item has been added
                    scopeMap[tagPtr->tagName] = hItem;

                    if ( !m_tagFilter.empty() )
                        setNodeItemExpanded(m_tvTags, hItem);
                }

                tagPtr->data.p = (void *) hItem;
            }
        }
        else
        {
            int n = addListViewItem(nItem++, *tagPtr);
            tagPtr->data.i = n;
        }
    }
}
