#ifndef RK_BST_H
# define RK_BST_H
# include "rk_alloc.h"
# include "rk_vec.h"
RK_HEADER_BEGIN

# define Bst(K, V)              bst_##K##_##V
# define BstEntry(K, V)         bst_entry_##K##_##V
# define RK__BstEntryPriv(K, V) RK__bst_entry_##K##_##V

# define RK__BST_F(K, V, pref)  bst_##pref##_##K##_##V
# define bst_init(K, V, ...)    rk_overload(RK__bst_init, K, V, ##__VA_ARGS__)

# define bst_release(self)      RK__bst_release(bst_esize(self), &((self)->_bst))

# define bst_count(self)        ((size_t)((self)->count))
# define bst_is_empty(self)     ((bool)(bst_count(self) == 0))

# define bst_copy(self)                                                        \
    ((typeof(*(self))){                                                        \
        RK_IFALLOC(.alloc = (self)->alloc, ).root                              \
        = (typeof((self)->root))RK__bst_cpy_(                                  \
            bst_esize(self),                                                   \
            ((bst_node*)(self)->root)RK_IFALLOC(, (self)->alloc)),             \
        .count = (self)->count, .search_fun = (self)->search_fun})

# define bst_etype(self) typeof((self)->root->entry)
# define bst_esize(self) sizeof((self)->root->entry)

# define bst_vtype(self) typeof((self)->root->entry.val)
# define bst_ktype(self) typeof((self)->root->entry.key)
# define bst_vsize(self) sizeof((self)->root->entry.val)
# define bst_ksize(self) sizeof((self)->root->entry.key)

# define bst_get(self, key)                                                    \
    ((bst_vtype(self)*)RK__bst_impl_search(offsetof(bst_etype(self), val),     \
                                           (self)->search_fun(self, key)))

# define bst_contains(self, key) (bst_get(self, key) != NULL)

# define RK__bst_insert_m(_ALWAYSINSERT, self, key, value)                     \
    RK__bst_impl_insert(&((self)->_bst), (self)->search_fun(self, key),        \
                        _ALWAYSINSERT, offsetof(bst_etype(self), val),         \
                        bst_esize(self),                                       \
                        (bst_etype(self)[]){{(key), (value)}})

# define bst_set(self, key, value) RK__bst_insert_m(1, self, key, value)

# define bst_add(self, key, value) RK__bst_insert_m(0, self, key, value)

# define bst_extract(self, key, value_outptr)                                  \
    RK__bst_extract(&((self)->_bst), (self)->search_fun(self, key),            \
                    offsetof(bst_etype(self), val), bst_vsize(self),           \
                    bst_esize(self), value_outptr)

# define bst_min(self) ((bst_etype(self)*)RK__bst_min((self)->_bst.root))
# define bst_max(self) ((bst_etype(self)*)RK__bst_max((self)->_bst.root))

# define bst_remove(self, key)                                                 \
    ((void)bst_extract(self, key, (bst_vtype(self)[1]){}))
// make node private

typedef struct bst_node {
  struct bst_node *l, *r;
  alignas_max char entry[];
} bst_node;

typedef struct bst_data {
  RK_IFALLOC(Allocator alloc;)
  size_t    count;
  bst_node* root;
  void (*DUMMY)(struct bst_data*, const void*); /*not used currently*/
} bst_data;

typedef struct bst_iter {
  bst_node **stack, *curr;
  size_t     cap, top;
} bst_iter;

# define BST_DEFINE(K, V, CMP_FUN)                                             \
    RK_EXTERNC_BEG                                                             \
    typedef struct BstEntry(K, V) {                                            \
      K const key;                                                             \
      V       val;                                                             \
    } BstEntry(K, V);                                                          \
    struct RK__BstNode(K, V) {                                                 \
      bst_node *l, *r;                                                         \
      BstEntry(K, V) entry;                                                    \
    };                                                                         \
    typedef struct Bst(K, V) {                                                 \
      union {                                                                  \
        bst_data _bst;                                                         \
        struct {                                                               \
          RK_IFALLOC(Allocator alloc;)                                         \
          size_t count;                                                        \
          struct RK__BstNode(K, V) * root;                                     \
          bst_node** (*const search_fun)(struct Bst(K, V) *, K);               \
        };                                                                     \
      };                                                                       \
    } Bst(K, V);                                                               \
    static_fun bst_node** RK__BST_F(K, V, search_ptr)(Bst(K, V) * self,        \
                                                      K key) {                 \
      bst_node** curr = &self->_bst.root;                                      \
      for (; *curr;) {                                                         \
        BstEntry(K, V)* entry = (BstEntry(K, V)*)(*curr)->entry;               \
        int cmp_res           = CMP_FUN(key, entry->key);                      \
        if (cmp_res == 0) { break; }                                           \
        curr = cmp_res < 0 ? &(*curr)->l : &(*curr)->r;                        \
      }                                                                        \
      return curr;                                                             \
    }

