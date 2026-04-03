#ifdef _WIN32
// Windows: define minimal types/macros you need
# include <stddef.h> // for size_t
typedef HANDLE rk_fd;
#else
// POSIX
# include <signal.h>

# include <assert.h>
# include <errno.h>
# include <fcntl.h>
# include <poll.h>
# include <unistd.h>
typedef int rk_fd;
#endif

#define RK_COUNTOF(...) (sizeof(__VA_ARGS__) / sizeof((__VA_ARGS__)[0]))
/// Set one or more file descriptors to nonblocking mode.
/// @return 0 on success, index of fd that failed otherwise.
#define rk_set_nonblocking(...)                                                \
  rk_set_nonblocking_n((int[]){__VA_ARGS__}, RK_COUNTOF((int[]){__VA_ARGS__}))
static inline int rk_set_nonblocking_n(int* fds, size_t nfds) {
  for (int i = 0, flags; i < nfds; ++i) {
    while ((flags = fcntl(fds[i], F_GETFL, 0)) == -1) {
      if (errno != EINTR) { return assert(!"fncl set nonblocking failed"), i; }
    }
    while (fcntl(fds[i], F_SETFL, flags | O_NONBLOCK) == -1) {
      if (errno != EINTR) { return assert(!"fncl set nonblocking failed"), i; }
    }
  }
  return 0;
}

/// Close one or more file descriptors.
/// @return 0 on success, index of fd that failed otherwise.
#define rk_close(...)                                                          \
  rk_close_n((rk_fd[]){__VA_ARGS__}, RK_COUNTOF((rk_fd[]){__VA_ARGS__}))
static inline int rk_close_n(rk_fd* fds, size_t nfds) {
  for (int i = 0, r; i < nfds; ++i) {
    while ((r = close(fds[i])) == -1 && errno == EINTR) {}
    if (r) { return assert(!"close failed"), i; }
  }
  return 0;
}
#define rk_make_sigset(...)                                                    \
  rk_make_sigset_n((int[]){__VA_ARGS__}, RK_COUNTOF((int[]){__VA_ARGS__}))
static inline sigset_t rk_make_sigset_n(int* signals, size_t nsignals) {
  sigset_t set;
  sigemptyset(&set);
  for (size_t i = 0; i < nsignals; ++i) { sigaddset(&set, signals[i]); }
  return set;
}
/// Direction for unidirectional pipes.
typedef enum RK_UNIPIPE_DIR_t {
  RK_UNIPIPE_DIR_CTOP, ///< Child writes, parent reads
  RK_UNIPIPE_DIR_PTOC, ///< Parent writes, child reads
} RK_UNIPIPE_DIR_t;

/// Returns 1 if the given process (pid) is the writer for a unipipe
#define rk_unipipe_is_writer(pid, dir)                                         \
  (((pid) > 0) ^ ((dir) == RK_UNIPIPE_DIR_CTOP))

/// Structure for a pipe with convenient .r/.w access
/// field '_' to allow safe treatment as an int[2]
typedef struct rk_Pipe {
  union {
    struct {
      int r, w;
    };
    int _[2];
  };
} rk_Pipe;

/// Fork and create a bidirectional pipe pair
/// @param r Pointer to receive read end fd.
/// @param w Pointer to receive write end fd.
/// @return pid of child in parent, 0 in child, -1/-2 on pipe/fork failure
static inline pid_t rk_fork_bipipe(int* r, int* w) {
  rk_Pipe ps[2];
  if (pipe(ps[0]._) < 0) { return -1; };
  if (pipe(ps[1]._) < 0) { return rk_close(ps[0].r, ps[0].w), -1; };
  pid_t pid = fork();
  if (pid == -1) { return rk_close(ps[0].r, ps[0].w, ps[1].r, ps[1].w), -2; }
  int d = pid == 0;
  return rk_close(ps[d].r, ps[!d].w), *r = ps[!d].r, *w = ps[d].w, pid;
}

