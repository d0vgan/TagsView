#include "Plugin.h"
#include "../TagsViewBase/resource.h"
#include "../TagsViewBase/SettingsDlg.h"

// CTagsViewPlugin class
//----------------------------------------------------------------------------

static tTbData dockData;
static TCHAR   szPluginName[128] = _T("TagsView");
static TCHAR   szPluginNameDll[128] = _T("TagsView.dll");

#define PLUGIN_WND_PROC 0

#if PLUGIN_WND_PROC
    static WNDPROC nppOriginalWndProc = NULL;
    static LRESULT CALLBACK nppPluginWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
#endif

static void funcTagsView();
static void funcSettings();
static void funcAbout();

FuncItem CNppTagsDlg::FUNC_ARRAY[EFI_COUNT] = {
    { _T("TagsView"),    funcTagsView, 0, false, NULL },
    { _T("Settings..."), funcSettings, 0, false, NULL },
    { _T(""),            NULL,         0, false, NULL }, // separator
    { _T("About"),       funcAbout,    0, false, NULL }
};

LPCTSTR CTagsViewPlugin::ewGetEditorName() const
{
    return _T("Notepad++");
}

LPCTSTR CTagsViewPlugin::ewGetEditorShortName() const
{
    return _T("npp");
}

bool CTagsViewPlugin::ewDoOpenFile(LPCTSTR pszFileName)
{
    bool bOpened = true;

    if ( !SendNppMsg(NPPM_SWITCHTOFILE, 0, (LPARAM) pszFileName) )
    {
        if ( !SendNppMsg(NPPM_DOOPEN, 0, (LPARAM) pszFileName) )
            bOpened = false;
    }

    return bOpened;
}

void CTagsViewPlugin::ewDoSaveFile()
{
    SendNppMsg(NPPM_SAVECURRENTFILE);
}

void CTagsViewPlugin::ewDoSetFocus()
{
    const HWND hSciEdit = ewGetEditHwnd();
    ::SetFocus(hSciEdit);
}

void CTagsViewPlugin::ewDoSetSelection(int selStart, int selEnd)
{
    const HWND hSciEdit = ewGetEditHwnd();
    int line = (int) ::SendMessage( hSciEdit, SCI_LINEFROMPOSITION, selStart, 0 );
    ::SendMessage(hSciEdit, SCI_ENSUREVISIBLE, line, 0);

    //int visLine = ::SendMessage(hSciEdit, SCI_VISIBLEFROMDOCLINE, line, 0);
    int firstLine = (int) ::SendMessage( hSciEdit, SCI_GETFIRSTVISIBLELINE, 0, 0 );
    int visibleLines = (int) ::SendMessage( hSciEdit, SCI_LINESONSCREEN, 0, 0 );

    if ( line > firstLine && line < firstLine + visibleLines - 1 )
    {
        ::SendMessage( hSciEdit, SCI_SETSEL, selStart, selEnd );
        return;
    }

    int offset = -visibleLines/12;
    if ( firstLine < line )
        offset = visibleLines - 1 + offset;

    ::SendMessage( hSciEdit, WM_SETREDRAW, (WPARAM) FALSE, 0 );
    ::SendMessage( hSciEdit, SCI_SETSEL, selStart, selEnd );
    ::SendMessage( hSciEdit, SCI_GOTOLINE, line, 0 );
    ::SendMessage( hSciEdit, SCI_LINESCROLL, 0, offset );
    ::SendMessage( hSciEdit, WM_SETREDRAW, (WPARAM) TRUE, 0 );
    ::InvalidateRect( hSciEdit, NULL, TRUE );
    ::UpdateWindow( hSciEdit );
}

HWND CTagsViewPlugin::ewGetEditHwnd() const
{
    int currentView = 0;
    SendNppMsg( NPPM_GETCURRENTSCINTILLA, 0, (LPARAM) &currentView );
    return ( (currentView == 0) ?
      m_nppData._scintillaMainHandle : m_nppData._scintillaSecondHandle );
}

