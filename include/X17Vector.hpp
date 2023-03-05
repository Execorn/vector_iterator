#ifndef X17_VECTOR_HPP
#define X17_VECTOR_HPP

////////////////////////////////////////////////////////////
/// Headers
////////////////////////////////////////////////////////////
#include <cstdint>
#include <iostream>
#include <cstdio>
#include <algorithm>

////////////////////////////////////////////////////////////

#define X17_VDEBUG

#ifdef X17_VDEBUG
#define vector_log()                                              \
    fprintf(stderr, "vector on addr %p entered (( %s ))\n", this, \
            __PRETTY_FUNCTION__)
#else
#define vector_log()
#endif

////////////////////////////////////////////////////////////

namespace X17 {

////////////////////////////////////////////////////////////////////////
/// CONSTANTS
////////////////////////////////////////////////////////////////////////

static const int8_t* POISON_PTR = reinterpret_cast<const int8_t*>(0xDEADDEAD);
static const uint64_t POISON_UINT = static_cast<uint64_t>(0xDEADBEEF);

template <typename T>
class vector {
   public:
    explicit vector();

    explicit vector(const uint64_t elem_total, T&& init_value = T());

    explicit vector(const vector<T>& other);

    // move constructor
    explicit vector(vector<T>&& other);

    ~vector();

   public:
    T& front();
    const T& front() const;

    T& back();
    const T& back() const;

   public:
    uint64_t size() const { return m_size; }
    bool empty() const { return m_size == 0; }
    uint64_t capacity() const { return m_capacity; }

   public:
    void push_back(const T& value);

    // this push_back implementation allows PERFECT FORWARDING, so std::forward
    // must be used
    void push_back(T&& value);

    // pop_back required correctly implemented ~T() to work
    void pop_back();

   public:
    // TODO: implement erase function (to do it, implement iterator first)
    T* erase(const T* first);

    T* erase(const T* first, const T* last);

    void clear() noexcept;

   public:
    void resize(uint64_t size);

    void reserve(uint64_t size, const T& value);

   public:
    /// WARNING!!!
    ///
    /// [] operators DO NOT provide noexcept behaviour
    ///
    /// WARNING!!!
    T& operator[](uint64_t position);
    const T& operator[](uint64_t position) const;

   public:
    vector& operator=(const vector& other) noexcept;
    // this operator= achieves PERFECT FORWARDING, so std::forward is used
    vector& operator=(vector&& other) noexcept;

   private:
    // warning: this function can't be marked with 'const' specifier,
    // compilation error will appear
    T* __data_ptr();

    const T* __data_ptr() const;

    void __obj_init(T* values, uint64_t begin_, uint64_t end_, T&& value = T());

    void __obj_init(T* values,
                    uint64_t begin_,
                    uint64_t end_,
                    const T* init_list);

    void __mv_obj_init(T* values,
                       uint64_t begin_,
                       uint64_t end_,
                       const T* move_values);

    void __copy_obj(T* values,
                    uint64_t begin_,
                    uint64_t end_,
                    const T* copy_values);

    void __copy_obj(T* values, uint64_t begin_, uint64_t end_, T&& value = T());

    // WARNING: do not pass copy_values as const T*, it will not work
    void __mv_copy_obj(T* values,
                       uint64_t begin_,
                       uint64_t end_,
                       T* copy_values);

    void __del_obj(T* values, uint64_t begin_, uint64_t end_);

    int8_t* __realloc_mem(T* current_data,
                          uint64_t current_size,
                          uint64_t required,
                          const T& value = T());

   private:
    uint64_t m_size;
    uint64_t m_capacity;
    uint64_t m_typesize;
    int8_t* m_data;

   private:
    /* CONSTANTS */
    static const uint32_t DEFAULT_CAPACITY = 16;

