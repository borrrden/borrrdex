#include "cstr.h"

const char* to_string(uint64_t value) {
    if(!value) {
        return "0";
    }

    static char itoaOutput[64];
    uint8_t size = 0;
    uint64_t sizeTest = value;
    while(sizeTest / 10 > 0) {
        sizeTest /= 10;
        size++; 
    }

    uint8_t index = 0;
    while(value > 0) {
        uint8_t remainder = value % 10;
        value /= 10;
        itoaOutput[size - index] = remainder + '0';
        index++;
    }

    itoaOutput[size + 1] = 0;
    return itoaOutput;
}

const char* to_string(int64_t value) {
    if(!value) {
        return "0";
    }

    static char itoaOutput[64];
    uint8_t isNegative = value < 0 ? 1 : 0;
    if(isNegative) {
        value *= -1;
        itoaOutput[0] = '-';
    }
    
    uint8_t size = 0;
    int64_t sizeTest = value;
    while(sizeTest / 10 > 0) {
        sizeTest /= 10;
        size++; 
    }

    uint8_t index = 0;
    while(value > 0) {
        uint8_t remainder = value % 10;
        value /= 10;
        itoaOutput[size - index + isNegative] = remainder + '0';
        index++;
    }

    itoaOutput[size + 1 + isNegative] = 0;
    return itoaOutput;
}

const char* to_hstring(uint64_t value, uint8_t size) {
    static char itoaOutput[64];
    uint64_t* valPtr = &value;
    uint8_t* ptr;
    uint8_t tmp;
    for(uint8_t i = 0; i < size; i++) {
        ptr = ((uint8_t *)valPtr + i);
        tmp = ((*ptr & 0xf0) >> 4);
        itoaOutput[size - (i * 2 + 1)] = tmp + (tmp > 9 ? 55 : '0');
        tmp = ((*ptr & 0x0f));
        itoaOutput[size - (i * 2)] = tmp + (tmp > 9 ? 55 : '0');
    }

    itoaOutput[size + 1] = 0;
    return itoaOutput;
}

const char* to_hstring(uint64_t value) {
    return to_hstring(value, 8 * 2 - 1);
}

const char* to_hstring(uint32_t value) {
    return to_hstring(value, 4 * 2 - 1);
}

const char* to_hstring(uint16_t value) {
    return to_hstring(value, 2 * 2 - 1);
}

const char* to_hstring(uint8_t value) {
    return to_hstring(value, 1 * 2 - 1);
}

const char* to_string(double value, uint8_t decimalPlaces) {
    static char dtoaOutput[64];
    if(decimalPlaces > 20) {
        decimalPlaces = 20;
    }

    const char* intPtr = to_string((int64_t)value);
    char* doublePtr = dtoaOutput;

    if(value < 0) {
        value *= -1;
    }

    while(*intPtr != 0) {
        *doublePtr++ = *intPtr++;
    }

    *doublePtr++ = '.';
    double newValue = value - (int64_t)value;
    for(uint8_t i = 0; i < decimalPlaces; i++) {
        newValue *= 10;
        *doublePtr++ = (int)newValue + '0';
        newValue -= (int)newValue;
    }

    *doublePtr = 0;
    return dtoaOutput;
}

const char* to_string(double value) {
    return to_string(value, 2);
}