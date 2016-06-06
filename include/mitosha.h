#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/* offset pointer type */

typedef struct {
    ptrdiff_t offset;
} mvoid_t;

inline static void mvoid_set(mvoid_t* ptr, void* addr) {
    if (addr == ptr) ptr->offset = INTPTR_MIN;
    else if (addr) ptr->offset = (char const*)addr - (char const*)ptr;
    else ptr->offset = 0;
}

inline static void* mvoid_get(mvoid_t const* ptr) {
  if (INTPTR_MIN == ptr->offset) return (void*)ptr;
  if (ptr->offset) return (char*)ptr + ptr->offset;
  return NULL;
}


/*---------------------------------------------------------------------------*/
/** memory manager  */

size_t mpool_size_hint(size_t itemsize, size_t nitem);
size_t mpool_size_stuff(size_t);

typedef struct mpool_s mpool_t;

mpool_t* mpool_attach(void*);
mpool_t* mpool_format(void* addr, size_t);
void mpool_cleanup(mpool_t*);
void mpool_clear(mpool_t*);
size_t mpool_memory_size(mpool_t const*);
size_t mpool_capacity(mpool_t const*);
size_t mpool_balance(mpool_t const*);
size_t mpool_avail(mpool_t const*);
double mpool_load(mpool_t const*);
void* mpool_alloc(mpool_t*, size_t);
void* mpool_realloc(mpool_t*, void*, size_t);
void* mpool_zalloc(mpool_t*, size_t);
void  mpool_free(mpool_t*, void*);
void* mpool_memdup(mpool_t*, void const*, size_t);

/*---------------------------------------------------------------------------*/
/* shared memory kit  */

typedef void mshm_t;

mshm_t* mshm_create(char const*, size_t);
mshm_t* mshm_open(char const*);
void    mshm_unlink(char const*);
void    mshm_cleanup(mshm_t*);
char const*   mshm_name(mshm_t const*);
int     mshm_trylock(mshm_t*);
int     mshm_lock(mshm_t*);
int     mshm_unlock(mshm_t*);
int     mshm_unlock_force(mshm_t*);
void*   mshm_memory_ptr(mshm_t const*);
size_t  mshm_memory_size(mshm_t const*);


/*---------------------------------------------------------------------------*/
/* offset member cast  */

#define mcontainer_of(nodepointer, type, member) \
  ((type *)((char *)(nodepointer) - offsetof(type, member)))


/*---------------------------------------------------------------------------*/
/* avl tree */

typedef struct {
    mvoid_t right;
    mvoid_t left;
    mvoid_t parent;
    signed balance:3;		/* balance factor [-2:+2] */
} avlnode_t;

typedef int (*avltree_compare_f)(avlnode_t const* , avlnode_t const*);

typedef struct {
    mvoid_t root;
    int     height;
    mvoid_t first;
    mvoid_t last;
} avltree_t;

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
int  avltree_init(avltree_t* tree);

/*---------------------------------------------------------------------------*/
/* linked list */

typedef struct {
    mvoid_t next;
    mvoid_t prev;
} listnode_t;

typedef int (*list_compare_f)(listnode_t const* l, listnode_t const* r);

typedef struct {
  mvoid_t first;
  mvoid_t last;
} list_t;

listnode_t* list_lookup(listnode_t const*, list_compare_f cmp, list_t const*);

listnode_t* list_front(list_t const*);
listnode_t* list_back(list_t const*);
listnode_t* list_first(listnode_t const* node);
listnode_t* list_last(listnode_t const* node);
listnode_t* list_next(listnode_t const* node);
listnode_t* list_prev(listnode_t const* node);

void list_insert_befor(listnode_t* where, listnode_t* node, list_t*);
void list_insert_after(listnode_t* where, listnode_t* node, list_t*);
void list_push_back(listnode_t* node, list_t*);
void list_push_front(listnode_t* node, list_t*);

void list_remove(listnode_t* node, list_t*);
void list_replace(listnode_t* old, listnode_t* node, list_t*);

void list_swap(listnode_t* node1, listnode_t* node2, list_t*);
void list_sort(list_t*, list_compare_f cmp);
int list_init(list_t*);

#ifdef __cplusplus
}
#endif
