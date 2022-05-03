#include "Plugin.h"
#include "../TagsViewBase/c_base/HexStr.h"
#include "../TagsViewBase/SettingsDlg.h"


// consts
const TCHAR* cszPluginName = _T("TagsView");


// funcs
LRESULT CALLBACK NewEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NewFrameProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NewMainProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


// CAkelOptionsReaderWriter class
//----------------------------------------------------------------------------

CAkelOptionsReaderWriter::CAkelOptionsReaderWriter() : _hOptions(NULL), _hMainWnd(NULL)
{

}

void CAkelOptionsReaderWriter::SetMainWnd(HWND hMainWnd)
{
    _hMainWnd = hMainWnd;
}

std::basic_string<TCHAR> CAkelOptionsReaderWriter::getFullOptionName(COptionsManager::const_opt_itr itr)
{
    std::basic_string<TCHAR> s;

    s += itr->getOptionGroup();
    s += _T('\\');
    s += itr->getOptionName();

    return s;
}

bool CAkelOptionsReaderWriter::beginReadOptions()
{
    if ( _hMainWnd )
    {
        _hOptions = (HANDLE) ::SendMessage( _hMainWnd,
            AKD_BEGINOPTIONS, POB_READ, (LPARAM) cszPluginName );
    }
    else
    {
        _hOptions = NULL;
    }

    return (_hOptions != NULL);
}

void CAkelOptionsReaderWriter::endReadOptions()
{
    ::SendMessage( _hMainWnd, AKD_ENDOPTIONS, (WPARAM) _hOptions, 0 );
    _hOptions = NULL;
}

bool CAkelOptionsReaderWriter::readDataOption(COptionsManager::opt_itr itr)
{
#ifdef UNICODE
    PLUGINOPTIONW po;
#else
    PLUGINOPTIONA po;
#endif
    COption::byte_tt data[COptionsManager::MAX_STRSIZE];
    auto s = getFullOptionName(itr);

    po.pOptionName = s.c_str();
    po.lpData = (BYTE *) data;
    po.dwData = (COptionsManager::MAX_STRSIZE - 1);
    po.dwType = PO_BINARY;
    int nsize = (int) ::SendMessage( _hMainWnd, AKD_OPTION, (WPARAM) _hOptions, (LPARAM) &po );

    itr->setData(data, nsize);

    return true;
}

bool CAkelOptionsReaderWriter::readHexIntOption(COptionsManager::opt_itr itr)
{
    TCHAR str[COptionsManager::MAX_NUMSTRSIZE];

#ifdef UNICODE
    PLUGINOPTIONW po;
#else
    PLUGINOPTIONA po;
#endif

    auto s = getFullOptionName(itr);

    str[0] = _T('0');
    str[1] = _T('x');
    str[2] = _T('0');
    str[3] = 0;
    po.pOptionName = s.c_str();
    po.lpData = (BYTE *) (str + 2);
    po.dwData = (COptionsManager::MAX_NUMSTRSIZE - 3)*sizeof(TCHAR);
    po.dwType = PO_STRING;
    ::SendMessage( _hMainWnd, AKD_OPTION, (WPARAM) _hOptions, (LPARAM) &po );

    unsigned int uval = _tcstoul(str, NULL, 16);
    itr->setHexInt(uval);

    return true;
}

bool CAkelOptionsReaderWriter::readIntOption(COptionsManager::opt_itr itr)
{
#ifdef UNICODE
    PLUGINOPTIONW po;
#else
    PLUGINOPTIONA po;
#endif
    DWORD dwValue = 0;
    auto s = getFullOptionName(itr);

    po.pOptionName = s.c_str();
    po.lpData = (BYTE *) &dwValue;
    po.dwData = sizeof(DWORD);
    po.dwType = PO_DWORD;
    if ( ::SendMessage(_hMainWnd, AKD_OPTION, (WPARAM) _hOptions, (LPARAM) &po) == sizeof(DWORD) )
    {
        itr->setUint(dwValue);

        return true;
    }

    return false;
}

bool CAkelOptionsReaderWriter::readStrOption(COptionsManager::opt_itr itr)
{
#ifdef UNICODE
    PLUGINOPTIONW po;
#else
    PLUGINOPTIONA po;
#endif
    TCHAR str[COptionsManager::MAX_STRSIZE];
    auto s = getFullOptionName(itr);

    str[0] = 0;
    po.pOptionName = s.c_str();
    po.lpData = (BYTE *) str;
    po.dwData = (COptionsManager::MAX_STRSIZE - 1)*sizeof(TCHAR);
    po.dwType = PO_STRING;
    ::SendMessage( _hMainWnd, AKD_OPTION, (WPARAM) _hOptions, (LPARAM) &po );

    itr->setStr(str);

    return true;
}

