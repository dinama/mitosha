#include <assert.h>
#include <mitosha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define massert(cond, ...) ((void) ((cond) || (fprintf(stderr, __VA_ARGS__), exit(EXIT_FAILURE), 0)))

typedef struct tag_s {
  size_t size;
  mvoid_t next;
} tag_t;

static inline void* tag_to_mem(tag_t const* tag) {
  return (void*) &tag->next;
}

static inline tag_t* tag_from_mem(void* mem) {
  return (tag_t*) ((char*) mem - sizeof(size_t));
}

static inline tag_t* tag_next(tag_t* tag) {
  return (tag_t*) mvoid_get(&tag->next);
}

static inline void tag_set_next(tag_t* tag, tag_t* next) {
  mvoid_set(&tag->next, next);
}

typedef struct mpool_s {
  size_t marker;
  size_t size;
  size_t balance;
  size_t ntags;
  mvoid_t bits;
  mvoid_t tags;
  mvoid_t free;
} mpool_t;

static size_t align_size(size_t need) {
  size_t const ntags = 1 + (need + sizeof(size_t) - 1) / sizeof(tag_t);
  return ntags * sizeof(tag_t);
}

static inline size_t bits_size(size_t ntags) {
  return (ntags + 7) / 8;
}

static inline void bits_set(tag_t* tag, mpool_t* pool) {
  uint8_t* bits = mvoid_get(&pool->bits);
  size_t const bit = tag - (tag_t*) mvoid_get(&pool->tags);
  bits[bit / 8] |= 1U << (bit % 8);
}

static inline void bits_clear(tag_t* tag, mpool_t* pool) {
  uint8_t* bits = mvoid_get(&pool->bits);
  size_t const bit = tag - (tag_t*) mvoid_get(&pool->tags);
  bits[bit / 8] &= ~(1U << (bit % 8));
}

static tag_t* tag_left(tag_t* tag, mpool_t* pool) {
  tag_t* tags = mvoid_get(&pool->tags);
  uint8_t* bits = mvoid_get(&pool->bits);
  size_t bit = tag - tags;
  while (bit-- > 0) {
    if (bits[bit / 8] & (1U << (bit % 8)))
      return tags + bit;
  }
  return NULL;
}

static void tag_merge(tag_t* tag, mpool_t* pool) {
  tag_t* next;
  while ((next = tag_next(tag))) {
    if ((char*) next > (char*) tag + tag->size)
      break;
    tag->size += next->size;
    tag_set_next(tag, tag_next(next));
    bits_clear(next, pool);
  }
  bits_set(tag, pool);
}

size_t mpool_calc_required_size(size_t itemsize, size_t nitem) {
  size_t const tags_bytes = align_size(itemsize) * nitem;
  size_t const bits_bytes = bits_size(tags_bytes / sizeof(tag_t));
  return sizeof(mpool_t) + bits_bytes + tags_bytes;
}

size_t mpool_size_stuff(size_t total_memory) {
  size_t const available = total_memory - mpool_calc_required_size(0, 0);
  size_t ntags = available / sizeof(tag_t);
  while (bits_size(ntags) + ntags * sizeof(tag_t) > available)
    --ntags;
  return total_memory - (ntags * sizeof(tag_t) - sizeof(size_t));
}

static const size_t MPOOL_MARKER = 0x4d504f4f4cfafafa; // MPOOL

mpool_t* mpool_attach_existing(void* src) {
  massert(src, "%s nullptr\n", __func__);
  mpool_t* pool = (mpool_t*) src;
  if ((pool->marker & MPOOL_MARKER) == MPOOL_MARKER)
    return pool;
  fprintf(stderr, "%s invalid MPOOL marker\n", __func__);
  return NULL;
}

