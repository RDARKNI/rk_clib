# Design of rklib
`rklib` is a small foundational library for C that explores how much type safety and ergonomic API design can be added to C without introducing a runtime object model or requiring C++.

The library is intentionally opinionated. It favors concise call sites, static type checking, predictable data layout, and zero-cost configuration over strict ISO-C portability or globally namespaced identifiers. It targets a selected set of mainstream compilers and hides differences between them behind a compatibility layer so that the public API remains uniform.

This document explains the design decisions behind the library rather than duplicating its API reference. It is intended to make the trade-offs visible to reviewers and to show why different parts of the library use different implementation techniques.

## Design goals
The library is built around several priorities:

1. **Ergonomic C APIs.** Common operations should read naturally: `vec_push`, `dict_get`, `str_cat`, `arena_new`, rather than requiring long project prefixes or repeated casts and boilerplate.
2. **Static type safety where C can provide it.** Macros use `typeof`, `_Generic`, typed compound literals, and generated functions to preserve element types and reject many category errors at compile time.
3. **No runtime generic-dispatch cost unless the abstraction inherently requires it.** Generic containers use compile-time specialization or shared type-erased helpers rather than per-container vtables. The allocator interface is the main deliberate runtime-dispatch abstraction, and even that can be compiled out globally.
4. **One public API across supported compilers and language versions.** Compiler and C-version differences are isolated in `rk_defs.h`. If a feature cannot be implemented with equivalent semantics on every targeted compiler, it is generally not used to create a public API that only works on some targets.
5. **Simple ownership.** Containers own their backing storage, not the resources held by their elements. Element destruction, deep copy, and other domain-specific ownership policies stay explicit in user code.
6. **Useful zero-initialized states.** Where practical, an all-zero object is a valid empty object. This reduces initialization ceremony and makes cleanup paths simpler.
7. **Pay only for enabled features.** Configuration such as custom allocators changes layouts and generated code at compile time rather than inserting runtime branches.

These goals sometimes conflict. The library deliberately chooses ergonomics and selected-toolchain portability over strict standards purity and namespace conservatism.

## Portability philosophy
`rklib` does not attempt to compile on every conforming C implementation. Instead, it defines a supported compiler set and builds a compatibility layer over the differences between those implementations.

The current code requires C11 or later plus a usable `typeof` implementation. GCC, Clang, and recent MSVC are handled explicitly; C++ compatibility is currently restricted to Clang. The library also uses extensions that are available across the targeted compilers, such as zero-argument variadic macro handling, anonymous structs/unions, and compiler attributes.

This distinction is important: the goal is **portable behavior across the supported toolchains**, not “only use syntax present in the oldest common language standard.”

`rk_defs.h` centralizes this policy. It provides fallbacks or wrappers for:

- `typeof`, `thread_local`, `restrict`, `nullptr`-like null values, and `countof`;
- C23 `<stdbit.h>` operations when the standard header is unavailable;
- compiler attributes such as `nodiscard`, `noreturn`, `pure`, `const`, allocation-size/alignment annotations, branch hints, and forced inlining;
- `unreachable`, static-expression assertions, type inspection, and selected bit operations;
- C/C++ linkage and inline-variable/function differences;
- compiler warning suppression for extensions that are intentionally used by the library.

A public feature is not made platform-specific merely because one compiler offers a convenient extension. For example, GNU statement expressions could make some macros evaluate every argument exactly once, but they are not available with equivalent semantics on all targets. The library therefore avoids depending on them. One consequence is that some macro APIs document that arguments with side effects are unsafe when an expression must be referenced more than once.

This is an intentional trade: **uniform semantics across the supported compiler set are more important than exploiting one compiler's best extension.**

## Header-only and multi-translation-unit use
The implementation is designed to retain the convenience of a header library without making multi-translation-unit programs depend on accidental linker behavior. `rk_defs.h` centralizes the linkage rules behind `static_fun`, `extern_fun`, `extern_var`, and the `RKI_HEADER_BEGIN`/`RKI_HEADER_END` wrappers.

In the simple configuration, definitions can live in headers with compiler-appropriate inline/static linkage. For projects that opt into the multi-translation-unit mode, one translation unit can provide the external definitions through `RK_IMPL` while the remaining translation units see compatible declarations. C++ inline variables are used when the language provides them.

This is another compatibility-layer decision: higher-level headers do not each reinvent C versus C++ versus compiler-specific inline/linkage rules.

## Naming and namespace trade-offs
The public API intentionally does not prefix every operation with `rk_`. Container-specific names such as `vec_push`, `dict_get`, `heap_pop`, and `str_cat` are shorter and read more naturally in ordinary C code than `rk_vec_push`, `rk_dict_get`, and so on.

