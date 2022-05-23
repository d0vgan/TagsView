#ifndef _TAGS_DLG_H_
#define _TAGS_DLG_H_
//---------------------------------------------------------------------------
#include "win32++/include/wxx_dialog.h"
#include "win32++/include/wxx_listview.h"
#include "win32++/include/wxx_treeview.h"
#include "win32++/include/wxx_toolbar.h"
#include "win32++/include/wxx_controls.h"
#include "win32++/include/wxx_gdi.h"
#include "win32++/include/wxx_criticalsection.h"
#include <vector>
#include <map>
#include <list>
#include "EditorWrapper.h"
#include "OptionsManager.h"
#include "CTagsResultParser.h"

using Win32xx::CDialog;
using Win32xx::CListView;
using Win32xx::CTreeView;
using Win32xx::CToolBar;
using Win32xx::CToolTip;
using Win32xx::CBrush;
using Win32xx::CCriticalSection;
using Win32xx::tString;

class CEditorWrapper;
class CTagsDlg;

class CTagsDlgChild
{
    public:
        CTagsDlgChild() : m_pDlg(NULL) { }
        void SetTagsDlg(CTagsDlg* pDlg) { m_pDlg = pDlg; }

    protected:
        static tString getTooltip(const CTagsResultParser::tTagData* pTagData);

    protected:
        CTagsDlg* m_pDlg;
};

class CTagsFilterEdit : public CWnd, public CTagsDlgChild
{
    public:
        LRESULT DirectMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            return WndProc(uMsg, wParam, lParam);
        }

    protected:
        virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};

class CTagsListView : public CListView, public CTagsDlgChild
{
    public:
        LRESULT DirectMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            return WndProc(uMsg, wParam, lParam);
        }

    protected:
        virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    protected:
        CPoint  m_lastPoint;
        tString m_lastTooltipText;
};

class CTagsTreeView : public CTreeView, public CTagsDlgChild
{
    public:
        LRESULT DirectMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            return WndProc(uMsg, wParam, lParam);
        }

    protected:
        virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    protected:
        CPoint  m_lastPoint;
        tString m_lastTooltipText;
};

class CTagsDlg : public CDialog
{
    public:
        enum eConsts {
            MAX_TAGNAME = 200
        };

        enum eTagsViewMode {
            TVM_NONE = 0,
            TVM_LIST,
            TVM_TREE
        };

        enum eTagsSortMode {
            TSM_NONE = 0,
            TSM_NAME,
            TSM_TYPE,
            TSM_LINE,
            TSM_FILE
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
            WM_UPDATETAGSVIEW  = (WM_USER + 7010),
            WM_CLOSETAGSVIEW   = (WM_USER + 7020),
            WM_FOCUSTOEDITOR   = (WM_USER + 7023),
            WM_TAGDBLCLICKED   = (WM_USER + 7030),
            WM_CTAGSPATHFAILED = (WM_USER + 7050),
            WM_FILECLOSED      = (WM_USER + 7070)
        };

        enum eDeleteTempFile {
            DTF_NEVERDELETE  = 0,  // don't delete (for debug purpose)
            DTF_ALWAYSDELETE = 1,  // delete once no more needed
            DTF_PRESERVELAST = 2   // preserve the last temp file until the next Parse() or exiting
        };

        enum eOptions {
            OPT_VIEW_MODE = 0,
            OPT_VIEW_SORT,
            OPT_VIEW_WIDTH,
            OPT_VIEW_NAMEWIDTH,
            OPT_VIEW_SHOWTOOLTIPS,
            OPT_VIEW_ESCFOCUSTOEDITOR,
            OPT_VIEW_NESTEDSCOPETREE,

            OPT_COLORS_USEEDITORCOLORS,
            //OPT_COLORS_BKGND,
            //OPT_COLORS_TEXT,
            //OPT_COLORS_TEXTSEL,
            //OPT_COLORS_SELA,
            //OPT_COLORS_SELN,

            OPT_BEHAVIOR_PARSEONSAVE,

            OPT_CTAGS_OUTPUTSTDOUT,
            OPT_CTAGS_SCANFOLDER,
            OPT_CTAGS_SCANFOLDERRECURSIVELY,
            OPT_CTAGS_SKIPFILEEXTS,

            OPT_DEBUG_DELETETEMPINPUTFILE,
            OPT_DEBUG_DELETETEMPOUTPUTFILE,

            OPT_COUNT
        };

        typedef CTagsResultParser::tTagData tTagData;
        typedef CTagsResultParser::file_tags file_tags;
        typedef CTagsResultParser::tags_map tags_map;

        struct tTagsByFile
        {
            tString filePath;
            std::vector<tTagData*> fileTags;
        };

        struct tCTagsThreadParam 
        {
            CTagsDlg* pDlg { nullptr };
            DWORD     dwThreadID { 0 };
            bool      isUTF8{ false };
            tString   cmd_line;
            tString   source_file_name;
            tString   temp_input_file;
            tString   temp_output_file;
        };

        static const TCHAR* cszListViewColumnNames[LVC_TOTAL];

    public:
        CTagsDlg();
        virtual ~CTagsDlg();

        static void DeleteTempFile(const tString& filePath);

        const eTagsSortMode GetSortMode() const { return m_sortMode; }
        const eTagsViewMode GetViewMode() const { return m_viewMode; }
        bool  GoToTag(const tString& filePath, const TCHAR* cszTagName); // not implemented yet
        void  ParseFile(const TCHAR* const cszFileName, bool bReparsePhysicalFile);
        void  ReparseCurrentFile();
        void  SetSortMode(eTagsSortMode sortMode);
        void  SetViewMode(eTagsViewMode viewMode, eTagsSortMode sortMode);
        void  UpdateTagsView();
        void  UpdateCurrentItem(); // set current item according to caret pos
        void  UpdateNavigationButtons();