bool CAkelOptionsReaderWriter::beginWriteOptions()
{
    if ( _hMainWnd )
    {
        _hOptions = (HANDLE) ::SendMessage( _hMainWnd,
            AKD_BEGINOPTIONS, POB_SAVE, (LPARAM) cszPluginName );
    }
    else
    {
        _hOptions = NULL;
    }

    return (_hOptions != NULL);
}

void CAkelOptionsReaderWriter::endWriteOptions()
{
    ::SendMessage( _hMainWnd, AKD_ENDOPTIONS, (WPARAM) _hOptions, 0 );
    _hOptions = NULL;
}

bool CAkelOptionsReaderWriter::writeDataOption(COptionsManager::const_opt_itr itr)
{
#ifdef UNICODE
    PLUGINOPTIONW po;
#else
    PLUGINOPTIONA po;
#endif
    int nsize;
    auto s = getFullOptionName(itr);

    po.pOptionName = s.c_str();
    po.lpData = (BYTE *) itr->getData(&nsize);
    po.dwData = nsize;
    po.dwType = PO_BINARY;
    int n = (int) ::SendMessage(_hMainWnd, AKD_OPTION, (WPARAM) _hOptions, (LPARAM) &po);

    return (n >= nsize);
}

bool CAkelOptionsReaderWriter::writeHexIntOption(COptionsManager::const_opt_itr itr)
{
    TCHAR str[COptionsManager::MAX_NUMSTRSIZE];

#ifdef UNICODE
    PLUGINOPTIONW po;
#else
    PLUGINOPTIONA po;
#endif
    int nlen = ::wsprintf( str, _T("%X"), itr->getUint() );
    auto s = getFullOptionName(itr);

    po.pOptionName = s.c_str();
    po.lpData = (BYTE *) str;
    po.dwData = (nlen + 1)*sizeof(TCHAR);
    po.dwType = PO_STRING;
    UINT n = (UINT) ::SendMessage(_hMainWnd, AKD_OPTION, (WPARAM) _hOptions, (LPARAM) &po);

    return (n >= nlen*sizeof(TCHAR));
}

bool CAkelOptionsReaderWriter::writeIntOption(COptionsManager::const_opt_itr itr)
{
#ifdef UNICODE
    PLUGINOPTIONW po;
#else
    PLUGINOPTIONA po;
#endif
    DWORD dwValue = itr->getUint();
    auto s = getFullOptionName(itr);

    po.pOptionName = s.c_str();
    po.lpData = (BYTE *) &dwValue;
    po.dwData = sizeof(DWORD);
    po.dwType = PO_DWORD;
    UINT n = (UINT) ::SendMessage(_hMainWnd, AKD_OPTION, (WPARAM) _hOptions, (LPARAM) &po);

    return (n == sizeof(DWORD));
}

bool CAkelOptionsReaderWriter::writeStrOption(COptionsManager::const_opt_itr itr)
{
#ifdef UNICODE
    PLUGINOPTIONW po;
#else
    PLUGINOPTIONA po;
#endif
    int nlen;
    auto s = getFullOptionName(itr);

    po.pOptionName = s.c_str();
    po.lpData = (BYTE *) itr->getStr(&nlen);
    po.dwData = (nlen + 1)*sizeof(TCHAR);
    po.dwType = PO_STRING;
    UINT n = (UINT) ::SendMessage(_hMainWnd, AKD_OPTION, (WPARAM) _hOptions, (LPARAM) &po);

    return (n >= nlen*sizeof(TCHAR));
}

// CAkelTagsDlg class
//----------------------------------------------------------------------------

void CAkelTagsDlg::initOptions()
{
    if ( m_opt.HasOptions() )
        return;

    // calling base initOptions()
    CTagsDlg::initOptions();

    const TCHAR cszView[]  = _T("View");

    m_opt.AddData( OPT_DOCKRECT,     cszView,  _T("DockRect"), NULL,  0 );
}

// CTagsViewPlugin class
//----------------------------------------------------------------------------

