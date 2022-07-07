#ifndef _TAGS_DLG_H_
#define _TAGS_DLG_H_
//---------------------------------------------------------------------------
#include "win32++/include/wxx_setup.h"
#include "win32++/include/wxx_dialog.h"
#include "win32++/include/wxx_toolbar.h"
#include "win32++/include/wxx_controls.h"
#include "win32++/include/wxx_gdi.h"
#include "win32++/include/wxx_criticalsection.h"
#include <vector>
#include <map>
#include <set>
#include "TagsDlgData.h"
#include "TagsCommon.h"
#include "CTagsResultParser.h"
#include "TagsFilterEdit.h"
#include "TagsListView.h"
#include "TagsTreeView.h"
#include "TagsPopupListBox.h"

using Win32xx::CDialog;
using Win32xx::CToolBar;
using Win32xx::CToolTip;
using Win32xx::CBrush;
using Win32xx::CCriticalSection;

class CEditorWrapper;

class CTagsDlg : public CDialog
{
    public:
        enum eConsts {
            MAX_TAGNAME = 200
        };

        enum eListViewColumns {
            LVC_NAME = 0,
            LVC_TYPE,
            LVC_LINE,
            LVC_FILE,

            LVC_TOTAL
        };

        enum eTagsViewFocus {
            TVF_FILTEREDIT = 0,
            TVF_TAGSVIEW
        };

        enum eMsg {
            WM_UPDATETAGSVIEW   = (WM_USER + 7010),
            WM_CLOSETAGSVIEW    = (WM_USER + 7020),
            WM_FOCUSTOEDITOR    = (WM_USER + 7023),
            WM_TAGDBLCLICKED    = (WM_USER + 7030),
            WM_CTAGSPATHFAILED  = (WM_USER + 7050),
            WM_FILECLOSED       = (WM_USER + 7070),
            WM_CLEARCACHEDTAGS  = (WM_USER + 7101),
            WM_PURIFYCACHEDTAGS = (WM_USER + 7102)
        };

        typedef TagsCommon::t_string t_string;
        typedef TagsCommon::tTagData tTagData;
        typedef TagsCommon::tstring_cmp_less tstring_cmp_less;
        typedef CTagsResultParser::file_tags file_tags;
        typedef CTagsResultParser::tags_map tags_map;

        struct tTagsByFile
        {
            t_string filePath;
            std::vector<tTagData*> fileTags;
        };

        struct tCTagsThreadParam 
        {
            CTagsDlg* pDlg { nullptr };
            DWORD     dwThreadID { 0 };
            bool      isUTF8{ false };
            t_string  cmd_line;
            t_string  source_file_name;
            t_string  temp_input_file;
            t_string  temp_output_file;
        };

        static const TCHAR* cszListViewColumnNames[LVC_TOTAL];

    public:
        CTagsDlg();
        virtual ~CTagsDlg();

        static void DeleteTempFile(const t_string& filePath);

        const CTagsDlgData::eTagsSortMode GetSortMode() const { return m_sortMode; }
        const CTagsDlgData::eTagsViewMode GetViewMode() const { return m_viewMode; }
        bool  GoToTag(const t_string& filePath, const t_string& tagName);
        void  ParseFile(const TCHAR* const cszFileName, bool bReparsePhysicalFile);
        void  ReparseCurrentFile();
        void  SetSortMode(CTagsDlgData::eTagsSortMode sortMode);
        void  SetViewMode(CTagsDlgData::eTagsViewMode viewMode, CTagsDlgData::eTagsSortMode sortMode);
        void  UpdateTagsView();
        void  UpdateCurrentItem(); // set current item according to caret pos
        void  UpdateNavigationButtons();

        void  CloseDlg()
        {
            if ( GetHwnd() && IsWindow() )
                EndDialog(0);
        }

        COptionsManager& GetOptions() { return m_Data.GetOptions(); }

        // MUST be called manually to read the options
        void  ReadOptions(COptionsReaderWriter* optRdWr)
        {
            initOptions();
            m_Data.SetOptionsReaderWriter(optRdWr);
            m_Data.ReadOptions();
        }

        // MUST be called manually to save the options
        void  SaveOptions()  { m_Data.SaveOptions(); }

        // MUST be called manually to set required editor wrapper
        void  SetEditorWrapper(CEditorWrapper* pEdWr) { m_pEdWr = pEdWr; }

        // MUST be called manually to set required path to ctags.exe
        void  SetCTagsExePath(const t_string& ctagsPath = _T("TagsView\\ctags.exe")) { m_ctagsExeFilePath = ctagsPath; }

        LPCTSTR GetEditorShortName() const;

        void  SetFocusTo(eTagsViewFocus tvf = TVF_TAGSVIEW)
        {
            switch ( tvf )
            {
                case TVF_FILTEREDIT:
                    m_edFilter.SetFocus();
                    break;

                case TVF_TAGSVIEW:
                    if ( m_lvTags.IsWindowVisible() )
                        m_lvTags.SetFocus();
                    else if ( m_tvTags.IsWindowVisible() )
                        m_tvTags.SetFocus();
                    else
                        this->SetFocus();
                    break;
            }
        }

        static CTagsDlgData::eTagsViewMode toViewMode(int viewMode)
        {
            switch ( viewMode )
            {
                case CTagsDlgData::TVM_TREE:
                    return CTagsDlgData::TVM_TREE;
            }
            return CTagsDlgData::TVM_LIST;
        }

