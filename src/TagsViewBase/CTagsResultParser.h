#ifndef _CTAGS_RESULT_PARSER_H_
#define _CTAGS_RESULT_PARSER_H_
//---------------------------------------------------------------------------
#include <string>
#include <map>
#include <set>
#include <wchar.h>
#include <tchar.h>

class CTagsResultParser
{
    public:
        typedef std::basic_string<TCHAR> t_string;

        struct tTagDataInternal
        {
            std::string tagName;
            std::string tagPattern;
            std::string tagType;
            std::string tagScope;
            std::string filePath;
            int         line;
            int         end_line;
        };

        struct tTagData
        {
            t_string tagName;
            t_string tagPattern;
            t_string tagType;
            t_string tagScope;
            t_string filePath;
            int      line;
            int      end_line;
            union uData {
                void* p;
                int   i;
            } data;
            void* pTagData;

            t_string getFullTagName() const
            {
                if ( tagScope.empty() )
                    return tagName;

                t_string s;
                s.reserve(tagScope.length() + tagName.length() + 2);
                s += tagScope;
                s += _T("::");
                s += tagName;
                return s;
            }

        };

        typedef std::multimap<int, tTagData> tags_map;

        enum eParseFlags {
            PF_ISUTF8          = 0x01,
            PF_INCLUDEFILEPATH = 0x02
        };
        static void Parse(const char* s, tags_map& m, unsigned int nParseFlags, std::set<t_string>& relatedFiles);
};

//---------------------------------------------------------------------------
#endif
