
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <type_traits>

namespace baginbix{
    template<typename T>
    class Vector{
        public:
        Vector(): cap(10), num_items(0) {
            data = static_cast<T*>(::operator new(cap * sizeof(T)));
        }
        explicit Vector(const size_t capacity):num_items(0){
            cap = capacity > 0 ? capacity : 10;
            data = static_cast<T*>(::operator new(cap * sizeof(T)));
        }
        Vector(std::initializer_list<T> init) {
            cap = init.size() > 10 ? init.size() : 10;
            data = data = static_cast<T*>(::operator new(cap * sizeof(T)));
            num_items = 0;

            for (const auto& elem : init) {
                new (data + num_items) T(elem);
                num_items++;
            }
        }
        Vector(const Vector<T>& other){
            num_items = other.num_items;
            cap = other.cap;
            data = static_cast<T*>(::operator new(cap * sizeof(T)));
            for (size_t i = 0; i < num_items; ++i) {
                new (data + i) T(other.data[i]);
            }
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
            ::operator delete(data);
        }

        Vector& operator=(const Vector<T>& other) noexcept{
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
            if(num_items == cap){
                resize(cap * 2);
            }
            new(&data[num_items]) T(value);
            ++num_items;
        }
        void pop_back(){
            if(num_items == 0) return;
            --num_items;
            if constexpr (!std::is_trivially_destructible_v<T>) {
                data[num_items].~T();
            }
        }

        void insert(size_t index, const T& value) {
            assert(index <= num_items);
            T copy(value);
            if (index == num_items) {
                emplace_back(std::move(copy));
                return;
            }
            if (num_items == cap) resize(cap * 2);
            ::new (data+ num_items) T(std::move(data[num_items-1]));
            std::move_backward(data + index, data + num_items-1, data + num_items);
            data[index] = value;
            ++num_items;
        }

        void erase(size_t index) {
            if (index >= num_items) return;
            std::move(data + index + 1, data + num_items, data + index);
            --num_items;
            if constexpr (!std::is_trivially_destructible_v<T>) {
                data[num_items].~T();
            }
        }


        size_t size() const{
            return num_items;
        }
        size_t capacity() const{
            return cap;
        }

        void reserve(size_t new_cap) {
            if (new_cap > cap)
                resize(new_cap);
        }

        void clear() {
            destroy_non_trivial_types();
            num_items = 0;
        }

        template<typename... Args>
        void emplace_back(Args&&... args){
            if(num_items == cap){
                resize(cap * 2);
            }
            new(&data[num_items]) T(std::forward<Args>(args)...);
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
            return data[0];
        }

        T& back() noexcept {
            return data[num_items - 1];
        }

        const T& front() const noexcept {
            return data[0];
        }

        const T& back() const noexcept {
            return data[num_items - 1];
        }

        void shrink_to_fit() {
            if (num_items < cap) {
                resize(num_items > 0 ? num_items : 1);
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

        private:
        T* data;
        size_t num_items;
        size_t cap;

        void resize(size_t new_cap){
            if (new_cap == cap ) return;

            T* new_data = static_cast<T*>(::operator new(new_cap * sizeof(T)));
            if constexpr (std::is_trivially_copyable_v<T>) {
                assert(new_data != nullptr);
                std::memcpy(new_data, data, num_items*sizeof(T));
            }
            else {
                for (size_t i = 0; i < num_items; ++i)
                    ::new(&new_data[i]) T(std::move_if_noexcept(data[i]));
            }
            destroy_non_trivial_types();

            ::operator delete(data);
            data = new_data;
            cap = new_cap;
        }
        inline constexpr void destroy_non_trivial_types(){
            if constexpr (!std::is_trivially_destructible_v<T>){
                for (int i = 0; i < num_items; ++i) {
                    data[i].~T();
                }
            }
        }
        void swap(Vector& o) noexcept{
            std::swap(data, o.data);
            std::swap(num_items, o.num_items);
            std::swap(cap, o.cap);
        }
    };

}
