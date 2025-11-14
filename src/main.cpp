#include <cstddef>
#include <iostream>
#include <ostream>
#include <print>
#include <string>
#include <thread>
#include <vector>
#include "mutex_buffer_v1.hpp"
#include "my_unique_ptr.hpp"
#include "vector.hpp"
void test_single_thread_perf() {
    constexpr size_t CAP = 1024;
    constexpr size_t OPS  = 10'000'000; // ten million heroic operations

    AtomicBuffer_V1<int> buffer(CAP);

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < OPS; i++) {
        buffer.push(i);
        buffer.consume_one([](int&) {});
    }
    auto end = std::chrono::high_resolution_clock::now();

    double seconds = std::chrono::duration<double>(end - start).count();
    std::cout << "Single-thread: " << (OPS / seconds) << " ops/sec\n";
}
class Me{

public:
    Me(int hp):value(hp){}
    int value;
};

void test_two_thread_perf() {
    constexpr size_t CAP = 1024;
    constexpr size_t OPS = 10'000'000;

    AtomicBuffer_V1<int> buffer(CAP);

    std::atomic<bool> start_flag{false};
    std::atomic<size_t> consumed{0};

    auto producer = [&] {
        while (!start_flag.load(std::memory_order_acquire)) {}
        for (size_t i = 0; i < OPS; ) {
            if (buffer.push(i))
                ++i;
        }
    };

    auto consumer = [&] {
        while (!start_flag.load(std::memory_order_acquire)) {}
        while (consumed.load(std::memory_order_relaxed) < OPS) {
            buffer.consume_one([&](int& value){
                consumed.fetch_add(1, std::memory_order_relaxed);
            });
        }
    };

    std::thread t1(producer);
    std::thread t2(consumer);

    auto start = std::chrono::high_resolution_clock::now();
    start_flag.store(true, std::memory_order_release);

    t1.join();
    t2.join();

    auto end = std::chrono::high_resolution_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();

    std::cout << "Two-thread SPSC: "
              << (OPS / seconds) << " ops/sec\n";
}

void m(Me* me){delete me;};
int main(){
    baginbix::Vector<std::string> vec;
    for(int i = 0; i < 11; ++i)
        vec.push_back("fjkdsfkldaskfdkf");

    std::vector<int> v;
    v.resize(10);
}
