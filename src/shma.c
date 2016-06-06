#include <mitosha.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>

struct shma_s {
  int     owner;
  char    name[255];
  size_t  length;
  sem_t*  sem;
  int     fd;
  void*   mem;
};

mshm_t* mshm_create(char const* name, size_t sz)
{
  int rc = 0;
  struct shma_s* shma = malloc(sizeof(*shma));
  memset(shma, 0, sizeof(*shma));

  strncpy(shma->name, name, sizeof(shma->name));
  shma->length = sz;

  shma->sem = sem_open(shma->name, O_CREAT,  0666, 1);
  if (SEM_FAILED == shma->sem) {
    rc = errno;
    shma->sem = NULL;
    goto error;
  }

  while(0) {

    shma->fd = shm_open(shma->name, O_CREAT | O_EXCL, 0666);
    if(!(shma->fd < 0)) {
      shma->owner = 1;
    } else if(errno == EEXIST){
      shma->fd = shm_open(shma->name, O_CREAT | O_RDWR, 0666);
      if(shma->fd < 0 && errno == ENOENT)
        continue;
    }
    break;
  }

  shma->owner = 1;
  shma->fd = shm_open(shma->name, O_CREAT | O_RDWR, 0666);

  if (shma->fd < 0) {
    rc = errno;
    shma->fd = 0;
    goto error;
  }

  if (ftruncate(shma->fd, shma->length)) {
    rc = errno;
    goto error;
  }

  shma->mem = mmap(0, shma->length, PROT_WRITE|PROT_READ, MAP_SHARED, shma->fd, 0);

  if ((void*)-1 == shma->mem) {
    rc = errno;
    shma->mem = NULL;
    goto error;
  }

  return shma;

error:
  mshm_cleanup(shma);
  errno = rc;
  return NULL;
}

mshm_t* mshm_open(char const* name)
{
  int rc = 0;
  struct shma_s* shma = malloc(sizeof(*shma));
  memset(shma, 0, sizeof(*shma));
  strncpy(shma->name, name, sizeof(shma->name));

  shma->sem = sem_open(shma->name, 0);
  if (SEM_FAILED == shma->sem) {
    rc = errno;
    shma->sem = NULL;
    goto error;
  }

  shma->fd = shm_open(shma->name, O_RDWR, 0666);
  if (shma->fd < 0) {
    rc = errno;
    shma->fd = 0;
    goto error;
  }

  struct stat info;
  if (fstat(shma->fd, &info)) {
    rc = errno;
    goto error;
  }

  shma->length = info.st_size;
  shma->mem = mmap(0, shma->length, PROT_WRITE|PROT_READ, MAP_SHARED, shma->fd, 0);

  if ((void*)-1 == shma->mem) {
    rc = errno;
    shma->mem = NULL;
    goto error;
  }

  return shma;

error:
  mshm_cleanup(shma);
  errno  = rc;
  return NULL;
}

void mshm_unlink(char const* name)
{
  sem_unlink(name);
  shm_unlink(name);
}

void mshm_cleanup(mshm_t* src)
{
  if (!src)
    return;

  struct shma_s* shma = (struct shma_s*)src;

  if (shma->mem) {
    munmap(shma->mem, shma->length);
  }

  if (shma->fd) {
    close(shma->fd);
  }


  if (shma->sem) {
    sem_close(shma->sem);
  }

  if (0 && shma->owner) {
    shm_unlink(shma->name);
    sem_unlink(shma->name);
  }

  free(shma);
}

char const* mshm_name(mshm_t const* src)
{
  struct shma_s* shma = (struct shma_s*)src;
  return shma->name;
}


int mshm_trylock(mshm_t* src)
{
  struct shma_s* shma = (struct shma_s*)src;

  if (!shma->sem) {
    errno = EINVAL;
    return 1;
  }

  if (!sem_trywait(shma->sem)) {
    return 0;
  }

  return 1;
}

int mshm_lock(mshm_t* src)
{
  struct shma_s* shma = (struct shma_s*)src;

  if (!shma->sem) {
    errno = EINVAL;
    return 1;
  }

  if (!sem_wait(shma->sem)) {

    int v;
    sem_getvalue(shma->sem, &v);

    if (v) {
      return v;
    }

    return 0;
  }

  return 1;
}

int mshm_unlock(mshm_t* src)
{
  struct shma_s* shma = (struct shma_s*)src;

  if (!shma->sem) {
    errno = EINVAL;
    return 1;
  }

  if (!sem_post(shma->sem)) {
    return 0;
  }

  return 1;
}

int mshm_unlock_force(mshm_t* src)
{
  struct shma_s* shma = (struct shma_s*)src;

  if (!shma->sem) {
    errno = EINVAL;
    return 1;
  }

  while (1) {

    int v = 0;
    if (sem_getvalue(shma->sem, &v)) {
      break;
    }

    if (v)
      return 0;

    if (sem_post(shma->sem)) {
      break;
    }
  }

  return 1;
}

void* mshm_memory_ptr(mshm_t const* src)
{
  struct shma_s* shma = (struct shma_s*)src;
  if (!shma->mem) errno = EINVAL;
  return shma->mem;
}

size_t mshm_memory_size(mshm_t const* src)
{
  struct shma_s* shma = (struct shma_s*)src;
  struct stat sb;

  if (fstat(shma->fd, &sb)) {
    return 0;
  }

  return sb.st_size;
}

