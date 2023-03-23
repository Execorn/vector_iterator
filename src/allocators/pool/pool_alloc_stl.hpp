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
    /* TYPEDEFS */
    using value_type = T;
    /* END OF TYPEDEFS */

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

    inline void construct(value_type* mem_ptr,
                          const value_type& initial_value) {
        new (mem_ptr) value_type(initial_value);
    }

    inline void destroy(value_type* mem_ptr) { mem_ptr->~value_type(); }
};

template <typename T>
class ChunkPolicy {
   public:
    //    typedefs
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

   public:
    //    convert an ChunkPolicy<T> to ChunkPolicy<U>
    template <typename U>
    struct rebind {
        typedef ChunkPolicy<U> other;
    };

   public:
    inline explicit ChunkPolicy() {}
    inline ~ChunkPolicy() {}

    inline explicit ChunkPolicy(ChunkPolicy const&) {}

    template <typename U>
    inline explicit ChunkPolicy(ChunkPolicy<U> const&) {}

    inline pointer allocate(size_type cnt,
                            typename std::allocator<void>::const_pointer = 0) {
        return reinterpret_cast<pointer>(::operator new(cnt * sizeof(T)));
    }

    inline void deallocate(pointer p, size_type) { ::operator delete(p); }

    inline size_type max_size() const {
        return std::numeric_limits<size_type>::max();
    }
};

template <typename T, typename T2>
inline bool operator==(ChunkPolicy<T> const&, ChunkPolicy<T2> const&) {
    return true;
}

template <typename T, typename OtherAllocator>
inline bool operator==(ChunkPolicy<T> const&, OtherAllocator const&) {
    return false;
}

template <typename T,
          typename Policy = ChunkPolicy<T>,
          typename Traits = ObjectTraits<T> >
class PoolAllocator : public Policy, public Traits {
   private:
    typedef Policy AllocationPolicy;
    typedef Traits TTraits;

   public:
    typedef typename AllocationPolicy::size_type size_type;
    typedef typename AllocationPolicy::difference_type difference_type;
    typedef typename AllocationPolicy::pointer pointer;
    typedef typename AllocationPolicy::const_pointer const_pointer;
    typedef typename AllocationPolicy::reference reference;
    typedef typename AllocationPolicy::const_reference const_reference;
    typedef typename AllocationPolicy::value_type value_type;

   public:
    template <typename U>
    struct rebind {
        typedef PoolAllocator<U,
                          typename AllocationPolicy::rebind<U>::other,
                          typename TTraits::rebind<U>::other>
            other;
    };

   public:
    inline explicit PoolAllocator() {}
    inline ~PoolAllocator() {}

    inline PoolAllocator(PoolAllocator const& rhs) : Traits(rhs), Policy(rhs) {}
    
    template <typename U>
    inline PoolAllocator(PoolAllocator<U> const&) {}
    
    template <typename U, typename P, typename T2>
    inline PoolAllocator(PoolAllocator<U, P, T2> const& rhs)
        : Traits(rhs), Policy(rhs) {}
};

template <typename T, typename P, typename Tr>
inline bool operator==(PoolAllocator<T, P, Tr> const& lhs,
                       PoolAllocator<T, P, Tr> const& rhs) {
    return operator==(static_cast<P&>(lhs), static_cast<P&>(rhs));
}

template <typename T,
          typename P,
          typename Tr,
          typename T2,
          typename P2,
          typename Tr2>
inline bool operator==(PoolAllocator<T, P, Tr> const& lhs,
                       PoolAllocator<T2, P2, Tr2> const& rhs) {
    return operator==(static_cast<P&>(lhs), static_cast<P2&>(rhs));
}

template <typename T, typename P, typename Tr, typename OtherAllocator>
inline bool operator==(PoolAllocator<T, P, Tr> const& lhs,
                       OtherAllocator const& rhs) {
    return operator==(static_cast<P&>(lhs), rhs);
}

template <typename T, typename P, typename Tr>
inline bool operator!=(PoolAllocator<T, P, Tr> const& lhs,
                       PoolAllocator<T, P, Tr> const& rhs) {
    return !operator==(lhs, rhs);
}

template <typename T,
          typename P,
          typename Tr,
          typename T2,
          typename P2,
          typename Tr2>
inline bool operator!=(PoolAllocator<T, P, Tr> const& lhs,
                       PoolAllocator<T2, P2, Tr2> const& rhs) {
    return !operator==(lhs, rhs);
}

template <typename T, typename P, typename Tr, typename OtherAllocator>
inline bool operator!=(PoolAllocator<T, P, Tr> const& lhs,
                       OtherAllocator const& rhs) {
    return !operator==(lhs, rhs);
}

}  // namespace X17

#endif  // !X17_POOL_ALLOC_STABLE