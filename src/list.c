#include <mitosha.h>
#include <assert.h>

static inline void NODE_INIT(listnode_t* node) {
  mvoid_set(&node->next, NULL);
  mvoid_set(&node->prev, NULL);
}

listnode_t* list_lookup(listnode_t const* key, list_compare_f cmp, list_t const* list) {
  listnode_t* node = mvoid_get(&list->first);
  while (node && cmp(node, key))
    node = mvoid_get(&node->next);
  return node;
}

listnode_t* list_front(list_t const* list) {
  return mvoid_get(&list->first);
}

listnode_t* list_back(list_t const* list) {
  return mvoid_get(&list->last);
}

listnode_t* list_first(listnode_t const* node) {
  while (node && mvoid_get(&node->prev))
    node = mvoid_get(&node->prev);
  return (listnode_t*) node;
}

listnode_t* list_last(listnode_t const* node) {
  while (node && mvoid_get(&node->next))
    node = mvoid_get(&node->next);
  return (listnode_t*) node;
}

listnode_t* list_next(listnode_t const* node) {
  return mvoid_get(&node->next);
}

listnode_t* list_prev(listnode_t const* node) {
  return mvoid_get(&node->prev);
}

void list_insert_befor(listnode_t* where, listnode_t* node, list_t* list) {
  NODE_INIT(node);

  listnode_t* prev = list_prev(where);
  if (prev) {
    mvoid_set(&prev->next, node);
    mvoid_set(&node->prev, prev);
  }
  mvoid_set(&where->prev, node);
  mvoid_set(&node->next, where);

  if (where == mvoid_get(&list->first))
    mvoid_set(&list->first, node);
}

void list_insert_after(listnode_t* where, listnode_t* node, list_t* list) {
  NODE_INIT(node);

  listnode_t* next = list_next(where);

  if (next) {
    mvoid_set(&next->prev, node);
    mvoid_set(&node->next, next);
  }

  mvoid_set(&where->next, node);
  mvoid_set(&node->prev, where);

  if (where == mvoid_get(&list->last))
    mvoid_set(&list->last, node);
}

void list_push_back(listnode_t* node, list_t* list) {
  if (mvoid_get(&list->last))
    list_insert_after(mvoid_get(&list->last), node, list);
  else
    list_push_front(node, list);
}

void list_push_front(listnode_t* node, list_t* list) {
  if (mvoid_get(&list->first)) {
    list_insert_befor(mvoid_get(&list->first), node, list);
  } else {
    NODE_INIT(node);
    mvoid_set(&list->first, node);
    mvoid_set(&list->last, node);
  }
}

void list_remove(listnode_t* node, list_t* list) {
  listnode_t* prev = list_prev(node);
  listnode_t* next = list_next(node);

  if (prev)
    mvoid_set(&prev->next, next);
  if (next)
    mvoid_set(&next->prev, prev);

  if (mvoid_get(&list->first) == node)
    mvoid_set(&list->first, next);
  if (mvoid_get(&list->last) == node)
    mvoid_set(&list->last, prev);

  NODE_INIT(node);
}

void list_replace(listnode_t* old, listnode_t* node, list_t* list) {
  NODE_INIT(node);

  listnode_t* prev = list_prev(old);
  listnode_t* next = list_next(old);

  if (prev) {
    mvoid_set(&prev->next, node);
    mvoid_set(&node->prev, prev);
  }

  if (next) {
    mvoid_set(&next->prev, node);
    mvoid_set(&node->next, next);
  }

  if (mvoid_get(&list->first) == old)
    mvoid_set(&list->first, node);
  if (mvoid_get(&list->last) == old)
    mvoid_set(&list->last, node);

  NODE_INIT(old);
}

