#include <cstddef>
#include <utility>
namespace baginbix{

    template<typename T>
    class DLList{
    private:

        struct Node{
            T value;
            Node* next;
            Node* previous;
        };

        class Iterator{
            Node* current;

            Iterator(Node* ptr) : current(ptr) {}

            T* operator->(){
                return &current->value;
            }

            T& operator*(){
                return current->value;
            }
            const T& operator*() const {
                return current->value; // Returns a const reference to the value
            }

            Iterator& operator++(){
                current = current->next;
                return *this;
            }
            Iterator operator++(int) {
                Iterator temp = *this;
                ++(*this);
                return temp;
            }

            Iterator& operator--() {
                current = current->previous;
                return *this;
            }

            bool operator!=(const Iterator& other) const {
                return current != other.current;
            }

            bool operator==(const Iterator& other) const {
                return current == other.current;
            }
        };
    public:
    DLList()
        : num_items(0), head(nullptr){}
    DLList(const DLList<T>& other)
        :num_items(other.num_items), head(nullptr){
        if(num_items == 0) return;
        Node* last = nullptr;
        for (Node* to_copy = other.head; to_copy != nullptr; to_copy = to_copy->next) {
            Node* current = new Node();
            current->value = to_copy->value;
            current->previous = last;
            current->next = nullptr;

            if(last != nullptr){
                last->next = current;
            }
            else {
                head = current;
            }
            last = current;
        }
    }

    DLList(DLList<T>&& other): num_items(other.num_items), head(other.head){
        other.head = nullptr;
        other.num_items = 0;
    }

    DLList<T>& operator=(const DLList<T>& other){
        if(&other == this) return;
        if(num_items > 0){
            delete_nodes();
        }
        Node* last = nullptr;
        for (Node* to_copy = other.head; to_copy != nullptr; to_copy = to_copy->next) {
            Node* current = new Node();
            current->value = to_copy->value;
            current->previous = last;
            current->next = nullptr;

            if(last != nullptr){
                last->next = current;
            }
            else {
                head = current;
            }
            last = current;
        }
        num_items = other.num_items;
        return *this;
    }

    DLList<T>& operator=(DLList<T>&& other){
        if(&other == this) return *this;
        if(num_items > 0){
            delete_nodes();
        }
        head = other.head;
        num_items = other.num_items;
        other.head = nullptr;
        other.num_items = 0;
        return *this;
    }

    void push_front(const T& value){
        Node* new_node = new Node();
        new_node->value = value;
        new_node->next = head;
        new_node->previous = nullptr;
        if(head != nullptr){
            head->previous = new_node;
        }

        head = new_node;
        ++num_items;
    }
    void push_front(T&& value){
        Node* new_node = new Node();
        new_node->value = std::move(value);
        new_node->next = head;
        new_node->previous = nullptr;
        if(head != nullptr){
            head->previous = new_node;
        }
        else{
            tail = new_node;
        }

        head = new_node;
        ++num_items;
    }

    void push_back(const T& value){
        Node* new_node = new Node();
        new_node->value = value;
        new_node->next = nullptr;
        new_node->previous = tail;

        if (head != nullptr) {
            tail->next = new_node;
        }
        else {
            head = new_node;
        }
        tail = new_node;

        ++num_items;
    }

    void push_back(T&& value){
        Node* new_node = new Node();
        new_node->value = std::move(value);
        new_node->next = nullptr;
        new_node->previous = tail;

        if (head != nullptr) {
            tail->next = new_node;
        }
        else {
            head = new_node;
        }
        tail = new_node;

        ++num_items;
    }

    void pop_front(){
        if(num_items == 0) return;
        Node* removed_node = head;
        head = head->next;
        if(head){
            head->previous = nullptr;
        }
        else{
            tail = nullptr;
        }
        delete removed_node;
        --num_items;
    }
    void pop_back(){
        if(num_items == 0) return;
        Node* removed_node = tail;
        if (num_items == 1) {

            head = nullptr;
            tail = nullptr;
        }
        else {
            tail = tail->previous;
            tail->next = nullptr;
        }
        delete removed_node;
        --num_items;
    }

    private:
        std::size_t num_items;
        Node* head;
        Node* tail;

        void delete_nodes(){
            for(Node* current = head; current != nullptr;){
                Node* next = current->next;
                delete current;
                current = next;
            }
            head = nullptr;
            num_items = 0;
        }
    };
}
