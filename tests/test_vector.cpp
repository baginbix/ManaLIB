// test_vector_edges.cpp
// Edge-case & regression tests for baginbix::Vector.
// Add this file to the same Catch2 project as your existing tests.
// No CATCH_CONFIG_MAIN here — your existing test file provides the entry point.
//
// Two tests are RED on purpose until the vector is fixed:
//   - "insert(T&&) moves the value"            (fails: the const T&& overload copies)
//   - "move-only element type works"           (won't compile until insert(size_t, T&&) exists)
// Fix:  void insert(size_t index, T&& value)   // not const T&&
//
// Build tip (Visual Studio): enable AddressSanitizer
//   Project Properties > C/C++ > Enable Address Sanitizer (/fsanitize=address)
// The Bomb tests then also *prove* there is no leak on the throwing paths.

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "../src/vector.hpp"

using baginbix::Vector;

// ---------------------------------------------------------------------------
// Contract checks the compiler enforces forever, for free
// ---------------------------------------------------------------------------

static_assert(std::is_nothrow_move_constructible_v<Vector<int>>);
static_assert(std::is_nothrow_move_assignable_v<Vector<int>>);
static_assert(std::is_nothrow_swappable_v<Vector<int>>);

// Reading through a const vector must NOT allow mutation:
// const at() yields Optional<const T>, whose unwrap() is a const reference.
static_assert(!std::is_assignable_v<
    decltype(std::declval<baginbix::Optional<const int>>().unwrap()), int>,
    "const at() must return a read-only view");

// ---------------------------------------------------------------------------
// Helper types
// ---------------------------------------------------------------------------

// Counts every special member separately — precision matters.
struct Counting {
    static inline int ctors = 0, copy_ctor = 0, move_ctor = 0,
        copy_assign = 0, move_assign = 0, dtors = 0;

    int value;
    Counting(int v = 0) : value(v) { ++ctors; }
    Counting(const Counting& o) : value(o.value) { ++copy_ctor; }
    Counting(Counting&& o) noexcept : value(o.value) { o.value = -1; ++move_ctor; }
    Counting& operator=(const Counting& o) { value = o.value; ++copy_assign; return *this; }
    Counting& operator=(Counting&& o) noexcept { value = o.value; o.value = -1; ++move_assign; return *this; }
    ~Counting() { ++dtors; }

    static void reset() {
        ctors = copy_ctor = move_ctor = copy_assign = move_assign = dtors = 0;
    }
};

// Copy constructor explodes when the fuse reaches zero.
// The move constructor is deliberately NOT noexcept, so inside reallocate()
// move_if_noexcept falls back to copy — and the guard must catch the throw.
struct Bomb {
    static inline int fuse = -1;   // -1 = never throw

    int value = 0;
    Bomb(int v = 0) : value(v) {}
    Bomb(const Bomb& o) : value(o.value) {
        if (fuse > 0 && --fuse == 0) throw std::runtime_error("boom");
    }
    Bomb(Bomb&& o) : value(o.value) {}   // intentionally not noexcept
    Bomb& operator=(const Bomb& o) { value = o.value; return *this; }
};

// Forces the over-aligned allocation path (alignof > __STDCPP_DEFAULT_NEW_ALIGNMENT__).
struct alignas(64) Big {
    char pad[64];
};

// ---------------------------------------------------------------------------
// Copy / move correctness
// ---------------------------------------------------------------------------

TEST_CASE("copy construction preserves every element (memcpy regression)") {
    Vector<int> v;
    for (int i = 0; i < 100; ++i)
        v.push_back(i);
    Vector<int> c = v;
    REQUIRE(c == v);                 // fails if the memcpy size used an element count

    Vector<std::string> vs{ "a", "bb", "ccc" };   // non-trivially-copyable path
    Vector<std::string> cs = vs;
    REQUIRE(cs == vs);

    Vector<int> empty;
    Vector<int> ce = empty;          // copying an empty vector must be a valid state
    REQUIRE(ce.empty());
    ce.push_back(7);
    REQUIRE(ce.size() == 1);
}

TEST_CASE("a moved-from vector is empty and reusable") {
    Vector<int> v;
    v.push_back(1);
    v.push_back(2);

    Vector<int> w = std::move(v);
    REQUIRE(v.empty());              // documented by our move ctor
    REQUIRE(w == (Vector<int>{1, 2}));

    v.push_back(9);                  // used to crash: grow() on cap == 0
    REQUIRE(v.size() == 1);
    REQUIRE(v[0] == 9);
}

