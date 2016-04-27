#ifndef LLSCHEME_LIB_READER_HPP
#define LLSCHEME_LIB_READER_HPP

#include <string>
#include "elfio/elfio.hpp"

namespace llscm {
    class LibReader {
        ELFIO::elfio reader;
        ELFIO::section * dynsym;
    public:
        bool load(const std::string & libname);
        void * getAddressOfSymbol(const std::string & symname);
    };
}

#endif //LLSCHEME_LIB_READER_HPP