/// Fork and create a single unidirectional pipe.
/// @param fdptr Pointer to receive the fd of the pipe end for caller.
/// @param dir Direction of the pipe (child->parent or parent->child).
/// @return pid of child in parent, 0 in child, -1/-2 on pipe/fork failure
static inline pid_t rk_fork_unipipe(int* fdptr, RK_UNIPIPE_DIR_t dir) {
  int pps[2];
  if (pipe(pps) < 0) { return -1; };
  pid_t pid;
  if ((pid = fork()) < 0) { return close(pps[0]), close(pps[1]), -2; }
  int end = rk_unipipe_is_writer(pid, dir);
  return close(pps[!end]), *fdptr = pps[end], pid;
}

/// Fork and create multiple unidirectional pipes.
/// @param fdptr Array to receive pipe fds for the caller.
/// @param dir Direction of the pipes.
/// @param npipes Number of pipes to create (max 255).
/// @return pid of child in parent, 0 in child, -1/-2 on pipe/fork failure
static inline pid_t rk_fork_unipipe_n(int* fdptr, RK_UNIPIPE_DIR_t dir,
                                      unsigned char npipes) {
  int   pps[255 * 2], opened;
  pid_t pid;
  for (opened = 0; opened < npipes; ++opened) {
    if (pipe(pps + opened * 2) < 0) {
      pid = -1;
      goto err;
    };
  }
  if ((pid = fork()) < 0) {
    pid = -2;
    goto err;
  }
  for (int i = 0, end = rk_unipipe_is_writer(pid, dir); i < npipes; ++i) {
    close(pps[i * 2 + !end]), fdptr[i] = pps[i * 2 + end];
  }
  return pid;
err:
  for (int i = 0; i < opened; ++i) { close(pps[i * 2]), close(pps[i * 2 + 1]); }
  return pid;
}
/// Result of I/O operations.
typedef enum RK_IO_RESULT_t {
  RK_IO_RESULT_ERR = -1, ///< Unexpected error
  RK_IO_RESULT_OK,       ///< All bytes processed successfully
  RK_IO_RESULT_EOF,      ///< Peer closed pipe (read returned 0)
} RK_IO_RESULT_t;

#ifndef _WIN32
/// Read exactly nbytes from fd, blocking until complete.
static inline RK_IO_RESULT_t rk_read_full(int fd, void* buf, size_t nbytes) {
  for (char* ptr = (char*)buf; nbytes;) {
    ssize_t r = read(fd, ptr, nbytes);
    switch (r) {
    case -1:
      if (errno != EINTR) { return RK_IO_RESULT_ERR; }
      break;
    case 0 : return RK_IO_RESULT_EOF;
    default: ptr += r, nbytes -= r;
    }
  }
  return RK_IO_RESULT_OK; // read all, no eof
}

/// Read exactly nbytes from fd, nonblocking.
/// @return RK_IO_RESULT_OK if all bytes read,
///         RK_IO_RESULT_EOF on EOF,
///         RK_IO_RESULT_ERR on error.
static inline RK_IO_RESULT_t rk_read_full_nb(int fd, void* buf, size_t nbytes) {
  struct pollfd pfd = {.fd = fd, .events = POLLIN};
  for (char* ptr = (char*)buf; nbytes;) {
    ssize_t r = read(fd, ptr, nbytes);
    switch (r) {
    case -1:
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        poll(&pfd, 1, -1); // todo maybe timeout
      } else if (errno != EINTR) {
        return RK_IO_RESULT_ERR;
      }
      break;
    case 0 : return RK_IO_RESULT_EOF; // eof
    default: ptr += r, nbytes -= r;
    }
  }
  return RK_IO_RESULT_OK; // read all, no eof
}

/// Write entire buffer to fd, blocking until complete.
/// @return RK_IO_RESULT_OK on success,
///         RK_IO_RESULT_ERR on error.
static inline RK_IO_RESULT_t rk_write_full(int fd, const void* buf,
                                           size_t len) {
  const char* ptr = (const char*)buf;
  for (ssize_t w; len;) {
    if ((w = write(fd, ptr, len)) == -1) {
      if (errno != EINTR) { return RK_IO_RESULT_ERR; }
    } else {
      len -= (size_t)w, ptr += w;
    }
  }
  return RK_IO_RESULT_OK;
}

