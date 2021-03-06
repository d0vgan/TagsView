#include "TagsDlg.h"
#include "TagsCommon.h"
#include "SettingsDlg.h"
#include "EditorWrapper.h"
#include "ConsoleOutputRedirector.h"
#include "resource.h"
#include <memory>
#include <algorithm>

#ifdef _AKEL_TAGSVIEW
#include "../AkelTags/AkelDLL.h"
#endif

using namespace TagsCommon;

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

        if ( !tt->pDlg->GetOptions().getBool(CTagsDlgData::OPT_CTAGS_OUTPUTSTDOUT) )
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
}

const TCHAR* CTagsDlg::cszListViewColumnNames[LVC_TOTAL] = {
    _T("Name"),
    _T("Type"),
    _T("Line"),
    _T("File")
};

CTagsDlg::CTagsDlg() : CDialog(IDD_MAIN)
, m_viewMode(CTagsDlgData::TVM_NONE)
, m_sortMode(CTagsDlgData::TSM_NONE)
, m_dwLastTagsThreadID(0)
, m_pEdWr(NULL)
, m_hMenu(NULL)
, m_crTextColor(0xFFFFFFFF)
, m_crBkgndColor(0xFFFFFFFF)
, m_hBkgndBrush(NULL)
, m_prevSelStart(-1)
, m_isUpdatingSelToItem(false)
, m_nThreadMsg(0)
, m_nTagsThreadCount(0)
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
    if ( m_hMenu != NULL )
    {
        ::DestroyMenu(m_hMenu);
    }

    if ( GetOptions().getInt(CTagsDlgData::OPT_DEBUG_DELETETEMPINPUTFILE) != CTagsDlgData::DTF_NEVERDELETE )
    {
        DeleteTempFile(m_ctagsTempInputFilePath);
    }
    if ( GetOptions().getInt(CTagsDlgData::OPT_DEBUG_DELETETEMPOUTPUTFILE) != CTagsDlgData::DTF_NEVERDELETE )
    {
        DeleteTempFile(m_ctagsTempOutputFilePath);
    }
}

void CTagsDlg::DeleteTempFile(const t_string& filePath)
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
    std::unique_ptr<tCTagsThreadParam> tt(reinterpret_cast<tCTagsThreadParam *>(lpParam));
    CTagsDlg* pDlg = tt->pDlg;
    bool bTagsAdded = false;

    if ( pDlg && pDlg->GetHwnd() )
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

        COptionsManager& opt = pDlg->GetOptions();
        if ( opt.getInt(CTagsDlgData::OPT_DEBUG_DELETETEMPINPUTFILE) == CTagsDlgData::DTF_ALWAYSDELETE )
        {
            DeleteTempFile(tt->temp_input_file);
        }
        if ( opt.getInt(CTagsDlgData::OPT_DEBUG_DELETETEMPOUTPUTFILE) == CTagsDlgData::DTF_ALWAYSDELETE )
        {
            DeleteTempFile(tt->temp_output_file);
        }

        ::WaitForSingleObject(pDlg->m_hTagsThreadEvent, 4000);
        if ( tt->dwThreadID == pDlg->m_dwLastTagsThreadID )
        {
            if ( pDlg->GetHwnd() /*&& pDlg->IsWindowVisible()*/ )
            {
                const CEditorWrapper* pEdWr = pDlg->m_pEdWr;
                if ( pEdWr && (pEdWr->ewGetFilePathName() == tt->source_file_name) )
                {
                    if ( tt->temp_output_file.empty() )
                        pDlg->OnAddTags( cor.GetOutputString().c_str(), tt.get() );
                    else
                        pDlg->OnAddTags( pTempTags.get(), tt.get() );

                    bTagsAdded = true;
                }
            }
        }
    }
    else
    {
        if ( !pDlg || pDlg->GetOptions().getInt(CTagsDlgData::OPT_DEBUG_DELETETEMPINPUTFILE) != CTagsDlgData::DTF_NEVERDELETE )
        {
            DeleteTempFile(tt->temp_input_file);
        }
        if ( !pDlg || pDlg->GetOptions().getInt(CTagsDlgData::OPT_DEBUG_DELETETEMPOUTPUTFILE) != CTagsDlgData::DTF_NEVERDELETE )
        {
            DeleteTempFile(tt->temp_output_file);
        }
    }

    if ( !bTagsAdded )
    {
        if ( !pDlg || pDlg->GetOptions().getInt(CTagsDlgData::OPT_DEBUG_DELETETEMPOUTPUTFILE) != CTagsDlgData::DTF_NEVERDELETE )
        {
            DeleteTempFile(tt->temp_output_file);
        }
        if ( pDlg && pDlg->m_tbButtons.IsWindow() )
        {
            pDlg->m_tbButtons.EnableButton(IDM_PARSE);
        }
    }

    if ( pDlg )
    {
        pDlg->removeCTagsThreadForFile(tt->source_file_name);
        ::InterlockedDecrement(&pDlg->m_nTagsThreadCount);
    }

    return 0;
}

