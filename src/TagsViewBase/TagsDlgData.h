#ifndef _TAGS_DLG_DATA_H_
#define _TAGS_DLG_DATA_H_
//---------------------------------------------------------------------------
#include "win32++/include/wxx_setup.h"
#include "win32++/include/wxx_criticalsection.h"
#include <vector>
#include <map>
#include <set>
#include <list>
#include "OptionsManager.h"
#include "TagsCommon.h"
#include "CTagsResultParser.h"
#include "EditorWrapper.h"


class CTagsDlgData
{
    public:
        /*
        enum eConsts {
            MAX_TAGNAME = 200
        };
        */

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
            OPT_VIEW_NESTEDSCOPETREE,
            OPT_VIEW_DBLCLICKTREE,
            OPT_VIEW_ESCFOCUSTOEDITOR,

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

        typedef TagsCommon::t_string t_string;
        typedef TagsCommon::tTagData tTagData;
        typedef TagsCommon::tstring_cmp_less tstring_cmp_less;
        typedef CTagsResultParser::file_tags file_tags;
        typedef CTagsResultParser::tags_map tags_map;
        typedef IEditorWrapper::file_set file_set;

    public:
        CTagsDlgData();
        ~CTagsDlgData();

        void  SetOptionsReaderWriter(COptionsReaderWriter* optRdWr) { m_optRdWr = optRdWr; }
        void  InitOptions();
        void  ReadOptions()  { m_opt.ReadOptions(m_optRdWr); }
        void  SaveOptions()  { m_opt.SaveOptions(m_optRdWr); }
        COptionsManager& GetOptions() { return m_opt; }

        const tags_map* GetTags() const { return m_tags; }
        tags_map* GetTags() { return m_tags; }
        void SetTags(tags_map* pTags) { m_tags = pTags; }
        bool IsTagsEmpty() const { return (m_tags == nullptr || m_tags->empty()); }
        file_tags* GetFileTags(const t_string& filePath); // can return nullptr

        void AddTagsToCache(tags_map&& tags);
        tags_map* GetTagsFromCache(const TCHAR* cszFileName); // sets m_tags, can return nullptr
        void RemoveTagsFromCache(const TCHAR* cszFileName);
        void RemoveAllTagsFromCache();
        void RemoveOutdatedTagsFromCache(const file_set& openedFiles);

        tTagData* FindTagByNameAndScope(const t_string& filePath, const t_string& tagName, const t_string& tagScope); // can return nullptr
        tTagData* FindTagByLine(file_tags& fileTags, const int line); // can return nullptr

    protected:
        std::list<tags_map>::iterator getCachedTagsMapItr(const TCHAR* cszFileName); // call it under m_csTagsMap!
        tags_map* getCachedTagsMap(const TCHAR* cszFileName); // call it under m_csTagsMap!
        file_tags::iterator getTagByLine(file_tags& fileTags, const int line);
        std::vector<tags_map::iterator> getTagsMapItrsByFilePath(const t_string& filePath);

    protected:
        Win32xx::CCriticalSection m_csTagsMap;
        COptionsManager       m_opt;
        COptionsReaderWriter* m_optRdWr;
        tags_map*             m_tags;
        std::list<tags_map>   m_cachedTags; // access it under m_csTagsMap!
};

//---------------------------------------------------------------------------
#endif
