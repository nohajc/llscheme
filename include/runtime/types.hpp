#ifndef LLSCHEME_TYPES_HPP
#define LLSCHEME_TYPES_HPP

// LLscheme runtime type tags and their string representation

#define TYPES_DEF(T) T(FALSE), T(TRUE), T(NIL), T(INT), T(FLOAT), T(STR), T(SYM), T(CONS), T(FUNC), T(VEC), T(NSPACE)

#define T_STR(name) "S_" #name
#define T_ENUM(name) S_##name

enum Tag {
    TYPES_DEF(T_ENUM)
};

static const char * TagName[] = {
    TYPES_DEF(T_STR)
};

#endif //LLSCHEME_TYPES_HPP