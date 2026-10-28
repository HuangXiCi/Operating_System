// Teaching COW regression tests for this xv6 tree.
// Covers physical-memory pressure, multiple generations, copyout, and reclaim.
// This is a local teaching suite, not the upstream MIT cowtest.c.

#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/memlayout.h"
#include "user/user.h"

#define CHILDREN 3
#define GENERATION_ROUNDS 4
#define PIPE_BYTES 16

static void
fail(const char *message)
{
  fprintf(2, "cowtest: FAIL: %s\n", message);
  exit(1);
}

static void
wait_ok(void)
{
  int status = -1;

  if (wait(&status) < 0)
    fail("wait failed");
  if (status != 0)
    fail("child failed or was killed");
}

static char *
grow(int bytes)
{
  char *p = sbrk(bytes);

  if (p == (char *)-1)
    fail("sbrk failed");
  return p;
}

static void
shrink(int bytes)
{
  if (sbrk(-bytes) == (char *)-1)
    fail("sbrk shrink failed");
}

// Touch/check one byte per page. Touching forces lazy pages to be allocated.
static void
fill_pages(char *p, int bytes, char value)
{
  for (int i = 0; i < bytes; i += PGSIZE)
    p[i] = value;
}

static void
check_pages(char *p, int bytes, char value)
{
  for (int i = 0; i < bytes; i += PGSIZE) {
    if (p[i] != value)
      fail("page contents changed unexpectedly");
  }
}

// Use about 2/3 of configured physical RAM: an eager-copy fork cannot
// duplicate this resident allocation. The child dirties only a small subset
// so a correct COW implementation still has ample room to run.
static void
large_memory(const char *label)
{
  int bytes = (int)(((PHYSTOP - KERNBASE) / PGSIZE * 2 / 3) * PGSIZE);

  printf("%s: start\n", label);
  for (int round = 0; round < 2; round++) {
    char *p = grow(bytes);
    fill_pages(p, bytes, 0x35);

    int pid = fork();
    if (pid < 0)
      fail("large-memory fork failed");

    if (pid == 0) {
      check_pages(p, bytes, 0x35);

      for (int i = 0; i < bytes; i += 64 * PGSIZE)
        p[i] = 0x71;
      for (int i = 0; i < bytes; i += PGSIZE) {
        char expected = (i % (64 * PGSIZE) == 0) ? 0x71 : 0x35;
        if (p[i] != expected)
          fail("large-memory child write failed");
      }
      exit(0);
    }

    wait_ok();
    check_pages(p, bytes, 0x35);
    shrink(bytes);
  }
  printf("%s: OK\n", label);
}

// Three children each fork a grandchild. All generations write their own
// versions of every page. Repetition exercises reference removal on exit.
static void
generations(void)
{
  int bytes = 8 * 1024 * 1024;

  printf("multi-generation: start\n");
  for (int round = 0; round < GENERATION_ROUNDS; round++) {
    char *p = grow(bytes);
    fill_pages(p, bytes, 0x22);

    for (int child = 0; child < CHILDREN; child++) {
      int pid = fork();
      if (pid < 0)
        fail("child fork failed");

      if (pid == 0) {
        char child_value = (char)(0x40 + child);
        char grand_value = (char)(0x60 + child);
        int grandchild = fork();

        if (grandchild < 0)
          fail("grandchild fork failed");

        if (grandchild == 0) {
          check_pages(p, bytes, 0x22);
          fill_pages(p, bytes, grand_value);
          check_pages(p, bytes, grand_value);
          exit(0);
        }

        check_pages(p, bytes, 0x22);
        fill_pages(p, bytes, child_value);
        wait_ok();
        check_pages(p, bytes, child_value);
        exit(0);
      }
    }

    fill_pages(p, bytes, 0x11);
    for (int child = 0; child < CHILDREN; child++)
      wait_ok();

    check_pages(p, bytes, 0x11);
    shrink(bytes);
  }
  printf("multi-generation: OK\n");
}

// read() writes into two still-shared pages via copyout(). The parent
// keeps its allocation unchanged and alive until the child has finished.
static void
pipe_copyout(void)
{
  int fd[2];
  int bytes = 3 * PGSIZE;
  int offset = PGSIZE - PIPE_BYTES / 2;
  char payload[PIPE_BYTES];
  char *p = grow(bytes);

  printf("copyout-cross-page: start\n");
  memset(p, 0x33, bytes);
  for (int i = 0; i < PIPE_BYTES; i++)
    payload[i] = (char)(0x50 + i);

  if (pipe(fd) < 0)
    fail("pipe failed");

  int pid = fork();
  if (pid < 0)
    fail("pipe fork failed");

  if (pid == 0) {
    int received = 0;
    close(fd[1]);

    while (received < PIPE_BYTES) {
      int n = read(fd[0], p + offset + received, PIPE_BYTES - received);
      if (n <= 0)
        fail("pipe read into COW pages failed");
      received += n;
    }
    close(fd[0]);

    for (int i = 0; i < bytes; i++) {
      char expected = 0x33;
      if (i >= offset && i < offset + PIPE_BYTES)
        expected = payload[i - offset];
      if (p[i] != expected)
        fail("copyout child contents incorrect");
    }
    exit(0);
  }

  close(fd[0]);
  if (write(fd[1], payload, PIPE_BYTES) != PIPE_BYTES)
    fail("pipe write failed");
  close(fd[1]);

  wait_ok();
  for (int i = 0; i < bytes; i++) {
    if (p[i] != 0x33)
      fail("copyout modified parent's shared buffer");
  }

  shrink(bytes);
  printf("copyout-cross-page: OK\n");
}

int
main(void)
{
  printf("=== COW Regression Tests ===\n");
  large_memory("large-memory");
  generations();
  pipe_copyout();

  // Reallocate the large region after the other tests to catch substantial
  // leaks. This is a regression check, not a proof of zero leaked pages.
  large_memory("reclaim");
  printf("ALL COW TESTS PASSED\n");
  exit(0);
}
