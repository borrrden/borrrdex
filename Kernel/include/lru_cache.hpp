#pragma once

#include <frg/list.hpp>
#include <frg/hash_map.hpp>
#include <lock.h>

#define LRU_CACHE_CPP_DELETE [](auto& val) { delete val; }
#define LRU_CACHE_C_FREE [](auto& val) { free(val); }
#define LRU_CACHE_NO_FREE nullptr

namespace kstd {
    template<typename Key, typename Value, typename Hasher, typename Allocator>
    class lru_cache {
    public:
        typedef void(*destructor_t)(Value& val);

        lru_cache(size_t capacity, destructor_t destructor = LRU_CACHE_NO_FREE) 
            :_capacity(capacity)
            ,_destructor(destructor)
        {

        }

        bool get(const Key& key, Value& val) {
            _lock.acquire_read();
            auto index = _hash_map.find(key);
            if(index == _hash_map.end()) {
                _lock.release_read();
                return false;
            }

            val = index->template get<1>();
            _lock.release_read();
            return true;
        }

        void set(const Key& key, const Value& val) {
            _lock.acquire_write();
            auto index = _hash_map.find(key);
            if(index != _hash_map.end()) {
                index->template get<1>() = val; // Returned by reference
                _lock.release_write();
                return;
            }

            while(_hash_map.size() >= _capacity) {
                auto back = _list.pop_back();
                _hash_map.remove(back->key);
                if(_destructor) {
                    _destructor(back->value);
                }

                delete back;
            }

            auto new_item = new lru_cache_node {
                key,
                val
            };

            _list.push_front(new_item);
            _hash_map.insert(key, val);
            _lock.release_write();
        }
    private:
        struct lru_cache_node {
            Key key;
            Value value;
            frg::default_list_hook<lru_cache_node> hook;
        };

        using map_t = frg::hash_map<Key, Value, Hasher, Allocator>;
        frg::intrusive_list<lru_cache_node, frg::locate_member<lru_cache_node, frg::default_list_hook<lru_cache_node>, &lru_cache_node::hook>> _list;
        map_t _hash_map{ Hasher() };
        kstd::read_write_lock _lock;
        size_t _capacity;
        destructor_t _destructor;
    };
}