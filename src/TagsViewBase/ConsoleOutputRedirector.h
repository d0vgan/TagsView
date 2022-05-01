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

        unsigned int Execute(LPCTSTR pszCmdLine);

        void ClearOutputBuffer();
        const ch_t* GetOutputBuffer() const;
        const unsigned int GetOutputBufferSize() const;
        void ReserveOutputBuffer(unsigned int uBufferSize);
        const string_t& GetOutputString() const { return m_output; }

    protected:
        string_t m_output;
};

class CProcess
{
    public:
        CProcess();
        ~CProcess();

        BOOL  Execute(LPCTSTR pszCmdLine, BOOL bWaitForFinish); // run a new process
        INT   WaitForFinish() const; // wait for the process to finish
        INT   GetExitCode() const; // process exit code
        DWORD GetId() const; // process id

    protected:
        mutable INT         m_nExitCode;
        PROCESS_INFORMATION m_pi;
};

//---------------------------------------------------------------------------
#endif
