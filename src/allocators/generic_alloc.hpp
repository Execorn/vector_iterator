#ifndef X17_GENERIC_ALLOC
#define X17_GENERIC_ALLOC

////////////////////////////////////////////////////////////
/// Headers
////////////////////////////////////////////////////////////
#include <unistd.h>
#include <cstdint>
#include <utility>
#include <new>
#include <stdexcept>
#include <list>

namespace X17 {

using data_t = intptr_t;

/* allocates N bytes (N >= n_bytes) */
data_t* allocate(const size_t n_bytes) {
    if (n_bytes == 0) {
        return nullptr;
    }

    size_t n_aligned_bytes = alignBytes(n_bytes);

    if (Chunk* reused_chunk = getFreeChunk(n_aligned_bytes)) {
        return reused_chunk->m_data;
    }

    Chunk* new_chunk = mapOSmemory(n_bytes);
    new_chunk->m_used = true;
    new_chunk->m_prev = nullptr;
    new_chunk->m_next = nullptr;
    new_chunk->m_size = n_aligned_bytes;

    if (m_heap_head == nullptr) {
        m_heap_head = new_chunk;
    }

    if (m_heap_tail != nullptr) {
        m_heap_tail->m_next = new_chunk;
        new_chunk->m_prev = m_heap_tail;
    }

    m_heap_tail = new_chunk;

    return new_chunk->m_data;
}

void deallocate(const data_t* data_ptr) {
    if (data_ptr == nullptr) {
        // TODO: should I 'throw std::invalid_argument' here?
        return;
    }

    Chunk* user_chunk = shiftToHeader(data_ptr);

    if (isCoalesceableNext(user_chunk)) {
        user_chunk = coalesceChunk(user_chunk);
    } else if (isCoalesceablePrev(user_chunk)) {
        user_chunk = coalesceChunk(user_chunk->m_prev);
    }

    user_chunk->m_used = false;
}

const size_t SPLIT_RATE_MIN_BYTES = 16;

enum class MemoryManagement {
    first_fit_search,
    // TODO: implement functions for best fit and free_list
    next_fit_search,
    best_fit_search,
    free_list_search,
    explicit_free_list,
};

struct Chunk {
    size_t m_size;

    bool m_used;

    Chunk* m_prev;
    Chunk* m_next;

