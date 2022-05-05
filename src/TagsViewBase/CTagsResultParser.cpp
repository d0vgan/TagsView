#include "CTagsResultParser.h"
#include <string.h>
#include <algorithm>
#include <cctype>

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

    // deletes the leading, trailing and consecutive spaces
    void delete_extra_spaces(std::string& s)
    {
        if ( s.empty() )
            return;

        // 1. condense the spaces
        auto s_begin = std::begin(s);
        auto s_end = std::end(s);
        auto s_last = std::unique( s_begin, s_end, [](char c1, char c2) { return (std::isspace(c1) && std::isspace(c2)); } );
        if ( s_last != s_end )
        {
            s.erase(s_last, s_end);
            if ( s.empty() )
                return;
        }

        // 2. remove the trailing space
        if ( std::isspace(s.back()) )
        {
            s.pop_back();
            if ( s.empty() )
                return;
        }

        // 3. remove the leading space
        if ( std::isspace(s.front()) )
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
}


// parses result of "ctags --fields=fKnste"
void CTagsResultParser::Parse(const char* s, tags_map& m, unsigned int nParseFlags)
{
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
    tTagData tagData;
    tags_map::iterator itr;

    while ( *s != 0 )
    {
        switch ( uParseState )
        {
            case EPS_TAGNAME:
                tagData.tagScope.clear();
                tagData.line = -1;
                tagData.end_line = -1;
                tagData.data.p = nullptr;
                tagData.pTagData = nullptr;

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
                    if ( nParseFlags & PF_INCLUDEFILEPATH )
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
                if ( nParseFlags & PF_INCLUDEFILEPATH )
                {
                    string_replace_all(tagData.filePath, '/', '\\');
                }

                itr = m.insert( std::make_pair(tagData.line, tagData) );
                if ( itr != m.end() )
                {
                    itr->second.pTagData = (void *) &(itr->second);
                }

                p = pn;
                uParseState = EPS_TAGNAME;
                break;
        }

        s = p;
        if ( *s != 0 ) // checking for the trailing '\0'
            ++s;
    }
}