LPCTSTR CTagsViewPlugin::ewGetEditorName() const
{
    return _T("AkelPad");
}

LPCTSTR CTagsViewPlugin::ewGetEditorShortName() const
{
    return _T("akel");
}

void CTagsViewPlugin::ewDoSaveFile()
{
    ::SendMessage(m_hMainWnd, WM_COMMAND, IDM_FILE_SAVE, 0);
}

void CTagsViewPlugin::ewDoSetFocus()
{
    HWND hEdit = ewGetEditHwnd();
    ::SetFocus(hEdit);
}

void CTagsViewPlugin::ewDoSetSelection(int selStart, int selEnd)
{
    CHARRANGE cr;

    cr.cpMin = selStart;
    cr.cpMax = selEnd;

    HWND hEdit = ewGetEditHwnd();
    ::SendMessage( hEdit, WM_SETREDRAW, (WPARAM) FALSE, 0 );
    ::SendMessage( hEdit, EM_EXSETSEL, 0, (LPARAM) &cr );
    if ( bAkelEdit )
    {
        AESCROLLTOPOINT stp;

        stp.dwFlags = AESC_POINTCARET | AESC_OFFSETCHARX | AESC_OFFSETCHARY | AESC_FORCETOP;
        stp.nOffsetX = 3;
        stp.nOffsetY = 2;
        SendMessage( hEdit, AEM_SCROLLTOPOINT, 0, (LPARAM) &stp );

    }
    else
    {
        int currentLine = (int) ::SendMessage( hEdit, EM_EXLINEFROMCHAR, 0, cr.cpMin );
        int firstLine = (int) ::SendMessage( hEdit, EM_GETFIRSTVISIBLELINE, 0, 0 );
        ::SendMessage( hEdit, EM_LINESCROLL, 0, (currentLine - firstLine) );
    }
    ::SendMessage( hEdit, WM_SETREDRAW, (WPARAM) TRUE, 0 );
    ::InvalidateRect( hEdit, NULL, TRUE );
    ::UpdateWindow( hEdit );
}

HWND CTagsViewPlugin::ewGetEditHwnd() const
{
    EDITINFO ei;

    ei.hWndEdit = NULL;
    ::SendMessage( m_hMainWnd, AKD_GETEDITINFO, (WPARAM) NULL, (LPARAM) &ei );

    return ei.hWndEdit;
}

CTagsViewPlugin::t_string CTagsViewPlugin::ewGetFilePathName() const
{
    t_string fileName;
    EDITINFO ei;

    ei.pFile = NULL;
    ::SendMessage( m_hMainWnd, AKD_GETEDITINFO, (WPARAM) NULL, (LPARAM) &ei );
    const TCHAR* pszFileName = (const TCHAR *) ei.pFile;
    if ( pszFileName && pszFileName[0] )
    {
        fileName = pszFileName;
    }

    return fileName;
}

int CTagsViewPlugin::ewGetLineFromPos(int pos) const
{
    HWND hEdit = ewGetEditHwnd();
    int  nLine = (int) ::SendMessage(hEdit, EM_EXLINEFROMCHAR, 0, pos);

    if ( bAkelEdit )
    {
        if ( ::SendMessage(hEdit, AEM_GETWORDWRAP, 0, (LPARAM)NULL) )
            nLine = (int) ::SendMessage(hEdit, AEM_GETUNWRAPLINE, nLine, 0);
    }

    return nLine;
}

int CTagsViewPlugin::ewGetPosFromLine(int line) const
{
    HWND hEdit = ewGetEditHwnd();

    if ( bAkelEdit )
    {
        if ( ::SendMessage(hEdit, AEM_GETWORDWRAP, 0, (LPARAM)NULL) )
            line = (int) ::SendMessage(hEdit, AEM_GETWRAPLINE, line, 0);
    }

    return (int) ::SendMessage(hEdit, EM_LINEINDEX, line, 0);
}

int CTagsViewPlugin::ewGetSelectionPos(int& selEnd) const
{
    CHARRANGE cr = { 0, 0 };

    ::SendMessage( ewGetEditHwnd(), EM_EXGETSEL, 0, (LPARAM) &cr );
    selEnd = cr.cpMax;
    return (int) cr.cpMin;
}

/*
CTagsViewPlugin::t_string CTagsViewPlugin::ewGetTextLine(int line) const
{
}
*/

