#ifndef _CONSOLE_OUTPUT_REDIRECTOR_H_
#define _CONSOLE_OUTPUT_REDIRECTOR_H_
//---------------------------------------------------------------------------
#include <windows.h>
#include <string>

class CConsoleOutputRedirector
{
    public:
        typedef char ch_t;
        typedef std::basic_string<ch_t> string_t;

        void ClearBuffer();
        unsigned int Execute(const TCHAR* cszCommandLine);
        const ch_t* GetBuffer() const;
        const unsigned int GetBufferSize() const;
        void ReserveBuffer(unsigned int uBufferSize);
        const string_t& GetDataString() const { return _data; }

    protected:
        string_t _data;
};


//---------------------------------------------------------------------------
#endif