This sacrifices some global-namespace isolation. C has no namespaces, so a foundational library must choose between collision resistance and call-site ergonomics. `rklib` chooses ergonomics for ordinary public operations while reserving prefixed families for infrastructure, configuration, utility functions, and internal implementation machinery.

The intended use case is a small-to-medium C project that deliberately adopts the library as part of its foundation, not a drop-in dependency that must coexist invisibly with an arbitrary number of unrelated C frameworks.

### Prefix and capitalization convention
Prefix depth indicates who an identifier is for, independent of which header it lives in:

- **No prefix** — ordinary, ergonomic domain operations meant to be called constantly: `vec_push`, `dict_get`, `arena_allocate`, `str_cat`.
- **`rk_foo` / `RK_FOO`** — public infrastructure: utility functions and macros (`rk_align_up`, `rk_memcpy`, `rk_arrdup`, `rk_assert`) and configuration macros (`RK_CUSTOM_ALLOCATORS`, `RKI_HEADER_BEGIN`, `RK_MULTI_TU`). Meant to be used directly, just not as often as the unprefixed operations.
- **`rki_foo` / `RKI_FOO`** — internal implementation machinery, not part of the public API: container-generation building blocks (`RKI_DICT_PUB`, `RKI_VEC_CAP`), internal types (`RKI_VecHdr`, `RKI_DynPool`), and internal helper functions (`rki_malloc_allocate`). The `i` marks "internal" the same way the outer `rk_`/`RK_` marks "this library."

Within each tier, capitalization tracks a different thing than the tier itself does: not what the identifier is (macro or function), but whether it's *allowed* to read like one. The driving motivation is the same ergonomic instinct behind the no-prefix domain operations: something used constantly should look plain and lowercase, not shout in uppercase. But a macro only earns that lowercase, ordinary-expression look if it's actually safe to use like one — every argument evaluated exactly once, so an expression with side effects (`rk_min(x++, y)`) behaves the way it would with a real function call. Real functions get this for free, trivially, from C's calling convention. Macros have to earn it deliberately, typically by forwarding to a real function or only ever touching an argument inside `sizeof`/`typeof` (which inspect a type, not a value, and so don't evaluate it) — which is how `rk_min`/`rk_max`/`rk_assert`/`rk_arrdup` get to sit lowercase right next to real functions like `rk_align_up`/`rk_memcpy`. A macro that can't make that guarantee stays uppercase, both as an honest signal to readers and because internal, library-controlled call sites (where every argument is already known to be side-effect-free) don't need to bother earning the lowercase look at all — which is why `RKI_`-prefixed macros vastly outnumber the handful of `rki_`-prefixed internal functions.

The unprefixed domain operations (`vec_push`, `dict_get`, ...) don't have this fallback: they're lowercase unconditionally, for the same ergonomic reason, even where a macro genuinely can't guarantee single evaluation. Those instead document the restriction explicitly (`@attention **Arguments with side effects are not safe in vec_ macros**`) rather than through case, since the ergonomic goal at that tier outweighs reserving uppercase as a warning sign.

This is orthogonal to the `static_fun`/`extern_fun`/`rklib_fun` linkage macros described above — those control *how* a declaration is compiled (inline vs. external, per `RK_MULTI_TU`/`RK_IMPL`/C-vs-C++), not what its name looks like.

## Generic programming strategy
There is deliberately no single “generic container mechanism.” Different abstractions need different amounts of type-specific information, so the library uses the least intrusive mechanism that preserves type safety and avoids runtime dispatch.

### 1. Type-derived genericity: `Vec`
`Vec(T)` is simply a typed pointer:
```c
Vec(int) values;
```
The allocation header containing length, capacity, and optional allocator state lives immediately before the element storage. Public macros retain the ordinary `T *` type, while implementation code derives facts such as `sizeof(*vec)` and uses shared byte-oriented helpers where appropriate.

No `VEC_DEFINE(T)` is required because there is no useful type-specific behavior to generate. A vector of `Foo` and a vector of `Bar` differ primarily in element size/alignment, which the compiler can already derive from the expression type.

This gives several benefits:

- `Vec(T)` works immediately for arbitrary complete element types;
- no generated function namespace is needed for every `T`;
- indexing and pointer iteration are ordinary C operations;
- the implementation can stay mostly shared rather than being duplicated per element type.

The cost is that a `Vec(T)` has no distinct struct identity: to `_Generic`, it is simply a `T *`.

