#ifndef _PLUGIN_H_
#define _PLUGIN_H_
//---------------------------------------------------------------------------
#include "../TagsViewBase/TagsDlg.h"
#include "../TagsViewBase/EditorWrapper.h"
#include <windows.h>
#include "AkelEdit.h"
#include "AkelDLL.h"
#include <string>

using Win32xx::CWinApp;

class CAkelOptionsReaderWriter : public COptionsReaderWriter
{
public:
    CAkelOptionsReaderWriter();

    void SetMainWnd(HWND hMainWnd);

    virtual bool beginReadOptions();
    virtual void endReadOptions();
    virtual bool readDataOption(COptionsManager::opt_itr itr);
    virtual bool readHexIntOption(COptionsManager::opt_itr itr);
    virtual bool readIntOption(COptionsManager::opt_itr itr);
    virtual bool readStrOption(COptionsManager::opt_itr itr);

    virtual bool beginWriteOptions();
    virtual void endWriteOptions();
    virtual bool writeDataOption(COptionsManager::const_opt_itr itr);
    virtual bool writeHexIntOption(COptionsManager::const_opt_itr itr);
    virtual bool writeIntOption(COptionsManager::const_opt_itr itr);
    virtual bool writeStrOption(COptionsManager::const_opt_itr itr);

protected:
    HANDLE _hOptions;
    HWND   _hMainWnd;

    std::basic_string<TCHAR> getFullOptionName(COptionsManager::const_opt_itr itr);
};

class CAkelTagsDlg : public CTagsDlg
{
    public:
        enum eOptions2 {
            OPT_DOCKRECT = CTagsDlgData::OPT_COUNT,
            OPT_DOCKSIDE
        };

    public:
        CAkelTagsDlg() : CTagsDlg(), m_pDock(NULL)
        {
        }

        void setDock(DOCK* pDock)
        {
            m_pDock = pDock;
        }

    protected:
        virtual void initOptions() override;
        virtual void onMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    protected:
        DOCK* m_pDock;
};

class CTagsViewPlugin : public CWinApp, public CEditorWrapper
{
    public:
        CTagsViewPlugin() : CEditorWrapper(&m_tagsDlg),
          bInitialized(FALSE), bTagsDlgVisible(FALSE), bOldWindows(FALSE),
          bOldRichEdit(FALSE), nMDI(0), bAkelEdit(FALSE),
          pMainProcData(NULL), pEditProcData(NULL), pFrameProcData(NULL),
          m_pDockData(NULL), m_isTagsDlgSubclassed(false)
        {
            m_tagsDlg.SetEditorWrapper(this);
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

        // all opened files 
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

        // navigate backward in current file
        // may be overridden if needed
        virtual void ewDoNavigateBackward() override;

        // navigate forward in current file
        // may be overridden if needed
        virtual void ewDoNavigateForward() override;

        const CTagsDlg& GetTagsDlg() const { return m_tagsDlg; }
        CTagsDlg& GetTagsDlg() { return m_tagsDlg; }

        void Initialize(PLUGINDATA* pd);
        void Uninitialize(bool bDestroyTagsDlg);

        void DockTagsDlg();

    public:
        // public members: ugly, but effective :)
        BOOL         bInitialized;
        BOOL         bTagsDlgVisible;
        BOOL         bOldWindows;
        BOOL         bOldRichEdit; // TRUE means Rich Edit 2.0
        int          nMDI;
        BOOL         bAkelEdit;
        WNDPROCDATA* pMainProcData;
        WNDPROCDATA* pEditProcData;
        WNDPROCDATA* pFrameProcData;

    protected:
        CAkelOptionsReaderWriter m_optRdWr;
        CAkelTagsDlg m_tagsDlg;
        DOCK* m_pDockData;
        bool  m_isTagsDlgSubclassed;
};


//---------------------------------------------------------------------------
#endif
