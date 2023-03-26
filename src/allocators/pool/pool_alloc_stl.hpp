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
#include <memory>

#define RELEASE
#define DEFAULT_CHUNKS_PER_BLOCK 4 * 1024

#ifndef RELEASE
#define DEBUG
#endif
namespace X17 {

#ifdef DEBUG
static const std::string m_log_filename = "alloc_logs/mempool.log";
static FILE* m_log_stream = nullptr;
#endif  // DEBUG

template <typename T, std::size_t chunksPerBlock = DEFAULT_CHUNKS_PER_BLOCK>
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

        // m_bytes_per_block is min(sizeof(T), sizeof(Chunk))
        static const std::size_t m_bytes_per_block = sizeof(Chunk) > sizeof(T)
                                                         ? sizeof(Chunk)
                                                         : sizeof(T);
        uint8_t m_buffer[chunksPerBlock * m_bytes_per_block];

        // TODO: ask how to mark this func as const and not get 'cast away
        // qualifiers' error
        pointer recieve_chunk(const std::size_t chunk_index) {
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
            if (!fs::create_directory("alloc_logs")) {
                throw std::runtime_error(
                    "log directory doesn't exist, creation failed");
            }
        }

        m_log_stream = fopen(m_log_filename.c_str(), "a+");
        if (m_log_stream == nullptr) {
            throw std::runtime_error("can't open/create log file");
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
};

template <typename T, std::size_t chunksPerBlock = DEFAULT_CHUNKS_PER_BLOCK>
class PoolAllocator : private MemoryPool<T, chunksPerBlock> {
   public:
    /* TYPEDEFS */
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    /* END OF TYPEDEFS */

   public:
    template <typename U>
    struct rebind {
        typedef PoolAllocator<U, chunksPerBlock> other;
    };

   public:
    inline explicit PoolAllocator() = default;

    inline ~PoolAllocator() {
        if (m_rebind_allocator != nullptr) {
            // free previously allocated memory
            delete m_rebind_allocator;
        }
    }

    inline explicit PoolAllocator(PoolAllocator const& other)
        : m_cp_allocator(other) {}

    template <typename U>
    inline explicit PoolAllocator(
        PoolAllocator<U, chunksPerBlock> const& other) {
        if (!std::is_same<T, U>::value == false) {
            m_rebind_allocator = new std::allocator<T>();
        }
    }

    //    address
    inline pointer address(reference r) { return &r; }
    inline const_pointer address(const_reference r) { return &r; }

    //    memory allocation
    inline pointer allocate(
        size_type cnt,
        typename std::allocator<void>::const_pointer mem_ptr = 0) {
        if (m_cp_allocator != nullptr) {
            return m_cp_allocator->allocate(cnt, mem_ptr);
        }

        if (m_rebind_allocator != nullptr) {
            return m_rebind_allocator->allocate(cnt, mem_ptr);
        }

        // main body of function that is called if there are no rebind and copy
        // allocators

        if (cnt != 1 || mem_ptr != nullptr) {
            throw std::bad_alloc();
        } else {
            return MemoryPool<T, chunksPerBlock>::allocate();
        }
    }

    inline void deallocate(pointer ptr, size_type cnt) {
        if (m_cp_allocator != nullptr) {
            m_cp_allocator->deallocate(ptr, cnt);
            return;
        }

        if (m_rebind_allocator != nullptr) {
            m_rebind_allocator->deallocate(ptr, cnt);
            return;
        }

        // main body of function that is called if there are no rebind and copy
        // allocators

        MemoryPool<T, chunksPerBlock>::deallocate(ptr);
    }

    inline size_type max_size() const {
        return std::numeric_limits<size_type>::max() / sizeof(T);
    }

    //    construction/destruction
    inline void construct(pointer p, const T& t) { new (p) T(t); }
    inline void destroy(pointer p) { p->~T(); }

    inline bool operator==(PoolAllocator const&) { return true; }
    inline bool operator!=(PoolAllocator const& a) { return !operator==(a); }

   private:
    PoolAllocator* m_cp_allocator;
    std::allocator<T>* m_rebind_allocator;
};  //    end of class PoolAllocator

template <typename T, typename T2>
inline bool operator==(PoolAllocator<T> const&, PoolAllocator<T2> const&) {
    return true;
}

template <typename T, typename OtherAllocator>
inline bool operator==(PoolAllocator<T> const&, OtherAllocator const&) {
    return false;
}

}  // namespace X17

#endif  // !X17_POOL_ALLOC_STABLE