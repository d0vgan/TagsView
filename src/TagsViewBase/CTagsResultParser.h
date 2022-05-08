#ifndef _CTAGS_RESULT_PARSER_H_
#define _CTAGS_RESULT_PARSER_H_
//---------------------------------------------------------------------------
#include <string>
#include <map>
#include <set>
#include <vector>
#include <wchar.h>
#include <tchar.h>
#include <Windows.h>

class CTagsResultParser
{
    public:
        typedef std::basic_string<TCHAR> t_string;

        class string_cmp_less
        {
            public:
                bool operator() (const t_string& s1, const t_string& s2) const
                {
                    return (lstrcmpi(s1.c_str(), s2.c_str()) < 0);
                }
        };

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

        typedef std::multimap<int, tTagData> file_tags_map; // tags within a file
        typedef std::vector<tTagData*> file_tags_vec;

        struct tFileTags
        {
            file_tags_map tagsMap;
            file_tags_vec tagsVec;
        };

        typedef std::map<t_string, tFileTags, string_cmp_less> tags_map; // tags in multiple files

        enum eParseFlags {
            PF_ISUTF8 = 0x01
        };

        struct tParseContext
        {
            tags_map& m; // output tags map
            std::set<t_string>& relatedFiles; // output related files
            t_string inputDir; // input directory
            t_string inputFileOverride; // input file override, use it only to override a temp file name

            tParseContext(tags_map& m_, std::set<t_string>& relatedFiles_, const t_string& inputDir_, const t_string& inputFileOverride_)
              : m(m_), relatedFiles(relatedFiles_), inputDir(inputDir_), inputFileOverride(inputFileOverride_)
            {
            }
        };

        static void Parse(const char* s, unsigned int nParseFlags, tParseContext& context);
};

//---------------------------------------------------------------------------
#endif
