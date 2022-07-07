#include "TagsDlgChild.h"

using namespace TagsCommon;

t_string CTagsDlgChild::getTooltip(const tTagData* pTag)
{
    t_string s;

    s.reserve(200);
    s += pTag->tagPattern;
    s += _T("\nscope: ");
    if ( !pTag->tagScope.empty() )
        s += pTag->tagScope;
    else
        s += _T("(global)");
    s += _T("\ntype: ");
    s += pTag->tagType;

    if ( pTag->hasFilePath() )
    {
        TCHAR szNum[20];

        s += _T("\nfile: ");
        s += getFileName(pTag);
        s += _T(':');
        ::wsprintf(szNum, _T("%d"), pTag->line);
        s += szNum;
    }

    return s;
}

t_string CTagsDlgChild::getPopupListBoxItem(const tTagData* pTag)
{
    t_string s;

    s.reserve(200);
    s += pTag->tagPattern;
    s += _T("\nscope: ");
    if ( !pTag->tagScope.empty() )
        s += pTag->tagScope;
    else
        s += _T("(global)");

    if ( pTag->hasFilePath() )
    {
        TCHAR szNum[20];

        s += _T(",  file: ");
        s += getFileName(pTag);
        s += _T(':');
        ::wsprintf(szNum, _T("%d"), pTag->line);
        s += szNum;
    }

    return s;
}
