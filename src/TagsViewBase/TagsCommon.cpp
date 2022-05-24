#include "TagsCommon.h"

namespace TagsCommon
{
    bool isFileExist(LPCTSTR pszFilePath)
    {
        if ( pszFilePath && pszFilePath[0] )
        {
            if ( ::GetFileAttributes(pszFilePath) != INVALID_FILE_ATTRIBUTES )
                return true;
        }
        return false;
    }

    LPCTSTR getFileName(LPCTSTR pszFilePathName)
    {
        if ( pszFilePathName && *pszFilePathName )
        {
            int n = lstrlen(pszFilePathName);
            while ( --n >= 0 )
            {
                if ( (pszFilePathName[n] == _T('\\')) || (pszFilePathName[n] == _T('/')) )
                {
                    break;
                }
            }

            if ( n >= 0 )
            {
                ++n;
                pszFilePathName += n;
            }
        }
        return pszFilePathName;
    }

    LPCTSTR getFileName(const t_string& filePathName)
    {
        t_string::size_type n = 0;
        if ( !filePathName.empty() )
        {
            n = filePathName.find_last_of(_T("\\/"));
            if ( n != t_string::npos )
                ++n;
            else
                n = 0;
        }
        return (filePathName.c_str() + n);
    }

    LPCTSTR getFileName(const tTagData* pTag)
    {
        if ( !pTag->hasFilePath() )
            return _T("");

        const t_string& filePath = *pTag->pFilePath;
        if ( filePath.length() <= pTag->nFileDirLen )
            return getFileName(filePath);

        size_t n = pTag->nFileDirLen;
        if ( filePath[n] == _T('\\') || filePath[n] == _T('/') )  ++n;
        return (filePath.c_str() + n);
    }

    LPCTSTR getFileExt(LPCTSTR pszFilePathName)
    {
        const TCHAR* pszExt = pszFilePathName + lstrlen(pszFilePathName);
        while ( --pszExt >= pszFilePathName )
        {
            if ( *pszExt == _T('.') )
                return pszExt;

            if ( *pszExt == _T('\\') || *pszExt == _T('/') )
                break;
        }
        return _T("");
    }

    t_string getFileDirectory(const t_string& filePathName)
    {
        t_string::size_type n = filePathName.find_last_of(_T("\\/"));
        return ( (n != t_string::npos) ? t_string(filePathName.c_str(), n + 1) : t_string() );
    }


    bool setClipboardText(const t_string& text, HWND hWndOwner)
    {
        bool bSucceeded = false;

        if ( ::OpenClipboard(hWndOwner) )
        {
            HGLOBAL hTextMem = ::GlobalAlloc( GMEM_MOVEABLE, (text.length() + 1)*sizeof(TCHAR) );
            if ( hTextMem != NULL )
            {
                LPTSTR pszText = (LPTSTR) ::GlobalLock(hTextMem);
                if ( pszText != NULL )
                {
                    lstrcpy(pszText, text.c_str());
                    ::GlobalUnlock(hTextMem);

                    ::EmptyClipboard();

#ifdef UNICODE
                    const UINT uClipboardFormat = CF_UNICODETEXT;
#else
                    const UINT uClipboardFormat = CF_TEXT;
#endif
                    if ( ::SetClipboardData(uClipboardFormat, hTextMem) != NULL )
                        bSucceeded = true;
                }
            }
            ::CloseClipboard();
        }

        return bSucceeded;
    }
}
