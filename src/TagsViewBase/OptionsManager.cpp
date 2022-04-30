#include "OptionsManager.h"
#include "c_base/HexStr.h"
#include <climits>

COption::COption() : _uintData(0), _type(0)
{

}

COption::~COption()
{

}

void COption::clear()
{
    _bufData.clear();
    _uintData = 0;
    _type = _type & OPTF_MASK;
}

bool COption::equal(const COption& opt) const
{
    if ( type() != opt.type() )
        return false;

    if ( isStr() || isData() )
    {
        return (_bufData == opt._bufData);
    }

    return (getUint() == opt.getUint());
}

bool COption::getBool() const
{
    if ( isBool() )
    {
        return _uintData ? true : false;
    }
    return false;
}

const void* COption::getData(int* psize ) const
{
    if ( !_bufData.empty() )
    {
        if ( psize )  *psize = static_cast<int>(_uintData);
        return _bufData.data();
    }
    if ( psize )  *psize = 0;
    return NULL;
}

int COption::getData(void* pdata, int max_size) const
{
    if ( !_bufData.empty() )
    {
        if ( max_size > static_cast<int>(_uintData) )
        {
            max_size = static_cast<int>(_uintData);
        }
        memcpy(pdata, _bufData.data(), max_size);
        return max_size;
    }
    if ( pdata && (max_size > 0) )  *reinterpret_cast<byte_tt *>(pdata) = 0;
    return 0;
}

int COption::getInt() const
{
    if ( isInt() )
    {
        if ( _uintData <= INT_MAX )
            return static_cast<int>(_uintData);

        int i = UINT_MAX - _uintData;
        i += 1;
        return (-i);
    }
    return 0;
}

const TCHAR* COption::getStr(int* plen ) const
{
    if ( !_bufData.empty() )
    {
        if ( plen )
        {
            int len = static_cast<int>(_uintData/sizeof(TCHAR));
            if ( _uintData % sizeof(TCHAR) )  ++len;
            *plen = len;
        }
        return reinterpret_cast<const TCHAR *>(_bufData.data());
    }
    if ( plen )  *plen = 0;
    return _T("\0");
}

int COption::getStr(TCHAR* pstr, int max_len) const
{
    if ( !_bufData.empty() )
    {
        if ( max_len > static_cast<int>(_uintData/sizeof(TCHAR)) )
        {
            max_len = static_cast<int>(_uintData/sizeof(TCHAR));
            if ( _uintData % sizeof(TCHAR) )  ++max_len;
        }
        memcpy(pstr, _bufData.data(), max_len*sizeof(TCHAR));
        max_len /= sizeof(TCHAR);
        pstr[max_len] = 0;
        return max_len;
    }
    if ( pstr )  *pstr = 0;
    return 0;
}

unsigned int COption::getUint() const
{
    if ( isUint() )
    {
        return _uintData;
    }
    return 0;
}

void COption::setBool(bool bval)
{
    clear();
    _type |= OPTT_INT;
    _uintData = (bval ? 1 : 0);
}

void COption::setData(const void* pdata, int nsize)
{
    clear();
    _type |= OPTT_DATA;
    if ( pdata && (nsize > 0) )
    {
        int nn = (nsize % sizeof(TCHAR)) + sizeof(TCHAR);
        _uintData = nsize; // real size in bytes
        _bufData.reserve( nsize + nn ); // plus trailing '\0'
        const byte_tt* p = reinterpret_cast<const byte_tt*>(pdata);
        _bufData.assign(p, p + nsize);
        while ( nn-- > 0 )
        {
            _bufData.push_back(0);
        }
    }
}

void COption::setHexInt(unsigned int uval)
{
    clear();
    _type |= OPTT_HEXINT;
    _uintData = uval;
}

void COption::setInt(int ival)
{
    clear();
    _type |= OPTT_INT;
    if ( ival >= 0 )
    {
        _uintData = ival;
    }
    else
    {
        _uintData = UINT_MAX;
        _uintData -= (-ival);
        _uintData += 1;
    }
}

void COption::setStr(const TCHAR* pstr, int nlen )
{
    clear();
    _type |= OPTT_STR;
    if ( nlen < 0 )
    {
        nlen = pstr ? static_cast<int>(_tcslen(pstr)) : 0;
    }
    if ( nlen > 0 )
    {
        int nn = sizeof(TCHAR);
        _uintData = nlen*sizeof(TCHAR); // real size in bytes
        _bufData.reserve( nlen*sizeof(TCHAR) + nn ); // plus trailing '\0'
        const byte_tt* p = reinterpret_cast<const byte_tt*>(pstr);
        _bufData.assign(p, p + nlen*sizeof(TCHAR));
        while ( nn-- )
        {
            _bufData.push_back(0);
        }
    }
}

