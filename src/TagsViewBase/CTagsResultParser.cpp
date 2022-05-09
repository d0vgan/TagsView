#include "CTagsResultParser.h"
#include <string.h>
#include <algorithm>
#include <vector>
#include <windows.h>

namespace
{
    // searching for c in [s_begin .. s_end)
    inline const char* strchr_pn(const char* s_begin, const char* s_end, char c)
    {
        return std::find(s_begin, s_end, c);
    }

    // searching for sub in [s_begin .. s_end)
    inline const char* strstr_pn(const char* s_begin, const char* s_end, const char* sub)
    {
        return std::search(s_begin, s_end, sub, sub + strlen(sub));
    }

    // replaces all occurrences of `ch_what` with `ch_with`
    inline void string_replace_all(std::string& str, char ch_what, char ch_with)
    {
        std::replace(std::begin(str), std::end(str), ch_what, ch_with);
    }

    // replaces all occurrences of `what` with `with`
    void string_replace_all(std::string& str, const std::string& what, const std::string& with)
    {
        size_t pos = 0;
        while ( (pos = str.find(what, pos)) != std::string::npos )
        {
            str.replace(pos, what.length(), with);
            pos += with.length();
        }
    }

    inline bool is_space_char(char ch)
    {
        switch ( ch )
        {
            case '\t':  // 0x09, tabulation
            case '\n':  // 0x0A, line feed
            case '\v':  // 0x0B, line tabulation
            case '\f':  // 0x0C, form feed
            case '\r':  // 0x0D, carriage return
            case ' ':   // 0x20, space
                return true;
        }
        return false;
    }

    // deletes the leading, trailing and consecutive spaces
    void delete_extra_spaces(std::string& s)
    {
        if ( s.empty() )
            return;

        // 1. condense the spaces
        auto s_begin = std::begin(s);
        auto s_end = std::end(s);
        auto s_last = std::unique( s_begin, s_end, [](char c1, char c2) { return (is_space_char(c1) && is_space_char(c2)); } );
        if ( s_last != s_end )
        {
            s.erase(s_last, s_end);
            if ( s.empty() )
                return;
        }

        // 2. remove the trailing space
        if ( is_space_char(s.back()) )
        {
            s.pop_back();
            if ( s.empty() )
                return;
        }

        // 3. remove the leading space
        if ( is_space_char(s.front()) )
        {
            s.erase(0, 1);
        }
    }

    void preprocess_tag_string(std::string& s)
    {
        static const std::string escaped_backslash = "\\\\";
        static const std::string single_backslash = "\\";

        static const std::string escaped_slash = "\\/";
        static const std::string single_slash = "/";

        delete_extra_spaces(s);
        string_replace_all(s, '\t', ' ');
        string_replace_all(s, escaped_backslash, single_backslash);
        string_replace_all(s, escaped_slash, single_slash);
    }

    CTagsResultParser::t_string to_tstring(const std::string& in, unsigned int nParseFlags, std::vector<TCHAR>& buf)
    {
        if ( in.empty() )
            return CTagsResultParser::t_string();

        UINT uCodePage = (nParseFlags & CTagsResultParser::PF_ISUTF8) ? CP_UTF8 : CP_ACP;
        int nLen = ::MultiByteToWideChar(uCodePage, 0, in.c_str(), static_cast<int>(in.length()), NULL, 0);
        buf.reserve(nLen + 1);
        buf.data()[0] = 0; // just in case
        ::MultiByteToWideChar(uCodePage, 0, in.c_str(), static_cast<int>(in.length()), buf.data(), nLen + 1);
        buf.data()[nLen] = 0; // the trailing '\0'
        return CTagsResultParser::t_string(buf.data(), nLen);
    }
}


