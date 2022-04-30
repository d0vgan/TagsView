#ifndef _options_manager_h_
#define _options_manager_h_
//----------------------------------------------------------------------------
#include "c_base/types.h"
#include <vector>
#include <string>
#include <windows.h>
#include <TCHAR.h>

class COption
{
    public:
        enum optt_type {
            OPTT_NONE    = 0x0000,
            OPTT_INT     = 0x0001,
            OPTT_HEXINT  = 0x0003, // includes OPTT_INT
            OPTT_DATA    = 0x0010,
            OPTT_STR     = 0x0020,
            OPTT_MASK    = 0x00FF
        };

        enum optf_type {
            OPTF_INTERNAL  = 0x0000,
            OPTF_READONLY  = 0x0100,
            OPTF_READWRITE = 0x0300,
            OPTF_MASK      = 0xFF00
        };

        typedef c_base::byte_t              byte_tt;

    protected:
        std::vector<byte_tt> _bufData;
        unsigned int _uintData;
        unsigned int _type;

    public:
        COption();
        ~COption();

        void         clear();
        bool         equal(const COption& opt) const;
        bool         getBool() const;
        const void*  getData(int* psize = NULL) const;
        int          getData(void* pdata, int max_size) const;
        int          getInt() const;
        const TCHAR* getStr(int* plen = NULL) const;
        int          getStr(TCHAR* pstr, int max_len) const;
        unsigned int getUint() const;
 inline bool         isBool() const   { return ((_type & OPTT_INT) == OPTT_INT);       }
 inline bool         isData() const   { return ((_type & OPTT_DATA) == OPTT_DATA);     }
 inline bool         isEmpty() const  { return ((_type & OPTT_MASK) == 0);             }
 inline bool         isHexInt() const { return ((_type & OPTT_HEXINT) == OPTT_HEXINT); }
 inline bool         isInt() const    { return ((_type & OPTT_HEXINT) != 0);           }
 inline bool         isStr() const    { return ((_type & OPTT_STR) == OPTT_STR);       }
 inline bool         isUint() const   { return ((_type & OPTT_HEXINT) != 0);           }
        void         setBool(bool bval);
        void         setData(const void* pdata, int nsize);
        void         setHexInt(unsigned int uval);
        void         setInt(int ival);
        void         setStr(const TCHAR* pstr, int nlen = -1);
        void         setUint(unsigned int uval);
 inline unsigned int type() const  { return (_type & OPTT_MASK); }

};

class CNamedOption : public COption
{
    protected:
        std::basic_string<TCHAR> _group;
        std::basic_string<TCHAR> _name;
        COption                  _saved;

    public:
        CNamedOption() : COption() { }

        void    clear() {
                    COption::clear();
                    _saved.clear();
                    _group.clear();
                    _name.clear();
                }

        LPCTSTR getOptionGroup() const {
                    return _group.empty() ? _T("\0") : _group.c_str();
                }
        LPCTSTR getOptionName() const {
                    return _name.empty() ? _T("\0") : _name.c_str();
                }
        void    setOptionGroup(LPCTSTR szOptGroup) {
                    _group = szOptGroup;
                }
        void    setOptionName(LPCTSTR szOptName) {
                    _name = szOptName;
                }
        void    setOptionReadWriteMode(unsigned int mode = OPTF_READWRITE) {
                    _type = ((_type & OPTT_MASK) | mode);
                }
 inline bool    isOptionInternal() const {
                    return ((_type & OPTF_MASK) == OPTF_INTERNAL);
                }
 inline bool    isOptionReadOnly() const {
                    return ((_type & OPTF_MASK) == OPTF_READONLY);
                }
 inline bool    isOptionReadWrite() const {
                    return ((_type & OPTF_MASK) == OPTF_READWRITE);
                }
 inline bool    isOptionChanged() const  { return !equal(_saved); }

        void    updateSavedValue();
};

class COptionsReaderWriter;

class COptionsManager
{
    public:
        enum eConsts {
            MAX_STRSIZE    = 1024,
            MAX_NUMSTRSIZE = 80
        };

        typedef std::vector<CNamedOption> options_tt;
        typedef options_tt::iterator opt_itr;
        typedef options_tt::const_iterator const_opt_itr;

    protected:
        options_tt _options;

    protected:
        void _AddNewOption(unsigned int id, LPCTSTR szOptGroup, LPCTSTR szOptName,
               unsigned int mode);
        void _updateSavedValue(unsigned int id);