bool CTagsViewPlugin::ewIsFileSaved() const
{
    EDITINFO ei;

    ei.pFile = NULL;
    ei.bModified = FALSE;
    ::SendMessage( m_hMainWnd, AKD_GETEDITINFO, (WPARAM) NULL, (LPARAM) &ei );
    const TCHAR* pszFileName = (const TCHAR *) ei.pFile;
    if ( pszFileName && pszFileName[0] )
    {
        return (ei.bModified ? false : true);
    }
    else if ( !pszFileName )
    {
        return true;
    }

    return false;
}

bool CTagsViewPlugin::ewGetEditorColors(sEditorColors& colors) const
{
    ::ZeroMemory(&colors, sizeof(colors));

    int nValuesRead = 0;
    PLUGINFUNCTION* pf = (PLUGINFUNCTION *) SendMessage( m_hMainWnd, AKD_DLLFIND, (WPARAM) _T("Coder::HighLight"), 0 );

    if ( pf && pf->bRunning )
    {
        struct sCallParams
        {
            UINT_PTR dwStructSize;
            INT_PTR nAction;
            HWND hEditWnd;
            LPVOID hEditDoc;
            LPCTSTR pszVarName;
            LPTSTR pszVarValue;
            int* pnVarValueLen;
        };

        PLUGINCALLSEND pcs;
        sCallParams callParams;
        int nValueLen;
        TCHAR szValue[64];

        nValueLen = 0;
        szValue[0] = 0;
        callParams.dwStructSize = sizeof(callParams);
        callParams.nAction = 22; // DLLA_CODER_GETVARIABLE
        callParams.hEditWnd = ewGetEditHwnd();
        callParams.hEditDoc = NULL;
        callParams.pszVarName = _T("HighLight_BasicTextColor");
        callParams.pszVarValue = szValue;
        callParams.pnVarValueLen = &nValueLen;

        pcs.pFunction = _T("Coder::Settings");
        pcs.lParam = (LPARAM) &callParams;
        pcs.dwSupport = 0;

        SendMessage( m_hMainWnd, AKD_DLLCALL, 0, (LPARAM) &pcs );
        if ( nValueLen != 0 )
        {
            if ( szValue[0] == _T('#') && nValueLen >= 7 )
            {
                c_base::_thexstr2buf( szValue + 1, reinterpret_cast<c_base::byte_t*>(&colors.crTextColor), 6 );
                ++nValuesRead;
            }
        }

        nValueLen = 0;
        szValue[0] = 0;
        callParams.pszVarName = _T("HighLight_BasicBkColor");

        pcs.pFunction = _T("Coder::Settings");
        pcs.lParam = (LPARAM) &callParams;
        pcs.dwSupport = 0;

        SendMessage( m_hMainWnd, AKD_DLLCALL, 0, (LPARAM) &pcs );
        if ( nValueLen != 0 )
        {
            if ( szValue[0] == _T('#') && nValueLen >= 7 )
            {
                c_base::_thexstr2buf( szValue + 1, reinterpret_cast<c_base::byte_t*>(&colors.crBkgndColor), 6 );
                ++nValuesRead;
            }
        }
    }

    return (nValuesRead == 2);
}

void CTagsViewPlugin::ewCloseTagsView()
{
    ewClearNavigationHistory(true);
    clearCurrentFilePathName();
    Uninitialize(false);
    GetTagsDlg().ClearItems();
}

void CTagsViewPlugin::ewDoNavigateBackward()
{
    //if ( ::GetFocus() != ewGetEditHwnd() )
    m_tagsDlg.SetFocusTo(CTagsDlg::TVF_TAGSVIEW);

    // calling basic implementation
    CEditorWrapper::ewDoNavigateBackward();
}

void CTagsViewPlugin::ewDoNavigateForward()
{
    //if ( ::GetFocus() != ewGetEditHwnd() )
    m_tagsDlg.SetFocusTo(CTagsDlg::TVF_TAGSVIEW);

    // calling basic implementation
    CEditorWrapper::ewDoNavigateForward();
}

