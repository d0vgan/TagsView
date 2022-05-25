#ifndef _TAGS_COMMON_H_
#define _TAGS_COMMON_H_
//---------------------------------------------------------------------------
#include <windows.h>
#include <tchar.h>
#include <string>

namespace TagsCommon
{
    typedef std::basic_string<TCHAR> t_string;

    class string_cmp_less
    {
    public:
        bool operator() (const t_string& s1, const t_string& s2) const
        {
            return (lstrcmpi(s1.c_str(), s2.c_str()) < 0);
        }
    };

    struct tTagData
    {
        t_string tagName;
        t_string tagPattern;
        t_string tagType;
        t_string tagScope;
        int      line;
        int      end_line;
        size_t   nFileDirLen;
        const t_string* pFilePath;
        union uData {
            void* p;
            int   i;
        } data;

        tTagData(const t_string& tagName_, const t_string& tagPattern_, const t_string& tagType_,
                 const t_string& tagScope_, int line_, int end_line_, size_t nFileDirLen_)
          : tagName(tagName_), tagPattern(tagPattern_), tagType(tagType_),
            tagScope(tagScope_), line(line_), end_line(end_line_),
            nFileDirLen(nFileDirLen_), pFilePath(nullptr)
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

        bool hasFilePath() const
        {
            return (pFilePath && !pFilePath->empty());
        }
    };

    bool isFileExist(LPCTSTR pszFilePath);
    LPCTSTR getFileName(LPCTSTR pszFilePathName);
    LPCTSTR getFileName(const t_string& filePathName);
    LPCTSTR getFileName(const tTagData* pTag);
    LPCTSTR getFileExt(LPCTSTR pszFilePathName);
    t_string getFileDirectory(const t_string& filePathName);
    bool setClipboardText(const t_string& text, HWND hWndOwner);
}

//---------------------------------------------------------------------------
#endif
