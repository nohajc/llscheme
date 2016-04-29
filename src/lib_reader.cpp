#include <cinttypes>
#include <llvm/ADT/STLExtras.h>
#include "../include/lib_reader.hpp"
#include "elfio/elfio.hpp"

namespace llscm {
    using namespace std;
    using namespace llvm;
    using namespace ELFIO;

    struct LibReader::Impl {
        ELFIO::elfio reader;
        ELFIO::section * dynsym;
    };

    LibReader::LibReader() {
        impl = make_unique<Impl>();
    }

    LibReader::~LibReader() {}

    bool LibReader::load(const string & libname) {
        if (!impl->reader.load(libname)) {
            return false;
        }

        Elf_Half sec_num = impl->reader.sections.size();

        for (uint32_t i = 0; i < sec_num; ++i) {
            section * sec = impl->reader.sections[i];

            if (sec->get_type() == SHT_DYNSYM) {
                impl->dynsym = sec;
                break;
            }
        }

        return true;
    }

    void * LibReader::getAddressOfSymbol(const string & symname) {
        const symbol_section_accessor symbols(impl->reader, impl->dynsym);

        for (uint32_t j = 0; j < symbols.get_symbols_num(); ++j) {
            string name;
            Elf64_Addr value;
            Elf_Xword size;
            uint8_t bind;
            uint8_t type;
            Elf_Half section_index;
            uint8_t other;

            symbols.get_symbol(j, name, value, size, bind, type, section_index, other);

            if (name == symname) {
                section * data_sec = impl->reader.sections[section_index];
                int64_t offset = value - data_sec->get_address();
                const char * data = data_sec->get_data() + offset;
                /*fprintf(stderr,
                        "Symbol: name = %s, address = %" PRIx64 ", section_offset = %" PRIx64 ", data = %" PRIx8 "\n",
                        name.c_str(), value, offset, *(uint8_t*)data
                );*/

                return (void*)data;
            }
        }

        return nullptr;
    }
}
