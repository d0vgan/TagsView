#include "CTagsResultParser.h"
#include <string.h>

namespace
{
    // searching for c in [s .. pn)
    const char* strchr_pn(const char* s, char c, const char* pn)
    {
        while ( s < pn && *s != c )
        {
            ++s;
        }
        return s;
    }

    // searching for sub in [s .. pn)
    const char* strstr_pn(const char* s, const char* sub, const char* pn)
    {
        int n = strlen(sub);
        const char* sn = pn - n;
        while ( s < sn && strncmp(s, sub, n) != 0 )
        {
            ++s;
        }
        if ( s >= sn )
        {
            s = pn;
        }
        return s;
    }
}

void CTagsResultParser::DelDupSpaces(string& s)
{
    string::size_type pos = 0;
    string::size_type n = 0;

    while ( pos < s.length() )
    {
        if ( s[pos] == ' ' )
        {
            ++n;
        }
        else
        {
            if ( n > 1 )
            {
                --n;
                s.erase(pos - n, n);
                pos -= n;
            }
            n = 0;
        }
        ++pos;
    }

    if ( n > 1 )
    {
        --n;
        s.erase(pos - n, n);
    }
}

// parses result of "ctags --fields=fKnste"
void CTagsResultParser::Parse(const char* s, tags_map& m)
{
    m.clear();

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

    while ( s != NULL && *s != 0 )
    {
        switch ( uParseState )
        {
            case EPS_TAGNAME:
                tagData.tagScope.clear();
                tagData.line = -1;
                tagData.end_line = -1;
                tagData.data.p = (void *) 0;
                tagData.pTagData = (void *) 0;

                pn = strchr(s, '\n');
                if ( pn == NULL )
                {
                    pn = s + strlen(s) + 1;
                }
                if ( *s == '!' )
                {
                    p = pn; // comment: skip it
                    uParseState = EPS_TAGNAME;
                }
                else
                {
                    p = strchr_pn(s, '\t', pn);
                    if ( p < pn )
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
                p = strchr_pn(s, '\t', pn); // skip file path
                if ( p < pn )
                {
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
                p = strstr_pn(s, ";\"\t", pn);
                if ( p < pn )
                {
                    const char* pp = p;
                    if ( *(pp - 1) == '/' )
                    {
                        --pp;
                        if ( *(pp - 1) == '$' )
                            --pp;
                    }
                    tagData.tagPattern.assign( s, static_cast<size_t>(pp - s) );

                    p += 2; // '\t' in ";\"\t"
                    uParseState = EPS_TAGTYPE;
                }
                else
                {
                    uParseState = EPS_TAGNAME;
                }
                break;

            case EPS_TAGTYPE:
                p = strchr_pn(s, '\t', pn);
                if ( p < pn )
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
                    tagData.line = atoi(s + 5);
                    tagData.end_line = tagData.line;
                    p = strchr_pn(s + 5, '\t', pn);
                    if ( p == pn )
                    {
                        p = s + 5;
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
                p = strstr_pn(s - 1, "\tend:", pn);
                if ( p < pn )
                {
                    tagData.end_line = atoi(p + 5);

                    if ( (s < p) && 
                         (strncmp(s, "typeref:", 8) != 0) &&
                         (strncmp(s, "file:", 8) != 0) )
                    {
                        // additional "scope" tag
                        const char* pp = strchr_pn(s, '\t', p); // stop at the nearest '\t'
                        const char* sp = strchr_pn(s, ':', pp); // e.g. exclude "class:" from "class:ClassName"
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
                }

                DelDupSpaces(tagData.tagName);
                DelDupSpaces(tagData.tagPattern);

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
        if ( s != NULL )
            ++s;
    }
}

char* CTagsResultParser::newUnicodeToPseudoChar(const wchar_t* ws, int wlen , int* plen )
{
    if ( wlen < 0 )
    {
        wlen = ws ? static_cast<int>(wcslen(ws)) : 0;
    }

    if ( wlen > 0 )
    {
        char* s = new char[2*wlen + 1];
        if ( s )
        {
            int len = 0;
            for ( int i = 0; i < wlen; i++ )
            {
                const wchar_t wch = ws[i] & 0xFF00;
                if ( wch != 0 )
                {
                    s[len++] = (char) ((wch >> 8) & 0x00FF);
                }
                s[len++] = (char) (ws[i] & 0x00FF);
            }
            s[len] = 0;
            if ( plen )  *plen = len;
            return s;
        }
    }

    if ( plen )  *plen = 0;
    return NULL;
}
