#include "TagsDlgChild.h"

using namespace TagsCommon;

t_string CTagsDlgChild::getTooltip(const tTagData* pTagData)
{
    t_string s;

    s.reserve(200);
    s += pTagData->tagPattern;
    s += _T("\nscope: ");
    if ( !pTagData->tagScope.empty() )
        s += pTagData->tagScope;
    else
        s += _T("(global)");
    s += _T("\ntype: ");
    s += pTagData->tagType;

    if ( pTagData->hasFilePath() )
    {
        TCHAR szNum[20];

        s += _T("\nfile: ");
        s += getFileName(pTagData);
        s += _T(':');
        ::wsprintf(szNum, _T("%d"), pTagData->line);
        s += szNum;
    }

    return s;
}
