#ifndef X17_POOL_ALLOC_STABLE
#define X17_POOL_ALLOC_STABLE

////////////////////////////////////////////////////////////
/// Headers
////////////////////////////////////////////////////////////
#include <iostream>
#include <cstddef>
#include <list>
#include <chrono>
#include <string>

namespace X17 {

template <typename T>
class ObjectTraits {
    using value_type = T;

   public:
    template <typename U>
    struct rebind {
        typedef ObjectTraits<U> other;
    };

   public:
    inline explicit ObjectTrait() {}
    inline ~ObjectTraits() {}

    template <typename U>
    inline explicit ObjectTraits(ObjectTraits<U> const&) {}

    inline value_type* address(value_type& ref) { return &ref; }

    inline value_type const* address(value_type const& ref) { return &ref; }

    inline void construct(value_type* mem_ptr, const value_type& initial_value) { new (mem_ptr) value_type(initial_value); }

    inline void destroy(value_type* mem_ptr) { mem_ptr->~value_type(); }
};

template <typename T>
class PoolAllocator {
    /* TYPEDEFS */
    using value_type = T;

    using pointer = value_type*;
    using const_pointer = const value_type*;

    using reference = value_type&;
    using const_reference = const value_type&;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    /* END OF TYPEDEFS */

   public:
    /* Struct to rebind allocator<T> to allocator<U> */
    template <typename U>
    struct rebind {
        typedef PoolAllocator<U> other;
    };

   public:
    /* use compiler-generator default contructor and destructor */
    inline explicit PoolAllocator() = default;
    inline explicit ~PoolAllocator() = default;

    inline explicit PoolAllocator(PoolAllocator const&) {}
    template <typename U>
    inline explicit PoolAllocator(PoolAllocator<U> const&) {}

    inline pointer address(reference ref) { return &ref; }

    inline const_pointer address(const_reference ref) { return &ref; }

    inline pointer allocate(size_type n_bytes,
                            typename std::allocator<void>::const_pointer = 0) {
        // TODO: implement my own PoolAllocator here
        return reinterpret_cast<const_pointer>(::operator new(cnt * sizeof(T)));
    }

    inline void deallocate(pointer mem_to_free, size_type n_bytes) {
        ::operator delete(mem_to_free);
    }

    /* max size possible to allocate */
    inline size_type max_size() const {
        return std::numeric_limits<size_type>::max() / sizeof(T);
    }

    /* construction / destruction */
    inline void construct(pointer mem_ptr, const value_type& initial_value) {
        new (mem_ptr) T(initial_value);
    }

    inline void destroy(pointer mem_ptr) { mem_ptr->~value_type(); }

    inline bool operator==(Allocator const&) { return true; }

    inline bool operator!=(Allocator const& a) { return !operator==(a); }
};

/*
explicit PoolAllocator(const size_t chunks_per_block) :
        m_chunks_per_block(chunks_per_block) {}

    void* allocate(const size_t n_bytes);
    void deallocate(void* mem_to_free, size_t n_bytes);

    struct Chunk {
        Chunk* m_next;
    }

    private:
    size_t m_chunks_per_block;

    Chunk* m_alloc = nullptr;
    Chunk* m_alloc_block();
*/

}  // namespace X17

#endif  // !X17_POOL_ALLOC_STABLE