void COption::setUint(unsigned int uval)
{
    clear();
    _type |= OPTT_INT;
    _uintData = uval;
}

//----------------------------------------------------------------------------

void CNamedOption::updateSavedValue()
{
    if ( isHexInt() )
    {
        _saved.setHexInt( getUint() );
    }
    else if ( isUint() )
    {
        _saved.setUint( getUint() );
    }
    else if ( isStr() )
    {
        int          nlen = 0;
        const TCHAR* pstr = getStr(&nlen);
        _saved.setStr(pstr, nlen);
    }
    else if ( isData() )
    {
        int         nsize = 0;
        const void* pdata = getData(&nsize);
        _saved.setData(pdata, nsize);
    }
}

//----------------------------------------------------------------------------

COptionsManager::COptionsManager()
{
}

COptionsManager::~COptionsManager()
{
}

bool COptionsManager::getBool(unsigned int id) const
{
    if ( id < _options.size() )
    {
        return _options[id].getBool();
    }
    return false;
}

const void* COptionsManager::getData(unsigned int id, int* psize ) const
{
    if ( id < _options.size() )
    {
        return _options[id].getData(psize);
    }
    if ( psize )  *psize = 0;
    return NULL;
}

int COptionsManager::getData(unsigned int id, void* pdata, int nsize) const
{
    if ( id < _options.size() )
    {
        return _options[id].getData(pdata, nsize);
    }
    return 0;
}

int COptionsManager::getInt(unsigned int id) const
{
    if ( id < _options.size() )
    {
        return _options[id].getInt();
    }
    return 0;
}

const TCHAR* COptionsManager::getStr(unsigned int id, int* plen ) const
{
    if ( id < _options.size() )
    {
        return _options[id].getStr(plen);
    }
    if ( plen )  *plen = 0;
    return _T("\0");
}

int COptionsManager::getStr(unsigned int id, TCHAR* pstr, int nlen) const
{
    if ( id < _options.size() )
    {
        return _options[id].getStr(pstr, nlen);
    }
    if ( pstr )  *pstr = 0;
    return 0;
}

unsigned int COptionsManager::getUint(unsigned int id) const
{
    if ( id < _options.size() )
    {
        return _options[id].getUint();
    }
    return 0;
}

bool COptionsManager::isEmpty(unsigned int id) const
{
    if ( id < _options.size() )
    {
        return _options[id].isEmpty();
    }
    return true;
}

void COptionsManager::setBool(unsigned int id, bool bval)
{
    if ( id < _options.size() )
    {
        _options[id].setBool(bval);
    }
}

void COptionsManager::setData(unsigned int id, const void* pdata, int nsize)
{
    if ( id < _options.size() )
    {
        _options[id].setData(pdata, nsize);
    }
}

void COptionsManager::setHexInt(unsigned int id, unsigned int uval)
{
    if ( id < _options.size() )
    {
        _options[id].setHexInt(uval);
    }
}

void COptionsManager::setInt(unsigned int id, int ival)
{
    if ( id < _options.size() )
    {
        _options[id].setInt(ival);
    }
}

void COptionsManager::setStr(unsigned int id, const TCHAR* pstr, int nlen )
{
    if ( id < _options.size() )
    {
        _options[id].setStr(pstr, nlen);
    }
}

void COptionsManager::setUint(unsigned int id, unsigned int uval)
{
    if ( id < _options.size() )
    {
        _options[id].setUint(uval);
    }
}

unsigned int COptionsManager::type(unsigned int id) const
{
    if ( id < _options.size() )
    {
        return _options[id].type();
    }
    return 0;
}


void COptionsManager::_AddNewOption(unsigned int id, LPCTSTR szOptGroup,
  LPCTSTR szOptName, unsigned int mode)
{
    if ( id >= _options.size() )
        _options.resize(id + 1);

    if ( id < _options.size() )
    {
        options_tt::reference optRef = _options[id];
        optRef.setOptionGroup(szOptGroup);
        optRef.setOptionName(szOptName);
        optRef.setOptionReadWriteMode(mode);
    }
}