### 2. Generated representation, shared implementation: `Pool`
Pools need a real type definition because their representation depends on the element type and, for static pools, on capacity. `POOL_DEFINE` therefore generates the struct type.

However, most pool operations do not need a user-supplied operation on `T`. Allocation and deletion depend only on layout information, the allocation bitset, element size, and alignment. The implementation therefore avoids generating a full independent function suite for every pool type.

Instead, the generated representation contains enough compile-time information for macros to recover the element type and distinguish static from dynamic pools. Shared helpers perform the actual work.

This produces an API such as:
```c
pool_new(&pool);

pool_delete(&pool, ptr);

pool_used(&pool);
```
rather than forcing the type name to be repeated on every operation after the type has already been established by `self`.

The static and dynamic forms intentionally share one conceptual API:

- `Pool(T)` has runtime capacity and allocated backing storage;
- `Pool(T, C)` embeds both storage and allocation bitmap directly in the object.

Compile-time dispatch chooses the appropriate representation path.

### 3. Generated typed behavior: `Deque`, `Dict`, `Set`, `Heap`, and trees
Other containers require genuinely type-specific functions.

A heap needs a comparator. A dictionary needs a hash and equality/comparison function. Search trees need an ordering function. These operations cannot be reconstructed from `sizeof(T)` or `alignof(T)`, and calling them through stored function pointers would add runtime state and indirect calls.

For these containers the library instead generates concrete typed functions at file scope:
```c
HEAP_DEFINE(MyType, my_cmp);

DICT_DEFINE(Key, Value, key_hash, key_cmp);

AVL_DEFINE(Key, Value, key_cmp);
```
The generated functions have real C signatures involving the actual key/value/element types. Public operation macros carry the type tokens where necessary so the preprocessor can form the correct generated function name:
```c
heap_push(MyType, &heap, value);

dict_get(Key, Value, &dict, key);

avl_set(Key, Value, &tree, key, value);
```
This repetition is deliberate. `typeof(expr)` can recover a C type, but the preprocessor cannot turn that type back into the identifier token needed to construct a generated function name. Avoiding the explicit type argument would require either:

- a global `_Generic` registry mapping every instantiated container type to every operation; or
- runtime function pointers/vtables stored in each container.

The library prefers the simpler generated-function model: a slightly more explicit call site in exchange for ordinary typed C functions and no runtime dispatch.

### Why not one universal trait system?
Libraries such as CC or M*LIB build sophisticated compile-time registries that map C types to operations such as hash, comparison, destruction, or container methods. That can remove repeated type names from call sites, but it shifts substantial complexity into extensible `_Generic` machinery and closed association lists.

`rklib` uses `_Generic` where it naturally solves a local problem, but does not make a global type-to-operation registry the foundation of the library. The design favors implementation transparency over maximal syntactic compression.

## `_Generic` as local overloading
`_Generic` is used when the set of accepted forms is naturally small and closed.

The string API is the clearest example. Most algorithms internally operate on `Strv`, while public macros accept a family of “Stringlike” values (`Str`, `Strv`, `char *`, and `const char *`) and convert them to a view automatically. This gives convenient calls such as:
```c
str_equals(a, b);

str_cat(&s, suffix);

str_find(text, needle);
```
without turning the entire library into a general trait system.

`rk_defs.h` similarly uses `_Generic` for numeric operations, qualifier/type inspection, and static dispatch between a small number of known cases.

## Allocation model
Memory allocation is a central library policy rather than an implementation detail hidden inside individual containers. All owning data structures are written against the same allocator contract, and higher-level memory managers such as `Arena` and `ArenaStack` can themselves be adapted to that contract. This lets allocation strategy vary independently of container implementation while keeping the public APIs consistent.

The allocator interface is also the main place where `rklib` deliberately permits runtime polymorphism. Container algorithms avoid stored comparison/hash/destructor vtables because those operations can usually be bound statically. Allocation is different: choosing where memory comes from is often genuinely a runtime property of an object or construction context, so a small runtime strategy object is useful here.

`rk_alloc.h` represents that strategy as a two-word handle:
```c
typedef struct Allocator {

    const AllocatorVTable *vtab;

    void *ctx;

} Allocator;
```
The vtable contains allocation, reallocation, and deallocation operations. Each operation receives enough information to support allocators more general than `malloc`:
```c
void *allocate(size_t size, size_t align, void *ctx);

void *reallocate(void *ptr,

                 size_t old_size,

                 size_t new_size,

                 size_t align,

                 void *ctx);

void deallocate(void *ptr,

                size_t old_size,

                size_t align,

                void *ctx);
```
This interface shape is deliberate.

