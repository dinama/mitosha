#include <mitosha.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define massert(condition, ...) \
  (void)( (!!( condition )) || ( fprintf(stderr, __VA_ARGS__), exit(EXIT_FAILURE), 0 ) )


struct tag_s {
  size_t   size;
  mvoid_t  link;
};

static inline void* tag_tomem(struct tag_s const* tag) {
  return (void*)&tag->link;
}

static inline struct tag_s* tag_ofmem(void* mem) {
  return (struct tag_s *)((char*)mem - sizeof(size_t));
}

static inline struct tag_s* tag_next(struct tag_s* tag) {
  return (struct tag_s *)mvoid_get(&tag->link);
}

static inline void tag_link(struct tag_s* tag, struct tag_s* link) {
  mvoid_set(&tag->link, link);
}

struct mpool_s {
  size_t  bom;
  size_t  size;
  size_t  balance;
  size_t  ntags;
  mvoid_t bits;
  mvoid_t tags;
  mvoid_t freetag;
};


static size_t mpool_size_aligned(size_t need) {
  size_t const ntags = (1 + (need + sizeof(size_t) - 1) / sizeof(struct tag_s));
  return sizeof(struct tag_s) * ntags;
}

static inline size_t mpool_size_bits(size_t ntags)
{
  return (ntags + 7) / 8;
}

static inline void mpool_bits_set(struct tag_s* tag, struct mpool_s* p) {
  uint8_t* const bits = mvoid_get(&p->bits);
  size_t const bit = tag - (struct tag_s*)mvoid_get(&p->tags);
  bits[bit / 8] |= 1 << (bit % 8);
}

static inline void mpool_bits_drop(struct tag_s* tag, struct mpool_s* p) {
  uint8_t* const bits = mvoid_get(&p->bits);
  size_t const bit = tag - (struct tag_s*)mvoid_get(&p->tags);
  bits[bit / 8] &= ~(1 << (bit % 8));
}

static struct tag_s* mpool_tag_left(struct tag_s* tag, struct mpool_s* p)
{
  struct tag_s* tags = mvoid_get(&p->tags);
  uint8_t* bits = mvoid_get(&p->bits);
  size_t bit = tag - tags;
  while (bit --> 0) {
    if (bits[bit / 8]) {
       if (1 << (bit % 8) == (bits[bit / 8] & (1 << (bit % 8))))
         return tags + bit;
    }
  }
  return NULL;
}

static void mpool_tag_merge(struct tag_s* tag, struct mpool_s* p)
{
  struct tag_s* n;
  while ((n = tag_next(tag))) {

    if ((char*)n > (char*)tag + tag->size)
      break;

    tag->size += n->size;
    tag_link(tag, tag_next(n));
    mpool_bits_drop(n, p);
  }

  mpool_bits_set(tag, p);
}

size_t mpool_size_hint(size_t itemsize, size_t nitem) {

  size_t const tagssize = mpool_size_aligned(itemsize) * nitem;
  size_t const bitssize = mpool_size_bits(tagssize / sizeof(struct tag_s));
  return tagssize + bitssize + sizeof(struct mpool_s);
}

size_t mpool_size_stuff(size_t memsize)
{
  size_t const asize = memsize - mpool_size_hint(0, 0);
  size_t ntags = asize / sizeof(struct tag_s);
  while (mpool_size_bits(ntags)  + ntags * sizeof(struct tag_s) > asize)
    --ntags;
  return memsize - (ntags * sizeof(struct tag_s) - sizeof(size_t));
}

static const size_t BOM = 0xfafafafafafafafa;

mpool_t* mpool_attach(void* src)
{
  massert(src, "%s nullptr\n", __func__);
  struct mpool_s* p = (struct mpool_s*) src;
  if (BOM == (BOM & p->bom)) return p;
  fprintf(stderr, "%s invalid bom marker\n", __func__);
  return NULL;
}

