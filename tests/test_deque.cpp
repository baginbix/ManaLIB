#include <catch2/catch_test_macros.hpp>
#include "../src/deque.hpp"

using namespace baginbix;

TEST_CASE("Deque basic push and pop", "[deque]") {
    Deque<int> dq;

    dq.push_back(1);
    dq.push_back(2);
    dq.push_back(3);

    REQUIRE(dq.size() == 3);
    REQUIRE(dq.front() == 1);
    REQUIRE(dq.back() == 3);

    dq.pop_back();
    REQUIRE(dq.back() == 2);

    dq.pop_front();
    REQUIRE(dq.front() == 2);

    dq.pop_front();
    REQUIRE(dq.size() == 0);
}

TEST_CASE("Deque push_front and pop_front", "[deque]") {
    Deque<int> dq;

    dq.push_front(10);
    dq.push_front(20);
    dq.push_front(30);

    REQUIRE(dq.size() == 3);
    REQUIRE(dq.front() == 30);
    REQUIRE(dq.back() == 10);

    dq.pop_front();
    REQUIRE(dq.front() == 20);

    dq.pop_back();
    REQUIRE(dq.back() == 20);
}

TEST_CASE("Deque random access and correctness", "[deque]") {
    Deque<int> dq;

    for (int i = 0; i < 100; ++i)
        dq.push_back(i);

    REQUIRE(dq.size() == 100);
    for (int i = 0; i < 100; ++i)
        REQUIRE(dq[i] == i);

    dq.pop_front();
    dq.pop_front();
    dq.pop_back();
    dq.push_front(999);

    REQUIRE(dq.front() == 999);
    REQUIRE(dq[0] == 999);
}

TEST_CASE("Deque growth and reallocation", "[deque]") {
    Deque<int> dq;
    const int count = 10000;

    for (int i = 0; i < count; ++i)
        dq.push_back(i);

    REQUIRE(dq.size() == count);
    REQUIRE(dq.front() == 0);
    REQUIRE(dq.back() == count - 1);

    for (int i = 0; i < count / 2; ++i)
        dq.pop_front();

    REQUIRE(dq.front() == count / 2);

    for (int i = 0; i < count / 2; ++i)
        dq.pop_back();

    REQUIRE(dq.size() == 0);
}

TEST_CASE("Deque copy and move semantics", "[deque]") {
    Deque<int> dq;
    for (int i = 0; i < 10; ++i)
        dq.push_back(i);

    SECTION("Copy constructor") {
        Deque<int> dq_copy = dq;
        REQUIRE(dq_copy.size() == dq.size());
        for (int i = 0; i < 10; ++i)
            REQUIRE(dq_copy[i] == i);
    }

    SECTION("Move constructor") {
        Deque<int> dq_copy = dq;
        Deque<int> dq_move = std::move(dq_copy);
        REQUIRE(dq_move.size() == 10);
        for (int i = 0; i < 10; ++i)
            REQUIRE(dq_move[i] == i);
    }

    SECTION("Copy assignment") {
        Deque<int> dq_copy;
        dq_copy = dq;
        REQUIRE(dq_copy.size() == dq.size());
        for (int i = 0; i < 10; ++i)
            REQUIRE(dq_copy[i] == i);
    }

    SECTION("Move assignment") {
        Deque<int> dq_copy = dq;
        Deque<int> dq_move;
        dq_move = std::move(dq_copy);
        REQUIRE(dq_move.size() == 10);
        for (int i = 0; i < 10; ++i)
            REQUIRE(dq_move[i] == i);
    }
}
