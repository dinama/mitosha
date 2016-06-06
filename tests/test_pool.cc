#include <mutest.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <mitosha.h>

extern "C" void mpool_dump(mpool_t const* p);

extern "C" void mu_test_heap_mini() {
  char buff[1024];
  mpool_t* p = mpool_format_memory(buff, sizeof(buff));
  mu_check(p);

  void* pts = mpool_alloc(p, mpool_total_capacity(p));
  mu_check(pts);

  mpool_free(p, pts);

  pts = mpool_alloc(p, mpool_total_capacity(p) + 1);
  mu_check(!pts);

  mpool_reset(p);
  mpool_cleanup(p);
}

extern "C" void mu_test_heap_off() {
  char b[mpool_calc_required_size(1, 1)];
  mpool_t* p = mpool_format_memory(b, sizeof(b));
  mu_check(p);

  void* s = mpool_alloc(p, 1);
  mu_check(s);

  mpool_cleanup(p);
}

extern "C" void mu_test_drive() {
  size_t const sz = 1024 * 1024 + 123;
  char* mem = (char*) malloc(sz);
  mu_check(mem);

  for (size_t i = 0; i < sz; ++i)
    mem[i] = (char) i;

  mpool_t* p = mpool_format_memory(mem, sz);
  mu_check(p);
  mpool_reset(p);
  mpool_cleanup(p);
  free(mem);
}

extern "C" void mu_test_pool_open() {
  size_t const sz = 1024;
  char b[sz] = {1};

  mpool_t* p = mpool_attach_existing(b);
  mu_check(!p);

  mpool_format_memory(b, sizeof(b));
  p = mpool_attach_existing(b);
  mu_check(p);

  mu_check(0 == mpool_used(p));
  mu_check(sizeof(b) == mpool_total_size(p));

  size_t const stuff = mpool_size_stuff(sz);
  mu_check(mpool_total_size(p) - stuff == mpool_total_capacity(p));
}

extern "C" void mu_test_pool_balance() {
  size_t const sz = mpool_calc_required_size(8, 3);
  char b[sz];
  mpool_t* p = mpool_format_memory(b, sizeof(b));
  mu_check(p);

  mu_check(0 == mpool_used(p));

  void* v1 = mpool_alloc(p, 8);
  void* v2 = mpool_alloc(p, 8);
  void* v3 = mpool_alloc(p, 8);

  mu_check(16 * 3 == mpool_used(p));

  mpool_free(p, v2);
  mu_check(16 * 2 == mpool_used(p));

  mpool_free(p, v1);
  mpool_free(p, v3);
  mu_check(0 == mpool_used(p));

  void* v4 = mpool_alloc(p, 16 + 16 + 8);
  mu_check(16 * 3 == mpool_used(p));
  mu_check(mpool_utilization(p) > 0.9f);

  mpool_free(p, v4);
  mu_check(0 == mpool_used(p));
  mu_check(mpool_utilization(p) < 0.1f);

  void* r1 = mpool_alloc(p, 16);
  mu_check(32 == mpool_used(p));
  ((uint8_t*) r1)[0] = 111;
  ((uint8_t*) r1)[1] = 222;

  void* r2 = mpool_realloc(p, r1, 16);
  mu_check(r2 == r1);
  mu_check(32 == mpool_used(p));
  mu_check(111 == ((uint8_t*) r2)[0]);
  mu_check(222 == ((uint8_t*) r2)[1]);

  r1 = mpool_realloc(p, r2, 8);
  mu_check(r1 == r2);
  mu_check(16 == mpool_used(p));
  mu_check(111 == ((uint8_t*) r1)[0]);

  r2 = mpool_realloc(p, r1, 24);
  mu_check(r2 != r1);
  mu_check(32 == mpool_used(p));
  mu_check(111 == ((uint8_t*) r2)[0]);

  mpool_cleanup(p);
}

// fragmentation test
extern "C" void mu_test_pool_fragmentation() {
  size_t const sz = mpool_calc_required_size(64, 100);
  char b[sz];
  mpool_t* p = mpool_format_memory(b, sizeof(b));
  mu_check(p);

  void* blocks[100];
  for (size_t i = 0; i < 100; ++i)
    blocks[i] = mpool_alloc(p, 1 + rand() % 64);

  for (size_t i = 0; i < 100; i += 3)
    mpool_free(p, blocks[i]);

  for (size_t i = 0; i < 100; ++i) {
    if (i % 3 == 0)
      blocks[i] = mpool_alloc(p, 1 + rand() % 32);
  }

  for (size_t i = 0; i < 100; ++i)
    mpool_free(p, blocks[i]);

  mu_check(mpool_used(p) == 0);
  mpool_cleanup(p);
}

