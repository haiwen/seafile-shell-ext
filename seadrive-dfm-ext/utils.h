#ifndef SEADRIVEUTILS_H
#define SEADRIVEUTILS_H

#include <unistd.h>

class MutexLocker {
public:
  MutexLocker(pthread_mutex_t *mutex);
  ~MutexLocker();

private:
  pthread_mutex_t *mutex_;
};

gssize
pipe_write_n(int fd, const void *vptr, size_t n);

gssize
pipe_read_n(int fd, void *vptr, size_t n);

#endif
