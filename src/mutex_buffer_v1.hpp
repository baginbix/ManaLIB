#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <unistd.h>
#include <vector>
template<typename T>
class MutexBuffer_V1{
public:
    MutexBuffer_V1(std::size_t capacity): capacity{capacity}{
        buffer.resize(capacity);
    }

    bool push(const T& item){
        std::lock_guard<std::mutex> lock(mutex);
        std::size_t next_tail = (tail + 1) % capacity;
        if (next_tail == head) {
            return false;
        }
        buffer[tail] = item;
        tail = next_tail;
        return true;
    }

    bool consume_one(auto&& func){
        std::lock_guard<std::mutex> lock(mutex);
        if(head == tail){
            return false;
        }
        T elem = std::move(buffer[head]);
        func(std::move(elem));
        head = (head + 1) % capacity;
        return true;
    }

private:
    std::size_t const capacity;
    std::size_t head = 0;
    std::size_t tail = 0;
    std::vector<T> buffer;
    std::mutex mutex;
};

template<typename T>
class MutexBuffer_V2{
public:
    MutexBuffer_V2(std::size_t capacity): capacity{capacity}{
        buffer = static_cast<Element*>(malloc(capacity * sizeof(T)));
    }

    bool push(const T& item){
        std::lock_guard<std::mutex> lock(mutex);
        std::size_t next_tail = (tail + 1) % capacity;
        if (next_tail == head) {
            return false;
        }
        std::construct_at(reinterpret_cast<T*>(&buffer[tail]), item);
        tail = next_tail;
        return true;
    }

    bool consume_one(auto&& func){
        std::lock_guard<std::mutex> lock(mutex);
        if(head == tail){
            return false;
        }
        T& elem = *reinterpret_cast<T*>(&buffer[head]);
        func(elem);
        std::destroy_at(&elem);
        head = (head + 1) % capacity;
        return true;
    }

private:
    struct Element{
        alignas(T) std::byte storage[sizeof(T)];
    };
    std::size_t capacity;
    std::size_t head = 0;
    std::size_t tail = 0;
    Element* buffer;
    std::mutex mutex;
};

template<typename T>
class AtomicBuffer_V1{
public:
    AtomicBuffer_V1(std::size_t capacity): capacity{capacity}{
        buffer = static_cast<Element*>(malloc(capacity * sizeof(T)));
    }

    bool push(const T& item){
        std::size_t const curr_tail = tail.load(std::memory_order_relaxed);
        std::size_t next_tail = (curr_tail + 1) % capacity;
        if (next_tail == head.load(std::memory_order_acquire)) {
            return false;
        }
        std::construct_at(reinterpret_cast<T*>(&buffer[curr_tail]), item);
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    bool consume_one(auto&& func){
        std::size_t const curr_head = head.load(std::memory_order_relaxed);
        if(head == tail.load(std::memory_order_acquire)){
            return false;
        }
        T& elem = *reinterpret_cast<T*>(&buffer[curr_head]);
        func(elem);
        std::destroy_at(&elem);
        head.store((curr_head + 1) % capacity,std::memory_order_release);
        return true;
    }

    ~AtomicBuffer_V1(){
        auto noop = [](T&){};
        while (consume_one(noop));
        free(buffer);
    }

private:
    struct Element{
        alignas(T) std::byte storage[sizeof(T)];
    };
    std::size_t capacity;
    std::atomic<std::size_t> head = 0;
    std::atomic<std::size_t> tail = 0;
    Element* buffer;
};

class TinyMutex{
    public:
    void lock(){
        while (flag.test_and_set(std::memory_order_acquire)) {

        }
    }

private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};


class Futex{
public:
    Futex():state(0){}

    void lock(){
        int expected = 0;
        if(state.compare_exchange_strong(expected, 1, std::memory_order_acquire)){
            return;
        }
        while(true){
            if(expected == 1 && state.compare_exchange_strong(expected, 2, std::memory_order_acquire)){
                futex_wait(&state, 2);
            }

            expected = 0;
            if(state.compare_exchange_strong(expected, 2, std::memory_order_acquire)){
                return;
            }
        }
    }

    void unlock(){
        int prev = state.exchange(0, std::memory_order_release);

        if(prev == 2){
            futex_wake(&state);
        }
    }
private:

    static int futex_wait(std::atomic<int>* addr, int expected){
        return syscall(SYS_futex, reinterpret_cast<int*>(addr), FUTEX_WAIT, expected, nullptr, nullptr, 0);
    }

    static int futex_wake(std::atomic<int>* addr){
        return syscall(SYS_futex, reinterpret_cast<int*>(addr), FUTEX_WAKE, 1, nullptr, nullptr,0);
    }
    std::atomic<int> state;
};