// realloc corner case
extern "C" void mu_test_pool_realloc_cases() {
  size_t const sz = mpool_calc_required_size(128, 10);
  char b[sz];
  mpool_t* p = mpool_format_memory(b, sizeof(b));
  mu_check(p);

  void* blk = mpool_alloc(p, 16);
  mu_check(blk);

  blk = mpool_realloc(p, blk, 64);
  mu_check(blk);

  blk = mpool_realloc(p, blk, 8);
  mu_check(blk);

  blk = mpool_realloc(p, blk, 128);
  mu_check(blk);

  mpool_free(p, blk);
  mu_check(mpool_used(p) == 0);
  mpool_cleanup(p);
}

// random stress
extern "C" void mu_test_pool_random_stress() {
  size_t const sz = mpool_calc_required_size(256, 500);
  char* b = (char*) malloc(sz);
  mu_check(b);

  mpool_t* p = mpool_format_memory(b, sz);
  mu_check(p);

  void* blocks[500] = {0};

  for (int iter = 0; iter < 10000; ++iter) {
    size_t idx = rand() % 500;
    if (blocks[idx]) {
      mpool_free(p, blocks[idx]);
      blocks[idx] = NULL;
    } else {
      size_t sz = 1 + rand() % 256;
      blocks[idx] = mpool_alloc(p, sz);
    }
  }

  for (size_t i = 0; i < 500; ++i)
    if (blocks[i])
      mpool_free(p, blocks[i]);

  mu_check(mpool_used(p) == 0);
  mpool_cleanup(p);
  free(b);
}

extern "C" void mu_test_pool_realloc_stress() {
  size_t const sz = mpool_calc_required_size(512, 100);
  char b[sz];
  mpool_t* p = mpool_format_memory(b, sizeof(b));
  mu_check(p);

  void* blk = mpool_alloc(p, 8);
  mu_check(blk);

  for (size_t i = 0; i < 100; ++i) {
    size_t s = 8 + rand() % 512;
    blk = mpool_realloc(p, blk, s);
    mu_check(blk);
  }

  mpool_free(p, blk);
  mu_check(mpool_used(p) == 0);
  mpool_cleanup(p);
}

// performance test
inline float measure(size_t count, clock_t cl) {
  return (float) count / ((float) (clock() - cl) / CLOCKS_PER_SEC);
}

static float pool_perf(size_t count, size_t size);
static float boost_perf(size_t count, size_t size);

extern "C" void mu_test_mpool_perf() {
  size_t count = 10000;
  size_t size = 128;

  float mp = pool_perf(count, size);
  float bp = boost_perf(count, size);

  printf("mpool perf: %.2f ops/sec\n", mp);
  printf("boost perf: %.2f ops/sec\n", bp);
  mu_check(mp > 3 * bp);
}

static float pool_perf(size_t count, size_t max_size) {
  size_t total_size = mpool_calc_required_size(max_size, count);
  char* b = (char*) malloc(total_size);
  mu_check(b);
  mpool_t* p = mpool_format_memory(b, total_size);
  mu_check(p);

  void** blocks = (void**) malloc(count * sizeof(void*));
  mu_check(blocks);

  clock_t cl = clock();

  for (size_t i = 0; i < count; ++i) {
    size_t sz = 1 + rand() % max_size;
    blocks[i] = mpool_alloc(p, sz);
    mu_check(blocks[i]);
  }

  for (size_t i = 0; i < count; i += 2) {
    mpool_free(p, blocks[i]);
    blocks[i] = nullptr;
  }

  for (size_t i = 0; i < count; i += 2) {
    size_t sz = 1 + rand() % max_size;
    blocks[i] = mpool_alloc(p, sz);
    mu_check(blocks[i]);
  }

  for (size_t i = 0; i < count; ++i) {
    if (blocks[i])
      mpool_free(p, blocks[i]);
  }

  float res = measure(count * 2, cl);
  mpool_cleanup(p);
  free(blocks);
  free(b);
  return res;
}

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/mem_algo/simple_seq_fit.hpp>

static float boost_perf(size_t count, size_t max_size) {
  using namespace boost::interprocess;

  struct shm_remove {
    shm_remove() { shared_memory_object::remove("/mitosha"); }
    ~shm_remove() { shared_memory_object::remove("/mitosha"); }
  } remover;

  typedef basic_managed_shared_memory<char, simple_seq_fit<mutex_family>, iset_index> shm_t;
  shm_t p(create_only, "/mitosha", size_t(1) << 27);

  void** blocks = (void**) malloc(count * sizeof(void*));
  mu_check(blocks);

  clock_t cl = clock();

  for (size_t i = 0; i < count; ++i) {
    size_t sz = 1 + rand() % max_size;
    blocks[i] = p.allocate(sz);
    mu_check(blocks[i]);
  }

  for (size_t i = 0; i < count; i += 2) {
    p.deallocate(blocks[i]);
    blocks[i] = nullptr;
  }

  for (size_t i = 0; i < count; i += 2) {
    size_t sz = 1 + rand() % max_size;
    blocks[i] = p.allocate(sz);
    mu_check(blocks[i]);
  }

  for (size_t i = 0; i < count; ++i) {
    if (blocks[i])
      p.deallocate(blocks[i]);
  }

  float res = measure(count * 2, cl);
  free(blocks);
  return res;
}
