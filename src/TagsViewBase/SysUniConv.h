#ifndef _sys_uni_conv_h_
#define _sys_uni_conv_h_
//---------------------------------------------------------------------------
#include <windows.h>
#include <wchar.h>

class SysUniConv
{
    public:
        static wchar_t* newMultiByteToUnicode(const char* aStr, int aLen = -1,
                            UINT aCodePage = CP_ACP, int* pwLen = NULL);
        // use delete[] to free the allocated memory
        static char*    newMultiByteToUTF8(const char* aStr, int aLen = -1,
                            UINT aCodePage = CP_ACP, int* puLen = NULL);
        // use delete[] to free the allocated memory
        static char*    newUnicodeToMultiByte(const wchar_t* wStr, int wLen = -1,
                            UINT aCodePage = CP_ACP, int* paLen = NULL);
        // use delete[] to free the allocated memory
        static char*    newUnicodeToUTF8(const wchar_t* wStr, int wLen = -1,
                            int* puLen = NULL);
        // use delete[] to free the allocated memory
        static char*    newUTF8ToMultiByte(const char* uStr, int uLen = -1,
                            UINT aCodePage = CP_ACP, int* paLen = NULL);
        // use delete[] to free the allocated memory
        static wchar_t* newUTF8ToUnicode(const char* uStr, int uLen = -1,
                            int* pwLen = NULL);
        // use delete[] to free the allocated memory

        static int MultiByteToUnicode(wchar_t* wStr, int wMaxLen, const char* aStr,
                       int aLen = -1, UINT aCodePage = CP_ACP);
        static int MultiByteToUTF8(char* uStr, int uMaxLen, const char* aStr,
                       int aLen = -1, UINT aCodePage = CP_ACP);
        static int UnicodeToMultiByte(char* aStr, int aMaxLen, const wchar_t* wStr,
                       int wLen = -1, UINT aCodePage = CP_ACP);
        static int UnicodeToUTF8(char* uStr, int uMaxLen, const wchar_t* wStr,
                       int wLen = -1);
        static int UTF8ToMultiByte(char* aStr, int aMaxLen, const char* uStr,
                       int uLen = -1, UINT aCodePage = CP_ACP);
        static int UTF8ToUnicode(wchar_t* wStr, int wMaxLen, const char* uStr,
                       int uLen = -1);

    protected:
        static int a2w(wchar_t* ws, int wml, const char* as, int al, UINT acp);
        static int a2u(char* us, int uml, const char* as, int al, UINT acp);
        static int w2a(char* as, int aml, const wchar_t* ws, int wl, UINT acp);
        static int w2u(char* us, int uml, const wchar_t* ws, int wl);
        static int u2a(char* as, int aml, const char* us, int ul, UINT acp);
        static int u2w(wchar_t* ws, int wml, const char* us, int ul);
};


//---------------------------------------------------------------------------
#endif
