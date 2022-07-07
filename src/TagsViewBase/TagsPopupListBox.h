#ifndef _TAGS_POPUP_LIST_BOX_H_
#define _TAGS_POPUP_LIST_BOX_H_
//---------------------------------------------------------------------------
#include "win32++/include/wxx_setup.h"
#include "win32++/include/wxx_stdcontrols.h"
#include "TagsCommon.h"
#include "TagsDlgChild.h"

using Win32xx::CListBox;

class CTagsPopupListBox : public CListBox, public CTagsDlgChild
{
    public:
        LRESULT DirectMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            return WndProc(uMsg, wParam, lParam);
        }

    protected:
        virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};

//---------------------------------------------------------------------------
#endif
