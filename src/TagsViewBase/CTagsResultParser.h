#ifndef _CTAGS_RESULT_PARSER_H_
#define _CTAGS_RESULT_PARSER_H_
//---------------------------------------------------------------------------
#include <string>
#include <map>
#include <wchar.h>

using std::string;
using std::multimap;

class CTagsResultParser
{
    public:
        typedef struct sTagData
        {
            string tagName;
            string tagPattern;
            string tagType;
            string tagScope;
            int    line;
            int    end_line;
            union uData {
                void* p;
                int   i;
            } data;
            void* pTagData;

            string getFullTagName() const
            {
                if ( tagScope.empty() )
                    return tagName;

                string s;
                s.reserve(tagScope.length() + tagName.length() + 2);
                s = tagScope;
                s += "::";
                s += tagName;
                return s;
            }

        } tTagData;

        typedef multimap<int, tTagData> tags_map;

        static void DelDupSpaces(string& s);
        static void Parse(const char* s, tags_map& m);
};

//---------------------------------------------------------------------------
#endif