- **Alignment is part of the contract.** Containers do not assume that every allocation only needs fundamental alignment. The same interface can therefore serve ordinary values and over-aligned types.
- **The old allocation size is supplied to resize and release operations.** `malloc` may ignore it, but arenas, page-based allocators, debug allocators, slab allocators, and other custom strategies can use it without maintaining a separate size lookup table.
- **Reallocation is a first-class operation.** Growable structures such as `Vec`, `Str`, and `Deque` can ask the allocator to resize directly instead of imposing an allocate-copy-free sequence at every call site. An allocator that cannot resize in place is still free to implement reallocation internally by allocating a new block, copying the minimum of the old and new sizes, and releasing the old block.
- **Allocator state is explicit through `ctx`.** The vtable describes the strategy; the context identifies a particular instance of that strategy. Multiple arenas can therefore share one allocator vtable while carrying different arena state, and containers can retain the allocator instance they were created with.

The result is intentionally closer to a small capability object than to a set of global replacement functions. An `Allocator` value says both *how* allocation is performed and *which allocator instance* a particular object belongs to.

### Typed allocation facade
The raw allocator interface is byte-oriented, but ordinary users rarely need to spell byte counts and alignments manually. `rk_alloc.h` exposes typed helpers such as `alloc_new`, `alloc_renew`, and `alloc_delete`, which derive element size, alignment, and result type from C types and expressions.

Conceptually:
```c
Foo *items = alloc_new(Foo, count);

items = alloc_renew(items, old_count, new_count);

alloc_delete(items, new_count);
```
is translated into the corresponding byte-oriented allocator operations with the appropriate `sizeof(Foo)` and alignment. This follows the same general pattern as the container APIs: preserve useful type information at the public boundary, erase to sizes and pointers only where the implementation no longer needs the concrete C type.

### Allocation strategy is part of object identity
When custom allocators are enabled, owning objects capture an `Allocator` value when they are constructed. Their later growth and destruction use that captured allocator rather than consulting whichever allocator happens to be current at the time of the operation.

This matters for correctness as well as configurability. Memory must be released through the same allocation strategy that created it, and an object should not silently migrate between allocators because a process-wide default changed later.

`alloc_ctx` exists only as a construction-time default. APIs that omit an explicit allocator use its current value, after which the created object owns a copy of that allocator handle. Changing `alloc_ctx` affects future constructions, not existing objects.

When `RK_ALLOC_MULTITHREADED` is enabled, `alloc_ctx` is thread-local. This allows different threads to establish different default allocation strategies without threading an allocator argument through every construction call, while explicit allocator arguments remain available where allocation policy should be visible at the call site.

This is a deliberate compromise between two extremes:

- hard-code `malloc` inside every data structure, making allocation policy impossible to change; or
- require an allocator parameter on every allocation-capable API, even when one construction-time policy should naturally govern the object's whole lifetime.

`rklib` instead makes the allocator explicit in the object model, while keeping the common construction path concise.

### Built-in and composable allocators
Two general-purpose allocator strategies are provided directly:

- `alloc_malloc_allocator`, backed by `malloc`/`free` with platform-specific handling for over-aligned requests;
- `alloc_page_allocator`, backed by `mmap` on POSIX systems and `VirtualAlloc` on Windows.

More importantly, the interface is used by higher-level library components rather than remaining isolated in `rk_alloc.h`. `Arena` and `ArenaStack` can expose themselves as `Allocator` values. A container can therefore allocate from a bump arena, a growable arena stack, the ordinary heap, or a user-defined allocator without requiring separate container implementations for each memory source.

This makes the allocator contract one of the main composition points in the library. The memory-management headers and the container headers are designed to interoperate through it rather than forming unrelated subsystems.

### Custom allocators can compile out
Runtime allocator flexibility is optional. `RK_CUSTOM_ALLOCATORS=0` removes custom-allocator members from owning objects, removes optional allocator arguments from the relevant macro paths, and bypasses vtable dispatch in favor of the malloc-backed implementation.

This is important to the library's definition of “zero-cost configuration.” Supporting custom allocation does not mean every build must permanently carry two extra pointers in each owning object or pay an indirect call on every resize. Projects that do not need allocator polymorphism can choose the simpler representation at compile time.

The same source-level library therefore supports two different cost models:
```text
custom allocators enabled

    object -> captured Allocator -> vtable -> allocator strategy

custom allocators disabled

    object -> direct malloc-backed implementation
```
The choice affects generated code and object layout rather than inserting a runtime `if` around every allocation. This mirrors the broader design principle used elsewhere in the library: optional abstraction should disappear when it is disabled.

