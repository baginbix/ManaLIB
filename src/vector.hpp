
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <print>
#include <type_traits>
namespace baginbix{
    template<typename T>
    class Vector{
        public:
        Vector(){
            cap = 10;
            data = new T[cap];
            num_items = 0;
        }
        Vector(const int capacity){
            cap = capacity > 0 ? capacity : 10;
            data = new T[cap];
            num_items = 0;
        }
        Vector(const Vector<T>& other){
            num_items = other.num_items;
            cap = other.cap;
            data = new T[cap];
            for (int i = 0; i < num_items; ++i) {
                data[i] = other.data[i];
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
            delete[] data;
        }

        Vector& operator=(const Vector<T>& other) noexcept{
            if(this == &other) return *this;
            num_items = other.num_items;
            cap = other.cap;

            if(data){
                delete[] data;
            }
            data = new T[cap];
            for (int i = 0; i < num_items; ++i) {
                data[i] = other.data[i];
            }

            return *this;
        }
        Vector& operator=(Vector<T>&& other){
            if(this == &other) return *this;

            num_items = other.num_items;
            cap = other.cap;
            data = other.data;

            other.data = nullptr;
            other.num_items = 0;
            other.cap = 0;

            return *this;
        }

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
            data[num_items] = value;
            ++num_items;
        }
        void pop_back(){
            --num_items;
            if (cap > 16 && num_items * 4 < cap) {
                resize(cap / 2);
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


        private:
        T* data;
        size_t num_items;
        size_t cap;

        void resize(size_t new_cap){
            T* new_data = new T[new_cap];
            if constexpr (std::is_trivially_copyable_v<T>) {
                std::memcpy(new_data, data, num_items*sizeof(T));
            }
            else{
                for (size_t i = 0; i < num_items; ++i)
                    new_data[i] = std::move(data[i]);
            }

            delete[] data;
            data = new_data;
            cap = new_cap;
        }
    };

}
