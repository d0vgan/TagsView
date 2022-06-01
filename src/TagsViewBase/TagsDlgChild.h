#ifndef _TAGS_DLG_CHILD_H_
#define _TAGS_DLG_CHILD_H_
//---------------------------------------------------------------------------
#include "TagsCommon.h"

class CTagsDlg;

class CTagsDlgChild
{
public:
    CTagsDlgChild() : m_pDlg(NULL) { }
    void SetTagsDlg(CTagsDlg* pDlg) { m_pDlg = pDlg; }

protected:
    static TagsCommon::t_string getTooltip(const TagsCommon::tTagData* pTag);

protected:
    CTagsDlg* m_pDlg;
};

//---------------------------------------------------------------------------
#endif