void list_swap(listnode_t* node1, listnode_t* node2, list_t* list) {
  listnode_t* p1 = list_prev(node1);
  listnode_t* n1 = list_next(node1);

  listnode_t* p2 = list_prev(node2);
  listnode_t* n2 = list_next(node2);

  if (n1 == node2) {
    if (p1)
      mvoid_set(&p1->next, node2);
    mvoid_set(&node2->prev, p1);
    mvoid_set(&node2->next, node1);
    mvoid_set(&node1->prev, node2);
    mvoid_set(&node1->next, n2);
    if (n2)
      mvoid_set(&n2->prev, node1);
  } else if (p1 == node2) {
    if (p2)
      mvoid_set(&p2->next, node1);
    mvoid_set(&node1->prev, p2);
    mvoid_set(&node1->next, node2);
    mvoid_set(&node2->prev, node1);
    mvoid_set(&node2->next, n1);
    if (n1)
      mvoid_set(&n1->prev, node2);
  } else {
    if (p1)
      mvoid_set(&p1->next, node2);
    mvoid_set(&node2->prev, p1);
    mvoid_set(&node2->next, n1);
    if (n1)
      mvoid_set(&n1->prev, node2);

    if (p2)
      mvoid_set(&p2->next, node1);
    mvoid_set(&node1->prev, p2);
    mvoid_set(&node1->next, n2);
    if (n2)
      mvoid_set(&n2->prev, node1);
  }

  if (mvoid_get(&list->first) == node1)
    mvoid_set(&list->first, node2);
  else if (mvoid_get(&list->first) == node2)
    mvoid_set(&list->first, node1);

  if (mvoid_get(&list->last) == node1)
    mvoid_set(&list->last, node2);
  else if (mvoid_get(&list->last) == node2)
    mvoid_set(&list->last, node1);
}

void list_sort(list_t* list, list_compare_f cmp) {
  listnode_t* first = mvoid_get(&list->first);
  listnode_t *p, *q, *e, *tail;
  int insize, nmerges, psize, qsize, i;

  insize = 1;

  while (1) {

    if (!first)
      break;

    p = first;

    first = NULL;
    tail = NULL;

    nmerges = 0; /* count number of merges we do in this pass */

    while (p) {

      nmerges++; /* there exists a merge to be done */
      /* step `insize' places along from p */
      q = p;
      psize = 0;
      for (i = 0; i < insize; i++) {
        psize++;
        q = mvoid_get(&q->next);
        if (!q)
          break;
      }

      /* if q hasn't fallen off end, we have two lists to merge */
      qsize = insize;

      /* now we have two lists; merge them */
      while (psize > 0 || (qsize > 0 && q)) {

        /* decide whether next element of merge comes from p or q */
        if (psize == 0) {
          /* p is empty; e must come from q. */
          e = q;
          q = mvoid_get(&q->next);
          qsize--;
        } else if (qsize == 0 || !q) {
          /* q is empty; e must come from p. */
          e = p;
          p = mvoid_get(&p->next);
          psize--;
        } else if (cmp(p, q) <= 0) {
          /* First element of p is lower (or same);
           * e must come from p. */
          e = p;
          p = mvoid_get(&p->next);
          psize--;
        } else {
          /* First element of q is lower; e must come from q. */
          e = q;
          q = mvoid_get(&q->next);
          qsize--;
        }

        /* add the next element to the merged list */
        if (tail) {
          mvoid_set(&tail->next, e);
        } else {
          first = e;
        }

        /* Maintain reverse pointers in a doubly linked list. */
        mvoid_set(&e->prev, tail);

        tail = e;
      }

      /* now p has stepped `insize' places along, and q has too */
      p = q;
    } // while (p)

    mvoid_set(&tail->next, NULL);

    /* If we have done only one merge, we're finished. */
    if (nmerges <= 1) /* allow for nmerges==0, the empty list case */
      break;

    /* Otherwise repeat, merging lists twice the size */
    insize *= 2;

  } // while (1)

  mvoid_set(&list->first, first);
  mvoid_set(&list->last, list_last(first));
}

int list_init(list_t* list) {
  mvoid_set(&list->first, NULL);
  mvoid_set(&list->last, NULL);
  return 0;
}