mpool_t* mpool_format_memory(void* src, size_t size) {
  massert(src, "%s nullptr\n", __func__);

  if (size < mpool_calc_required_size(0, 0)) {
    fprintf(stderr, "%s need %zu or more memory\n", __func__, mpool_calc_required_size(0, 0));
    return NULL;
  }
  size_t const available = size - mpool_calc_required_size(0, 0);
  size_t ntags = available / sizeof(tag_t);
  while (bits_size(ntags) + ntags * sizeof(tag_t) > available)
    --ntags;

  mpool_t* pool = (mpool_t*) src;
  memset(pool, 0, sizeof(*pool));
  pool->marker = MPOOL_MARKER;
  pool->size = size;
  pool->ntags = ntags;

  mvoid_set(&pool->bits, pool + 1);
  uint8_t* bits = mvoid_get(&pool->bits);
  memset(bits, 0, bits_size(ntags));

  mvoid_set(&pool->tags, bits + bits_size(ntags));
  mvoid_set(&pool->free, mvoid_get(&pool->tags));

  tag_t* tag = mvoid_get(&pool->free);
  tag->size = ntags * sizeof(tag_t);
  tag_set_next(tag, NULL);
  pool->balance = 0;

  return pool;
}

void mpool_cleanup(mpool_t* src) {
  struct mpool_s* p = (struct mpool_s*) src;
  (void) p;
}

void mpool_reset(mpool_t* pool) {
  massert(pool, "%s nullptr\n", __func__);
  mvoid_set(&pool->bits, pool + 1);
  uint8_t* bits = mvoid_get(&pool->bits);
  memset(bits, 0, bits_size(pool->ntags));

  mvoid_set(&pool->tags, bits + bits_size(pool->ntags));
  mvoid_set(&pool->free, mvoid_get(&pool->tags));

  tag_t* tag = mvoid_get(&pool->free);
  tag->size = pool->ntags * sizeof(tag_t);
  tag_set_next(tag, NULL);
  pool->balance = 0;
}

size_t mpool_total_size(mpool_t const* pool) {
  massert(pool, "%s nullptr\n", __func__);
  return pool->size;
}

size_t mpool_total_capacity(mpool_t const* pool) {
  massert(pool, "%s nullptr\n", __func__);
  if (!pool->ntags)
    return 0;
  return pool->ntags * sizeof(tag_t) - sizeof(size_t);
}

size_t mpool_used(mpool_t const* pool) {
  massert(pool, "%s nullptr\n", __func__);
  return pool->balance;
}

double mpool_utilization(mpool_t const* pool) {
  massert(pool, "%s nullptr\n", __func__);
  if (!pool->ntags)
    return 1.0;
  return (double) pool->balance / (pool->ntags * sizeof(tag_t));
}

size_t mpool_free_space(mpool_t const* pool) {
  massert(pool, "%s nullptr\n", __func__);
  size_t avail = 0;
  for (tag_t* tag = mvoid_get(&pool->free); tag; tag = tag_next(tag))
    avail += tag->size - sizeof(size_t);
  return avail;
}

void* mpool_alloc(mpool_t* pool, size_t size) {
  massert(pool, "%s nullptr\n", __func__);
  size_t const aligned = align_size(size);

  tag_t* prev = NULL;
  tag_t* tag = mvoid_get(&pool->free);
  while (tag && tag->size < aligned) {
    prev = tag;
    tag = tag_next(tag);
  }
  if (!tag)
    return NULL;

  if (tag->size > aligned) {
    tag_t* n = (tag_t*) ((char*) tag + aligned);
    n->size = tag->size - aligned;
    tag->size = aligned;
    tag_set_next(n, tag_next(tag));
    tag_set_next(tag, n);
    tag_merge(n, pool);
  }

  if (prev)
    tag_set_next(prev, tag_next(tag));
  else
    mvoid_set(&pool->free, tag_next(tag));

  bits_clear(tag, pool);
  pool->balance += aligned;
  return tag_to_mem(tag);
}

