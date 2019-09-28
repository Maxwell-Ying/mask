#ifndef CMD5_H
#define CMD5_H

#include <string>
#include <array>
#include <vector>

using std::string;
using std::array;
using std::vector;

typedef unsigned char byte;

typedef array<uint32_t, 4> state;


class CMD5 {
public:
    CMD5(const string& message);

    CMD5(const uint8_t*, size_t length);

    const uint8_t* get_result();

    string get_result_str();

private:
    void init(const uint8_t* message, size_t length);

    void do_transform();

    uint8_t* m_message;

    vector<state> m_parts;

    vector<uint64_t> m_index;

    size_t m_count;

    const static state init_state;
};

#endif