TEST_CASE("self-assignment and self-move are safe") {
    Vector<int> v{ 1, 2, 3 };
    Vector<int>& ref = v;

    ref = v;                            // copy self-assign (copy-and-swap)
    REQUIRE(v == (Vector<int>{1, 2, 3}));

    ref = std::move(v);                 // self-move: copy-and-swap makes this safe here
    REQUIRE(v == (Vector<int>{1, 2, 3}));
    REQUIRE(v.size() == 3);
}

TEST_CASE("push_back(T&&) actually moves") {
    Counting::reset();
    {
        Vector<Counting> v;
        Counting a(5);
        v.push_back(std::move(a));
        REQUIRE(Counting::move_ctor == 1);
        REQUIRE(Counting::copy_ctor == 0);
        REQUIRE(a.value == -1);         // the source was really moved from

        Counting b(6);
        v.push_back(b);                 // lvalue: must copy
        REQUIRE(Counting::copy_ctor == 1);
        REQUIRE(b.value == 6);          // and not disturb the source
    }
}

TEST_CASE("no leaks and no double-destruction across a mixed workload") {
    Counting::reset();
    {
        Vector<Counting> v;
        for (int i = 0; i < 30; ++i) v.emplace_back(i);
        Vector<Counting> c = v;
        Vector<Counting> m = std::move(v);
        v = c;                            // copy-assign over the moved-from vector
        v.push_back(Counting(99));
        v.erase(2);
        v.clear();
        v.shrink_to_fit();
    }
    // Every object ever created (value-ctor + copy + move) must be destroyed
    // exactly once — this fails on leaks AND on double-destruction.
    REQUIRE(Counting::dtors
        == Counting::ctors + Counting::copy_ctor + Counting::move_ctor);
    REQUIRE(Counting::copy_ctor > 0);     // the copy paths were actually exercised
    REQUIRE(Counting::move_ctor > 0);
}

// ---------------------------------------------------------------------------
// insert / erase — including the aliasing cases that found real bugs
// ---------------------------------------------------------------------------

TEST_CASE("insert keeps order at front, middle and end") {
    Vector<int> v{ 1, 2, 3, 4, 5 };

    v.insert(0, 10);                        // front
    REQUIRE(v == (Vector<int>{10, 1, 2, 3, 4, 5}));

    v.insert(3, 99);                        // middle
    REQUIRE(v == (Vector<int>{10, 1, 2, 99, 3, 4, 5}));

    v.insert(v.size(), 0);                  // end
    REQUIRE(v == (Vector<int>{10, 1, 2, 99, 3, 4, 5, 0}));
}

TEST_CASE("insert with an argument aliasing the vector (const T&)") {
    Vector<int> v{ 1, 2, 3, 4, 5 };

    v.insert(0, v[0]);                      // self-aliasing
    REQUIRE(v == (Vector<int>{1, 1, 2, 3, 4, 5}));

    v = Vector<int>{ 1, 2, 3, 4, 5 };
    v.insert(0, v[2]);                      // alias inside the shifted range
    REQUIRE(v == (Vector<int>{3, 1, 2, 3, 4, 5}));

    // alias + forced reallocation: the reference would dangle without the snapshot
    Vector<int> w;
    for (int i = 0; i < 10; ++i) w.push_back(i);   // w is at capacity
    w.insert(3, w[8]);
    REQUIRE(w == (Vector<int>{0, 1, 2, 8, 3, 4, 5, 6, 7, 8, 9}));
}

