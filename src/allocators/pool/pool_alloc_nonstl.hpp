#ifndef X17_POOL_ALLOC_NONSTL_STABLE
#define X17_POOL_ALLOC_NONSTL_STABLE

////////////////////////////////////////////////////////////
/// Headers
////////////////////////////////////////////////////////////
#include <cstdint>
#include <iostream>
#include <new>
#include <exception>

#define POOL_NOEXCEPT 1

namespace X17 {

struct Chunk {
    Chunk* m_next;
};

class PoolAllocator {
   public:
    PoolAllocator(const size_t chunks_per_block)
        : m_chunks_per_block(chunks_per_block) {}

    void*  allocate(const size_t n_bytes);
    void deallocate(void* mem_ptr, const size_t n_bytes);

   private:
    size_t m_chunks_per_block;

    Chunk* m_alloc = nullptr;
    Chunk* m_alloc_block(const size_t n_bytes);
};

struct Object {
    // exactly 16 bytes for object data
    uint64_t m_data[2];

    static PoolAllocator m_allocator;

    static void* operator new(const size_t n_bytes) {
        return m_allocator.allocate(n_bytes);
    }

    static void operator delete(void* mem_ptr, const size_t n_bytes) {
        return m_allocator.deallocate(mem_ptr, n_bytes);
    }
};

// allocator instance
PoolAllocator Object::m_allocator{8};

void* PoolAllocator::allocate(const size_t n_bytes) {
    if (n_bytes == 0) {
        return nullptr;
    }

    if (m_alloc == nullptr) {
        m_alloc = m_alloc_block(n_bytes);
    }

    Chunk* allocated_chunk = m_alloc;
    m_alloc = m_alloc->m_next;

    return allocated_chunk;
}

void PoolAllocator::deallocate(void* mem_ptr, const size_t n_bytes) {
    if (mem_ptr == nullptr || n_bytes == 0) {
        return;
    }

    reinterpret_cast<Chunk*>(mem_ptr)->m_next = m_alloc;
    m_alloc = reinterpret_cast<Chunk*>(mem_ptr);

    return;
}

Chunk* PoolAllocator::m_alloc_block(const size_t chunk_size) {
    size_t block_size = m_chunks_per_block * chunk_size;

    // in case malloc(block_size) returns NULL, reinterpreter_cast will cast it to nullptr anyway, so we don't need to do external check here
    Chunk* first_chunk_of_new_block = reinterpret_cast<Chunk*>(malloc(block_size));
    if (first_chunk_of_new_block == nullptr) {
        #ifndef POOL_NOEXCEPT
        throw std::bad_alloc();
        #endif // POOL_NOEXCEPT

        return nullptr;
    }

    Chunk* current_chunk = first_chunk_of_new_block;

    for (size_t chunk_index = 0; chunk_index < m_chunks_per_block - 1; ++chunk_index) {
        // shift m_chunk by chunk_size bytes
        current_chunk->m_next = reinterpret_cast<Chunk*>(reinterpret_cast<uint8_t>(current_chunk) + chunk_size);
        current_chunk = current_chunk->m_next;
    }

    // last chunk in chain always should point to nullptr
    current_chunk->m_next = nullptr;

    return current_chunk;
}

};  // namespace X17

#endif  // !X17_POOL_ALLOC_NONSTL_STABLE