        static CTagsDlgData::eTagsSortMode toSortMode(int sortMode)
        {
            switch ( sortMode )
            {
                case CTagsDlgData::TSM_NAME:
                    return CTagsDlgData::TSM_NAME;
                case CTagsDlgData::TSM_TYPE:
                    return CTagsDlgData::TSM_TYPE;
                case CTagsDlgData::TSM_FILE:
                    return CTagsDlgData::TSM_FILE;
            }
            return CTagsDlgData::TSM_LINE;
        }

        void ApplyColors();
        void ClearItems(bool bDelayedRedraw = false);
        void ClearCachedTags();
        void PurifyCachedTags();

        void OnTreeCopyItemToClipboard();
        void OnTreeCopyItemAndChildrenToClipboard();
        void OnTreeCopyAllItemsToClipboard();
        void OnTreeExpandChildNodes();
        void OnTreeCollapseChildNodes();
        void OnTreeCollapseParentNode();
        void OnTreeExpandAllNodes();
        void OnTreeCollapseAllNodes();
        void OnListCopyItemToClipboard();
        void OnListCopyAllItemsToClipboard();
        void OnShowSettings();
        void OnSettingsChanged();
        void OnTagDblClicked(const tTagData* pTag);

        LRESULT ProcessPopupListBoxNotifications(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL* pIsProcessed);

    protected:
        virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
        virtual void EndDialog(INT_PTR nResult) override;
        virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;
        virtual void OnCancel() override;
        virtual void OnOK() override;
        virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam) override;

        void OnAddTags(const char* s, const tCTagsThreadParam* tt);
        void OnFileClosed();
        BOOL OnInitDialog();
        void OnSize(bool bInitial = false);
        INT_PTR OnCtlColorEdit(WPARAM wParam, LPARAM lParam);

        void OnPrevPosClicked();
        void OnNextPosClicked();
        void OnParseClicked();

        void createTooltips();
        void destroyTooltips();

        virtual void initOptions()  { m_Data.InitOptions(); }
        virtual void onMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) { }

        void deleteAllItems(bool bDelayedRedraw);
        int addListViewItem(int nItem, const tTagData* pTag);
        HTREEITEM addTreeViewItem(HTREEITEM hParent, const t_string& tagName, tTagData* pTag);
        bool isTagMatchFilter(const t_string& tagName) const;
        size_t getNumTotalItemsForSorting() const;
        size_t getNumItemsForSorting(const CTagsDlg::file_tags& fileTags) const;
        void sortTagsByLineLV();
        void sortTagsByNameOrTypeLV(CTagsDlgData::eTagsSortMode sortMode);
        void sortTagsTV(CTagsDlgData::eTagsSortMode sortMode);
        void addFileTagsToTV(tTagsByFile& tagsByFile);
        t_string getItemTextLV(int iItem) const;
        t_string getAllItemsTextLV() const;
        t_string getItemTextTV(HTREEITEM hItem) const;
        t_string getItemAndChildrenTextTV(HTREEITEM hItem, const t_string& indent = t_string()) const;
        t_string getAllItemsTextTV() const;

        enum eTreeNodeAction {
            TNA_EXPANDCHILDREN,
            TNA_COLLAPSECHILDREN
        };
        void applyActionToItemTV(HTREEITEM hItem, eTreeNodeAction action);

        void checkCTagsExePath();

        bool addCTagsThreadForFile(const t_string& filePath);
        void removeCTagsThreadForFile(const t_string& filePath);
        bool hasAnyCTagsThread() const;

        static DWORD WINAPI CTagsThreadProc(LPVOID lpParam);

    protected:
        friend class CTagsFilterEdit;
        friend class CTagsListView;
        friend class CTagsTreeView;
        friend class CTagsPopupListBox;

        CTagsDlgData::eTagsViewMode   m_viewMode;
        CTagsDlgData::eTagsSortMode   m_sortMode;
        DWORD           m_dwLastTagsThreadID;
        HANDLE          m_hTagsThreadEvent;
        CCriticalSection m_csTagsItemsUI;
        CCriticalSection m_csCTagsThreads;
        CImageList      m_tbImageList;
#ifdef _AKEL_TAGSVIEW
        CWnd            m_stTitle;
        CWnd            m_btClose;
#endif
        CToolBar        m_tbButtons;
        CTagsFilterEdit m_edFilter;
        CTagsListView   m_lvTags;
        CTagsTreeView   m_tvTags;
        CTagsPopupListBox m_lbPopup;
        CToolTip        m_ttHints;
        t_string        m_tagFilter;
        t_string        m_ctagsExeFilePath;
        t_string        m_ctagsTempInputFilePath;
        t_string        m_ctagsTempOutputFilePath;
        CTagsDlgData    m_Data;
        CEditorWrapper* m_pEdWr;
        HMENU           m_hMenu;
        COLORREF        m_crTextColor;
        COLORREF        m_crBkgndColor;
        HBRUSH          m_hBkgndBrush;
        INT_PTR         m_prevSelStart;
        bool            m_isUpdatingSelToItem;
        unsigned int    m_nThreadMsg; // access it under m_csCTagsThreads!
        volatile mutable LONG m_nTagsThreadCount;
        std::set<t_string, tstring_cmp_less> m_ctagsThreads; // access it under m_csCTagsThreads!
        std::map<int, Win32xx::CString> m_DispInfoText;
};

//---------------------------------------------------------------------------
#endif
