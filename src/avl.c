/*
 * based on libtree code:
 * https://github.com/fbuihuu/libtree
 */
#include <mitosha.h>
#include <assert.h>
#include <stdio.h>

static inline int is_root_avl(avlnode_t* node) {
  return NULL == mvoid_get(&node->parent);
}

static inline void INIT_NODE_AVL(avlnode_t* node) {
  mvoid_set(&node->left, NULL);
  mvoid_set(&node->right, NULL);
  mvoid_set(&node->parent, NULL);
  node->balance = 0;
  ;
}

static inline signed get_balance(avlnode_t* node) {
  return node->balance;
}

static inline void set_balance(int balance, avlnode_t* node) {
  node->balance = balance;
}

static inline int inc_balance(avlnode_t* node) {
  return ++node->balance;
}

static inline int dec_balance(avlnode_t* node) {
  return --node->balance;
}

static inline avlnode_t* get_parent_avl(const avlnode_t* node) {
  return mvoid_get(&node->parent);
}

static inline void set_parent_avl(avlnode_t* parent, avlnode_t* node) {
  mvoid_set(&node->parent, parent);
}

/*
 * Iterators
 */
static inline avlnode_t* get_first_avl(avlnode_t* node) {
  while (mvoid_get(&node->left))
    node = mvoid_get(&node->left);
  return node;
}

static inline avlnode_t* get_last_avl(avlnode_t* node) {
  while (mvoid_get(&node->right))
    node = mvoid_get(&node->right);
  return node;
}

avlnode_t* avltree_first(const avltree_t* tree) {
  return mvoid_get(&tree->first);
}

avlnode_t* avltree_last(const avltree_t* tree) {
  return mvoid_get(&tree->last);
}

avlnode_t* avltree_next(const avlnode_t* node) {
  avlnode_t* r;

  if (mvoid_get(&node->right))
    return get_first_avl(mvoid_get(&node->right));

  while ((r = get_parent_avl(node)) && (mvoid_get(&r->right) == node)) {
    node = r;
  }

  return r;
}

avlnode_t* avltree_prev(const avlnode_t* node) {
  avlnode_t* r;

  if (mvoid_get(&node->left))
    return get_last_avl(mvoid_get(&node->left));

  while ((r = get_parent_avl(node)) && (mvoid_get(&r->left) == node)) {
    node = r;
  }
  return r;
}

/*
 * The AVL tree is more rigidly balanced than Red-Black trees, leading
 * to slower insertion and removal but faster retrieval.
 */

/* node->balance = height(node->right) - height(node->left); */
static void rotate_left_avl(avlnode_t* node, avltree_t* tree) {
  avlnode_t* p = node;
  avlnode_t* q = mvoid_get(&node->right); /* can't be NULL */
  avlnode_t* parent = get_parent_avl(p);

  if (!is_root_avl(p)) {

    if (mvoid_get(&parent->left) == p)
      mvoid_set(&parent->left, q);
    else
      mvoid_set(&parent->right, q);

  } else {
    mvoid_set(&tree->root, q);
  }

  set_parent_avl(parent, q);
  set_parent_avl(q, p);

  mvoid_set(&p->right, mvoid_get(&q->left));

  if (mvoid_get(&p->right))
    set_parent_avl(p, mvoid_get(&p->right));

  mvoid_set(&q->left, p);
}

static void rotate_right_avl(avlnode_t* node, avltree_t* tree) {
  avlnode_t* p = node;
  avlnode_t* q = mvoid_get(&node->left); /* can't be NULL */
  avlnode_t* parent = get_parent_avl(p);

  if (!is_root_avl(p)) {

    if (mvoid_get(&parent->left) == p)
      mvoid_set(&parent->left, q);
    else
      mvoid_set(&parent->right, q);

  } else {
    mvoid_set(&tree->root, q);
  }

  set_parent_avl(parent, q);
  set_parent_avl(q, p);

  mvoid_set(&p->left, mvoid_get(&q->right));

  if (mvoid_get(&p->left))
    set_parent_avl(p, mvoid_get(&p->left));

  mvoid_set(&q->right, p);
}

/*
 * 'pparent', 'unbalanced' and 'is_left' are only used for
 * insertions. Normally GCC will notice this and get rid of them for
 * lookups.
 */
static inline avlnode_t* do_lookup_avl(const avlnode_t* key, avltree_compare_f cmp, const avltree_t* tree,
                                       avlnode_t** pparent, avlnode_t** unbalanced, int* is_left) {
  avlnode_t* node = mvoid_get(&tree->root);
  int res = 0;

  *pparent = NULL;
  *unbalanced = node;
  *is_left = 0;

  while (node) {
    if (get_balance(node) != 0)
      *unbalanced = node;

    res = cmp(node, key);
    if (res == 0)
      return node;
    *pparent = node;
    if ((*is_left = res > 0))
      node = mvoid_get(&node->left);
    else
      node = mvoid_get(&node->right);
  }
  return NULL;
}

