# Hash Set

[Hash Set](https://en.wikipedia.org/wiki/Hash_table) implementation in pure C as an [stb-style single-file library](https://github.com/nothings/stb).

## Quick Start

The library does not require any special building. You can simply copy-past [./hashset.h](./hashset.h) to your project an `#include` it.

```c
#define HASHSET_IMPLEMENTATION
#include "hashset.h"

int main(void)
{
    hashset_t hs = {0};

    hashset_insert(&hs, "Hello, World!", sizeof("Hello, World!"));

    puts(hashset_contains(&hs, "Hello, World!", sizeof("Hello, World!")) ? "true" : "false");
    
    hashset_free(&hs);
}
```

## Public API

The library signals errors through an non-zero return code.
Mixing types is the caller's responsibility.

```c
int hashset_insert(hashset_t *hs, void *item, size_t item_size);
int hashset_remove(hashset_t *hs, void *item, size_t item_size);
size_t hashset_size(hashset_t *hs);
bool hashset_contains(hashset_t *hs, void *item, size_t item_size);
void hashset_free(hashset_t *hs);
```

## Customizing the Hash Set

The library gives the user freedom to provide their custom hash function, equality checks, default initialization capacity and grow rate.
_Before_ including the header file, the user can `#define` the following macros to its liking. For example:

```c
#define HASHSET_INIT_CAP 1024
#define HASHSET_LOAD_FACTOR 0.75
#define HASHSET_GROW_RATE 2
#define hashset_hash hashset__hash
#define hashset_equal hashset__equal
```

Otherwise the above listed defaults will be used.

If your code is a single source file, and you want to get rid of unused functions, you can `#define HSDEF static inline`, to let the compiler remove the unused functions.