TEST_CASE("insert with an argument aliasing the vector (T&&)") {
    Vector<int> w;
    for (int i = 0; i < 10; ++i) w.push_back(i);
    w.insert(0, std::move(w[7]));
    REQUIRE(w == (Vector<int>{7, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
}

TEST_CASE("insert(T&&) moves the value (regression for the const T&& overload)") {
    Counting::reset();
    {
        Vector<Counting> v;
        v.emplace_back(Counting(1));
        v.emplace_back(Counting(2));

        Counting x(9);
        v.insert(0, std::move(x));

        REQUIRE(x.value == -1);           // RED today: const T&& copies, never moves
        REQUIRE(Counting::copy_ctor == 0); // RED today
        REQUIRE(v[0].value == 9);
        REQUIRE(v.size() == 3);
    }
}

TEST_CASE("move-only element type works (unique_ptr)") {
    Vector<std::unique_ptr<int>> v;
    v.push_back(std::make_unique<int>(1));
    v.insert(0, std::make_unique<int>(0));   // RED today: won't compile until insert(T&&) exists
    v.push_back(std::make_unique<int>(2));
    REQUIRE(v.size() == 3);
    REQUIRE(*v[0] == 0);
    REQUIRE(*v[1] == 1);
    REQUIRE(*v[2] == 2);

    auto p = std::make_unique<int>(42);
    v.insert(1, std::move(p));
    REQUIRE(p == nullptr);                    // ownership actually transferred
    REQUIRE(*v[1] == 42);
    REQUIRE(v.size() == 4);
}

TEST_CASE("erase removes and shifts down") {
    Vector<int> v{ 1, 2, 3, 4, 5 };

    v.erase(0);                     // front
    REQUIRE(v == (Vector<int>{2, 3, 4, 5}));

    v.erase(2);                     // middle
    REQUIRE(v == (Vector<int>{2, 3, 5}));

    v.erase(2);                     // last element
    REQUIRE(v == (Vector<int>{2, 3}));

    v.erase(0);
    v.erase(0);
    REQUIRE(v.empty());

    v.erase(99);                    // out of range: documented no-op policy
    REQUIRE(v.empty());
}

TEST_CASE("emplace_back with a self-referencing argument") {
    Vector<int> v;
    for (int i = 0; i < 10; ++i) v.push_back(i);   // v is at capacity
    v.emplace_back(v[0]);                          // growth path: snapshot taken first
    REQUIRE(v.size() == 11);
    REQUIRE(v[10] == 0);
    REQUIRE(v[0] == 0);

    Vector<int> w{ 1, 2, 3 };
    w.emplace_back(w[1]);                          // spare capacity: direct construction
    REQUIRE(w == (Vector<int>{1, 2, 3, 2}));
}

// ---------------------------------------------------------------------------
// Exception safety — the NewBufferGuard's report card
// ---------------------------------------------------------------------------

TEST_CASE("throwing copy during copy construction leaves the source intact") {
    Vector<Bomb> v;
    for (int i = 0; i < 5; ++i) v.emplace_back(i);

    Bomb::fuse = 3;                       // the 3rd copied element explodes
    REQUIRE_THROWS_AS([&] { Vector<Bomb> c(v); (void)c; }(), std::runtime_error);
    Bomb::fuse = -1;

    REQUIRE(v.size() == 5);               // source untouched
    for (int i = 0; i < 5; ++i)
        REQUIRE(v[i].value == i);

    v.emplace_back(9);                    // container still fully usable
    REQUIRE(v.size() == 6);
}

TEST_CASE("throwing relocation during grow leaves the vector unchanged") {
    Vector<Bomb> v;
    for (int i = 0; i < 10; ++i) v.emplace_back(i);   // at capacity

    Bomb::fuse = 1;                      // the first relocated element explodes
    REQUIRE_THROWS_AS(v.emplace_back(10), std::runtime_error);
    Bomb::fuse = -1;

    REQUIRE(v.size() == 10);             // old state fully intact
    REQUIRE(v.capacity() == 10);
    for (int i = 0; i < 10; ++i)
        REQUIRE(v[i].value == i);

    v.emplace_back(11);                  // works fine afterwards
    REQUIRE(v.size() == 11);
}

// ---------------------------------------------------------------------------
// Allocation & alignment
// ---------------------------------------------------------------------------

TEST_CASE("over-aligned element types get aligned storage") {
    Vector<Big> v;
    v.push_back(Big{});
    REQUIRE(reinterpret_cast<std::uintptr_t>(v.data_ptr()) % alignof(Big) == 0);

    for (int i = 0; i < 5; ++i) v.push_back(Big{});   // force a reallocation too
    REQUIRE(reinterpret_cast<std::uintptr_t>(v.data_ptr()) % alignof(Big) == 0);
}

TEST_CASE("capacity doubles when full") {
    Vector<int> v;
    REQUIRE(v.capacity() == 10);
    for (int i = 0; i < 10; ++i) v.push_back(i);
    REQUIRE(v.capacity() == 10);
    v.push_back(10);
    REQUIRE(v.capacity() == 20);
    for (int i = 11; i < 20; ++i) v.push_back(i);
    v.push_back(20);
    REQUIRE(v.capacity() == 40);
    REQUIRE(v.size() == 21);
    REQUIRE(v == (Vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20}));
}

TEST_CASE("shrink_to_fit keeps elements and trims capacity") {
    Vector<int> v;
    v.reserve(100);
    for (int i = 0; i < 5; ++i) v.push_back(i);

    v.shrink_to_fit();
    REQUIRE(v.capacity() == 5);
    REQUIRE(v == (Vector<int>{0, 1, 2, 3, 4}));

    v.push_back(5);                      // still usable at the new capacity
    REQUIRE(v.size() == 6);
}

// ---------------------------------------------------------------------------
// Lookup & empty-state behavior
// ---------------------------------------------------------------------------

TEST_CASE("at() reports presence and absence") {
    Vector<int> v{ 1, 2, 3 };

    REQUIRE(v.at(1));
    REQUIRE(v.at(1).unwrap() == 2);
    REQUIRE_FALSE(v.at(3));              // one past the end
    REQUIRE_FALSE(v.at(1000));

    v.at(1).unwrap() = 42;               // mutation through the reference
    REQUIRE(v[1] == 42);

    const Vector<int>& cv = v;
    REQUIRE(cv.at(0));
    REQUIRE(cv.at(0).unwrap() == 1);
    REQUIRE_FALSE(cv.at(3));

    Vector<int> empty;
    REQUIRE_FALSE(empty.at(0));
}

TEST_CASE("find and contains") {
    Vector<int> v{ 10, 20, 30, 20 };

    REQUIRE(v.contains(20));
    REQUIRE(v.find(20) == 1);                        // first match
    REQUIRE_FALSE(v.contains(99));
    REQUIRE(v.find(99) == static_cast<size_t>(-1));  // npos — consider naming it
    REQUIRE(v.find(30) == 2);
}

TEST_CASE("empty vector edge behavior") {
    Vector<int> v;
    REQUIRE(v.begin() == v.end());

    v.pop_back();                        // no-op, must not crash
    REQUIRE(v.empty());
    v.erase(0);                          // no-op
    REQUIRE(v.empty());
    v.clear();
    REQUIRE(v.empty());

    // range-for over empty must execute zero iterations
    int iterations = 0;
    for (int x : v) { (void)x; ++iterations; }
    REQUIRE(iterations == 0);
}

TEST_CASE("initializer_list construction") {
    Vector<int> v{ 1, 2, 3, 4, 5 };
    REQUIRE(v.size() == 5);
    REQUIRE(v == (Vector<int>{1, 2, 3, 4, 5}));

    Vector<int> e{};
    REQUIRE(e.empty());

    Vector<int> big{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };   // bigger than min capacity
    REQUIRE(big.size() == 12);
    REQUIRE(big[11] == 12);
}

TEST_CASE("swap exchanges contents") {
    Vector<int> a{ 1, 2, 3 };
    Vector<int> b{ 9, 8, 7, 6 };
    a.swap(b);
    REQUIRE(a == (Vector<int>{9, 8, 7, 6}));
    REQUIRE(b == (Vector<int>{1, 2, 3}));
}

TEST_CASE("const iteration is read-only and correct") {
    Vector<int> v{ 1, 2, 3 };
    const Vector<int>& cv = v;
    int sum = 0;
    for (const int& x : cv) sum += x;
    REQUIRE(sum == 6);
}

// ---------------------------------------------------------------------------
// Oracle: same workload against std::vector, compare after every step
// ---------------------------------------------------------------------------

TEST_CASE("oracle: mirrors std::vector through a mixed workload") {
    Vector<int> mine;
    std::vector<int> oracle;

    auto verify = [&] {
        REQUIRE(mine.size() == oracle.size());
        for (size_t i = 0; i < oracle.size(); ++i)
            REQUIRE(mine[i] == oracle[i]);
        };

    for (int i = 0; i < 50; ++i) { mine.push_back(i * 3); oracle.push_back(i * 3); }
    verify();

    for (int i = 0; i < 10; ++i) { mine.erase(5); oracle.erase(oracle.begin() + 5); }
    verify();

    mine.insert(3, 999);   oracle.insert(oracle.begin() + 3, 999);   verify();
    mine.insert(0, -1);   oracle.insert(oracle.begin(), -1);        verify();
    mine.insert(mine.size(), 7); oracle.push_back(7);               verify();

    mine.pop_back();  oracle.pop_back();
    verify();

    auto mine_copy = mine;
    auto oracle_copy = oracle;
    REQUIRE(mine_copy == mine);
    REQUIRE(oracle_copy == oracle);
}