#ifndef LLSCHEME_ANY_PTR_HPP
#define LLSCHEME_ANY_PTR_HPP

// A simplified version of boost::any class.
// It is capable of holding a generic pointer.
// It serves as a type safe alternative to void *.
// We only support pointers because if non-pointer types were used,
// any_cast would have to throw an exception. LLVM does not like exceptions.
// When a non-pointer type is passed to any_ptr constructor, it is allocated
// on the heap and its pointer is stored in the holder. This is also the only
// case when any_ptr containter owns the pointer and deletes it automatically
// in holder destructor. Otherwise the pointer is presumed to be owned by someone else.
// It is considerably slower than void pointer, therefore we use it in the Debug build only.

#define APC any_ptr_cast

#ifdef DEBUG

#include <typeinfo>

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
        bool has_ownership;

        holder(T * v, bool own = false): val(v) {
            has_ownership = own;
        }

        virtual ~holder() {
            if (has_ownership) {
                delete val;
            }
        }

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

    template<typename T>
    any_ptr(const T & v): dat(new holder<T>(new T(v), true)) {}

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

#else

// This is way faster but type unsafe. Used in Release build.
// This is basically just a void * pointer wrapped in a class
// which allows us to omit explicit casts to any_ptr type
// because there's a template constructor for every type we'd
// like to store. Casting from any_ptr to the original type is
// still necessary, so we have also a simplified any_ptr_cast
// helper which does const_ and static_cast.

class any_ptr {
    const void * val;
public:
    any_ptr(): val(nullptr) {}

    template<typename T>
    any_ptr(T * v): val(static_cast<const void*>(v)) {}

    template<typename T>
    friend T * any_ptr_cast(const any_ptr & ptr);
};

template<typename T>
T * any_ptr_cast(const any_ptr & ptr) {
    return static_cast<T*>(const_cast<void*>(ptr.val));
}

#endif

#endif //LLSCHEME_ANY_PTR_HPP
