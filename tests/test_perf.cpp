#include <mutest.h>
#include <ctime>
#include <cstdlib>
#include <cinttypes>
#include <mitosha.h>
#include <cstdio>
#include <cassert>

extern "C" void mpool_dump(mpool_t const* p);

namespace test {

inline static float measure(clock_t cl, int count) {
  return static_cast<float>(count) / (static_cast<float>(clock() - cl) / CLOCKS_PER_SEC);
}

struct person_s {
  int age;
  int validate;
};

struct person_avl_s : avlnode_t, person_s {};

static int avl_cmp(avlnode_t const* l, avlnode_t const* r) {
  auto lp = static_cast<person_avl_s const*>(l);
  auto rp = static_cast<person_avl_s const*>(r);
  return lp->age - rp->age;
}

constexpr int CYCLE = 10;
constexpr int COUNT = 50000;
static size_t DATASIZE = 0;

static void avl_perf(float&);
static void std_perf(float&);
static void std_perf_pool(float&);
static void std_perf_shared(float&);
} // namespace test

//--------------------------------------------------------------------------

extern "C" void mu_test_perf() {
  using namespace test;

  const size_t isize = mpool_calc_required_size(sizeof(person_avl_s), COUNT);
  const size_t tsize = mpool_calc_required_size(sizeof(avltree_t), 1);
  DATASIZE = (isize + tsize) * 3;

  printf("data size: %zu bytes\n", DATASIZE);
  printf("objects  : %d\n", COUNT);
  printf("operations: %d\n", COUNT * CYCLE);

  float avl_tm = 1, ipc_tm = 1, std_tm = 1, stdp_tm = 1;

  avl_perf(avl_tm);
  std_perf_shared(ipc_tm);
  std_perf(std_tm);
  std_perf_pool(stdp_tm);

  printf("AVL    : %d ops/sec\n", (int) avl_tm);
  printf("IPC    : %d ops/sec  (%.2fx)\n", (int) ipc_tm, avl_tm / ipc_tm);
  printf("STD    : %d ops/sec  (%.2fx)\n", (int) std_tm, avl_tm / std_tm);
  printf("STD+POOL: %d ops/sec (%.2fx)\n", (int) stdp_tm, avl_tm / stdp_tm);
}

//--------------------------------------------------------------------------

void test::avl_perf(float& r) {
  mshm_t* shma = mshm_create("mitosha", DATASIZE);
  mu_ensure(shma);

  void* mem = mshm_memory_ptr(shma);
  mpool_t* pool = mpool_format_memory(mem, mshm_memory_size(shma));
  mu_ensure(pool);

  mpool_dump(pool);
  avltree_t* set = static_cast<avltree_t*>(mpool_alloc(pool, sizeof(*set)));
  avltree_init(set);

  // fill
  for (int i = 0; i < COUNT; ++i) {
    person_avl_s* p = static_cast<person_avl_s*>(mpool_alloc(pool, sizeof(*p)));
    mu_check(p);
    p->age = i;
    p->validate = i + 1;
    avltree_insert(p, avl_cmp, set);
  }

  mpool_dump(pool);

  clock_t cl = clock();

  for (int j = 0; j < CYCLE; ++j) {
    person_avl_s p;

    for (p.age = 0; p.age < COUNT; ++p.age) {
      auto* n = static_cast<person_avl_s*>(avltree_lookup(&p, avl_cmp, set));
      mu_ensure(n);

      person_avl_s v = *n;
      avltree_remove(n, set);
      mpool_free(pool, n);

      n = static_cast<person_avl_s*>(mpool_alloc(pool, sizeof(*n)));
      *n = v;

      avltree_insert(n, avl_cmp, set);
      n = static_cast<person_avl_s*>(avltree_lookup(&p, avl_cmp, set));
      mu_ensure(n);
      mu_check(n->validate == p.age + 1);
    }
  }

  r = measure(cl, COUNT * CYCLE);
  mshm_cleanup(shma);
}

//--------------------------------------------------------------------------

#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <set>

namespace std {
template <> struct less<test::person_s> {
  bool operator()(test::person_s const& lhs, test::person_s const& rhs) const { return lhs.age < rhs.age; }
};
} // namespace std

namespace test {
template <typename Container> void do_test(Container& set, float& r) {
  person_s p;

  for (p.age = 0; p.age < COUNT; ++p.age) {
    p.validate = p.age + 1;
    set.insert(p);
  }

  clock_t cl = clock();

  for (int j = 0; j < CYCLE; ++j) {
    for (p.age = 0; p.age < COUNT; ++p.age) {
      auto it = set.find(p);
      mu_ensure(it != set.end());

      person_s v = *it;
      set.erase(it);
      set.insert(v);

      it = set.find(p);
      mu_check(it->validate == p.age + 1);
    }
  }

  r = measure(cl, COUNT * CYCLE);
}
} // namespace test

//--------------------------------------------------------------------------

void test::std_perf(float& r) {
  using test_set_t = std::set<person_s>;
  test_set_t set;
  do_test(set, r);
}

void test::std_perf_pool(float& r) {
  using test_alloc_t = boost::fast_pool_allocator<person_s>;
  using test_set_t = std::set<person_s, std::less<person_s>, test_alloc_t>;
  test_set_t set;
  do_test(set, r);
}

//--------------------------------------------------------------------------

#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/set.hpp>

void test::std_perf_shared(float& r) {
  namespace ipc = boost::interprocess;
  using shared_memory =
      ipc::basic_managed_shared_memory<char, ipc::rbtree_best_fit<ipc::null_mutex_family>, ipc::iset_index>;
  using test_alloc_t = ipc::allocator<person_s, shared_memory::segment_manager>;
  using test_set_t = ipc::set<person_s, std::less<person_s>, test_alloc_t>;

  char memname[] = "/mitosha_ipc";
  ipc::shared_memory_object::remove(memname);

  {
    shared_memory mem(ipc::open_or_create, memname, DATASIZE);
    test_alloc_t alloc(mem.get_segment_manager());
    test_set_t* s = mem.find_or_construct<test_set_t>("SET")(std::less<person_s>(), alloc);
    mu_ensure(s);

    do_test(*s, r);

    printf("> IPC size: %zu used: %zu free: %zu\n", mem.get_size(), mem.get_size() - mem.get_free_memory(),
           mem.get_free_memory());
  }

  ipc::shared_memory_object::remove(memname);
}
