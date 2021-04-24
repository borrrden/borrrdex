#include "SmartPointer.h"

template<typename T>
SmartPointer<T>::~SmartPointer() {
    if(_val) {
        _destructor(_val);
        _val = nullptr;
    }
}