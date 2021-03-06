#include "TagsDlg.h"
#include "EditorWrapper.h"

CEditorWrapper::CEditorWrapper(CTagsDlg* pDlg) : m_pDlg(pDlg), m_hMainWnd(NULL)
{

}

CEditorWrapper::~CEditorWrapper()
{

}

bool CEditorWrapper::ewCanNavigateBackward()
{
    t_navmap::iterator itr = getNavItr( getCurrentFilePathName() );
    if ( itr != m_navMap.end() )
    {
        return itr->second.CanBackward();
    }

    return false;
}

bool CEditorWrapper::ewCanNavigateForward()
{
    t_navmap::iterator itr = getNavItr( getCurrentFilePathName() );
    if ( itr != m_navMap.end() )
    {
        return itr->second.CanForward();
    }

    return false;
}

void CEditorWrapper::ewClearNavigationHistory(bool bAllFiles)
{
    if ( bAllFiles )
    {
        m_navMap.clear();
    }
    else // current file
    {
        clearNavItem(getCurrentFilePathName(), true);
    }
    if ( m_pDlg && m_pDlg->GetHwnd() )
    {
        m_pDlg->UpdateNavigationButtons();
    }
}

void CEditorWrapper::ewDoNavigateBackward()
{
    t_navmap::iterator itr = getNavItr( getCurrentFilePathName() );
    if ( itr != m_navMap.end() )
    {
        int selPos = itr->second.Backward();
        if ( selPos < 0 )
            return;

        int selEnd;
        int selStart = ewGetSelectionPos(selEnd);
        if ( selStart == selPos )
        {
            selPos = itr->second.Backward();
            if ( selPos < 0 )
                return;
        }
        ewDoSetSelection(selPos, selPos);
    }
}

void CEditorWrapper::ewDoNavigateForward()
{
    t_navmap::iterator itr = getNavItr( getCurrentFilePathName() );
    if ( itr != m_navMap.end() )
    {
        int selPos = itr->second.Forward();
        if ( selPos >= 0 )
        {
            ewDoSetSelection(selPos, selPos);
        }
    }
}

void CEditorWrapper::ewDoParseFile(bool bReparsePhysicalFile)
{
    if ( !getCurrentFilePathName().empty() )
    {
        if ( m_pDlg->GetHwnd() && m_pDlg->IsWindowVisible() )
            m_pDlg->ParseFile( getCurrentFilePathName().c_str(), bReparsePhysicalFile );
    }
    else
    {
        m_pDlg->ParseFile(nullptr, FALSE); // calls ClearItems() and sets m_tags to nullptr
    }
}

CEditorWrapper::t_navmap::const_iterator CEditorWrapper::getNavItr(const t_string& filePathName) const
{
    if ( !filePathName.empty() )
    {
        return m_navMap.find(filePathName);
    }
    return m_navMap.end();
}

CEditorWrapper::t_navmap::iterator CEditorWrapper::getNavItr(const t_string& filePathName)
{
    if ( !filePathName.empty() )
    {
        return m_navMap.find(filePathName);
    }
    return m_navMap.end();
}

void CEditorWrapper::clearNavItem(const t_string& filePathName, bool bEraseItem)
{
    t_navmap::iterator itr = getNavItr(filePathName);
    if ( itr != m_navMap.end() )
    {
        itr->second.Clear();
        if ( bEraseItem )
        {
            m_navMap.erase(itr);
        }
    }
}

const CEditorWrapper::t_string& CEditorWrapper::getCurrentFilePathName()
{
    if ( m_currentFilePathName.empty() )
    {
        m_currentFilePathName = ewGetFilePathName();
    }
    return m_currentFilePathName;
}

void CEditorWrapper::clearCurrentFilePathName()
{
    m_currentFilePathName.clear();
}

CEditorWrapper::t_string CEditorWrapper::ewGetNavigateBackwardHint()
{
    t_navmap::const_iterator itr = getNavItr( getCurrentFilePathName() );
    if ( itr != m_navMap.end() )
    {
        return itr->second.GetBackwardHint();
    }

    return t_string(_T(""));
}

CEditorWrapper::t_string CEditorWrapper::ewGetNavigateForwardHint()
{
    t_navmap::const_iterator itr = getNavItr( getCurrentFilePathName() );
    if ( itr != m_navMap.end() )
    {
        return itr->second.GetForwardHint();
    }

    return t_string(_T(""));
}

bool CEditorWrapper::ewHasNavigationHistory()
{
    t_navmap::const_iterator itr = getNavItr( getCurrentFilePathName() );
    if ( itr != m_navMap.end() )
    {
        return !itr->second.IsEmpty();
    }
    return false;
}

void CEditorWrapper::ewSetNavigationPoint(const t_string& hint, bool incPos )
{
    const t_string filePathName = ewGetFilePathName();
    if ( filePathName == getCurrentFilePathName() )
    {
        int selEnd;
        int selStart = ewGetSelectionPos(selEnd);

        t_navmap::iterator itr = getNavItr(filePathName);
        if ( itr != m_navMap.end() )
        {
            CNavigationHistory& nh = itr->second;
            if ( nh.IsEmpty() || (nh.GetLastItem().selPos != selStart) )
            {
                nh.Add(selStart, hint, incPos);
            }
        }
        else
        {
            CNavigationHistory nh;

            nh.Add(selStart, hint, incPos);
            m_navMap.insert( std::make_pair(filePathName, nh) );
        }
    }
}

void CEditorWrapper::ewOnFileActivated()
{
    const t_string filePathName = ewGetFilePathName();
    if ( m_currentFilePathName != filePathName )
    {
        m_currentFilePathName = filePathName;
        ewDoParseFile(false);
    }
}

void CEditorWrapper::ewOnFileClosed()
{
    clearNavItem(m_currentFilePathName, true);

    m_currentFilePathName.clear();
    if ( m_pDlg && m_pDlg->GetHwnd() && m_pDlg->IsWindowVisible() )
    {
        //m_pDlg->ParseFile(NULL, true);
        m_pDlg->PostMessage(CTagsDlg::WM_FILECLOSED);
    }
}

void CEditorWrapper::ewOnFileOpened()
{
    const t_string filePathName = ewGetFilePathName();
    if ( m_currentFilePathName != filePathName )
    {
        m_currentFilePathName = filePathName;
        clearNavItem(m_currentFilePathName, false);
        ewDoParseFile(false);
        //m_pDlg->PurifyCachedTags();
    }
}

void CEditorWrapper::ewOnFileReloaded()
{
    const t_string filePathName = ewGetFilePathName();
    clearNavItem(filePathName, false);

    if ( filePathName == getCurrentFilePathName() )
    {
        ewDoParseFile(true);
    }
}

void CEditorWrapper::ewOnFileSaved()
{
    const t_string filePathName = ewGetFilePathName();
    if ( filePathName == getCurrentFilePathName() )
    {
        if ( m_pDlg->GetOptions().getBool(CTagsDlgData::OPT_BEHAVIOR_PARSEONSAVE) )
        {
            clearNavItem(filePathName, false);
            ewDoParseFile(true);
        }
    }
}

void CEditorWrapper::ewOnSelectionChanged()
{
    const t_string filePathName = ewGetFilePathName();
    if ( filePathName == getCurrentFilePathName() )
    {
        m_pDlg->UpdateCurrentItem();
    }
}