INT_PTR CTagsDlg::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR nResult = 0;
    bool hasResult = false;

    switch ( uMsg )
    {
        case WM_CLEARCACHEDTAGS:
            ClearCachedTags();
            return 0;

        case WM_PURIFYCACHEDTAGS:
            PurifyCachedTags();
            return 0;

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
            if ( GetOptions().getInt(CTagsDlgData::OPT_DEBUG_DELETETEMPINPUTFILE) != CTagsDlgData::DTF_NEVERDELETE )
            {
                DeleteTempFile(m_ctagsTempInputFilePath);
            }
            if ( GetOptions().getInt(CTagsDlgData::OPT_DEBUG_DELETETEMPOUTPUTFILE) != CTagsDlgData::DTF_NEVERDELETE )
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
                t_string err = _T("File \'ctags.exe\' was not found. The path is incorrect:\n");
                err += m_ctagsExeFilePath;
                ::MessageBox( m_pEdWr->ewGetMainHwnd(), err.c_str(), _T("TagsView Error"), MB_OK | MB_ICONERROR );
            }
            return 0;

        case WM_CTLCOLOREDIT:
            if ( GetOptions().getBool(CTagsDlgData::OPT_COLORS_USEEDITORCOLORS) )
            {
                nResult = OnCtlColorEdit(wParam, lParam);
                hasResult = true;
            }
            break;

        case WM_SIZE:
            OnSize();
            break;

        case WM_SHOWWINDOW:
            if ( wParam ) // Showing the window
            {
                nResult = DialogProcDefault(uMsg, wParam, lParam);
                hasResult = true;
                if ( m_pEdWr )
                {
                    t_string filePath = m_pEdWr->ewGetFilePathName();
                    ParseFile( filePath.c_str(), false );
                }
            }
            break;

        case WM_INITDIALOG:
            nResult = OnInitDialog();
            hasResult = true;
            break;
    }

    onMessage(uMsg, wParam, lParam);

    if ( hasResult )
        return nResult;

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

    tags_map tags;

    CTagsResultParser::Parse(
        s,
        nParseFlags,
        CTagsResultParser::tParseContext(
            tags,
            getFileDirectory(tt->source_file_name),
            tt->temp_input_file.empty() ? t_string() : tt->source_file_name
        ) 
    );

    if ( tags.find(tt->source_file_name) == tags.end() )
    {
        // No tags found in the source_file_name itself.
        // Anyway, let's have this file in the cache to avoid parsing it again and again.
        tags[tt->source_file_name] = file_tags();
    }

    m_Data.AddTagsToCache(std::move(tags));

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
            SetSortMode(CTagsDlgData::TSM_LINE);
            break;

        case IDM_SORTNAME:
            SetSortMode(CTagsDlgData::TSM_NAME);
            break;

        case IDM_VIEWLIST:
            SetViewMode(CTagsDlgData::TVM_LIST, m_sortMode);
            break;

        case IDM_VIEWTREE:
            SetViewMode(CTagsDlgData::TVM_TREE, m_sortMode);
            break;

        case IDM_PARSE:
            OnParseClicked();
            break;

        case IDM_CLOSE:
        case IDC_BT_CLOSE:
            this->PostMessage(WM_CLOSETAGSVIEW);
            break;

        case IDM_TREE_COPYITEMTOCLIPBOARD:
            OnTreeCopyItemToClipboard();
            break;

        case IDM_TREE_COPYITEMANDCHILDRENTOCLIPBOARD:
            OnTreeCopyItemAndChildrenToClipboard();
            break;

        case IDM_TREE_COPYALLITEMSTOCLIPBOARD:
            OnTreeCopyAllItemsToClipboard();
            break;

        case IDM_TREE_COLLAPSECHILDNODES:
            OnTreeCollapseChildNodes();
            break;

        case IDM_TREE_EXPANDCHILDNODES:
            OnTreeExpandChildNodes();
            break;

        case IDM_TREE_COLLAPSEPARENTNODE:
            OnTreeCollapseParentNode();
            break;

        case IDM_TREE_COLLAPSEALLNODES:
            OnTreeCollapseAllNodes();
            break;

        case IDM_TREE_EXPANDALLNODES:
            OnTreeExpandAllNodes();
            break;

        case IDM_LIST_COPYITEMTOCLIPBOARD:
            OnListCopyItemToClipboard();
            break;

        case IDM_LIST_COPYALLITEMSTOCLIPBOARD:
            OnListCopyAllItemsToClipboard();
            break;

        case IDM_SETTINGS:
            OnShowSettings();
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
        if ( GetOptions().getBool(CTagsDlgData::OPT_VIEW_ESCFOCUSTOEDITOR) )
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

    m_edFilter.SetTagsDlg(this);
    m_lvTags.SetTagsDlg(this);
    m_tvTags.SetTagsDlg(this);

#ifdef _AKEL_TAGSVIEW
    m_stTitle.Attach( ::GetDlgItem(GetHwnd(), IDC_ST_TITLE) );
    m_btClose.Attach( ::GetDlgItem(GetHwnd(), IDC_BT_CLOSE) );

    BUTTONDRAW bd;
    ::ZeroMemory(&bd, sizeof(BUTTONDRAW));
    bd.dwFlags = BIF_CROSS | BIF_ETCHED;
    SendMessage( m_pEdWr->ewGetMainHwnd(), AKD_SETBUTTONDRAW, (WPARAM) m_btClose.GetHwnd(), (LPARAM) &bd );
#endif

    m_edFilter.Attach( ::GetDlgItem(GetHwnd(), IDC_ED_FILTER) );
    m_lvTags.Attach( ::GetDlgItem(GetHwnd(), IDC_LV_TAGS) );
    m_tvTags.Attach( ::GetDlgItem(GetHwnd(), IDC_TV_TAGS) );

    int y = 1;
#ifdef _AKEL_TAGSVIEW
    y += (m_stTitle.GetWindowRect().Height() + 2);
#endif

    DWORD dwTbStyle = /*WS_TABSTOP | */ WS_VISIBLE | WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS;
#ifdef _AKEL_TAGSVIEW
    dwTbStyle |= (CCS_NOPARENTALIGN | CCS_NOMOVEY);