avlnode_t* avltree_lookup(const avlnode_t* key, avltree_compare_f cmp, const avltree_t* tree) {
  avlnode_t *parent, *unbalanced;
  int is_left;

  return do_lookup_avl(key, cmp, tree, &parent, &unbalanced, &is_left);
}

avlnode_t* avltree_lower(const avlnode_t* key, avltree_compare_f cmp, const avltree_t* tree) {
  int rc = 0;
  avlnode_t* node = mvoid_get(&tree->root);
  avlnode_t* prev = NULL;

  while (node) {
    rc = cmp(node, key);
    if (0 == rc)
      return node;
    if (rc > 0) {
      prev = node;
      node = mvoid_get(&node->left);
    } else
      node = mvoid_get(&node->right);
  }
  return prev;
}

avlnode_t* avltree_upper(const avlnode_t* key, avltree_compare_f cmp, const avltree_t* tree) {
  avlnode_t* r = avltree_lower(key, cmp, tree);
  while (r && 0 == cmp(r, key))
    r = avltree_next(r);
  return r;
}

inline static void set_child_avl(avlnode_t* child, avlnode_t* node, int left) {
  if (left)
    mvoid_set(&node->left, child);
  else
    mvoid_set(&node->right, child);
}

/* Insertion never needs more than 2 rotations */
avlnode_t* avltree_insert(avlnode_t* node, avltree_compare_f cmp, avltree_t* tree) {
  avlnode_t *key, *parent, *unbalanced;
  int is_left;

  key = do_lookup_avl(node, cmp, tree, &parent, &unbalanced, &is_left);

  if (key)
    return key;

  INIT_NODE_AVL(node);

  if (!parent) {

    mvoid_set(&tree->root, node);
    mvoid_set(&tree->first, node);
    mvoid_set(&tree->last, node);
    tree->height++;
    return NULL;
  }

  if (is_left) {

    if (mvoid_get(&tree->first) == parent)
      mvoid_set(&tree->first, node);

  } else {

    if (mvoid_get(&tree->last) == parent)
      mvoid_set(&tree->last, node);
  }

  set_parent_avl(parent, node);
  set_child_avl(node, parent, is_left);

  for (;;) {

    if (mvoid_get(&parent->left) == node)
      dec_balance(parent);

    else
      inc_balance(parent);

    if (parent == unbalanced)
      break;

    node = parent;
    parent = get_parent_avl(parent);
  }

  switch (get_balance(unbalanced)) {

  case 1:
  case -1:
    tree->height++;
    /* fall through */
  case 0:
    break;
  case 2: {
    avlnode_t* right = mvoid_get(&unbalanced->right);

    if (get_balance(right) == 1) {

      set_balance(0, unbalanced);
      set_balance(0, right);
    } else {

      switch (get_balance(mvoid_get(&right->left))) {
      case 1:
        set_balance(-1, unbalanced);
        set_balance(0, right);
        break;
      case 0:
        set_balance(0, unbalanced);
        set_balance(0, right);
        break;
      case -1:
        set_balance(0, unbalanced);
        set_balance(1, right);
        break;
      }
      set_balance(0, mvoid_get(&right->left));
      rotate_right_avl(right, tree);
    }
    rotate_left_avl(unbalanced, tree);
    break;
  }

  case -2: {
    avlnode_t* left = mvoid_get(&unbalanced->left);

    if (get_balance(left) == -1) {
      set_balance(0, unbalanced);
      set_balance(0, left);
    } else {
      switch (get_balance(mvoid_get(&left->right))) {
      case 1:
        set_balance(0, unbalanced);
        set_balance(-1, left);
        break;
      case 0:
        set_balance(0, unbalanced);
        set_balance(0, left);
        break;
      case -1:
        set_balance(1, unbalanced);
        set_balance(0, left);
        break;
      }
      set_balance(0, mvoid_get(&left->right));

      rotate_left_avl(left, tree);
    }

    rotate_right_avl(unbalanced, tree);
    break;
  }
  }
  return NULL;
}

