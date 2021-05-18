#pragma once

#include <kmove.h>
#include <stddef.h>

namespace kstd {
    template<typename T>
    class ref_counted {
    public:
        ref_counted()
            :ref_counted(nullptr)
        {}

        ref_counted(T* obj) 
            :_obj(obj)
            ,_refcount(new unsigned(1))
        {}

        ref_counted(nullptr_t)
            :_obj(nullptr)
            ,_refcount(nullptr)
        {}

        ref_counted(const ref_counted<T>& other)
            :_obj(other._obj)
            ,_refcount(other._refcount) 
            {
                retain();
            }

        ref_counted(ref_counted &&other) 
            :_obj(other._obj)
            ,_refcount(other._refcount)
        {
            other._obj = nullptr;
            other._refcount = nullptr;
        }

        ~ref_counted() {
            release();
        }

        ALWAYS_INLINE ref_counted<T>& operator=(const ref_counted<T>& ptr) {
            release();

            _obj = ptr._obj;
            _refcount = ptr._refcount;
            retain();

            return *this;
        }

        ALWAYS_INLINE ref_counted<T>& operator=(ref_counted<T> &&ptr) {
            release();

            _obj = ptr._obj;
            _refcount = ptr._refcount;

            ptr._obj = nullptr;
            ptr._refcount = nullptr;

            return *this;
        }

        ALWAYS_INLINE T* get() const {
            return _obj;
        }

        ALWAYS_INLINE ref_counted& operator=(nullptr_t) {
            release();
            return *this;
        }

        ALWAYS_INLINE bool operator==(const T* p) {
            return _obj == p;
        }

        ALWAYS_INLINE bool operator!=(const T* p) {
            return _obj != p;
        }

        ALWAYS_INLINE T& operator*() const {
            return *(_obj);
        }

        ALWAYS_INLINE T* operator->() const {
            return _obj;
        }

        ALWAYS_INLINE operator bool() const {
            return _obj;
        }

        ALWAYS_INLINE void retain() {
            if(_refcount) {
                __sync_fetch_and_add(_refcount, 1);
            }
        }

        ALWAYS_INLINE void release() {
            if(_refcount) {
                if(__sync_fetch_and_sub(_refcount, 1) == 1) {
                    delete _obj;
                    delete _refcount;
                    _obj = nullptr;
                    _refcount = nullptr;
                }
            }
        }
    private:
        T* _obj;
        unsigned* _refcount;
    };
}