mpool_t* mpool_format(void* src, size_t nsrc)
{
  massert(src, "%s nullptr\n", __func__);

  if (nsrc < mpool_size_hint(0, 0) ) {
    fprintf(stderr, "%s need %zu or more memory \n", __func__,
            mpool_size_hint(0, 0));
    return NULL;
  }

  size_t const asize = nsrc - mpool_size_hint(0, 0);
  size_t ntags = asize / sizeof(struct tag_s);
  while (mpool_size_bits(ntags)  + ntags * sizeof(struct tag_s) > asize)
    --ntags;

  struct mpool_s* p = (struct mpool_s*) src;
  memset(p, 0, sizeof(*p));

  p->bom = BOM;
  p->size = nsrc;
  p->ntags = ntags;
  mpool_clear(p);
  return p;
}

void mpool_cleanup(mpool_t* src) {
  struct mpool_s* p = (struct mpool_s*) src;
  (void)p;
}

void mpool_clear(mpool_t* src)
{
  massert(src, "%s nullptr\n", __func__);
  struct mpool_s* p = (struct mpool_s*) src;

  mvoid_set(&p->bits, p + 1);
  uint8_t* bits = mvoid_get(&p->bits);
  memset(bits, 0, mpool_size_bits(p->ntags));

  mvoid_set(&p->tags, bits + mpool_size_bits(p->ntags));
  mvoid_set(&p->freetag, mvoid_get(&p->tags));

  struct tag_s* tag = mvoid_get(&p->freetag);
  tag->size = p->ntags * sizeof(struct tag_s);
  tag_link(tag, NULL);
}

size_t mpool_memory_size(mpool_t const* src)
{
  massert(src, "%s nullptr\n", __func__);
  struct mpool_s const* p = (struct mpool_s const*) src;
  return p->size;
}

size_t mpool_capacity(mpool_t const* src)
{
  massert(src, "%s nullptr\n", __func__);
  struct mpool_s const* p = (struct mpool_s const*) src;
  if (!p->ntags) return 0;
  return p->ntags * sizeof(struct tag_s) - sizeof(size_t);
}

size_t mpool_balance(mpool_t const* src)
{
  massert(src, "%s nullptr\n", __func__);
  struct mpool_s* p = (struct mpool_s*) src;
  return p->balance;
}

double mpool_load(mpool_t const* src)
{
  massert(src, "%s nullptr\n", __func__);
  struct mpool_s* p = (struct mpool_s*) src;
  if (!p->ntags) return 100.;
  return (double)p->balance / (p->ntags * sizeof(struct tag_s));
}

size_t mpool_avail(mpool_t const* src)
{
  massert(src, "%s nullptr\n", __func__);
  struct mpool_s const* p = (struct mpool_s const*) src;
  size_t avail = 0;
  for (struct tag_s* tag = mvoid_get(&p->freetag); tag; tag = tag_next(tag))
    avail += (tag->size - sizeof(size_t));
  return avail;
}

void mpool_dump(mpool_t const* src)
{
  massert(src, "%s nullptr\n", __func__);
  struct mpool_s const* p = src;
  size_t const avail = mpool_avail(src);
  fprintf(stderr, "> page [%p] size: %zu tags: %zu capacity: %zu avail: %zu used: %zu balance: %zu load: %f\n",
          (void*)p,
          p->size,
          p->ntags,
          mpool_capacity(p),
          avail,
          mpool_capacity(p) - avail,
          p->balance,
          mpool_load(p));

#if 0
  struct tag_s const* tag = mvoid_get(&p->freetag);
  while (tag) {
    fprintf(stderr, "[%ld] %zu -> ", tag - (struct tag_s*)(p + 1), tag->size);
    tag = tag_next(tag);
  }
  fprintf(stderr, "\n");
#endif
}