    public:
        COptionsManager();
        ~COptionsManager();

        bool         getBool(unsigned int id) const;
        const void*  getData(unsigned int id, int* psize = NULL) const;
        int          getData(unsigned int id, void* pdata, int nsize) const;
        int          getInt(unsigned int id) const;
        const TCHAR* getStr(unsigned int id, int* plen = NULL) const;
        int          getStr(unsigned int id, TCHAR* pstr, int nlen) const;
        unsigned int getUint(unsigned int id) const;
        bool         isEmpty(unsigned int id) const;
        void         setBool(unsigned int id, bool bval);
        void         setData(unsigned int id, const void* pdata, int nsize);
        void         setHexInt(unsigned int id, unsigned int uval);
        void         setInt(unsigned int id, int ival);
        void         setStr(unsigned int id, const TCHAR* pstr, int nlen = -1);
        void         setUint(unsigned int id, unsigned int uval);
        unsigned int type(unsigned int id) const;

        void AddBool(unsigned int id, LPCTSTR szOptGroup, LPCTSTR szOptName,
               bool bValue, unsigned int mode = COption::OPTF_READWRITE);
        void AddInt(unsigned int id, LPCTSTR szOptGroup, LPCTSTR szOptName,
               int nValue, unsigned int mode = COption::OPTF_READWRITE);
        void AddUint(unsigned int id, LPCTSTR szOptGroup, LPCTSTR szOptName,
               unsigned int uValue, unsigned int mode = COption::OPTF_READWRITE);
        void AddHexInt(unsigned int id, LPCTSTR szOptGroup, LPCTSTR szOptName,
               unsigned int uValue, unsigned int mode = COption::OPTF_READWRITE);
        void AddStr(unsigned int id, LPCTSTR szOptGroup, LPCTSTR szOptName,
               LPCTSTR szValue, unsigned int mode = COption::OPTF_READWRITE);
        void AddData(unsigned int id, LPCTSTR szOptGroup, LPCTSTR szOptName,
               const void* pValue, int nSize, unsigned int mode = COption::OPTF_READWRITE);
        void DeleteOption(unsigned int id);
        bool HasOptions() const  { return !_options.empty(); }
        int  ReadOptions(COptionsReaderWriter* prw);
        void ReserveMemory(int nOptionsCount); // avoids memory re-allocations
        int  SaveOptions(COptionsReaderWriter* prw);
};

class COptionsReaderWriter
{
    public:
        virtual ~COptionsReaderWriter() { }

        virtual bool beginReadOptions() { return true; }
        virtual void endReadOptions() { }
        virtual bool readDataOption(COptionsManager::opt_itr itr) = 0;
        virtual bool readHexIntOption(COptionsManager::opt_itr itr) = 0;
        virtual bool readIntOption(COptionsManager::opt_itr itr) = 0;
        virtual bool readStrOption(COptionsManager::opt_itr itr) = 0;

        virtual bool beginWriteOptions() { return true; }
        virtual void endWriteOptions() { }
        virtual bool writeDataOption(COptionsManager::const_opt_itr itr) = 0;
        virtual bool writeHexIntOption(COptionsManager::const_opt_itr itr) = 0;
        virtual bool writeIntOption(COptionsManager::const_opt_itr itr) = 0;
        virtual bool writeStrOption(COptionsManager::const_opt_itr itr) = 0;
};

class CIniOptionsReaderWriter : public COptionsReaderWriter
{
    public:
        LPCTSTR GetIniFilePathName() const { return _iniFilePathName.c_str(); }
        void    SetIniFilePathName(LPCTSTR cszIniFilePathName);

        virtual bool beginReadOptions() { return !_iniFilePathName.empty(); }
        virtual bool readDataOption(COptionsManager::opt_itr itr);
        virtual bool readHexIntOption(COptionsManager::opt_itr itr);
        virtual bool readIntOption(COptionsManager::opt_itr itr);
        virtual bool readStrOption(COptionsManager::opt_itr itr);

        virtual bool beginWriteOptions() { return !_iniFilePathName.empty(); }
        virtual bool writeDataOption(COptionsManager::const_opt_itr itr);
        virtual bool writeHexIntOption(COptionsManager::const_opt_itr itr);
        virtual bool writeIntOption(COptionsManager::const_opt_itr itr);
        virtual bool writeStrOption(COptionsManager::const_opt_itr itr);

    protected:
        std::basic_string<TCHAR> _iniFilePathName;
};

//----------------------------------------------------------------------------
#endif