/// Write entire buffer to fd, nonblocking.
/// Blocks internally if partial write occurs.
/// @return RK_IO_RESULT_OK on success,
///         RK_IO_RESULT_ERR on error.
static inline RK_IO_RESULT_t rk_write_full_nb(int fd, const void* buf,
                                              size_t len) {
  struct pollfd pfd = {.fd = fd, .events = POLLOUT};
  const char*   ptr = (const char*)buf;
  for (ssize_t w; len;) {
    if ((w = write(fd, ptr, len)) == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        poll(&pfd, 1, -1);
      } else if (errno != EINTR) {
        return RK_IO_RESULT_ERR;
      }
    } else {
      len -= w, ptr += w;
    }
  }
  return RK_IO_RESULT_OK;
}

#else
static inline RK_IO_RESULT_t rk_read_full(rk_fd h, void* buf, size_t nbytes) {
  char* ptr = (char*)buf;
  for (DWORD r; nbytes; ptr += r, nbytes -= r) {
    if (!ReadFile(h, ptr, (DWORD)nbytes, &r, NULL)) {
      return GetLastError() == ERROR_BROKEN_PIPE ? RK_IO_RESULT_EOF
                                                 : RK_IO_RESULT_ERR;
    }
    if (!r) { return RK_IO_RESULT_EOF; }
  }
  return RK_IO_RESULT_OK;
}
static inline RK_IO_RESULT_t rk_read_full_nb(rk_fd h, void* buf,
                                             size_t nbytes) {
  char* ptr = (char*)buf;
  for (DWORD r; nbytes;) {
    if (!ReadFile(h, ptr, (DWORD)nbytes, &r, NULL)) {
      switch (GetLastError()) {
      case ERROR_BROKEN_PIPE: return RK_IO_RESULT_EOF;
      case ERROR_NO_DATA    : continue;
      default               : return RK_IO_RESULT_ERR;
      }
    }
    if (!r) { return RK_IO_RESULT_EOF; }
    ptr += r, nbytes -= r;
  }
  return RK_IO_RESULT_OK;
}
static inline RK_IO_RESULT_t rk_write_full(rk_fd h, const void* buf,
                                           size_t len) {
  const char* ptr = (const char*)buf;
  for (DWORD w; len > 0; ptr += w, len -= w) {
    if (!WriteFile(h, ptr, (DWORD)len, &w, NULL)) {
      return RK_IO_RESULT_ERR; // todo
    }
  }
  return RK_IO_RESULT_OK;
}
static inline RK_IO_RESULT_t rk_write_full_nb(rk_fd h, const void* buf,
                                              size_t len) {
  DWORD w = 0;
  for (const char* ptr = buf; len;) {
    if (!WriteFile(h, ptr, (DWORD)len, &w, NULL)) {
      switch (GetLastError()) {
      case ERROR_NO_DATA:
      case ERROR_IO_PENDING:
      case ERROR_PIPE_LISTENING: continue;
      default                  : return RK_IO_RESULT_ERR;
      }
    }
    len -= w, ptr += w;
  }
  return RK_IO_RESULT_OK;
}
#endif

#if 0
# include <stdio.h>

/// Write entire buffer to a FILE stream, retrying as needed.
/// @return 0 on success, -1 on error.
static inline int rk_fwrite_full(FILE* fd, const char* buf, size_t len) {
    for (size_t w; len > 0; buf += w, len -= w) {
        if (!(w = fwrite(buf, 1, len, fd))) { return -1; }
    }
    return 0;
}
/// Write entire buffer to a FILE stream, retrying as needed.
/// @return 0 on success, -1 on error.
static inline int rk_fread_full(FILE* fd, const char* buf, size_t len) {
    for (size_t w; len > 0; buf += w, len -= w) {
        if (!(w = fwrite(buf, 1, len, fd))) { return -1; }
    }
    return 0;
}
#endif