        void  CloseDlg()
        {
            if ( GetHwnd() && IsWindow() )
                EndDialog(0);
        }

        // MUST be called manually to read the options
        void  ReadOptions()  { initOptions(); m_opt.ReadOptions(m_optRdWr); }

        // MUST be called manually to save the options
        void  SaveOptions()  { m_opt.SaveOptions(m_optRdWr); }

        // MUST be called manually to set required options reader-writer
        void  SetOptionsReaderWriter(COptionsReaderWriter* optRdWr) { m_optRdWr = optRdWr; }

        // MUST be called manually to set required editor wrapper
        void  SetEditorWrapper(CEditorWrapper* pEdWr) { m_pEdWr = pEdWr; }

        // MUST be called manually to set required path to ctags.exe
        void  SetCTagsExePath(const tString& ctagsPath = _T("TagsView\\ctags.exe")) { m_ctagsExeFilePath = ctagsPath; }

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

        static eTagsViewMode toViewMode(int viewMode)
        {
            switch ( viewMode )
            {
                case TVM_TREE:
                    return TVM_TREE;
            }
            return TVM_LIST;
        }

        static eTagsSortMode toSortMode(int sortMode)
        {
            switch ( sortMode )
            {
                case TSM_NAME:
                    return TSM_NAME;
                case TSM_TYPE:
                    return TSM_TYPE;
            }
            return TSM_LINE;
        }

        COptionsManager& GetOptions() { return m_opt; }

        void ApplyColors();
        void ClearItems(bool bDelayedRedraw = false);
        void ClearCachedTags();
        void PurifyCachedTags();

        void OnTreeCopyItemToClipboard();
        void OnTreeCopyItemAndChildrenToClipboard();
        void OnTreeCopyAllItemsToClipboard();
        void OnTreeExpandChildNodes();
        void OnTreeCollapseChildNodes();
        void OnTreeExpandAllNodes();
        void OnTreeCollapseAllNodes();
        void OnListCopyItemToClipboard();
        void OnListCopyAllItemsToClipboard();
        void OnShowSettings();
        void OnSettingsChanged();
        void OnTagDblClicked(const tTagData* pTagData);

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

        void deleteAllItems(bool bDelayedRedraw);
        int addListViewItem(int nItem, const tTagData* pTag);
        HTREEITEM addTreeViewItem(HTREEITEM hParent, const tString& tagName, tTagData* pTag);
        bool isTagMatchFilter(const tString& tagName) const;
        size_t getNumTotalItemsForSorting() const;
        size_t getNumItemsForSorting(const CTagsDlg::file_tags& fileTags) const;
        void sortTagsByLineLV();
        void sortTagsByNameOrTypeLV(eTagsSortMode sortMode);
        void sortTagsTV(eTagsSortMode sortMode);
        void addFileTagsToTV(tTagsByFile& tagsByFile);
        tString getItemTextLV(int iItem) const;
        tString getAllItemsTextLV() const;
        tString getItemTextTV(HTREEITEM hItem) const;
        tString getItemAndChildrenTextTV(HTREEITEM hItem, const tString& indent = tString()) const;
        tString getAllItemsTextTV() const;

        void checkCTagsExePath();

        CTagsResultParser::file_tags::iterator getTagByLine(CTagsResultParser::file_tags& fileTags, const int line);
        CTagsResultParser::file_tags::iterator findTagByLine(CTagsResultParser::file_tags& fileTags, const int line);
        CTagsResultParser::file_tags::iterator getTagByName(CTagsResultParser::file_tags& fileTags, const tString& tagName);
        std::list<CTagsResultParser::tags_map>::iterator getCachedTagsMapItr(const TCHAR* cszFileName, bool bAddIfNotExist, bool& bJustAdded);
        CTagsResultParser::tags_map* getCachedTagsMap(const TCHAR* cszFileName, bool bAddIfNotExist, bool& bJustAdded);

        virtual void initOptions();

        static DWORD WINAPI CTagsDlg::CTagsThreadProc(LPVOID lpParam);

    protected:
        friend class CTagsFilterEdit;
        friend class CTagsListView;
        friend class CTagsTreeView;

        eTagsViewMode   m_viewMode;
        eTagsSortMode   m_sortMode;
        DWORD           m_dwLastTagsThreadID;
        HANDLE          m_hTagsThreadEvent;
        CCriticalSection m_csTagsItems;
        CImageList      m_tbImageList;
        CToolBar        m_tbButtons;
        CTagsFilterEdit m_edFilter;
        CTagsListView   m_lvTags;
        CTagsTreeView   m_tvTags;
        CToolTip        m_ttHints;
        tString         m_tagFilter;
        tString         m_ctagsExeFilePath;
        tString         m_ctagsTempInputFilePath;
        tString         m_ctagsTempOutputFilePath;
        tags_map*       m_tags;
        std::list<tags_map> m_cachedTags;
        COptionsManager m_opt;
        COptionsReaderWriter* m_optRdWr;
        CEditorWrapper* m_pEdWr;
        HMENU           m_hMenu;
        COLORREF        m_crTextColor;
        COLORREF        m_crBkgndColor;
        HBRUSH          m_hBkgndBrush;
        int             m_prevSelStart;
        bool            m_isUpdatingSelToItem;
        volatile LONG m_nTagsThreadCount;
        std::map<int, Win32xx::CString> m_DispInfoText;
};

//---------------------------------------------------------------------------
#endif
