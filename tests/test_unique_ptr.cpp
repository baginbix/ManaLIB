#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include "../src/my_unique_ptr.hpp"

struct Test {
    int x;
    Test(int v) : x(v) {}
};

TEST_CASE("Default constructor creates null pointer") {
    MyUniquePtr<int> ptr;
    REQUIRE(ptr.get() == nullptr);
    REQUIRE(ptr == false);
}

TEST_CASE("Construction with raw pointer") {
    auto ptr = MyUniquePtr<int>(new int(42));
    REQUIRE(*ptr == 42);
    REQUIRE(ptr != false);
}

TEST_CASE("Move construction transfers ownership") {
    MyUniquePtr<int> a(new int(10));
    MyUniquePtr<int> b(std::move(a));

    REQUIRE(a.get() == nullptr);
    REQUIRE(*b == 10);
}

TEST_CASE("Reset releases old memory") {
    auto ptr = MyUniquePtr<int>(new int(7));
    ptr.reset(new int(20));
    REQUIRE(*ptr == 20);
}

TEST_CASE("Release returns pointer without deleting") {
    auto ptr = MyUniquePtr<int>(new int(5));
    int* raw = ptr.release();
    REQUIRE(ptr.get() == nullptr);
    REQUIRE(*raw == 5);
    delete raw;
}

TEST_CASE("Array version supports indexing") {
    MyUniquePtr<int[]> arr(new int[3]{1, 2, 3});
    REQUIRE(arr[0] == 1);
    REQUIRE(arr[1] == 2);
    REQUIRE(arr[2] == 3);
}
