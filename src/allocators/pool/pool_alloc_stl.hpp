#ifndef X17_POOL_ALLOC_STABLE
#define X17_POOL_ALLOC_STABLE

////////////////////////////////////////////////////////////
/// Headers
////////////////////////////////////////////////////////////
#include <iostream>
#include <cstddef>
#include <list>
#include <string>
#include <chrono>
#include <string>
#include <cstdint>
#include <cstdio>
#include <filesystem>

#define DEBUG
namespace X17 {
template <typename T, std::size_t chunksPerBlock = 512>
class MemoryPool {
    /* TYPEDEFS */
    typedef T* pointer;
    typedef const T* const const_pointer;
    /* END OF TYPEDEFS*/

   public:
    struct Chunk {
        Chunk* m_next;
    };

    class ObjectPool {
       public:
        ObjectPool(ObjectPool* const next_pool) : m_next(next_pool) {
#ifdef DEBUG
            fprintf(m_log_stream,
                    "\nObjectPool created with argument next_pool='%p'.");
#endif  // DEBUG
        }

       public:
        // cannot modify m_next!! (to avoid collisions)
        ObjectPool* const m_next;

       private:
        // m_bytes_per_block is min(sizeof(T), sizeof(Chunk))
        static constexpr std::size_t m_bytes_per_block = sizeof(Chunk) >
                                                                 sizeof(T)
                                                             ? sizeof(Chunk)
                                                             : sizeof(T);
        uint8_t m_buffer[chunksPerBlock * m_bytes_per_block];

        pointer recieve_chunk(const std::size_t chunk_index) const {
#ifdef DEBUG
            fprintf(m_log_stream,
                    "\nAccess in ObjectPool chunk with index '%zu'. Chunk "
                    "addr:'%p'.",
                    chunk_index, m_buffer + chunk_index * m_bytes_per_block);
#endif  // DEBUG

            return reinterpret_cast<pointer>(m_buffer +
                                             chunk_index * m_bytes_per_block);
        }
    };

   public:
    pointer allocate() {
#ifdef DEBUG
        fprintf(m_log_stream,
                "\nallocate() called in MemoryPool with addr:'%p'.", this);
#endif  // DEBUG

        if (m_free_blocks_head != nullptr) {
            Chunk* current_chunk = m_free_blocks_head;
            m_free_blocks_head = current_chunk->m_next;

            // just return pointer to first free block
            return reinterpret_cast<pointer>(current_chunk);
        }

        if (m_blocks_in_pool >= chunksPerBlock) {
            m_head_pool = new ObjectPool(m_head_pool);

            // no blocks in new pool yet
            m_blocks_in_pool = 0;
        }

        // get one block and increase pointer
        return m_head_pool->recieve_chunk(m_blocks_in_pool++);
    }

    void deallocate(pointer mem_to_dealloc) {
#ifdef DEBUG
        fprintf(m_log_stream,
                "\ndeallocate() with mem_to_dealloc='%p' called in MemoryPool "
                "with addr:'%p'.",
                mem_to_dealloc, this);
#endif  // DEBUG

        Chunk* current_chunk = reinterpret_cast<Chunk*>(mem_to_dealloc);
        current_chunk->m_next = m_free_blocks_head;

        // add freed chunk to list's head
        m_free_blocks_head = current_chunk;
    }

    MemoryPool()
        : m_blocks_in_pool(chunksPerBlock),
          m_free_blocks_head(nullptr),
          m_head_pool(nullptr) {
#ifdef DEBUG
        namespace fs = std::filesystem;
        // TODO: ask ded if check is needed
        if (!fs::is_directory("alloc_logs") || !fs::exists("alloc_logs")) {
            fs::create_directories("alloc_logs");
        }

        m_log_stream = fopen(m_log_filename.c_str(), "a+");
        if (m_log_stream == nullptr) {
            throw runtime_error("can't open/create log file");
        }

        fprintf(m_log_stream,
                "\nMemoryPool created. Addr:'%p'. Chunks per block:'%zu'.",
                this, m_blocks_in_pool);
#endif  // DEBUG
    }

    ~MemoryPool() {
        while (m_head_pool != nullptr) {
            ObjectPool* current_object_pool = m_head_pool;
            m_head_pool = current_object_pool->m_next;

            // freeing current object pool
            delete current_object_pool;
        }

#ifdef DEBUG
        fclose(m_log_stream);
#endif  // DEBUG
    }

    /* MOVE-SEMANTICS ARE COMPLETELY PROHIBITED FOR MemoryPool */
    MemoryPool(const MemoryPool& other) = delete;
    MemoryPool(MemoryPool&& other) = delete;

    /* ASSIGNMENT OPERATORS ARE COMPLETELY PROHIBITED FOR MemoryPool */
    MemoryPool operator=(MemoryPool&& other) = delete;
    MemoryPool operator=(const MemoryPool& MemoryPool) = delete;

   private:
    Chunk* m_free_blocks_head{nullptr};
    ObjectPool* m_head_pool{nullptr};

    std::size_t m_blocks_in_pool{chunksPerBlock};

#ifdef DEBUG
    std::string m_log_filename{"alloc_logs/mempool.log"};
    FILE* m_log_stream{nullptr};
#endif  // DEBUG
};

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
class PoolAllocPolicy {
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
    //    convert an PoolAllocPolicy<T> to PoolAllocPolicy<U>
    template <typename U>
    struct rebind {
        typedef PoolAllocPolicy<U> other;
    };

   public:
    inline explicit PoolAllocPolicy() {}
    inline ~PoolAllocPolicy() {}

    inline explicit PoolAllocPolicy(PoolAllocPolicy const&) {}

    template <typename U>
    inline explicit PoolAllocPolicy(PoolAllocPolicy<U> const&) {}

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
inline bool operator==(PoolAllocPolicy<T> const&, PoolAllocPolicy<T2> const&) {
    return true;
}

template <typename T, typename OtherAllocator>
inline bool operator==(PoolAllocPolicy<T> const&, OtherAllocator const&) {
    return false;
}

template <typename T,
          typename Policy = PoolAllocPolicy<T>,
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