### Failure policy belongs to the allocator
The allocator contract is intentionally “infallible” for ordinary dynamic allocation: a non-zero allocation request either returns valid memory or handles failure inside the allocator. The built-in allocators use configurable failure hooks from `rk_config.h`, which assert/abort by default.

This keeps allocation failure policy out of every container API. `vec_push`, `dict_set`, `str_cat`, and tree insertion do not all grow status-return plumbing merely because their implementation may need memory. Applications that consider process-level allocation exhaustion fatal can therefore use direct, value-oriented APIs without repetitive checks.

This policy is distinct from finite-resource exhaustion. When failure is an expected property of a bounded resource, the API exposes it explicitly: fixed-capacity `Arena` and `Pool` operations provide `try_` forms that return `NULL` when their backing resource has no room. The library therefore distinguishes:

- **global/dynamic allocation failure**, handled by allocator policy; and
- **normal bounded-capacity exhaustion**, represented in the API as a recoverable result.

A custom allocator is free to adopt a different failure mechanism internally, but it must honor the same outward contract: callers of ordinary allocation operations do not receive `NULL` for a non-zero successful request path.

### Why allocation gets a vtable when containers do not
The use of a vtable here is intentionally narrower than a general runtime object model.

A dictionary's hash function or a heap's comparator is normally a property of its compile-time specialization. Storing those functions in each object would add state and indirect calls without enabling a use case the type system cannot already express. Those operations are therefore statically bound.

An allocator, by contrast, often is a runtime strategy. Two objects of exactly the same `Vec(Foo)` type may legitimately allocate from different arenas. Which allocator an object uses is not necessarily encoded by its element type and may be selected by construction context. A small vtable/context pair therefore represents real runtime variability rather than compensating for limitations in the generic-programming machinery.

This distinction is central to the library's abstraction policy: **runtime polymorphism is used where the policy is genuinely dynamic; compile-time genericity is preferred where behavior is naturally determined by type.**

### Size-overflow policy
Size multiplication is unchecked by default through the configurable `rk_mult` hook. Projects that prefer fail-fast overflow checking can redefine it to `rk_mult_safe` globally.

This makes the performance/safety trade explicit rather than silently imposing repeated overflow checks on every container operation.

## Memory ownership and element semantics

Containers manage their own backing allocations but do not provide C++-style element constructors, destructors, or deep-copy hooks.

Elements are ordinary C values. Moving or resizing storage may copy their object representations or assign them as values. If an element contains an owning pointer or another resource, ownership of that nested resource remains the caller's responsibility.

This keeps the generic machinery substantially simpler and matches C's explicit ownership model. It also makes container behavior predictable: the library owns **storage**, while user code owns the semantic lifetime of the values stored in it.

## Header-by-header design
### `rk_config.h` — compile-time policy
This header collects configuration that can affect ABI, layout, or failure behavior. Important options include custom allocators, thread-local allocator defaults, allocation failure handlers, dictionary load factor, debug logging, and checked/unchecked size multiplication.

Configuration must be consistent across translation units because options such as `RK_CUSTOM_ALLOCATORS` alter public struct layouts.

Keeping these switches centralized makes compile-time policy visible instead of scattering `#ifdef` decisions throughout application code.

### `rk_defs.h` — compatibility and low-level vocabulary
This is the foundation layer. It normalizes compiler/language features and provides the internal vocabulary used by every other header.

Besides portability wrappers, it includes:

- fixed-width shorthand integer typedefs;
- alignment and pointer-range helpers;
- typed `memcpy`/`memmove` wrappers;
- compile-time assertions usable inside expressions;
- macro arity overloading and token-generation helpers;
- const/type inspection;
- type-safe numeric min/max/clamp and saturating arithmetic;
- fallback implementations for C23 bit operations.

The intent is to isolate non-portable code here so higher-level headers can be written against one consistent dialect.

### `rk_alloc.h` — allocator abstraction
Defines the allocator vtable/context model, malloc and page allocators, typed allocation macros, aligned allocation, array duplication, and the default `alloc_ctx` mechanism.

Typed allocation macros derive sizes and alignments from C types, reducing repeated casts and manual `sizeof` expressions at call sites.

### `rk_arena.h` — fixed backing-store arena
`Arena` is a minimal three-pointer bump allocator over caller-provided storage. Allocation is O(1), individual deallocation is normally a no-op, and bulk reset is constant-time.

Marks and rewinds support stack-like lifetime regions. The most recent allocation can be resized in place when capacity permits.

