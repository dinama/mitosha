#include <mutest.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <mitosha.h>

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

typedef struct {
  avlnode_t node;
  int v;
} value_t;

static int value_cmp(avlnode_t const* a, avlnode_t const* b) {
  value_t const* pa = (value_t*) a;
  value_t const* pb = (value_t*) b;
  printf(" a:%d b:%d\n", pa->v, pb->v);
  return pa->v - pb->v;
}

static void tree_dump(avltree_t* tree) {
  value_t* val = mcontainer_of(mvoid_get(&tree->root), value_t, node);
  printf("root -> %d\n\t", val->v);
  for (avlnode_t* no = avltree_first(tree); no; no = avltree_next(no)) {
    val = mcontainer_of(no, value_t, node);
    printf("%d -> ", val->v);
  }
  printf("\n");
}

void mu_test_avltree_operations() {
  avltree_t tree;
  ;
  avltree_init(&tree);

  value_t v1 = {{}, 1};
  value_t v2 = {{}, 2};
  value_t v3 = {{}, 3};
  value_t v4 = {{}, 4};

  avlnode_t* r;
  r = avltree_insert(&v1.node, value_cmp, &tree);
  mu_ensure(!r);
  r = avltree_insert(&v4.node, value_cmp, &tree);
  mu_ensure(!r);
  r = avltree_insert(&v2.node, value_cmp, &tree);
  mu_ensure(!r);

  value_t p = {{}, 4};
  r = avltree_lookup(&p.node, value_cmp, &tree);
  mu_ensure(r == &v4.node);

  p.v = 2;
  r = avltree_lookup(&p.node, value_cmp, &tree);
  mu_ensure(r == &v2.node);

  avltree_remove(r, &tree);
  r = avltree_lookup(&p.node, value_cmp, &tree);
  mu_ensure(r == NULL);

  r = avltree_insert(&v3.node, value_cmp, &tree);
  mu_ensure(!r);
  p.v = 3;
  r = avltree_lookup(&p.node, value_cmp, &tree);
  mu_ensure(r == &v3.node);

  r = avltree_insert(&v2.node, value_cmp, &tree);
  mu_check(!r);

  p.v = 2;
  r = avltree_lookup(&p.node, value_cmp, &tree);
  mu_check(r == &v2.node);
}

/*-------------------------------------------------------------------------*/

void mu_test_avltree_first() {
  value_t v1 = {{}, 1};
  value_t v2 = {{}, 2};
  value_t v3 = {{}, 3};
  value_t v4 = {{}, 4};

  avltree_t tree;
  avltree_init(&tree);

  avltree_insert(&v3.node, value_cmp, &tree);
  avltree_insert(&v2.node, value_cmp, &tree);
  avltree_insert(&v4.node, value_cmp, &tree);
  avltree_insert(&v1.node, value_cmp, &tree);

  avlnode_t* r = avltree_first(&tree);
  mu_ensure(r);
  mu_check(&v1 == mcontainer_of(r, value_t, node));
}

void mu_test_avltree_last() {
  value_t v1 = {{}, 1};
  value_t v2 = {{}, 2};
  value_t v3 = {{}, 3};
  value_t v4 = {{}, 4};

  avltree_t tree;
  avltree_init(&tree);

  avltree_insert(&v3.node, value_cmp, &tree);
  avltree_insert(&v2.node, value_cmp, &tree);
  avltree_insert(&v4.node, value_cmp, &tree);
  avltree_insert(&v1.node, value_cmp, &tree);

  avlnode_t* r = avltree_last(&tree);
  mu_ensure(r);
  mu_check(&v4 == mcontainer_of(r, value_t, node));
}

void mu_test_avltree_next() {
  value_t v1 = {{}, 1};
  value_t v2 = {{}, 2};
  value_t v3 = {{}, 3};
  value_t v4 = {{}, 4};

  avltree_t tree;
  avltree_init(&tree);

  avltree_insert(&v3.node, value_cmp, &tree);
  avltree_insert(&v2.node, value_cmp, &tree);
  avltree_insert(&v4.node, value_cmp, &tree);
  avltree_insert(&v1.node, value_cmp, &tree);

  avlnode_t* r = avltree_first(&tree);
  mu_ensure(r);
  mu_check(&v1 == mcontainer_of(r, value_t, node));

  r = avltree_next(&v1.node);
  mu_ensure(r);
  mu_check(&v2 == mcontainer_of(r, value_t, node));

  r = avltree_next(r);
  mu_ensure(r);
  mu_check(&v3 == mcontainer_of(r, value_t, node));

  r = avltree_next(r);
  mu_ensure(r);
  mu_check(&v4 == mcontainer_of(r, value_t, node));

  r = avltree_next(r);
  mu_check(!r);
}