void COptionsManager::_updateSavedValue(unsigned int id)
{
    if ( id < _options.size() )
    {
        _options[id].updateSavedValue();
    }
}

void COptionsManager::AddBool(unsigned int id, LPCTSTR szOptGroup,
  LPCTSTR szOptName, bool bValue, unsigned int mode )
{
    _AddNewOption(id, szOptGroup, szOptName, mode);
    setBool(id, bValue);
    _updateSavedValue(id);
}

void COptionsManager::AddInt(unsigned int id, LPCTSTR szOptGroup,
  LPCTSTR szOptName, int nValue, unsigned int mode )
{
    _AddNewOption(id, szOptGroup, szOptName, mode);
    setInt(id, nValue);
    _updateSavedValue(id);
}

void COptionsManager::AddUint(unsigned int id, LPCTSTR szOptGroup,
  LPCTSTR szOptName, unsigned int uValue, unsigned int mode )
{
    _AddNewOption(id, szOptGroup, szOptName, mode);
    setUint(id, uValue);
    _updateSavedValue(id);
}

void COptionsManager::AddHexInt(unsigned int id, LPCTSTR szOptGroup,
  LPCTSTR szOptName, unsigned int uValue, unsigned int mode )
{
    _AddNewOption(id, szOptGroup, szOptName, mode);
    setHexInt(id, uValue);
    _updateSavedValue(id);
}

void COptionsManager::AddStr(unsigned int id, LPCTSTR szOptGroup,
  LPCTSTR szOptName, LPCTSTR szValue, unsigned int mode )
{
    _AddNewOption(id, szOptGroup, szOptName, mode);
    setStr(id, szValue);
    _updateSavedValue(id);
}

void COptionsManager::AddData(unsigned int id, LPCTSTR szOptGroup,
  LPCTSTR szOptName, const void* pValue, int nSize, unsigned int mode )
{
    _AddNewOption(id, szOptGroup, szOptName, mode);
    setData(id, pValue, nSize);
    _updateSavedValue(id);
}

void COptionsManager::DeleteOption(unsigned int id)
{
    if ( id < _options.size() )
    {
        _options[id].clear();
    }
}

int COptionsManager::ReadOptions(COptionsReaderWriter* prw)
{
    if ( !prw )
        return -1;

    if ( !prw->beginReadOptions() )
        return -1;

    int rd = 0;
    opt_itr itr = _options.begin();
    while ( itr != _options.end() )
    {
        if ( (itr->isOptionReadWrite() || itr->isOptionReadOnly()) &&
             (itr->getOptionGroup()[0]) &&
             (itr->getOptionName()[0])
           )
        {
            bool isRead = false;

            if ( itr->isHexInt() )
            {
                isRead = prw->readHexIntOption(itr);
            }
            else if ( itr->isInt() )
            {
                isRead = prw->readIntOption(itr);
            }
            else if ( itr->isStr() )
            {
                isRead = prw->readStrOption(itr);
            }
            else if ( itr->isData() )
            {
                isRead = prw->readDataOption(itr);
            }

            if ( isRead )
            {
                itr->updateSavedValue();
                ++rd;
            }
        }
        ++itr;
    }

    prw->endReadOptions();

    return rd;
}

void COptionsManager::ReserveMemory(int nOptionsCount)
{
    if ( nOptionsCount > 0)
        _options.reserve(nOptionsCount);
}

int COptionsManager::SaveOptions(COptionsReaderWriter* prw)
{
    if ( !prw )
        return -1;

    if ( !prw->beginWriteOptions() )
        return -1;

    int wr = 0;
    opt_itr itr = _options.begin();
    while ( itr != _options.end() )
    {
        if ( (itr->isOptionReadWrite()) &&
             (itr->getOptionGroup()[0]) &&
             (itr->getOptionName()[0]) &&
             (itr->isOptionChanged())
           )
        {
            bool isWritten = false;

            if ( itr->isHexInt() )
            {
                isWritten = prw->writeHexIntOption(itr);
            }
            else if ( itr->isInt() )
            {
                isWritten = prw->writeIntOption(itr);
            }
            else if ( itr->isStr() )
            {
                isWritten = prw->writeStrOption(itr);
            }
            else if ( itr->isData() )
            {
                isWritten = prw->writeDataOption(itr);
            }

            if ( isWritten )
            {
                itr->updateSavedValue();
                ++wr;
            }
        }
        ++itr;
    }

    prw->endWriteOptions();

    return wr;
}

