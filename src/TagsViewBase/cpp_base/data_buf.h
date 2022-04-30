#ifndef _data_buf_h_
#define _data_buf_h_
//---------------------------------------------------------------------------

// Data Buffer for simple data types (unsigned char, wchar_t, int, etc.)
// data_buf template ver. 1.5
// (C) DV, February 2008 - December 2009

namespace cpp_base {
// cpp_base >>>

template <class T> class data_buf
{
    public:
        typedef T        value_type;
        typedef int      size_type;

    protected:
        T*  _data;
        int _data_size;
        int _allocated_size;

    public:
        static void buf_unsafe_copy(T* dst, const T* src, int count);
        static int  buf_unsafe_size(const T* data); // null-terminated

    public:
        data_buf();
        data_buf(const T* data, int count);
        data_buf(const data_buf& data);
        ~data_buf();
        data_buf& append(const T* data, int count);
        data_buf& append(const data_buf& data);
        data_buf& append(T item);
        data_buf& assign(const T* data, int count);
        data_buf& assign(const data_buf& data);
        data_buf& assign(T item);
        const T   at(int index) const;
        T&        at(int index);
 inline int       capacity() const  { return _allocated_size ? (_allocated_size - 1) : 0; }
        void      clear();
        int       compare(const T* data, int count) const;
        int       compare(const data_buf& data) const;
        int       copy(T* dst, int count, int pos = 0) const; // without trailing null
 inline const T*  data() const  { return _data; } // can be NULL
 inline T*        data_ptr()  { return _data; } // unsafe! direct access to _data
 inline bool      empty() const  { return (_data_size == 0); }
        bool      equal(const T* data, int count) const;
        bool      equal(const data_buf& data) const;
        data_buf& erase(int pos, int count = -1); // -1 means till the end
        int       find(const T* data, int count, int pos = 0) const; // -1 if not found
        int       find(const data_buf& data, int pos = 0) const; // -1 if not found
        int       find(T item, int pos = 0) const; // -1 if not found
        void      free_memory();
        data_buf& insert(int pos, const T* data, int count);
        data_buf& insert(int pos, const data_buf& data);
        data_buf& insert(int pos, T item);
        int       replace(const T* dataOld, int countOld,
                          const T* dataNew, int countNew,
                          int pos = 0, int times = -1);
        bool      reserve(int count);
        bool      resize(int count, T item = 0);
        void      set_data_ptr(T* data, int allocated_size, int count);
                    // unsafe! sets _data and so on
                    // may not contain trailing 0 when count==allocated_size
        bool      set_data_size(int count);
                    // unsafe! sets _data_size
                    // may not contain trailing 0 when count==_allocated_size
 inline int       size() const  { return _data_size; }
        void      swap(data_buf& data);

 inline data_buf& operator+=(const data_buf& data)  { return append(data); }
        data_buf& operator+=(const T* data); // null-terminated
 inline data_buf& operator+=(T item)  { return append(item); }
 inline data_buf& operator=(const data_buf& data)  { return assign(data); }
        data_buf& operator=(const T* data); // null-terminated
 inline data_buf& operator=(T item)  { return assign(item); }
 inline bool      operator==(const data_buf& data) const  { return equal(data); }
 inline bool      operator!=(const data_buf& data) const  { return !equal(data); }
 inline bool      operator>(const data_buf& data) const   { return compare(data) > 0; }
 inline bool      operator<(const data_buf& data) const   { return compare(data) < 0; }
 inline bool      operator>=(const data_buf& data) const  { return compare(data) >= 0; }
 inline bool      operator<=(const data_buf& data) const  { return compare(data) <= 0; }
 inline const T   operator[](int index) const  { return _data[index]; }
 inline T&        operator[](int index)  { return _data[index]; }

    protected:
        int replace_eq(const T* dataOld, int countOld,
                       const T* dataNew, int countNew,
                       int pos, int times);
        int replace_gt(const T* dataOld, int countOld,
                       const T* dataNew, int countNew,
                       int pos, int times);
        int replace_lt(const T* dataOld, int countOld,
                       const T* dataNew, int countNew,
                       int pos, int times);
};


/*

// Example 1: data_ptr() and set_data_size()

data_buf<char> s;

s.reserve( 10 );               // allocate a memory (internal buffer)
                               // (now _allocated_size >= 10)
strcpy( s.data_ptr(), "abc" ); // fill the internal buffer with some data
                               // (size of the data must be < _allocated_size)
s.set_data_size( 3 );          // set the actual value of the data size

// Example 2: set_data_ptr()

data_buf<char> s;
char* p = new char[10];

strcpy( p, "abc" );
s.set_data_ptr( p, 10, 3 ); // now 's' handles the memory of 'p'
                            // (do not call 'delete [] p' now)

*/


template <class T> data_buf<T>::data_buf()
{
    _data = NULL;
    _data_size = 0;
    _allocated_size = 0;
}

template <class T> data_buf<T>::data_buf(const T* data, int count)
{
    _data = NULL;
    _data_size = 0;
    _allocated_size = 0;
    append(data, count);
}

template <class T> data_buf<T>::data_buf(const data_buf& data)
{
    _data = NULL;
    _data_size = 0;
    _allocated_size = 0;
    append(data.data(), data.size());
}

template <class T> data_buf<T>::~data_buf()
{
    if ( _data )  delete [] _data;
}

template <class T> data_buf<T>& data_buf<T>::append(const T* data, int count)
{
    if ( data && (count > 0) )
    {
        int selfDataOffset = -1;
        if ( (data >= _data) && (data < _data + _data_size) )
        {
            selfDataOffset = (int) (data - _data);
            if ( count > _data_size - selfDataOffset )
                count = _data_size - selfDataOffset;
        }

        if ( reserve(_data_size + count) )
        {
            buf_unsafe_copy( _data + _data_size,
              (selfDataOffset < 0) ? data : (_data + selfDataOffset), count );
            _data_size += count;
            _data[_data_size] = 0;
        }
    }
    return *this;
}

template <class T> data_buf<T>& data_buf<T>::append(const data_buf& data)
{
    return append(data.data(), data.size());
}

template <class T> data_buf<T>& data_buf<T>::append(T item)
{
    if ( reserve(_data_size + 1) )
    {
        _data[_data_size++] = item;
        _data[_data_size] = 0;
    }
    return *this;
}

template <class T> data_buf<T>& data_buf<T>::assign(const T* data, int count)
{
    if ( data != _data )
    {
        clear();
        // works OK due to implementation of clear() and append()
        append(data, count);
    }
    else
    {
        if ( count < _data_size )
        {
            _data_size = count;
            _data[_data_size] = 0;
        }
    }
    return *this;
}

template <class T> data_buf<T>& data_buf<T>::assign(const data_buf& data)
{
    return assign(data.data(), data.size());
}

template <class T> data_buf<T>& data_buf<T>::assign(T item)
{
    clear();
    return append(item);
}

template <class T> const T data_buf<T>::at(int index) const
{
    return ( (index >= 0) && (index < _data_size) ) ? _data[index] : 0;
}

template <class T> T& data_buf<T>::at(int index)
{
    static T item = 0;

    return ( (index >= 0) && (index < _data_size) ) ? _data[index] : (item = 0);
}

template <class T> void data_buf<T>::buf_unsafe_copy(T* dst, const T* src, int count)
{
    if ( (dst > src) && (dst < src + count) )
    {
        dst += count;
        src += count;
        while ( count-- > 0 )
        {
            *(--dst) = *(--src);
        }
    }
    else if ( dst != src )
    {
        while ( count-- > 0 )
        {
            *(dst++) = *(src++);
        }
    }
}

template <class T> int data_buf<T>::buf_unsafe_size(const T* data)
{
    const T* data0 = data;
    while ( *data )  ++data;
    return (int) (data - data0);
}

template <class T> void data_buf<T>::clear()
{
    _data_size = 0;
    if ( _data )
    {
        _data[0] = 0;
    }
}

template <class T> int data_buf<T>::compare(const T* data, int count) const
{
    if ( (!data) || (count <= 0) )
        return (_data_size > 0) ? 1 : 0;

    if ( !_data_size )
        return -1;

    int i = 0;
    int sz = (_data_size > count) ? count : _data_size;
    do
    {
        if ( _data[i] != data[i] )
            return (_data[i] > data[i]) ? 1 : -1;
    }
    while ( ++i < sz );

    return (_data_size == count) ? 0 : ((_data_size > count) ? 1 : -1);
}

template <class T> int data_buf<T>::compare(const data_buf& data) const
{
    return compare(data.data(), data.size());
}

template <class T> int data_buf<T>::copy(T* dst, int count, int pos ) const
{
    if ( dst && (count > 0) && (pos >= 0) && (pos < _data_size) )
    {
        if ( count > _data_size - pos )
            count = _data_size - pos;
        buf_unsafe_copy(dst, _data + pos, count);
        return count;
    }
    return 0;
}

template <class T> bool data_buf<T>::equal(const T* data, int count) const
{
    if ( (!data) || (count <= 0) )
        return (_data_size == 0);

    if ( count != _data_size )
        return false; // _data_size is 0 when _data is NULL

    count = 0;
    do
    {
        if ( _data[count] != data[count] )
            return false;
    }
    while ( ++count < _data_size );

    return true;
}

template <class T> bool data_buf<T>::equal(const data_buf& data) const
{
    return equal(data.data(), data.size());
}

template <class T> data_buf<T>& data_buf<T>::erase(int pos, int count)
{
    if ( (pos >= 0) && (pos < _data_size) && (count != 0) )
    {
        if ( (count < 0) || (count >= _data_size - pos) )
        {
            _data_size = pos;
        }
        else
        {
            buf_unsafe_copy(_data + pos, _data + pos + count, _data_size - pos - count);
            _data_size -= count;
        }
        _data[_data_size] = 0;
    }
    return *this;
}

template <class T> int data_buf<T>::find(const T* data, int count, int pos ) const
{
    if ( data && (count > 0) && (pos >= 0) )
    {
        while ( pos <= _data_size - count )
        {
            int i = 0;
            while ( _data[pos + i] == data[i] )
            {
                if ( ++i == count )
                    return pos;
            }
            ++pos;
        }
    }
    return -1;
}

template <class T> int data_buf<T>::find(const data_buf& data, int pos ) const
{
    return find(data.data(), data.size(), pos);
}

template <class T> int data_buf<T>::find(T item, int pos ) const
{
    if ( pos >= 0 )
    {
        while ( pos < _data_size )
        {
            if ( _data[pos] != item )
                ++pos;
            else
                return pos;
        }
    }
    return -1;
}

template <class T> void data_buf<T>::free_memory()
{
    if ( _data )
    {
        delete [] _data;
        _data = NULL;
    }
    _data_size = 0;
    _allocated_size = 0;
}

template <class T> data_buf<T>& data_buf<T>::insert(int pos, const T* data, int count)
{
    if ( pos >= 0 )
    {
        if ( pos < _data_size )
        {
            if ( data && (count > 0) )
            {
                int selfDataOffset = -1;
                if ( (data >= _data) && (data < _data + _data_size) )
                {
                    selfDataOffset = (int) (data - _data);
                    if ( count > _data_size - selfDataOffset )
                        count = _data_size - selfDataOffset;
                }

                if ( reserve(_data_size + count) )
                {
                    data_buf<T> temp;
                    if ( selfDataOffset >= 0 )
                    {
                        temp.append(_data + selfDataOffset, count);
                        data = temp.data();
                        if ( !data )
                            return *this;
                    }

                    buf_unsafe_copy( _data + pos + count, _data + pos, _data_size - pos );
                    buf_unsafe_copy( _data + pos, data, count );
                    _data_size += count;
                    _data[_data_size] = 0;
                }
            }
        }
        else if ( pos == _data_size )
        {
            append(data, count);
        }
    }
    return *this;
}

template <class T> data_buf<T>& data_buf<T>::insert(int pos, const data_buf& data)
{
    return insert(pos, data.data(), data.size());
}

template <class T> data_buf<T>& data_buf<T>::insert(int pos, T item)
{
    if ( pos >= 0 )
    {
        if ( pos < _data_size )
        {
            if ( reserve(_data_size + 1) )
            {
                buf_unsafe_copy(_data + pos + 1, _data + pos, _data_size - pos);
                _data[pos] = item;
                _data[++_data_size] = 0;
            }
        }
        else if ( pos == _data_size )
        {
            append(item);
        }
    }
    return *this;
}

template <class T> int data_buf<T>::replace(const T* dataOld, int countOld,
                                            const T* dataNew, int countNew,
                                            int pos , int times )
{
    if ( dataOld &&
         (countOld > 0) &&
         (pos >= 0) &&
         (pos < _data_size) &&
         (times != 0) )
    {
        if ( !dataNew )
            countNew = 0;

        if ( (countNew >= 0) &&
             (pos + countOld <= _data_size) )
        {
            data_buf<T> tempOld;
            data_buf<T> tempNew;

            if ( (dataOld >= _data) &&
                 (dataOld < _data + _data_size) )
            {
                tempOld.append(dataOld, countOld);
                dataOld = tempOld.data();
            }

            if ( (countNew != 0) &&
                 (dataNew >= _data) &&
                 (dataNew < _data + _data_size) )
            {
                tempNew.append(dataNew, countNew);
                dataNew = tempNew.data();
            }

            if ( countNew == countOld )
                return replace_eq(dataOld, countOld, dataNew, countNew, pos, times);
            else if ( countNew > countOld )
                return replace_gt(dataOld, countOld, dataNew, countNew, pos, times);
            else
                return replace_lt(dataOld, countOld, dataNew, countNew, pos, times);
        }
    }
    return 0;
}

template <class T> int data_buf<T>::replace_eq(const T* dataOld, int countOld,
                                               const T* dataNew, int countNew,
                                               int pos, int times)
{
    int n = 0;

    while ( (pos = find(dataOld, countOld, pos)) >= 0 )
    {
        for ( int i = 0; i < countNew; i++ )
        {
            _data[pos++] = dataNew[i];
        }
        if ( ++n == times )
            break;
    }

    return n;
}

template <class T> int data_buf<T>::replace_gt(const T* dataOld, int countOld,
                                               const T* dataNew, int countNew,
                                               int pos, int times)
{
    int         n = 0;
    int         pos0 = 0;
    data_buf<T> temp;

    temp.reserve(_data_size * 2);

    while ( (pos = find(dataOld, countOld, pos)) >= 0 )
    {
        temp.append( _data + pos0, pos - pos0 );
        temp.append( dataNew, countNew );

        if ( ++n == times )
            break;
        else {
            pos += countOld;
            pos0 = pos;
        }
    }

    if ( n > 0 )
    {
        temp.append( _data + pos0, _data_size - pos0 );
        this->swap(temp);
    }

    return n;
}

template <class T> int data_buf<T>::replace_lt(const T* dataOld, int countOld,
                                               const T* dataNew, int countNew,
                                               int pos, int times)
{
    int n = 0;
    int pos0 = 0;
    int sz = 0;

    while ( (pos = find(dataOld, countOld, pos)) >= 0 )
    {
        buf_unsafe_copy( _data + sz, _data + pos0, pos - pos0 );
        sz += (pos - pos0);
        buf_unsafe_copy( _data + sz, dataNew, countNew );
        sz += countNew;

        if ( ++n == times )
            break;
        else {
            pos += countOld;
            pos0 = pos;
        }
    }

    if ( n > 0 )
    {
        buf_unsafe_copy( _data + sz, _data + pos0, _data_size - pos0 );
        sz += (_data_size - pos0);
        _data_size = sz;
        _data[_data_size] = 0;
    }

    return n;
}

template <class T> bool data_buf<T>::reserve(int count)
{
    if ( ++count > _allocated_size )
    {
        if ( count <= 64 )
            count = 64;
        else if ( count <= 256 )
            count = 256;
        else if ( count <= 1024 )
            count = 1024;
        else if ( count <= 8192 )
            count = 8192;
        else if ( count <= 65536 )
            count = 65536;
        else
            count = (1 + count/524288)*524288;

        T* newData = new T[count];
        if ( !newData )
            return false;

        if ( _data )
        {
            int i = 0;
            while ( i < _data_size )
            {
                newData[i] = _data[i];
                ++i;
            }
            delete [] _data;
        }
        _data = newData;
        _data[_data_size] = 0;
        _allocated_size = count;
    }
    return true;
}

template <class T> bool data_buf<T>::resize(int count, T item )
{
    if ( reserve(count) )
    {
        if ( _data_size < count )
        {
            do
            {
                _data[_data_size++] = item;
            }
            while ( _data_size < count );
        }
        else
        {
            _data_size = count;
        }
        _data[_data_size] = 0;
        return true;
    }
    return false;
}

template <class T> void data_buf<T>::set_data_ptr(T* data, int allocated_size, int count)
{
    free_memory();

    if ( data && (allocated_size > 0) )
    {
        _data = data;
        _allocated_size = allocated_size;
        if ( count > 0 )
            _data_size = count;
        if ( _data_size < _allocated_size )
            _data[_data_size] = 0;
    }
}

template <class T> bool data_buf<T>::set_data_size(int count)
{
    if ( (count >= 0) && (count <= _allocated_size) )
    {
        _data_size = count;
        if ( _data_size < _allocated_size )
            _data[_data_size] = 0;
        return true;
    }
    return false;
}

template <class T> void data_buf<T>::swap(data_buf& data)
{
    T*  p = _data;
    int n = _data_size;

    _data = data._data;
    data._data = p;
    _data_size = data._data_size;
    data._data_size = n;
    n = _allocated_size;
    _allocated_size = data._allocated_size;
    data._allocated_size = n;
}

template <class T> data_buf<T>& data_buf<T>::operator+=(const T* data)
{
    if ( data )
    {
        int dataSize = buf_unsafe_size(data);
        append(data, dataSize);
    }
    return *this;
}

template <class T> data_buf<T>& data_buf<T>::operator=(const T* data)
{
    if ( data != _data )
    {
        int dataSize = data ? buf_unsafe_size(data) : 0;
        clear();
        append(data, dataSize);
    }
    return *this;
}

// <<< cpp_base
}

//---------------------------------------------------------------------------
#endif