An arena can be exposed as a general `Allocator`, allowing ordinary containers to use stack/static temporary storage without special container implementations.

The header also includes an embedded array-backed allocator helper for small local allocation regions.

### `rk_arenastack.h` — growable arena with stable addresses
`ArenaStack` composes `Arena` and `Vec(Arena)`. When the current arena fills, another backing arena is reused or allocated instead of reallocating existing arena storage. Existing allocation addresses therefore remain stable.

Arena sizes grow to powers of two when larger requests require it. Marks and rewinds work across arena boundaries, and the whole structure can also be adapted to the generic allocator interface.

This header illustrates deliberate composition: a higher-level allocator is built from the simpler arena and vector abstractions rather than duplicating their mechanisms.

### `rk_bitset.h` — explicit logical-size bit operations
The bitset layer is intentionally lightweight: a bitset is storage plus an explicit logical bit count, not a heap-owning object.

`bitset(n)` provides fixed-size typed storage for compile-time sizes, while functions operate on `bitset`/`cbitset` pointers and `nbits`.

The header documents and maintains a zero-padding invariant for unused bits in the final word. Query naming deliberately follows C23 `<stdbit.h>` conventions where appropriate, while index-oriented search functions use ordinary zero-based indices and `SIZE_MAX` sentinels.

This header also supplies the occupancy bitmap used by pools.

### `rk_vec.h` — contiguous growable array
`Vec(T)` uses the least expensive generic representation in the library: the public object is just `T *`, with metadata stored immediately before the element array.

This preserves native pointer indexing and keeps `Vec(T)` usable without prior instantiation. Capacity grows geometrically, and optional allocator state is stored in the hidden header.

The API includes reserve/resize/shrink operations, ordered and unordered insertion/erase paths, bulk operations, and pointer-based iteration.

The representation is intentionally unlike the generated-struct containers because a separate type-specific struct would add ceremony without enabling useful additional static behavior.

### `rk_heap.h` — typed binary heap composed over `Vec`
A heap is represented as a generated struct containing `Vec(T)`. The comparator is bound at specialization time by `HEAP_DEFINE` so sift-up/down operations are ordinary typed calls with no stored comparator pointer.

Storage management delegates to `Vec`, while heap-specific code handles ordering. Bulk `from`, `adopt`, `assign`, and `extend` paths heapify in O(n) rather than repeatedly pushing in O(n log n).

This is a deliberate example of layering rather than reimplementation.

### `rk_deque.h` — generated circular buffer
`Deque(T)` is a generated typed struct containing a circular buffer, head index, count, capacity, and optional allocator.

Capacities are powers of two, allowing wraparound with masks instead of modulo in hot paths. Reallocation linearizes the logical sequence into a fresh buffer.

The implementation is specialized because many operations manipulate and return actual `T` values. The API provides ordered bulk pushes at both ends, checked/try pop variants, const-propagating accessors, and forward/reverse iteration.

### `rk_pool.h` — static/dynamic fixed-slot allocator
Pools combine typed element storage with a bitset tracking occupied slots.

The static form embeds all storage in the object; the dynamic form allocates the bitmap and element array. Macro-level compile-time dispatch lets both forms share the same user-facing operations.

The struct definition is type-specific, but the behavior is largely shared because pool allocation depends on layout rather than user-supplied element operations. This is why the pool occupies the middle ground between `Vec`'s no-instantiation model and fully generated containers such as `Dict`.

### `rk_dict.h` — open-addressed dictionary and set
`Dict` and `Set` share one open-addressing implementation. Each slot has a compact one-byte state/fingerprint field:

- empty;
- tombstone;
- occupied with a seven-bit hash fingerprint.

The fingerprint lets probing reject most non-matches before invoking the key comparator. Capacity is maintained as a power of two, and the configurable load threshold is evaluated with integer arithmetic rather than floating point.

Hash and comparison functions are bound at specialization time. This avoids storing callbacks in every table and gives generated operations concrete key/value signatures.

`Set` reuses the same implementation machinery while omitting value storage rather than maintaining a separate unrelated hash-table implementation.

### `rk_string.h` — owning strings and views
The string layer separates ownership from observation:

- `Str` owns a growable mutable buffer;
- `Strv` is a non-owning immutable pointer/length view.

Most algorithms are implemented in terms of `Strv`. `_Generic`-based conversion lets public functions accept Stringlike values without requiring explicit conversion at every call.

`Str` keeps a stable invariant around length, capacity, and null termination; explicitly named `_raw` operations are the exceptions. Operations that may receive an aliased source expose separate `*_mayalias` paths rather than silently paying for overlap handling in the common case.