//----------------------------------------------------------------------------

void CIniOptionsReaderWriter::SetIniFilePathName(LPCTSTR cszIniFilePathName)
{
    _iniFilePathName = cszIniFilePathName;
}

bool CIniOptionsReaderWriter::readDataOption(COptionsManager::opt_itr itr)
{
    TCHAR str[COptionsManager::MAX_STRSIZE];

    str[0] = _T('~');
    str[1] = 0;
    unsigned int len = ::GetPrivateProfileString( itr->getOptionGroup(),
        itr->getOptionName(), str, str,
        COptionsManager::MAX_STRSIZE - 1, _iniFilePathName.c_str() );
    if ( str[0] != _T('~') )
    {
        if ( len > 0 )
        {
            len = 2 + len/(2*sizeof(TCHAR));
            BYTE* buf = new BYTE[len];
            if ( buf )
            {
                len = c_base::_thexstr2buf( str, buf, len );
                itr->setData( (const void *) buf, len );
                delete [] buf;

                return true;
            }
        }
        else
        {
            itr->setData(NULL, 0);

            return true;
        }
    }

    return false;
}

bool CIniOptionsReaderWriter::readHexIntOption(COptionsManager::opt_itr itr)
{
    TCHAR str[COptionsManager::MAX_NUMSTRSIZE];

    str[0] = _T('0');
    str[1] = _T('x');
    str[2] = _T('0');
    str[3] = 0;
    ::GetPrivateProfileString( itr->getOptionGroup(),
        itr->getOptionName(), itr->getStr(), str + 2,
        COptionsManager::MAX_NUMSTRSIZE - 3, _iniFilePathName.c_str() );
    unsigned int uval = _tcstoul(str, NULL, 16);
    itr->setHexInt(uval);

    return true;
}

bool CIniOptionsReaderWriter::readIntOption(COptionsManager::opt_itr itr)
{
    unsigned int i = ::GetPrivateProfileInt( itr->getOptionGroup(),
        itr->getOptionName(), itr->getInt(), _iniFilePathName.c_str() );
    itr->setUint(i);

    return true;
}

bool CIniOptionsReaderWriter::readStrOption(COptionsManager::opt_itr itr)
{
    TCHAR str[COptionsManager::MAX_STRSIZE];

    str[0] = 0;
    unsigned int len = ::GetPrivateProfileString( itr->getOptionGroup(),
        itr->getOptionName(), itr->getStr(), str,
        COptionsManager::MAX_STRSIZE - 1, _iniFilePathName.c_str() );
    itr->setStr(str, len);

    return true;
}

bool CIniOptionsReaderWriter::writeDataOption(COptionsManager::const_opt_itr itr)
{
    TCHAR str[COptionsManager::MAX_STRSIZE];

    int ndata = 0;
    c_base::byte_t* pdata = (c_base::byte_t *) itr->getData(&ndata);
    ndata = c_base::_tbuf2hexstr(pdata, ndata, str, COptionsManager::MAX_STRSIZE, NULL);
    if ( ::WritePrivateProfileString( itr->getOptionGroup(),
        itr->getOptionName(), str, _iniFilePathName.c_str() ) )
    {
        return true;
    }

    return false;
}

bool CIniOptionsReaderWriter::writeHexIntOption(COptionsManager::const_opt_itr itr)
{
    TCHAR str[COptionsManager::MAX_NUMSTRSIZE];

    ::wsprintf( str, _T("%X"), itr->getUint() );
    if ( ::WritePrivateProfileString(itr->getOptionGroup(),
        itr->getOptionName(), str, _iniFilePathName.c_str()) )
    {
        return true;
    }

    return false;
}

bool CIniOptionsReaderWriter::writeIntOption(COptionsManager::const_opt_itr itr)
{
    TCHAR str[COptionsManager::MAX_NUMSTRSIZE];

    ::wsprintf( str, _T("%d"), itr->getInt() );
    if ( ::WritePrivateProfileString(itr->getOptionGroup(),
        itr->getOptionName(), str, _iniFilePathName.c_str()) )
    {
        return true;
    }

    return false;
}

bool CIniOptionsReaderWriter::writeStrOption(COptionsManager::const_opt_itr itr)
{
    if ( ::WritePrivateProfileString(itr->getOptionGroup(),
        itr->getOptionName(), itr->getStr(), _iniFilePathName.c_str()) )
    {
        return true;
    }

    return false;
}
