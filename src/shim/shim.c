/* shim.c
 */

// Link with -ldl

#define _XOPEN_SOURCE 500   // for pread(2), pwrite(2)
#define _GNU_SOURCE         // for RTLD_NEXT in dlsym(2)
#include <sys/types.h>      // open(2)
#include <sys/stat.h>       // open(2)
#include <fcntl.h>          // open(2)
#include <unistd.h>         // pread(2), pwrite(2), read(2), write(2)
#include <dlfcn.h>          // dlsym(3)
#include <stdlib.h>         // exit(3)
#include <string.h>         // strncmp(3), strlen(3)
#include <stdio.h>          // sscanf(3)
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <asssert.h>
#include "../variorum/msr/msr_core.h"   // Various PATH #defines

typedef enum arch_t{
    ARCH_INTEL          = 0x0001;
    ARCH_INTEL_GPU      = 0x0002;
    ARCH_AMD            = 0x0004;
    ARCH_AMD_GPU        = 0x0008;
    ARCH_ARM            = 0x0010;
    ARCH_NVIDIA         = 0x0020;
    ARCH_IBM            = 0x0040;
};

typedef enum fd_t{
    FD_MSR_BATCH        = 0x0001;
    FD_MSR_VERSION      = 0x0002;
    FD_MSR_ALLOWLIST    = 0x0003;
    FD_MSR_SAFE_BASE    = 0x0100;
    FD_MSR_SAFE_MAX     = 0x01FF;   // 256 CPUs max
    FD_MSR_STOCK_BASE   = 0x0200;
    FD_MSR_STOCK_MAX    = 0x02FF;   // 256 CPUs max
    FD_ARRAY_MAX        = 0x0300;
    FD_BASE             = 0x1000;   // Add this to above for dummy fd
};

typedef enum status_t{
    STAT_CLOSED         = 0x0000;
    STAT_OPEN           = 0x0001;
};

typedef int     (*open_t)(const char *pathname, int flags, mode_t mode);
typedef int     (*close_t)(int fd);
typedef ssize_t (*pread_t)(int fd, void *buf, size_t count, off_t offset);
typedef ssize_t (*pwrite_t)(int fd, const void *buf, size_t count, off_t offset);
typedef int     (*ioctl_t)(int fd, unsigned long request, char *argp);// See [1]

static arch_t arch;
static open_t      actual_open;
static close_t     actual_close;
static pread_t     actual_pread;
static pwrite_t    actual_pwrite;
static ioctl_t     actual_ioctl;
static status_t fd_status[ FD_ARRAY_MAX ];    // Default to closed.

static bool is_init;

typedef enum sym_idx{
    SYM_OPEN    = 0,
    SYM_PREAD,
    SYM_PWRITE,
    SYM_CLOSE,
    SYM_IOCTL,
    NUM_SYMS
};
static char *symbols[NUM_SYMS] = {"open", "pread", "pwrite", "close", "ioctl"};


static void
init(void){
    arch = ARCH_INTEL;

    actual_open = dlsym(RTLD_NEXT, "open");
    if( NULL == actual_open ){
        fprintf( stderr, "%s:%d dlsym error: %s\n", __FILE__, __LINE__, dlerror());
        exit(-1);
    }

    actual_close = dlsym(RTLD_NEXT, "close");
    if( NULL == actual_close ){
        fprintf( stderr, "%s:%d dlsym error: %s\n", __FILE__, __LINE__, dlerror());
        exit(-1);
    }

    actual_pread = dlsym(RTLD_NEXT, "pread");
    if( NULL == actual_pread ){
        fprintf( stderr, "%s:%d dlsym error: %s\n", __FILE__, __LINE__, dlerror());
        exit(-1);
    }

    actual_pwrite = dlsym(RTLD_NEXT, "pwrite");
    if( NULL == actual_pwrite ){
        fprintf( stderr, "%s:%d dlsym error: %s\n", __FILE__, __LINE__, dlerror());
        exit(-1);
    }

    actual_ioctl = dlsym(RTLD_NEXT, "ioctl");
    if( NULL == actual_ioctl ){
        fprintf( stderr, "%s:%d dlsym error: %s\n", __FILE__, __LINE__, dlerror());
        exit(-1);
    }

    return;
}


