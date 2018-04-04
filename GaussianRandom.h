#ifndef GAUSSIAN_RANDOM_H
#define GAUSSIAN_RANDOM_H

#include <pthread.h>
#include <stdint.h>

class DoubleRandom {
 public:
  virtual ~DoubleRandom();

  // return [0..1.0)
  virtual double get() = 0;
  virtual void fill(double* dest, int count);
};

class ClibDoubleRandom : public DoubleRandom {
 public:
  ClibDoubleRandom();
  virtual double get();
};

class LockingDoubleRandomWrapper : public DoubleRandom {
 public:
  LockingDoubleRandomWrapper(DoubleRandom*);
  virtual ~LockingDoubleRandomWrapper();

  virtual double get();
  // one lock, many gets
  virtual void fill(double* dest, int count);

 private:
  DoubleRandom* source;
  pthread_mutex_t lock;
};

class BufferDoubleRandomWrapper : public DoubleRandom {
 public:
  BufferDoubleRandomWrapper(
      DoubleRandom* source_, int size_, bool delete_source_);
  virtual ~BufferDoubleRandomWrapper();

  virtual double get();

 private:
  DoubleRandom* source;
  double* buffer;
  int buffer_length;
  int pos;
  bool delete_source;
};

class FileDoubleRandom : public DoubleRandom {
 public:
  // /dev/urandom
  FileDoubleRandom();
  // use /dev/urandom or /dev/random, buffer_size in bytes
  FileDoubleRandom(const char* path, size_t buffer_size);
  virtual ~FileDoubleRandom();

  virtual double get();

 private:
  uint64_t* buffer;
  int buffer_count;
  int pos;
  int fd;
  char* name;

  void init();

  static const char* kDefaultPath;
  static const int kDefaultBufferSize;
};

class GaussianRandom {
 public:
  GaussianRandom();
  GaussianRandom(DoubleRandom* impl);
  double get();

 private:
  DoubleRandom* source;
  double y2;
  bool have_y2;
};


#endif /* GAUSSIAN_RANDOM_H */
