#pragma once

#include <stddef.h>
#include <liballoc/liballoc.h>
#include <panic.h>
#include <kmove.h>

template<typename T>
class list {
    class list_iterator {
        friend class list<T>;
    public:
        list_iterator(const list<T>& list)
            :_list(list)
        {}

        list_iterator(const list_iterator& other)
            :_list(other._list)
            ,_pos(other._pos)
        {}

        list_iterator& operator++() {
            _pos++;
            return *this;
        }

        list_iterator& operator++(int) {
            auto* it = this;
            _pos++;
            return *it;
        }

        list_iterator& operator=(const list_iterator& other) {
            list_iterator(other._list);

            _pos = other._pos;
            return *this;
        }

        ALWAYS_INLINE T& operator*() const {
            return _list._mem[_pos];
        }

        ALWAYS_INLINE T* operator->() const {
            return &_list._mem[_pos];
        }

        ALWAYS_INLINE friend bool operator==(const list_iterator& left, const list_iterator& right) {
            return left._pos == right._pos;
        }

        ALWAYS_INLINE friend bool operator!=(const list_iterator& left, const list_iterator& right) {
            return left._pos != right._pos;
        }

    private:
        size_t _pos {0};
        const list<T>& _list;
    };
public:
    list() 
        :list(8)
    {

    }

    list(size_t capacity)
        :_size(0)
        ,_capacity(capacity)
        ,_mem((T*)malloc(sizeof(T) * capacity))
    {

    }

    ~list() {
        free(_mem);
    }

    T& add(const T& item) {
        if(_size >= _capacity) {
            grow();
        }

        _mem[_size++] = item;
        return _mem[_size - 1];
    }

    T& insert(const T& obj, list_iterator& it) {
        if(it == end()) {
            return add(obj);
        }

        if(_size >= _capacity) {
            grow();
        }

        for(size_t i = _size; i > it._pos; i--) {
            _mem[i] = _mem[i - 1];
        }

        _size++;
        _mem[it._pos] = obj;
        return _mem[it._pos];
    }

    bool set(const T& obj, size_t idx) {
        if(idx >= _size) {
            return false; 
        }

        _mem[idx] = obj;
        return true;
    }

    T remove_at(size_t index) {
        T item = _mem[index];
        for(size_t i = index; i < _size - 1; i++) {
            _mem[i] = _mem[i + 1];
        }

        _size--;
        return item;
    }

    void remove(T item) {
        for(int i = 0; i < _size; i++) {
            if(_mem[i] == item) {
                remove_at(i);
                return;
            }
        }
    }

    void clear() {
        _size = 0;
    }

    T get(size_t index) const {
        if(index >= _size) {
            const char* reasons[] = { "Out of bounds list access" };
            kernel_panic(reasons, 1);
            __builtin_unreachable();
        }

        return _mem[index];
    }

    size_t size() const { return _size; }

    list_iterator begin() const {
        list_iterator it(*this);
        it._pos = 0;
        return it;
    }

    list_iterator end() const {
        list_iterator it(*this);
        it._pos = _size;
        return it;
    }

    ALWAYS_INLINE T operator[](size_t pos) const {
        return get(pos);
    }

private:
    void grow() {
        _capacity *= 2;
        void* new_mem = realloc(_mem, sizeof(T) * _capacity);
        if(!new_mem) {
            const char* reasons[] = { "Out of memory!" };
            kernel_panic(reasons, 1);
            __builtin_unreachable();
        }

        _mem = (T*)new_mem;
    }

    size_t _size;
    size_t _capacity;
    T* _mem;
};