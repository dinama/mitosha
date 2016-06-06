#include <mutest.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <mitosha.h>

extern "C" void mpool_dump(mpool_t const* p);

extern "C" void mu_test_heap_mini() {

  char buff[1024];
  mpool_t* p = mpool_format(buff, sizeof(buff));

  void* pts = 0;
  pts = mpool_alloc(p, mpool_capacity(p));
  mu_check(pts);

  mpool_free(p, pts);

  pts = mpool_alloc(p, mpool_capacity(p) + 1);
  mu_check(!pts);

  mpool_clear(p);
  mpool_cleanup(p);

}

extern "C" void mu_test_heap_off() {

  char b[mpool_size_hint(1, 1)];
  mpool_t*  p = mpool_format(b, sizeof(b));
  mu_ensure(p);

  char* s = (char*)mpool_alloc(p, 1);
  mu_ensure(s);

  mpool_cleanup(p);
}


extern "C" void mu_test_drive()
{
  size_t const sz = 1024 * 1024 + 123;
  char* mem = (char*)malloc(sz);
  for (size_t i = 0; i < sz; ++ i)
    mem[i] = i;

  mpool_t* p = mpool_format(mem, sz);
  mu_check(p);
  mpool_clear(p);
  mpool_cleanup(p);
}

extern "C" void mu_test_pool_open()
{
  size_t const sz = 1024;
  char b[sz] = {1};

  mpool_t* p = NULL;

  p = mpool_attach(b);
  mu_check(NULL == p);

  mpool_format(b, sizeof(b));
  p = mpool_attach(b);
  mu_check(p);


  mu_check(0 == mpool_balance(p));

  mu_check(sizeof(b) == mpool_memory_size(p));
  size_t const stuff = mpool_size_stuff(1024);
  mu_check(mpool_memory_size(p) - stuff == mpool_capacity(p));

}

extern "C" void mu_test_pool_balance()
{
  size_t const sz = mpool_size_hint(8, 3);

  char b[sz];
  mpool_t* p = mpool_format(b, sizeof(b));
  mu_check(0 == mpool_balance(p));

  void* v1 = mpool_alloc(p, 8);
  void* v2 = mpool_alloc(p, 8);
  void* v3 = mpool_alloc(p, 8);

  mu_check(16 * 3 == mpool_balance(p));
  mpool_free(p, v2);

  mu_check(16 * 2 == mpool_balance(p));
  mpool_free(p, v1);
  mpool_free(p, v3);

  mu_check(0 == mpool_balance(p));
  void* v4 = mpool_alloc(p, 16 + 16 + 8);

  mu_check(16 * 3 == mpool_balance(p));

  mu_check(0.9 < mpool_load(p));  
  mpool_free(p, v4);
  mu_check(0 == mpool_balance(p));
  mu_check(0.1 > mpool_load(p));

  // realloc

  void* r1 = mpool_alloc(p, 16);
  mu_check(32 == mpool_balance(p));
  ((uint8_t*)r1)[0] = 111;
  ((uint8_t*)r1)[1] = 222;


  void* r2 = mpool_realloc(p, r1, 16);
  mu_check(r2 == r1);
  mu_check(32 == mpool_balance(p));
  mu_check(111 == ((uint8_t*)r2)[0]);
  mu_check(222 == ((uint8_t*)r2)[1]);


  r1 = mpool_realloc(p, r2, 8);
  mu_check(r2 == r1);
  mu_check(16 == mpool_balance(p));
  mu_check(111 == ((uint8_t*)r1)[0]);


  r2 = mpool_realloc(p, r1, 24);
  mu_check(r2 != r1);
  mu_check(32 == mpool_balance(p));
  mu_check(111 == ((uint8_t*)r2)[0]);

  mpool_cleanup(p);
}


inline float measure(size_t count, clock_t cl) {
  return (float)count / ((float)(clock() - cl) / (float)CLOCKS_PER_SEC);
}

static float pool_perf(size_t count, size_t size);
static float boost_perf(size_t count, size_t size);


extern "C" void mu_test_pool_perf()
{
  size_t const count  = 100;

  printf("boost perf: %f\n", boost_perf(count, 53));
  printf("pool perf: %f\n", pool_perf(count, 53));
}

static float pool_perf(size_t count, size_t size)
{

  char b[mpool_size_hint(size, count)];
  mpool_t* p = mpool_format(b, sizeof(b));

  clock_t cl = clock();

  void* mem[count];
  for (size_t i = 0; i < count; ++i) {

    mem[i] = mpool_alloc(p, rand() %  size + 1);
    mu_check(mem[i]);

    for (size_t j = 0; j < i; ++ j) {

        if (j % 2) {
          mpool_free(p, mem[j]);
        }

    }

    for (size_t j = 0; j < i; ++j) {
      if (j % 2) {
        mem[j] = mpool_alloc(p, rand() %  size + 1);
        mu_check(mem[j]);
      }
    }

  }

  printf("used: %zu\n", mpool_balance(p));

  for (size_t i = 0; i < count; ++i) {
    mpool_free(p, mem[i]);
  }

  //mpool_dump(p);

  return measure(count, cl);
}

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/mem_algo/simple_seq_fit.hpp>

static float boost_perf(size_t count, size_t size)
{
   using namespace boost::interprocess;

   struct shm_remove
   {
      shm_remove() { shared_memory_object::remove("/mitosha"); }
      ~shm_remove(){ shared_memory_object::remove("/mitosha"); }
   } remover;

   //Managed memory segment that allocates portions of a shared memory
   //segment with the default management algorithm
   // typedef basic_managed_shared_memory
   //    <char
  //  ,simple_seq_fit<mutex_family>
  //     ,iset_index> shared_memory_t;

   typedef basic_managed_shared_memory
      <char
      ,rbtree_best_fit<mutex_family>
      ,iset_index> shared_memory_t; //   managed_shared_memory


   shared_memory_t p(create_only, "/mitosha", 1 << 24);

   clock_t cl = clock();

   void* mem[count];
   for (size_t i = 0; i < count; ++i) {

     mem[i] = p.allocate(rand() %  size + 1);
     mu_check(mem[i]);

     for (size_t j = 0; j < i; ++ j) {

         if (j % 2) {
           p.deallocate(mem[j]);
         }
     }

     for (size_t j = 0; j < i; ++j) {
       if (j % 2) {
         mem[j] = p.allocate(rand() %  size + 1);
         mu_check(mem[j]);
       }
     }
   }

   printf("used: %zu\n", p.get_size() - p.get_free_memory());

   for (size_t i = 0; i < count; ++i) {
     p.deallocate(mem[i]);
   }


   return measure(count, cl);
}
