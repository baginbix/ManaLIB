#pragma once
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <cassert>
#include <utility>
#include <memory>
#include <new>
#include <initializer_list>
#include <optional>


namespace baginbix{

    // Optional<T>: maybe-a-reference. Non-owning view of a T;
    // invalidated if the container reallocates. Not a copy — see std::optional for that.
    template<typename T>
    class Optional{
        public:
        explicit Optional(T& val):value(&val){}
        explicit Optional():value(nullptr){}
        [[nodiscard]]
        explicit operator bool() const noexcept {
            return value != nullptr;
        }
        [[nodiscard]]
        bool has_value() const noexcept {
            return value != nullptr;
        }
        [[nodiscard]]
        T& unwrap() noexcept {
            assert(value != nullptr);
            return *value;
        }
        [[nodiscard]]
        T& unwrap() const noexcept {
            assert(value != nullptr);
            return *value;
        }

        private:
            T* value;
    };
    template<typename T>
    class Vector{
        public:
        Vector(): cap(10), num_items(0) {
            data = allocate(cap);
        }
        explicit Vector(const size_t capacity):num_items(0){
            cap = capacity > 0 ? capacity : 10;
            data = allocate(cap);
        }
        Vector(std::initializer_list<T> init) {
            cap = init.size() > 10 ? init.size() : 10;
            data = allocate(cap);
            num_items = 0;
            NewBufferGuard guard;
            guard.arm(data);
            for (const auto& elem : init) {
                new (data + num_items) T(elem);
                num_items++;
                guard.constructed = num_items;
            }
            guard.disarm();
        }
        Vector(const Vector<T>& other){
            data = nullptr;
            if(other.num_items) 
                data = allocate(other.num_items);
            
            NewBufferGuard guard;
            guard.arm(data);

            if constexpr (std::is_trivially_copyable_v<T>){
                if(other.num_items)
                    std::memcpy(data, other.data, other.num_items * sizeof(T));
                guard.constructed = other.num_items;
            }
            else{
                for(; guard.constructed < other.num_items; ++guard.constructed){
                    ::new (static_cast<void*>(data + guard.constructed)) T(other.data[guard.constructed]);
                }
            }
            num_items = other.num_items;
            cap = other.num_items;
            guard.disarm();
        }
        Vector(Vector<T>&& other) noexcept{
            num_items = other.num_items;
            cap = other.cap;
            data = other.data;

            other.data = nullptr;
            other.num_items = 0;
            other.cap = 0;
        }

        ~Vector(){
            destroy_non_trivial_types();
            deallocate(data);
        }

        Vector& operator=(const Vector<T>& other){
            Vector tmp(other);
            swap(tmp);

            return *this;
        }
        Vector& operator=(Vector<T>&& other)  noexcept{
            Vector tmp(std::move(other));
            swap(tmp);
            return *this;
        }

        bool operator==(const Vector& other) const{
            if (num_items != other.num_items) return false;
            for (size_t i = 0; i < num_items; ++i)
                if (!(data[i] == other.data[i])) return false;
            return true;
        }

        bool operator!=(const Vector& other) const { return !(*this == other); }

        T& operator[](std::size_t index){
            return data[index];
        }

        const T& operator[](std::size_t index) const{
            return data[index];
        }
        void push_back(const T& value){
            emplace_back(value);
        }
        void push_back(T&& value){
            emplace_back(std::move(value));
        }
        void pop_back(){
            if(num_items == 0) return;
            --num_items;
            if constexpr (!std::is_trivially_destructible_v<T>) {
                data[num_items].~T();
            }
        }

        void insert(size_t index, const T& value) {
            insert_impl(index, value);
        }

        void insert(size_t index, T&& value) {
            insert_impl(index, std::move(value));
        }

        void erase(size_t index) {
            if (index >= num_items) return;
            std::move(data + index + 1, data + num_items, data + index);
            --num_items;
            if constexpr (!std::is_trivially_destructible_v<T>) {
                data[num_items].~T();
            }
        }


        [[nodiscard]]
        size_t size() const{
            return num_items;
        }

        [[nodiscard]]
        size_t capacity() const{
            return cap;
        }

        [[nodiscard]]
        bool empty() const{
            return num_items == 0;
        }
        [[nodiscard]]
        Optional<T> at(size_t index){
            if (index >= num_items) {
                return Optional<T>();
            }
            return Optional<T>(data[index]);
        }