void* mpool_alloc(mpool_t* src, size_t size)
{
  massert(src, "%s nullptr\n", __func__);
  struct mpool_s* p = (struct mpool_s*) src;
  size_t const aligned = mpool_size_aligned(size);

  struct tag_s* prev = NULL;
  struct tag_s* tag = mvoid_get(&p->freetag);
  while (tag && tag->size < aligned) {
    prev = tag;
    tag = tag_next(tag);
  }

  if (!tag) {
    fprintf(stderr, "%s no memory\n", __func__);
    return NULL;
  }

  if (tag->size > aligned) {

    struct tag_s* n = (struct tag_s*)((char*)tag + aligned);
    n->size = tag->size - aligned;
    tag->size = aligned;
    tag_link(n, tag_next(tag));
    tag_link(tag, n);
    mpool_tag_merge(n, p);
  }

  if (prev) {
    tag_link(prev, tag_next(tag));
  } else {
    mvoid_set(&p->freetag, tag_next(tag));
  }

  mpool_bits_drop(tag, p);
  p->balance += aligned;  
  return tag_tomem(tag);
}

void* mpool_realloc(mpool_t* src, void* ptr, size_t newsz)
{
  massert(src, "%s nullptr\n", __func__);
  struct mpool_s* p = (struct mpool_s*) src;
  if (!ptr) return mpool_alloc(src, newsz);

  size_t const aligned = mpool_size_aligned(newsz);
  struct tag_s* tag = tag_ofmem(ptr);

  if (aligned == tag->size) {
    return ptr;
  }

  if (tag->size < aligned) {
    void* np = mpool_alloc(src, newsz);
    if (!np) return NULL;
    memcpy(np, ptr, tag->size - sizeof(size_t));
    mpool_free(src, ptr);
    return np;
  }

  struct tag_s* n = (struct tag_s*)((char*)tag + aligned);
  n->size = tag->size - aligned;
  tag->size = aligned;

  if (p->balance < n->size) {
    fprintf(stderr, "%s pool balance error\n", __func__);
    return NULL;
  }

  p->balance -= n->size;

  struct tag_s* tmp = mvoid_get(&p->freetag);

  if (!tmp || tmp > n) {
    mvoid_set(&p->freetag, n);
    tag_link(n, tmp);
    mpool_tag_merge(n, p);
    return ptr;
  }

  tmp = mpool_tag_left(tag, p);

  if (!tmp) {
    fprintf(stderr, "%s bits error \n", __func__);
    return NULL;
  }

  tag_link(n, tag_next(tmp));
  tag_link(tmp, n);

  mpool_tag_merge(n, p);
  mpool_tag_merge(tmp, p);
  return ptr;
}

void* mpool_zalloc(mpool_t* src, size_t nbytes)
{
  massert(src, "%s nullptr\n", __func__);
  struct mpool_s* p = (struct mpool_s*) src;
  void* r = mpool_alloc(p, nbytes);
  if (r) memset(r, 0, nbytes);
  return r;
}

void mpool_free(mpool_t* src, void* ptr)
{
  massert(src, "%s nullptr\n", __func__);

  struct mpool_s* p = (struct mpool_s*) src;

  if (!ptr)
    return;

  struct tag_s* tag = tag_ofmem(ptr);  

  if (p->balance < tag->size) {
    fprintf(stderr, "%s pool balance error\n", __func__);
    return;
  }

  p->balance -= tag->size;

  struct tag_s* tmp = mvoid_get(&p->freetag);

  if (!tmp || tmp > tag) {
    tag_link(tag, tmp);
    mpool_tag_merge(tag, p);
    mvoid_set(&p->freetag, tag);
    return;
  }

  tmp = mpool_tag_left(tag, p);

  if (!tmp) {
    fprintf(stderr, "%s bits error \n", __func__);
    return;
  }

  tag_link(tag, tag_next(tmp));
  tag_link(tmp, tag);

  mpool_tag_merge(tag, p);
  mpool_tag_merge(tmp, p);
}

void* mpool_memdup(mpool_t* src, void const* d, size_t dsz)
{
  massert(src, "%s nullptr\n", __func__);
  struct mpool_s* p = (struct mpool_s*) src;
  void* r = mpool_alloc(p, dsz);
  if (r) memcpy(r, d, dsz);
  return r;
}
