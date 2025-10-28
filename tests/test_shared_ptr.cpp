#include <catch2/catch_test_macros.hpp>
#include "../src/my_shared_ptr.hpp"

// A simple test struct
struct Me {
    int value = 0;
    Me(int val) : value(val) {}
};

TEST_CASE("Basic MySharedPtr operations") {
    MySharedPtr<Me> sp1(new Me(10));
    REQUIRE(sp1);
    REQUIRE(sp1->value == 10);
    REQUIRE(sp1.use_count() == 1);

    MySharedPtr<Me> sp2 = sp1; // copy
    REQUIRE(sp1.use_count() == 2);
    REQUIRE(sp2.use_count() == 2);

    MySharedPtr<Me> sp3 = std::move(sp2); // move
    REQUIRE(sp1.use_count() == 2);
    REQUIRE(!sp2); // moved-from
    REQUIRE(sp3.use_count() == 2);

    sp3.reset();
    REQUIRE(sp3.use_count() == 0);
    REQUIRE(sp1.use_count() == 1);
}

TEST_CASE("MyWeakPtr functionality") {
    MySharedPtr<Me> sp(new Me(42));
    MyWeakPtr<Me> wp(sp);
    REQUIRE(!wp.expired());
    REQUIRE(wp.use_count() == 1);

    MySharedPtr<Me> sp2 = wp.lock();
    REQUIRE(sp2);
    REQUIRE(sp2->value == 42);
    REQUIRE(wp.use_count() == 2);

    sp.reset();
    REQUIRE(!wp.expired());
    REQUIRE(wp.use_count() == 1);

    sp2.reset();
    REQUIRE(wp.expired());
    REQUIRE(wp.use_count() == 0);
}

TEST_CASE("WeakPtr copy and move") {
    MySharedPtr<Me> sp(new Me(100));
    MyWeakPtr<Me> wp1(sp);
    REQUIRE(!wp1.expired());

    MyWeakPtr<Me> wp2 = wp1; // copy
    REQUIRE(wp1.use_count() == 1);
    REQUIRE(wp2.use_count() == 1);

    MyWeakPtr<Me> wp3 = std::move(wp2); // move
    REQUIRE(wp3.use_count() == 1);
}

TEST_CASE("Release and reset") {
    MySharedPtr<Me> sp(new Me(5));
    Me* raw = sp.get();
    REQUIRE(raw->value == 5);

    sp.reset(new Me(10));
    REQUIRE(sp->value == 10);

    MySharedPtr<Me> sp2(new Me(20));
    sp2.release();
    REQUIRE(sp2.get() == nullptr);
}


TEST_CASE("Basic MySharedPtr creation and access") {
    MySharedPtr<Me> sp1(new Me(42));
    REQUIRE(sp1);
    REQUIRE(sp1->value == 42);
    REQUIRE((*sp1).value == 42);
    REQUIRE(sp1.use_count() == 1);
}

TEST_CASE("Copy construction increments strong count") {
    MySharedPtr<Me> sp1(new Me(10));
    MySharedPtr<Me> sp2(sp1);
    REQUIRE(sp1.use_count() == 2);
    REQUIRE(sp2.use_count() == 2);
    REQUIRE(sp1->value == 10);
    REQUIRE(sp2->value == 10);
}

TEST_CASE("Move construction transfers ownership MySharedPtr") {
    MySharedPtr<Me> sp1(new Me(5));
    long old_count = sp1.use_count();
    MySharedPtr<Me> sp2(std::move(sp1));
    REQUIRE(sp1.get() == nullptr);
    REQUIRE(sp2.use_count() == old_count);
}

TEST_CASE("Copy assignment increments strong count") {
    MySharedPtr<Me> sp1(new Me(7));
    MySharedPtr<Me> sp2;
    sp2 = sp1;
    REQUIRE(sp1.use_count() == 2);
    REQUIRE(sp2.use_count() == 2);
}

TEST_CASE("Move assignment transfers ownership") {
    MySharedPtr<Me> sp1(new Me(9));
    MySharedPtr<Me> sp2;
    sp2 = std::move(sp1);
    REQUIRE(sp1.get() == nullptr);
    REQUIRE(sp2->value == 9);
}

TEST_CASE("reset() releases ownership") {
    MySharedPtr<Me> sp1(new Me(20));
    sp1.reset();
    REQUIRE(sp1.get() == nullptr);
    REQUIRE(sp1.use_count() == 0);
}

TEST_CASE("release() releases ownership without resetting pointer") {
    MySharedPtr<Me> sp1(new Me(30));
    sp1.release();
    REQUIRE(sp1.get() == nullptr);
    REQUIRE(sp1.use_count() == 0);
}

TEST_CASE("MyWeakPtr lock and expired") {
    MySharedPtr<Me> sp1(new Me(50));
    MyWeakPtr<Me> wp1(sp1);
    REQUIRE(!wp1.expired());
    REQUIRE(wp1.use_count() == 1);

    MySharedPtr<Me> sp2 = wp1.lock();
    REQUIRE(sp2);
    REQUIRE(sp2->value == 50);
    REQUIRE(sp1.use_count() == 2);

    sp1.reset();
    sp2.reset();
    REQUIRE(wp1.expired());
    REQUIRE(wp1.lock().get() == nullptr);
}

TEST_CASE("Multiple weak_ptrs") {
    MySharedPtr<Me> sp1(new Me(100));
    MyWeakPtr<Me> wp1(sp1);
    MyWeakPtr<Me> wp2(sp1);

    REQUIRE(wp1.use_count() == 1);
    REQUIRE(wp2.use_count() == 1);

    sp1.reset();
    REQUIRE(wp1.expired());
    REQUIRE(wp2.expired());
}

TEST_CASE("Self-assignment in MySharedPtr") {
    MySharedPtr<Me> sp1(new Me(77));
    sp1 = sp1; // self-assignment
    REQUIRE(sp1.use_count() == 1);
    REQUIRE(sp1->value == 77);
}

TEST_CASE("Self-assignment in MyWeakPtr") {
    MySharedPtr<Me> sp1(new Me(88));
    MyWeakPtr<Me> wp1(sp1);
    wp1 = wp1; // self-assignment
    REQUIRE(!wp1.expired());
    REQUIRE(wp1.use_count() == 1);
}

TEST_CASE("lock after shared_ptr reset") {
    MySharedPtr<Me> sp1(new Me(200));
    MyWeakPtr<Me> wp1(sp1);
    sp1.reset();
    MySharedPtr<Me> sp2 = wp1.lock();
    REQUIRE(sp2.get() == nullptr);
}