/* Deletion might require up to log(n) rotations */
void avltree_remove(avlnode_t* node, avltree_t* tree) {
  avlnode_t* parent = get_parent_avl(node);
  avlnode_t* left = mvoid_get(&node->left);
  avlnode_t* right = mvoid_get(&node->right);
  avlnode_t* next;
  int is_left = 0;

  if (node == mvoid_get(&tree->first))
    mvoid_set(&tree->first, avltree_next(node));
  if (node == mvoid_get(&tree->last))
    mvoid_set(&tree->last, avltree_prev(node));

  if (!left)
    next = right;
  else if (!right)
    next = left;
  else
    next = get_first_avl(right);

  if (parent) {
    is_left = mvoid_get(&parent->left) == node;
    set_child_avl(next, parent, is_left);
  } else
    mvoid_set(&tree->root, next);

  if (left && right) {
    set_balance(get_balance(node), next);

    mvoid_set(&next->left, left);
    set_parent_avl(next, left);

    if (next != right) {
      parent = get_parent_avl(next);
      set_parent_avl(get_parent_avl(node), next);

      node = mvoid_get(&next->right);
      mvoid_set(&parent->left, node);
      is_left = 1;

      mvoid_set(&next->right, right);
      set_parent_avl(next, right);
    } else {
      set_parent_avl(parent, next);
      parent = next;
      node = mvoid_get(&parent->right);
      is_left = 0;
    }
    assert(parent != NULL);
  } else
    node = next;

  if (node)
    set_parent_avl(parent, node);

  /*
   * At this point, 'parent' can only be null, if 'node' is the
   * tree's root and has at most one child.
   *
   * case 1: the subtree is now balanced but its height has
   * decreased.
   *
   * case 2: the subtree is mostly balanced and its height is
   * unchanged.
   *
   * case 3: the subtree is unbalanced and its height may have
   * been changed during the rebalancing process, see below.
   *
   * case 3.1: after a left rotation, the subtree becomes mostly
   * balanced and its height is unchanged.
   *
   * case 3.2: after a left rotation, the subtree becomes
   * balanced but its height has decreased.
   *
   * case 3.3: after a left and a right rotation, the subtree
   * becomes balanced or mostly balanced but its height has
   * decreased for all cases.
   */
  while (parent) {
    int balance;
    node = parent;
    parent = get_parent_avl(parent);

    if (is_left) {
      is_left = parent && mvoid_get(&parent->left) == node;

      balance = inc_balance(node);
      if (balance == 0) /* case 1 */
        continue;
      if (balance == 1) /* case 2 */
        return;
      right = mvoid_get(&node->right); /* case 3 */
      switch (get_balance(right)) {
      case 0: /* case 3.1 */
        set_balance(1, node);
        set_balance(-1, right);
        rotate_left_avl(node, tree);
        return;
      case 1: /* case 3.2 */
        set_balance(0, node);
        set_balance(0, right);
        break;
      case -1: /* case 3.3 */
        switch (get_balance(mvoid_get(&right->left))) {
        case 1:
          set_balance(-1, node);
          set_balance(0, right);
          break;
        case 0:
          set_balance(0, node);
          set_balance(0, right);
          break;
        case -1:
          set_balance(0, node);
          set_balance(1, right);
          break;
        }
        set_balance(0, mvoid_get(&right->left));

        rotate_right_avl(right, tree);
      }
      rotate_left_avl(node, tree);
    } else {
      is_left = parent && mvoid_get(&parent->left) == node;

      balance = dec_balance(node);
      if (balance == 0)
        continue;
      if (balance == -1)
        return;
      left = mvoid_get(&node->left);
      switch (get_balance(left)) {
      case 0:
        set_balance(-1, node);
        set_balance(1, left);
        rotate_right_avl(node, tree);
        return;
      case -1:
        set_balance(0, node);
        set_balance(0, left);
        break;
      case 1:
        switch (get_balance(mvoid_get(&left->right))) {
        case 1:
          set_balance(0, node);
          set_balance(-1, left);
          break;
        case 0:
          set_balance(0, node);
          set_balance(0, left);
          break;
        case -1:
          set_balance(1, node);
          set_balance(0, left);
          break;
        }
        set_balance(0, mvoid_get(&left->right));

        rotate_left_avl(left, tree);
      }
      rotate_right_avl(node, tree);
    }
  }
  tree->height--;
}

void avltree_replace(avlnode_t* old, avlnode_t* n, avltree_t* tree) {
  avlnode_t* parent = get_parent_avl(old);

  if (parent) {
    set_child_avl(n, parent, mvoid_get(&parent->left) == old);
  } else {
    mvoid_set(&tree->root, n);
  }

  if (mvoid_get(&old->left))
    set_parent_avl(n, mvoid_get(&old->left));

  if (mvoid_get(&old->right))
    set_parent_avl(n, mvoid_get(&old->right));

  if (mvoid_get(&old->left))
    set_parent_avl(n, mvoid_get(&old->left));
  if (mvoid_get(&old->right))
    set_parent_avl(n, mvoid_get(&old->right));

  if (mvoid_get(&tree->first) == old)
    mvoid_set(&tree->first, n);
  if (mvoid_get(&tree->last) == old)
    mvoid_set(&tree->last, n);

  n->balance = old->balance;
  mvoid_set(&n->parent, mvoid_get(&old->parent));
  mvoid_set(&n->left, mvoid_get(&old->left));
  mvoid_set(&n->right, mvoid_get(&old->right));
}

int avltree_init(avltree_t* tree) {
  mvoid_set(&tree->root, NULL);
  tree->height = -1;
  mvoid_set(&tree->first, NULL);
  mvoid_set(&tree->last, NULL);
  return 0;
}
