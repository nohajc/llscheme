#ifndef LLSCHEME_READLINESTREAM_HPP
#define LLSCHEME_READLINESTREAM_HPP

#include <streambuf>
#include <iostream>
#include <vector>

using namespace std;

class readlinebuf: public streambuf {
    virtual int_type underflow() override;
    readlinebuf(const readlinebuf &) = delete;
    readlinebuf & operator=(const readlinebuf &) = delete;

    vector<char> buffer;
    const size_t put_back_max;
public:
    string prompt;

    explicit readlinebuf(size_t pb = 8);
};

class readlinestream: public istream {
    readlinebuf rlbuf;
public:
    readlinestream(): ios(nullptr), istream(&rlbuf) {}

    void setPrompt(const string & str) {
        rlbuf.prompt = str;
    }
};

#endif //LLSCHEME_READLINESTREAM_HPP