void* mpool_realloc(mpool_t* pool, void* ptr, size_t new_size) {
  massert(pool, "%s nullptr\n", __func__);
  if (!ptr)
    return mpool_alloc(pool, new_size);

  size_t const aligned = align_size(new_size);
  tag_t* tag = tag_from_mem(ptr);

  if (aligned == tag->size)
    return ptr;

  if (aligned < tag->size) {
    size_t const shrink = tag->size - aligned;
    if (pool->balance < shrink) {
      fprintf(stderr, "%s pool balance error\n", __func__);
      return NULL;
    }
    pool->balance -= shrink;
    tag->size = aligned;

    tag_t* n = (tag_t*) ((char*) tag + aligned);
    n->size = shrink;

    tag_t* head = mvoid_get(&pool->free);
    if (!head || head > n) {
      mvoid_set(&pool->free, n);
      tag_set_next(n, head);
      tag_merge(n, pool);
      return ptr;
    }

    tag_t* left = tag_left(tag, pool);
    if (!left) {
      fprintf(stderr, "%s bits error\n", __func__);
      return NULL;
    }

    tag_set_next(n, tag_next(left));
    tag_set_next(left, n);
    tag_merge(n, pool);
    tag_merge(left, pool);
    return ptr;
  }

  void* np = mpool_alloc(pool, new_size);
  if (!np)
    return NULL;

  memcpy(np, ptr, tag->size - sizeof(size_t));
  mpool_free(pool, ptr);
  return np;
}

void* mpool_zalloc(mpool_t* pool, size_t size) {
  massert(pool, "%s nullptr\n", __func__);
  void* ptr = mpool_alloc(pool, size);
  if (ptr)
    memset(ptr, 0, size);
  return ptr;
}

void mpool_free(mpool_t* pool, void* ptr) {
  if (!ptr)
    return;

  tag_t* tag = tag_from_mem(ptr);
  if (pool->balance < tag->size) {
    fprintf(stderr, "%s pool balance error\n", __func__);
    return;
  }
  pool->balance -= tag->size;

  tag_t* head = mvoid_get(&pool->free);
  if (!head || head > tag) {
    tag_set_next(tag, head);
    tag_merge(tag, pool);
    mvoid_set(&pool->free, tag);
    return;
  }

  tag_t* left = tag_left(tag, pool);
  if (!left) {
    fprintf(stderr, "%s bits error\n", __func__);
    return;
  }
  tag_set_next(tag, tag_next(left));
  tag_set_next(left, tag);
  tag_merge(tag, pool);
  tag_merge(left, pool);
}

void* mpool_memdup(mpool_t* pool, void const* src, size_t size) {
  massert(pool, "%s nullptr\n", __func__);
  if (!src || !size)
    return NULL;
  void* dst = mpool_alloc(pool, size);
  if (dst)
    memcpy(dst, src, size);
  return dst;
}

void mpool_dump(mpool_t const* pool) {
  massert(pool, "%s nullptr\n", __func__);

  fprintf(stderr, "\n=== MPOOL DUMP ===\n");
  fprintf(stderr, "Pool address   : %p\n", (void*) pool);
  fprintf(stderr, "Total size     : %zu bytes\n", pool->size);
  fprintf(stderr, "Total tags     : %zu tags\n", pool->ntags);
  fprintf(stderr, "Payload capacity: %zu bytes\n", mpool_total_capacity(pool));
  fprintf(stderr, "Used space     : %zu bytes\n", mpool_used(pool));
  fprintf(stderr, "Free space     : %zu bytes\n", mpool_free_space(pool));
  fprintf(stderr, "Utilization    : %.2f%%\n", mpool_utilization(pool) * 100.0);

  fprintf(stderr, "\nFree list:\n");

  tag_t const* tag = mvoid_get(&pool->free);
  while (tag) {
    fprintf(stderr, "  [tag: %p] size: %zu bytes\n", (void*) tag, tag->size);
    tag = tag_next((tag_t*) tag);
  }

  fprintf(stderr, "==================\n\n");
}
