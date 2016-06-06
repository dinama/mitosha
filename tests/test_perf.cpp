#include <mutest.h>
#include <ctime>
#include <cstdlib>
#include <cinttypes>
#include <mitosha.h>

extern "C" void mpool_dump(mpool_t const* p);

namespace test {

  inline static float measure(clock_t cl, int count) {
    return (float)count / ((float)(clock() - cl) / (float)CLOCKS_PER_SEC);
  }

  struct person_s {
    int age;
    int validate;
  };

  struct person_avl_s : avlnode_t, person_s { };

  static inline int avl_cmp(avlnode_t const* l, avlnode_t const* r) {
    return ((person_avl_s const*)l)->age - ((person_avl_s const*)r)->age;
  }


  static const int CYCLE = 10;
  static const int COUNT = 50000;
  static size_t DATASIZE = 0;

  static void avl_perf(float&);
  static void std_perf(float&);
  static void std_perf_pool(float&);
  static void std_perf_shared(float&);
}

/*--------------------------------------------------------------------------*/

extern "C" void mu_test_perf()
{
  const size_t isize = mpool_size_hint(sizeof(test::person_avl_s), test::COUNT);
  const size_t tsize = mpool_size_hint(sizeof(avltree_t), 1);
  test::DATASIZE = (isize + tsize) * 3;

  std::printf("data size: %zu\n", test::DATASIZE);
  std::printf("objects : %d\n", test::COUNT);
  std::printf("operations: %d\n", test::COUNT * test::CYCLE);

  float avl_tm = 1; test::avl_perf(avl_tm);
  float ipc_tm = 1; test::std_perf_shared(ipc_tm);
  float std_tm = 1; test::std_perf(std_tm);
  float stdp_tm = 1; test::std_perf_pool(stdp_tm);

  std::printf("avl: %d\n", (int)avl_tm);
  std::printf("ipc: %d %f\n", (int)ipc_tm, avl_tm / ipc_tm);
  std::printf("std: %d %f\n", (int)std_tm, avl_tm / std_tm);
  std::printf("stdp: %d %f\n", (int)stdp_tm, avl_tm / stdp_tm);
}

/*--------------------------------------------------------------------------*/

static void test::avl_perf(float& r)
{
  mshm_t* shma = nullptr;
  void* mem = nullptr;
  mpool_t* pool = nullptr;

  shma = mshm_create("/mitosha", test::DATASIZE);
  mu_ensure(shma);
  mem = mshm_memory_ptr(shma);

  pool = mpool_format(mem, mshm_memory_size(shma));
  mu_ensure(pool);

  avltree_t* set = (avltree_t*)mpool_alloc(pool, sizeof(*set));
  avltree_init(set);

  // fill
  for (int i = 0;  i < test::COUNT; ++i) {
    test::person_avl_s* p = (test::person_avl_s*)mpool_alloc(pool, sizeof(*p));
    mu_check(p);
    p->age = i;
    p->validate = i + 1;
    avltree_insert(p, test::avl_cmp, set);
  }

  // print
  mpool_dump(pool);

  std::clock_t cl = std::clock();

  for (int j = 0; j < test::CYCLE; ++j) {

    test::person_avl_s p;

    for (p.age = 0;  p.age < test::COUNT; ++p.age) {

      // search
      test::person_avl_s* n = (test::person_avl_s*)avltree_lookup(&p, test::avl_cmp, set);
      mu_ensure(n);

      //  remove && free
      test::person_avl_s v = *n;
      avltree_remove(n, set);

      mpool_free(pool, n);
      n = (test::person_avl_s*)mpool_alloc(pool, sizeof(*n));
      *n = v;

      // insert
      avltree_insert(n, test::avl_cmp, set);

      // search
      n = (test::person_avl_s*)avltree_lookup(&p, test::avl_cmp, set);
      mu_ensure(n);
      mu_check(n->validate == p.age  + 1);
    }
  }

  r = test::measure(cl, test::COUNT * test::CYCLE);
  mshm_cleanup(shma);
}

/*--------------------------------------------------------------------------*/


#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <set>

namespace std {
    template<>
    struct less<test::person_s> : public binary_function<test::person_avl_s, test::person_avl_s, bool> {
        inline bool  operator()(test::person_s const& __x, test::person_s const& __y) const {
            return __x.age < __y.age;
        }
    };
}

namespace test {
  template<typename C>
  static void do_test(C& set, float& r) {

    test::person_s p;

    // fill
    for (p.age = 0;  p.age < test::COUNT; ++p.age) {
      p.validate = p.age + 1;
      set.insert(p);
    }

    clock_t cl = clock();

    for (int j = 0; j < test::CYCLE; ++j) {
      for (p.age = 0;  p.age < test::COUNT; ++p.age) {
        // search
        typename C::const_iterator it = set.find(p);
        mu_ensure(it != set.end());

        // remove
        test::person_s v = *it;
        set.erase(it);

        // insert
        set.insert(v);

        // search
        it  = set.find(p);
        mu_ensure(it->validate == p.age + 1);
      }
    }
    r = test::measure(cl, test::COUNT * test::CYCLE);
  }
}

static void test::std_perf(float& r)
{
  using test_set_t = std::set<test::person_s>;
  test_set_t set;
  test::do_test(set, r);
}


static void test::std_perf_pool(float& r)
{
    using test_alloc_t = boost::fast_pool_allocator<test::person_s>;
    using test_set_t = std::set<test::person_s, std::less<test::person_s>, test_alloc_t>;

    test_set_t set;
    test::do_test(set, r);
}


#include <boost/interprocess/mem_algo/simple_seq_fit.hpp>
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/segment_manager.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/set.hpp>

static void test::std_perf_shared(float& r)
{
  namespace ipc = boost::interprocess;
  // using shared_memory = ipc::basic_managed_shared_memory<char, ipc::simple_seq_fit<ipc::null_mutex_family>, ipc::iset_index>;
  // using shared_memory = managed_shared_memory;
  using shared_memory = ipc::basic_managed_shared_memory<char, ipc::rbtree_best_fit<ipc::null_mutex_family>, ipc::iset_index>;
  using test_alloc_t = ipc::allocator<test::person_s, shared_memory::segment_manager>;
  using test_set_t = ipc::set<test::person_s, std::less<test::person_s>, test_alloc_t>;

  char memname[] = "/mitosha_ipc";
  ipc::shared_memory_object::remove(memname);
  {
    shared_memory mem(ipc::open_or_create, memname, test::DATASIZE);
    test_alloc_t alloc(mem.get_segment_manager());
    test_set_t* s = mem.find_or_construct<test_set_t>("SET")(std::less<test::person_s>(), alloc);
    mu_ensure(s);

    test::do_test(*s, r);

    std::printf("> size: %zu used: %zu avail: %zu\n",
             mem.get_size(),
             mem.get_size() - mem.get_free_memory(),
             mem.get_free_memory());
  }
  ipc::shared_memory_object::remove(memname);
}