CTagsViewPlugin::t_string CTagsViewPlugin::ewGetFilePathName() const
{
    TCHAR szFilePathName[1024];

    szFilePathName[0] = 0;
    SendNppMsg( NPPM_GETFULLCURRENTPATH,
      sizeof(szFilePathName)/sizeof(TCHAR) - 1, (LPARAM) szFilePathName );

    t_string fileName = szFilePathName;

    return fileName;
}

CTagsViewPlugin::file_set CTagsViewPlugin::ewGetOpenedFilePaths() const
{
    file_set openedFiles;

    int nOpenedFiles = (int) SendNppMsg(NPPM_GETNBOPENFILES, 0, ALL_OPEN_FILES);
    if ( nOpenedFiles > 0 )
    {
        const int nMaxPathSize = 512;
        std::vector<TCHAR*> filePaths(nOpenedFiles);
        for ( TCHAR*& p : filePaths )
        {
            p = new TCHAR[nMaxPathSize];
        }

        SendNppMsg(NPPM_GETOPENFILENAMES, (WPARAM) filePaths.data(), nOpenedFiles );

        for ( TCHAR*& p : filePaths )
        {
            openedFiles.insert( t_string(p) );
            delete [] p;
        }
    }

    return openedFiles;
}

int CTagsViewPlugin::ewGetLineFromPos(int pos) const
{
    return (int) ::SendMessage( ewGetEditHwnd(), SCI_LINEFROMPOSITION, pos, 0 );
}

int CTagsViewPlugin::ewGetPosFromLine(int line) const
{
    return (int) ::SendMessage( ewGetEditHwnd(), SCI_POSITIONFROMLINE, line, 0 );
}

int CTagsViewPlugin::ewGetSelectionPos(int& selEnd) const
{
    const HWND hSciEdit = ewGetEditHwnd();
    int selStart = (int) ::SendMessage( hSciEdit, SCI_GETSELECTIONSTART, 0, 0 );
    selEnd = (int) ::SendMessage( hSciEdit, SCI_GETSELECTIONEND, 0, 0 );

    return selStart;
}

/*
CTagsViewPlugin::t_string CTagsViewPlugin::ewGetTextLine(int line) const
{
}
*/

bool CTagsViewPlugin::ewIsFileSaved() const
{
    return (::SendMessage(ewGetEditHwnd(), SCI_GETMODIFY, 0, 0) == 0);
}

bool CTagsViewPlugin::ewGetEditorColors(sEditorColors& colors) const
{
    colors.crTextColor = static_cast<COLORREF>( SendNppMsg(NPPM_GETEDITORDEFAULTFOREGROUNDCOLOR) );
    colors.crBkgndColor = static_cast<COLORREF>( SendNppMsg(NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR) );
    return true;
}

void CTagsViewPlugin::ewCloseTagsView()
{
    OnHideTagsDlg();

    if ( GetTagsDlg().GetHwnd() )
    {
        if ( GetTagsDlg().IsWindowVisible() )
        {
            SendNppMsg( NPPM_DMMHIDE, 0, (LPARAM) GetTagsDlg().GetHwnd() );
            SendNppMsg( NPPM_SETMENUITEMCHECK,
              CNppTagsDlg::FUNC_ARRAY[CNppTagsDlg::EFI_TAGSVIEW]._cmdID, FALSE );
        }
    }

    /*Uninitialize();*/
}

/*HWND CTagsViewPlugin::getCurrentEdit() const
{
    int currentView = 0;
    SendNppMsg( NPPM_GETCURRENTSCINTILLA, 0, (LPARAM) &currentView );
    return ( (currentView == 0) ?
        m_nppData._scintillaMainHandle : m_nppData._scintillaSecondHandle );
}*/

