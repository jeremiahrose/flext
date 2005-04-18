/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

/*! \file flmap.h
	\brief special map class for all 32-bit key/value-pairs   
*/

#ifndef __FLMAP_H
#define __FLMAP_H

#include "flprefix.h"

/*!	\defgroup FLEXT_SUPPORT Flext support classes
	@{
*/

class FLEXT_SHARE TableAnyMap
{
protected:
    virtual TableAnyMap *_newmap(TableAnyMap *parent) = 0;
    virtual void _delmap(TableAnyMap *map) = 0;

    struct Data {
        void operator()(size_t k,void *v) { key = k,value = v; }
        void operator =(void *v) { value = v; }

        size_t key;
        void *value;
    };

    TableAnyMap(TableAnyMap *p,Data *dt)
        : data(dt)
        , parent(p),left(NULL),right(NULL) 
        , n(0)
    {}

    virtual ~TableAnyMap();

#if 0
    void check(int tsize) { if(n) _check(tsize); }
#else
    void check(int tsize) {}
#endif

    void *insert(int tsize,size_t k,void *t)
    {
        void *r;
        if(n) 
            r = _set(tsize,k,t);
        else {
            data[n++](k,t);
            r = NULL;
        }
        check(tsize);
        return r;
    }

    void *find(int tsize,size_t k) const { return n?_find(tsize,k):NULL; }

    void *remove(int tsize,size_t k) { void *r = n?_remove(tsize,k):NULL; check(tsize); return r; }

    virtual void clear();

    class FLEXT_SHARE iterator
    {
    public:
        iterator(): map(NULL) {}
        iterator(const TableAnyMap &m): map(&m),ix(0) { leftmost(); }
        iterator(iterator &it): map(it.map),ix(it.ix) {}
    
        iterator &operator =(const iterator &it) { map = it.map,ix = it.ix; return *this; }

        operator bool() const { return map && /*ix >= 0 &&*/ ix < map->n; }

        // no checking here!
        void *data() const { return map->data[ix].value; }
        size_t key() const { return map->data[ix].key; }

        iterator &operator ++() { forward(); return *this; }  

    protected:
        void leftmost()
        {
            // search smallest branch (go left as far as possible)
            const TableAnyMap *nmap;
            while((nmap = map->left) != NULL) map = nmap;
        }

        void forward();

        const TableAnyMap *map;
        int ix;
    };

private:

    void _init(size_t k,void *t) { data[0](k,t); n = /*count =*/ 1; }

    void *_toleft(int tsize,size_t k,void *t)
    {
        if(left)
            return left->_set(tsize,k,t);
        else {
            (left = _newmap(this))->_init(k,t);
            return NULL;
        }
    }

    void *_toright(int tsize,size_t k,void *t)
    {
        if(right)
            return right->_set(tsize,k,t);
        else {
            (right = _newmap(this))->_init(k,t);
            return NULL;
        }
    }

    void *_toleft(int tsize,Data &v) { return _toleft(tsize,v.key,v.value); }
    void *_toright(int tsize,Data &v) { return _toright(tsize,v.key,v.value); }

    void *_set(int tsize,size_t k,void *t);
    void *_find(int tsize,size_t k) const;
    void *_remove(int tsize,size_t k);

#ifdef FLEXT_DEBUG
    void _check(int tsize);
#endif

//    const int tsize;
    Data *const data;
    TableAnyMap *parent,*left,*right;
    short n;

    //! return index of data item with key <= k
    //! \note index can point past the last item!
    int _tryix(size_t k) const
    {
        int ix = 0;
        {
            int b = n;
            while(ix != b) {
                const int c = (ix+b)/2;
                const size_t dk = data[c].key;
                if(k == dk)
                    return c;
                else if(k < dk)
                    b = c;
                else if(ix < c)
                    ix = c;
                else {
                    ix = b;
                    break;
                }
            }
        }
        return ix;
    }

    void _eraseempty(TableAnyMap *&b)
    {
        if(!b->n) { 
            // remove empty branch
            _delmap(b); b = NULL; 
        }
    }

    void _getsmall(Data &dt);
    void _getbig(Data &dt);
};

template <typename K,typename T,int N = 8>
class TablePtrMap
    : TableAnyMap
{
public:
    TablePtrMap(): TableAnyMap(NULL,slots),count(0) {}
    virtual ~TablePtrMap() { clear(); }

    virtual void clear() { TableAnyMap::clear(); count = 0; }

    inline int size() const { return count; }

    inline T insert(K k,T t) 
    { 
        void *d = TableAnyMap::insert(N,*(size_t *)&k,(void *)t); 
        if(!d) ++count;
        return (T)d;
    }

    inline T find(K k) const { return (T)TableAnyMap::find(N,*(size_t *)&k); }

    inline T remove(K k) 
    { 
        void *d = TableAnyMap::remove(N,*(size_t *)&k); 
        if(d) --count;
        return (T)d;
    }

    class iterator
        : TableAnyMap::iterator
    {
    public:
        iterator() {}
        iterator(const TablePtrMap &m): TableAnyMap::iterator(m) {}
        iterator(iterator &it): TableAnyMap::iterator(it) {}

        inline iterator &operator =(const iterator &it) { TableAnyMap::operator =(it); return *this; }

        inline operator bool() const {return TableAnyMap::iterator::operator bool(); }
        inline T data() const { return (T)TableAnyMap::iterator::data(); }
        inline K key() const { return (K)TableAnyMap::iterator::key(); }

        inline iterator &operator ++() { TableAnyMap::iterator::operator ++(); return *this; }  

    };

protected:
    TablePtrMap(TableAnyMap *p): TableAnyMap(p,slots),count(0) {}

    virtual TableAnyMap *_newmap(TableAnyMap *parent) { return new TablePtrMap(parent); }
    virtual void _delmap(TableAnyMap *map) { delete (TablePtrMap *)map; }

    int count;
    Data slots[N];
};

//! @} // FLEXT_SUPPORT

#endif
