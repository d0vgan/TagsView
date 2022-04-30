#include "SysUniConv.h"
#include <string.h>

wchar_t* SysUniConv::newMultiByteToUnicode(const char* aStr, int aLen ,
                                           UINT aCodePage, int* pwLen )
{
    if ( aStr )
    {
        if ( aLen < 0 )
            aLen = (int)strlen(aStr);
        if ( aLen > 0 )
        {
            int      wLen = aLen;
            wchar_t* wStr = new wchar_t[wLen + 1];
            if ( wStr )
            {
                wLen = a2w(wStr, wLen, aStr, aLen, aCodePage);
                if ( pwLen )
                    *pwLen = wLen;
                return wStr;
            }
        }
    }
    if ( pwLen )
        *pwLen = 0;
    return NULL;
}

char* SysUniConv::newMultiByteToUTF8(const char* aStr, int aLen ,
                                     UINT aCodePage, int* puLen )
{
    if ( aStr )
    {
        if ( aLen < 0 )
            aLen = (int)strlen(aStr);
        if ( aLen > 0 )
        {
            int   uLen = 3*aLen;
            char* uStr = new char[uLen + 1];
            if ( uStr )
            {
                uLen = a2u(uStr, uLen, aStr, aLen, aCodePage);
                if ( puLen )
                    *puLen = uLen;
                return uStr;
            }
        }
    }
    if ( puLen )
        *puLen = 0;
    return NULL;
}

char* SysUniConv::newUnicodeToMultiByte(const wchar_t* wStr, int wLen ,
                                        UINT aCodePage, int* paLen )
{
    if ( wStr )
    {
        if ( wLen < 0 )
            wLen = (int)wcslen(wStr);
        if ( wLen > 0 )
        {
            int   aLen = 2*wLen; // 2*wLen - for DBCS
            char* aStr = new char[aLen + 1];
            if ( aStr )
            {
                aLen = w2a(aStr, aLen, wStr, wLen, aCodePage);
                if ( paLen )
                    *paLen = aLen;
                return aStr;
            }
        }
    }
    if ( paLen )
        *paLen = 0;
    return NULL;
}

char* SysUniConv::newUnicodeToUTF8(const wchar_t* wStr, int wLen ,
                                   int* puLen )
{
    if ( wStr )
    {
        if ( wLen < 0 )
            wLen = (int)wcslen(wStr);
        if ( wLen > 0 )
        {
            int   uLen = wLen*3;
            char* uStr = new char[uLen + 1];
            if ( uStr )
            {
                uLen = w2u(uStr, uLen, wStr, wLen);
                if ( puLen )
                    *puLen = uLen;
                return uStr;
            }
        }
    }
    if ( puLen )
        *puLen = 0;
    return NULL;
}

char* SysUniConv::newUTF8ToMultiByte(const char* uStr, int uLen ,
                                     UINT aCodePage, int* paLen )
{
    if ( uStr )
    {
        if ( uLen < 0 )
            uLen = (int)strlen(uStr);
        if ( uLen > 0 )
        {
            int   aLen = uLen;
            char* aStr = new char[aLen + 1];
            if ( aStr )
            {
                aLen = u2a(aStr, aLen, uStr, uLen, aCodePage);
                if ( paLen )
                    *paLen = aLen;
                return aStr;
            }
        }
    }
    if ( paLen )
        *paLen = 0;
    return NULL;
}

