#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/* offset pointer type (relative pointer) */

typedef struct {
  ptrdiff_t offset;
} mvoid_t;

/* Set relative pointer */
inline static void mvoid_set(mvoid_t* ptr, void* addr) {
  if (addr == ptr)
    ptr->offset = INTPTR_MIN;
  else if (addr)
    ptr->offset = (char const*) addr - (char const*) ptr;
  else
    ptr->offset = 0;
}

/* Get absolute address from relative pointer */
inline static void* mvoid_get(mvoid_t const* ptr) {
  if (INTPTR_MIN == ptr->offset)
    return (void*) ptr;
  if (ptr->offset)
    return (char*) ptr + ptr->offset;
  return NULL;
}

/*---------------------------------------------------------------------------*/
/** memory pool allocator (mpool) */

/**
 * Estimate required memory size for given item size/count.
 * This includes internal metadata overhead.
 */
size_t mpool_calc_required_size(size_t itemsize, size_t nitem);

/**
 * Estimate available payload size for given allocated memory block.
 * Useful for determining effective usable size after allocation.
 */
size_t mpool_size_stuff(size_t total_memory_size);

typedef struct mpool_s mpool_t;

/**
 * Attach to an existing memory pool (previously formatted).
 */
mpool_t* mpool_attach_existing(void*);

/**
 * Format raw memory as a memory pool.
 * Memory must be pre-allocated and large enough (use mpool_size_hint()).
 */
mpool_t* mpool_format_memory(void* addr, size_t);

/**
 * Completely cleanup memory pool (zero state, no deallocation of raw memory).
 */
void mpool_cleanup(mpool_t*);

/**
 * Clear pool to empty state while preserving structure.
 */
void mpool_reset(mpool_t*);

/**
 * Return total memory size of the pool.
 */
size_t mpool_total_size(mpool_t const*);

/**
 * Return total capacity available for allocation (excluding overhead).
 */
size_t mpool_total_capacity(mpool_t const*);

/**
 * Return currently used memory inside the pool.
 */
size_t mpool_used(mpool_t const*);

/**
 * Return currently free available memory.
 */
size_t mpool_free_space(mpool_t const*);

/**
 * Return pool utilization as float [0.0 ... 1.0].
 */
double mpool_utilization(mpool_t const*);

/**
 * Allocate memory from pool.
 * Returns pointer to allocated block or NULL if not enough space.
 */
void* mpool_alloc(mpool_t*, size_t);

/**
 * Reallocate memory block inside the pool.
 * May move memory, old content is preserved on grow.
 */
void* mpool_realloc(mpool_t*, void*, size_t);

/**
 * Allocate memory and zero it (calloc-like).
 */
void* mpool_zalloc(mpool_t*, size_t);

/**
 * Free previously allocated block, may trigger block merging.
 */
void mpool_free(mpool_t*, void*);

/**
 * Duplicate external buffer into pool.
 */
void* mpool_memdup(mpool_t*, void const*, size_t);

/*---------------------------------------------------------------------------*/
/* shared memory interface */

typedef void mshm_t;

/**
 * Create named shared memory segment.
 */
mshm_t* mshm_create(char const*, size_t);

/**
 * Open existing shared memory segment.
 */
mshm_t* mshm_open(char const*);

/**
 * Remove named shared memory segment.
 */
void mshm_unlink(char const*);

/**
 * Cleanup shm object.
 */
void mshm_cleanup(mshm_t*);

/**
 * Return shm name.
 */
char const* mshm_name(mshm_t const*);

/**
 * Try lock shared memory segment (non-blocking).
 */
int mshm_trylock(mshm_t*);

/**
 * Lock shared memory segment (blocking).
 */
int mshm_lock(mshm_t*);

/**
 * Unlock shared memory.
 */
int mshm_unlock(mshm_t*);

/**
 * Force unlock shared memory.
 */
int mshm_unlock_force(mshm_t*);

/**
 * Return raw pointer to shm memory.
 */
void* mshm_memory_ptr(mshm_t const*);

/**
 * Return size of shm memory segment.
 */
size_t mshm_memory_size(mshm_t const*);

/*---------------------------------------------------------------------------*/
/* offset member cast */

#define mcontainer_of(nodepointer, type, member) ((type*) ((char*) (nodepointer) - offsetof(type, member)))

/*---------------------------------------------------------------------------*/
/* AVL tree structure */

/* AVL tree node */
typedef struct {
  mvoid_t right;
  mvoid_t left;
  mvoid_t parent;
  signed balance : 3; /* balance factor [-2..+2] */
} avlnode_t;

/* Comparison callback for AVL insert/search */
typedef int (*avltree_compare_f)(avlnode_t const*, avlnode_t const*);

/* AVL tree container */
typedef struct {
  mvoid_t root;
  int height;
  mvoid_t first;
  mvoid_t last;
} avltree_t;

/* AVL tree operations */
avlnode_t* avltree_first(avltree_t const* tree);
avlnode_t* avltree_last(avltree_t const* tree);
avlnode_t* avltree_next(avlnode_t const* node);
avlnode_t* avltree_prev(avlnode_t const* node);
avlnode_t* avltree_lookup(avlnode_t const* key, avltree_compare_f cmp, avltree_t const* tree);
avlnode_t* avltree_lower(avlnode_t const* key, avltree_compare_f cmp, avltree_t const* tree);
avlnode_t* avltree_upper(avlnode_t const* key, avltree_compare_f cmp, avltree_t const* tree);
avlnode_t* avltree_insert(avlnode_t* node, avltree_compare_f cmp, avltree_t* tree);
void avltree_remove(avlnode_t* node, avltree_t* tree);
void avltree_replace(avlnode_t* old, avlnode_t* node, avltree_t* tree);
int avltree_init(avltree_t* tree);

/*---------------------------------------------------------------------------*/
/* intrusive doubly linked list */

/* List node */
typedef struct {
  mvoid_t next;
  mvoid_t prev;
} listnode_t;

/* Comparison callback for list search */
typedef int (*list_compare_f)(listnode_t const* l, listnode_t const* r);

/* List container */
typedef struct {
  mvoid_t first;
  mvoid_t last;
} list_t;

/* List search */
listnode_t* list_lookup(listnode_t const*, list_compare_f cmp, list_t const*);

/* List iteration */
listnode_t* list_front(list_t const*);
listnode_t* list_back(list_t const*);
listnode_t* list_first(listnode_t const* node);
listnode_t* list_last(listnode_t const* node);
listnode_t* list_next(listnode_t const* node);
listnode_t* list_prev(listnode_t const* node);

/* List insertion */
void list_insert_befor(listnode_t* where, listnode_t* node, list_t*);
void list_insert_after(listnode_t* where, listnode_t* node, list_t*);
void list_push_back(listnode_t* node, list_t*);
void list_push_front(listnode_t* node, list_t*);

/* List deletion */
void list_remove(listnode_t* node, list_t*);
void list_replace(listnode_t* old, listnode_t* node, list_t*);

/* List utility */
void list_swap(listnode_t* node1, listnode_t* node2, list_t*);
void list_sort(list_t*, list_compare_f cmp);
int list_init(list_t*);

#ifdef __cplusplus
}
#endif