void CTagsViewPlugin::Initialize(PLUGINDATA* pd)
{
    if ( bInitialized )
        return;

    TCHAR szCurDir[2*MAX_PATH];
    unsigned int uLen;

    ewSetMainHwnd(pd->hMainWnd);
    bOldWindows  = pd->bOldWindows;
    bOldRichEdit = pd->bOldRichEdit;
    nMDI         = pd->nMDI;
    bAkelEdit    = pd->bAkelEdit;

    uLen = ::GetModuleFileName( (HMODULE) pd->hInstanceDLL, szCurDir, sizeof(szCurDir) - 1 );
    while ( uLen > 0 )
    {
        --uLen;
        if ( (szCurDir[uLen] == _T('\\')) || (szCurDir[uLen] == _T('/')) )
        {
            break;
        }
    }
    szCurDir[uLen] = 0;

    if ( uLen > 0 )
    {
        tString ctagsExePath = szCurDir;
        ctagsExePath += _T("\\TagsView\\ctags.exe");
        m_tagsDlg.SetCTagsExePath(ctagsExePath);
    }

    m_optRdWr.SetMainWnd(m_hMainWnd);
    m_tagsDlg.SetOptionsReaderWriter(&m_optRdWr);
    m_tagsDlg.ReadOptions();

    if ( !m_pDockData )
    {
        static DOCK dk = { 0 };
        dk.nSide = DKS_LEFT;
        dk.dwFlags = DKF_DRAGDROP | DKF_NODROPTOP | DKF_NODROPBOTTOM;

        int nsize = 0;
        const void* pdata = m_tagsDlg.GetOptions().getData(CAkelTagsDlg::OPT_DOCKRECT, &nsize);
        if ( nsize == sizeof(RECT) )
        {
            CopyMemory( &dk.rcSize, pdata, sizeof(RECT) );
        }

        m_pDockData = (DOCK *) ::SendMessage( m_hMainWnd, AKD_DOCK, DK_ADD, (LPARAM) &dk );
    }

    // SubClass main window
    pMainProcData = NULL;
    ::SendMessage( m_hMainWnd, AKD_SETMAINPROC,
        (WPARAM) NewMainProc, (LPARAM) &pMainProcData );

    if ( nMDI == WMD_MDI )
    {
        // SubClass frame window
        pFrameProcData = NULL;
        ::SendMessage( m_hMainWnd, AKD_SETFRAMEPROC,
            (WPARAM) NewFrameProc, (LPARAM) &pFrameProcData );
    }

    // SubClass edit window
    pEditProcData = NULL;
    ::SendMessage( m_hMainWnd, AKD_SETEDITPROC,
        (WPARAM) NewEditProc, (LPARAM) &pEditProcData );

    bInitialized = TRUE;
}

void CTagsViewPlugin::Uninitialize(bool bDestroyTagsDlg)
{
    if ( bDestroyTagsDlg )
    {
        if ( m_pDockData )
        {
            m_tagsDlg.GetOptions().setData(CAkelTagsDlg::OPT_DOCKRECT, &m_pDockData->rcSize, sizeof(RECT));
        }

        m_tagsDlg.SaveOptions();
    }

    if ( !bInitialized )
        return;

    bInitialized = FALSE;
    bTagsDlgVisible = FALSE;

    if ( pMainProcData )
    {
        // Remove subclass (main window)
        ::SendMessage( m_hMainWnd, AKD_SETMAINPROC,
            (WPARAM) NULL, (LPARAM) &pMainProcData );
        pMainProcData = NULL;
    }
    if ( (nMDI == WMD_MDI) && pFrameProcData )
    {
        // Remove subclass (frame window)
        ::SendMessage( m_hMainWnd, AKD_SETFRAMEPROC,
            (WPARAM) NULL, (LPARAM) &pFrameProcData );
        pFrameProcData = NULL;
    }

    if ( pEditProcData )
    {
        // Remove subclass (edit window)
        ::SendMessage( m_hMainWnd, AKD_SETEDITPROC,
            (WPARAM) NULL, (LPARAM) &pEditProcData );
        pEditProcData = NULL;
    }

    if ( bDestroyTagsDlg && m_tagsDlg.GetHwnd() )
    {
        m_tagsDlg.Destroy();
    }

    if ( m_pDockData )
    {
        ::SendMessage( m_hMainWnd, AKD_DOCK, DK_HIDE, (LPARAM) m_pDockData );
        if ( bDestroyTagsDlg )
        {
            ::SendMessage( m_hMainWnd, AKD_DOCK, DK_DELETE | DK_UNSUBCLASS, (LPARAM) m_pDockData );
            m_pDockData = NULL;
            m_isTagsDlgSubclassed = false;
        }
    }

}