This header uses local ad-hoc polymorphism where it improves ergonomics without introducing a global generic type registry.

### `rk_trees.h` — BST, AVL, and left-leaning red-black trees
The three tree variants expose parallel typed APIs but share the parts that do not depend on balancing strategy.

Concrete node types contain typed entries and variant-specific metadata. A small type-erased `tree_node` prefix represents only left/right links, allowing release, min/max lookup, and in-order traversal machinery to be shared safely across variants.

Comparators are bound at specialization time, keeping search and rotation code fully typed. The three structures then differ only where balancing actually matters:

- `Bst` performs no balancing;
- `Avl` maintains strict height balance;
- `Rbt` uses left-leaning red-black invariants for looser balance and fewer updates.

The common API shape makes the balancing choice replaceable without inventing a runtime tree interface.

### `rklib.h` — umbrella include
The umbrella header collects the ordinary library modules for applications that prefer one include. Individual headers remain independently includable so projects can keep dependencies narrow.

## API consistency principles
Several conventions recur across otherwise different data structures:

- `*_init` constructs, `*_release` releases owned backing storage, and `*_clear` retains storage for reuse;
- `*_count`, `*_cap`, `*_is_empty`, and `*_allocator` use parallel meanings where applicable;
- `*_reserve` grows capacity without changing logical contents;
- `*_shrink_to_fit` reduces retained storage;
- `*_try_*` denotes a recoverable failure path rather than the library's normal fail-fast path;
- removal follows the same split: `vec_pop`, `deque_pop_*`, `heap_pop`, and `str_pop` assert that the container is nonempty, while `vec_try_pop`, `deque_try_pop_*`, `heap_try_pop`, and `str_try_pop` return `false` and leave their output untouched when it is empty. No pop returns a sentinel value, which could not be told apart from a stored element;
- positional element access comes in two forms: `*_front`, `*_back`, `*_at`, `heap_top`, and the trees' `*_min`/`*_max` return an lvalue and assert their precondition, while the matching `*_peek_*` (and `heap_peek`) return a nullable pointer (`NULL` when the element does not exist). Keyed lookup follows the same split: `dict_at`/`set_at` return an asserted lvalue, `*_get` a nullable pointer. A stored key is always const (`set_at` and `set_get` are const even for a mutable Set), since modifying it would break its hash slot. All of them propagate the container's constness; `heap_top`/`heap_peek` are always const, since writing the top in place would break the heap order;
- bulk operations exist when they can avoid repeated allocation or repeated O(log n) work;
- unordered erase/insert variants are exposed when relaxing order can materially reduce work;
- iterators are usually plain typed pointers rather than opaque iterator objects where representation permits it.

Consistency is treated as a design feature, but not as a reason to expose meaningless operations. A tree has no capacity API; a vector does not need a generated comparator; a pool does not carry an element-operation vtable merely to resemble a dictionary.

## Performance principles
The code generally favors simple predictable costs over abstraction uniformity:

- powers-of-two capacities allow inexpensive masking and growth;
- vectors keep metadata adjacent to data and expose native element pointers;
- deques copy at most two contiguous segments when wrapping;
- dictionaries use compact fingerprints before full key comparison;
- heap bulk construction uses linear-time heapification;
- arena allocation is pointer bumping;
- arena stacks preserve allocation addresses by adding arenas rather than moving existing ones;
- allocator metadata and dispatch can disappear entirely at compile time;
- type-specific comparison/hash behavior is statically bound rather than stored as callbacks.

Compiler attributes communicate purity, allocation size/alignment, and similar facts where supported, but correctness does not rely on one compiler understanding a particular annotation.

`rk_const` and `rk_pure` let the compiler remove a call whose result is unused, and any assertion inside goes with it. With `rk_const` on the string bounds-check helpers, `(void)str_at(s, i)` skipped its bounds check at `-O2`. So helpers behind asserting accessors (`*_front`, `*_back`, `*_at`, `*_pop`) must not carry these annotations: such a call may be made only for its check. Pure queries such as `bitset_test()` or `rk_align_up()` may keep them, accepting that their assertions only run when the result is used, since a discarded query has no purpose.

## Safety philosophy
This is a low-level C library, not a bounds-checked runtime.

The design uses static checks when the compiler has enough information, assertions for violated programming contracts, and explicit checked/`try` variants where failure is expected to be recoverable. It does not add a status branch to every operation simply to turn programmer errors into runtime error codes.

Examples include:

- compile-time type compatibility checks around typed memory operations;
- compile-time or asserted alignment validation;
- assertions for invalid indices/preconditions in debug-oriented paths;
- configurable fail-fast handling for allocation exhaustion;
- an optional globally checked size-multiplication policy;
- `try` APIs for fixed-capacity arena/pool exhaustion.

The guiding distinction is between **recoverable resource conditions** and **violated API contracts**.

## Testing and verification
`rklib`'s test suite is built on Triax, a C/C++ testing framework also written by this project's author and vendored directly into `triax/`. The relationship runs in both directions: Triax provides the assertion, parameterization, and process-isolation machinery the test suite is built on, while `rklib`'s test suite is one of Triax's most demanding real-world consumers, regularly exercising corners of Triax itself (its fault-injection and process-isolation paths in particular) that a smaller test suite would not reach. Bugs found on either side have, in practice, driven fixes in the other.

Contract violations documented as assertions (invalid indices, null out-parameters, allocator failure) are exercised, not just declared. Triax's isolation mode runs each such case in a separate process and checks that the expected fault actually occurs, so an assertion that silently stopped firing (for example, because of a compiler optimizing away a supposedly pure call) would show up as a test failure rather than passing unnoticed.

The suite runs across the actual compiler/platform matrix the library claims to support, not just one reference toolchain:

- Clang, GCC, and MSVC, each in a strict-warnings configuration (`-Wall -Wextra -Wpedantic -Werror` or the MSVC equivalent), since the three frontends disagree on which extensions and coercions to flag;
- native builds on Linux (x86-64 and ARM64), macOS, and Windows;
- AddressSanitizer + UndefinedBehaviorSanitizer under Clang, and AddressSanitizer under MSVC;
- both `RK_CUSTOM_ALLOCATORS` on and off, since that option changes struct layout and generated code rather than just runtime behavior.

Running the same suite through multiple compilers has repeatedly caught real, platform-specific bugs that a single toolchain missed: GCC's stricter handling of constant-expression initializers, GNU-extension-dependent code paths Clang tolerates and GCC (correctly) does not, and GCC's dead-call elimination for functions marked `pure` when their result is discarded but the call also has to run for the assertion inside it to fire — a genuine hazard for any `pure`-annotated function that asserts on its input, uncovered directly by this cross-compiler CI matrix rather than by manual review.

## Known trade-offs
The design intentionally accepts several costs:

- Public names occupy more of C's global namespace than a strictly prefixed library would.
- The supported language is a practical compiler intersection, not portable ISO C11 alone.
- Macro-based ergonomics make diagnostics more complex than equivalent hand-written functions.
- Without statement expressions, some macros cannot both preserve an expression-like API and guarantee single evaluation of every argument; these cases must document side-effect restrictions.
- Specialization macros require typedef-friendly identifier tokens for some compound/pointer types because token pasting works on preprocessing tokens, not arbitrary C declarators.
- Generated containers increase compile time/code size relative to one completely type-erased implementation, in exchange for typed functions and static binding.
- Containers intentionally do not manage nested element lifetimes.
- `_Generic`-based dispatch is closed-world by nature: every association list must exist at the point of the macro definition, and macro expansion cannot generate new `#define` directives to extend it later.

These are not accidental inconsistencies. They follow from the central decision to push genericity and policy to compile time while keeping runtime objects straightforward.

## Non-goals
`rklib` is not intended to be:

- a replacement for the C standard library;
- a C++ STL reimplementation with constructors/destructors and allocator traits;
- ABI-stable across arbitrary configuration changes;
- portable to every historical or embedded C compiler;
- a framework that hides ownership or allocation policy behind pervasive runtime interfaces;
- a macro metaprogramming language in its own right.

The goal is narrower: provide a compact set of reusable low-level facilities that feel natural in C while making deliberate use of the compile-time facilities modern mainstream C compilers actually provide.

## Summary
The central design rule is:

> **Use the least powerful generic mechanism that solves the problem without adding runtime cost.**

That leads naturally to different implementations for different headers:

- derive type information directly when the C expression already carries everything required (`Vec`);
- generate only representation when behavior can remain shared (`Pool`);
- generate concrete typed functions when behavior depends on user-supplied type operations (`Heap`, `Deque`, `Dict`, `Set`, and trees);
- use local `_Generic` overloading for small closed families (`Stringlike`, numeric helpers);
- use runtime vtables only for the abstraction that genuinely represents runtime strategy selection (`Allocator`), and allow even that feature to compile out.

The result is intentionally not one uniform metaprogramming trick. It is a collection of C techniques chosen according to the actual information each abstraction needs, with a uniform public style layered over compiler and platform differences.
