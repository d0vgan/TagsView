#include "TagsDlgData.h"
#include <algorithm>

using namespace TagsCommon;

using Win32xx::CThreadLock;

CTagsDlgData::CTagsDlgData()
: m_optRdWr(nullptr)
, m_tags(nullptr)
{
}

CTagsDlgData::~CTagsDlgData()
{
}

void CTagsDlgData::InitOptions()
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
    m_opt.AddBool( OPT_VIEW_NESTEDSCOPETREE,  cszView,  _T("NestedScopeTree"),  true     );
    m_opt.AddBool( OPT_VIEW_DBLCLICKTREE,     cszView,  _T("DblClickTree"),     true     );
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

void CTagsDlgData::AddTagsToCache(tags_map&& tags)
{
    CThreadLock lock(m_csTagsMap);

    m_cachedTags.emplace_back(std::move(tags));
    m_tags = &m_cachedTags.back();

    if ( GetOptions().getBool(CTagsDlgData::OPT_CTAGS_SCANFOLDER) && !m_tags->empty() )
    {
        std::list<std::list<tags_map>::iterator> tagsToRemove;

        for ( auto itr = m_cachedTags.begin(); itr != m_cachedTags.end(); ++itr )
        {
            const tags_map& cacheTags = *itr;
            if ( m_tags == &cacheTags )
                continue;

            bool bAllFilesIncluded = true;
            for ( const auto& fileItem : cacheTags )
            {
                if ( m_tags->find(fileItem.first) == m_tags->end() )
                {
                    bAllFilesIncluded = false;
                    break;
                }
            }

            if ( bAllFilesIncluded )
                tagsToRemove.push_back(itr);
        }

        if ( !tagsToRemove.empty() )
        {
            for ( auto& itr : tagsToRemove )
            {
                m_cachedTags.erase(itr);
            }
        }
    }
}

CTagsDlgData::tags_map* CTagsDlgData::GetTagsForFile(const TCHAR* cszFileName)
{
    CThreadLock lock(m_csTagsMap);

    m_tags = getCachedTagsMap(cszFileName); // can be nullptr
    return m_tags;
}

void CTagsDlgData::RemoveTagsForFile(const TCHAR* cszFileName)
{
    CThreadLock lock(m_csTagsMap);

    auto itr = getCachedTagsMapItr(cszFileName);
    m_cachedTags.erase(itr);
    m_tags = nullptr;
}

void CTagsDlgData::RemoveAllTagsFromCache()
{
    CThreadLock lock(m_csTagsMap);

    m_cachedTags.clear();
    m_tags = nullptr;
}

void CTagsDlgData::PurifyTagsInCache(const file_set& openedFiles)
{
    std::list<std::list<tags_map>::const_iterator> itemsToDelete;

    CThreadLock lock(m_csTagsMap);

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
}

std::list<CTagsDlgData::tags_map>::iterator CTagsDlgData::getCachedTagsMapItr(const TCHAR* cszFileName)
{
    return std::find_if(
        m_cachedTags.begin(), m_cachedTags.end(),
        [&cszFileName](const tags_map& tagsMap){ return (tagsMap.find(cszFileName) != tagsMap.end()); }
    );
}

CTagsDlgData::tags_map* CTagsDlgData::getCachedTagsMap(const TCHAR* cszFileName)
{
    auto itrTags = getCachedTagsMapItr(cszFileName);
    return ( (itrTags != m_cachedTags.end()) ? &(*itrTags) : nullptr );
}

std::vector<CTagsDlgData::tags_map::iterator> CTagsDlgData::GetTagsMapItrsByFilePath(const t_string& filePath)
{
    if ( m_tags )
    {
        std::vector<tags_map::iterator> vTagsMapItr;
        t_string filePathNoExt(filePath, 0, getFileExtPos(filePath));

        for ( tags_map::iterator itr = m_tags->begin(); itr != m_tags->end(); ++itr )
        {
            t_string fpathNoExt(itr->first, 0, getFileExtPos(itr->first));
            size_t n = (filePathNoExt.length() < fpathNoExt.length()) ? filePathNoExt.length() : fpathNoExt.length();
            if ( _tcsnicmp(filePathNoExt.c_str(), fpathNoExt.c_str(), n) == 0 )
            {
                vTagsMapItr.push_back(itr);
            }
        }
        return vTagsMapItr;
    }
    return std::vector<tags_map::iterator>();
}

CTagsDlgData::tTagData* CTagsDlgData::GetTagByNameAndScope(const t_string& filePath, const t_string& tagName, const t_string& tagScope)
{
    if ( m_tags )
    {
        std::vector<tags_map::iterator> vTagsMapItr = GetTagsMapItrsByFilePath(filePath);
        for ( tags_map::iterator itrTagsMap : vTagsMapItr )
        {
            file_tags& fileTags = itrTagsMap->second;
            auto itr = std::find_if(fileTags.begin(), fileTags.end(),
                [&tagName, &tagScope](const std::unique_ptr<tTagData>& tag){ return (tag->tagName == tagName && tag->tagScope == tagScope); }
            );
            if ( itr != fileTags.end() )
            {
                tTagData* pTag = itr->get();
                pTag->pFilePath = &itrTagsMap->first;
                return pTag;
            }
        }
    }
    return nullptr;
}

CTagsDlgData::file_tags::iterator CTagsDlgData::GetTagByLine(file_tags& fileTags, const int line)
{
    file_tags::iterator itr = std::upper_bound(
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

        itr = fileTags.end(); // will return end() if itr2 does not succeed

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

CTagsDlgData::file_tags::iterator CTagsDlgData::FindTagByLine(file_tags& fileTags, const int line)
{
    file_tags::iterator itr = GetTagByLine(fileTags, line);

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