void CTagsViewPlugin::DockTagsDlg()
{
    if ( m_pDockData )
    {
        RECT& rcSize = m_pDockData->rcSize;
        if ( (rcSize.right == 0) || (rcSize.bottom == 0) )
        {
            if ( ::GetWindowRect(m_tagsDlg.GetHwnd(), &rcSize) )
            {
                rcSize.right -= rcSize.left;
                rcSize.left = 0;
                rcSize.bottom -= rcSize.top;
                rcSize.top = 0;
            }
        }
        m_pDockData->hWnd = m_tagsDlg.GetHwnd();
        if ( !m_isTagsDlgSubclassed )
        {
            ::SendMessage( m_hMainWnd, AKD_DOCK, (DK_SUBCLASS | DK_SHOW), (LPARAM) m_pDockData );
            m_isTagsDlgSubclassed = true;
        }
        else
            ::SendMessage( m_hMainWnd, AKD_DOCK, DK_SHOW, (LPARAM) m_pDockData );
    }
}

//----------------------------------------------------------------------------


// global vars
CTagsViewPlugin thePlugin;
bool g_isClosing = false; // closing a document or exiting AkelPad
bool g_isOpeningDocument = false;
bool g_isAkelPadStarting = false;
bool g_isMainWindowShown = false;


// Identification
extern "C" void __declspec(dllexport) DllAkelPadID(PLUGINVERSION *pv)
{
    pv->dwAkelDllVersion  = AKELDLL;
    pv->dwExeMinVersion3x = MAKE_IDENTIFIER(-1, -1, -1, -1);
    pv->dwExeMinVersion4x = MAKE_IDENTIFIER(4, 9, 8, 0);
    pv->pPluginName = "TagsView";
}

// Plugin extern function
extern "C" void __declspec(dllexport) TagsView(PLUGINDATA *pd)
{
#ifdef UNICODE
    pd->dwSupport |= PDS_NOANSI;
#else
    pd->dwSupport |= PDS_NOUNICODE;
#endif
    if ( pd->dwSupport & PDS_GETSUPPORT )
        return;

#ifdef UNICODE
    if ( pd->bOldWindows )
#else
    if ( !pd->bOldWindows )
#endif
    {
        ::MessageBox(
            pd->hMainWnd,
#ifdef UNICODE
            _T("This version of TagsView does not support Windows 9x/Me"),
#else
            _T("This version of TagsView supports only Windows 9x/Me"),
#endif
            _T("TagsView plugin"),
            MB_OK | MB_ICONERROR
        );
        pd->nUnload = UD_UNLOAD;
        return;
    }

    // Is plugin already loaded?
    if ( thePlugin.bInitialized && thePlugin.bTagsDlgVisible )
    {
        thePlugin.ewCloseTagsView();
        pd->nUnload = UD_NONUNLOAD_NONACTIVE;
        return;
    }
    else
    {
        thePlugin.Initialize(pd);
        if ( !thePlugin.bTagsDlgVisible )
        {
            thePlugin.GetTagsDlg().SetInstDll(pd->hInstanceDLL);
            if ( !thePlugin.GetTagsDlg().GetHwnd() )
            {
                thePlugin.GetTagsDlg().DoModeless(thePlugin.ewGetMainHwnd()); // create modeless dialog
            }
            //thePlugin.GetTagsDlg().SetWindowLongPtr(GWL_STYLE, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
            thePlugin.DockTagsDlg(); // dock the dialog
            //thePlugin.GetTagsDlg().ShowWindow();
            thePlugin.bTagsDlgVisible = TRUE;
        }

        if ( !pd->bOnStart )
        {
            g_isMainWindowShown = true;
            thePlugin.GetTagsDlg().ReparseCurrentFile();
        }
    }

    // Stay in memory, and show as active
    pd->nUnload = UD_NONUNLOAD_ACTIVE;
}

