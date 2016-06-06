#include <mutest.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <mitosha.h>

typedef struct {
  listnode_t link;
  int i;
} my_t;

static int my_cmp(listnode_t const* l, listnode_t const* r) {
  my_t const* ml = mcontainer_of(l, my_t, link);
  my_t const* mr = mcontainer_of(r, my_t, link);
  printf("compare: %d %d\n", ml->i, mr->i);
  return ml->i - mr->i;
}

void print(listnode_t const* list) {
  while (list) {
    my_t const* my = mcontainer_of(list, my_t, link);
    printf("%d ->", my->i);
    list = list_next(list);
  }
  printf("\n");
}

void mu_test_list() {
  my_t n1 = {{{0}, {0}}, 1};
  my_t n2 = {{{0}, {0}}, 2};
  my_t n3 = {{{0}, {0}}, 3};
  my_t n4 = {{{0}, {0}}, 4};
  my_t n5 = {{{0}, {0}}, 5};
  my_t n6 = {{{0}, {0}}, 6};

  list_t list;
  list_init(&list);

  // 5 -> 1
  list_push_back(&n5.link, &list);
  list_insert_after(&n5.link, &n1.link, &list);

  mu_check(list_next(&n5.link) == &n1.link);
  mu_check(list_last(&n5.link) == &n1.link);
  mu_check(list_last(&n1.link) == &n1.link);
  mu_check(list_first(&n1.link) == &n5.link);

  // 5 -> 3 -> 1
  list_insert_befor(&n1.link, &n3.link, &list);

  mu_check(list_next(&n5.link) == &n3.link);
  mu_check(list_last(&n5.link) == &n1.link);
  mu_check(list_prev(&n1.link) == &n3.link);
  mu_check(list_first(&n3.link) == &n5.link);

  // 1 -> 3 -> 5
  list_swap(&n5.link, &n1.link, &list);

  mu_check(list_next(&n1.link) == &n3.link);
  mu_check(list_last(&n1.link) == &n5.link);
  mu_check(list_prev(&n5.link) == &n3.link);
  mu_check(list_first(&n3.link) == &n1.link);

  // 5 -> 3 -> 1
  list_swap(&n5.link, &n1.link, &list);

  mu_check(list_next(&n5.link) == &n3.link);
  mu_check(list_last(&n5.link) == &n1.link);
  mu_check(list_prev(&n1.link) == &n3.link);
  mu_check(list_first(&n3.link) == &n5.link);

  // 2 -> 5 -> 3 -> 1
  list_push_front(&n2.link, &list);

  mu_check(!list_prev(&n2.link));
  mu_check(list_last(&n2.link) == &n1.link);
  mu_check(list_prev(&n5.link) == &n2.link);
  mu_check(list_first(&n3.link) == &n2.link);

  // 2 -> 5 -> 3 -> 1 -> 4
  list_push_back(&n4.link, &list);
  print(list_first(&n2.link));

  mu_check(!list_next(&n4.link));
  mu_check(list_last(&n2.link) == &n4.link);
  mu_check(list_prev(&n4.link) == &n1.link);
  mu_check(list_next(&n1.link) == &n4.link);
  mu_check(list_first(&n4.link) == &n2.link);

  // 2 -> 5 -> 1 -> 4
  list_remove(&n3.link, &list);

  mu_check(list_next(&n5.link) == &n1.link);
  mu_check(list_prev(&n1.link) == &n5.link);

  // 2 -> 6 -> 1 -> 4
  list_replace(&n5.link, &n6.link, &list);

  mu_check(list_next(&n6.link) == &n1.link);
  mu_check(list_prev(&n1.link) == &n6.link);
  mu_check(list_prev(&n6.link) == &n2.link);

  // 2 -> 1 -> 6 -> 4
  list_swap(&n6.link, &n1.link, &list);

  mu_check(list_next(&n1.link) == &n6.link);
  mu_check(list_prev(&n6.link) == &n1.link);
  mu_check(list_next(&n6.link) == &n4.link);

  // 2 -> 6 -> 1 -> 4
  list_swap(&n6.link, &n1.link, &list);

  mu_check(list_next(&n6.link) == &n1.link);
  mu_check(list_prev(&n1.link) == &n6.link);
  mu_check(list_next(&n1.link) == &n4.link);
  mu_check(list_prev(&n6.link) == &n2.link);

  mu_check(list_front(&list) == &n2.link);
  mu_check(list_back(&list) == &n4.link);

  puts("pre sort:");
  print(list_first(&n1.link));

  list_sort(&list, my_cmp);
  puts("post sort:");
  print(list_first(&n1.link));
}

void mu_test_sort() {
  my_t n1 = {{{2}, {3}}, 1};
  my_t n2 = {{{10}, {6}}, 2};
  my_t n3 = {{{11}, {34}}, 2};
  my_t n4 = {{{1}, {33}}, 3};

  list_t list;
  list_init(&list);

  // 2 ->  1 -> 3 -> 2
  list_push_back(&n2.link, &list);
  list_push_back(&n1.link, &list);
  list_push_back(&n4.link, &list);
  list_push_back(&n3.link, &list);

  mu_check(list_front(&list) == &n2.link);
  mu_check(list_next(&n2.link) == &n1.link);
  mu_check(list_next(&n1.link) == &n4.link);
  mu_check(list_next(&n4.link) == &n3.link);
  mu_check(list_back(&list) == &n3.link);

  puts("pre sort:");
  print(list_first(&n1.link));

  list_sort(&list, my_cmp);
  mu_check(list_front(&list) == &n1.link);
  mu_check(list_next(&n1.link) == &n2.link);
  mu_check(list_next(&n2.link) == &n3.link);
  mu_check(list_next(&n3.link) == &n4.link);
  mu_check(list_back(&list) == &n4.link);

  puts("post sort:");
  print(list_first(&n1.link));
}