        [[nodiscard]]
        Optional<const T> at(size_t index) const{
            if (index >= num_items) {
                return Optional<const T>();
            }
            return Optional<const T>(data[index]);
        }
        
        [[nodiscard]]
        T* data_ptr() {
            return data;
        }

        [[nodiscard]]
        const T* data_ptr() const {
            return data;
        }

        void reserve(size_t new_cap) {
            if (new_cap > cap)
                reallocate(new_cap);
        }

        void clear() {
            destroy_non_trivial_types();
            num_items = 0;
        }

        template<typename... Args>
        void emplace_back(Args&&... args){
            if(num_items == cap){
                T tmp(std::forward<Args>(args)...);
                grow();
                ::new (static_cast<void*>(data + num_items)) T(std::move(tmp));
            } else {
                ::new (static_cast<void*>(data + num_items)) T(std::forward<Args>(args)...);
            }
            ++num_items;
        }

        T* begin() noexcept {
            return data;
        }

        T* end() noexcept {
            return data + num_items;
        }

        const T* begin() const noexcept {
            return data;
        }

        const T* end() const noexcept {
            return data + num_items;
        }

        const T* cbegin() const noexcept {
            return data;
        }

        const T* cend() const noexcept {
            return data + num_items;
        }

        T& front() noexcept {
            assert(!empty());
            return data[0];
        }

        T& back() noexcept {
            assert(!empty());
            return data[num_items - 1];
        }

        const T& front() const noexcept {
            assert(!empty());
            return data[0];
        }

        const T& back() const noexcept {
            assert(!empty());
            return data[num_items - 1];
        }

        void shrink_to_fit() {
            if (num_items < cap) {
                reallocate(num_items > 0 ? num_items : 1);
            }
        }

        bool contains(const T& value) const {
            for (size_t i = 0; i < num_items; ++i) {
                if (data[i] == value) return true;
            }
            return false;
        }

        size_t find(const T& value) const {
            for (size_t i = 0; i < num_items; ++i) {
                if (data[i] == value) {
                    return i;
                }
            }
            return static_cast<size_t>(-1);
        }

        void swap(Vector& o) noexcept{
            std::swap(data, o.data);
            std::swap(num_items, o.num_items);
            std::swap(cap, o.cap);
        }

        private:
        T* data;
        size_t num_items;
        size_t cap;

        struct NewBufferGuard{
            T* p = nullptr;
            size_t constructed = 0;
            void arm(T* ptr) {p = ptr;}
            void disarm() noexcept { p = nullptr;}

            ~NewBufferGuard(){
                if(p){
                    std::destroy_n(p,constructed);
                    deallocate(p);
                }
            }
        };

        void grow(){
            size_t new_capacity = cap > 0 ? cap * 2 : 10;
            reallocate(new_capacity);
        }
        void reallocate(size_t new_capacity){
            if(new_capacity == 0) new_capacity = 10;
            T* new_data = allocate(new_capacity);

            if constexpr(std::is_trivially_copyable_v<T>){
                if(num_items > 0){
                    std::memcpy(new_data, data, num_items*sizeof(T));
                }
            }else{
                NewBufferGuard guard;
                guard.arm(new_data);
                
                for (size_t i = 0; i < num_items; i++)
                {
                    ::new (static_cast<void*>(new_data + i))
                        T(std::move_if_noexcept(data[i]));
                    guard.constructed = i+1;
                }
                std::destroy_n(data, num_items);
                guard.disarm();
                
            }
            deallocate(data);
            data = new_data;
            cap = new_capacity;
        }
        inline constexpr void destroy_non_trivial_types(){
            if constexpr (!std::is_trivially_destructible_v<T>){
                for (size_t i = 0; i < num_items; ++i) {
                    data[i].~T();
                }
            }
        }

        static T* allocate(size_t n) {
            if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
                return static_cast<T*>(::operator new(n * sizeof(T), std::align_val_t{alignof(T)}));
            }
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }

        static void deallocate(T* ptr) {
            if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
                ::operator delete(ptr, std::align_val_t{alignof(T)});
            } else {
                ::operator delete(ptr);
            }
        }

        void insert_impl(size_t index, T value) {
            assert(index <= num_items);
            if (index == num_items) {
                emplace_back(std::move(value));
                return;
            }
            if (num_items == cap) grow();
            ::new (data + num_items) T(std::move(data[num_items - 1]));
            std::move_backward(data + index, data + num_items - 1, data + num_items);
            data[index] = std::move(value);
            ++num_items;
        }


    };

    
}