// Plugin extern function
extern "C" void __declspec(dllexport) Settings(PLUGINDATA *pd)
{
#ifdef UNICODE
    pd->dwSupport |= (PDS_NOAUTOLOAD | PDS_NOANSI);
#else
    pd->dwSupport |= (PDS_NOAUTOLOAD | PDS_NOUNICODE);
#endif
    if ( pd->dwSupport & PDS_GETSUPPORT )
        return;

#ifdef UNICODE
    if ( pd->bOldWindows )
#else
    if ( !pd->bOldWindows )
#endif
    {
        ::MessageBox(
            pd->hMainWnd,
#ifdef UNICODE
            _T("This version of TagsView does not support Windows 9x/Me"),
#else
            _T("This version of TagsView supports only Windows 9x/Me"),
#endif
            _T("TagsView plugin"),
            MB_OK | MB_ICONERROR
        );
        pd->nUnload = UD_UNLOAD;
        return;
    }

    thePlugin.Initialize(pd);

    if ( !pd->bOnStart )
    {
        g_isMainWindowShown = true;
    }

    CSettingsDlg dlg(thePlugin.GetTagsDlg().GetOptions());
    dlg.DoModal(thePlugin.ewGetMainHwnd());

    if ( thePlugin.GetTagsDlg().GetHwnd() &&
         thePlugin.GetTagsDlg().IsWindowVisible() )
    {
        thePlugin.GetTagsDlg().ApplyColors();
    }

    // Stay in memory, and show as active
    pd->nUnload = UD_NONUNLOAD_NONACTIVE;
}

// Entry point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID lpReserved)
{
    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
            g_isClosing = false;
            g_isOpeningDocument = false;
            g_isAkelPadStarting = false;
            g_isMainWindowShown = false;
            thePlugin.SetResourceHandle(hInstDLL);
            break;
        case DLL_PROCESS_DETACH:
            thePlugin.Uninitialize(true);
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        default:
            break;
    }
    return TRUE;
}

LRESULT CALLBACK NewEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_KEYDOWN:
        {
            LRESULT lResult = 0;


            if ( thePlugin.pEditProcData && thePlugin.pEditProcData->NextProc )
                lResult = thePlugin.pEditProcData->NextProc(hWnd, uMsg, wParam, lParam);


            return lResult;
        }
        case WM_KEYUP:
        {
            break;
        }
        case WM_LBUTTONDOWN:
        {
            LRESULT lResult = 0;

            if ( thePlugin.pEditProcData && thePlugin.pEditProcData->NextProc )
                lResult = thePlugin.pEditProcData->NextProc(hWnd, uMsg, wParam, lParam);

            return lResult;
        }
        case WM_LBUTTONUP:
        {
            break;
        }
        case WM_PAINT:
        {
            thePlugin.GetTagsDlg().ApplyColors();
            break;
        }
        default:
        {
            break;
        }
    }

    if ( thePlugin.pEditProcData && thePlugin.pEditProcData->NextProc )
        return thePlugin.pEditProcData->NextProc(hWnd, uMsg, wParam, lParam);
    else
        return 0;
}

LRESULT CALLBACK NewFrameProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_NOTIFY:
            if ( !g_isClosing )
            {
                LPNMHDR pnmh = (LPNMHDR) lParam;
                if ( pnmh->code == EN_SELCHANGE )
                {
                    LRESULT lResult = 0;

                    if ( thePlugin.pFrameProcData && thePlugin.pFrameProcData->NextProc )
                        lResult = thePlugin.pFrameProcData->NextProc(hWnd, uMsg, wParam, lParam);

                    if ( g_isMainWindowShown && !g_isAkelPadStarting && !g_isOpeningDocument )
                        thePlugin.ewOnSelectionChanged();

                    return lResult;
                }
            }
            break;

        default:
            break;
    }

    if ( thePlugin.pFrameProcData && thePlugin.pFrameProcData->NextProc )
        return thePlugin.pFrameProcData->NextProc(hWnd, uMsg, wParam, lParam);
    else
        return 0;
}