    // TODO: make use of load factor (unused rn)
    constexpr static double DEFAULT_LOAD_FACTOR = 1.0;
    constexpr static double DEFAULT_GROWTH_FACTOR = 1.618; /* golden ratio */
};

template <>
class vector<bool> {
    // nested reference class to access separate bits
    class reference {
       public:
        reference(uint64_t* segment, uint8_t bitidx)
            : m_segment(segment), m_shift(bitidx) {
            vector_log();
        }

        ~reference() {}

       public:
        reference& operator=(bool x) noexcept {
            if (!x) {
                *m_segment &= ~(1 << m_shift);
            } else {
                *m_segment |= (1 << m_shift);
            }

            return *this;
        }

        reference& operator=(const reference& x) noexcept {
            m_segment = x.m_segment;
            m_shift = x.m_shift;

            return *this;
        }

       public:
        operator bool() const noexcept {
            // TODO: check is there is a better way to do this
            return (*m_segment & (1 << m_shift)) >> m_shift;
        }

        void flip() noexcept { operator=(!operator bool()); }

       private:
        uint64_t* m_segment;
        uint8_t m_shift;  // bit idx
    };

   public:
    explicit vector()
        : m_capacity(DEFAULT_CAPACITY), m_size(0), m_data(nullptr) {
        vector_log();
    }

    explicit vector(const uint64_t elem_total, bool value)
        : m_size(elem_total), m_capacity(m_size * DEFAULT_GROWTH_FACTOR) {
        vector_log();

        m_data = new uint64_t[__uints_cap(m_capacity)]();
        for (size_t uint_idx = 0; uint_idx < __uints_cap(m_size); ++uint_idx) {
            m_data[uint_idx] = value ? UINT_FAST64_MAX : 0;
        }
    }

    explicit vector(const vector<bool>& other)
        : m_size(other.m_size), m_capacity(other.m_capacity) {
        vector_log();

        m_data = new uint64_t[__uints_cap(m_capacity)]();

        // TODO: make it faster (maybe intel intrinsics??)
        for (size_t uint_idx = 0; uint_idx < __uints_cap(m_size); ++uint_idx) {
            m_data[uint_idx] = other.m_data[uint_idx];
        }
    }
    // move constructor
    explicit vector(vector<bool>&& other) {
        vector_log();

        m_size = other.m_size;
        m_capacity = other.m_capacity;
        m_data = nullptr;

        std::swap(m_data, other.m_data);
    }

    ~vector() {
        vector_log();

        if (m_data != nullptr) {
            delete[] m_data;
        }

        // TODO: assign POISON_PTR to m_data
        m_data = nullptr;
        m_capacity = POISON_UINT;
        m_size = POISON_UINT;
        m_typesize = 0;
    }

   public:
    uint64_t size() const { return m_size; }
    bool empty() const { return m_size == 0; }
    uint64_t capacity() const { return m_capacity; }

   public:
    void push_back(const bool value) {
        vector_log();

        resize(m_capacity ? m_size + 1 : 0);
        operator[](m_size++) = value;
    }

    void pop_back() {
        if (!m_size) {
            throw std::range_error("vector underflow");
        }

        // notice how we don't even need to actually pop anything
        // it's bit! decreasing size will be enough
        --m_size;
    }

    void clear() noexcept {
        vector_log();

        for (size_t uint_idx = 0; uint_idx < __uints_cap(m_size); ++uint_idx) {
            m_data[uint_idx] = 0;
        }

        m_size = 0;
    }

   public:
    // TODO: change resize() and reserve(), now they're swapped
    void resize(uint64_t request) {
        vector_log();

        // have to use __uints_cap() here since normal checks
        // can exceed memory usage
        if (__uints_cap(request) <= __uints_cap(m_capacity)) {
            return;
        }

        request = std::max(
            request, static_cast<uint64_t>(m_capacity * DEFAULT_GROWTH_FACTOR));
        uint64_t* new_data = new uint64_t[__uints_cap(request)]();
        // reassign everything
        for (uint64_t uint_idx = 0; uint_idx < __uints_cap(m_size);
             ++uint_idx) {
            new_data[uint_idx] = m_data[uint_idx];
        }

        // WARNING: don't forget to delete this to avoid memory leaks
        delete[] m_data;

        m_data = new_data;
        m_capacity = request;
    }