// parses result of "ctags --fields=fKnste"
void CTagsResultParser::Parse(const char* s, unsigned int nParseFlags, tParseContext& context)
{
    tags_map& m = context.m;
    std::set<t_string>& relatedFiles = context.relatedFiles;

    for ( auto& fileItem : m )
    {
        for ( tTagData* pTag : fileItem.second )
        {
            delete pTag;
        }
    }
    m.clear();

    if ( !s )
        return;

    enum eParseState {
        EPS_TAGNAME = 0,
        EPS_TAGFILEPATH,
        EPS_TAGPATTERN,
        EPS_TAGTYPE,
        EPS_TAGLINE,
        EPS_TAGENDLINE
    };

    unsigned char uParseState = EPS_TAGNAME;
    const char* p;
    const char* pn; // pos of the nearest '\n'
    tTagData* pTag;
    tTagDataInternal tagData;
    std::vector<TCHAR> buf;
    tags_map::iterator itrFileTags;

    buf.reserve(200);

    while ( *s != 0 )
    {
        switch ( uParseState )
        {
            case EPS_TAGNAME:
                tagData.tagScope.clear();
                tagData.line = -1;
                tagData.end_line = -1;

                pn = strchr(s, '\n');
                if ( pn == NULL )
                {
                    pn = s + strlen(s); // position of the trailing '\0'
                }
                if ( *s == '!' )
                {
                    p = pn; // comment: skip the whole line
                    uParseState = EPS_TAGNAME;
                }
                else
                {
                    p = strchr_pn(s, pn, '\t');
                    if ( p != pn )
                    {
                        tagData.tagName.assign( s, static_cast<size_t>(p - s) );
                        uParseState = EPS_TAGFILEPATH;
                    }
                    else
                    {
                        uParseState = EPS_TAGNAME;
                    }
                }
                break;

            case EPS_TAGFILEPATH:
                p = strchr_pn(s, pn, '\t'); // skip file path
                if ( p != pn )
                {
                    if ( context.inputFileOverride.empty() )
                    {
                        tagData.filePath.assign( s, static_cast<size_t>(p - s) );
                    }
                    uParseState = EPS_TAGPATTERN;
                }
                else
                {
                    uParseState = EPS_TAGNAME;
                }
                break;

            case EPS_TAGPATTERN:
                if ( *s == '/' )
                {
                    ++s;
                    if ( *s == '^' )
                        ++s;
                }
                p = strstr_pn(s, pn, ";\"\t");
                if ( p != pn )
                {
                    const char* pp = p;
                    if ( *(pp - 1) == '/' )
                    {
                        --pp;
                        if ( *(pp - 1) == '$' )
                            --pp;
                    }
                    tagData.tagPattern.assign( s, static_cast<size_t>(pp - s) );

                    p += 2; // pointing to '\t' in ";\"\t"
                    uParseState = EPS_TAGTYPE;
                }
                else
                {
                    uParseState = EPS_TAGNAME;
                }
                break;

            case EPS_TAGTYPE:
                p = strchr_pn(s, pn, '\t');
                if ( p != pn )
                {
                    tagData.tagType.assign( s, static_cast<size_t>(p - s) );
                    uParseState = EPS_TAGLINE;
                }
                else
                {
                    uParseState = EPS_TAGNAME;
                }
                break;

            case EPS_TAGLINE:
                if ( strncmp(s, "line:", 5) == 0 )
                {
                    s += 5; // right after "line:"
                    tagData.line = atoi(s);
                    tagData.end_line = tagData.line;
                    while ( *s >= '0' && *s <= '9' )
                    {
                        ++s; // skip the line number
                    }
                    p = strchr_pn(s, pn, '\t');
                    if ( p == pn )
                    {
                        p = s;
                    }
                    uParseState = EPS_TAGENDLINE;
                }
                else
                {
                    p = pn;
                    uParseState = EPS_TAGNAME;
                }
                break;

            case EPS_TAGENDLINE:
                p = strstr_pn(s - 1, pn, "\tend:");
                if ( p != pn )
                {
                    tagData.end_line = atoi(p + 5);
                }
                if ( (s < p) && 
                     (strncmp(s, "typeref:", 8) != 0) &&
                     (strncmp(s, "file:", 5) != 0) )
                {
                    // additional "scope" tag
                    const char* pp = strchr_pn(s, p, '\t'); // stop at the nearest '\t'
                    const char* sp = strchr_pn(s, pp, ':'); // e.g. exclude "class:" from "class:ClassName"
                    if ( sp == pp )
                    {
                        sp = s;
                    }
                    else
                    {
                        ++sp;
                    }
                    tagData.tagScope.assign( sp, static_cast<size_t>(pp - sp) );
                }

                preprocess_tag_string(tagData.tagName);
                preprocess_tag_string(tagData.tagPattern);
                preprocess_tag_string(tagData.tagScope);
                if ( context.inputFileOverride.empty() )
                {
                    string_replace_all(tagData.filePath, '/', '\\');
                }

                pTag = new tTagData(
                    to_tstring(tagData.tagName, nParseFlags, buf),
                    to_tstring(tagData.tagPattern, nParseFlags, buf),
                    to_tstring(tagData.tagType, nParseFlags, buf),
                    to_tstring(tagData.tagScope, nParseFlags, buf),
                    context.inputFileOverride.empty() ? to_tstring(tagData.filePath, nParseFlags, buf) : context.inputFileOverride,
                    tagData.line,
                    tagData.end_line
                );

                itrFileTags = m.find(pTag->filePath);
                if ( itrFileTags == m.end() )
                {
                    itrFileTags = m.insert( std::make_pair(pTag->filePath, file_tags()) ).first;
                    itrFileTags->second.reserve(256);
                }

                itrFileTags->second.push_back(pTag);
                if ( !pTag->filePath.empty() )
                {
                    relatedFiles.insert(pTag->filePath);
                }

                p = pn;
                uParseState = EPS_TAGNAME;
                break;
        }

        s = p;
        if ( *s != 0 ) // checking for the trailing '\0'
            ++s;
    }

    for ( auto& fileItem : m )
    {
        file_tags& fileTags = fileItem.second;
        std::sort(
            fileTags.begin(),
            fileTags.end(),
            [](const tTagData* pTag1, const tTagData* pTag2){ return (pTag1->line < pTag2->line); }
        );
    }
}