// todo + val offset &((*node)->entry.val)
static_fun void* RK__bst_impl_search(size_t valstart, bst_node** lnk) {
  return *lnk ? ((char*)(*lnk)->entry) + valstart : NULL;
}
static_fun bool RK__bst_impl_insert(bst_data* self, bst_node** lnk,
                                    bool always_insert, size_t valstart,
                                    size_t size, void* pair) {
  /* no key offset (must be right at data)*/
  if (*lnk) {
    if (always_insert) {
      size_t valsize = (size - valstart);
      memcpy((*lnk)->entry + valstart, (char*)pair + valstart, valsize);
    }
    return false;
  }
  rk_set_alloc_fallback(self->alloc);
  bst_node* n = alloc_allocate(sizeof(bst_node) + size, align_max, self->alloc);
  n->r = n->l = NULL;
  memcpy(n->entry, pair, size);
  *lnk = n;
  ++self->count;
  return true;
}
static_fun bool RK__bst_extract(bst_data* self, bst_node** lnk, size_t valstart,
                                size_t valsize, size_t size,
                                void* restrict val_out) {
  rk_assert_ptr_nonnull(val_out);
  if (!*lnk) { return false; }
  --self->count;
  bst_node* curr = *lnk;
  memcpy(val_out, (char*)curr->entry + valstart, valsize);
  if (curr->l && curr->r) {
    lnk = &curr->r;
    while ((*lnk)->l) { lnk = &(*lnk)->l; }
    bst_node* succ = *lnk;
    memcpy(curr->entry, succ->entry, size);
    *lnk = succ->r;
    alloc_deallocate(succ, sizeof(bst_node) + size, align_max, self->alloc);
    return true;
  }
  *lnk = curr->l ? curr->l : curr->r;
  alloc_deallocate(curr, sizeof(bst_node) + size, align_max, self->alloc);
  return true;
}
RK_EXTERNC_END
# define RK__BstNode(K, V) RK__bst_node_##K##_##V

# define RK__bst_init3(K, V, _Alloc)                                           \
    ((Bst(K, V)){RK_IFALLOC(.alloc = _Alloc, ).count = 0, .root = 0,           \
                 .search_fun = RK__BST_F(K, V, search_ptr)})

# define RK__bst_init2(K, V) RK__bst_init3(K, V, alloc_ctx)

# define RK__BstIter(K, V)   RK__bstiter_##K##_##V
static_fun void
    RK__bst_release_(size_t                  size,
                     bst_node* restrict node RK_IFALLOC(, Allocator alloc)) {
  if (!node) { return; };
  RK__bst_release_(size, node->l RK_IFALLOC(, alloc));
  RK__bst_release_(size, node->r RK_IFALLOC(, alloc));
  alloc_deallocate(node, sizeof(bst_node) + size, align_max, alloc);
}

static_fun void RK__bst_release(size_t size, bst_data* self) {
  RK__bst_release_(size, self->root RK_IFALLOC(, self->alloc));
  self->root = NULL, self->count = 0;
}

static_fun bst_node* RK__bst_cpy_(
    const size_t                        size,
    const bst_node* const restrict node RK_IFALLOC(, Allocator alloc)) {
  if (!node) { return NULL; }
  bst_node* out = alloc_allocate(sizeof(bst_node) + size, align_max, alloc);
  out->l        = RK__bst_cpy_(size, node->l RK_IFALLOC(, alloc));
  out->r        = RK__bst_cpy_(size, node->r RK_IFALLOC(, alloc));
  memcpy(out->entry, node->entry, size);
  return out;
}

static_fun void* RK__bst_min(bst_node* cur) {
  if (!cur) { return NULL; }
  while (cur->l) { cur = cur->l; }
  return cur->entry;
}
static_fun void* RK__bst_max(bst_node* cur) {
  if (!cur) { return NULL; }
  while (cur->r) { cur = cur->r; }
  return cur->entry;
}

static_fun bool bst_iter_next(bst_iter* restrict it, bst_node** node_out) {
  while (it->curr) {
    rk_assert(it->top < it->cap && "BST iterator stack overflow");
    it->stack[it->top++] = it->curr;
    it->curr             = it->curr->l;
  }
  if (it->top == 0) { return false; }
  bst_node* node = it->stack[--it->top];
  *node_out      = node;
  it->curr       = node->r;
  return true;
}

# define bst_foreach(self, stack_buf, stack_buf_cap, _entry)                   \
    for (typeof(*(self))*const RK__bs = (self), *RK__ONCE = RK__bs; RK__ONCE;) \
      for (bst_node * RK__node; RK__ONCE; RK__ONCE = 0)                        \
        for (bst_iter it = {.stack = (stack_buf),                              \
                            .curr  = (bst_node*)RK__bs->root,                  \
                            .cap   = (stack_buf_cap),                          \
                            .top   = 0};                                         \
             bst_iter_next(&it, &RK__node);)                                   \
          for (typeof(RK__bs->root->entry)*const _entry                        \
               = &(((typeof(RK__bs->root))RK__node)->entry),                   \
               *RK__ONCE          = _entry;                                    \
               RK__ONCE; RK__ONCE = 0)

RK_HEADER_END
/// @}
#endif

int cmp(int, int);

BST_DEFINE(int, int, cmp);
