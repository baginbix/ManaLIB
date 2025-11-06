#include <atomic>

#include "my_unique_ptr.hpp"
struct ControlBlockBase{
    std::atomic<long> weak_count{0};
    std::atomic<long> strong_count{0};

    virtual void dispose() = 0;
    virtual void destroy() = 0;

    virtual void* get_ptr() const = 0;

    virtual ~ControlBlockBase() = default;
};
template<typename Type, typename Deleter>
struct ControlBlock: ControlBlockBase{
    Type* ptr;
    Deleter deleter;

    ControlBlock(Type* ptr, Deleter d)
        : ptr(ptr), deleter(d){
        strong_count.store(1, std::memory_order_relaxed);
        weak_count.store(0, std::memory_order_relaxed);
    }

    ~ControlBlock(){
    }

    void dispose() override{
        if(ptr){
            deleter(ptr);
        }
    }
    void destroy() override{
        delete this;
    }
    void* get_ptr() const{
        return ptr;
    }
};

template<typename Type, typename Deleter = DefaultDeleter<Type>>
class MySharedPtr{
    template<typename T>
    friend class MyWeakPtr;
public:

    MySharedPtr()
        :ctrl_block(nullptr), ptr(nullptr){}
    MySharedPtr(Type* ptr): ptr(ptr), ctrl_block(new ControlBlock(ptr, Deleter())){

    }

    MySharedPtr(const MySharedPtr& other){
        ptr = other.ptr;
        ctrl_block = other.ctrl_block;
        if(ctrl_block){
            ctrl_block->strong_count.fetch_add(1,std::memory_order_relaxed);
        }
    }

    MySharedPtr( MySharedPtr&& other) noexcept{
        ptr = other.ptr;
        ctrl_block = other.ctrl_block;
        other.ctrl_block = nullptr;
        other.ptr = nullptr;
    }

    MySharedPtr& operator=(const MySharedPtr& other){
        if(this != &other){
            release();
            ptr = other.ptr;
            ctrl_block = other.ctrl_block;
            if(ctrl_block){
                ctrl_block->strong_count.fetch_add(1,std::memory_order_relaxed);
            }
        }
        return *this;
    }

    MySharedPtr& operator=(MySharedPtr&& other) noexcept{
        if(this != &other){
            release();
            ptr = other.ptr;
            ctrl_block = other.ctrl_block;
            other.ctrl_block = nullptr;
            other.ptr = nullptr;
    }
        return *this;
    }

    operator bool() const{return ptr != nullptr;}

    Type* operator->() const { return ptr; }
    Type& operator*() const { return *ptr; }

    ~MySharedPtr(){
        release();
    }
    Type* get(){
        return ptr;
    }

    long use_count() const{
        if(!ctrl_block) return 0;
        return ctrl_block->strong_count.load(std::memory_order_relaxed);
    }

    void reset(Type* new_ptr = nullptr){
        release();
        ptr = new_ptr;
        ctrl_block = nullptr;
        if(ptr) ctrl_block = new ControlBlock<Type,Deleter>(ptr, Deleter());
    }
    void release(){
        if(ctrl_block && ctrl_block->strong_count.fetch_sub(1,std::memory_order_acq_rel) == 1){
            ctrl_block->dispose();
            if(ctrl_block->weak_count.fetch_sub(1, std::memory_order_acq_rel) == 1){
                ctrl_block->destroy();
            }
        }

        ptr = nullptr;
        ctrl_block = nullptr;
    }

private:
    MySharedPtr(Type* type, ControlBlockBase* ctrl_block)
        : ptr(type), ctrl_block(ctrl_block){}


    Type* ptr;
    ControlBlockBase* ctrl_block;
};

template<typename Type>
class MyWeakPtr{

public:
    MyWeakPtr(): ctrl_block(nullptr){}
    MyWeakPtr(MySharedPtr<Type>& ptr){
        ctrl_block = ptr.ctrl_block;
        if(ctrl_block){
            ctrl_block->weak_count.fetch_add(1,std::memory_order_acq_rel);
        }
    }
    ~MyWeakPtr(){
        if(ctrl_block){
            ctrl_block->weak_count.fetch_sub(1,std::memory_order_acq_rel);
        }
    }

    MyWeakPtr(const MyWeakPtr<Type>& other){
        ctrl_block = other.ctrl_block;
        if(ctrl_block){
            ctrl_block->weak_count.fetch_add(1,std::memory_order_acq_rel);
        }
    }

    MyWeakPtr(MyWeakPtr<Type>&& other){
        ctrl_block = other.ctrl_block;
        other.ctrl_block = nullptr;
    }

    MyWeakPtr& operator=(const MyWeakPtr& other){
        if(this == &other) return *this;
        if(ctrl_block){
            ctrl_block->weak_count.fetch_sub(1, std::memory_order_acq_rel);
        }
        ctrl_block = other.ctrl_block;

        if(ctrl_block){
            ctrl_block->weak_count.fetch_add(1,std::memory_order_acq_rel);
        }
        return *this;
    }

    MyWeakPtr& operator=(MyWeakPtr&& other){
        if(this == &other) return *this;

        if(ctrl_block){
            ctrl_block->weak_count.fetch_sub(1, std::memory_order_acq_rel);
        }
        ctrl_block = other.ctrl_block;
        other.ctrl_block = nullptr;
        return *this;
    }




    MySharedPtr<Type> lock() const{
        if(expired()) return MySharedPtr<Type>();

        long count = ctrl_block->strong_count.load(std::memory_order_acquire);
        while (count != 0) {
            if(ctrl_block->strong_count.compare_exchange_weak(count, count + 1, std::memory_order_acq_rel , std::memory_order_relaxed)){
                return MySharedPtr<Type>(static_cast<Type*>(ctrl_block->get_ptr()), ctrl_block);
            }
        }

        return MySharedPtr<Type>();
    }
    bool expired() const{
        return use_count() == 0;
    }

    long use_count() const{
        if(!ctrl_block) return 0;
        return ctrl_block->strong_count.load(std::memory_order_relaxed);
    }


private:

ControlBlockBase* ctrl_block;
};
