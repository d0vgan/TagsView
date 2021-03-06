#include "TagsDlgData.h"
#include "SettingsDlg.h"
#include "resource.h"

namespace
{
    struct sCheckBoxOption
    {
        UINT uCheckBoxId;
        UINT uOptionId;
    };

    const sCheckBoxOption arrCheckOptions[] = 
    {
        { IDC_CH_OPT_PARSEONSAVE,           CTagsDlgData::OPT_BEHAVIOR_PARSEONSAVE },
        { IDC_CH_OPT_EDITORCOLORS,          CTagsDlgData::OPT_COLORS_USEEDITORCOLORS },
        { IDC_CH_OPT_SHOWTOOLTIPS,          CTagsDlgData::OPT_VIEW_SHOWTOOLTIPS },
        { IDC_CH_OPT_NESTEDSCOPETREE,       CTagsDlgData::OPT_VIEW_NESTEDSCOPETREE },
        { IDC_CH_OPT_DBLCLICKTREE,          CTagsDlgData::OPT_VIEW_DBLCLICKTREE },
        { IDC_CH_OPT_ESCFOCUSTOEDITOR,      CTagsDlgData::OPT_VIEW_ESCFOCUSTOEDITOR },
        { IDC_CH_OPT_CTAGSSTDOUT,           CTagsDlgData::OPT_CTAGS_OUTPUTSTDOUT },
        { IDC_CH_OPT_SCANFOLDER,            CTagsDlgData::OPT_CTAGS_SCANFOLDER },
        { IDC_CH_OPT_SCANFOLDERRECURSIVELY, CTagsDlgData::OPT_CTAGS_SCANFOLDERRECURSIVELY },
    };
}

CSettingsDlg::CSettingsDlg(COptionsManager& opt) : CDialog(IDD_SETTINGS), m_opt(opt)
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
    for ( const auto& chOption : arrCheckOptions )
    {
        HWND hCheckBox = GetDlgItem(chOption.uCheckBoxId) ;
        if ( hCheckBox )
        {
            ::SendMessage(hCheckBox, BM_SETCHECK, m_opt.getBool(chOption.uOptionId) ? BST_CHECKED : BST_UNCHECKED, 0);
        }
    }

    OnChScanFolderClicked();

    return TRUE;
}

BOOL CSettingsDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch ( LOWORD(wParam) )
    {
        case IDC_CH_OPT_SCANFOLDER:
            if ( HIWORD(wParam) == BN_CLICKED )
            {
                OnChScanFolderClicked();
            }
            break;
    }

    return FALSE;
}

void CSettingsDlg::OnOK()
{
    for ( const auto& chOption : arrCheckOptions )
    {
        HWND hCheckBox = GetDlgItem(chOption.uCheckBoxId);
        if ( hCheckBox )
        {
            LRESULT nResult = ::SendMessage(hCheckBox, BM_GETCHECK, 0, 0);
            m_opt.setBool(chOption.uOptionId, (nResult == BST_CHECKED));
        }
    }

    EndDialog(IDOK);
}

void CSettingsDlg::OnChScanFolderClicked()
{
    HWND hCheckBox = GetDlgItem(IDC_CH_OPT_SCANFOLDER);
    if ( hCheckBox )
    {
        LRESULT nResult = ::SendMessage(hCheckBox, BM_GETCHECK, 0, 0);
        hCheckBox = GetDlgItem(IDC_CH_OPT_SCANFOLDERRECURSIVELY);
        if ( hCheckBox )
        {
            ::EnableWindow(hCheckBox, (nResult == BST_CHECKED) ? TRUE : FALSE);
        }
    }
}
