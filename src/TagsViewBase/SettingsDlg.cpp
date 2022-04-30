#include "SettingsDlg.h"
#include "TagsDlg.h"
#include "resource.h"

CSettingsDlg::CSettingsDlg(COptionsManager& opt) : CDialog(IDD_SETTINGS)
, m_opt(opt)
, m_hChParseOnSave(NULL)
, m_hCdEditColors(NULL)
{
}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::OnCancel()
{
    EndDialog(IDCANCEL);
}

BOOL CSettingsDlg::OnInitDialog()
{
    m_hChParseOnSave = ::GetDlgItem(GetHwnd(), IDC_CH_OPT_PARSEONSAVE);
    m_hCdEditColors = ::GetDlgItem(GetHwnd(), IDC_CH_OPT_EDITORCOLORS);

    if ( m_hChParseOnSave )
    {
        ::SendMessage( m_hChParseOnSave, 
                       BM_SETCHECK, 
                       m_opt.getBool(CTagsDlg::OPT_BEHAVIOR_PARSEONSAVE) ? BST_CHECKED : BST_UNCHECKED,
                       0
                     );
    }

    if ( m_hCdEditColors )
    {
        ::SendMessage( m_hCdEditColors, 
                       BM_SETCHECK, 
                       m_opt.getBool(CTagsDlg::OPT_COLORS_USEEDITORCOLORS) ? BST_CHECKED : BST_UNCHECKED,
                       0
                     );
    }

    return TRUE;
}

void CSettingsDlg::OnOK()
{
    if ( m_hChParseOnSave )
    {
        LRESULT nResult = ::SendMessage(m_hChParseOnSave, BM_GETCHECK, 0, 0);
        m_opt.setBool(CTagsDlg::OPT_BEHAVIOR_PARSEONSAVE, (nResult == BST_CHECKED));
    }

    if ( m_hCdEditColors )
    {
        LRESULT nResult = ::SendMessage(m_hCdEditColors, BM_GETCHECK, 0, 0);
        m_opt.setBool(CTagsDlg::OPT_COLORS_USEEDITORCOLORS, (nResult == BST_CHECKED));
    }

    EndDialog(IDOK);
}