#endif

    m_tbButtons.CreateEx(
        0, //WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE,
        TOOLBARCLASSNAME, _T(""),
        dwTbStyle,
        1, y, 0, 0, GetHwnd(), NULL);

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
    m_lvTags.InsertColumn( LVC_NAME, cszListViewColumnNames[LVC_NAME], LVCFMT_LEFT, GetOptions().getInt(CTagsDlgData::OPT_VIEW_NAMEWIDTH) );
    m_lvTags.InsertColumn( LVC_TYPE, cszListViewColumnNames[LVC_TYPE], LVCFMT_RIGHT, 60 );
    m_lvTags.InsertColumn( LVC_LINE, cszListViewColumnNames[LVC_LINE], LVCFMT_RIGHT, 60 );
    m_lvTags.InsertColumn( LVC_FILE, cszListViewColumnNames[LVC_FILE], LVCFMT_LEFT, 100 );

    //m_tbButtons.SetButtonState(IDM_PREVPOS, 0);
    //m_tbButtons.SetButtonState(IDM_NEXTPOS, 0);

    ApplyColors();

    if ( GetOptions().getBool(CTagsDlgData::OPT_VIEW_SHOWTOOLTIPS) )
    {
        createTooltips();
    }

    CTagsDlgData::eTagsViewMode viewMode = toViewMode( GetOptions().getInt(CTagsDlgData::OPT_VIEW_MODE) );
    CTagsDlgData::eTagsSortMode sortMode = toSortMode( GetOptions().getInt(CTagsDlgData::OPT_VIEW_SORT) );
    SetViewMode(viewMode, sortMode);

    OnSize(true);

    checkCTagsExePath();

    m_hMenu = ::LoadMenu( GetApp()->GetResourceHandle(), MAKEINTRESOURCE(IDR_TAGSVIEW_MENU) );

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
            SetSortMode( toSortMode(CTagsDlgData::TSM_NAME + ((LPNMLISTVIEW) lParam)->iSubItem) );
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
        ::MoveWindow( GetHwnd(), 0, 0, GetOptions().getInt(CTagsDlgData::OPT_VIEW_WIDTH), r.Height(), TRUE );
        return;
    }

    CRect r = GetClientRect();
    const int width = r.Width();
    const int height = r.Height();
    const int left = 2;
    int top = 1;

#ifdef _AKEL_TAGSVIEW
    // AkelTags: "x" button
    r = m_btClose.GetWindowRect();
    ::MoveWindow( m_btClose, width - r.Width() - left, top, r.Width(), r.Height(), TRUE );

    // AkelTags: static title
    CRect rcTitle = m_stTitle.GetWindowRect();
    ::MoveWindow( m_stTitle, left, top, width - r.Width() - 3*left, rcTitle.Height(), TRUE );
    top += (rcTitle.Height() + 2);
