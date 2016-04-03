#ifndef LLSCHEME_MACROS_HPP
#define LLSCHEME_MACROS_HPP

namespace llscm {
    namespace runtime {

        // Inspired by http://codereview.stackexchange.com/questions/45549/c-template-range

        template<int... Elems>
        struct RangeElems{
            typedef RangeElems<Elems...> type;
        };

        template<int Left, int Next, int... Elems>
        struct GetRange: GetRange<Left-1, Next+1, Elems..., Next> {};

        template<int Next, int... Elems>
        struct GetRange<0, Next, Elems...>: RangeElems<Elems...> {};

        template<int End>
        struct Range: GetRange<End, 0> {};

        // Returns number of function arguments
        // http://stackoverflow.com/a/8645270
        template<typename R, typename... Args>
        constexpr int arity(R(Args...)) {
            return sizeof...(Args);
        }
    }
}

#endif //LLSCHEME_MACROS_HPP
