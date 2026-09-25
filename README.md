# Simple Header-Only C Utility Library

A lightweight, easy-to-use collection of header-only C11+ utilities.

## Features

- Requires a **C11-compatible compiler** with typeof extension.
- Modular headers for flexible usage.
- Just copy the headers you need (and their dependencies) or include `rklib.h` to get everything.
- Designed for seamless integration into your C projects.

## Usage

### Quickest: single-file drop-in

Download [`single_include/rklib.h`](single_include/rklib.h) and drop it into your project — no
other files needed:

```c
#include "rklib.h"
```

It's regenerated automatically on every push to `main`, so it always matches the latest source.

### Modular: pick only what you need

If you'd rather not pull in the whole library, copy just the header files you need from
`include/` (plus their dependencies), or copy the whole `include/` directory and include the
comprehensive `rklib.h`, which bundles every module:

```c
#include "rklib.h"
```
