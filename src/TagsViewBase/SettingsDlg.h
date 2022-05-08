#ifndef _SETTINGS_DLG_H_
#define _SETTINGS_DLG_H_
//---------------------------------------------------------------------------
#include "win32++/include/wxx_dialog.h"
#include "win32++/include/wxx_stdcontrols.h"
#include "OptionsManager.h"

using Win32xx::CDialog;
using Win32xx::CButton;

class CSettingsDlg : public CDialog
{
    public:
        CSettingsDlg(COptionsManager& opt);
        virtual ~CSettingsDlg();

    protected:
        virtual void OnCancel() override;
        virtual BOOL OnInitDialog() override;
        virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;
        virtual void OnOK() override;

        void OnChScanFolderClicked();

    private:
        COptionsManager& m_opt;
};

//---------------------------------------------------------------------------
#endif
