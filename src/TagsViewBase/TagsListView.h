#ifndef _TAGS_LIST_VIEW_H_
#define _TAGS_LIST_VIEW_H_
//---------------------------------------------------------------------------
#include "win32++/include/wxx_listview.h"
#include "TagsCommon.h"
#include "TagsDlgChild.h"

using Win32xx::CListView;

class CTagsListView : public CListView, public CTagsDlgChild
{
    public:
        LRESULT DirectMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            return WndProc(uMsg, wParam, lParam);
        }

    protected:
        virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    protected:
        CPoint m_lastPoint;
        TagsCommon::t_string m_lastTooltipText;
};

//---------------------------------------------------------------------------
#endif
