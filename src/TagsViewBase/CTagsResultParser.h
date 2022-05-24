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
#include "TagsCommon.h"

class CTagsResultParser
{
    protected:
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

    public:
        typedef TagsCommon::t_string t_string;
        typedef TagsCommon::tTagData tTagData;
        typedef TagsCommon::string_cmp_less string_cmp_less;
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
