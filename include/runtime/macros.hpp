#ifndef LLSCHEME_MACROS_HPP
#define LLSCHEME_MACROS_HPP

#include <functional>

namespace llscm {
    namespace runtime {

        // Inspired by http://codereview.stackexchange.com/questions/45549/c-template-range

        template<int... Elems>
        struct RangeElems{
            typedef RangeElems<Elems...> type;
        };

        template<int Left, int Next, int... Elems>
        struct GetRange: GetRange<Left-1, Next+1, Elems..., Next> {};

        /*struct GetRange {
            typedef typename GetRange<Left-1, Next+1, Elems..., Next>::type type;
        };*/

        template<int Next, int... Elems>
        struct GetRange<0, Next, Elems...>: RangeElems<Elems...> {};

        /*struct GetRange<0, Next, Elems...> {
            typedef typename RangeElems<Elems...>::type type;
        };*/

        template<int End>
        struct Range: GetRange<End, 0> {};

        /*struct Range {
            typedef typename GetRange<End, 0>::type type;
        };*/

        // Returns number of function arguments
        template<typename R, typename... Args>
        constexpr int arity(R(Args...)) {
            return sizeof...(Args);
        }
    }
}

#endif //LLSCHEME_MACROS_HPP
