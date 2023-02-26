#ifndef X17_VECTOR_HPP
#define X17_VECTOR_HPP

#include <cstdint>
#include <iostream>
#include <cstdio>

#ifdef X17_VDEBUG
#define vector_log()                                            \
    fprintf(stderr, "vector on addr %p entered (( %s ))", this, \
            __PRETTY_FUNCTION__)
#else
#define vector_log()
#endif

namespace X17 {

template <typename T>
class vector {
   public:
    typedef T* iterator;
    typedef const T* const const_iterator;

    typedef T& reference;
    typedef const T& const_reference;

   public:
    explicit vector()
        : m_capacity(DEFAULT_CAPACITY), m_size(0), m_typesize(sizeof(T)) {
        vector_log();

        m_data = new int8_t[m_capacity * m_typesize];
    }

    explicit vector(const uint64_t elem_counter, const T&& init_value = T()) 
        : m_capacity(elem_counter), m_size(0) {
        vector_log();

        m_data = new int8_t[m_capacity * m_typesize];
        std::fill(static_cast<T>(m_data), static_cast<T>(m_data + m_capacity * m_typesize), init_value);
    }

    explicit vector(const vector<T>& other) {
        vector_log();
        
        m_capacity = other.m_capacity;
        m_size = other.m_size;

        m_data = new int8_t[m_capacity * m_typesize];
    }
    explicit vector(vector<T>&& other);

   public:
    iterator begin();
    iterator end();

   public:
    uint64_t size() const { return m_size; }
    bool empty() const { return m_size == 0; }
    uint64_t capacity() const { return m_capacity; }

   public:
    constexpr void push_back(const T& element);
    constexpr void push_back(T&& element);
    constexpr void pop_back();

   public:
    constexpr iterator erase(const_iterator first, const_iterator last);
    constexpr void clear() noexcept;

   public:
    constexpr void resize(uint64_t size);
    constexpr void resize(uint64_t size, const T& value);

   public:
    constexpr reference operator[](uint64_t position);
    constexpr const_reference operator[](uint64_t position) const;

   private:
    // warning: this function can't be market with 'const' specifier, compilation error will appear
    iterator get_iterator() {
        // TODO:
        // make specific log for this function
        return reinterpret_cast<iterator>(m_data);
    }

    const_iterator get_iterator() const {
        return reinterpret_cast<const_iterator>(m_data);
    }

    void init_memory(iterator values, uint64_t begin_, uint64_t end_, T&& value = T()) {
        for (uint64_t val_idx = begin_; val_idx < end_; ++val_idx) {
            // placement new
            new (values + val_idx) T(value);
        }
    }

    void init_memory(iterator values, uint64_t begin_, uint64_t end_, const_iterator fill_values) {
        for (uint64_t val_idx = begin_; val_idx < end_; ++val_idx) {
            // placement new
            new (values + val_idx) T(fill_values[val_idx]);
        }
    }

    void move_init_memory(iterator values, uint64_t begin_, uint64_t end_, const_iterator move_values) {
        for (uint64_t val_idx = begin_; val_idx < end_; ++val_idx) {
            // placement new and move-semantics
            new (values + val_idx) T(std::move(move_values[val_idx]));
        }
    } 

    void copy_memory(iterator values, uint64_t begin_, uint64_t end_, const_iterator copy_values) {
        for (uint64_t val_idx = begin_; val_idx < end_; ++val_idx) {
            // just assigning the values
            values[val_idx] = copy_values[val_idx];
        }
    }

    void copy_memory(iterator values, uint64_t begin_, uint64_t end_, T&& value = T()) {
        for (uint64_t val_idx = begin_; val_idx < end_; ++val_idx) {
            // just assigning the values
            values[val_idx] = value;
        }
    }

    // WARNING: do not pass copy_values as const_iterator, it will not work
    void move_copy_memory(iterator values, uint64_t begin_, uint64_t end_, iterator copy_values) {
        for (uint64_t val_idx = begin_; val_idx < end_; ++val_idx) {
            // just assigning the values
            values[val_idx] = std::move(copy_values[val_idx]);
        }
    }

    void destroy_memory(iterator values, uint64_t begin_, uint64_t end_) {
        for (uint64_t val_idx = begin_; val_idx < end_; ++val_idx) {
            // call destructor for each element
            values[val_idx].~T();
        }
    }

    void realloc_memory(iterator current_data, uint64_t current_size, uint64_t required) {
        char* reallocated_memory = new char[required * m_typesize]();
        iterator new_data = reinterpret_cast<iterator>(reallocated_memory);

        move_init_memory(new_data, 0, current_size, current_data);
        destroy_memory(current_data, 0, current_size);

        // IMPORTANT: don't forget to DELETE
        delete[] reinterpret_cast<char*>(current_data);
    }


   private:
    uint64_t m_size;
    uint64_t m_capacity;
    uint64_t m_typesize;
    int8_t* m_data;

    private:
    /* CONSTANTS */
    const uint32_t DEFAULT_CAPACITY = 16;
    const double DEFAULT_LOAD_FACTOR = 1.0;
    const double DEFAULT_GROWTH_FACTOR = 1.618; /* golden ratio */
};
}  // namespace X17

#endif  // !X17_VECTOR_HPP