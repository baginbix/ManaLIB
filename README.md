# ManaLIB

*Your core resource pool for casting C++ spells.*

Every good RPG character needs a reliable mana pool to cast their best spells. In C++, your standard template library (STL) is that pool. **ManaLIB** is a from-scratch, bare-metal rewrite of the C++ STL, built to strip away the magic and see exactly how it works under the hood.

They say "never reinvent the wheel." But reinventing the wheel is the absolute best way to learn how wheels are made! 

This is a personal learning project by [@baginbix](https://github.com/baginbix) designed to master C++ templates, raw memory management, and thread-safe concurrency.

## The Grimoire (What's Inside)

Currently brewing in the cauldron:

### Containers
* **`baginbix::Vector`** - The trusty broadsword of dynamic arrays. Built with raw memory allocation (`::operator new`), placement new, and `type_traits` for optimized trivial copies.
* **`baginbix::Deque`** - A double-ended queue utilizing a chunked map allocation strategy for fast front-and-back operations.
* **`baginbix::DLList`** - A raw doubly-linked list featuring custom bidirectional iterators.

### Smart Pointers (Memory Magic)
* **`baginbix::MyUniquePtr`** - Soulbound memory. Strictly exclusive ownership with support for custom deleters and array types (`T[]`).
* **`baginbix::MySharedPtr` & `MyWeakPtr`** - Thread-safe, shared memory management. Implements a custom control block with `std::atomic` reference counting (separate strong and weak counts) to prevent data races.

### Concurrency (Dark Arts)
* **SPSC Atomic Buffers** - Single-Producer/Single-Consumer lock-free buffer testing capable of pushing 10+ million operations a second across multiple threads.

## Quick Cast

```cpp
#include <iostream>
#include "vector.hpp"
#include "my_shared_ptr.hpp"

int main() {
    // Fill up your mana pool using your custom Vector
    baginbix::Vector<int> mana_points;
    mana_points.push_back(100);
    mana_points.push_back(250);

    // Share your items safely across threads
    baginbix::MySharedPtr<int> shared_potion(new int(50));
    baginbix::MyWeakPtr<int> weak_potion(shared_potion);

    if (auto locked_potion = weak_potion.lock()) {
        std::cout << "Potion strength: " << *locked_potion << "\n";
    }
    
    return 0;
}
