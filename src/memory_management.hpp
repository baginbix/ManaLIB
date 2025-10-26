template<typename Type>
struct DefaultDeleter{
    void operator()(Type* ptr) const {delete ptr;}
};
template<typename Type, typename Deleter = DefaultDeleter<Type>>
class MyUniquePtr{
public:
    MyUniquePtr(){
        ptr = nullptr;
    }
    MyUniquePtr(Type* obj)
    :ptr(obj)
    {}

    MyUniquePtr(MyUniquePtr&& obj){
        ptr = obj.ptr;
        obj.ptr = nullptr;
    }
    MyUniquePtr& operator= (MyUniquePtr&& other){
        if(&other == this) return;
        delete ptr;
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    MyUniquePtr(MyUniquePtr& obj) = delete;

    MyUniquePtr& operator= (const MyUniquePtr& other) = delete;


    ~MyUniquePtr(){
         deleter(ptr);

    }
    Type& operator* (){
        return *ptr;
    }
    Type* operator->(){
        return ptr;
    }

    operator bool() const{return ptr != nullptr;}

    Type* Get(){
        return ptr;
    }
private:
    Type* ptr;
    Deleter deleter;
}