void mu_test_avltree_prev() {
  value_t v1 = {{}, 1};
  value_t v2 = {{}, 2};
  value_t v3 = {{}, 3};
  value_t v4 = {{}, 4};

  avltree_t tree;
  avltree_init(&tree);

  avltree_insert(&v4.node, value_cmp, &tree);
  avltree_insert(&v2.node, value_cmp, &tree);
  avltree_insert(&v3.node, value_cmp, &tree);
  avltree_insert(&v1.node, value_cmp, &tree);

  avlnode_t* r = avltree_last(&tree);
  mu_ensure(r);
  mu_check(&v4 == mcontainer_of(r, value_t, node));

  r = avltree_prev(r);
  mu_ensure(r);
  mu_check(&v3 == mcontainer_of(r, value_t, node));

  r = avltree_prev(r);
  mu_ensure(r);
  mu_check(&v2 == mcontainer_of(r, value_t, node));

  r = avltree_prev(r);
  mu_ensure(r);
  mu_check(&v1 == mcontainer_of(r, value_t, node));

  r = avltree_prev(r);
  mu_check(!r);
}

void mu_test_avltree_lookup() {
  value_t v1 = {{}, 1};
  value_t v2 = {{}, 2};
  value_t v3 = {{}, 3};
  value_t v4 = {{}, 4};

  avltree_t tree;
  avltree_init(&tree);

  avltree_insert(&v4.node, value_cmp, &tree);
  avltree_insert(&v2.node, value_cmp, &tree);
  avltree_insert(&v3.node, value_cmp, &tree);
  avltree_insert(&v1.node, value_cmp, &tree);

  value_t key;
  key.v = 3;
  avlnode_t* r = avltree_lookup(&key.node, value_cmp, &tree);
  mu_ensure(r);
  mu_check(&v3 == mcontainer_of(r, value_t, node));

  key.v = 2;
  r = avltree_lookup(&key.node, value_cmp, &tree);
  mu_ensure(r);
  mu_check(&v2 == mcontainer_of(r, value_t, node));
}

void mu_test_avltree_lower() {
  value_t v1 = {{}, -1};
  value_t v2 = {{}, 3};
  value_t v3 = {{}, 12};
  value_t v4 = {{}, 13};
  value_t v5 = {{}, 24};
  value_t v6 = {{}, 47};
  value_t v7 = {{}, 600};

  avltree_t tree;
  avltree_init(&tree);

  avltree_insert(&v4.node, value_cmp, &tree);
  avltree_insert(&v2.node, value_cmp, &tree);
  avltree_insert(&v3.node, value_cmp, &tree);
  avltree_insert(&v1.node, value_cmp, &tree);
  avltree_insert(&v5.node, value_cmp, &tree);
  avltree_insert(&v6.node, value_cmp, &tree);
  avltree_insert(&v7.node, value_cmp, &tree);

  // tree_dump(&tree);

  value_t* rv = NULL;
  value_t key;
  key.v = -2;
  avlnode_t* r = avltree_lower(&key.node, value_cmp, &tree);
  mu_ensure(r);
  rv = mcontainer_of(r, value_t, node);
  mu_check(&v1 == rv);

  key.v = -1;
  r = avltree_lower(&key.node, value_cmp, &tree);
  mu_ensure(r);
  rv = mcontainer_of(r, value_t, node);
  mu_check(&v1 == rv);

  key.v = 4;
  r = avltree_lower(&key.node, value_cmp, &tree);
  mu_ensure(r);
  rv = mcontainer_of(r, value_t, node);
  mu_ensure(&v3 == rv);

  key.v = 12;
  r = avltree_lower(&key.node, value_cmp, &tree);
  mu_ensure(r);
  rv = mcontainer_of(r, value_t, node);
  mu_check(&v3 == rv);

  key.v = 600;
  r = avltree_lower(&key.node, value_cmp, &tree);
  mu_ensure(r);
  rv = mcontainer_of(r, value_t, node);
  mu_check(&v7 == rv);

  key.v = 601;
  r = avltree_lower(&key.node, value_cmp, &tree);
  mu_check(!r);
}

