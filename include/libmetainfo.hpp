#ifndef LLSCHEME_LIBMETAINFO_HPP
#define LLSCHEME_LIBMETAINFO_HPP

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace llscm {
    using namespace std;

    // TODO: We also need global variables in the Metadata structure
    struct __attribute__ ((__packed__)) FunctionInfo {
        int32_t argc;
        char name[1];

        void * operator new(std::size_t s, void * p, int32_t namelen);

        // Disable default new
        void * operator new(std::size_t s) = delete;
        void * operator new(std::size_t s, void * p) = delete;

        FunctionInfo(int32_t argc, const char * name);

        static size_t size(size_t namelen);

        size_t size();
    };

    struct Metadata {
        uint8_t * metadata;
        size_t md_size;
        size_t alloc_size;
        static const size_t InitSize;
        static const char magic[];

        Metadata();
        ~Metadata();

        void addRecord(int32_t argc, const string & name);

        vector<uint8_t> getBlob();
    };
}

#endif //LLSCHEME_LIBMETAINFO_HPP
