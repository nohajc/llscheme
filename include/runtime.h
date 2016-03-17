#ifndef LLSCHEME_RUNTIME_H
#define LLSCHEME_RUNTIME_H

struct RuntimeSymbols {
    static const char * malloc;
    static const char * cons;
    static const char * car;
    static const char * cdr;
    static const char * isNull;
    static const char * plus;
    static const char * minus;
    static const char * times;
    static const char * div;
    static const char * gt;
    static const char * print;
};

#endif //LLSCHEME_RUNTIME_H
