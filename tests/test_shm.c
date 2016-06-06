#include <mutest.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <mitosha.h>

void mu_test_shm_create() {
  mshm_t* r = mshm_create("tmp/filo", 10);
  mu_check(!r);

  r = mshm_create("testmitosha1", 10);
  mu_ensure(r);
  mu_check(!strcmp(mshm_name(r), "testmitosha1"));

  void* mem = mshm_memory_ptr(r);
  mu_check(mem);

  mu_check(mshm_memory_size(r) == 10);

  mshm_cleanup(r);
}

void mu_test_shm_lock() {
  mshm_t* r = mshm_create("mitosha", 64);
  mu_check(r);

  mu_check(!mshm_unlock_force(r));
  mu_check(!mshm_lock(r));
  mu_check(!mshm_unlock(r));
  mshm_cleanup(r);
}

void mu_test_shm_lock2() {
  mshm_t* r = mshm_create("mitosha", 64);
  mu_check(r);

  mshm_t* r2 = mshm_create("mitosha", 64);
  mu_check(r2);

  mu_check(0 == mshm_lock(r));
  mu_check(mshm_trylock(r2));
  mu_check(0 == mshm_unlock(r));
  mu_check(0 == mshm_trylock(r2));
  mu_check(mshm_trylock(r));
  mu_check(0 == mshm_unlock(r2));

  mshm_cleanup(r2);
  mshm_cleanup(r);
}

void mu_test_shm_fork() {
  pid_t pid;
  int status;

  if ((pid = fork()) > 0) {

    usleep(5000);

    if (waitpid(pid, &status, WNOHANG)) {
      perror("waitpid");
      mu_ensure(0);
    }

    usleep(25000);

  } else if (pid < 0) {
    perror("fork");
    mu_ensure(0);
  }

  if (!pid) {

    printf("child %d\n", getpid());

    mshm_t* r = mshm_create("mitosha", 64);
    mu_check(r);
    mu_check(!mshm_trylock(r));
    sprintf(mshm_memory_ptr(r), "Hello!");

    while (1) {
      usleep(100);
    }
  }

  usleep(20000);
  printf("parent %d\n", getpid());

  mshm_t* r = mshm_create("mitosha", 64);
  mu_check(r);
  mu_check(mshm_trylock(r));

  if (kill(pid, SIGTERM)) {
    perror("kill");
  } else {
    int status;
    waitpid(pid, &status, WNOHANG);
  }

  mu_check(!mshm_unlock_force(r));
  mu_check(!mshm_trylock(r));
  mu_check(0 == strcmp(mshm_memory_ptr(r), "Hello!"));
  mu_check(!mshm_unlock(r));
  mshm_cleanup(r);
}

void mu_test_shm_memory_ptr() {
  mshm_t* r = mshm_create("mitosha", 64);
  mu_check(r);

  mshm_t* r2 = mshm_create("mitosha", 128);
  mu_check(r2);

  mu_check(mshm_memory_ptr(r) != mshm_memory_ptr(r2));

  int rc = snprintf(mshm_memory_ptr(r), mshm_memory_size(r), "Hello World!");
  mu_check(rc == sizeof("Hello World!") - 1);
  mu_check(0 == memcmp(mshm_memory_ptr(r2), "Hello World!", rc));

  mshm_cleanup(r2);
  mshm_cleanup(r);
}

void mu_test_shm_memory_size() {
  mshm_unlink("mitosha");

  mshm_t* r = mshm_create("mitosha", 64);
  mu_check(r);

  mshm_t* r2 = mshm_create("mitosha", 128);
  mu_check(r2);

  mu_check(mshm_memory_size(r2) == 128);
  mu_check(mshm_memory_size(r) == 128);

  mshm_cleanup(r2);
  mshm_cleanup(r);
}

void mu_test_shm_open() {
  mshm_t* o = mshm_create("mitosha_test_open", 1024);
  mu_check(o);

  void* mem = mshm_memory_ptr(o);
  for (size_t i = 0; i < mshm_memory_size(o); ++i)
    ((char*) mem)[i] = i % 255;

  mshm_t* r = mshm_open("mitosha_test_open");
  mu_ensure(r);
  void* mem2 = mshm_memory_ptr(r);

  mu_check(mshm_memory_size(o) == mshm_memory_size(r));
  mu_check(!memcmp(mem, mem2, mshm_memory_size(o)));

  mshm_cleanup(r);
  mshm_cleanup(o);
}

void mu_test_shm_reopen_after_cleanup() {
  mshm_unlink("mitosha_reopen");

  mshm_t* r1 = mshm_create("mitosha_reopen", 128);
  mu_check(r1);
  mu_check(mshm_memory_size(r1) == 128);

  strcpy((char*) mshm_memory_ptr(r1), "REOPEN_TEST");
  mshm_cleanup(r1);

  mshm_t* r2 = mshm_open("mitosha_reopen");
  mu_check(r2);
  mu_check(mshm_memory_size(r2) == 128);
  mu_check(0 == strcmp((char*) mshm_memory_ptr(r2), "REOPEN_TEST"));

  mshm_cleanup(r2);
}