LRESULT CTagsViewPlugin::SendNppMsg(UINT uMsg, WPARAM wParam , LPARAM lParam ) const
{
    return ::SendMessage(m_nppData._nppHandle, uMsg, wParam, lParam);
}

LRESULT CTagsViewPlugin::SendNppMsg(UINT uMsg, WPARAM wParam , LPARAM lParam )
{
    return ::SendMessage(m_nppData._nppHandle, uMsg, wParam, lParam);
}

void CTagsViewPlugin::OnNppReady()
{
    // InitOptions();
    // CreateTagsDlg();

    BOOL bCheck = (m_tagsDlg.GetHwnd() && m_tagsDlg.IsWindowVisible()) ? TRUE : FALSE;
    SendNppMsg( NPPM_SETMENUITEMCHECK,
      CNppTagsDlg::FUNC_ARRAY[CNppTagsDlg::EFI_TAGSVIEW]._cmdID,
      bCheck
    );

#if PLUGIN_WND_PROC
    nppOriginalWndProc = (WNDPROC) SetWindowLongPtrW( 
        m_nppData._nppHandle, GWLP_WNDPROC, 
        (LONG_PTR) nppPluginWndProc );
#endif
}

void CTagsViewPlugin::OnNppShutdown()
{
#if PLUGIN_WND_PROC
    if ( nppOriginalWndProc )
    {
        ::SetWindowLongPtrW(m_nppData._nppHandle, 
            GWLP_WNDPROC, (LONG_PTR) nppOriginalWndProc);
    }
#endif
}

void CTagsViewPlugin::InitOptions()
{
    if ( m_isOptionsInited )
        return;

    TCHAR szConfigPath[2*MAX_PATH];

    szConfigPath[0] = 0;
    SendNppMsg( NPPM_GETPLUGINSCONFIGDIR, sizeof(szConfigPath) - 1, (LPARAM) szConfigPath );

    if ( szConfigPath[0] )
    {
        lstrcat( szConfigPath, _T("\\TagsView.ini") );
        m_optRdWr.SetIniFilePathName(szConfigPath);
    }

    m_tagsDlg.SetOptionsReaderWriter(&m_optRdWr);
    m_tagsDlg.ReadOptions();

    m_isOptionsInited = true;
}

void CTagsViewPlugin::CreateTagsDlg()
{
    if ( m_isTagsDlgCreated )
        return;

    m_tagsDlg.DoModeless();

    m_isTagsDlgCreated = true;
}

void CTagsViewPlugin::DockTagsDlg()
{
    if ( m_isTagsDlgDocked )
        return;

    dockData.hClient = GetTagsDlg().GetHwnd();
    dockData.pszName = szPluginName;
    dockData.dlgID = CNppTagsDlg::EFI_TAGSVIEW;
    dockData.uMask = DWS_DF_CONT_LEFT | DWS_ICONTAB;
    dockData.hIconTab = GetTabIcon();
    dockData.pszAddInfo = NULL;
    dockData.rcFloat.left = 0;
    dockData.rcFloat.top = 0;
    dockData.rcFloat.right = 0;
    dockData.rcFloat.bottom = 0;
    dockData.iPrevCont = -1;
    dockData.pszModuleName = szPluginNameDll;

    SendNppMsg( NPPM_DMMREGASDCKDLG, 0, (LPARAM) &dockData );
    SendNppMsg( NPPM_MODELESSDIALOG, MODELESSDIALOGADD, (LPARAM) dockData.hClient );

    m_isTagsDlgDocked = true;
}

void CTagsViewPlugin::OnNppTBModification()
{
    SendNppMsg( NPPM_ADDTOOLBARICON_FORDARKMODE,
      CNppTagsDlg::FUNC_ARRAY[CNppTagsDlg::EFI_TAGSVIEW]._cmdID,
      (LPARAM) &m_TB_Icon );
}

