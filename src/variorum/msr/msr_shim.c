// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other
// // Variorum Project Developers. See the top-level LICENSE file for details.
// //
// // SPDX-License-Identifier: MIT

/* msr_fake.c
 * Compile:
 * gcc -Wextra -Werror -shared -fPIC -ldl -o msr_fake.so msr_fake.c
 *
 * Use:
 * LD_PRELOAD=./msr_fake.so ~/copperopolis/build/examples/variorum-print-frequency-example
 */


#define _GNU_SOURCE

#include <dlfcn.h>	// dlopen
#include <stdio.h>	// fprintf
#include <stdarg.h>	// va_start(), va_end()
#include <sys/types.h>	// open(), stat()
#include <sys/stat.h>	// open(), stat()
#include <fcntl.h>	// open()
#include <sys/ioctl.h>	// ioctl()
#include <unistd.h>	// close(), pread(), pwrite(), stat()

// Need pread, pwrite, ioctl, open, close, stat.

//
// typedefs
//
// int open(const char *pathname, int flags);
// int open(const char *pathname, int flags, mode_t mode);
typedef int (*real_open_t)( const char *pathname, int flags, ... );
// int close(int fd);
typedef int (*real_close_t)( int fd );
// ssize_t pread(int fd, void *buf, size_t count, off_t offset);
typedef int (*real_pread_t)( int fd, void *buf, size_t count, off_t offset );
// ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
typedef int (*real_pwrite_t)( int fd, const void *buf, size_t count, off_t offset );
// int ioctl(int fd, unsigned long request, ...);
typedef int (*real_ioctl_t)( int fd, unsigned long request, ... );
// int stat(const char *pathname, struct stat *statbuf);
typedef int (*real_stat_t)( const char *pathname, struct stat *statbuf );

//
// Pass-through calls
// Template provided by:
// http://www.goldsborough.me/c/low-level/kernel/2016/08/29/16-48-53-the_-ld_preload-_trick/
//

int
real_open( const char *pathname, int flags, mode_t mode ){
	return ((real_open_t)dlsym(RTLD_NEXT, "open"))( pathname, flags, mode );
}

int
real_close( int fd ){
	return ((real_close_t)dlsym(RTLD_NEXT, "close"))( fd );
}

ssize_t
real_pread( int fd, void *buf, size_t count, off_t offset ){
	return ((real_pread_t)dlsym(RTLD_NEXT, "pread"))( fd, buf, count, offset );
}

ssize_t
real_pwrite( int fd, const void *buf, size_t count, off_t offset ){
	return ((real_pwrite_t)dlsym(RTLD_NEXT, "pwrite"))( fd, buf, count, offset );
}

int
real_ioctl( int fd, unsigned long request, char *arg_p ){
	return ((real_ioctl_t)dlsym(RTLD_NEXT, "ioctl"))( fd, request, arg_p );
}

int
real_stat(const char *pathname, struct stat *statbuf){
	return ((real_stat_t)dlsym(RTLD_NEXT, "stat"))( pathname, statbuf );
}


//
// Interceptors
//
int
open( const char *pathname, int flags, ... ){

	mode_t mode=0;

	// Stanza taken from https://musl.libc.org
	if ((flags & O_CREAT) || (flags & O_TMPFILE) == O_TMPFILE) {
                va_list ap;
                va_start(ap, flags);
                mode = va_arg(ap, mode_t);
                va_end(ap);
        }
	fprintf( stdout, "BLR: %s:%d Intercepted open() call to %s\n",
			__FILE__, __LINE__, pathname );

	return real_open( pathname, flags, mode );
}


int
close(int fd){
	return real_close(fd);
}

ssize_t
pread(int fd, void *buf, size_t count, off_t offset){
	return real_pread( fd, buf, count, offset );
}

ssize_t
pwrite(int fd, const void *buf, size_t count, off_t offset){
	return real_pwrite( fd, buf, count, offset );
}

int
ioctl(int fd, unsigned long request, ...){
	char *arg_p = NULL;
	va_list ap;
	va_start(ap, request);
	arg_p = va_arg(ap, char*);
	va_end(ap);
	return real_ioctl( fd, request, arg_p );
}

int
stat(const char *pathname, struct stat *statbuf){
	return real_stat( pathname, statbuf );
}



