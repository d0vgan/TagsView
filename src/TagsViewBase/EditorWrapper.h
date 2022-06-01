#ifndef _TAGS_EDITOR_WRAPPER_H_
#define _TAGS_EDITOR_WRAPPER_H_
//---------------------------------------------------------------------------
#include "TagsCommon.h"
#include <windows.h>
#include <string>
#include <map>
#include <vector>
#include <set>

class IEditorWrapper
{
    public:
        typedef TagsCommon::t_string t_string;
        typedef TagsCommon::tstring_cmp_less tstring_cmp_less;
        typedef std::set<t_string, tstring_cmp_less> file_set;

        struct sEditorColors {
            COLORREF crTextColor;
            COLORREF crBkgndColor;
        };

    public:
        // the editor name, e.g. "AkelPad" or "Notepad++"
        virtual LPCTSTR ewGetEditorName() const = 0;

        // the editor short name, e.g. "akel" or "npp"
        virtual LPCTSTR ewGetEditorShortName() const = 0;

        // open a file
        virtual bool ewDoOpenFile(LPCTSTR pszFileName) = 0;

        // save current file
        virtual void ewDoSaveFile() = 0;

        // set focus to the editor's window
        virtual void ewDoSetFocus() = 0;

        // set selection in current file
        virtual void ewDoSetSelection(int selStart, int selEnd) = 0;

        // current edit window
        virtual HWND ewGetEditHwnd() const = 0;

        // current file pathname (e.g. "C:\My Project\File Name.cpp")
        virtual t_string ewGetFilePathName() const = 0;

        // all opened files 
        virtual file_set ewGetOpenedFilePaths() const = 0;

        virtual int ewGetLineFromPos(int pos) const = 0; // 0-based

        virtual int ewGetPosFromLine(int line) const = 0; // 0-based

        virtual int ewGetSelectionPos(int& selEnd) const = 0;

        //virtual t_string ewGetTextLine(int line) const = 0;

        // true when current file is saved (unmodified)
        virtual bool ewIsFileSaved() const = 0;

        // editor colors
        virtual bool ewGetEditorColors(sEditorColors& colors) const = 0;

        // close TagsView dialog
        virtual void ewCloseTagsView() = 0;
};

class CTagsDlg;

class CEditorWrapper : public IEditorWrapper
{
    public:
        CEditorWrapper(CTagsDlg* pDlg);
        virtual ~CEditorWrapper();

        // can navigate backward in current file
        bool ewCanNavigateBackward();

        // can navigate forward in current file
        bool ewCanNavigateForward();

        // clear navigation history for all files or just current file
        // call it manually
        void ewClearNavigationHistory(bool bAllFiles);

        // navigate backward in current file
        // may be overridden if needed
        virtual void ewDoNavigateBackward();

        // navigate forward in current file
        // may be overridden if needed
        virtual void ewDoNavigateForward();

        // (re)parse current file
        void ewDoParseFile(bool bReparsePhysicalFile);

        // returns "<FunctionName>, line <NN>:\n  <TextLine>"
        t_string ewGetNavigateBackwardHint();

        // returns "<FunctionName>, line <NN>:\n  <TextLine>"
        t_string ewGetNavigateForwardHint();

        // true when current file has navigation history
        bool ewHasNavigationHistory();

        void ewSetNavigationPoint(const t_string& hint, bool incPos = true);

        void ewOnFileActivated();
        void ewOnFileClosed();
        void ewOnFileOpened();
        void ewOnFileReloaded();
        void ewOnFileSaved();
        void ewOnSelectionChanged();

        HWND ewGetMainHwnd() const { return m_hMainWnd; }
        void ewSetMainHwnd(HWND hWnd) { m_hMainWnd = hWnd; }

    protected:
        typedef struct sNavigationPoint {
            int selPos;
            t_string hint;
        } tNavigationPoint;

        class CNavigationHistory
        {
            public:
                CNavigationHistory()
                {
                    Clear();
                }

                void Add(int selPos, const t_string& hint, bool incPos)
                {
                    tNavigationPoint np = { selPos, hint };

                    if ( incPos )
                    {
                        m_history.erase( m_history.begin() + m_pos, m_history.end() );
                        ++m_pos;
                    }

                    if ( incPos || (m_pos == m_history.size()) )
                        m_history.push_back(np);
                }

                bool CanBackward() const
                {
                    return (m_pos > 0);
                }

                bool CanForward() const
                {
                    return (m_pos + 1 < m_history.size());
                }

                void Clear()
                {
                    m_pos = 0;
                    m_history.clear();
                }

                int Backward()
                {
                    if ( m_pos > 0 )
                    {
                        const tNavigationPoint& np = m_history[--m_pos];
                        return np.selPos;
                    }

                    return -1;
                }

                int Forward()
                {
                    if ( m_pos + 1 < m_history.size() )
                    {
                        const tNavigationPoint& np = m_history[++m_pos];
                        return np.selPos;
                    }

                    return -1;
                }

                t_string GetBackwardHint() const
                {
                    t_string hint;

                    if ( m_pos > 0 )
                    {
                        const tNavigationPoint& np = m_history[m_pos - 1];
                        hint = np.hint;
                    }

                    return hint;
                }

                t_string GetForwardHint() const
                {
                    t_string hint;

                    if ( m_pos + 1 < m_history.size() )
                    {
                        const tNavigationPoint& np = m_history[m_pos + 1];
                        hint = np.hint;
                    }

                    return hint;
                }

                const tNavigationPoint& GetLastItem() const
                {
                    return m_history.back();
                }

                tNavigationPoint& GetLastItem()
                {
                    return m_history.back();
                }

                bool IsEmpty() const
                {
                    return m_history.empty();
                }

            protected:
                std::vector<t_string>::size_type m_pos;
                std::vector<tNavigationPoint> m_history;
        };

        typedef std::map<t_string, CNavigationHistory> t_navmap;

        t_navmap::const_iterator getNavItr(const t_string& filePathName) const;
        t_navmap::iterator getNavItr(const t_string& filePathName);
        void clearNavItem(const t_string& filePathName, bool bEraseItem);

        const t_string& getCurrentFilePathName();
        void clearCurrentFilePathName();

    protected:
        CTagsDlg* m_pDlg;
        HWND      m_hMainWnd;
        t_navmap  m_navMap;
        t_string  m_currentFilePathName;
};


//---------------------------------------------------------------------------
#endif
