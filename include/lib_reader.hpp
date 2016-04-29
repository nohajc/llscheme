#ifndef LLSCHEME_LIB_READER_HPP
#define LLSCHEME_LIB_READER_HPP

#include <string>
#include <memory>

namespace llscm {
    class LibReader {
        struct Impl;
        // We have to use pimpl in order to avoid including elfio/elfio.hpp here.
        // There is a collision with LLVM in some global enums and #defines.
        std::unique_ptr<Impl> impl;
    public:
        LibReader();
        ~LibReader();
        bool load(const std::string & libname);
        void * getAddressOfSymbol(const std::string & symname);
    };
}

#endif //LLSCHEME_LIB_READER_HPP
