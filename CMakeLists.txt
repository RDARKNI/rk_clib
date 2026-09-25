cmake_minimum_required(VERSION 3.16)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(
    rk_clib
    VERSION 1.0.0
    LANGUAGES C CXX
)

# rk_clib requires C11 or later, plus `typeof` (a GNU/Clang/MSVC extension
# prior to C23, where it became a standard keyword) — unlike Triax, which
# needs no such extension at its own C11/C++11 baseline, rk_clib cannot set
# CMAKE_C_EXTENSIONS/CMAKE_CXX_EXTENSIONS OFF without breaking that baseline.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

add_library(rk_clib INTERFACE)
add_library(rk_clib::rk_clib ALIAS rk_clib)
target_include_directories(
    rk_clib
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(
    rk_clib
    INTERFACE
        c_std_11
        cxx_std_11
)


include(CTest)
if(BUILD_TESTING)
    # Triax is vendored at triax/triax.h (see triax/triax.h's own header
    # comment for why: it's dev/test-only tooling, never referenced by
    # include/*.h, so there's nothing to gain from a submodule/FetchContent
    # dependency for a single header used only here).
    add_library(rk_clib-triax INTERFACE)
    target_include_directories(rk_clib-triax INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/triax)

    # Strict warning flags are applied only to rk_clib's own test
    # executables, mirroring Triax::Triax's public interface staying free of
    # them. Flag set matches the one already used to find and fix several
    # bugs in this codebase by hand this session (ArenaMark's const-qualifier
    # cast-qual warning, the RK__qcharptr _Generic cast-qual case, etc.) —
    # this just puts that same set into CI instead of ad hoc invocations.
    add_library(rk_clib-warnings INTERFACE)
    set(RK_CLIB_STRICT_WARNINGS_GNU_COMMON
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wcast-align
        -Wcast-qual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wformat=2
        -Wimplicit-fallthrough
        -Wswitch-enum
        -Wundef
    )
    set(RK_CLIB_STRICT_WARNINGS_GNU_C
        ${RK_CLIB_STRICT_WARNINGS_GNU_COMMON}
        -Wstrict-prototypes
        -Wmissing-prototypes
    )
    set(RK_CLIB_STRICT_WARNINGS_GNU_CXX
        ${RK_CLIB_STRICT_WARNINGS_GNU_COMMON}
        -Wmissing-declarations
    )
    set(RK_CLIB_STRICT_WARNINGS_CLANG_EXTRA
        -Wextra-semi
        -Wextra-semi-stmt
        -Wnewline-eof
    )
    if(MSVC)
        target_compile_options(rk_clib-warnings INTERFACE /W4)
        # _CRT_SECURE_NO_WARNINGS covers "unsafe" CRT functions (strcpy etc.);
        # _CRT_NONSTDC_NO_WARNINGS is the separate switch for "POSIX name is
        # deprecated, use the ISO name" (e.g. strdup vs _strdup) -- the test
        # suite intentionally uses the POSIX names for portability.
        target_compile_definitions(
            rk_clib-warnings
            INTERFACE
                _CRT_SECURE_NO_WARNINGS
                _CRT_NONSTDC_NO_WARNINGS
        )
    else()
        target_compile_options(
            rk_clib-warnings
            INTERFACE
                $<$<COMPILE_LANGUAGE:C>:${RK_CLIB_STRICT_WARNINGS_GNU_C}>
                $<$<COMPILE_LANGUAGE:CXX>:${RK_CLIB_STRICT_WARNINGS_GNU_CXX}>
        )
        if(CMAKE_C_COMPILER_ID MATCHES "^(Clang|AppleClang)$")
            target_compile_options(
                rk_clib-warnings
                INTERFACE
                    $<$<COMPILE_LANGUAGE:C>:${RK_CLIB_STRICT_WARNINGS_CLANG_EXTRA}>
            )
        endif()
        if(CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|AppleClang)$")
            target_compile_options(
                rk_clib-warnings
                INTERFACE
                    $<$<COMPILE_LANGUAGE:CXX>:${RK_CLIB_STRICT_WARNINGS_CLANG_EXTRA}>
            )
        endif()
    endif()

    option(
        RK_CLIB_WARNINGS_AS_ERRORS
        "Treat warnings in rk_clib's own test targets as errors"
        OFF
    )
    if(RK_CLIB_WARNINGS_AS_ERRORS)
        if(MSVC)
            target_compile_options(
                rk_clib-warnings
                INTERFACE
                    /WX
                    # C4189 ('local variable is initialized but not
                    # referenced): rk_static_if's _Generic-based fallback
                    # (used pervasively by RK__pool_dispatch etc.) must
                    # type-check both branches even though only one is ever
                    # selected -- on MSVC this synthesizes a compiler-internal
                    # temporary for the unselected branch at every one of the
                    # hundreds of call sites across rk_pool.h/rk_trees.h/
                    # rk_dict.h, and MSVC flags each as unreferenced. Same
                    # false-positive class as -Wcast-align above, just
                    # surfacing as a different diagnostic on this compiler.
                    /wd4189
                    # C4146 (unary minus on unsigned, "result still
                    # unsigned"): rk_align_pad and bitset_write's
                    # -(unsigned)x two's-complement masking idiom is
                    # deliberate, not a mistake -- there's no way to "fix" it
                    # without obscuring the intent, since producing an
                    # unsigned result is the entire point.
                    /wd4146
                    # C4100 (unreferenced parameter): the Allocator/vtable
                    # style dispatch functions across rk_alloc.h/rk_arena.h/
                    # rk_bitset.h/rk_pool.h intentionally share a common
                    # signature where not every implementation uses every
                    # parameter (e.g. malloc-backed allocators ignore their
                    # ctx). Same rationale as the GNU/Clang cast-qual/
                    # cast-align exemptions below.
                    /wd4100
            )
        else()
            target_compile_options(
                rk_clib-warnings
                INTERFACE
                    -Werror
                    # -Wcast-qual: RK__qcharptr (rk_string.h) and the
                    # equivalent pattern in test code deliberately strip
                    # const to hand back a mutable-typed pointer derived
                    # from a const-typed input (mirroring strchr's own
                    # signature quirk) -- safe by construction, not a bug,
                    # and pervasive enough (any str_find()-style call on a
                    # literal) that it can't reasonably be pragma-silenced
                    # call-by-call. Still active as a non-fatal warning.
                    -Wno-error=cast-qual
                    # -Wcast-align: RK__pool_dispatch's static/dynamic
                    # selection (rk_pool.h) type-checks both branches even
                    # though only one is ever reached for a given pool, so
                    # a static pool's dummy "_pool.els" union member (a
                    # char[] that exists only so the generic-dispatch macro
                    # type-checks, per its own comment) always gets a
                    # char*->T* cast checked at compile time -- never
                    # actually executed for that pool. Still active as a
                    # non-fatal warning.
                    -Wno-error=cast-align
            )
        endif()
    endif()

    option(
        RK_CLIB_ENABLE_SANITIZERS
        "Enable AddressSanitizer and UndefinedBehaviorSanitizer for rk_clib tests"
        OFF
    )
    if(RK_CLIB_ENABLE_SANITIZERS)
        if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$"
           AND CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
            add_library(rk_clib-sanitizers INTERFACE)
            target_compile_options(
                rk_clib-sanitizers
                INTERFACE
                    -fsanitize=address,undefined
                    -fno-omit-frame-pointer
            )
            target_link_options(rk_clib-sanitizers INTERFACE -fsanitize=address,undefined)
        elseif(MSVC)
            add_library(rk_clib-sanitizers INTERFACE)
            target_compile_options(rk_clib-sanitizers INTERFACE /fsanitize=address)
        else()
            message(
                WARNING
                "RK_CLIB_ENABLE_SANITIZERS is only configured for MSVC or "
                "GCC/Clang-style compiler frontends; ignoring it"
            )
        endif()
    endif()

    # tests/cli.c is the single-TU aggregate entry point: via tests/conf.h it
    # #includes every test_rk_*.c file (see conf.h's own comment), so it's
    # registered here as its one and only source — listing the individual
    # test_rk_*.c files as separate sources too, alongside cli.c, would
    # compile each of them twice (once standalone, once again through
    # cli.c's own #include chain).
    add_executable(rk_clib-selftest-c tests/cli.c)
    target_link_libraries(
        rk_clib-selftest-c
        PRIVATE
            rk_clib::rk_clib
            rk_clib-triax
            rk_clib-warnings
    )

    # Same C suite, rebuilt from scratch with RK_CUSTOM_ALLOCATORS=0, so the
    # malloc-only code paths (RK_IFALLOC-erased params, the fixed alloc_ctx,
    # etc.) get exercised by CI too, not just the default enabled-allocators
    # build above.
    add_executable(rk_clib-selftest-c-noalloc tests/cli.c)
    target_compile_definitions(rk_clib-selftest-c-noalloc PRIVATE RK_CUSTOM_ALLOCATORS=0)
    target_link_libraries(
        rk_clib-selftest-c-noalloc
        PRIVATE
            rk_clib::rk_clib
            rk_clib-triax
            rk_clib-warnings
    )

    if(TARGET rk_clib-sanitizers)
        target_link_libraries(rk_clib-selftest-c PRIVATE rk_clib-sanitizers)
        target_link_libraries(rk_clib-selftest-c-noalloc PRIVATE rk_clib-sanitizers)
    endif()

    add_test(NAME rk_clib-c COMMAND rk_clib-selftest-c --no-color)
    add_test(NAME rk_clib-c-noalloc COMMAND rk_clib-selftest-c-noalloc --no-color)

    # rk_clib's C++ support is Clang-only (see rk_defs.h's own header comment):
    # its headers lean on _Generic pervasively, which only Clang's C++
    # frontend accepts as an extension — GCC and MSVC reject it outright in
    # C++ mode. Building rk_clib-selftest-cpp under any other compiler isn't
    # a regression to catch, it's an expected, permanent failure, so it's
    # only wired in here when the C++ compiler actually is Clang.
    if(CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|AppleClang)$")
        # tests/cli.cpp does not aggregate the C test suite (tests/conf.hpp's
        # equivalent #include block is commented out) — it carries its own,
        # separate set of triax_test registrations exercising the C++-specific
        # formatting path.
        add_executable(rk_clib-selftest-cpp tests/cli.cpp)
        target_link_libraries(
            rk_clib-selftest-cpp
            PRIVATE
                rk_clib::rk_clib
                rk_clib-triax
                rk_clib-warnings
        )
        if(TARGET rk_clib-sanitizers)
            target_link_libraries(rk_clib-selftest-cpp PRIVATE rk_clib-sanitizers)
        endif()
        add_test(NAME rk_clib-cpp COMMAND rk_clib-selftest-cpp --no-color)
    else()
        message(
            STATUS
            "Skipping rk_clib-selftest-cpp: rk_clib's C++ support requires "
            "Clang (CMAKE_CXX_COMPILER_ID is '${CMAKE_CXX_COMPILER_ID}')"
        )
    endif()
endif()
