#include <new>
#include <iostream>
#include "../include/libmetainfo.hpp"

namespace llscm {
    const size_t Metadata::InitSize = 32; // TODO: increase
    const char Metadata::magic[] = "\xFE\xEDLLSMETA";

    Metadata::Metadata() {
        metadata = (uint8_t*)malloc(InitSize * sizeof(uint8_t));
        if (!metadata) {
            cerr << "Allocation failed." << endl;
            exit(EXIT_FAILURE);
        }
        alloc_size = InitSize;

        size_t magiclen = strlen(magic);
        memcpy(metadata, magic, magiclen);

        md_size = magiclen;
    }

    Metadata::~Metadata() {
        free(metadata);
    }

    void Metadata::addRecord(int32_t argc, const string & name) {
        size_t namelen = name.length();
        uint8_t * arr;

        if (alloc_size - md_size < FunctionInfo::size(namelen)) {
            alloc_size *= 2;
            arr = (uint8_t*)realloc(metadata, alloc_size * sizeof(uint8_t));
            if (!arr) {
                cerr << "Allocation failed." << endl;
                exit(EXIT_FAILURE);
            }
            metadata = arr;
        }
        FunctionInfo * rec = new(metadata + md_size, namelen) FunctionInfo(argc, name.c_str());
        md_size += rec->size();
    }

    vector<uint8_t> Metadata::getBlob() {
        addRecord(0, ""); // Empty record marks the end of array.
        return vector<uint8_t>(metadata, metadata + md_size);
    }

    void * FunctionInfo::operator new(std::size_t s, void * p, int32_t namelen) {
        // Allocate enough space for variable length function name
        std::size_t size = s;
        size += namelen * sizeof(char);

        return ::operator new(size, p);
    }

    size_t FunctionInfo::size() {
        return size(strlen(name));
    }

    size_t FunctionInfo::size(size_t namelen) {
        size_t s = sizeof(FunctionInfo);
        s += namelen * sizeof(char);
        return s;
    }

    FunctionInfo::FunctionInfo(int32_t argc, const char * name) {
        this->argc = argc;
        strcpy(this->name, name);
    }
}