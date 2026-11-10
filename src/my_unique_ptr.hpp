#pragma once
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace baginbix {
    template<typename Type>
    struct DefaultDeleter {
        void operator()(Type* ptr) const { delete ptr; }
    };
    template<typename Type>
    struct DefaultDeleter<Type[]> {
        void operator()(Type* ptr) const { delete[]  ptr; }
    };

    template<typename Type, typename Deleter = DefaultDeleter<Type>>
    class UniquePointer {
        static_assert(std::is_invocable_v<Deleter&, Type*>, "Deleter is not invocable");
    public:
        UniquePointer() noexcept
            : ptr(nullptr), deleter(){
        }
        explicit UniquePointer(Type* obj) noexcept 
            : ptr(obj), deleter()  {}
        UniquePointer(Type* obj, Deleter d) noexcept(std::is_nothrow_move_constructible_v<Deleter>)
            :ptr(obj), deleter(std::move(d))
        {
        }
        UniquePointer(UniquePointer&& obj) noexcept
            : ptr(obj.ptr), deleter(std::move(obj.deleter)) {
            obj.ptr = nullptr;
        }
        UniquePointer(const UniquePointer&) = delete;
        UniquePointer(UniquePointer&) = delete;

        ~UniquePointer() {
            if (ptr) {
                deleter(ptr);
            }

        }

        explicit operator bool() const noexcept {
            return ptr;
        }

        Type& operator* () const noexcept requires(!std::is_array_v<Type>){
            assert(ptr);
            return *ptr;
        }
        Type* operator->() const noexcept requires(!std::is_array_v<Type>){
            return ptr;
        }

        Type& operator[](std::size_t index) noexcept requires(std::is_array_v<Type>){
            return ptr[index];
        }
        const Type& operator[](std::size_t index) const noexcept requires(std::is_array_v<Type>) {
            return ptr[index]; 
        }

        friend bool operator==(const UniquePointer& a, const UniquePointer& b) noexcept {
            return a.get() == b.get();
        }
        friend bool operator==(const UniquePointer& a, std::nullptr_t) noexcept {
            return a.get() == nullptr;
        }
        friend bool operator==(std::nullptr_t, const UniquePointer& a) noexcept {
            return  a.get() == nullptr;
        }

        void swap(UniquePointer& other) noexcept {
            using std::swap;
            swap(ptr, other.ptr);
            swap(deleter, other.deleter);
        }

        Type* release() noexcept {
            auto oldPtr = ptr;
            ptr = nullptr;
            return oldPtr;
        }

        void reset(Type* newPtr = nullptr) noexcept {
            if (newPtr == ptr) return;
            Type* old = ptr;
            ptr = newPtr;
            if (old) {
                deleter(old);
            }
        }
        [[nodiscard]]
        Type* get() const noexcept {
            return ptr;
        }

        [[nodiscard]]
        Deleter& get_deleter() {
            return deleter;
        }

        [[nodiscard]]
        const Deleter& get_deleter() {
            return deleter;
        }
    private:
        Type* ptr;

#if defined(_MSC_VER) && !defined(__clang__)
        [[msvc::no_unique_address]] Deleter deleter;
#else

        [[no_unique_address]] Deleter deleter;
#endif
    };
}