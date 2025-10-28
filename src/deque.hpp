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
        ~Deque(){
            for(size_t i = 0; i < map_size; ++i){
                if(map[i])
                    delete[] map[i];
            }
            delete[] map;
        }

        T& operator[](size_t index);
        T& operator[](size_t index) const;

        void push_back(const T& item);
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

    };
}
