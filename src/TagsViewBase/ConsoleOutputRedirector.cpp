#include "ConsoleOutputRedirector.h"
#include <vector>

#define DEFAULT_PIPE_SIZE 0
#define PIPE_READBUFSIZE  4096
#define MAX_CMDLINE       4096

namespace
{
    void readFromPipe(HANDLE hPipe, CConsoleOutputRedirector::string_t& data)
    {
        DWORD dwBytesRead;
        CConsoleOutputRedirector::ch_t buf[PIPE_READBUFSIZE];
        const DWORD uMaxBytesToRead = (PIPE_READBUFSIZE - 1)*sizeof(CConsoleOutputRedirector::ch_t);

        do {
            ::Sleep(10); // it prevents from 100% CPU usage while reading!

            dwBytesRead = 0;
            if ( ::PeekNamedPipe(hPipe, NULL, 0, NULL, &dwBytesRead, NULL) && (dwBytesRead > 0) )
            {
                dwBytesRead = 0;
                if ( ::ReadFile(hPipe, buf, uMaxBytesToRead, &dwBytesRead, NULL) && (dwBytesRead > 0) )
                {
                    data.append( buf, dwBytesRead );
                }
            }

        } while ( dwBytesRead > 0 );
    }
}

unsigned int CConsoleOutputRedirector::Execute(LPCTSTR pszCmdLine)
{
    m_output.clear();

    if ( pszCmdLine && pszCmdLine[0] )
    {
        SECURITY_DESCRIPTOR sd;
        SECURITY_ATTRIBUTES sa;

        HANDLE hStdOutReadPipe = NULL;
        HANDLE hStdOutWritePipe = NULL;

        ::InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION );
        ::SetSecurityDescriptorDacl( &sd, TRUE, NULL, FALSE );
        sa.lpSecurityDescriptor = &sd;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;

        if ( ::CreatePipe(&hStdOutReadPipe, &hStdOutWritePipe, &sa, DEFAULT_PIPE_SIZE) )
        {
            PROCESS_INFORMATION pi;
            STARTUPINFO         si;

            ::SetHandleInformation(hStdOutReadPipe, HANDLE_FLAG_INHERIT, 0);

            // initialize STARTUPINFO struct
            ::ZeroMemory( &si, sizeof(STARTUPINFO) );
            si.cb = sizeof(STARTUPINFO);
            si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
            si.wShowWindow = SW_HIDE;
            si.hStdInput = NULL;
            si.hStdOutput = hStdOutWritePipe;
            si.hStdError = hStdOutWritePipe;

            ::ZeroMemory( &pi, sizeof(PROCESS_INFORMATION) );

            int nCmdLineLen = lstrlen(pszCmdLine);
            std::vector<TCHAR> cmdLine(pszCmdLine, pszCmdLine + nCmdLineLen + 1); // including the trailing '\0'

            if ( ::CreateProcess(
                     NULL,
                     cmdLine.data(),
                     NULL,                     // security
                     NULL,                     // security
                     TRUE,                     // inherits handles
                     0,                        // creation flags
                     NULL,                     // environment
                     NULL,                     // current directory
                     &si,                      // startup info
                     &pi                       // process info
               ) )
            {
                ::CloseHandle(pi.hThread);

                ::WaitForSingleObject(pi.hProcess, 100);

                do
                {
                    readFromPipe(hStdOutReadPipe, m_output);
                }
                while ( ::WaitForSingleObject(pi.hProcess, 50) == WAIT_TIMEOUT );

                readFromPipe(hStdOutReadPipe, m_output);

                ::CloseHandle(pi.hProcess);
            }

            ::CloseHandle(hStdOutReadPipe);
            ::CloseHandle(hStdOutWritePipe);
        }
    }

    return static_cast<unsigned int>( m_output.size() );
}

void CConsoleOutputRedirector::ClearOutputBuffer()
{
    m_output.clear();
}

const CConsoleOutputRedirector::ch_t* CConsoleOutputRedirector::GetOutputBuffer() const
{
    static const ch_t empty_buf[2] = { 0, 0 };
    return ( m_output.empty() ? empty_buf : m_output.c_str() );
}

const unsigned int CConsoleOutputRedirector::GetOutputBufferSize() const
{
    return static_cast<unsigned int>( m_output.size() );
}

void CConsoleOutputRedirector::ReserveOutputBuffer(unsigned int uBufferSize)
{
    m_output.reserve(uBufferSize);
}

///////////////////////////////////////////////////////////////////////////////

CProcess::CProcess() : m_nExitCode(-1)
{
    ::ZeroMemory(&m_pi, sizeof(m_pi));
}

CProcess::~CProcess()
{
    if ( m_pi.hProcess )
    {
        ::CloseHandle(m_pi.hProcess);
    }
}

BOOL CProcess::Execute(LPCTSTR pszCmdLine, BOOL bWaitForFinish)
{
    m_nExitCode = -1;
    if ( m_pi.hProcess )
    {
        ::CloseHandle(m_pi.hProcess);
    }
    ::ZeroMemory(&m_pi, sizeof(m_pi));

    if ( pszCmdLine && pszCmdLine[0] )
    {
        STARTUPINFOW si;
        ::ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(STARTUPINFO);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        int nCmdLineLen = lstrlen(pszCmdLine);
        std::vector<TCHAR> cmdLine(pszCmdLine, pszCmdLine + nCmdLineLen + 1); // including the trailing '\0'

        if ( ::CreateProcess(NULL, cmdLine.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &m_pi) )
        {
            ::CloseHandle( m_pi.hThread );
            m_pi.hThread = NULL;

            if ( bWaitForFinish )
            {
                WaitForFinish();
            }

            return TRUE;
        }
    }

    return FALSE;
}

INT CProcess::WaitForFinish() const
{
    if ( m_pi.hProcess )
    {
        ::WaitForSingleObject(m_pi.hProcess, INFINITE);

        DWORD dwExitCode = 0;
        if ( ::GetExitCodeProcess(m_pi.hProcess, &dwExitCode) )
        {
            m_nExitCode = static_cast<INT>(dwExitCode);
        }
    }

    return m_nExitCode;
}

INT CProcess::GetExitCode() const
{
    return ( (m_nExitCode != -1) ? m_nExitCode : WaitForFinish() );
}

DWORD CProcess::GetId() const
{
    return m_pi.dwProcessId;
}