#endif

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
    ::MoveWindow( m_lvTags, left, top, width - 2*left, height - top - 2, (m_viewMode == CTagsDlgData::TVM_LIST) );
    if ( GetOptions().getBool(CTagsDlgData::OPT_VIEW_SHOWTOOLTIPS) )
    {
        if ( m_ttHints.GetHwnd() && m_ttHints.IsWindow() )
            m_ttHints.SetToolRect(m_lvTags.GetClientRect(), m_lvTags, IDC_LV_TAGS);
    }

    // tree-view window
    r = m_tvTags.GetWindowRect();
    ::MoveWindow( m_tvTags, left, top, width - 2*left, height - top - 2, (m_viewMode == CTagsDlgData::TVM_TREE) );
    if ( GetOptions().getBool(CTagsDlgData::OPT_VIEW_SHOWTOOLTIPS) )
    {
        if ( m_ttHints.GetHwnd() && m_ttHints.IsWindow() )
            m_ttHints.SetToolRect(m_tvTags.GetClientRect(), m_tvTags, IDC_TV_TAGS);
    }


    // saving options...

    r = GetWindowRect();
    if ( r.Width() > 0 )
    {
        GetOptions().setInt( CTagsDlgData::OPT_VIEW_WIDTH, r.Width() );
    }

    int iNameWidth = m_lvTags.GetColumnWidth(0);
    if ( iNameWidth > 0 )
    {
        GetOptions().setInt( CTagsDlgData::OPT_VIEW_NAMEWIDTH, iNameWidth );
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

bool CTagsDlg::GoToTag(const t_string& filePath, const TCHAR* cszTagName, const TCHAR* cszTagScope)  // not implemented yet
{
    if ( cszTagName && cszTagName[0] && !m_Data.IsTagsEmpty() )
    {
        tTagData* pTag = m_Data.FindTagByNameAndScope(filePath, cszTagName, cszTagScope);
        if ( pTag )
        {
        }
    }

    return false;
}

void CTagsDlg::ParseFile(const TCHAR* const cszFileName, bool bReparsePhysicalFile)
{
    if ( !cszFileName )
    {
        ClearItems(true);
        m_Data.SetTags(nullptr);
        return;
    }

    const tags_map* prev_tags = m_Data.GetTags();
    const tags_map* curr_tags = m_Data.GetTagsFromCache(cszFileName);

    if ( curr_tags && !bReparsePhysicalFile )
    {
        if ( curr_tags != prev_tags )
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
        const TCHAR* pszSkipFileExts = GetOptions().getStr(CTagsDlgData::OPT_CTAGS_SKIPFILEEXTS, &nLen);
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

    if ( !addCTagsThreadForFile(cszFileName) )
        return;

    if ( curr_tags )
    {
        m_Data.RemoveTagsFromCache(cszFileName);
    }

    t_string ctagsOptPath = m_ctagsExeFilePath;
    if ( ctagsOptPath.length() > 3 )
    {
        t_string::size_type len = ctagsOptPath.length();
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
        if ( GetOptions().getInt(CTagsDlgData::OPT_DEBUG_DELETETEMPINPUTFILE) != CTagsDlgData::DTF_NEVERDELETE )
        {
            DeleteTempFile(m_ctagsTempInputFilePath);
        }
        m_ctagsTempInputFilePath = tt->temp_input_file;
    }

    if ( tt->temp_output_file != m_ctagsTempOutputFilePath )
    {
        if ( GetOptions().getInt(CTagsDlgData::OPT_DEBUG_DELETETEMPOUTPUTFILE) != CTagsDlgData::DTF_NEVERDELETE )
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
    tt->cmd_line += _T('\"');
    tt->cmd_line += m_ctagsExeFilePath;
    if ( !ctagsOptPath.empty() )
    {
        tt->cmd_line += _T("\" --options=\"");
        tt->cmd_line += ctagsOptPath;
    }
    tt->cmd_line += _T("\" -f ");
    if ( tt->temp_output_file.empty() )
    {
        tt->cmd_line += _T('-');
    }
    else
    {
        tt->cmd_line += _T('\"');
        tt->cmd_line += tt->temp_output_file;
        tt->cmd_line += _T('\"');
    }
    tt->cmd_line += _T(" --fields=fKnste ");
    if ( tt->temp_input_file.empty() )
    {
        if ( !GetOptions().getBool(CTagsDlgData::OPT_CTAGS_SCANFOLDER) )
        {
            tt->cmd_line += _T('\"');
            tt->cmd_line += tt->source_file_name;
            tt->cmd_line += _T('\"');

            std::list<t_string> relatedFiles = getRelatedSourceFiles(tt->source_file_name);
            if ( !relatedFiles.empty() )
            {
                for ( auto& relatedFile : relatedFiles )
                {
                    tt->cmd_line += _T(" \"");
                    tt->cmd_line += relatedFile;
                    tt->cmd_line += _T('\"');
                }
            }
        }
        else
        {
            if ( GetOptions().getBool(CTagsDlgData::OPT_CTAGS_SCANFOLDERRECURSIVELY) )
            {
                // Note: the recursive scanning may generate a tag file of several MB that takes toooooo long to visualize
                // TODO: consider virtual TreeView & ListView
                //tt->cmd_line += _T("--recurse=yes ");
            }

            t_string ctagsLang = getCtagsLangFamily(tt->source_file_name);
            if ( !ctagsLang.empty() )
            {
                tt->cmd_line += _T("--languages=\"");
                tt->cmd_line += ctagsLang;
                tt->cmd_line += _T("\" ");
            }

            size_t n = tt->source_file_name.find_last_of(_T("\\/"));
            t_string source_dir(tt->source_file_name.c_str(), n + 2); // ends with "\X" or "/X"
            source_dir.back() = _T('*'); // now ends with "\*" or "/*"

            tt->cmd_line += _T('\"');
            tt->cmd_line += source_dir;
            tt->cmd_line += _T('\"');
        }
    }
    else
    {
        tt->cmd_line += _T('\"');
        tt->cmd_line += tt->temp_input_file;
        tt->cmd_line += _T('\"');
    }

    ::ResetEvent(m_hTagsThreadEvent);
    HANDLE hThread = ::CreateThread(NULL, 0, CTagsThreadProc, tt, 0, &tt->dwThreadID);
    if ( hThread )
    {
        ::InterlockedIncrement(&m_nTagsThreadCount);
        m_dwLastTagsThreadID = tt->dwThreadID;
        ::CloseHandle(hThread);
        if ( m_tbButtons.IsWindow() )
        {
            m_tbButtons.DisableButton(IDM_PARSE);
        }
    }
    else
    {
        removeCTagsThreadForFile(tt->source_file_name);
        delete tt;
    }
    ::SetEvent(m_hTagsThreadEvent);
}

void CTagsDlg::ReparseCurrentFile()
{
    OnParseClicked();
}

void CTagsDlg::SetSortMode(CTagsDlgData::eTagsSortMode sortMode)
{
    if ( sortMode == CTagsDlgData::TSM_FILE )
    {
        sortMode = CTagsDlgData::TSM_LINE;
    }

    if ( (sortMode != CTagsDlgData::TSM_NONE) && (sortMode != m_sortMode) )
    {
        GetOptions().setInt( CTagsDlgData::OPT_VIEW_SORT, (int) sortMode );

        m_prevSelStart = -1;
        deleteAllItems(true);

        if ( m_viewMode == CTagsDlgData::TVM_TREE )
        {
            m_tvTags.SetRedraw(FALSE);

            switch ( sortMode )
            {
                case CTagsDlgData::TSM_LINE:
                case CTagsDlgData::TSM_TYPE:
                    sortTagsTV(CTagsDlgData::TSM_LINE);
                    m_tbButtons.SetButtonState(IDM_SORTNAME, TBSTATE_ENABLED);
                    m_tbButtons.SetButtonState(IDM_SORTLINE, TBSTATE_ENABLED | TBSTATE_CHECKED);
                    break;

                case CTagsDlgData::TSM_NAME:
                    sortTagsTV(CTagsDlgData::TSM_NAME);
                    m_tbButtons.SetButtonState(IDM_SORTLINE, TBSTATE_ENABLED);
                    m_tbButtons.SetButtonState(IDM_SORTNAME, TBSTATE_ENABLED | TBSTATE_CHECKED);
                    break;
            }

            m_tvTags.SetRedraw(TRUE);
            m_tvTags.InvalidateRect(TRUE);

            UpdateCurrentItem();
        }
        else
        {
            m_lvTags.SetRedraw(FALSE);

            switch ( sortMode )
            {
                case CTagsDlgData::TSM_LINE:
                    sortTagsByLineLV();
                    m_tbButtons.SetButtonState(IDM_SORTNAME, TBSTATE_ENABLED);
                    m_tbButtons.SetButtonState(IDM_SORTLINE, TBSTATE_ENABLED | TBSTATE_CHECKED);
                    break;

                case CTagsDlgData::TSM_TYPE:
                    sortTagsByNameOrTypeLV(CTagsDlgData::TSM_TYPE);
                    break;

                case CTagsDlgData::TSM_NAME:
                    sortTagsByNameOrTypeLV(CTagsDlgData::TSM_NAME);
                    m_tbButtons.SetButtonState(IDM_SORTLINE, TBSTATE_ENABLED);
                    m_tbButtons.SetButtonState(IDM_SORTNAME, TBSTATE_ENABLED | TBSTATE_CHECKED);
                    break;
            }

            m_lvTags.SetRedraw(TRUE);
            m_lvTags.InvalidateRect(TRUE);

            UpdateCurrentItem();
        }

        m_sortMode = sortMode;
    }
}

void CTagsDlg::SetViewMode(CTagsDlgData::eTagsViewMode viewMode, CTagsDlgData::eTagsSortMode sortMode)
{
    if ( (viewMode != CTagsDlgData::TVM_NONE) && (viewMode != m_viewMode) )
    {
        GetOptions().setInt( CTagsDlgData::OPT_VIEW_MODE, (int) viewMode );

        m_viewMode = viewMode;
        m_sortMode = CTagsDlgData::TSM_NONE;
        m_lvTags.ShowWindow(SW_HIDE);
        m_tvTags.ShowWindow(SW_HIDE);
    }

    SetSortMode(sortMode);

    switch ( m_viewMode )
    {
        case CTagsDlgData::TVM_LIST:
            if ( !m_lvTags.IsWindowVisible() )
                m_lvTags.ShowWindow(SW_SHOWNORMAL);
            m_tbButtons.SetButtonState(IDM_VIEWTREE, TBSTATE_ENABLED);
            m_tbButtons.SetButtonState(IDM_VIEWLIST, TBSTATE_ENABLED | TBSTATE_CHECKED);
            break;

        case CTagsDlgData::TVM_TREE:
            if ( !m_tvTags.IsWindowVisible() )
                m_tvTags.ShowWindow(SW_SHOWNORMAL);
            m_tbButtons.SetButtonState(IDM_VIEWLIST, TBSTATE_ENABLED);
            m_tbButtons.SetButtonState(IDM_VIEWTREE, TBSTATE_ENABLED | TBSTATE_CHECKED);
            break;
    }
}

void CTagsDlg::UpdateCurrentItem()
{
    if ( m_pEdWr && !m_Data.IsTagsEmpty() && !m_isUpdatingSelToItem )
    {
        int selEnd, selStart = m_pEdWr->ewGetSelectionPos(selEnd);

        if ( m_prevSelStart != selStart )
        {
            m_prevSelStart = selStart;

            int line = m_pEdWr->ewGetLineFromPos(selStart) + 1; // 1-based line

            const t_string filePath = m_pEdWr->ewGetFilePathName();
            file_tags* pFileTags = m_Data.GetFileTags(filePath);
            if ( !pFileTags )
                return;

            const tTagData* pTag = m_Data.FindTagByLine(*pFileTags, line);
            if ( !pTag )
                return;

            if ( m_viewMode == CTagsDlgData::TVM_TREE )
            {
                HTREEITEM hItem = (HTREEITEM) pTag->data.p;
                if ( hItem )
                {
                    CThreadLock lock(m_csTagsItemsUI);

                    if ( m_tvTags.GetCount() > 0 )
                    {
                        m_tvTags.SelectItem(hItem);
                        m_tvTags.EnsureVisible(hItem);
                    }
                }
            }
            else
            {
                int iItem = pTag->data.i;
                if ( iItem >= 0 )
                {
                    CThreadLock lock(m_csTagsItemsUI);

                    if ( m_lvTags.GetItemCount() > 0 )
                    {
                        int iSelItem = m_lvTags.GetSelectionMark();
                        if ( iSelItem != -1 )
                        {
                            m_lvTags.SetItemState(iSelItem, 0, LVIS_FOCUSED | LVIS_SELECTED);
                        }
                        m_lvTags.SetItemState(iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                        m_lvTags.SetSelectionMark(iItem);
                        m_lvTags.EnsureVisible(iItem, FALSE);
                    }
                }
            }
        }
    }
}

void CTagsDlg::UpdateTagsView()
{
    if ( m_Data.GetTags() && m_Data.GetTags()->empty() )
    {
        m_Data.GetTags()->insert( std::make_pair(m_pEdWr->ewGetFilePathName(), file_tags()) );
    }

    m_prevSelStart = -1;

    CTagsDlgData::eTagsViewMode viewMode = m_viewMode;
    CTagsDlgData::eTagsSortMode sortMode = m_sortMode;
    //disabled to avoid the background blink: m_viewMode = TVM_NONE;
    m_sortMode = CTagsDlgData::TSM_NONE;
    SetViewMode(viewMode, sortMode);
    UpdateNavigationButtons();
    if ( m_tbButtons.IsWindow() )
    {
        m_tbButtons.EnableButton(IDM_PARSE);
    }
}

void CTagsDlg::UpdateNavigationButtons()
{
    if ( m_pEdWr )
    {
        if ( m_tbButtons.IsWindow() )
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
    CThreadLock lock(m_csTagsItemsUI);

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
    t_string tagName = pTag->getFullTagName();

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

    m_lvTags.SetItemText(nItem, LVC_FILE, getFileName(pTag));

    return nRet;
}

HTREEITEM CTagsDlg::addTreeViewItem(HTREEITEM hParent, const t_string& tagName, tTagData* pTag)
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

    if ( GetOptions().getBool(CTagsDlgData::OPT_COLORS_USEEDITORCOLORS) &&
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
    deleteAllItems(bDelayedRedraw);

    if ( m_tbButtons.IsWindow() )
    {
        m_tbButtons.SetButtonState(IDM_PREVPOS, 0);
        m_tbButtons.SetButtonState(IDM_NEXTPOS, 0);
    }

    m_prevSelStart = -1;
}

void CTagsDlg::ClearCachedTags()
{
    {
        CThreadLock lock(m_csCTagsThreads);

        if ( !m_ctagsThreads.empty() )
        {
            m_nThreadMsg = WM_CLEARCACHEDTAGS;
            return;
        }
    }

    if ( !GetHwnd() || !IsWindowVisible() )
    {
        m_Data.RemoveAllTagsFromCache();
    }
}

void CTagsDlg::PurifyCachedTags()
{
    {
        CThreadLock lock(m_csCTagsThreads);

        if ( !m_ctagsThreads.empty() )
        {
            m_nThreadMsg = WM_PURIFYCACHEDTAGS;
            return;
        }
    }

    IEditorWrapper::file_set openedFiles = m_pEdWr->ewGetOpenedFilePaths();
    m_Data.RemoveOutdatedTagsFromCache(openedFiles);

    if ( openedFiles.empty() )
    {
        ClearItems(true);
    }
}

void CTagsDlg::OnTreeCopyItemToClipboard()
{
    HTREEITEM hItem = m_tvTags.GetSelection();
    setClipboardText( getItemTextTV(hItem), m_pEdWr->ewGetMainHwnd() );
}

void CTagsDlg::OnTreeCopyItemAndChildrenToClipboard()
{
    HTREEITEM hItem = m_tvTags.GetSelection();
    setClipboardText( getItemAndChildrenTextTV(hItem), m_pEdWr->ewGetMainHwnd() );
}

void CTagsDlg::OnTreeCopyAllItemsToClipboard()
{
    setClipboardText( getAllItemsTextTV(), m_pEdWr->ewGetMainHwnd() );
}

void CTagsDlg::OnTreeExpandChildNodes()
{
    HTREEITEM hItem = m_tvTags.GetSelection();
    if ( hItem )
    {
        m_tvTags.SetRedraw(FALSE);

        applyActionToItemTV(hItem, TNA_EXPANDCHILDREN);

        m_tvTags.SetRedraw(TRUE);
        m_tvTags.InvalidateRect(TRUE);

        m_tvTags.EnsureVisible(hItem);
    }
}

void CTagsDlg::OnTreeCollapseChildNodes()
{
    HTREEITEM hItem = m_tvTags.GetSelection();
    if ( hItem )
    {
        m_tvTags.SetRedraw(FALSE);

        applyActionToItemTV(hItem, TNA_COLLAPSECHILDREN);

        m_tvTags.SetRedraw(TRUE);
        m_tvTags.InvalidateRect(TRUE);
    }
}

void CTagsDlg::OnTreeCollapseParentNode()
{
    HTREEITEM hItem = m_tvTags.GetSelection();
    if ( hItem )
    {
        HTREEITEM hParentItem = m_tvTags.GetParentItem(hItem);
        if ( hParentItem )
        {
            m_tvTags.SetRedraw(FALSE);

            m_tvTags.Expand(hParentItem, TVE_COLLAPSE);

            m_tvTags.SetRedraw(TRUE);
            m_tvTags.InvalidateRect(TRUE);

            m_tvTags.SelectItem(hParentItem);
            m_tvTags.EnsureVisible(hParentItem);
        }
    }
}

void CTagsDlg::OnTreeExpandAllNodes()
{
    HTREEITEM hItem = m_tvTags.GetRootItem();
    if ( hItem )
    {
        m_tvTags.SetRedraw(FALSE);

        do
        {
            applyActionToItemTV(hItem, TNA_EXPANDCHILDREN);
            hItem = m_tvTags.GetNextSibling(hItem);
        }
        while ( hItem != NULL );

        m_tvTags.SetRedraw(TRUE);
        m_tvTags.InvalidateRect(TRUE);

        hItem = m_tvTags.GetSelection();
        if ( hItem )
        {
            m_tvTags.EnsureVisible(hItem);
        }
    }
}

void CTagsDlg::OnTreeCollapseAllNodes()
{
    HTREEITEM hItem = m_tvTags.GetRootItem();
    if ( hItem )
    {
        m_tvTags.SetRedraw(FALSE);

        do
        {
            applyActionToItemTV(hItem, TNA_COLLAPSECHILDREN);
            hItem = m_tvTags.GetNextSibling(hItem);
        }
        while ( hItem != NULL );

        m_tvTags.SetRedraw(TRUE);
        m_tvTags.InvalidateRect(TRUE);

        hItem = m_tvTags.GetSelection();
        if ( hItem )
        {
            m_tvTags.EnsureVisible(hItem);
        }
    }
}

void CTagsDlg::OnListCopyItemToClipboard()
{
    int iItem = m_lvTags.GetSelectionMark();
    setClipboardText( getItemTextLV(iItem), m_pEdWr->ewGetMainHwnd() );
}

void CTagsDlg::OnListCopyAllItemsToClipboard()
{
    setClipboardText( getAllItemsTextLV(), m_pEdWr->ewGetMainHwnd() );
}

void CTagsDlg::OnShowSettings()
{
    CSettingsDlg dlg(GetOptions());
    dlg.DoModal(m_pEdWr->ewGetMainHwnd());

    OnSettingsChanged();
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
            if ( GetOptions().getBool(CTagsDlgData::OPT_VIEW_SHOWTOOLTIPS) )
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

void CTagsDlg::OnTagDblClicked(const tTagData* pTag)
{
    if ( pTag && m_pEdWr )
    {
        if ( pTag->hasFilePath() )
        {
            const t_string& filePath = *pTag->pFilePath;
            t_string currFilePath = m_pEdWr->ewGetFilePathName();
            if ( lstrcmpi(filePath.c_str(), currFilePath.c_str()) != 0 )
            {
                m_isUpdatingSelToItem = true;
                m_pEdWr->ewDoOpenFile(filePath.c_str());
            }
        }

        const int line = pTag->line;
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
        }
        m_isUpdatingSelToItem = false;
    }
}

void CTagsDlg::checkCTagsExePath()
{
    if ( !isFileExist(m_ctagsExeFilePath.c_str()) )
    {
        this->PostMessage(WM_CTAGSPATHFAILED, 0, 0);
    }
}

bool CTagsDlg::addCTagsThreadForFile(const t_string& filePath)
{
    bool bAdded = false;

    CThreadLock lock(m_csCTagsThreads);

    if ( m_ctagsThreads.insert(filePath).second == true )
    {
        bAdded = true;
    }

    return bAdded;
}

void CTagsDlg::removeCTagsThreadForFile(const t_string& filePath)
{
    UINT uMsg = 0;

    {
        CThreadLock lock(m_csCTagsThreads);

        m_ctagsThreads.erase(filePath);
        if ( m_ctagsThreads.empty() && (m_nThreadMsg != 0) )
        {
            uMsg = m_nThreadMsg;
            m_nThreadMsg = 0;
        }
    }

    if ( uMsg != 0 )
    {
        ::PostMessage(GetHwnd(), uMsg, 0, 0);
    }
}

bool CTagsDlg::hasAnyCTagsThread() const
{
    return (::InterlockedCompareExchange(&m_nTagsThreadCount, 0, 0) == 0);
}

bool CTagsDlg::isTagMatchFilter(const t_string& tagName) const
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

    if ( m_Data.GetTags() )
    {
        for ( const auto& fileItem : *m_Data.GetTags() )
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
    if ( !m_Data.GetTags() )
        return;

    int nItem = 0;
    for ( auto& fileItem : *m_Data.GetTags() )
    {
        file_tags& fileTags = fileItem.second;
        for ( std::unique_ptr<tTagData>& pTag : fileTags )
        {
            pTag->data.i = -1;

            if ( m_tagFilter.empty() || isTagMatchFilter(pTag->getFullTagName()) )
            {
                pTag->pFilePath = &fileItem.first;
                pTag->data.i = addListViewItem(nItem++, pTag.get());
            }
            else
            {
                pTag->pFilePath = nullptr;
            }
        }
    }
}

void CTagsDlg::sortTagsByNameOrTypeLV(CTagsDlgData::eTagsSortMode sortMode)
{
    if ( !m_Data.GetTags() )
        return;

    std::vector<tTagData*> tags;
    tags.reserve(getNumTotalItemsForSorting());

    for ( auto& fileItem : *m_Data.GetTags() )
    {
        file_tags& fileTags = fileItem.second;
        for ( std::unique_ptr<tTagData>& pTag : fileTags )
        {
            pTag->data.i = -1;

            if ( m_tagFilter.empty() || isTagMatchFilter(pTag->getFullTagName()) )
            {
                pTag->pFilePath = &fileItem.first;
                tags.push_back(pTag.get());
            }
            else
            {
                pTag->pFilePath = nullptr;
            }
        }
    }

    if ( sortMode == CTagsDlgData::TSM_NAME )
    {
        std::sort(
            tags.begin(),
            tags.end(),
            [](tTagData* pTag1, tTagData* pTag2){ return (pTag1->getFullTagName() < pTag2->getFullTagName()); }
        );
    }
    else // if ( sortMode == TSM_TYPE )
    {
        if ( m_sortMode == CTagsDlgData::TSM_NAME )
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

void CTagsDlg::sortTagsTV(CTagsDlgData::eTagsSortMode sortMode)
{
    if ( !m_Data.GetTags() )
        return;

    std::list<tTagsByFile> tags;

    for ( auto& fileItem : *m_Data.GetTags() )
    {
        file_tags& fileTags = fileItem.second;

        tTagsByFile tagsByFile;
        tagsByFile.filePath = fileItem.first;
        tagsByFile.fileTags.reserve(getNumItemsForSorting(fileTags));

        for ( std::unique_ptr<tTagData>& pTag : fileTags )
        {
            pTag->data.p = nullptr;

            if ( m_tagFilter.empty() || isTagMatchFilter(pTag->getFullTagName()) )
            {
                pTag->pFilePath = &fileItem.first;
                tagsByFile.fileTags.push_back(pTag.get());
            }
            else
            {
                pTag->pFilePath = nullptr;
            }
        }

        if ( !tagsByFile.fileTags.empty() )
        {
            if ( sortMode == CTagsDlgData::TSM_LINE )
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

    for ( tTagsByFile& tagsByFile : tags )
    {
        addFileTagsToTV(tagsByFile);
    }
}

void CTagsDlg::addFileTagsToTV(tTagsByFile& tagsByFile)
{
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

    // adding a root file item
    HTREEITEM hFileItem = addTreeViewItem( TVI_ROOT, getFileName(tagsByFile.fileTags.front()), nullptr );
    setNodeItemExpanded(m_tvTags, hFileItem);

    std::map<t_string, HTREEITEM> scopeMap;

    t_string tagScopeName;
    t_string scopeSep;
    t_string langFamily = getCtagsLangFamily(tagsByFile.filePath);
    if ( langFamily.find(_T("C++")) != t_string::npos )
    {
        scopeSep = _T("::");  // nested scopes in a form of "X::Y::Z"
    }
    else
    {
        scopeSep = _T(".");  // nested scopes in a form of "X.Y.Z"
    }

    if ( GetOptions().getBool(CTagsDlgData::OPT_VIEW_NESTEDSCOPETREE) )
    {
        t_string tagScope;

        for ( tTagData* pTag : tagsByFile.fileTags )
        {
            HTREEITEM hScopeItem = hFileItem;
            t_string::size_type nScopePos = 0;
            bool bAddItem = true;

            do
            {
                if ( !pTag->tagScope.empty() )
                {
                    t_string::size_type nScopeEnd = scopeSep.empty() ? t_string::npos : pTag->tagScope.find(scopeSep, nScopePos);
                    t_string::size_type nEndPos = (nScopeEnd != t_string::npos) ? nScopeEnd : pTag->tagScope.length();
                    tagScope.assign(pTag->tagScope, 0, nEndPos);
                    nScopePos = nScopeEnd;
                    if ( nScopePos != t_string::npos )
                        nScopePos += scopeSep.length();
                }
                else
                {
                    nScopePos = t_string::npos;
                    tagScope.clear();
                }

                auto scopeIt = scopeMap.find(!tagScope.empty() ? tagScope : pTag->tagName);
                if ( scopeIt != scopeMap.end() )
                {
                    hScopeItem = scopeIt->second;
                    if ( tagScope.empty() )
                    {
                        tTagData* pScopeTag = (tTagData *) m_tvTags.GetItemData(hScopeItem);
                        if ( !pScopeTag || pScopeTag->line != pTag->line )
                        {
                            if ( pTag->isTagTypeAllowingMultiScope() )
                            {
                                pTag->data.p = (void *) hScopeItem;
                                bAddItem = false;
                            }
                            else
                                hScopeItem = hFileItem;
                        }
                        else
                            bAddItem = false;
                    }
                }
                else
                {
                    if ( !tagScope.empty() )
                    {
                        // adding a scope item
                        t_string::size_type nPos = scopeSep.empty() ? t_string::npos : tagScope.rfind(scopeSep);
                        tagScopeName = (nPos == t_string::npos) ? tagScope : tagScope.substr(nPos + scopeSep.length());
                        tTagData* pScopeTag = m_Data.FindTagByNameAndScope(tagsByFile.filePath, tagScopeName, (nPos == t_string::npos) ? t_string() : tagScope.substr(0, nPos));
                        hScopeItem = addTreeViewItem(hScopeItem, tagScopeName, pScopeTag);
                        scopeMap[tagScope] = hScopeItem;

                        if ( !m_tagFilter.empty() )
                            setNodeItemExpanded(m_tvTags, hScopeItem);
                    }
                }
            }
            while ( nScopePos != t_string::npos );

            if ( bAddItem )
            {
                HTREEITEM hItem = addTreeViewItem(hScopeItem, pTag->tagName, pTag);
                pTag->data.p = (void *) hItem;

                if ( pTag->tagScope.empty() )
                {
                    tagScope = pTag->tagName;
                }
                else if ( !scopeSep.empty() )
                {
                    tagScope = pTag->tagScope;
                    tagScope += scopeSep;
                    tagScope += pTag->tagName;
                }
                else
                {
                    tagScope.clear();
                }

                if ( !tagScope.empty() )
                {
                    scopeMap[tagScope] = hItem;
                    if ( !m_tagFilter.empty() )
                        setNodeItemExpanded(m_tvTags, hItem);
                }
            }
        }
    }
    else
    {
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
                else
                {
                    tTagData* pScopeTag = (tTagData *) m_tvTags.GetItemData(hScopeItem);
                    if ( !pScopeTag || pScopeTag->line != pTag->line )
                    {
                        if ( pTag->isTagTypeAllowingMultiScope() )
                        {
                            pTag->data.p = (void *) hScopeItem;
                        }
                        else
                        {
                            HTREEITEM hItem = addTreeViewItem(hFileItem, pTag->tagName, pTag);
                            pTag->data.p = (void *) hItem;

                            // a scope item has been added
                            scopeMap[pTag->tagName] = hItem;

                            if ( !m_tagFilter.empty() )
                                setNodeItemExpanded(m_tvTags, hItem);
                        }
                    }
                }
            }
            else
            {
                HTREEITEM hScopeItem = hFileItem;

                if ( !pTag->tagScope.empty() )
                {
                    // adding a scope item
                    t_string::size_type nPos = scopeSep.empty() ? t_string::npos : pTag->tagScope.rfind(scopeSep);
                    tagScopeName = (nPos == t_string::npos) ? pTag->tagScope : pTag->tagScope.substr(nPos + scopeSep.length());
                    tTagData* pScopeTag = m_Data.FindTagByNameAndScope(tagsByFile.filePath, tagScopeName, (nPos == t_string::npos) ? t_string() : pTag->tagScope.substr(0, nPos));
                    hScopeItem = addTreeViewItem(hFileItem, pTag->tagScope, pScopeTag);
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

t_string CTagsDlg::getItemTextLV(int iItem) const
{
    if ( iItem >= 0 )
    {
        const tTagData* pTag = (const tTagData *) m_lvTags.GetItemData(iItem);
        if ( pTag )
        {
            TCHAR szNum[32];
            t_string S;

            ::wsprintf(szNum, _T("\t%d\t"), pTag->line);

            S.reserve(256);
            S += pTag->getFullTagName();
            S += _T('\t');
            S += pTag->tagType;
            S += szNum;
            S += getFileName(pTag);
            return S;
        }
    }
    return t_string();
}

t_string CTagsDlg::getAllItemsTextLV() const
{
    int nItems = m_lvTags.GetItemCount();
    if ( nItems > 0 )
    {
        t_string S;

        S.reserve(96*nItems);
        for ( int i = 0; i < nItems; ++i )
        {
            S += getItemTextLV(i);
            S += _T('\n');
        }
        return S;
    }
    return t_string();
}

t_string CTagsDlg::getItemTextTV(HTREEITEM hItem) const
{
    if ( hItem )
    {
        return m_tvTags.GetItemText(hItem).GetString();
    }
    return t_string();
}

t_string CTagsDlg::getItemAndChildrenTextTV(HTREEITEM hItem, const t_string& indent ) const
{
    if ( hItem )
    {
        t_string S;

        HTREEITEM hChildItem = m_tvTags.GetChild(hItem);
        if ( hChildItem != NULL )
        {
            S.reserve(1024);
            S += indent;
            S += getItemTextTV(hItem);
            S += _T('\n');
            do
            {
                S += getItemAndChildrenTextTV(hChildItem, indent + _T("  "));
                if ( !S.empty() && S.back() != _T('\n') )
                {
                    S += _T('\n');
                }
                hChildItem = m_tvTags.GetNextSibling(hChildItem);
            }
            while ( hChildItem != NULL );
        }
        else
        {
            S.reserve( 96 );
            S += indent;
            S += getItemTextTV(hItem);
        }
        return S;
    }
    return t_string();
}

t_string CTagsDlg::getAllItemsTextTV() const
{
    HTREEITEM hItem = m_tvTags.GetRootItem();
    if ( hItem )
    {
        t_string S;

        S.reserve(32*m_tvTags.GetCount());
        do
        {
            S += getItemAndChildrenTextTV(hItem);
            if ( !S.empty() && S.back() != _T('\n') )
            {
                S += _T('\n');
            }
            hItem = m_tvTags.GetNextSibling(hItem);
        }
        while ( hItem != NULL );

        return S;
    }
    return t_string();
}

void CTagsDlg::applyActionToItemTV(HTREEITEM hItem, eTreeNodeAction action)
{
    HTREEITEM hChildItem = m_tvTags.GetChild(hItem);
    if ( hChildItem )
    {
        m_tvTags.Expand(hItem, (action == TNA_EXPANDCHILDREN) ? TVE_EXPAND : TVE_COLLAPSE);
        do
        {
            applyActionToItemTV(hChildItem, action);
            hChildItem = m_tvTags.GetNextSibling(hChildItem);
        }
        while ( hChildItem != NULL );
    }
}