int
open(const char *pathname, int flags, mode_t mode){
    int cpu = -1;
    char filename[1025];

    if( !is_init ){
        init();
        is_init = true;
    }

    if( arch & ARCH_INTEL ){
                    // msr-safe batch interface
        if(     ( 0 == strncmp( pathname, MSR_BATCH_PATH, strlen( MSR_BATCH_PATH ) ) )
            &&  ( strlen( pathname ) == strlen( MSR_BATCH_PATH ) )
        ){
            fd_status[ FD_MSR_BATCH ] = STAT_OPEN;
            return FD_BASE + FD_MSR_BATCH;
        }else if(   // Stock msr interface
               ( 0 == strncmp( path, "/dev/cpu/", sizeof("/dev/cpu/") ) )
            && ( 1 == sscanf ( path, "/dev/cpu/%d%1024s", &cpu, filename ) )
            && ( cpu < 256 )   // FIXME make this a #define
            && ( cpu > -1  )   // FIXME make this a #define
            && ( 0 == strncmp( filename, "/msr", strlen("/msr") ) )
        ){
            fd_status[ FD_MSR_STOCK_BASE + cpu ] = STAT_OPEN;
            return FD_BASE + FD_MSR_STOCK_BASE + cpu;
        }else if(  // msr-safe interface
               ( 0 == strncmp( path, "/dev/cpu/", sizeof("/dev/cpu/") ) )
            && ( 1 == sscanf ( path, "/dev/cpu/%d%1024s", &cpu, filename ) )
            && ( cpu < 256 )    // FIXME make this a #define
            && ( cpu > -1  )    // FIXME make this a #define
            && ( 0 == strncmp( filename, "/msr", strlen("/msr_safe") ) )
        ){
            fd_status[ FD_MSR_SAFE_BASE + cpu ] = STAT_OPEN;
            return FD_BASE + FD_MSR_SAFE_BASE + cpu;
        }else if(   // msr-safe version device
               ( 0 == strncmp( filename, "/dev/cpu/msr_safe_version", strlen("/dev/cpu/msr_safe_version") ) )
            && ( strlen( filename ) == strlen("/dev/cpu/msr_safe_version") )
        ){
            fd_status[ FD_MSR_VERSION ] = STAT_OPEN;
            return FD_BASE + FD_MSR_VERSION;
        }else if(   // msr-safe allow list
               ( 0 == strncmp( filename, MSR_ALLOWLIST_PATH, strlen(MSR_ALLOWLIST_PATH) ) )
            && ( strlen( filename ) == strlen(MSR_ALLOWLIST_PATH) )
        ){
            fd_status[ FD_MSR_ALLOWLIST ] = STAT_OPEN;
            return FD_BASE + FD_MSR_ALLOWLIST;
        }
    }
    // No matches, so don't hijack this call.
    return actual_open( pathname, flags, mode );

}


int
close( int fd ){
    if( !is_init ){
        init();\
            is_init = true;
    }
    if( (fd >= FD_BASE) && ( fd < FD_BASE + FD_ARRAY_MAX ) ){
        fd_status[ fd - FD_BASE ] = STAT_CLOSED;
        return 0;
    }
    // Not one of our file descriptors.
    return close( fd );
}

size_t
pread( int fd, void *buf, size_t count, off_t offset ){

}

size_t
pwrite( int fd, const void *buf, size_t count, off_t offset){

}

int
ioctl( int fd, unsigned long request, char *argp){

}

/* Notes
 *
 * [1] The signature for ioctl(2) is
 *      int ioctl( int fd, unsigned long request, ... );
 * However, the variadic notation exists just to defeat compiler type checking.
 * The actual call implemented in the kernel is:
 *      int ioctl( int fd, unsigned long request, char *argp );
 * See this Stack Overflow answer for details.
 * https://stackoverflow.com/questions/28462523/
 *  how-to-wrap-ioctlint-d-unsigned-long-request-using-ld-preload
 */