void CTagsViewPlugin::OnHideTagsDlg()
{
    ewClearNavigationHistory(true);
    clearCurrentFilePathName();
    GetTagsDlg().ClearItems(true);
    GetTagsDlg().ClearCachedTags();
}

void CTagsViewPlugin::Initialize(HINSTANCE hInstance)
{
    TCHAR szCurDir[2*MAX_PATH];

    SetResourceHandle(hInstance);
    m_hTabIcon = (HICON) ::LoadImage( hInstance,
      MAKEINTRESOURCE(IDI_TAGSVIEW), IMAGE_ICON, 0, 0,
      LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT );
    m_TB_Icon.hToolbarBmp = (HBITMAP) ::LoadImage( hInstance,
     MAKEINTRESOURCE(IDB_TAGSVIEW), IMAGE_BITMAP, 0, 0,
     LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS );
    m_TB_Icon.hToolbarIcon = m_hTabIcon;
    m_TB_Icon.hToolbarIconDarkMode = m_hTabIcon;

    int nDirLen = ::GetModuleFileName( (HMODULE) hInstance, szCurDir, sizeof(szCurDir) - 1 );
    while ( nDirLen > 0 )
    {
        --nDirLen;
        if ( (szCurDir[nDirLen] == _T('\\')) || (szCurDir[nDirLen] == _T('/')) )
        {
            break;
        }
    }
    szCurDir[nDirLen] = 0;

    lstrcpy(szPluginNameDll, szCurDir + nDirLen + 1);
    lstrcpy(szPluginName, szPluginNameDll);
    int nNameLen = lstrlen(szPluginName);
    while ( nNameLen > 0 )
    {
        --nNameLen;
        if ( szPluginName[nNameLen] == _T('.') )
        {
            szPluginName[nNameLen] = 0;
            break;
        }
    }

    if ( nDirLen > 0 )
    {
        tString ctagsExePath = szCurDir;
        ctagsExePath += _T("\\TagsView\\ctags.exe");
        m_tagsDlg.SetCTagsExePath(ctagsExePath);
    }
}

void CTagsViewPlugin::Uninitialize()
{
    m_tagsDlg.SaveOptions();
    if ( m_hTabIcon )
    {
        ::DestroyIcon(m_hTabIcon);
        m_hTabIcon = NULL;
        m_TB_Icon.hToolbarIcon = NULL;
        m_TB_Icon.hToolbarIconDarkMode = NULL;
    }
    if ( m_TB_Icon.hToolbarBmp )
    {
        ::DeleteObject(m_TB_Icon.hToolbarBmp);
        m_TB_Icon.hToolbarBmp = NULL;
    }
}

//----------------------------------------------------------------------------


// global vars
CTagsViewPlugin thePlugin;

// static func
void funcTagsView()
{
    bool bParseFile = true;

    thePlugin.InitOptions();
    thePlugin.CreateTagsDlg();

    if ( thePlugin.GetTagsDlg().GetHwnd() )
    {
        UINT uMsg = NPPM_DMMSHOW;
        if ( thePlugin.GetTagsDlg().IsWindowVisible() )
        {
            uMsg = NPPM_DMMHIDE;
            bParseFile = false;
        }
        else
        {
            thePlugin.DockTagsDlg();
        }

        thePlugin.SendNppMsg( uMsg, 0, (LPARAM) dockData.hClient );

        if ( uMsg == NPPM_DMMSHOW )
        {
            thePlugin.GetTagsDlg().ApplyColors();
        }
        else
        {
            thePlugin.OnHideTagsDlg();
        }
    }
    else
    {
        thePlugin.DockTagsDlg();

        thePlugin.SendNppMsg( NPPM_DMMSHOW, 0, (LPARAM) dockData.hClient );
    }

    thePlugin.SendNppMsg( NPPM_SETMENUITEMCHECK,
      CNppTagsDlg::FUNC_ARRAY[CNppTagsDlg::EFI_TAGSVIEW]._cmdID,
      thePlugin.GetTagsDlg().IsWindowVisible()
    );

    if ( bParseFile )
    {
        thePlugin.GetTagsDlg().ReparseCurrentFile();
    }
}

