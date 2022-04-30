#include "CTagsResultParser.h"
#include <string.h>

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

// parses result of "ctags --fields=fKnst"
void CTagsResultParser::Parse(const string& s, multimap<int, tTagData>& m)
{
    enum eParsedState {
        EPS_NEWLINE = 0,
        EPS_TAGNAME,
        EPS_TAGPATTERN,
        EPS_TAGTYPE,
        EPS_TAGLINE,

        EPS_ALL = EPS_TAGLINE
    };

    tTagData tagData;
    unsigned char uParsedState = EPS_NEWLINE;
    string::size_type pos = 0;
    string::size_type pos0 = 0;
    string::size_type pos1 = 0;
    multimap<int, tTagData>::iterator itr;

    m.clear();

    while ( pos != string::npos )
    {
        pos0 = pos;

        switch ( uParsedState )
        {
            case EPS_NEWLINE:
                tagData.data.p = (void *) 0;
                tagData.pTagData = (void *) 0;
                tagData.tagScope.clear();
                if ( s[pos] == '!' )
                {
                    // comment: skip it
                    ++pos;
                    pos = s.find( '\n', pos );
                }
                else
                {
                    pos = s.find( '\t', pos );
                    if ( pos != string::npos )
                    {
                        tagData.tagName.assign( s.c_str() + pos0, pos - pos0 );
                        ++pos;
                        uParsedState = EPS_TAGNAME;
                    }
                }
                break;

            case EPS_TAGNAME:
                pos = s.find( '\t', pos );
                if ( pos != string::npos )
                {
                    ++pos; // skip position of File Name
                    pos0 = pos;
                    pos = s.find( ";\"\t", pos );
                    if ( pos != string::npos )
                    {
                        if ( s[pos0] == '/' )
                        {
                            ++pos0;
                            if ( s[pos0] == '^' )
                                ++pos0;
                        }
                        tagData.tagPattern.assign( s.c_str() + pos0, pos - pos0 );
                        string::size_type uLen = tagData.tagPattern.length();
                        if ( uLen >= 0 )
                        {
                            if ( tagData.tagPattern[uLen - 1] == '/' )
                            {
                                if ( (uLen > 1) && (tagData.tagPattern[uLen - 2] == '$') )
                                {
                                    tagData.tagPattern.erase(uLen - 2, 2);
                                }
                                else
                                    tagData.tagPattern.erase(uLen - 1, 1);
                            }
                        }
                        pos += 3; // ";\"\t"
                        uParsedState = EPS_TAGPATTERN;
                    }
                }
                break;

            case EPS_TAGPATTERN:
                pos = s.find( '\t', pos );
                if ( pos != string::npos )
                {
                    tagData.tagType.assign( s.c_str() + pos0, pos - pos0 );
                    ++pos;
                    uParsedState = EPS_TAGTYPE;
                }
                break;

            case EPS_TAGTYPE:
                pos = s.find( "line:", pos );
                if ( pos == pos0 )
                {
                    tagData.line = atoi( s.c_str() + pos + 5 );
                    pos += 5; // "line:"
                    uParsedState = EPS_TAGLINE;
                }
                break;

            case EPS_TAGLINE:
                pos1 = s.find( "\tend:", pos );
                if ( pos1 != string::npos )
                {
                    tagData.end_line = atoi( s.c_str() + pos1 + 5 );
                    ++pos1;
                }
                else
                {
                    tagData.end_line = -1;
                    pos1 = s.find( '\n', pos );
                    if ( pos1 != string::npos )
                    {
                        ++pos1;
                    }
                }
                pos0 = s.find( '\t', pos0 );
                if ( pos0 != string::npos )
                {
                    if ( (pos1 == string::npos) || (pos0 < pos1) )
                    {
                        // additional "scope" tag
                        const string::size_type len = (pos1 == string::npos) ? s.length() : pos1;
                        string::size_type i;

                        for ( i = pos0; i < len; i++ )
                        {
                            if ( s[i] == ':' )
                            {
                                // exclude "class:" from "class:ClassName"
                                if ( (i == len - 1) || (s[i + 1] != ':') )
                                {
                                    pos0 = i;
                                }
                                break;
                            }
                        }

                        ++pos0;

                        for ( i = pos0; i < len; i++ )
                        {
                            if ( s[i] == '\t' )
                            {
                                break;
                            }
                        }
                        if ( (i == len) && (i > 0) && (s[i-1] == '\n') )
                        {
                            i = len - 1; // position of '\n'
                            if ( (i > 0) && (s[i-1] == '\r') )  --i;
                        }
                        i -= pos0;

                        if ( i > 0 )
                        {
                            tagData.tagScope.assign( s.c_str() + pos0, i );
                            DelDupSpaces(tagData.tagScope);
                        }
                    }
                }
                DelDupSpaces(tagData.tagName);
                DelDupSpaces(tagData.tagPattern);
                pos0 = tagData.tagPattern.find("\\/");
                while ( pos0 != string::npos )
                {
                    tagData.tagPattern.erase(pos0, 1);
                    pos0 = tagData.tagPattern.find("\\/", pos0);
                }
                itr = m.insert( std::make_pair(tagData.line, tagData) );
                if ( itr != m.end() )
                {
                    itr->second.pTagData = (void *) &(itr->second);
                }
                uParsedState = EPS_NEWLINE;
                pos = s.find( '\n', pos );
                if ( pos != string::npos )
                {
                    ++pos;
                }
                break;
        }
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
