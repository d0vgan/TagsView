#ifndef _CTAGS_RESULT_PARSER_H_
#define _CTAGS_RESULT_PARSER_H_
//---------------------------------------------------------------------------
#include <string>
#include <map>
#include <vector>
#include <memory>
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
            int      line;
            int      end_line;
            const t_string* pFilePath;
            union uData {
                void* p;
                int   i;
            } data;

            tTagData(const t_string& tagName_, const t_string& tagPattern_, const t_string& tagType_,
                     const t_string& tagScope_, int line_, int end_line_)
              : tagName(tagName_), tagPattern(tagPattern_), tagType(tagType_),
                tagScope(tagScope_), line(line_), end_line(end_line_), pFilePath(nullptr)
            {
                data.p = nullptr;
            }

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

        typedef std::vector<std::unique_ptr<tTagData>> file_tags; // tags within a file
        typedef std::map<t_string, file_tags, string_cmp_less> tags_map; // tags in multiple files

        enum eParseFlags {
            PF_ISUTF8 = 0x01
        };

        struct tParseContext
        {
            tags_map& m; // output tags map
            t_string inputDir; // input directory
            t_string inputFileOverride; // input file override, use it only to override a temp file name

            tParseContext(tags_map& m_, const t_string& inputDir_, const t_string& inputFileOverride_)
              : m(m_), inputDir(inputDir_), inputFileOverride(inputFileOverride_)
            {
            }
        };

        static void Parse(const char* s, unsigned int nParseFlags, tParseContext& context);
};

//---------------------------------------------------------------------------
#endif