// static func
void funcSettings()
{
    thePlugin.InitOptions();

    CSettingsDlg dlg(thePlugin.GetTagsDlg().GetOptions());
    dlg.DoModal(thePlugin.ewGetMainHwnd());

    thePlugin.GetTagsDlg().OnSettingsChanged();
}

// static func
void funcAbout()
{
    ::MessageBox( 
        thePlugin.ewGetMainHwnd(),
        _T("TagsView ver. 0.5 beta\r\n") \
        _T("(C) Vitaliy Dovgan aka DV, 2022\r\n") \
        _T("(C) Vitaliy Dovgan aka DV, 2009 (original idea)"),
        _T("TagsView plugin for Notepad++"),
        MB_OK
    );
}

#if PLUGIN_WND_PROC
    #define    IDM    40000
    #define    IDM_LANG    (IDM + 6000)
    #define    IDM_LANGSTYLE_CONFIG_DLG    (IDM_LANG + 1)

    // static func
    LRESULT CALLBACK nppPluginWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
    {
        if ( uMessage == WM_COMMAND )
        {
            if ( wParam == IDM_LANGSTYLE_CONFIG_DLG )
            {
                LRESULT nResult = ::CallWindowProcW(nppOriginalWndProc, hWnd, uMessage, wParam, lParam);

                thePlugin.GetTagsDlg().ApplyColors();

                return nResult;
            }
        }

        return ::CallWindowProcW(nppOriginalWndProc, hWnd, uMessage, wParam, lParam);
    }
#endif


extern "C" __declspec(dllexport) void setInfo(NppData nppd)
{
    thePlugin.SetNppData(nppd);
    thePlugin.ewSetMainHwnd(nppd._nppHandle);
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
    return szPluginName;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int * pnbFuncItems)
{
    *pnbFuncItems = CNppTagsDlg::EFI_COUNT;
    return CNppTagsDlg::FUNC_ARRAY;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification * pscn)
{
    //static CTagsViewPlugin::t_string fileToBeClosed;

    if ( pscn->nmhdr.hwndFrom == thePlugin.GetNppData()._nppHandle )
    {
        switch ( pscn->nmhdr.code )
        {
            case NPPN_BUFFERACTIVATED:
                thePlugin.ewOnFileActivated();
                break;

            case NPPN_FILESAVED:
                thePlugin.ewOnFileSaved();
                break;

            case NPPN_FILEOPENED:
                thePlugin.ewOnFileOpened();
                break;

            case NPPN_FILEBEFORECLOSE:
                //fileToBeClosed = thePlugin.ewGetFilePathName();
                break;

            case NPPN_FILECLOSED:
                thePlugin.ewOnFileClosed();
                //fileToBeClosed.clear();
                break;

            case NPPN_TBMODIFICATION:
                thePlugin.OnNppTBModification();
                break;

            case NPPN_READY:
                thePlugin.OnNppReady();
                break;

            case NPPN_SHUTDOWN:
                thePlugin.OnNppShutdown();
                break;
        }
    }
    else
    {
        switch ( pscn->nmhdr.code )
        {
            case SCN_PAINTED:
                thePlugin.ewOnSelectionChanged();
                if ( thePlugin.GetTagsDlg().GetHwnd() &&
                     thePlugin.GetTagsDlg().IsWindowVisible() )
                {
                    thePlugin.GetTagsDlg().ApplyColors();
                }
                break;
        }
    }
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    return 1;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif //UNICODE

extern "C" BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
            thePlugin.Initialize(hInstance);
            break;

        case DLL_PROCESS_DETACH:
            thePlugin.Uninitialize();
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
