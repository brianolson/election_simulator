#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "GaussianRandom.h"

DoubleRandom::~DoubleRandom() {}

void DoubleRandom::fill(double* dest, int count) {
	for (int i = 0; i < count; ++i) {
		dest[i] = get();
	}
}

double ClibDoubleRandom::get() {
	return random() / 2147483647.0;
}

LockingDoubleRandomWrapper::LockingDoubleRandomWrapper(DoubleRandom* x)
	: source(x) {
	pthread_mutex_init(&lock, NULL);
}
LockingDoubleRandomWrapper::~LockingDoubleRandomWrapper() {
	pthread_mutex_destroy(&lock);
	if (source != NULL) {
		delete source;
	}
}

double LockingDoubleRandomWrapper::get() {
	double out;
	pthread_mutex_lock(&lock);
	out = source->get();
	pthread_mutex_unlock(&lock);
	return out;
}
void LockingDoubleRandomWrapper::fill(double* dest, int count) {
	pthread_mutex_lock(&lock);
	for (int i = 0; i < count; ++i) {
		dest[i] = source->get();
	}
	pthread_mutex_unlock(&lock);
}

BufferDoubleRandomWrapper::BufferDoubleRandomWrapper(
		DoubleRandom* source_, int size_, bool delete_source_)
	: source(source_), buffer_length(size_), delete_source(delete_source_) {
	assert(source != NULL);
	buffer = new double[buffer_length];
	assert(buffer);
	pos = buffer_length + 1;
}
BufferDoubleRandomWrapper::~BufferDoubleRandomWrapper() {
	delete buffer;
	if (delete_source) {
		delete source;
	}
}

double BufferDoubleRandomWrapper::get() {
	if (pos >= buffer_length) {
		source->fill(buffer, buffer_length);
		pos = 0;
	}
	double out = buffer[pos];
	pos++;
	return out;
}

const char* FileDoubleRandom::kDefaultPath = "/dev/urandom";
const int FileDoubleRandom::kDefaultBufferSize = 4096;

FileDoubleRandom::FileDoubleRandom() {
	name = strdup(kDefaultPath);
	buffer_count = kDefaultBufferSize / sizeof(uint64_t);
	init();
}
FileDoubleRandom::FileDoubleRandom(const char* path, size_t buffer_size) {
	name = strdup(path);
	buffer_count = buffer_size / sizeof(uint64_t);
	init();
}

void FileDoubleRandom::init() {
	fd = open(name, O_RDONLY);
	if (fd < 0) {
		return;
	}
	buffer = (uint64_t*)malloc(buffer_count * sizeof(uint64_t));
	if (buffer == NULL) {
		close(fd);
		fd = -1;
		return;
	}
	pos = buffer_count + 1;
}

FileDoubleRandom::~FileDoubleRandom() {
	if (fd > 0) {
		close(fd);
	}
	if (buffer != NULL) {
		free(buffer);
	}
}

namespace {
	static ssize_t readWithRetry(int fd, void* buf, size_t count, int retries) {
		ssize_t err;
		ssize_t total_read = 0;
		void* tbuf = buf;
		size_t countr = count;
		while (retries > 0) {
			retries--;
			err = read(fd, tbuf, countr);
			if (err < 0) {
				return err;
			}
			total_read += err;
			assert(total_read <= count);
			if (err < countr) {
				tbuf = (void*)(((uintptr_t)tbuf) + err);
				countr -= err;
			} else {
				return total_read;
			}
		}
		return total_read;
	}
} // namespace

double FileDoubleRandom::get() {
	if (pos >= buffer_count) {
		ssize_t desired_readlen = buffer_count * sizeof(*buffer);
		ssize_t readlen = readWithRetry(fd, buffer, desired_readlen, 50);
		if (readlen != desired_readlen) {
			fprintf(
					stderr,
					"read %zd bytes from \"%s\" failed, got %zd, errno=%d %s\n",
					desired_readlen, name, readlen, errno, strerror(errno));
			return NAN;
		}
		pos = 0;
	}
	double out = buffer[pos] / ((double)0xffffffffffffffff);
	pos++;
	return out;
}

GaussianRandom::GaussianRandom()
	: have_y2(false) {
	source = new ClibDoubleRandom();
}

GaussianRandom::GaussianRandom(DoubleRandom* impl)
	: source(impl), have_y2(false) {
	assert(source != NULL);
}

double GaussianRandom::get() {
	if (have_y2) {
		have_y2 = false;
		return y2;
	}
	double x1, x2, w;
	do {
		x1 = 2.0 * source->get() - 1.0;
		x2 = 2.0 * source->get() - 1.0;
		w = x1 * x1 + x2 * x2;
	} while ( w >= 1.0 );
	
	w = sqrt( (-2.0 * log( w ) ) / w );
	y2 = x2 * w;
	have_y2 = true;
	return x1 * w;
}