    data_t m_data[1];
};

static Chunk* m_heap_head;
static Chunk* m_heap_tail;

// for the next_fit_search mem-management
static Chunk* m_last_found;

static std::list<Chunk*> m_free_list;

static MemoryManagement m_mem_mode;  // = MemoryManagement::first_fit_search;

void resetProgramHeap() {
    if (m_heap_head == nullptr) {
        return;
    }

    brk(m_heap_head);

    m_heap_head = nullptr;
    m_heap_tail = nullptr;
    m_last_found = nullptr;
}

void configure(MemoryManagement search_mode) {
    m_mem_mode = search_mode;

    resetProgramHeap();
}

inline size_t alignBytes(const size_t n_bytes) {
    /* sizeof(data_t) = 4 or 8 -> 7 is 111 -> ~7 is 000 -> last 3 bits will be
     * zeros*/
    return ~(sizeof(data_t) - 1) & (n_bytes + sizeof(data_t) - 1);
}

inline size_t allocationSize(const size_t n_bytes) {
    // adding header (Chunk) size BUT without Chunk().m_data (it's part of
    // user's data)
    return n_bytes + sizeof(Chunk) - sizeof(std::declval<Chunk>().m_data);
}

Chunk* mapOSmemory(const size_t n_bytes) {
    if (n_bytes == 0) {
        return nullptr;
    }

    // obtain pointer of current heap break
    Chunk* current_chunk = (Chunk*)sbrk(0);

    if (sbrk(allocationSize(n_bytes)) == (void*)-1) {
        throw std::bad_alloc();

        return nullptr;
    }

    return current_chunk;
}

Chunk* shiftToHeader(const data_t* chunk_ptr) {
    if (chunk_ptr == nullptr) {
        throw std::invalid_argument("chunk pointer is null");
    }

    return (Chunk*)((uint8_t*)chunk_ptr + sizeof(std::declval<Chunk>().m_data) -
                    sizeof(Chunk));
}

Chunk* getFreeChunk(const size_t n_bytes) {
    if (n_bytes == 0) {
        return nullptr;
    }

    switch (m_mem_mode) {
        case MemoryManagement::first_fit_search: {
            return memFirstFit(n_bytes);
        }

        case MemoryManagement::next_fit_search: {
            return memNextFit(n_bytes);
        }

        case MemoryManagement::best_fit_search: {
            // TODO: implement best-fit
            throw std::runtime_error(
                "Don't use best-fit. Rumors say cats hate it...");
        }

        case MemoryManagement::free_list_search: {
            return freeList(n_bytes);
        }

        default: {
            throw std::runtime_error("Amogus");
        }
    }
}

Chunk* memFirstFit(const size_t n_bytes) {
    if (n_bytes == 0) {
        return nullptr;
    }

    Chunk* current_chunk = m_heap_head;

    // start searching (unoptimized)

    while (current_chunk) {
        if (current_chunk->m_used == true || current_chunk->m_size < n_bytes) {
            current_chunk = current_chunk->m_next;
            // continue is needed so we can return current_chunk after the loop
            continue;
        }

        return current_chunk;
    }

    return nullptr;
}

Chunk* memNextFit(const size_t n_bytes) {
    if (n_bytes == 0) {
        return nullptr;
    }

    // restore the last success position
    Chunk* current_chunk = m_last_found != nullptr ? m_last_found : m_heap_head;

    // TODO: think how to perform it better (maybe bitset?)
    // start searching (O(n) I guess)
    while (current_chunk) {
        if (current_chunk->m_used == true || current_chunk->m_size < n_bytes) {
            current_chunk = current_chunk->m_next;
            // continue is needed so we can return current_chunk after the loop
            continue;
        }

        // update last success position
        m_last_found = current_chunk;
        return current_chunk;
    }

    return nullptr;
}

Chunk* splitChunk(Chunk* cur_chunk, const size_t n_bytes) {
    if (isSplittable(cur_chunk, n_bytes)) {
        size_t splitted_chunk_old_size = cur_chunk->m_size;
        cur_chunk->m_size = n_bytes;

        Chunk* next_chunk_pointer =
            (Chunk*)(((uint8_t*)cur_chunk) + allocationSize(0) +
                     cur_chunk->m_size);
        next_chunk_pointer->m_size =
            splitted_chunk_old_size - cur_chunk->m_size - allocationSize(0);
        next_chunk_pointer->m_used = false;
        next_chunk_pointer->m_next = cur_chunk->m_next;
        next_chunk_pointer->m_prev = cur_chunk;

        cur_chunk->m_next = next_chunk_pointer;
        return cur_chunk;
    }

    return nullptr;
}

inline bool isSplittable(const Chunk* cur_chunk, const size_t n_bytes) {
    // free space left for the rest part of out empty block is not allowed to be
    // less than 16
    return cur_chunk->m_size - n_bytes - allocationSize(0) >=
           SPLIT_RATE_MIN_BYTES;
}

Chunk* allocateFromList(Chunk* cur_chunk, const size_t n_bytes) {
    if (cur_chunk == nullptr) {
        throw std::invalid_argument("chunk pointer is null");
    }

    if (isSplittable(cur_chunk, n_bytes)) {
        cur_chunk = splitChunk(cur_chunk, n_bytes);
    }

    cur_chunk->m_used = true;
    return cur_chunk;
}

inline bool isCoalesceableNext(const Chunk* chunk_ptr) {
    return chunk_ptr->m_next != nullptr && chunk_ptr->m_next->m_used == false;
}

inline bool isCoalesceablePrev(const Chunk* chunk_ptr) {
    return chunk_ptr->m_prev != nullptr && chunk_ptr->m_prev->m_used == false;
}

Chunk* coalesceChunk(Chunk* cur_chunk) {
    if (cur_chunk == nullptr || cur_chunk->m_next == nullptr) {
        throw std::invalid_argument(
            "coalesce chunks is broken, report it to devs");
    }

    Chunk* next_chunk = cur_chunk->m_next;

    cur_chunk->m_next = next_chunk->m_next;
    cur_chunk->m_size += allocationSize(next_chunk->m_size);

    /* TODO: (maybe add poison values for free coalesced chunks ??) */
    next_chunk->m_used = false;
    next_chunk->m_size = 0;

    next_chunk->m_next = nullptr;
    next_chunk->m_prev = nullptr;

    return cur_chunk;
}

Chunk* freeList(const size_t n_bytes) {
    if (n_bytes <= 0) {
        return nullptr;
    }

    for (const auto& cur_chunk : m_free_list) {
        if (cur_chunk->m_size < n_bytes) {
            continue;
        }

        m_free_list.remove(cur_chunk);
        return allocateFromList(cur_chunk, n_bytes);
    }

    return nullptr;
}

};  // namespace X17

#endif  // !X17_GENERIC_ALLOC