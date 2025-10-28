
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>
template<typename Type>
struct DefaultDeleter{
    void operator()(Type* ptr) const {delete ptr;}
};

template<typename Type, typename Deleter = DefaultDeleter<Type>>
class MyUniquePtr{
static_assert(std::is_invocable<Deleter, Type*>::value, "Deleter is not invocable");
public:
    MyUniquePtr(){
        ptr = nullptr;
    }
    MyUniquePtr(Type* obj): ptr(obj), deleter(){}
    MyUniquePtr(Type* obj, Deleter deleter)
    :ptr(obj), deleter(deleter)
    {}

    MyUniquePtr(MyUniquePtr&& obj)
    : ptr(obj.ptr), deleter(std::move(obj.deleter)){
        obj.ptr = nullptr;
    }
    MyUniquePtr& operator= (MyUniquePtr&& other) noexcept{
        if(&other != this) {
            swap(other);
        }
        return *this;
    }

    MyUniquePtr(MyUniquePtr& obj) = delete;

    MyUniquePtr& operator= (const MyUniquePtr& other) = delete;



    ~MyUniquePtr(){
        if(ptr){
            deleter(ptr);
        }

    }
    Type& operator* () const{
        return *ptr;
    }
    Type* operator->() const{
        return ptr;
    }

    operator bool() const{return ptr != nullptr;}

    Type* release(){
        Type* p = ptr;
        ptr = nullptr;
        return p;
    }
    void reset(Type* newptr){
        if(newptr == ptr){return;}
        deleter(ptr);
        ptr = newptr;
    }

    void swap(MyUniquePtr& other){
        using std::swap;
        swap(ptr, other.ptr);
        swap(deleter, other.deleter);
    }
    [[nodiscard]]
    Type* get() const{
        return ptr;
    }
private:
    Type* ptr;
    Deleter deleter;
};
template<typename Type>
struct DefaultDeleter<Type[]>{
    void operator()(Type* ptr) const {delete[]  ptr;}
};
template<typename Type, typename Deleter>
class MyUniquePtr<Type[], Deleter>{
    static_assert(std::is_invocable<Deleter, Type*>::value, "Deleter must be callable");
public:
    MyUniquePtr() noexcept : ptr(nullptr), deleter(){
    }
    MyUniquePtr(Type* obj) noexcept:ptr(obj), deleter(){}
    ~MyUniquePtr(){
        if(ptr){
            deleter(ptr);
        }
    }
    MyUniquePtr(Type* obj, Deleter del) noexcept
        :ptr(obj), deleter(del){}


    MyUniquePtr(MyUniquePtr&& other) noexcept
        : ptr(other.ptr), deleter(std::move(other.deleter)) {
        other.ptr = nullptr;
    }

    MyUniquePtr(const MyUniquePtr& obj) = delete;

    operator bool() const{
        return ptr;
    }
    MyUniquePtr& operator= (const MyUniquePtr& other) = delete;

    MyUniquePtr& operator=(MyUniquePtr&& other) noexcept{
        if(this != &other){
            swap(other);
        }
        return *this;
    }
    Type& operator[](std::size_t index){
        return ptr[index];
    }
    const Type& operator[](std::size_t index) const{return ptr[index];}

    void swap(MyUniquePtr& other) noexcept{
        using std::swap;
        swap(ptr, other.ptr);
        swap(deleter, other.deleter);
    }

    Type* release() noexcept{
        auto oldPtr = ptr;
        ptr = nullptr;
        return oldPtr;
    }

    void reset(Type* newPtr) noexcept{
        if(newPtr == ptr) return;
        deleter(ptr);
        ptr = newPtr;
    }
    [[nodiscard]]
    Type* get() const{
        return ptr;
    }
private:
    Type* ptr;
    Deleter deleter;
};