    void reserve(uint64_t size, bool value) {
        vector_log();

        resize(size);
        for (uint64_t bit_idx = m_size; bit_idx < size; ++bit_idx) {
            // TODO: think if I can rewrite next line without using operator[]
            // syntax like that
            operator[](bit_idx) = value;
        }

        // change current size
        m_size = size;
    }

   public:
    vector<bool>& operator=(const vector<bool>& other) noexcept {
        vector_log();

        resize(other.size());
        for (size_t uint_idx = 0; uint_idx < __uints_cap(other.size());
             ++uint_idx) {
            m_data[uint_idx] = other[uint_idx];
        }
        m_size = other.m_size;

        // return this vector
        return *this;
    }
    // this operator= achieves PERFECT FORWARDING, so std::forward is used
    vector<bool>& operator=(vector<bool>&& other) noexcept {
        vector_log();
        // WARNING: don't forget to destroy this first
        // to avoid memory leaks
        vector<bool>::~vector();

        vector<bool>(std::forward<vector<bool>>(other));
        return *this;
    }

   public:
    reference operator[](const uint64_t index) noexcept {
        return reference(m_data + __uints_cap(index),
                         static_cast<uint8_t>(index % (sizeof(uint64_t) * 8)));
    }

    const reference operator[](const uint64_t index) const noexcept {
        return reference(m_data + __uints_cap(index),
                         static_cast<uint8_t>(index % (sizeof(uint64_t) * 8)));
    }

   public:
    reference front() noexcept {
        if (m_size == 0) {
            return {m_data, 0};
        }

        return reference(m_data, 0);
    }

    const reference front() const noexcept {
        if (m_size == 0) {
            return {m_data, 0};
        }

        return reference(m_data, 0);
    }

    reference back() noexcept {
        if (m_size == 0) {
            return {m_data, 0};
        }

        return reference(
            m_data + __uints_cap(m_size - 1),
            static_cast<uint8_t>((m_size - 1) % (sizeof(uint64_t) * 8)));
    }

    const reference back() const noexcept {
        if (m_size == 0) {
            return {m_data, 0};
        }

        return reference(
            m_data + __uints_cap(m_size - 1),
            static_cast<uint8_t>((m_size - 1) % (sizeof(uint64_t) * 8)));
    }

   private:
    uint64_t m_size;
    uint64_t m_capacity;
    uint64_t m_typesize;

    // IMPORTANT: use unsigned int to avoid extra job with first bit
    uint64_t* m_data;

   private:
    uint64_t __seg_ptr(uint64_t index) const {
        return index / sizeof(uint64_t) * 8;
    }

    uint64_t __uints_cap(uint64_t requested) const {
        return requested ? (requested / (sizeof(uint64_t) * 8)) : 0;
    }

   private:
    /* CONSTANTS */
    static const uint32_t DEFAULT_CAPACITY = 16;

