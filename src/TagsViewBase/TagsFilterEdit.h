#ifndef _TAGS_FILTER_EDIT_H_
#define _TAGS_FILTER_EDIT_H_
//---------------------------------------------------------------------------
#include "win32++/include/wxx_setup.h"
#include "win32++/include/wxx_wincore.h"
#include "TagsCommon.h"
#include "TagsDlgChild.h"

using Win32xx::CWnd;

class CTagsFilterEdit : public CWnd, public CTagsDlgChild
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
