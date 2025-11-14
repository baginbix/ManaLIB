#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include "../src/vector.hpp"

using namespace baginbix;

struct Tracker {
    static inline int constructions = 0;
    static inline int destructions = 0;
    static inline int copies = 0;
    static inline int moves = 0;

    int value;

    Tracker(int v = 0) : value(v) { ++constructions; }
    Tracker(const Tracker& other) : value(other.value) { ++copies; }
    Tracker(Tracker&& other) noexcept : value(other.value) { other.value = -1; ++moves; }
    ~Tracker() { ++destructions; }

    Tracker& operator=(const Tracker& other) {
        value = other.value;
        ++copies;
        return *this;
    }

    Tracker& operator=(Tracker&& other) noexcept {
        value = other.value;
        other.value = -1;
        ++moves;
        return *this;
    }

    static void reset() {
        constructions = destructions = copies = moves = 0;
    }
};

TEST_CASE("Vector basic push_back and indexing") {
    Vector<int> v;

    REQUIRE(v.size() == 0);
    REQUIRE(v.capacity() >= 10);

    v.push_back(42);
    REQUIRE(v.size() == 1);
    REQUIRE(v[0] == 42);

    v.push_back(99);
    REQUIRE(v[1] == 99);
}

TEST_CASE("Vector automatic resizing works") {
    Vector<int> v;
    size_t initial_cap = v.capacity();

    for (int i = 0; i < 200; ++i)
        v.push_back(i);

    REQUIRE(v.size() == 200);
    REQUIRE(v.capacity() >= 200);
    REQUIRE(v.capacity() > initial_cap);
}

TEST_CASE("Vector shrinking after pop_back") {
    Vector<int> v;

    for (int i = 0; i < 100; ++i)
        v.push_back(i);

    size_t big_cap = v.capacity();

    for (int i = 0; i < 90; ++i)
        v.pop_back();

    REQUIRE(v.size() == 10);
    REQUIRE(v.capacity() <= big_cap);
}

TEST_CASE("Copy constructor and assignment") {
    Vector<int> v1;
    for (int i = 0; i < 5; ++i)
        v1.push_back(i);

    Vector<int> v2 = v1; // copy constructor
    REQUIRE(v2.size() == 5);
    REQUIRE(v2[3] == 3);

    Vector<int> v3;
    v3 = v1; // copy assignment
    REQUIRE(v3.size() == 5);
    REQUIRE(v3[4] == 4);
}

TEST_CASE("Move constructor and assignment") {
    Vector<int> v1;
    for (int i = 0; i < 5; ++i)
        v1.push_back(i);

    size_t old_cap = v1.capacity();

    Vector<int> v2 = std::move(v1); // move constructor
    REQUIRE(v2.size() == 5);
    REQUIRE(v2.capacity() == old_cap);
    REQUIRE(v2[0] == 0);

    Vector<int> v3;
    v3 = std::move(v2); // move assignment
    REQUIRE(v3.size() == 5);
    REQUIRE(v3[1] == 1);
}

TEST_CASE("Emplace_back constructs in-place") {
    Tracker::reset();
    {
        Vector<Tracker> v;
        v.emplace_back(10);
        v.emplace_back(20);
        v.emplace_back(30);
        REQUIRE(v.size() == 3);
        REQUIRE(v[0].value == 10);
        REQUIRE(v[2].value == 30);
    }
    // check that destructors are called for all
    REQUIRE(Tracker::constructions >= 3);
    REQUIRE(Tracker::destructions == Tracker::constructions);
}

TEST_CASE("Reserve increases capacity but not size") {
    Vector<int> v;
    size_t before = v.capacity();
    v.reserve(before * 4);
    REQUIRE(v.capacity() >= before * 4);
    REQUIRE(v.size() == 0);
}

TEST_CASE("Clear resets size but not capacity") {
    Vector<int> v;
    for (int i = 0; i < 20; ++i)
        v.push_back(i);

    size_t before = v.capacity();
    v.clear();
    REQUIRE(v.size() == 0);
    REQUIRE(v.capacity() == before);
}

TEST_CASE("Vector empty state and boundary stress") {
    Vector<int> v;

    // Test pop_back on empty vector (should do nothing, not crash)
    v.pop_back();
    REQUIRE(v.size() == 0);

    // Test single element
    v.push_back(10);
    REQUIRE(v[0] == 10);
    REQUIRE(v.size() == 1);

    // Test pop_back to empty
    v.pop_back();
    REQUIRE(v.size() == 0);

    // Test push_back after pop_back to empty (re-use of memory)
    v.push_back(20);
    REQUIRE(v[0] == 20);
}

TEST_CASE("Move semantics during resize are used correctly") {
    Tracker::reset();
    {
        Vector<Tracker> v;
        // Assume initial capacity is small, e.g., 10 (or force small for test)
        // Insert enough to force a resize (e.g., 11 if capacity is 10)

        for (int i = 0; i < 10; ++i) {
            v.emplace_back(i); // 10 constructions
        }

        // Force Reallocation (This will trigger moves)
        v.emplace_back(10);

        // 1. Initial 10 elements constructed (10 constructions)
        // 2. The 11th element constructed in place (1 construction)
        // 3. The original 10 elements should have been MOVED, not copied (10 moves)
        // 4. The original 10 elements must be destructed after the move (10 destructions)

        REQUIRE(Tracker::constructions == 11);
        REQUIRE(Tracker::copies == 0); // Crucial: must be zero copies
        REQUIRE(Tracker::moves == 10);
        REQUIRE(Tracker::destructions == 10); // Destructions of the moved-from objects

        REQUIRE(v.size() == 11);
        REQUIRE(v[0].value == 0);
        REQUIRE(v[10].value == 10);
        // Ensure moved-from objects were nulled/reset (optional but good practice)
        REQUIRE(v[0].value != -1);
    }
    // Check total cleanup
    REQUIRE(Tracker::destructions == Tracker::constructions + Tracker::moves);
}

TEST_CASE("Vector iterators and range-based loop functionality") {
    Vector<int> v;
    for (int i = 10; i < 15; ++i) {
        v.push_back(i);
    }

    // Test manual iteration (read-only)
    int sum = 0;
    for (auto it = v.begin(); it != v.end(); ++it) {
        sum += *it;
    }
    REQUIRE(sum == 60); // 10 + 11 + 12 + 13 + 14 = 60

    // Test modification via range-based loop (requires non-const T& operator*)
    for (int& val : v) {
        val *= 2;
    }
    REQUIRE(v[0] == 20);
    REQUIRE(v[4] == 28);
}
