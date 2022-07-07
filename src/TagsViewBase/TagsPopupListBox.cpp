#include "TagsPopupListBox.h"
#include "TagsDlg.h"

using namespace TagsCommon;

LRESULT CTagsPopupListBox::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( m_pDlg )
    {
        if ( uMsg == WM_CHAR )
        {
            //return 0;
        }
        else if ( uMsg == WM_KEYDOWN )
        {
            switch ( wParam )
            {
                case VK_RETURN:
                {
                    const tTagData* pTag = nullptr;
                    int nItem = GetCurSel();
                    if ( nItem != LB_ERR )
                    {
                        pTag = (const tTagData *) GetItemDataPtr(nItem);
                    }
                    this->Destroy();
                    if ( pTag )
                    {
                        m_pDlg->PostMessage(CTagsDlg::WM_TAGDBLCLICKED, 0, (LPARAM) pTag);
                    }
                    return 0;
                }
                case VK_ESCAPE:
                {
                    this->Destroy();
                    m_pDlg->m_pEdWr->ewDoSetFocus();
                    return 0;
                }
            }
        }
        else if ( uMsg == WM_NOTIFY )
        {
        }
    }

    return WndProcDefault(uMsg, wParam, lParam);
}