void mu_test_avltree_upper() {
  value_t v1 = {{}, -1};
  value_t v2 = {{}, 3};
  value_t v3 = {{}, 12};
  value_t v4 = {{}, 13};
  value_t v5 = {{}, 24};
  value_t v6 = {{}, 47};
  value_t v7 = {{}, 600};

  avltree_t tree;
  avltree_init(&tree);

  avltree_insert(&v4.node, value_cmp, &tree);
  avltree_insert(&v2.node, value_cmp, &tree);
  avltree_insert(&v3.node, value_cmp, &tree);
  avltree_insert(&v1.node, value_cmp, &tree);
  avltree_insert(&v5.node, value_cmp, &tree);
  avltree_insert(&v6.node, value_cmp, &tree);
  avltree_insert(&v7.node, value_cmp, &tree);

  tree_dump(&tree);

  value_t key;
  key.v = -2;
  avlnode_t* r = avltree_upper(&key.node, value_cmp, &tree);
  mu_ensure(r);
  value_t const* rv = mcontainer_of(r, value_t, node);
  mu_check(&v1 == rv);

  key.v = -1;
  r = avltree_upper(&key.node, value_cmp, &tree);
  mu_ensure(r);
  rv = mcontainer_of(r, value_t, node);
  mu_check(&v2 == rv);

  key.v = 4;
  r = avltree_upper(&key.node, value_cmp, &tree);
  mu_ensure(r);
  rv = mcontainer_of(r, value_t, node);
  mu_check(&v3 == rv);

  key.v = 600;
  r = avltree_upper(&key.node, value_cmp, &tree);
  mu_check(!r);

  key.v = 601;
  r = avltree_upper(&key.node, value_cmp, &tree);
  mu_check(!r);
}

void mu_test_avltree_insert() {
  ;
}

void mu_test_avltree_remove() {
  value_t v1 = {{}, 1};
  value_t v2 = {{}, 2};
  value_t v3 = {{}, 3};
  value_t v4 = {{}, 4};

  avltree_t tree;
  avltree_init(&tree);

  avltree_insert(&v3.node, value_cmp, &tree);
  avltree_insert(&v2.node, value_cmp, &tree);
  avltree_insert(&v4.node, value_cmp, &tree);
  avltree_insert(&v1.node, value_cmp, &tree);

  avltree_remove(&v2.node, &tree);
  mu_check(avltree_next(&v1.node) == &v3.node);
  mu_check(avltree_prev(&v3.node) == &v1.node);
}

void mu_test_avltree_replace() {
  value_t v1 = {{}, 1};
  value_t v2 = {{}, 2};
  value_t v3 = {{}, 3};
  value_t v4 = {{}, 4};

  avltree_t tree;
  avltree_init(&tree);

  avltree_insert(&v4.node, value_cmp, &tree);
  avltree_insert(&v2.node, value_cmp, &tree);
  avltree_insert(&v1.node, value_cmp, &tree);

  avltree_replace(&v2.node, &v3.node, &tree);

  mu_check(avltree_next(&v1.node) == &v3.node);
  mu_check(avltree_prev(&v4.node) == &v3.node);
}

/*-------------------------------------------------------------------------*/

static struct test_case_t {
  long age;
  char const* name;
} test_data[] = {

    {3, "1111aaaa"}, {2, "1111bbbb"}, {1, "1111cccc"}, {4, "2222aaaa"},
    {5, "2222bbbb"}, {6, "2222cccc"}

};

/*-------------------------------------------------------------------------*/

typedef struct {
  long age;
  char const* name;
  avlnode_t avl_age;
} person_t;

static int by_age(avlnode_t const* a, avlnode_t const* b) {
  person_t const* pa = mcontainer_of(a, person_t, avl_age);
  person_t const* pb = mcontainer_of(b, person_t, avl_age);
  return pa->age - pb->age;
}

/*-------------------------------------------------------------------------*/

static void* memory = NULL;

static void prepare() {

  avltree_t* iage = (avltree_t*) malloc(sizeof(avltree_t));
  avltree_init(iage);

  const int count = sizeof(test_data) / sizeof(struct test_case_t);

  person_t* arr = malloc(count * sizeof(person_t));

  int i;
  for (i = 0; i < count; ++i) {
    arr[i].name = test_data[i].name;
    arr[i].age = test_data[i].age;

    avltree_insert(&arr[i].avl_age, by_age, iage);
  }

  memory = iage;
}

static void clear() {
  free(memory);
}

void mu_test_avltree_example() {

  prepare();
  avltree_t* iage = (avltree_t*) memory;

  puts("by age:");
  for (avlnode_t* node = avltree_first(iage); node; node = avltree_next(node)) {
    person_t* p = mcontainer_of(node, person_t, avl_age);
    printf("\tage: %ld name: %s\n", p->age, p->name);
  }

  puts("by age desc:");
  for (avlnode_t* node = avltree_last(iage); node; node = avltree_prev(node)) {
    person_t* p = mcontainer_of(node, person_t, avl_age);
    printf("\tage: %ld name: %s\n", p->age, p->name);
  }

  clear();
}