LRESULT CALLBACK NewMainProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_NOTIFY:
            if ( !g_isClosing )
            {
                NMHDR* pnmh = (NMHDR*) lParam;
                if ( pnmh->code == EN_SELCHANGE )
                {
                    LRESULT lResult = 0;

                    if ( thePlugin.pMainProcData && thePlugin.pMainProcData->NextProc)
                        lResult = thePlugin.pMainProcData->NextProc(hWnd, uMsg, wParam, lParam);

                    if ( g_isMainWindowShown && !g_isAkelPadStarting && !g_isOpeningDocument )
                        thePlugin.ewOnSelectionChanged();

                    return lResult;
                }
            }
            break;

        case WM_COMMAND:
        {
            LRESULT lResult = 0;

            if ( thePlugin.pMainProcData && thePlugin.pMainProcData->NextProc)
                lResult = thePlugin.pMainProcData->NextProc(hWnd, uMsg, wParam, lParam);

            switch ( LOWORD(wParam) )
            {
                case IDM_WINDOW_FRAMECLOSE:
                case IDM_WINDOW_FRAMECLOSEALL:
                case IDM_WINDOW_FRAMECLOSEALL_BUTACTIVE:
                case IDM_WINDOW_FILECLOSE:
                    if ( thePlugin.ewGetFilePathName().empty() )
                    {
                        thePlugin.ewOnFileClosed();
                    }
                    break;
            }

            return lResult;
        }

        case AKDN_SAVEDOCUMENT_FINISH:
        {
            LRESULT lResult = 0;

            if ( thePlugin.pMainProcData && thePlugin.pMainProcData->NextProc)
                lResult = thePlugin.pMainProcData->NextProc(hWnd, uMsg, wParam, lParam);

            thePlugin.ewOnFileSaved();

            return lResult;
        }

        case AKDN_FRAME_ACTIVATE:
            if ( !g_isClosing )
            {
                LRESULT lResult = 0;

                if ( thePlugin.pMainProcData && thePlugin.pMainProcData->NextProc)
                    lResult = thePlugin.pMainProcData->NextProc(hWnd, uMsg, wParam, lParam);

                if ( g_isMainWindowShown && !g_isAkelPadStarting )
                {
                    thePlugin.ewOnFileActivated();
                }

                return lResult;
            }
            break;

        case AKDN_OPENDOCUMENT_START:
            g_isOpeningDocument = true;
            break;

        case AKDN_OPENDOCUMENT_FINISH:
            if ( !g_isClosing )
            {
                LRESULT lResult = 0;

                if ( thePlugin.pMainProcData && thePlugin.pMainProcData->NextProc)
                    lResult = thePlugin.pMainProcData->NextProc(hWnd, uMsg, wParam, lParam);

                g_isOpeningDocument = false;
                if ( g_isMainWindowShown && !g_isAkelPadStarting )
                {
                    thePlugin.ewOnFileOpened();
                }

                return lResult;
            }
            break;

        case AKDN_EDIT_ONSTART:
            break;

        case AKDN_EDIT_ONFINISH:
        {
            LRESULT lResult = 0;
            HWND    hEditParent = NULL;

            g_isClosing = true;

            if ( thePlugin.bAkelEdit )
            {
                hEditParent = (HWND) ::SendMessage((HWND)wParam, AEM_GETMASTER, 0, 0);
                if ( hEditParent == (HWND)wParam )
                {
                    hEditParent = NULL;
                }
            }

            if ( thePlugin.pMainProcData && thePlugin.pMainProcData->NextProc)
                lResult = thePlugin.pMainProcData->NextProc(hWnd, uMsg, wParam, lParam);

            if ( !hEditParent )
            {
                thePlugin.ewOnFileClosed();
            }

            g_isClosing = false;

            return lResult;
        }

        case AKDN_MAIN_ONFINISH:
        {
            LRESULT lResult = 0;

            g_isClosing = true;

            if ( thePlugin.pMainProcData && thePlugin.pMainProcData->NextProc)
                lResult = thePlugin.pMainProcData->NextProc(hWnd, uMsg, wParam, lParam);

            thePlugin.Uninitialize(true);

            return lResult;
        }

        case AKDN_MAIN_ONSTART_PRESHOW:
            g_isAkelPadStarting = true;
            break;

        case AKDN_MAIN_ONSTART_SHOW:
            g_isAkelPadStarting = true;
            g_isMainWindowShown = true;
            break;

        case AKDN_MAIN_ONSTART_FINISH:
        {
            LRESULT lResult = 0;

            if ( thePlugin.pMainProcData && thePlugin.pMainProcData->NextProc)
                lResult = thePlugin.pMainProcData->NextProc(hWnd, uMsg, wParam, lParam);

            g_isAkelPadStarting = false;
            thePlugin.ewOnFileOpened();

            return lResult;
        }

        case WM_CLOSE:
        {
            WNDPROC pWndProc = thePlugin.pMainProcData ? thePlugin.pMainProcData->NextProc : NULL;

            thePlugin.GetTagsDlg().CloseDlg();

            if ( pWndProc )
                return pWndProc(hWnd, uMsg, wParam, lParam);
            else
                return 0;
        }

        default:
            break;
    }

    if ( thePlugin.pMainProcData && thePlugin.pMainProcData->NextProc )
        return thePlugin.pMainProcData->NextProc(hWnd, uMsg, wParam, lParam);
    else
        return 0;
}

