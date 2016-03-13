#ifndef LLSCHEME_ANY_PTR_HPP
#define LLSCHEME_ANY_PTR_HPP

// A simplified version of boost::any class.
// It is capable of holding generic pointer.
// It serves as a type safe alternative to void *.
// We only support pointers because if non-pointer types were used,
// any_cast would have to throw an exception. LLVM does not like exceptions.

#include <typeinfo>
#include <memory>

using namespace std;

class any_ptr {
public:
    class placeholder {
    public:
        virtual ~placeholder() {}

        virtual const type_info & type() const = 0;
        virtual placeholder * clone() const = 0;
    };

    template<typename T>
    class holder: public placeholder {
    public:
        T * val;

        holder(T * v): val(v) {}

        virtual const type_info & type() const {
            return typeid(T);
        }
        virtual placeholder * clone() const {
            return new holder(val);
        }
    };

    placeholder * dat;

    any_ptr(): dat(nullptr) {}

    template<typename T>
    any_ptr(T * v): dat(new holder<T>(v)) {}

    any_ptr(const any_ptr & other):
            dat(other.dat ? other.dat->clone() : nullptr) {}

    ~any_ptr() { delete dat; }

    template<typename T>
    any_ptr & operator=(T * other) {
        delete dat;
        dat = new holder<T>(other);
        return *this;
    }

    const type_info & type() const {
        return dat ? dat->type() : typeid(void);
    }

    template<typename T>
    friend T * any_ptr_cast(const any_ptr & ptr);
};

template<typename T>
T * any_ptr_cast(const any_ptr & ptr) {
    if (ptr.type() == typeid(T)) {
        return static_cast<any_ptr::holder<T>*>(ptr.dat)->val;
    }
    return nullptr;
}

#endif //LLSCHEME_ANY_PTR_HPP
