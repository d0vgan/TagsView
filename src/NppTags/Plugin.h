#ifndef _PLUGIN_H_
#define _PLUGIN_H_
//---------------------------------------------------------------------------
#include "../TagsViewBase/EditorWrapper.h"
#include "../TagsViewBase/TagsDlg.h"
#include <windows.h>
#include "PluginInterface.h"
#include "Notepad_plus_msgs.h"
#include "Docking.h"

using Win32xx::CWinApp;

class CNppTagsDlg : public CTagsDlg
{
    public:
        enum eFuncItem {
            EFI_TAGSVIEW = 0,
            EFI_SETTINGS,
            EFI_SEPARATOR1,
            EFI_ABOUT,

            EFI_COUNT
        };

        static FuncItem FUNC_ARRAY[EFI_COUNT];

    public:
        CNppTagsDlg() : m_hNppWnd(NULL)
        {
        }

        void SetNppWnd(HWND hNppWnd)
        {
            m_hNppWnd = hNppWnd;
        }

    protected:
        virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            if ( uMsg == WM_NOTIFY )
            {
                NMHDR* pnmh = (NMHDR*) lParam;
                if ( pnmh->hwndFrom == m_hNppWnd )
                {
                    if ( LOWORD(pnmh->code) == DMN_CLOSE )
                    {
                        ::SendMessage( m_hNppWnd, NPPM_SETMENUITEMCHECK, FUNC_ARRAY[EFI_TAGSVIEW]._cmdID, FALSE );
                        if ( m_pEdWr )
                        {
                            m_pEdWr->ewClearNavigationHistory(true);
                        }
                        ClearCachedTags();
                        return 0;
                    }
                }
            }

            return CTagsDlg::DialogProc(uMsg, wParam, lParam);
        }

    protected:
        HWND m_hNppWnd;
};

class CTagsViewPlugin : public CWinApp, public CEditorWrapper
{
    public:
        CTagsViewPlugin() : CEditorWrapper(&m_tagsDlg)
        {
            m_tagsDlg.SetEditorWrapper(this);
            m_hTabIcon = NULL;
            m_TB_Icon.hToolbarBmp = NULL;
            m_TB_Icon.hToolbarIcon = NULL;
            m_TB_Icon.hToolbarIconDarkMode = NULL;
            m_isOptionsInited = false;
            m_isTagsDlgCreated = false;
            m_isTagsDlgDocked = false;
        }

        // the editor name, e.g. "AkelPad" or "Notepad++"
        virtual LPCTSTR ewGetEditorName() const override;

        // the editor short name, e.g. "akel" or "npp"
        virtual LPCTSTR ewGetEditorShortName() const override;

        // open a file
        virtual bool ewDoOpenFile(LPCTSTR pszFileName) override;

        // save current file
        virtual void ewDoSaveFile() override;

        // set focus to the editor's window
        virtual void ewDoSetFocus() override;

        // set selection in current file
        virtual void ewDoSetSelection(int selStart, int selEnd) override;

        // current edit window
        virtual HWND ewGetEditHwnd() const override;

        // current file pathname (e.g. "C:\My Project\File Name.cpp")
        virtual t_string ewGetFilePathName() const override;

        virtual file_set ewGetOpenedFilePaths() const override;

        virtual int ewGetLineFromPos(int pos) const override; // 0-based

        virtual int ewGetPosFromLine(int line) const override; // 0-based

        virtual int ewGetSelectionPos(int& selEnd) const override;

        //virtual t_string ewGetTextLine(int line) const override;

        // true when current file is saved (unmodified)
        virtual bool ewIsFileSaved() const override;

        // editor colors
        virtual bool ewGetEditorColors(sEditorColors& colors) const override;

        // close TagsView dialog
        virtual void ewCloseTagsView() override;

        const CTagsDlg& GetTagsDlg() const { return m_tagsDlg; }
        CTagsDlg& GetTagsDlg() { return m_tagsDlg; }

        const NppData& GetNppData() const { return m_nppData; }

        void SetNppData(const NppData& nppd)
        {
            m_nppData = nppd;
            m_tagsDlg.SetNppWnd(nppd._nppHandle);
        }

        void OnNppReady();
        void OnNppShutdown();
        void OnNppTBModification();

        void OnHideTagsDlg();

        void Initialize(HINSTANCE hInstance);
        void Uninitialize();

    public:
        LRESULT SendNppMsg(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0) const;
        LRESULT SendNppMsg(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);

        HICON GetTabIcon() const { return m_hTabIcon; }

        void InitOptions();
        void CreateTagsDlg();
        void DockTagsDlg();


    protected:
        CIniOptionsReaderWriter m_optRdWr;
        CNppTagsDlg  m_tagsDlg;
        NppData      m_nppData;
        HICON        m_hTabIcon;
        toolbarIconsWithDarkMode m_TB_Icon;
        bool         m_isOptionsInited;
        bool         m_isTagsDlgCreated;
        bool         m_isTagsDlgDocked;
};

//---------------------------------------------------------------------------
#endif
