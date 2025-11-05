#include <cstddef>
namespace baginbix{
    static const size_t BLOCK_SIZE = 64;
    template<typename T>
    class Deque{
    public:
        Deque(): num_items(0){
            map_size = 8;
            map = new T*[map_size];
            for(size_t i = 0; i < map_size; ++i){
                map[i] = nullptr;
            }
            start_block  = map_size/2;
            end_block    = start_block;
            start_offset = 0;
            end_offset   = 0;
            map[start_block] = new T[BLOCK_SIZE];
        }
        Deque(const Deque& other) {
            map_size = other.map_size;
            map = new T * [map_size];
            for (size_t i = 0; i < map_size; ++i) {
                map[i] = nullptr;
                if (other.map[i] != nullptr) {
                    map[i] = new T[BLOCK_SIZE];
                    for (size_t j = 0; j < BLOCK_SIZE; ++j)
                        map[i][j] = other.map[i][j];
                }
            }

            start_block = other.start_block;
            end_block = other.end_block;
            start_offset = other.start_offset;
            end_offset = other.end_offset;
            num_items = other.num_items;
        }

        Deque(Deque&& other) {
            map = other.map;
            map_size = other.map_size;
            start_block = other.start_block;
            end_block = other.end_block;
            start_offset = other.start_offset;
            end_offset = other.end_offset;
            num_items = other.num_items;

            other.map = nullptr;
            other.num_items = 0;
        }
        ~Deque(){
            cleanup();
        }

        Deque& operator=(const Deque& other) {
            if (this == &other) return *this;

            
            cleanup();
            map_size = other.map_size;
            map = new T * [map_size];
            for (size_t i = 0; i < map_size; ++i) {
                map[i] = nullptr;
                if (other.map[i] != nullptr) {
                    map[i] = new T[BLOCK_SIZE];
                    for (size_t j = 0; j < BLOCK_SIZE; ++j)
                        map[i][j] = other.map[i][j];
                }
            }

            start_block = other.start_block;
            end_block = other.end_block;
            start_offset = other.start_offset;
            end_offset = other.end_offset;
            num_items = other.num_items;

            return *this;
        }
        Deque& operator=(Deque&& other) noexcept {
            if (this == &other) return *this;

            cleanup();
            map = other.map;
            map_size = other.map_size;
            start_block = other.start_block;
            end_block = other.end_block;
            start_offset = other.start_offset;
            end_offset = other.end_offset;
            num_items = other.num_items;

            other.map = nullptr;
            other.map_size = 0;
            other.start_block = 0;
            other.end_block = 0;
            other.start_offset = 0;
            other.end_offset = 0;
            other.num_items = 0;

            return *this;
        }

        T& operator[](size_t index);
        T& operator[](size_t index) const;

        void push_back(const T& item) {
            map[end_block][end_offset] = item;
            ++end_offset;
            ++num_items;
            if (end_offset == BLOCK_SIZE) {
                ++end_block;
                if (end_block == map_size) {
                    T** new_map;
                    size_t new_map_size = map_size * 2;
                    new_map = new T * [new_map_size];
                    for (size_t i = 0; i < map_size; ++i) {
                        new_map[i] = map[i];
                    }
                    for (size_t i = map_size; i < new_map_size; i++)
                    {
                        new_map[i] = nullptr;
                    }
                    delete[] map;
                    map_size = new_map_size;
                    map = new_map;
                }
                map[end_block] = new T[BLOCK_SIZE];
                end_offset = 0;
            }
        }
        void push_front(const T& item);
        void pop_back();
        void pop_front();

        std::size_t size() const;

    private:
        T** map;
        size_t map_size;

        size_t start_block;
        size_t end_block;

        size_t start_offset;
        size_t end_offset;

        size_t num_items;

        void cleanup() noexcept{
            if (!map)return;
            for (size_t i = 0; i < map_size; ++i) {
                if (map[i])
                    delete[] map[i];
            }
            delete[] map;
            map = nullptr;
            map_size = 0;
        }

    };
}