    // TODO: make use of load factor (unused rn)
    constexpr static double DEFAULT_LOAD_FACTOR = 1.0;
    // growth factor for bool vector is NOT the same as for vector<T>
    constexpr static double DEFAULT_GROWTH_FACTOR = 2.0;
};

////////////////////////////////////////////////////////////////////////
/// TEMPLATE FUNCTIONS DEFINITIONS
////////////////////////////////////////////////////////////////////////

template <typename T>
vector<T>::vector()
    : m_capacity(DEFAULT_CAPACITY), m_size(0), m_typesize(sizeof(T)) {
    vector_log();

    m_data = new int8_t[m_capacity * m_typesize];
}

template <typename T>
vector<T>::vector(const uint64_t elem_total, T&& init_value)
    : m_capacity(elem_total), m_size(0), m_typesize(sizeof(T)) {
    vector_log();

    m_data = new int8_t[m_capacity * m_typesize];
    __obj_init(__data_ptr(), 0, elem_total, std::move(init_value));
}

template <typename T>
vector<T>::vector(const vector<T>& other)
    : m_capacity(other.m_capacity),
      m_size(other.m_size),
      m_typesize(other.m_typesize) {
    vector_log();

    m_data = new int8_t[m_capacity * m_typesize];
    // other.__data_ptr() returns const T*
    __obj_init(__data_ptr(), 0, m_size, other.__data_ptr());
}

template <typename T>
vector<T>::vector(vector<T>&& other)
    : m_capacity(other.m_capacity),
      m_size(other.m_size),
      m_data(nullptr),
      m_typesize(other.m_typesize) {
    vector_log();

    // ONLY swapping pointers, stealing it, no copying!
    std::swap(this->m_data, other.m_data);
}

template <typename T>
vector<T>::~vector() {
    vector_log();

    if (m_data != nullptr) {
        __del_obj(reinterpret_cast<T*>(m_data), 0, m_size);

        // WARNING: don't forget to delete[] and avoid memory leaks
        delete[] m_data;
    }

    // fill everything with poison!
    m_data = (int8_t*)POISON_PTR;
    m_size = m_capacity = POISON_UINT;
}

template <typename T>
void vector<T>::clear() noexcept {
    vector_log();

    __del_obj(__data_ptr(), 0, m_size);
    m_size = 0;
}

template <typename T>
T& vector<T>::front() {
    vector_log();

    return __data_ptr()[m_size - 1];
}

template <typename T>
const T& vector<T>::front() const {
    // TODO: make specific log for this and back() functions
    vector_log();

    return __data_ptr()[m_size - 1];
}

template <typename T>
T& vector<T>::back() {
    vector_log();

    return __data_ptr()[0];
}

template <typename T>
const T& vector<T>::back() const {
    vector_log();

    return __data_ptr()[0];
}

template <typename T>
void vector<T>::push_back(const T& value) {
    vector_log();

    resize((m_capacity == 0 ? DEFAULT_CAPACITY : m_size + 1));
    __data_ptr()[m_size++] = value;
}

template <typename T>
void vector<T>::push_back(T&& value) {
    vector_log();

    resize((m_capacity == 0 ? DEFAULT_CAPACITY : m_size + 1));
    __data_ptr()[m_size++] = std::forward<T>(value);
}

template <typename T>
void vector<T>::pop_back() {
    vector_log();

    if (m_size == 0) {
        throw std::range_error("vector underflow");
    }

    __data_ptr()[--m_size].~T();
}

template <typename T>
void vector<T>::resize(uint64_t size) {
    vector_log();

    if (size < m_size) {
        // ignore, more than enough space already
        return;
    }

    uint64_t new_size =
        std::max(size, static_cast<uint64_t>(m_size * DEFAULT_GROWTH_FACTOR));

    m_data = __realloc_mem(__data_ptr(), m_size, new_size);
    m_capacity = new_size;
}

template <typename T>
void vector<T>::reserve(uint64_t size, const T& value) {
    vector_log();

    if (size < m_size) {
        // ignore, more than enough space already
        return;
    }

    uint64_t new_size =
        std::max(size, static_cast<uint64_t>(m_size * DEFAULT_GROWTH_FACTOR));

    m_data = __realloc_mem(__data_ptr(), m_size, new_size, value);
    // TODO: check is this even works
    __obj_init(__data_ptr(), m_size, m_capacity, value);
    m_capacity = new_size;
}

template <typename T>
T& vector<T>::operator[](uint64_t position) {
    return __data_ptr()[position];
}

template <typename T>
const T& vector<T>::operator[](uint64_t position) const {
    return __data_ptr()[position];
}

template <typename T>
vector<T>& vector<T>::operator=(const vector& other) noexcept {
    // TODO: decide, should vector_log() be here
    vector_log();

    if (other.m_size < m_size) {
        __copy_obj(__data_ptr(), 0, other.m_size);
        __del_obj(__data_ptr(), other.m_size, m_size);

        m_size = other.m_size;
        return *this;
    }

    resize(other.m_size);
    __copy_obj(__data_ptr(), 0, m_size, other.__data_ptr());

    // WARNING: don't forget to init memory here
    __obj_init(__data_ptr(), m_size, other.m_size, other.__data_ptr());
    return *this;
}

template <typename T>
vector<T>& vector<T>::operator=(vector&& other) noexcept {
    // remove everything that we have now
    ~vector();
    vector(std::forward<vector>(other));
}

template <typename T>
T* vector<T>::__data_ptr() {
    // TODO: make specific log for this function
    return reinterpret_cast<T*>(m_data);
}

template <typename T>
const T* vector<T>::__data_ptr() const {
    return reinterpret_cast<const T*>(m_data);
}

////////////////////////////////////////////////////////////////
/// VECTOR SERVICE FUNCTION (NEVER USE THEM DIRECTLY)
////////////////////////////////////////////////////////////////

template <typename T>
void vector<T>::__obj_init(T* values,
                           uint64_t begin_,
                           uint64_t end_,
                           T&& value) {
    for (uint64_t val_idx = begin_; val_idx < end_; ++val_idx) {
        // placement new
        new (values + val_idx) T(value);
    }
}

// IMPORTANT: function will cause segfault if size of init_list < size of values
template <typename T>
void vector<T>::__obj_init(T* values,
                           uint64_t begin_,
                           uint64_t end_,
                           const T* init_list) {
    for (uint64_t val_idx = begin_; val_idx < end_; ++val_idx) {
        // placement new
        new (values + val_idx) T(init_list[val_idx]);
    }
}

template <typename T>
void vector<T>::__mv_obj_init(T* values,
                              uint64_t begin_,
                              uint64_t end_,
                              const T* move_values) {
    for (uint64_t val_idx = begin_; val_idx < end_; ++val_idx) {
        // placement new and move-semantics
        new (values + val_idx) T(std::move(move_values[val_idx]));
    }
}

template <typename T>
void vector<T>::__copy_obj(T* values,
                           uint64_t begin_,
                           uint64_t end_,
                           const T* copy_values) {
    for (uint64_t val_idx = begin_; val_idx < end_; ++val_idx) {
        // just assigning the values
        values[val_idx] = copy_values[val_idx];
    }
}

template <typename T>
void vector<T>::__copy_obj(T* values,
                           uint64_t begin_,
                           uint64_t end_,
                           T&& value) {
    for (uint64_t val_idx = begin_; val_idx < end_; ++val_idx) {
        // just assigning the values
        values[val_idx] = value;
    }
}

// WARNING: do not pass copy_values as const T*, it will not work
template <typename T>
void vector<T>::__mv_copy_obj(T* values,
                              uint64_t begin_,
                              uint64_t end_,
                              T* copy_values) {
    for (uint64_t val_idx = begin_; val_idx < end_; ++val_idx) {
        // just assigning the values
        values[val_idx] = std::move(copy_values[val_idx]);
    }
}

template <typename T>
void vector<T>::__del_obj(T* values, uint64_t begin_, uint64_t end_) {
    for (uint64_t val_idx = begin_; val_idx < end_; ++val_idx) {
        // call destructor for each element
        values[val_idx].~T();
    }
}

template <typename T>
int8_t* vector<T>::__realloc_mem(T* current_data,
                                 uint64_t current_size,
                                 uint64_t required,
                                 const T& value) {
    int8_t* reallocated_memory = new int8_t[required * m_typesize]();
    T* new_data = reinterpret_cast<T*>(reallocated_memory);

    __mv_obj_init(new_data, 0, current_size, current_data);
    __del_obj(current_data, 0, current_size);

    // IMPORTANT: don't forget to DELETE
    delete[] reinterpret_cast<int8_t*>(current_data);

    return reallocated_memory;
}

////////////////////////////////////////////////////////////////
/// BOOL VECTOR FUNCTIONS
////////////////////////////////////////////////////////////////

}  // namespace X17

#endif  // !X17_VECTOR_HPP