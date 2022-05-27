#ifndef _TAGS_TREE_VIEW_H_
#define _TAGS_TREE_VIEW_H_
//---------------------------------------------------------------------------
#include "win32++/include/wxx_treeview.h"
#include "TagsCommon.h"
#include "TagsDlgChild.h"

using Win32xx::CTreeView;

class CTagsTreeView : public CTreeView, public CTagsDlgChild
{
    public:
        LRESULT DirectMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            return WndProc(uMsg, wParam, lParam);
        }

    protected:
        virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    protected:
        TagsCommon::t_string m_lastTooltipText;
};

//---------------------------------------------------------------------------
#endif