wchar_t* SysUniConv::newUTF8ToUnicode(const char* uStr, int uLen ,
                                      int* pwLen )
{
    if ( uStr )
    {
        if ( uLen < 0 )
            uLen = (int)strlen(uStr);
        if ( uLen > 0 )
        {
            int      wLen = uLen;
            wchar_t* wStr = new wchar_t[wLen + 1];
            if ( wStr )
            {
                wLen = u2w(wStr, wLen, uStr, uLen);
                if ( pwLen )
                    *pwLen = wLen;
                return wStr;
            }
        }
    }
    if ( pwLen )
        *pwLen = 0;
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////

int SysUniConv::MultiByteToUnicode(wchar_t* wStr, int wMaxLen, const char* aStr,
                                   int aLen , UINT aCodePage )
{
    if ( aStr && wStr && (wMaxLen > 0) )
    {
        if ( aLen < 0 )
            aLen = (int)strlen(aStr);
        if ( aLen > 0 )
        {
            return a2w(wStr, wMaxLen, aStr, aLen, aCodePage);
        }
    }
    if ( wStr && (wMaxLen > 0) )
        wStr[0] = 0;
    return 0;
}

int SysUniConv::MultiByteToUTF8(char* uStr, int uMaxLen, const char* aStr,
                                int aLen , UINT aCodePage )
{
    if ( aStr && uStr && (uMaxLen > 0) )
    {
        if ( aLen < 0 )
            aLen = (int)strlen(aStr);
        if ( aLen > 0 )
        {
            return a2u(uStr, uMaxLen, aStr, aLen, aCodePage);
        }
    }
    if ( uStr && (uMaxLen > 0) )
        uStr[0] = 0;
    return 0;
}

int SysUniConv::UnicodeToMultiByte(char* aStr, int aMaxLen, const wchar_t* wStr,
                                   int wLen , UINT aCodePage )
{
    if ( wStr && aStr && (aMaxLen > 0) )
    {
        if ( wLen < 0 )
            wLen = (int)wcslen(wStr);
        if ( wLen > 0 )
        {
            return w2a(aStr, aMaxLen, wStr, wLen, aCodePage);
        }
    }
    if ( aStr && (aMaxLen > 0) )
        aStr[0] = 0;
    return 0;
}

int SysUniConv::UnicodeToUTF8(char* uStr, int uMaxLen, const wchar_t* wStr,
                              int wLen )
{
    if ( wStr && uStr && (uMaxLen > 0) )
    {
        if ( wLen < 0 )
            wLen = (int)wcslen(wStr);
        if ( wLen > 0 )
        {
            return w2u(uStr, uMaxLen, wStr, wLen);
        }
    }
    if ( uStr && (uMaxLen > 0) )
        uStr[0] = 0;
    return 0;
}

int SysUniConv::UTF8ToMultiByte(char* aStr, int aMaxLen, const char* uStr,
                                int uLen , UINT aCodePage )
{
    if ( uStr && aStr && (aMaxLen > 0) )
    {
        if ( uLen < 0 )
            uLen = (int)strlen(uStr);
        if ( uLen > 0 )
        {
            return u2a(aStr, aMaxLen, uStr, uLen, aCodePage);
        }
    }
    if ( aStr && (aMaxLen > 0) )
        aStr[0] = 0;
    return 0;
}

int SysUniConv::UTF8ToUnicode(wchar_t* wStr, int wMaxLen, const char* uStr,
                              int uLen )
{
    if ( uStr && wStr && (wMaxLen > 0) )
    {
        if ( uLen < 0 )
            uLen = (int)strlen(uStr);
        if ( uLen > 0 )
        {
            return u2w(wStr, wMaxLen, uStr, uLen);
        }
    }
    if ( wStr && (wMaxLen > 0) )
        wStr[0] = 0;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

int SysUniConv::a2w(wchar_t* ws, int wml, const char* as, int al, UINT acp)
{
    int len = ::MultiByteToWideChar(acp, 0, as, al, ws, wml);
    ws[len] = 0;
    return len;
}

int SysUniConv::a2u(char* us, int uml, const char* as, int al, UINT acp)
{
    int      len = 0;
    wchar_t* ws = new wchar_t[al + 1];
    if ( ws )
    {
        len = ::MultiByteToWideChar(acp, 0, as, al, ws, al);
        ws[len] = 0;
        len = ::WideCharToMultiByte(CP_UTF8, 0, ws, len, us, uml, NULL, NULL);
        delete [] ws;
    }
    us[len] = 0;
    return len;
}

int SysUniConv::w2a(char* as, int aml, const wchar_t* ws, int wl, UINT acp)
{
    int len = ::WideCharToMultiByte(acp, 0, ws, wl, as, aml, NULL, NULL);
    as[len] = 0;
    return len;
}

int SysUniConv::w2u(char* us, int uml, const wchar_t* ws, int wl)
{
    int len = ::WideCharToMultiByte(CP_UTF8, 0, ws, wl, us, uml, NULL, NULL);
    us[len] = 0;
    return len;
}

int SysUniConv::u2a(char* as, int aml, const char* us, int ul, UINT acp)
{
    int      len = 0;
    wchar_t* ws = new wchar_t[ul + 1];
    if ( ws )
    {
        len = ::MultiByteToWideChar(CP_UTF8, 0, us, ul, ws, ul);
        ws[len] = 0;
        len = ::WideCharToMultiByte(acp, 0, ws, len, as, aml, NULL, NULL);
        delete [] ws;
    }
    as[len] = 0;
    return len;
}

int SysUniConv::u2w(wchar_t* ws, int wml, const char* us, int ul)
{
    int len = ::MultiByteToWideChar(CP_UTF8, 0, us, ul, ws, wml);
    ws[len] = 0;
    return len;
}
