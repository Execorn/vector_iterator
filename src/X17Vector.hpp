#ifndef X17_VECTOR_HPP
#define X17_VECTOR_HPP

#include <cstdint>

namespace X17 {

template <typename T>
class vector {
   public:
    typedef T* iterator;
    typedef const T* const const_iterator;

   public:
    explicit vector();
    explicit vector(const uint64_t elem_counter, const T& init_value = T());
    explicit vector(const vector<T>& other);
    explicit vector(vector<T>&& other);

   public:
    iterator begin();
    iterator end();

   public:
    uint64_t size() const;
    bool empty() const;
    uint64_t capacity() const;

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
    constexpr T& operator[](uint64_t position);
    constexpr const T& operator[](uint64_t position);
    
    private:
    uint64_t m_size;
    uint64_t m_capacity;
    
    const double DEFAULT_LOAD_FACTOR = 1.0;
    const double DEFAULT_GROWTH_FACTOR = 1.618; /* golden ratio */

    T* m_data;
};
}  // namespace X17

#endif  // !X17_VECTOR_HPP