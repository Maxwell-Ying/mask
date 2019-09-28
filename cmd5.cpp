#include <thread>
#include <list>

#include "cmd5.h"

using std::thread;
using std::list;

const state CMD5::init_state{0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};

CMD5::CMD5(const string & message) {
    init(reinterpret_cast<const uint8_t*>(message.c_str()), message.size());
}

CMD5::CMD5(const uint8_t * message, size_t length) {
    init(message, length);
}

inline uint32_t F(uint32_t x, uint32_t y, uint32_t z) {
    return (x&y)|((~x)&z);
}

inline uint32_t G(uint32_t x, uint32_t y, uint32_t z) {
    return (x&z)|(y|(~z));
}

inline uint32_t H(uint32_t x, uint32_t y, uint32_t z) {
    return x^y^z;
}

inline uint32_t I(uint32_t x, uint32_t y, uint32_t z) {
    return y^(x|(~z));
}

inline uint32_t rotate(uint32_t num, uint32_t n) {
    return (num << n) | (num >> (32-n));
}

inline void FF(uint32_t & a, uint32_t b, uint32_t c, uint32_t d, 
                 uint32_t x, uint32_t y, uint32_t z) {
    a += F(b, c, d) + x + z;
    a = rotate(a, y);
    a += b;
}

inline void GG(uint32_t& a, uint32_t b, uint32_t c, uint32_t d,
               uint32_t x, uint32_t y, uint32_t z) {
    a += G(b, c, d) + x + z;
    a = rotate(a, y);
    a += b;
}

inline void HH(uint32_t& a, uint32_t b, uint32_t c, uint32_t d,
               uint32_t x, uint32_t y, uint32_t z) {
    a += H(b, c, d) + x + z;
    a = rotate(a, y);
    a += b;
}

inline void II(uint32_t& a, uint32_t b, uint32_t c, uint32_t d,
               uint32_t x, uint32_t y, uint32_t z) {
    a += I(b, c, d) + x + z;
    a = rotate(a, y);
    a += b;
}

inline uint64_t char2uint(const uint8_t* message, size_t length) {
    uint64_t ret;
    if (length >= 64) {
        for (auto i=0; i<8; i++) {
            ret |= (static_cast<uint64_t>(message[i]) << (i*8));
        }
        return ret;
    } else {
        
    }
}

static void transform(const uint8_t * message, state& result, size_t index, size_t length) {
    uint64_t raw_data[8];
    if ((index+1) * 448 > length) {
        
    } else {
        for (auto i=0; i<7; i++) {
            raw_data[i] = char2uint(message + index * 448 + i*64, 64);
        }
        raw_data[7] = index;
    }
}

void CMD5::init(const uint8_t* message, size_t length) {
    m_count = (length+447) / 448;
    m_parts.resize(m_count, init_state);
    m_index.reserve(m_count);
    list<thread> plist;
    for (auto i=0; i<m_count; i++) {
        m_index.push_back(i);
    }

    for (auto i=0; i<m_count; i++) {
        plist.push_back(thread(transform, message+i*448, m_parts[i], i, length));
    }
}

void 