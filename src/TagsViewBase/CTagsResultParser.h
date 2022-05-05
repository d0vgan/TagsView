#ifndef _CTAGS_RESULT_PARSER_H_
#define _CTAGS_RESULT_PARSER_H_
//---------------------------------------------------------------------------
#include <string>
#include <map>
#include <wchar.h>

class CTagsResultParser
{
    public:
        struct tTagData
        {
            std::string tagName;
            std::string tagPattern;
            std::string tagType;
            std::string tagScope;
            std::string filePath;
            int         line;
            int         end_line;
            union uData {
                void* p;
                int   i;
            } data;
            void* pTagData;

            std::string getFullTagName() const
            {
                if ( tagScope.empty() )
                    return tagName;

                std::string s;
                s.reserve(tagScope.length() + tagName.length() + 2);
                s += tagScope;
                s += "::";
                s += tagName;
                return s;
            }

        };

        typedef std::multimap<int, tTagData> tags_map;

        enum eParseFlags {
            PF_INCLUDEFILEPATH = 0x01
        };
        static void Parse(const char* s, tags_map& m, unsigned int nParseFlags = 0);
};

//---------------------------------------------------------------------------
#endif
