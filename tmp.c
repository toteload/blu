typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;
typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;
typedef int8_t int_fast8_t;
typedef int16_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef int64_t int_fast64_t;
typedef uint8_t uint_fast8_t;
typedef uint16_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
typedef uint64_t uint_fast64_t;
typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef short __int16_t;
typedef unsigned short __uint16_t;
typedef int __int32_t;
typedef unsigned int __uint32_t;
typedef long long __int64_t;
typedef unsigned long long __uint64_t;
typedef long __darwin_intptr_t;
typedef unsigned int __darwin_natural_t;
typedef int __darwin_ct_rune_t;
typedef union {
 char __mbstate8[128];
 long long _mbstateL;
} __mbstate_t;
typedef __mbstate_t __darwin_mbstate_t;
typedef long int __darwin_ptrdiff_t;
typedef long unsigned int __darwin_size_t;
typedef __builtin_va_list __darwin_va_list;
typedef int __darwin_wchar_t;
typedef __darwin_wchar_t __darwin_rune_t;
typedef int __darwin_wint_t;
typedef unsigned long __darwin_clock_t;
typedef __uint32_t __darwin_socklen_t;
typedef long __darwin_ssize_t;
typedef long __darwin_time_t;
typedef __int64_t __darwin_blkcnt_t;
typedef __int32_t __darwin_blksize_t;
typedef __int32_t __darwin_dev_t;
typedef unsigned int __darwin_fsblkcnt_t;
typedef unsigned int __darwin_fsfilcnt_t;
typedef __uint32_t __darwin_gid_t;
typedef __uint32_t __darwin_id_t;
typedef __uint64_t __darwin_ino64_t;
typedef __darwin_ino64_t __darwin_ino_t;
typedef __darwin_natural_t __darwin_mach_port_name_t;
typedef __darwin_mach_port_name_t __darwin_mach_port_t;
typedef __uint16_t __darwin_mode_t;
typedef __int64_t __darwin_off_t;
typedef __int32_t __darwin_pid_t;
typedef __uint32_t __darwin_sigset_t;
typedef __int32_t __darwin_suseconds_t;
typedef __uint32_t __darwin_uid_t;
typedef __uint32_t __darwin_useconds_t;
typedef unsigned char __darwin_uuid_t[16];
typedef char __darwin_uuid_string_t[37];
struct __darwin_pthread_handler_rec {
 void (*__routine)(void *);
 void *__arg;
 struct __darwin_pthread_handler_rec *__next;
};
struct _opaque_pthread_attr_t {
 long __sig;
 char __opaque[56];
};
struct _opaque_pthread_cond_t {
 long __sig;
 char __opaque[40];
};
struct _opaque_pthread_condattr_t {
 long __sig;
 char __opaque[8];
};
struct _opaque_pthread_mutex_t {
 long __sig;
 char __opaque[56];
};
struct _opaque_pthread_mutexattr_t {
 long __sig;
 char __opaque[8];
};
struct _opaque_pthread_once_t {
 long __sig;
 char __opaque[8];
};
struct _opaque_pthread_rwlock_t {
 long __sig;
 char __opaque[192];
};
struct _opaque_pthread_rwlockattr_t {
 long __sig;
 char __opaque[16];
};
struct _opaque_pthread_t {
 long __sig;
 struct __darwin_pthread_handler_rec *__cleanup_stack;
 char __opaque[8176];
};
typedef struct _opaque_pthread_attr_t __darwin_pthread_attr_t;
typedef struct _opaque_pthread_cond_t __darwin_pthread_cond_t;
typedef struct _opaque_pthread_condattr_t __darwin_pthread_condattr_t;
typedef unsigned long __darwin_pthread_key_t;
typedef struct _opaque_pthread_mutex_t __darwin_pthread_mutex_t;
typedef struct _opaque_pthread_mutexattr_t __darwin_pthread_mutexattr_t;
typedef struct _opaque_pthread_once_t __darwin_pthread_once_t;
typedef struct _opaque_pthread_rwlock_t __darwin_pthread_rwlock_t;
typedef struct _opaque_pthread_rwlockattr_t __darwin_pthread_rwlockattr_t;
typedef struct _opaque_pthread_t *__darwin_pthread_t;
typedef __darwin_intptr_t intptr_t;
typedef unsigned long uintptr_t;
typedef long int intmax_t;
typedef long unsigned int uintmax_t;


typedef int __darwin_nl_item;
typedef int __darwin_wctrans_t;
typedef __uint32_t __darwin_wctype_t;
typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned int u_int32_t;
typedef unsigned long long u_int64_t;
typedef int64_t register_t;
typedef u_int64_t user_addr_t;
typedef u_int64_t user_size_t;
typedef int64_t user_ssize_t;
typedef int64_t user_long_t;
typedef u_int64_t user_ulong_t;
typedef int64_t user_time_t;
typedef int64_t user_off_t;
typedef u_int64_t syscall_arg_t;
typedef __darwin_va_list va_list;
typedef __darwin_size_t size_t;

int renameat(int, const char *, int, const char *) __attribute__((availability(macosx,introduced=10.10)));
int renamex_np(const char *, const char *, unsigned int) __attribute__((availability(macosx,introduced=10.12))) __attribute__((availability(ios,introduced=10.0))) __attribute__((availability(tvos,introduced=10.0))) __attribute__((availability(watchos,introduced=3.0)));
int renameatx_np(int, const char *, int, const char *, unsigned int) __attribute__((availability(macosx,introduced=10.12))) __attribute__((availability(ios,introduced=10.0))) __attribute__((availability(tvos,introduced=10.0))) __attribute__((availability(watchos,introduced=3.0)));
int printf(const char * restrict, ...) __attribute__((__format__ (__printf__, 1, 2)));
typedef __darwin_off_t fpos_t;
struct __sbuf {
 unsigned char * _base;
 int _size;
};
struct __sFILEX;
typedef struct __sFILE {
 unsigned char * _p;
 int _r;
 int _w;
 short _flags;
 short _file;
 struct __sbuf _bf;
 int _lbfsize;
 void *_cookie;
 int (* _Nullable _close)(void *);
 int (* _Nullable _read) (void *, char *, int __n);
 fpos_t (* _Nullable _seek) (void *, fpos_t, int);
 int (* _Nullable _write)(void *, const char *, int __n);
 struct __sbuf _ub;
 struct __sFILEX *_extra;
 int _ur;
 unsigned char _ubuf[3];
 unsigned char _nbuf[1];
 struct __sbuf _lb;
 int _blksize;
 fpos_t _offset;
} FILE;
extern FILE *__stdinp __attribute__((__swift_attr__("nonisolated(unsafe)")));
extern FILE *__stdoutp __attribute__((__swift_attr__("nonisolated(unsafe)")));
extern FILE *__stderrp __attribute__((__swift_attr__("nonisolated(unsafe)")));
void clearerr(FILE *);
int fclose(FILE *);
int feof(FILE *);
int ferror(FILE *);
int fflush(FILE *);
int fgetc(FILE *);
int fgetpos(FILE * restrict, fpos_t *);
char * fgets(char * restrict , int __size, FILE *);
FILE *fopen(const char * restrict __filename, const char * restrict __mode) __asm("_" "fopen" );
int fprintf(FILE * restrict, const char * restrict, ...) __attribute__((__format__ (__printf__, 2, 3)));
int fputc(int, FILE *);
int fputs(const char * restrict, FILE * restrict) __asm("_" "fputs" );
size_t fread(void * restrict __ptr, size_t __size, size_t __nitems, FILE * restrict __stream);
FILE *freopen(const char * restrict, const char * restrict,
     FILE * restrict) __asm("_" "freopen" );
int fscanf(FILE * restrict, const char * restrict, ...) __attribute__((__format__ (__scanf__, 2, 3)));
int fseek(FILE *, long, int);
int fsetpos(FILE *, const fpos_t *);
long ftell(FILE *);
size_t fwrite(const void * restrict __ptr, size_t __size, size_t __nitems, FILE * restrict __stream) __asm("_" "fwrite" );
int getc(FILE *);
int getchar(void);
__attribute__((__deprecated__("This function is provided for compatibility reasons only.  Due to security concerns inherent in the design of gets(3), it is highly recommended that you use fgets(3) instead.")))
char * gets(char *) ;
void perror(const char *) __attribute__((__cold__));
int putc(int, FILE *);
int putchar(int);
int puts(const char *);
int remove(const char *);
int rename (const char *__old, const char *__new);
void rewind(FILE *);
int scanf(const char * restrict, ...) __attribute__((__format__ (__scanf__, 1, 2)));
void setbuf(FILE * restrict, char * restrict );
int setvbuf(FILE * restrict, char * restrict , int, size_t __size);
__attribute__((__availability__(swift, unavailable, message="Use snprintf instead.")))
__attribute__((__deprecated__("This function is provided for compatibility reasons only.  Due to security concerns inherent in the design of sprintf(3), it is highly recommended that you use snprintf(3) instead.")))
int sprintf(char * restrict , const char * restrict, ...) __attribute__((__format__ (__printf__, 2, 3))) ;
int sscanf(const char * restrict, const char * restrict, ...) __attribute__((__format__ (__scanf__, 2, 3)));
FILE *tmpfile(void);
__attribute__((__availability__(swift, unavailable, message="Use mkstemp(3) instead.")))
__attribute__((__deprecated__("This function is provided for compatibility reasons only.  Due to security concerns inherent in the design of tmpnam(3), it is highly recommended that you use mkstemp(3) instead.")))
char * tmpnam(char *);
int ungetc(int, FILE *);
int vfprintf(FILE * restrict, const char * restrict, va_list) __attribute__((__format__ (__printf__, 2, 0)));
int vprintf(const char * restrict, va_list) __attribute__((__format__ (__printf__, 1, 0)));
__attribute__((__availability__(swift, unavailable, message="Use vsnprintf instead.")))
__attribute__((__deprecated__("This function is provided for compatibility reasons only.  Due to security concerns inherent in the design of sprintf(3), it is highly recommended that you use vsnprintf(3) instead.")))
int vsprintf(char * restrict , const char * restrict, va_list) __attribute__((__format__ (__printf__, 2, 0))) ;
char * ctermid(char *);
FILE *fdopen(int, const char *) __asm("_" "fdopen" );
int fileno(FILE *);
int pclose(FILE *) __attribute__((__availability__(swift, unavailable, message="Use posix_spawn APIs or NSTask instead. (On iOS, process spawning is unavailable.)")));
FILE *popen(const char *, const char *) __asm("_" "popen" ) __attribute__((__availability__(swift, unavailable, message="Use posix_spawn APIs or NSTask instead. (On iOS, process spawning is unavailable.)")));
int __srget(FILE *);
int __svfscanf(FILE *, const char *, va_list) __attribute__((__format__ (__scanf__, 2, 0)));
int __swbuf(int, FILE *);
inline __attribute__ ((__always_inline__)) int __sputc(int _c, FILE *_p) {
 if (--_p->_w >= 0 || (_p->_w >= _p->_lbfsize && (char)_c != '\n'))
  return (*_p->_p++ = _c);
 else
  return (__swbuf(_c, _p));
}
void flockfile(FILE *);
int ftrylockfile(FILE *);
void funlockfile(FILE *);
int getc_unlocked(FILE *);
int getchar_unlocked(void);
int putc_unlocked(int, FILE *);
int putchar_unlocked(int);
int getw(FILE *);
int putw(int, FILE *);
__attribute__((__availability__(swift, unavailable, message="Use mkstemp(3) instead.")))
__attribute__((__deprecated__("This function is provided for compatibility reasons only.  Due to security concerns inherent in the design of tempnam(3), it is highly recommended that you use mkstemp(3) instead.")))
char * tempnam(const char *__dir, const char *__prefix) __asm("_" "tempnam" );
typedef __darwin_off_t off_t;
int fseeko(FILE * __stream, off_t __offset, int __whence);
off_t ftello(FILE * __stream);
int snprintf(char * restrict __str, size_t __size, const char * restrict __format, ...) __attribute__((__format__ (__printf__, 3, 4)));
int vfscanf(FILE * restrict __stream, const char * restrict __format, va_list) __attribute__((__format__ (__scanf__, 2, 0)));
int vscanf(const char * restrict __format, va_list) __attribute__((__format__ (__scanf__, 1, 0)));
int vsnprintf(char * restrict __str, size_t __size, const char * restrict __format, va_list) __attribute__((__format__ (__printf__, 3, 0)));
int vsscanf(const char * restrict __str, const char * restrict __format, va_list) __attribute__((__format__ (__scanf__, 2, 0)));
typedef __darwin_ssize_t ssize_t;
int dprintf(int, const char * restrict, ...) __attribute__((__format__ (__printf__, 2, 3))) __attribute__((availability(macosx,introduced=10.7)));
int vdprintf(int, const char * restrict, va_list) __attribute__((__format__ (__printf__, 2, 0))) __attribute__((availability(macosx,introduced=10.7)));
ssize_t getdelim(char * *restrict __linep, size_t * restrict __linecapp, int __delimiter, FILE * restrict __stream) __attribute__((availability(macosx,introduced=10.7)));
ssize_t getline(char * *restrict __linep, size_t * restrict __linecapp, FILE * restrict __stream) __attribute__((availability(macosx,introduced=10.7)));
FILE *fmemopen(void * restrict __buf , size_t __size, const char * restrict __mode) __attribute__((availability(macos,introduced=10.13))) __attribute__((availability(ios,introduced=11.0))) __attribute__((availability(tvos,introduced=11.0))) __attribute__((availability(watchos,introduced=4.0)));
FILE *open_memstream(char * *__bufp, size_t *__sizep) __attribute__((availability(macos,introduced=10.13))) __attribute__((availability(ios,introduced=11.0))) __attribute__((availability(tvos,introduced=11.0))) __attribute__((availability(watchos,introduced=4.0)));
extern const int sys_nerr;
extern const char *const sys_errlist[];
int asprintf(char * *restrict, const char * restrict, ...) __attribute__((__format__ (__printf__, 2, 3)));
char * ctermid_r(char *);
char * fgetln(FILE *, size_t *__len);
const char *fmtcheck(const char *, const char *) __attribute__((format_arg(2)));
int fpurge(FILE *);
void setbuffer(FILE *, char *, int __size);
int setlinebuf(FILE *);
int vasprintf(char * *restrict, const char * restrict, va_list) __attribute__((__format__ (__printf__, 2, 0)));
FILE *funopen(const void *,
     int (* _Nullable)(void *, char *, int __n),
     int (* _Nullable)(void *, const char *, int __n),
     fpos_t (* _Nullable)(void *, fpos_t, int),
     int (* _Nullable)(void *));
extern int __snprintf_chk (char * restrict , size_t __maxlen, int, size_t,
     const char * restrict, ...);
extern int __vsnprintf_chk (char * restrict , size_t __maxlen, int, size_t,
     const char * restrict, va_list);
extern int __sprintf_chk (char * restrict , int, size_t,
     const char * restrict, ...);
extern int __vsprintf_chk (char * restrict , int, size_t,
     const char * restrict, va_list);
void *
  memchr(const void * __s, int __c, size_t __n);
int memcmp(const void * __s1, const void * __s2,
  size_t __n);
void *
  memcpy(void * __dst, const void * __src,
  size_t __n);
void *
  memmove(void * __dst,
  const void * __src, size_t __len);
void *
  memset(void * __b, int __c, size_t __len);
char *
  strcat(char * __s1, const char *__s2)
                                  ;
char * strchr(const char *__s, int __c);
int strcmp(const char *__s1, const char *__s2);
int strcoll(const char *__s1, const char *__s2);
char *
  strcpy(char * __dst, const char *__src)
                                  ;
size_t strcspn(const char *__s, const char *__charset);
char * strerror(int __errnum) __asm("_" "strerror" );
size_t strlen(const char *__s);
char *
  strncat(char * __s1,
  const char * __s2, size_t __n)
                                  ;
int strncmp(const char * __s1,
  const char * __s2, size_t __n);
char *
  strncpy(char * __dst,
        const char * __src, size_t __n)
                                        ;
char * strpbrk(const char *__s, const char *__charset);
char * strrchr(const char *__s, int __c);
size_t strspn(const char *__s, const char *__charset);
char * strstr(const char *__big, const char *__little);
char * strtok(char * __str, const char *__sep);
size_t strxfrm(char * __s1, const char *__s2, size_t __n);
char *
        strtok_r(char * __str, const char *__sep,
        char * *__lasts);
int strerror_r(int __errnum, char * __strerrbuf,
        size_t __buflen);
char * strdup(const char *__s1);
void *
        memccpy(void * __dst, const void * __src,
        int __c, size_t __n);
char *
        stpcpy(char * __dst, const char *__src) ;
char *
        stpncpy(char * __dst,
        const char * __src, size_t __n)
        __attribute__((availability(macosx,introduced=10.7)))
                                        ;
char * strndup(const char * __s1, size_t __n) __attribute__((availability(macosx,introduced=10.7)));
size_t strnlen(const char * __s1, size_t __n) __attribute__((availability(macosx,introduced=10.7)));
char * strsignal(int __sig);
typedef __darwin_size_t rsize_t;
typedef int errno_t;
errno_t memset_s(void * __s, rsize_t __smax, int __c, rsize_t __n) __attribute__((availability(macosx,introduced=10.9)));
void *
        memmem(const void * __big, size_t __big_len,
        const void * __little, size_t __little_len) __attribute__((availability(macosx,introduced=10.7)));
void memset_pattern4(void * __b, const void * __pattern4, size_t __len) __attribute__((availability(macosx,introduced=10.5)));
void memset_pattern8(void * __b, const void * __pattern8, size_t __len) __attribute__((availability(macosx,introduced=10.5)));
void memset_pattern16(void * __b, const void * __pattern16, size_t __len) __attribute__((availability(macosx,introduced=10.5)));
char *
        strcasestr(const char *__big, const char *__little);
__attribute__((availability(macosx,introduced=15.4))) __attribute__((availability(ios,introduced=18.4)))
__attribute__((availability(tvos,introduced=18.4))) __attribute__((availability(watchos,introduced=11.4)))
char *
        strchrnul(const char *__s, int __c);
char *
        strnstr(const char * __big, const char *__little, size_t __len);
size_t strlcat(char * __dst, const char *__source, size_t __size);
size_t strlcpy(char * __dst, const char *__source, size_t __size);
void strmode(int __mode, char * __bp);
char *
        strsep(char * *__stringp, const char *__delim);
void swab(const void * restrict, void * restrict, ssize_t __len);
__attribute__((availability(macosx,introduced=10.12.1))) __attribute__((availability(ios,introduced=10.1)))
__attribute__((availability(tvos,introduced=10.0.1))) __attribute__((availability(watchos,introduced=3.1)))
int timingsafe_bcmp(const void * __b1, const void * __b2, size_t __len);
__attribute__((availability(macosx,introduced=11.0))) __attribute__((availability(ios,introduced=14.0)))
__attribute__((availability(tvos,introduced=14.0))) __attribute__((availability(watchos,introduced=7.0)))
int strsignal_r(int __sig, char * __strsignalbuf, size_t __buflen);
int bcmp(const void *, const void *, size_t __n) ;
void bcopy(const void *, void *, size_t __n) ;
void bzero(void *, size_t __n) ;
char * index(const char *, int) ;
char * rindex(const char *, int) ;
int ffs(int);
int strcasecmp(const char *, const char *);
int strncasecmp(const char *, const char *, size_t);
int ffsl(long) __attribute__((availability(macosx,introduced=10.5)));
int ffsll(long long) __attribute__((availability(macosx,introduced=10.9)));
int fls(int) __attribute__((availability(macosx,introduced=10.5)));
int flsl(long) __attribute__((availability(macosx,introduced=10.5)));
int flsll(long long) __attribute__((availability(macosx,introduced=10.9)));
void __assert_rtn(const char *, const char *, int, const char *) __attribute__((__noreturn__)) __attribute__((__cold__)) __attribute__((__disable_tail_calls__));
typedef enum {
 P_ALL,
 P_PID,
 P_PGID
} idtype_t;
typedef __darwin_pid_t pid_t;
typedef __darwin_id_t id_t;
typedef int sig_atomic_t;
struct __darwin_i386_thread_state
{
    unsigned int __eax;
    unsigned int __ebx;
    unsigned int __ecx;
    unsigned int __edx;
    unsigned int __edi;
    unsigned int __esi;
    unsigned int __ebp;
    unsigned int __esp;
    unsigned int __ss;
    unsigned int __eflags;
    unsigned int __eip;
    unsigned int __cs;
    unsigned int __ds;
    unsigned int __es;
    unsigned int __fs;
    unsigned int __gs;
};
struct __darwin_fp_control
{
    unsigned short __invalid :1,
        __denorm :1,
    __zdiv :1,
    __ovrfl :1,
    __undfl :1,
    __precis :1,
      :2,
    __pc :2,
    __rc :2,
             :1,
      :3;
};
typedef struct __darwin_fp_control __darwin_fp_control_t;
struct __darwin_fp_status
{
    unsigned short __invalid :1,
        __denorm :1,
    __zdiv :1,
    __ovrfl :1,
    __undfl :1,
    __precis :1,
    __stkflt :1,
    __errsumm :1,
    __c0 :1,
    __c1 :1,
    __c2 :1,
    __tos :3,
    __c3 :1,
    __busy :1;
};
typedef struct __darwin_fp_status __darwin_fp_status_t;
struct __darwin_mmst_reg
{
 char __mmst_reg[10];
 char __mmst_rsrv[6];
};
struct __darwin_xmm_reg
{
 char __xmm_reg[16];
};
struct __darwin_ymm_reg
{
 char __ymm_reg[32];
};
struct __darwin_zmm_reg
{
 char __zmm_reg[64];
};
struct __darwin_opmask_reg
{
 char __opmask_reg[8];
};
struct __darwin_i386_float_state
{
 int __fpu_reserved[2];
 struct __darwin_fp_control __fpu_fcw;
 struct __darwin_fp_status __fpu_fsw;
 __uint8_t __fpu_ftw;
 __uint8_t __fpu_rsrv1;
 __uint16_t __fpu_fop;
 __uint32_t __fpu_ip;
 __uint16_t __fpu_cs;
 __uint16_t __fpu_rsrv2;
 __uint32_t __fpu_dp;
 __uint16_t __fpu_ds;
 __uint16_t __fpu_rsrv3;
 __uint32_t __fpu_mxcsr;
 __uint32_t __fpu_mxcsrmask;
 struct __darwin_mmst_reg __fpu_stmm0;
 struct __darwin_mmst_reg __fpu_stmm1;
 struct __darwin_mmst_reg __fpu_stmm2;
 struct __darwin_mmst_reg __fpu_stmm3;
 struct __darwin_mmst_reg __fpu_stmm4;
 struct __darwin_mmst_reg __fpu_stmm5;
 struct __darwin_mmst_reg __fpu_stmm6;
 struct __darwin_mmst_reg __fpu_stmm7;
 struct __darwin_xmm_reg __fpu_xmm0;
 struct __darwin_xmm_reg __fpu_xmm1;
 struct __darwin_xmm_reg __fpu_xmm2;
 struct __darwin_xmm_reg __fpu_xmm3;
 struct __darwin_xmm_reg __fpu_xmm4;
 struct __darwin_xmm_reg __fpu_xmm5;
 struct __darwin_xmm_reg __fpu_xmm6;
 struct __darwin_xmm_reg __fpu_xmm7;
 char __fpu_rsrv4[14*16];
 int __fpu_reserved1;
};
struct __darwin_i386_avx_state
{
 int __fpu_reserved[2];
 struct __darwin_fp_control __fpu_fcw;
 struct __darwin_fp_status __fpu_fsw;
 __uint8_t __fpu_ftw;
 __uint8_t __fpu_rsrv1;
 __uint16_t __fpu_fop;
 __uint32_t __fpu_ip;
 __uint16_t __fpu_cs;
 __uint16_t __fpu_rsrv2;
 __uint32_t __fpu_dp;
 __uint16_t __fpu_ds;
 __uint16_t __fpu_rsrv3;
 __uint32_t __fpu_mxcsr;
 __uint32_t __fpu_mxcsrmask;
 struct __darwin_mmst_reg __fpu_stmm0;
 struct __darwin_mmst_reg __fpu_stmm1;
 struct __darwin_mmst_reg __fpu_stmm2;
 struct __darwin_mmst_reg __fpu_stmm3;
 struct __darwin_mmst_reg __fpu_stmm4;
 struct __darwin_mmst_reg __fpu_stmm5;
 struct __darwin_mmst_reg __fpu_stmm6;
 struct __darwin_mmst_reg __fpu_stmm7;
 struct __darwin_xmm_reg __fpu_xmm0;
 struct __darwin_xmm_reg __fpu_xmm1;
 struct __darwin_xmm_reg __fpu_xmm2;
 struct __darwin_xmm_reg __fpu_xmm3;
 struct __darwin_xmm_reg __fpu_xmm4;
 struct __darwin_xmm_reg __fpu_xmm5;
 struct __darwin_xmm_reg __fpu_xmm6;
 struct __darwin_xmm_reg __fpu_xmm7;
 char __fpu_rsrv4[14*16];
 int __fpu_reserved1;
 char __avx_reserved1[64];
 struct __darwin_xmm_reg __fpu_ymmh0;
 struct __darwin_xmm_reg __fpu_ymmh1;
 struct __darwin_xmm_reg __fpu_ymmh2;
 struct __darwin_xmm_reg __fpu_ymmh3;
 struct __darwin_xmm_reg __fpu_ymmh4;
 struct __darwin_xmm_reg __fpu_ymmh5;
 struct __darwin_xmm_reg __fpu_ymmh6;
 struct __darwin_xmm_reg __fpu_ymmh7;
};
struct __darwin_i386_avx512_state
{
 int __fpu_reserved[2];
 struct __darwin_fp_control __fpu_fcw;
 struct __darwin_fp_status __fpu_fsw;
 __uint8_t __fpu_ftw;
 __uint8_t __fpu_rsrv1;
 __uint16_t __fpu_fop;
 __uint32_t __fpu_ip;
 __uint16_t __fpu_cs;
 __uint16_t __fpu_rsrv2;
 __uint32_t __fpu_dp;
 __uint16_t __fpu_ds;
 __uint16_t __fpu_rsrv3;
 __uint32_t __fpu_mxcsr;
 __uint32_t __fpu_mxcsrmask;
 struct __darwin_mmst_reg __fpu_stmm0;
 struct __darwin_mmst_reg __fpu_stmm1;
 struct __darwin_mmst_reg __fpu_stmm2;
 struct __darwin_mmst_reg __fpu_stmm3;
 struct __darwin_mmst_reg __fpu_stmm4;
 struct __darwin_mmst_reg __fpu_stmm5;
 struct __darwin_mmst_reg __fpu_stmm6;
 struct __darwin_mmst_reg __fpu_stmm7;
 struct __darwin_xmm_reg __fpu_xmm0;
 struct __darwin_xmm_reg __fpu_xmm1;
 struct __darwin_xmm_reg __fpu_xmm2;
 struct __darwin_xmm_reg __fpu_xmm3;
 struct __darwin_xmm_reg __fpu_xmm4;
 struct __darwin_xmm_reg __fpu_xmm5;
 struct __darwin_xmm_reg __fpu_xmm6;
 struct __darwin_xmm_reg __fpu_xmm7;
 char __fpu_rsrv4[14*16];
 int __fpu_reserved1;
 char __avx_reserved1[64];
 struct __darwin_xmm_reg __fpu_ymmh0;
 struct __darwin_xmm_reg __fpu_ymmh1;
 struct __darwin_xmm_reg __fpu_ymmh2;
 struct __darwin_xmm_reg __fpu_ymmh3;
 struct __darwin_xmm_reg __fpu_ymmh4;
 struct __darwin_xmm_reg __fpu_ymmh5;
 struct __darwin_xmm_reg __fpu_ymmh6;
 struct __darwin_xmm_reg __fpu_ymmh7;
 struct __darwin_opmask_reg __fpu_k0;
 struct __darwin_opmask_reg __fpu_k1;
 struct __darwin_opmask_reg __fpu_k2;
 struct __darwin_opmask_reg __fpu_k3;
 struct __darwin_opmask_reg __fpu_k4;
 struct __darwin_opmask_reg __fpu_k5;
 struct __darwin_opmask_reg __fpu_k6;
 struct __darwin_opmask_reg __fpu_k7;
 struct __darwin_ymm_reg __fpu_zmmh0;
 struct __darwin_ymm_reg __fpu_zmmh1;
 struct __darwin_ymm_reg __fpu_zmmh2;
 struct __darwin_ymm_reg __fpu_zmmh3;
 struct __darwin_ymm_reg __fpu_zmmh4;
 struct __darwin_ymm_reg __fpu_zmmh5;
 struct __darwin_ymm_reg __fpu_zmmh6;
 struct __darwin_ymm_reg __fpu_zmmh7;
};
struct __darwin_i386_exception_state
{
 __uint16_t __trapno;
 __uint16_t __cpu;
 __uint32_t __err;
 __uint32_t __faultvaddr;
};
struct __darwin_x86_debug_state32
{
 unsigned int __dr0;
 unsigned int __dr1;
 unsigned int __dr2;
 unsigned int __dr3;
 unsigned int __dr4;
 unsigned int __dr5;
 unsigned int __dr6;
 unsigned int __dr7;
};
struct __x86_instruction_state
{
        int __insn_stream_valid_bytes;
        int __insn_offset;
 int __out_of_synch;
        __uint8_t __insn_bytes[(2448 - 64 - 4)];
 __uint8_t __insn_cacheline[64];
};
struct __last_branch_record
{
 __uint64_t __from_ip;
 __uint64_t __to_ip;
 __uint32_t __mispredict : 1,
   __tsx_abort : 1,
   __in_tsx : 1,
   __cycle_count: 16,
   __reserved : 13;
};
struct __last_branch_state
{
        int __lbr_count;
 __uint32_t __lbr_supported_tsx : 1,
     __lbr_supported_cycle_count : 1,
     __reserved : 30;
 struct __last_branch_record __lbrs[32];
};
struct __x86_pagein_state
{
 int __pagein_error;
};
struct __darwin_x86_thread_state64
{
 __uint64_t __rax;
 __uint64_t __rbx;
 __uint64_t __rcx;
 __uint64_t __rdx;
 __uint64_t __rdi;
 __uint64_t __rsi;
 __uint64_t __rbp;
 __uint64_t __rsp;
 __uint64_t __r8;
 __uint64_t __r9;
 __uint64_t __r10;
 __uint64_t __r11;
 __uint64_t __r12;
 __uint64_t __r13;
 __uint64_t __r14;
 __uint64_t __r15;
 __uint64_t __rip;
 __uint64_t __rflags;
 __uint64_t __cs;
 __uint64_t __fs;
 __uint64_t __gs;
};
struct __darwin_x86_thread_full_state64
{
 struct __darwin_x86_thread_state64 __ss64;
 __uint64_t __ds;
 __uint64_t __es;
 __uint64_t __ss;
 __uint64_t __gsbase;
};
struct __darwin_x86_float_state64
{
 int __fpu_reserved[2];
 struct __darwin_fp_control __fpu_fcw;
 struct __darwin_fp_status __fpu_fsw;
 __uint8_t __fpu_ftw;
 __uint8_t __fpu_rsrv1;
 __uint16_t __fpu_fop;
 __uint32_t __fpu_ip;
 __uint16_t __fpu_cs;
 __uint16_t __fpu_rsrv2;
 __uint32_t __fpu_dp;
 __uint16_t __fpu_ds;
 __uint16_t __fpu_rsrv3;
 __uint32_t __fpu_mxcsr;
 __uint32_t __fpu_mxcsrmask;
 struct __darwin_mmst_reg __fpu_stmm0;
 struct __darwin_mmst_reg __fpu_stmm1;
 struct __darwin_mmst_reg __fpu_stmm2;
 struct __darwin_mmst_reg __fpu_stmm3;
 struct __darwin_mmst_reg __fpu_stmm4;
 struct __darwin_mmst_reg __fpu_stmm5;
 struct __darwin_mmst_reg __fpu_stmm6;
 struct __darwin_mmst_reg __fpu_stmm7;
 struct __darwin_xmm_reg __fpu_xmm0;
 struct __darwin_xmm_reg __fpu_xmm1;
 struct __darwin_xmm_reg __fpu_xmm2;
 struct __darwin_xmm_reg __fpu_xmm3;
 struct __darwin_xmm_reg __fpu_xmm4;
 struct __darwin_xmm_reg __fpu_xmm5;
 struct __darwin_xmm_reg __fpu_xmm6;
 struct __darwin_xmm_reg __fpu_xmm7;
 struct __darwin_xmm_reg __fpu_xmm8;
 struct __darwin_xmm_reg __fpu_xmm9;
 struct __darwin_xmm_reg __fpu_xmm10;
 struct __darwin_xmm_reg __fpu_xmm11;
 struct __darwin_xmm_reg __fpu_xmm12;
 struct __darwin_xmm_reg __fpu_xmm13;
 struct __darwin_xmm_reg __fpu_xmm14;
 struct __darwin_xmm_reg __fpu_xmm15;
 char __fpu_rsrv4[6*16];
 int __fpu_reserved1;
};
struct __darwin_x86_avx_state64
{
 int __fpu_reserved[2];
 struct __darwin_fp_control __fpu_fcw;
 struct __darwin_fp_status __fpu_fsw;
 __uint8_t __fpu_ftw;
 __uint8_t __fpu_rsrv1;
 __uint16_t __fpu_fop;
 __uint32_t __fpu_ip;
 __uint16_t __fpu_cs;
 __uint16_t __fpu_rsrv2;
 __uint32_t __fpu_dp;
 __uint16_t __fpu_ds;
 __uint16_t __fpu_rsrv3;
 __uint32_t __fpu_mxcsr;
 __uint32_t __fpu_mxcsrmask;
 struct __darwin_mmst_reg __fpu_stmm0;
 struct __darwin_mmst_reg __fpu_stmm1;
 struct __darwin_mmst_reg __fpu_stmm2;
 struct __darwin_mmst_reg __fpu_stmm3;
 struct __darwin_mmst_reg __fpu_stmm4;
 struct __darwin_mmst_reg __fpu_stmm5;
 struct __darwin_mmst_reg __fpu_stmm6;
 struct __darwin_mmst_reg __fpu_stmm7;
 struct __darwin_xmm_reg __fpu_xmm0;
 struct __darwin_xmm_reg __fpu_xmm1;
 struct __darwin_xmm_reg __fpu_xmm2;
 struct __darwin_xmm_reg __fpu_xmm3;
 struct __darwin_xmm_reg __fpu_xmm4;
 struct __darwin_xmm_reg __fpu_xmm5;
 struct __darwin_xmm_reg __fpu_xmm6;
 struct __darwin_xmm_reg __fpu_xmm7;
 struct __darwin_xmm_reg __fpu_xmm8;
 struct __darwin_xmm_reg __fpu_xmm9;
 struct __darwin_xmm_reg __fpu_xmm10;
 struct __darwin_xmm_reg __fpu_xmm11;
 struct __darwin_xmm_reg __fpu_xmm12;
 struct __darwin_xmm_reg __fpu_xmm13;
 struct __darwin_xmm_reg __fpu_xmm14;
 struct __darwin_xmm_reg __fpu_xmm15;
 char __fpu_rsrv4[6*16];
 int __fpu_reserved1;
 char __avx_reserved1[64];
 struct __darwin_xmm_reg __fpu_ymmh0;
 struct __darwin_xmm_reg __fpu_ymmh1;
 struct __darwin_xmm_reg __fpu_ymmh2;
 struct __darwin_xmm_reg __fpu_ymmh3;
 struct __darwin_xmm_reg __fpu_ymmh4;
 struct __darwin_xmm_reg __fpu_ymmh5;
 struct __darwin_xmm_reg __fpu_ymmh6;
 struct __darwin_xmm_reg __fpu_ymmh7;
 struct __darwin_xmm_reg __fpu_ymmh8;
 struct __darwin_xmm_reg __fpu_ymmh9;
 struct __darwin_xmm_reg __fpu_ymmh10;
 struct __darwin_xmm_reg __fpu_ymmh11;
 struct __darwin_xmm_reg __fpu_ymmh12;
 struct __darwin_xmm_reg __fpu_ymmh13;
 struct __darwin_xmm_reg __fpu_ymmh14;
 struct __darwin_xmm_reg __fpu_ymmh15;
};
struct __darwin_x86_avx512_state64
{
 int __fpu_reserved[2];
 struct __darwin_fp_control __fpu_fcw;
 struct __darwin_fp_status __fpu_fsw;
 __uint8_t __fpu_ftw;
 __uint8_t __fpu_rsrv1;
 __uint16_t __fpu_fop;
 __uint32_t __fpu_ip;
 __uint16_t __fpu_cs;
 __uint16_t __fpu_rsrv2;
 __uint32_t __fpu_dp;
 __uint16_t __fpu_ds;
 __uint16_t __fpu_rsrv3;
 __uint32_t __fpu_mxcsr;
 __uint32_t __fpu_mxcsrmask;
 struct __darwin_mmst_reg __fpu_stmm0;
 struct __darwin_mmst_reg __fpu_stmm1;
 struct __darwin_mmst_reg __fpu_stmm2;
 struct __darwin_mmst_reg __fpu_stmm3;
 struct __darwin_mmst_reg __fpu_stmm4;
 struct __darwin_mmst_reg __fpu_stmm5;
 struct __darwin_mmst_reg __fpu_stmm6;
 struct __darwin_mmst_reg __fpu_stmm7;
 struct __darwin_xmm_reg __fpu_xmm0;
 struct __darwin_xmm_reg __fpu_xmm1;
 struct __darwin_xmm_reg __fpu_xmm2;
 struct __darwin_xmm_reg __fpu_xmm3;
 struct __darwin_xmm_reg __fpu_xmm4;
 struct __darwin_xmm_reg __fpu_xmm5;
 struct __darwin_xmm_reg __fpu_xmm6;
 struct __darwin_xmm_reg __fpu_xmm7;
 struct __darwin_xmm_reg __fpu_xmm8;
 struct __darwin_xmm_reg __fpu_xmm9;
 struct __darwin_xmm_reg __fpu_xmm10;
 struct __darwin_xmm_reg __fpu_xmm11;
 struct __darwin_xmm_reg __fpu_xmm12;
 struct __darwin_xmm_reg __fpu_xmm13;
 struct __darwin_xmm_reg __fpu_xmm14;
 struct __darwin_xmm_reg __fpu_xmm15;
 char __fpu_rsrv4[6*16];
 int __fpu_reserved1;
 char __avx_reserved1[64];
 struct __darwin_xmm_reg __fpu_ymmh0;
 struct __darwin_xmm_reg __fpu_ymmh1;
 struct __darwin_xmm_reg __fpu_ymmh2;
 struct __darwin_xmm_reg __fpu_ymmh3;
 struct __darwin_xmm_reg __fpu_ymmh4;
 struct __darwin_xmm_reg __fpu_ymmh5;
 struct __darwin_xmm_reg __fpu_ymmh6;
 struct __darwin_xmm_reg __fpu_ymmh7;
 struct __darwin_xmm_reg __fpu_ymmh8;
 struct __darwin_xmm_reg __fpu_ymmh9;
 struct __darwin_xmm_reg __fpu_ymmh10;
 struct __darwin_xmm_reg __fpu_ymmh11;
 struct __darwin_xmm_reg __fpu_ymmh12;
 struct __darwin_xmm_reg __fpu_ymmh13;
 struct __darwin_xmm_reg __fpu_ymmh14;
 struct __darwin_xmm_reg __fpu_ymmh15;
 struct __darwin_opmask_reg __fpu_k0;
 struct __darwin_opmask_reg __fpu_k1;
 struct __darwin_opmask_reg __fpu_k2;
 struct __darwin_opmask_reg __fpu_k3;
 struct __darwin_opmask_reg __fpu_k4;
 struct __darwin_opmask_reg __fpu_k5;
 struct __darwin_opmask_reg __fpu_k6;
 struct __darwin_opmask_reg __fpu_k7;
 struct __darwin_ymm_reg __fpu_zmmh0;
 struct __darwin_ymm_reg __fpu_zmmh1;
 struct __darwin_ymm_reg __fpu_zmmh2;
 struct __darwin_ymm_reg __fpu_zmmh3;
 struct __darwin_ymm_reg __fpu_zmmh4;
 struct __darwin_ymm_reg __fpu_zmmh5;
 struct __darwin_ymm_reg __fpu_zmmh6;
 struct __darwin_ymm_reg __fpu_zmmh7;
 struct __darwin_ymm_reg __fpu_zmmh8;
 struct __darwin_ymm_reg __fpu_zmmh9;
 struct __darwin_ymm_reg __fpu_zmmh10;
 struct __darwin_ymm_reg __fpu_zmmh11;
 struct __darwin_ymm_reg __fpu_zmmh12;
 struct __darwin_ymm_reg __fpu_zmmh13;
 struct __darwin_ymm_reg __fpu_zmmh14;
 struct __darwin_ymm_reg __fpu_zmmh15;
 struct __darwin_zmm_reg __fpu_zmm16;
 struct __darwin_zmm_reg __fpu_zmm17;
 struct __darwin_zmm_reg __fpu_zmm18;
 struct __darwin_zmm_reg __fpu_zmm19;
 struct __darwin_zmm_reg __fpu_zmm20;
 struct __darwin_zmm_reg __fpu_zmm21;
 struct __darwin_zmm_reg __fpu_zmm22;
 struct __darwin_zmm_reg __fpu_zmm23;
 struct __darwin_zmm_reg __fpu_zmm24;
 struct __darwin_zmm_reg __fpu_zmm25;
 struct __darwin_zmm_reg __fpu_zmm26;
 struct __darwin_zmm_reg __fpu_zmm27;
 struct __darwin_zmm_reg __fpu_zmm28;
 struct __darwin_zmm_reg __fpu_zmm29;
 struct __darwin_zmm_reg __fpu_zmm30;
 struct __darwin_zmm_reg __fpu_zmm31;
};
struct __darwin_x86_exception_state64
{
    __uint16_t __trapno;
    __uint16_t __cpu;
    __uint32_t __err;
    __uint64_t __faultvaddr;
};
struct __darwin_x86_debug_state64
{
 __uint64_t __dr0;
 __uint64_t __dr1;
 __uint64_t __dr2;
 __uint64_t __dr3;
 __uint64_t __dr4;
 __uint64_t __dr5;
 __uint64_t __dr6;
 __uint64_t __dr7;
};
struct __darwin_x86_cpmu_state64
{
 __uint64_t __ctrs[16];
};
struct __darwin_mcontext32
{
 struct __darwin_i386_exception_state __es;
 struct __darwin_i386_thread_state __ss;
 struct __darwin_i386_float_state __fs;
};
struct __darwin_mcontext_avx32
{
 struct __darwin_i386_exception_state __es;
 struct __darwin_i386_thread_state __ss;
 struct __darwin_i386_avx_state __fs;
};
struct __darwin_mcontext_avx512_32
{
 struct __darwin_i386_exception_state __es;
 struct __darwin_i386_thread_state __ss;
 struct __darwin_i386_avx512_state __fs;
};
struct __darwin_mcontext64
{
 struct __darwin_x86_exception_state64 __es;
 struct __darwin_x86_thread_state64 __ss;
 struct __darwin_x86_float_state64 __fs;
};
struct __darwin_mcontext64_full
{
 struct __darwin_x86_exception_state64 __es;
 struct __darwin_x86_thread_full_state64 __ss;
 struct __darwin_x86_float_state64 __fs;
};
struct __darwin_mcontext_avx64
{
 struct __darwin_x86_exception_state64 __es;
 struct __darwin_x86_thread_state64 __ss;
 struct __darwin_x86_avx_state64 __fs;
};
struct __darwin_mcontext_avx64_full
{
 struct __darwin_x86_exception_state64 __es;
 struct __darwin_x86_thread_full_state64 __ss;
 struct __darwin_x86_avx_state64 __fs;
};
struct __darwin_mcontext_avx512_64
{
 struct __darwin_x86_exception_state64 __es;
 struct __darwin_x86_thread_state64 __ss;
 struct __darwin_x86_avx512_state64 __fs;
};
struct __darwin_mcontext_avx512_64_full
{
 struct __darwin_x86_exception_state64 __es;
 struct __darwin_x86_thread_full_state64 __ss;
 struct __darwin_x86_avx512_state64 __fs;
};
typedef struct __darwin_mcontext64 *mcontext_t;

typedef __darwin_pthread_attr_t pthread_attr_t;

struct __darwin_sigaltstack
{
 void *ss_sp;
 __darwin_size_t ss_size;
 int ss_flags;
};
typedef struct __darwin_sigaltstack stack_t;
struct __darwin_ucontext
{
 int uc_onstack;
 __darwin_sigset_t uc_sigmask;
 struct __darwin_sigaltstack uc_stack;
 struct __darwin_ucontext *uc_link;
 __darwin_size_t uc_mcsize;
 struct __darwin_mcontext64 *uc_mcontext;
};
typedef struct __darwin_ucontext ucontext_t;
typedef __darwin_sigset_t sigset_t;
typedef __darwin_uid_t uid_t;

union sigval {
 int sival_int;
 void *sival_ptr;
};
struct sigevent {
 int sigev_notify;
 int sigev_signo;
 union sigval sigev_value;
 void (*sigev_notify_function)(union sigval);
 pthread_attr_t *sigev_notify_attributes;
};
typedef struct __siginfo {
 int si_signo;
 int si_errno;
 int si_code;
 pid_t si_pid;
 uid_t si_uid;
 int si_status;
 void *si_addr;
 union sigval si_value;
 long si_band;
 unsigned long __pad[7];
} siginfo_t;
union __sigaction_u {
 void (*__sa_handler)(int);
 void (*__sa_sigaction)(int, struct __siginfo *,
     void *);
};
struct __sigaction {
 union __sigaction_u __sigaction_u;
 void (*sa_tramp)(void *, int, int, siginfo_t *, void *);
 sigset_t sa_mask;
 int sa_flags;
};
struct sigaction {
 union __sigaction_u __sigaction_u;
 sigset_t sa_mask;
 int sa_flags;
};
typedef void (*sig_t)(int);
struct sigvec {
 void (*sv_handler)(int);
 int sv_mask;
 int sv_flags;
};
struct sigstack {
 char *ss_sp;
 int ss_onstack;
};
void(*signal(int, void (*)(int)))(int);
struct timeval
{
 __darwin_time_t tv_sec;
 __darwin_suseconds_t tv_usec;
};
typedef __uint64_t rlim_t;
struct rusage {
 struct timeval ru_utime;
 struct timeval ru_stime;
 long ru_maxrss;
 long ru_ixrss;
 long ru_idrss;
 long ru_isrss;
 long ru_minflt;
 long ru_majflt;
 long ru_nswap;
 long ru_inblock;
 long ru_oublock;
 long ru_msgsnd;
 long ru_msgrcv;
 long ru_nsignals;
 long ru_nvcsw;
 long ru_nivcsw;
};
typedef void *rusage_info_t;
struct rusage_info_v0 {
 uint8_t ri_uuid[16];
 uint64_t ri_user_time;
 uint64_t ri_system_time;
 uint64_t ri_pkg_idle_wkups;
 uint64_t ri_interrupt_wkups;
 uint64_t ri_pageins;
 uint64_t ri_wired_size;
 uint64_t ri_resident_size;
 uint64_t ri_phys_footprint;
 uint64_t ri_proc_start_abstime;
 uint64_t ri_proc_exit_abstime;
};
struct rusage_info_v1 {
 uint8_t ri_uuid[16];
 uint64_t ri_user_time;
 uint64_t ri_system_time;
 uint64_t ri_pkg_idle_wkups;
 uint64_t ri_interrupt_wkups;
 uint64_t ri_pageins;
 uint64_t ri_wired_size;
 uint64_t ri_resident_size;
 uint64_t ri_phys_footprint;
 uint64_t ri_proc_start_abstime;
 uint64_t ri_proc_exit_abstime;
 uint64_t ri_child_user_time;
 uint64_t ri_child_system_time;
 uint64_t ri_child_pkg_idle_wkups;
 uint64_t ri_child_interrupt_wkups;
 uint64_t ri_child_pageins;
 uint64_t ri_child_elapsed_abstime;
};
struct rusage_info_v2 {
 uint8_t ri_uuid[16];
 uint64_t ri_user_time;
 uint64_t ri_system_time;
 uint64_t ri_pkg_idle_wkups;
 uint64_t ri_interrupt_wkups;
 uint64_t ri_pageins;
 uint64_t ri_wired_size;
 uint64_t ri_resident_size;
 uint64_t ri_phys_footprint;
 uint64_t ri_proc_start_abstime;
 uint64_t ri_proc_exit_abstime;
 uint64_t ri_child_user_time;
 uint64_t ri_child_system_time;
 uint64_t ri_child_pkg_idle_wkups;
 uint64_t ri_child_interrupt_wkups;
 uint64_t ri_child_pageins;
 uint64_t ri_child_elapsed_abstime;
 uint64_t ri_diskio_bytesread;
 uint64_t ri_diskio_byteswritten;
};
struct rusage_info_v3 {
 uint8_t ri_uuid[16];
 uint64_t ri_user_time;
 uint64_t ri_system_time;
 uint64_t ri_pkg_idle_wkups;
 uint64_t ri_interrupt_wkups;
 uint64_t ri_pageins;
 uint64_t ri_wired_size;
 uint64_t ri_resident_size;
 uint64_t ri_phys_footprint;
 uint64_t ri_proc_start_abstime;
 uint64_t ri_proc_exit_abstime;
 uint64_t ri_child_user_time;
 uint64_t ri_child_system_time;
 uint64_t ri_child_pkg_idle_wkups;
 uint64_t ri_child_interrupt_wkups;
 uint64_t ri_child_pageins;
 uint64_t ri_child_elapsed_abstime;
 uint64_t ri_diskio_bytesread;
 uint64_t ri_diskio_byteswritten;
 uint64_t ri_cpu_time_qos_default;
 uint64_t ri_cpu_time_qos_maintenance;
 uint64_t ri_cpu_time_qos_background;
 uint64_t ri_cpu_time_qos_utility;
 uint64_t ri_cpu_time_qos_legacy;
 uint64_t ri_cpu_time_qos_user_initiated;
 uint64_t ri_cpu_time_qos_user_interactive;
 uint64_t ri_billed_system_time;
 uint64_t ri_serviced_system_time;
};
struct rusage_info_v4 {
 uint8_t ri_uuid[16];
 uint64_t ri_user_time;
 uint64_t ri_system_time;
 uint64_t ri_pkg_idle_wkups;
 uint64_t ri_interrupt_wkups;
 uint64_t ri_pageins;
 uint64_t ri_wired_size;
 uint64_t ri_resident_size;
 uint64_t ri_phys_footprint;
 uint64_t ri_proc_start_abstime;
 uint64_t ri_proc_exit_abstime;
 uint64_t ri_child_user_time;
 uint64_t ri_child_system_time;
 uint64_t ri_child_pkg_idle_wkups;
 uint64_t ri_child_interrupt_wkups;
 uint64_t ri_child_pageins;
 uint64_t ri_child_elapsed_abstime;
 uint64_t ri_diskio_bytesread;
 uint64_t ri_diskio_byteswritten;
 uint64_t ri_cpu_time_qos_default;
 uint64_t ri_cpu_time_qos_maintenance;
 uint64_t ri_cpu_time_qos_background;
 uint64_t ri_cpu_time_qos_utility;
 uint64_t ri_cpu_time_qos_legacy;
 uint64_t ri_cpu_time_qos_user_initiated;
 uint64_t ri_cpu_time_qos_user_interactive;
 uint64_t ri_billed_system_time;
 uint64_t ri_serviced_system_time;
 uint64_t ri_logical_writes;
 uint64_t ri_lifetime_max_phys_footprint;
 uint64_t ri_instructions;
 uint64_t ri_cycles;
 uint64_t ri_billed_energy;
 uint64_t ri_serviced_energy;
 uint64_t ri_interval_max_phys_footprint;
 uint64_t ri_runnable_time;
};
struct rusage_info_v5 {
 uint8_t ri_uuid[16];
 uint64_t ri_user_time;
 uint64_t ri_system_time;
 uint64_t ri_pkg_idle_wkups;
 uint64_t ri_interrupt_wkups;
 uint64_t ri_pageins;
 uint64_t ri_wired_size;
 uint64_t ri_resident_size;
 uint64_t ri_phys_footprint;
 uint64_t ri_proc_start_abstime;
 uint64_t ri_proc_exit_abstime;
 uint64_t ri_child_user_time;
 uint64_t ri_child_system_time;
 uint64_t ri_child_pkg_idle_wkups;
 uint64_t ri_child_interrupt_wkups;
 uint64_t ri_child_pageins;
 uint64_t ri_child_elapsed_abstime;
 uint64_t ri_diskio_bytesread;
 uint64_t ri_diskio_byteswritten;
 uint64_t ri_cpu_time_qos_default;
 uint64_t ri_cpu_time_qos_maintenance;
 uint64_t ri_cpu_time_qos_background;
 uint64_t ri_cpu_time_qos_utility;
 uint64_t ri_cpu_time_qos_legacy;
 uint64_t ri_cpu_time_qos_user_initiated;
 uint64_t ri_cpu_time_qos_user_interactive;
 uint64_t ri_billed_system_time;
 uint64_t ri_serviced_system_time;
 uint64_t ri_logical_writes;
 uint64_t ri_lifetime_max_phys_footprint;
 uint64_t ri_instructions;
 uint64_t ri_cycles;
 uint64_t ri_billed_energy;
 uint64_t ri_serviced_energy;
 uint64_t ri_interval_max_phys_footprint;
 uint64_t ri_runnable_time;
 uint64_t ri_flags;
};
struct rusage_info_v6 {
 uint8_t ri_uuid[16];
 uint64_t ri_user_time;
 uint64_t ri_system_time;
 uint64_t ri_pkg_idle_wkups;
 uint64_t ri_interrupt_wkups;
 uint64_t ri_pageins;
 uint64_t ri_wired_size;
 uint64_t ri_resident_size;
 uint64_t ri_phys_footprint;
 uint64_t ri_proc_start_abstime;
 uint64_t ri_proc_exit_abstime;
 uint64_t ri_child_user_time;
 uint64_t ri_child_system_time;
 uint64_t ri_child_pkg_idle_wkups;
 uint64_t ri_child_interrupt_wkups;
 uint64_t ri_child_pageins;
 uint64_t ri_child_elapsed_abstime;
 uint64_t ri_diskio_bytesread;
 uint64_t ri_diskio_byteswritten;
 uint64_t ri_cpu_time_qos_default;
 uint64_t ri_cpu_time_qos_maintenance;
 uint64_t ri_cpu_time_qos_background;
 uint64_t ri_cpu_time_qos_utility;
 uint64_t ri_cpu_time_qos_legacy;
 uint64_t ri_cpu_time_qos_user_initiated;
 uint64_t ri_cpu_time_qos_user_interactive;
 uint64_t ri_billed_system_time;
 uint64_t ri_serviced_system_time;
 uint64_t ri_logical_writes;
 uint64_t ri_lifetime_max_phys_footprint;
 uint64_t ri_instructions;
 uint64_t ri_cycles;
 uint64_t ri_billed_energy;
 uint64_t ri_serviced_energy;
 uint64_t ri_interval_max_phys_footprint;
 uint64_t ri_runnable_time;
 uint64_t ri_flags;
 uint64_t ri_user_ptime;
 uint64_t ri_system_ptime;
 uint64_t ri_pinstructions;
 uint64_t ri_pcycles;
 uint64_t ri_energy_nj;
 uint64_t ri_penergy_nj;
 uint64_t ri_secure_time_in_system;
 uint64_t ri_secure_ptime_in_system;
 uint64_t ri_neural_footprint;
 uint64_t ri_lifetime_max_neural_footprint;
 uint64_t ri_interval_max_neural_footprint;
 uint64_t ri_reserved[9];
};
typedef struct rusage_info_v6 rusage_info_current;
struct rlimit {
 rlim_t rlim_cur;
 rlim_t rlim_max;
};
struct proc_rlimit_control_wakeupmon {
 uint32_t wm_flags;
 int32_t wm_rate;
};
int getpriority(int, id_t);
int getiopolicy_np(int, int) __attribute__((availability(macosx,introduced=10.5)));
int getrlimit(int, struct rlimit *) __asm("_" "getrlimit" );
int getrusage(int, struct rusage *);
int setpriority(int, id_t, int);
int setiopolicy_np(int, int, int) __attribute__((availability(macosx,introduced=10.5)));
int setrlimit(int, const struct rlimit *) __asm("_" "setrlimit" );
static inline
__uint16_t
_OSSwapInt16(
 __uint16_t _data
 )
{
 return (__uint16_t)((_data << 8) | (_data >> 8));
}
static inline
__uint32_t
_OSSwapInt32(
 __uint32_t _data
 )
{
 return __builtin_bswap32(_data);
}
static inline
__uint64_t
_OSSwapInt64(
 __uint64_t _data
 )
{
 return __builtin_bswap64(_data);
}
union wait {
 int w_status;
 struct {
  unsigned int w_Termsig:7,
      w_Coredump:1,
      w_Retcode:8,
      w_Filler:16;
 } w_T;
 struct {
  unsigned int w_Stopval:8,
      w_Stopsig:8,
      w_Filler:16;
 } w_S;
};
pid_t wait(int *) __asm("_" "wait" );
pid_t waitpid(pid_t, int *, int) __asm("_" "waitpid" );
int waitid(idtype_t, id_t, siginfo_t *, int) __asm("_" "waitid" );
pid_t wait3(int *, int, struct rusage *);
pid_t wait4(pid_t, int *, int, struct rusage *);

void * alloca(size_t __size);
typedef __darwin_ct_rune_t ct_rune_t;
typedef __darwin_rune_t rune_t;
typedef __darwin_wchar_t wchar_t;
typedef struct {
 int quot;
 int rem;
} div_t;
typedef struct {
 long quot;
 long rem;
} ldiv_t;
typedef struct {
 long long quot;
 long long rem;
} lldiv_t;
extern int __mb_cur_max;
typedef unsigned long long malloc_type_id_t;

__attribute__((availability(macos,introduced=14.0))) __attribute__((availability(ios,introduced=17.0))) __attribute__((availability(tvos,introduced=17.0))) __attribute__((availability(watchos,introduced=10.0))) __attribute__((availability(visionos,introduced=1.0))) __attribute__((availability(driverkit,introduced=23.0)))
void * malloc_type_malloc(size_t size, malloc_type_id_t type_id) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(1)));
__attribute__((availability(macos,introduced=14.0))) __attribute__((availability(ios,introduced=17.0))) __attribute__((availability(tvos,introduced=17.0))) __attribute__((availability(watchos,introduced=10.0))) __attribute__((availability(visionos,introduced=1.0))) __attribute__((availability(driverkit,introduced=23.0)))
void * malloc_type_calloc(size_t count, size_t size, malloc_type_id_t type_id) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(1,2)));
__attribute__((availability(macos,introduced=14.0))) __attribute__((availability(ios,introduced=17.0))) __attribute__((availability(tvos,introduced=17.0))) __attribute__((availability(watchos,introduced=10.0))) __attribute__((availability(visionos,introduced=1.0))) __attribute__((availability(driverkit,introduced=23.0)))
void malloc_type_free(void * ptr, malloc_type_id_t type_id);
__attribute__((availability(macos,introduced=14.0))) __attribute__((availability(ios,introduced=17.0))) __attribute__((availability(tvos,introduced=17.0))) __attribute__((availability(watchos,introduced=10.0))) __attribute__((availability(visionos,introduced=1.0))) __attribute__((availability(driverkit,introduced=23.0)))
void * malloc_type_realloc(void * ptr, size_t size, malloc_type_id_t type_id) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(2)));
__attribute__((availability(macos,introduced=14.0))) __attribute__((availability(ios,introduced=17.0))) __attribute__((availability(tvos,introduced=17.0))) __attribute__((availability(watchos,introduced=10.0))) __attribute__((availability(visionos,introduced=1.0))) __attribute__((availability(driverkit,introduced=23.0)))
void * malloc_type_valloc(size_t size, malloc_type_id_t type_id) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(1)));
__attribute__((availability(macos,introduced=14.0))) __attribute__((availability(ios,introduced=17.0))) __attribute__((availability(tvos,introduced=17.0))) __attribute__((availability(watchos,introduced=10.0))) __attribute__((availability(visionos,introduced=1.0))) __attribute__((availability(driverkit,introduced=23.0)))
void * malloc_type_aligned_alloc(size_t alignment, size_t size, malloc_type_id_t type_id) __attribute__((__warn_unused_result__)) __attribute__((alloc_align(1))) __attribute__((alloc_size(2)));
__attribute__((availability(macos,introduced=14.0))) __attribute__((availability(ios,introduced=17.0))) __attribute__((availability(tvos,introduced=17.0))) __attribute__((availability(watchos,introduced=10.0))) __attribute__((availability(visionos,introduced=1.0))) __attribute__((availability(driverkit,introduced=23.0)))
int malloc_type_posix_memalign(void * *memptr, size_t alignment, size_t size, malloc_type_id_t type_id) ;
typedef struct _malloc_zone_t malloc_zone_t;
__attribute__((availability(macos,introduced=14.0))) __attribute__((availability(ios,introduced=17.0))) __attribute__((availability(tvos,introduced=17.0))) __attribute__((availability(watchos,introduced=10.0))) __attribute__((availability(visionos,introduced=1.0))) __attribute__((availability(driverkit,introduced=23.0)))
void * malloc_type_zone_malloc(malloc_zone_t *zone, size_t size, malloc_type_id_t type_id) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(2)));
__attribute__((availability(macos,introduced=14.0))) __attribute__((availability(ios,introduced=17.0))) __attribute__((availability(tvos,introduced=17.0))) __attribute__((availability(watchos,introduced=10.0))) __attribute__((availability(visionos,introduced=1.0))) __attribute__((availability(driverkit,introduced=23.0)))
void * malloc_type_zone_calloc(malloc_zone_t *zone, size_t count, size_t size, malloc_type_id_t type_id) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(2,3)));
__attribute__((availability(macos,introduced=14.0))) __attribute__((availability(ios,introduced=17.0))) __attribute__((availability(tvos,introduced=17.0))) __attribute__((availability(watchos,introduced=10.0))) __attribute__((availability(visionos,introduced=1.0))) __attribute__((availability(driverkit,introduced=23.0)))
void malloc_type_zone_free(malloc_zone_t *zone, void * ptr, malloc_type_id_t type_id);
__attribute__((availability(macos,introduced=14.0))) __attribute__((availability(ios,introduced=17.0))) __attribute__((availability(tvos,introduced=17.0))) __attribute__((availability(watchos,introduced=10.0))) __attribute__((availability(visionos,introduced=1.0))) __attribute__((availability(driverkit,introduced=23.0)))
void * malloc_type_zone_realloc(malloc_zone_t *zone, void * ptr, size_t size, malloc_type_id_t type_id) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(3)));
__attribute__((availability(macos,introduced=14.0))) __attribute__((availability(ios,introduced=17.0))) __attribute__((availability(tvos,introduced=17.0))) __attribute__((availability(watchos,introduced=10.0))) __attribute__((availability(visionos,introduced=1.0))) __attribute__((availability(driverkit,introduced=23.0)))
void * malloc_type_zone_valloc(malloc_zone_t *zone, size_t size, malloc_type_id_t type_id) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(2)));
__attribute__((availability(macos,introduced=14.0))) __attribute__((availability(ios,introduced=17.0))) __attribute__((availability(tvos,introduced=17.0))) __attribute__((availability(watchos,introduced=10.0))) __attribute__((availability(visionos,introduced=1.0))) __attribute__((availability(driverkit,introduced=23.0)))
void * malloc_type_zone_memalign(malloc_zone_t *zone, size_t alignment, size_t size, malloc_type_id_t type_id) __attribute__((__warn_unused_result__)) __attribute__((alloc_align(2))) __attribute__((alloc_size(3)));
void * malloc(size_t __size) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(1))) ;
void * calloc(size_t __count, size_t __size) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(1,2))) ;
void free(void * );
void * realloc(void * __ptr, size_t __size) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(2))) ;
void * reallocf(void * __ptr, size_t __size) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(2)));
void * valloc(size_t __size) __attribute__((__warn_unused_result__)) __attribute__((alloc_size(1))) ;
void * aligned_alloc(size_t __alignment, size_t __size) __attribute__((__warn_unused_result__)) __attribute__((alloc_align(1))) __attribute__((alloc_size(2))) __attribute__((availability(macosx,introduced=10.15))) __attribute__((availability(ios,introduced=13.0))) __attribute__((availability(tvos,introduced=13.0))) __attribute__((availability(watchos,introduced=6.0)));
int posix_memalign(void * *__memptr, size_t __alignment, size_t __size) __attribute__((availability(macosx,introduced=10.6)));
void abort(void) __attribute__((__cold__)) __attribute__((__noreturn__));
int abs(int) __attribute__((__const__));
int atexit(void (* _Nonnull)(void));
int at_quick_exit(void (*)(void));
double atof(const char *);
int atoi(const char *);
long atol(const char *);
long long
  atoll(const char *);
void *bsearch(const void * __key, const void * __base, size_t __nel,
     size_t __width, int (* _Nonnull __compar)(const void *, const void *));
div_t div(int, int) __attribute__((__const__));
void exit(int) __attribute__((__noreturn__));
char * getenv(const char *);
long labs(long) __attribute__((__const__));
ldiv_t ldiv(long, long) __attribute__((__const__));
long long
  llabs(long long);
lldiv_t lldiv(long long, long long);
int mblen(const char * __s, size_t __n);
size_t mbstowcs(wchar_t * restrict , const char * restrict, size_t __n);
int mbtowc(wchar_t * restrict, const char * restrict , size_t __n);
void qsort(void * __base, size_t __nel, size_t __width,
     int (* _Nonnull __compar)(const void *, const void *));
void quick_exit(int) __attribute__((__noreturn__));
int rand(void) __attribute__((__availability__(swift, unavailable, message="Use arc4random instead.")));
void srand(unsigned) __attribute__((__availability__(swift, unavailable, message="Use arc4random instead.")));
double strtod(const char *, char * *) __asm("_" "strtod" );
float strtof(const char *, char * *) __asm("_" "strtof" );
long strtol(const char *__str, char * *__endptr, int __base);
long double
  strtold(const char *, char * *);
long long
  strtoll(const char *__str, char * *__endptr, int __base);
unsigned long
  strtoul(const char *__str, char * *__endptr, int __base);
unsigned long long
  strtoull(const char *__str, char * *__endptr, int __base);
__attribute__((__availability__(swift, unavailable, message="Use posix_spawn APIs or NSTask instead. (On iOS, process spawning is unavailable.)")))
__attribute__((availability(macos,introduced=10.0))) __attribute__((availability(ios,unavailable)))
__attribute__((availability(watchos,unavailable))) __attribute__((availability(tvos,unavailable)))
int system(const char *) __asm("_" "system" );
size_t wcstombs(char * restrict , const wchar_t * restrict, size_t __n);
int wctomb(char *, wchar_t);
void _Exit(int) __attribute__((__noreturn__));
long a64l(const char *);
double drand48(void);
char * ecvt(double, int, int *restrict, int *restrict);
double erand48(unsigned short[3]);
char * fcvt(double, int, int *restrict, int *restrict);
char * gcvt(double, int, char *) ;
int getsubopt(char * *, char * const *, char * *);
int grantpt(int);
char *
  initstate(unsigned, char *, size_t __size);
long jrand48(unsigned short[3]) __attribute__((__availability__(swift, unavailable, message="Use arc4random instead.")));
char *l64a(long);
void lcong48(unsigned short[7]);
long lrand48(void) __attribute__((__availability__(swift, unavailable, message="Use arc4random instead.")));
__attribute__((__deprecated__("This function is provided for compatibility reasons only.  Due to security concerns inherent in the design of mktemp(3), it is highly recommended that you use mkstemp(3) instead.")))
char * mktemp(char *);
int mkstemp(char *);
long mrand48(void) __attribute__((__availability__(swift, unavailable, message="Use arc4random instead.")));
long nrand48(unsigned short[3]) __attribute__((__availability__(swift, unavailable, message="Use arc4random instead.")));
int posix_openpt(int);
char * ptsname(int);
int ptsname_r(int fildes, char * buffer, size_t buflen) __attribute__((availability(macos,introduced=10.13.4))) __attribute__((availability(ios,introduced=11.3))) __attribute__((availability(tvos,introduced=11.3))) __attribute__((availability(watchos,introduced=4.3)));
int putenv(char *) __asm("_" "putenv" );
long random(void) __attribute__((__availability__(swift, unavailable, message="Use arc4random instead.")));
int rand_r(unsigned *) __attribute__((__availability__(swift, unavailable, message="Use arc4random instead.")));
char * realpath(const char * restrict, char * restrict ) __asm("_" "realpath" "$DARWIN_EXTSN");
unsigned short * seed48(unsigned short[3]);
int setenv(const char * __name, const char * __value, int __overwrite) __asm("_" "setenv" );
void setkey(const char *) __asm("_" "setkey" );
char * setstate(const char *);
void srand48(long);
void srandom(unsigned);
int unlockpt(int);
int unsetenv(const char *) __asm("_" "unsetenv" );
typedef __darwin_dev_t dev_t;
typedef __darwin_mode_t mode_t;
uint32_t arc4random(void);
void arc4random_addrandom(unsigned char * , int __datlen)
    __attribute__((availability(macosx,introduced=10.0))) __attribute__((availability(macosx,deprecated=10.12,message="use arc4random_stir")))
    __attribute__((availability(ios,introduced=2.0))) __attribute__((availability(ios,deprecated=10.0,message="use arc4random_stir")))
    __attribute__((availability(tvos,introduced=2.0))) __attribute__((availability(tvos,deprecated=10.0,message="use arc4random_stir")))
    __attribute__((availability(watchos,introduced=1.0))) __attribute__((availability(watchos,deprecated=3.0,message="use arc4random_stir")));
void arc4random_buf(void * __buf, size_t __nbytes) __attribute__((availability(macosx,introduced=10.7)));
void arc4random_stir(void);
uint32_t
  arc4random_uniform(uint32_t __upper_bound) __attribute__((availability(macosx,introduced=10.7)));
int atexit_b(void (^ _Nonnull)(void)) __attribute__((availability(macosx,introduced=10.6)));
void *bsearch_b(const void * __key, const void * __base, size_t __nel,
     size_t __width, int (^ _Nonnull __compar)(const void *, const void *) __attribute__((__noescape__)))
     __attribute__((availability(macosx,introduced=10.6)));
char * cgetcap(char *, const char *, int);
int cgetclose(void);
int cgetent(char * *, char * *, const char *);
int cgetfirst(char * *, char * *);
int cgetmatch(const char *, const char *);
int cgetnext(char * *, char * *);
int cgetnum(char *, const char *, long *);
int cgetset(const char *);
int cgetstr(char *, const char *, char * *);
int cgetustr(char *, const char *, char * *);
int daemon(int, int) __asm("_" "daemon" "$1050") __attribute__((availability(macosx,introduced=10.0,deprecated=10.5,message="Use posix_spawn APIs instead."))) __attribute__((availability(watchos,unavailable))) __attribute__((availability(tvos,unavailable)));
char * devname(dev_t, mode_t);
char * devname_r(dev_t, mode_t, char * buf, int len);
char * getbsize(int *, long *);
int getloadavg(double [], int __nelem);
const char
 *getprogname(void);
void setprogname(const char *);
int heapsort(void * __base, size_t __nel, size_t __width,
     int (* _Nonnull __compar)(const void *, const void *));
int heapsort_b(void * __base, size_t __nel, size_t __width,
     int (^ _Nonnull __compar)(const void *, const void *) __attribute__((__noescape__)))
     __attribute__((availability(macosx,introduced=10.6)));
int mergesort(void * __base, size_t __nel, size_t __width,
     int (* _Nonnull __compar)(const void *, const void *));
int mergesort_b(void * __base, size_t __nel, size_t __width,
     int (^ _Nonnull __compar)(const void *, const void *) __attribute__((__noescape__)))
     __attribute__((availability(macosx,introduced=10.6)));
void psort(void * __base, size_t __nel, size_t __width,
     int (* _Nonnull __compar)(const void *, const void *))
     __attribute__((availability(macosx,introduced=10.6)));
void psort_b(void * __base, size_t __nel, size_t __width,
     int (^ _Nonnull __compar)(const void *, const void *) __attribute__((__noescape__)))
     __attribute__((availability(macosx,introduced=10.6)));
void psort_r(void * __base, size_t __nel, size_t __width, void *,
     int (* _Nonnull __compar)(void *, const void *, const void *))
     __attribute__((availability(macosx,introduced=10.6)));
void qsort_b(void * __base, size_t __nel, size_t __width,
     int (^ _Nonnull __compar)(const void *, const void *) __attribute__((__noescape__)))
     __attribute__((availability(macosx,introduced=10.6)));
void qsort_r(void * __base, size_t __nel, size_t __width, void *,
     int (* _Nonnull __compar)(void *, const void *, const void *));
int radixsort(const unsigned char * * __base, int __nel, const unsigned char * __table,
     unsigned __endbyte);
int rpmatch(const char *)
 __attribute__((availability(macos,introduced=10.15))) __attribute__((availability(ios,introduced=13.0))) __attribute__((availability(tvos,introduced=13.0))) __attribute__((availability(watchos,introduced=6.0)));
int sradixsort(const unsigned char * * __base, int __nel, const unsigned char * __table,
     unsigned __endbyte);
void sranddev(void);
void srandomdev(void);
long long
 strtonum(const char *__numstr, long long __minval, long long __maxval, const char * *__errstrp)
 __attribute__((availability(macos,introduced=11.0))) __attribute__((availability(ios,introduced=14.0))) __attribute__((availability(tvos,introduced=14.0))) __attribute__((availability(watchos,introduced=7.0)));
long long
  strtoq(const char *__str, char * *__endptr, int __base);
unsigned long long
  strtouq(const char *__str, char * *__endptr, int __base);
extern char * suboptarg;
typedef long int ptrdiff_t;
typedef long double max_align_t;

typedef float f32;
typedef double f64;
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef uintptr_t usize;
typedef intptr_t isize;
typedef uint8_t b8;
typedef uint16_t b16;
typedef uint32_t b32;
typedef uint64_t b64;
__attribute__((always_inline)) inline u32 bitwidth(u64 x) {
  if (x == 0) {
    return 0;
  }
  return 64 - ((u32)(__builtin_clzll(x)));
}
__attribute__((always_inline)) inline u32 count_trailing_zeros64(u64 x) {
  return ((u32)(__builtin_ctzll(x)));
}
__attribute__((always_inline)) inline b32 is_zero_or_power_of_two(usize x) {
  return ((((x)-1) & (x)) == 0);
}
__attribute__((always_inline)) inline usize round_up_to_power_of_two(usize x, usize power) {
  return (x + (power - 1)) & (~(power - 1));
}
__attribute__((always_inline)) inline void *ptr_offset(void const *p, usize offset) {
  return ((void*)(((u8 const*)(p)) + offset));
}
__attribute__((always_inline)) inline isize ptr_diff(void const *a, void const *b) {
  return ((isize)(((u8*)(a)) - ((u8*)(b))));
}
__attribute__((always_inline)) inline void *ptr_forward_align(void const *p, u32 align) {
  return ((void*)(round_up_to_power_of_two(((usize)(p)), align)));
}
typedef struct String {
  u8 const *str;
  usize len;
} String;
String string_from_cstr(char const *s);
i64 parse_i64(String s);
__attribute__((always_inline)) inline b32 string_eq(String a, String b) {
  if (a.len != b.len) {
    return 0;
  }
  return memcmp(a.str, b.str, a.len) == 0;
}
typedef void *(*AllocatorFunction)(
  void *ctx, void *ptr, size_t old_byte_size, size_t new_byte_size, u32 align
);
typedef struct Allocator {
  AllocatorFunction fn;
  void *ctx;
} Allocator;
usize vmem_page_size(void);
void *vmem_reserve(usize size);
b32 vmem_commit(void *p, usize size);
void vmem_release(void *p, usize size);
typedef struct {
  void *base;
  void *commit_end;
  void *reserve_end;
  void *at;
} Arena;
typedef struct {
  Arena *arena;
  void *at;
} ArenaSnapshot;
typedef struct {
  usize reserve_size;
  usize initial_commit_size;
} ArenaOptions;
void arena_init(Arena *arena, ArenaOptions *options);
void arena_deinit(Arena *arena);
void *arena_push(Arena *arena, usize size, u32 align);
String arena_copy_string(Arena *arena, String s);
__attribute__((always_inline)) inline ArenaSnapshot arena_scope_begin(Arena *arena) {
  return (ArenaSnapshot){ .arena = arena, .at = arena->at, };
}
__attribute__((always_inline)) inline void arena_scope_end(Arena *arena, ArenaSnapshot snapshot) {
  (__builtin_expect(!(arena == snapshot.arena), 0) ? __assert_rtn(__func__, "toteload.h", 236, "arena == snapshot.arena") : (void)0);
  arena->at = snapshot.at;
}
__attribute__((always_inline)) inline void freelist_grow(void **freelist, void *mem, usize stride, usize count) {
  for (usize i = 0; i < count-1; i++) {
    void **p = ptr_offset(mem, i * stride);
    *p = ptr_offset(mem, (i + 1) * stride);
  }
  void **p = ptr_offset(mem, (count - 1) * stride);
  *p = *freelist;
  *freelist = mem;
}
__attribute__((always_inline)) inline void *freelist_alloc(void **freelist) {
  void *p = *freelist;
  *freelist = *((void***)(freelist));
  return p;
}
__attribute__((always_inline)) inline void freelist_free(void **freelist, void *p) {
  *((void**)(p)) = **((void***)(freelist));
  *freelist = p;
}
typedef u32 TokenIndex;
typedef u32 AstIndex;
typedef u32 TypeIndex;
typedef u32 StringIndex;
typedef u32 InstructionIndex;
typedef u32 SourceIndex;
typedef u32 ValueIndex;
typedef u32 DeclarationIndex;
typedef struct EnvAllocator EnvAllocator;
typedef struct Env Env;
typedef struct ValueStore ValueStore;
typedef struct SourceAllocator SourceAllocator;
typedef struct Source Source;
enum MessageSeverity {
  Severity_Error,
  Severity_Warning,
  Severity_Info,
};
enum MessageLocationKind {
  MessageLocation_unspecified,
  MessageLocation_end_of_file,
  MessageLocation_byte_offset,
  MessageLocation_token_index,
  MessageLocation_ast_index,
};
typedef struct {
  u8 kind;
  union {
    TokenIndex token_index;
    AstIndex ast_index;
    u32 offset;
  } data;
} MessageLocation;
typedef union {
  u8 token_kind;
  AstIndex ast_index;
  TypeIndex type_index;
} MessageArg;
typedef struct {
  u8 severity;
  SourceIndex source;
  MessageLocation location;
  String format;
  MessageArg args[];
} Message;
typedef void (*FnAddMessage)(void *user, u8 severity, SourceIndex source, MessageLocation location, String format, ...);
typedef struct {
  void *user;
  FnAddMessage add_message;
} MessageSink;
typedef Message* MessagePtr;
typedef struct {
  usize len;
  usize segment_count;
  MessagePtr *segments[24];
} MessageList;
                    usize msglist_cap(MessageList *list);
                    MessagePtr *msglist_push(MessageList *list, Arena *arena);
                    MessagePtr msglist_pop(MessageList *list);
                    void msglist_append(MessageList *list, Arena *arena, MessagePtr item);
                    MessagePtr *msglist_ptr_at_unchecked(MessageList *list, usize i);
                    MessagePtr msglist_at_unchecked(MessageList *list, usize i);
                    void msglist_copy_to_array(MessageList *list, MessagePtr *out);
u32 message_format_arg_count(String fmt);
void print_message(Message *message, Source *source);
enum TypeKind {
  Type_comptime_int,
  Type_integer,
  Type_boolean,
  Type_function,
  Type_nil,
  Type_never,
  Type_slice,
  Type_array,
  Type_type,
};
typedef i64 ComptimeInt;
enum Signedness {
  Unsigned,
  Signed,
};
typedef struct {
  u32 size;
  u32 stride;
  u32 align;
} TypeSizeInfo;
typedef struct {
  u8 signedness;
  u16 bitwidth;
} TypeInteger;
typedef struct {
  TypeIndex base_type;
} TypeSlice;
typedef struct {
  TypeIndex base_type;
  u64 size;
} TypeArray;
typedef struct {
  TypeIndex return_type;
  u32 param_count;
  TypeIndex param_types[];
} TypeFunction;
typedef struct {
  u8 kind;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wflexible-array-extensions"
  union {
    TypeInteger integer;
    TypeSlice slice;
    TypeArray array;
    TypeFunction function;
  } data;
#pragma clang diagnostic pop
} Type;
typedef Type *TypePtr;
typedef struct {
  usize len;
  usize segment_count;
  TypePtr *segments[24];
} TypeInternerList;
typedef struct TypeInternerMapBucket {
  TypePtr key;
  TypeIndex val;
} TypeInternerMapBucket;
typedef struct TypeInternerMap {
  Allocator allocator;
  u32 *meta;
  void *context;
  TypeInternerMapBucket *buckets;
  u32 mask;
  u32 item_count;
} TypeInternerMap;
typedef struct {
  Allocator allocator;
  u32 initial_size;
  void *context;
} HashMapOptions;

typedef struct {
  Arena *arena;
  TypeInternerList list;
  TypeInternerMap map;
} TypeInterner;
typedef struct {
  Arena *arena;
  Allocator map_allocator;
  u32 map_initial_size;
  void *context;
} InternerOptions;
                 void types_init(TypeInterner *interner, InternerOptions *options);
                 void types_deinit(TypeInterner *interner);
                 TypeIndex types_add(TypeInterner *interner, TypePtr item);
                 TypeIndex types_add_checked(TypeInterner *interner, TypePtr item, b32 *already_present);
                 b32 types_find(TypeInterner *interner, TypePtr item, TypeIndex *idx);
                 TypePtr types_get(TypeInterner *interner, TypeIndex idx);
u32 type_intern_byte_size(Type *type);
TypeSizeInfo types_size_info(TypeInterner *types, Type *type);
TypeSizeInfo types_size_info_by_index(TypeInterner *types, TypeIndex idx);

typedef struct {
  TypeIndex type;
  u32 data_size;
  void *data;
} Value;
typedef struct {
  usize len;
  usize segment_count;
  Value *segments[24];
} ValueList;

typedef struct {
  usize len;
  void *data;
} ValueSlice;
struct ValueStore {
  Arena *arena;
  Allocator payload_allocator;
  ValueList list;
};
typedef struct {
  Arena *arena;
  Allocator payload_allocator;
} ValueStoreOptions;
void values_init(ValueStore *values, ValueStoreOptions *options);
void values_deinit(ValueStore *values);
ValueIndex values_alloc(ValueStore *values, Value **out);
void *values_alloc_data(ValueStore *values, u32 size, u32 align);
void values_dealloc_data(ValueStore *values, void *data, u32 size);
void values_dealloc(ValueStore *values, ValueIndex idx);
Value *values_get(ValueStore *values, ValueIndex idx);
ValueIndex values_copy(ValueStore *values, ValueIndex val);
typedef struct {
  usize len;
  usize segment_count;
  String *segments[24];
} StringInternerList;
typedef struct StringInternerMapBucket {
  String key;
  StringIndex val;
} StringInternerMapBucket;
typedef struct StringInternerMap {
  Allocator allocator;
  u32 *meta;
  void *context;
  StringInternerMapBucket *buckets;
  u32 mask;
  u32 item_count;
} StringInternerMap;

typedef struct {
  Arena *arena;
  StringInternerList list;
  StringInternerMap map;
} StringInterner;
                 void strings_init(StringInterner *interner, InternerOptions *options);
                 void strings_deinit(StringInterner *interner);
                 StringIndex strings_add(StringInterner *interner, String item);
                 StringIndex strings_add_checked(StringInterner *interner, String item, b32 *already_present);
                 b32 strings_find(StringInterner *interner, String item, StringIndex *idx);
                 String strings_get(StringInterner *interner, StringIndex idx);
enum IrResult {
  IrResult_ok,
};
typedef u32 IrRef;
__attribute__((always_inline)) inline b32 ref_is_value_index(IrRef ref) {
  return (ref & (((u32)(1)) << 31)) == 0;
}
__attribute__((always_inline)) inline b32 ref_is_instruction_index(IrRef ref) {
  return (ref & (((u32)(1)) << 31)) != 0;
}
__attribute__((always_inline)) inline b32 ir_ref_is_nil(IrRef ref) {
  return ref == 0;
}
__attribute__((always_inline)) inline ValueIndex ref_to_value_index(IrRef ref) {
  return ref;
}
__attribute__((always_inline)) inline InstructionIndex ref_to_instruction_index(IrRef ref) {
  return ref & ~(((u32)(1)) << 31);
}
__attribute__((always_inline)) inline IrRef ir_ref_from_instruction_index(InstructionIndex idx) {
  return idx | (((u32)(1)) << 31);
}
__attribute__((always_inline)) inline IrRef ir_ref_from_value_index(ValueIndex idx) {
  return idx;
}
enum IrOpcode {
  IR_func,
  IR_param,
  IR_alloc,
  IR_cond_br,
  IR_block,
  IR_loop,
  IR_br,
  IR_ret,
  IR_repeat,
  IR_load,
  IR_store,
  IR_call,
  IR_declaration,
  IR_lookup,
  IR_as,
  IR_unify,
  IR_type,
  IR_return_type,
  IR_param_type,
};
typedef struct {
  IrRef function;
  u32 param_index;
} IrParamType;
typedef struct {
  u8 kind;
  u32 arg_count;
  IrRef args[];
} IrType;
typedef struct {
  IrRef declared_type;
  IrRef value;
} IrDeclaration;
typedef struct {
  IrRef type_lhs;
  IrRef type_rhs;
} IrUnify;
typedef struct {
  IrRef type_to;
  IrRef val;
} IrAs;
typedef struct {
  u32 param_count;
  u32 instruction_count;
} IrFunc;
typedef struct {
  InstructionIndex block;
  IrRef value;
} IrBr;
typedef struct {
  IrRef cond;
  InstructionIndex then;
  InstructionIndex otherwise;
} IrCondBr;
typedef struct {
  IrRef dst;
  IrRef value;
} IrStore;
typedef struct {
  IrRef func;
  u32 arg_count;
  IrRef args[];
} IrCall;
typedef struct {
  u32 opcode_count;
  u8 *opcodes;
  u32 *data;
  void *extra;
} IrChunk;
u8 chunk_opcode(IrChunk *chunk, InstructionIndex idx);
u32 chunk_data(IrChunk *chunk, InstructionIndex idx);
void *chunk_extra(IrChunk *chunk, InstructionIndex idx);
typedef struct {
  usize len;
  usize segment_count;
  u8 *segments[24];
} OpcodeList;

typedef union {
  u32 data;
  void *ptr;
} InstData;
typedef struct {
  usize len;
  usize segment_count;
  InstData *segments[24];
} InstDataList;

typedef struct {
  Arena *scratch;
  OpcodeList kinds;
  InstDataList data;
} IrBuilder;
InstructionIndex inst_alloc(IrBuilder *builder);
void inst_set_opcode(IrBuilder *builder, InstructionIndex idx, u8 opcode);
void inst_set_data(IrBuilder *builder, InstructionIndex idx, u32 data);
void *inst_push_data_raw(IrBuilder *builder, InstructionIndex idx, u32 size, u32 align);
u32 inst_offset(IrBuilder *builder, InstructionIndex start);
InstructionIndex inst_block_begin(IrBuilder *builder);
void inst_block_end(IrBuilder *builder, InstructionIndex block, IrRef val);
InstructionIndex inst_as(IrBuilder *builder, IrRef type_destination, IrRef val);
void irbuilder_flatten(IrBuilder *builder, Arena *arena, IrChunk *chunk);
void ir_chunk_print(FILE *out, IrChunk *chunk, TypeInterner *types, ValueStore *values);

typedef struct {
  struct {
    TypeIndex comptime_int;
    TypeIndex nil;
    TypeIndex type;
    TypeIndex i32;
  } type;
  struct {
    ValueIndex type;
    ValueIndex nil;
    ValueIndex i32;
  } val;
} Common;
typedef struct {
  DeclarationIndex parent;
  StringIndex name;
} DeclarationKey;
typedef enum {
  ResolveStatus_unresolved,
  ResolveStatus_resolving_type,
  ResolveStatus_type_resolved,
  ResolveStatus_resolving_value,
  ResolveStatus_fully_resolved,
} ResolveStatus;
enum DeclarationKind {
  Declaration_root,
  Declaration_primitive,
  Declaration_mod,
  Declaration_decl,
};
typedef struct {
  u8 kind;
  u8 resolve_status;
  union {
    ValueIndex primitive;
    struct {
      Source *source;
      u32 tree_idx;
      u32 typecheck_end;
      IrChunk *chunk;
      ValueIndex val;
    } decl;
  } data;
} Declaration;
typedef struct { DeclarationKey key; Declaration extra; } DeclarationInternerExtra;
typedef struct {
  usize len;
  usize segment_count;
  DeclarationInternerExtra *segments[24];
} DeclarationInternerList;
typedef struct DeclarationInternerMapBucket {
  DeclarationKey key;
  DeclarationIndex val;
} DeclarationInternerMapBucket;
typedef struct DeclarationInternerMap {
  Allocator allocator;
  u32 *meta;
  void *context;
  DeclarationInternerMapBucket *buckets;
  u32 mask;
  u32 item_count;
} DeclarationInternerMap;

typedef struct {
  Arena *arena;
  DeclarationInternerList list;
  DeclarationInternerMap map;
} DeclarationInterner;
                 void decls_init(DeclarationInterner *interner, InternerOptions *options);
                 void decls_deinit(DeclarationInterner *interner);
                 DeclarationIndex decls_add(DeclarationInterner *interner, DeclarationKey item);
                 DeclarationIndex decls_add_checked(DeclarationInterner *interner, DeclarationKey item, b32 *already_present);
                 b32 decls_find(DeclarationInterner *interner, DeclarationKey item, DeclarationIndex *idx);
                 DeclarationKey decls_get(DeclarationInterner *interner, DeclarationIndex idx);
                 Declaration decls_get_extra(DeclarationInterner *interner, DeclarationIndex idx);
                 Declaration *decls_extra_get_ptr(DeclarationInterner *interner, DeclarationIndex idx);
                 void decls_set_extra(DeclarationInterner *interner, DeclarationIndex idx, Declaration);
typedef struct {
  usize len;
  usize segment_count;
  Source *segments[20];
} SourceList;

typedef struct {
  Arena arena;
  Arena scratch;
  SourceList sources;
  MessageList msg_list;
  MessageSink msg_sink;
  Common common;
  ValueStore values;
  StringInterner strings;
  TypeInterner types;
  DeclarationInterner decls;
} Compiler;
void compiler_init(Compiler *compiler);
void compiler_deinit(Compiler *compiler);
void compiler_add_sourcefile(Compiler *compiler, String filename);
b32 lookup_identifier(DeclarationInterner *decls_keys, DeclarationIndex *mods, u32 mod_count, StringIndex name, DeclarationIndex *out);
void compiler_print_all_messages(Compiler *compiler);
b32 compile(Compiler *compiler);
enum TokenKind {
  Tok_colon,
  Tok_semicolon,
  Tok_comma,
  Tok_dot,
  Tok_arrow,
  Tok_equals,
  Tok_minus,
  Tok_plus,
  Tok_star,
  Tok_slash,
  Tok_percent,
  Tok_plus_equals,
  Tok_exclamation,
  Tok_ampersand,
  Tok_bar,
  Tok_caret,
  Tok_tilde,
  Tok_left_shift,
  Tok_right_shift,
  Tok_cmp_eq,
  Tok_cmp_ne,
  Tok_cmp_gt,
  Tok_cmp_ge,
  Tok_cmp_lt,
  Tok_cmp_le,
  Tok_literal_int,
  Tok_literal_string,
  Tok_brace_open,
  Tok_brace_close,
  Tok_paren_open,
  Tok_paren_close,
  Tok_bracket_open,
  Tok_bracket_close,
  Tok_keyword_if,
  Tok_keyword_else,
  Tok_keyword_for,
  Tok_keyword_do,
  Tok_keyword_break,
  Tok_keyword_continue,
  Tok_keyword_return,
  Tok_keyword_and,
  Tok_keyword_or,
  Tok_keyword_defer,
  Tok_keyword_const,
  Tok_keyword_cast,
  Tok_keyword_bitcast,
  Tok_keyword_as,
  Tok_keyword_mod,
  Tok_keyword_no_cache,
  Tok_keyword_inline,
  Tok_identifier,
  Tok_builtin_print,
  Tok_line_comment,
  Tok_newline,
  Tok_kind_max,
};
typedef struct {
  u32 start;
  u32 end;
} SpanU32;
typedef struct {
  u32 tok_count;
  u32 line_count;
  u8 *kinds;
  SpanU32 *spans;
  u32 *lines;
} Tokens;
typedef struct {
  u32 line;
  u32 offset_start_of_line;
  u32 line_len;
} LineInfo;
LineInfo tokens_find_line_info(Tokens *tokens, u32 byte_offset);
String token_string(Tokens *tokens, String text, TokenIndex tok);
char const *token_kind_string(u8 kind);
typedef struct {
  MessageSink *msg_sink;
  Arena *arena;
  Arena *scratch;
} TokenizeContext;
b32 tokenize(TokenizeContext *context, String text, Tokens *tokens);
typedef enum {
  Ast_source,
  Ast_mod_section,
  Ast_block,
  Ast_type_slice,
  Ast_type_array,
  Ast_type_function,
  Ast_builtin,
  Ast_declaration,
  Ast_assign,
  Ast_literal_int,
  Ast_literal_string,
  Ast_identifier,
  Ast_call,
  Ast_index,
  Ast_unary_op,
  Ast_binary_op,
  Ast_function,
  Ast_param,
  Ast_if_else,
  Ast_for,
  Ast_defer,
  Ast_const,
  Ast_cast,
  Ast_as,
  Ast_kind_max,
} AstKind;
enum BuiltinKind {
  Builtin_print,
};
enum BinaryOpKind {
  Mul,
  Div,
  Mod,
  Sub,
  Add,
  Bit_shift_left,
  Bit_shift_right,
  Bit_and,
  Bit_or,
  Bit_xor,
  Cmp_equal,
  Cmp_not_equal,
  Cmp_greater_than,
  Cmp_greater_equal,
  Cmp_less_than,
  Cmp_less_equal,
  Logical_and,
  Logical_or,
  BinaryOpKind_max,
};
enum AssignKind {
  Assign_normal,
  AssignKind_max,
};
enum UnaryOpKind {
  Negate,
  Not,
  UnaryOpKind_max,
};
enum AttributeFlag {
  Attribute_comptime = 1 << 0,
  Attribute_no_cache = 1 << 1,
};
typedef struct {
  u8 kind;
  u32 count;
  AstIndex args[];
} AstBuiltin;
typedef struct {
  AstIndex return_type;
  u32 count;
  AstIndex param_types[];
} AstTypeFunction;
typedef struct {
  AstIndex base;
} AstTypeSlice;
typedef struct {
  AstIndex size;
  AstIndex base;
} AstTypeArray;
typedef struct {
  TokenIndex name;
  AstIndex type;
  AstIndex value;
} AstDeclaration;
typedef struct {
  u32 count;
  AstIndex items[];
} AstSource;
typedef struct {
  TokenIndex name;
  u32 count;
  AstIndex items[];
} AstModSection;
typedef struct {
  u32 count;
  AstIndex items[];
} AstBlock;
typedef struct {
  u32 count;
  AstIndex items[];
} AstLiteralSequence;
typedef struct {
  TokenIndex name;
  AstIndex type;
} AstParam;
typedef struct {
  AstIndex return_type;
  AstIndex body;
  u32 count;
  AstIndex params[];
} AstFunction;
typedef struct {
  AstIndex cond;
  AstIndex then;
  AstIndex otherwise;
} AstIfElse;
typedef struct {
  AstIndex iterable;
  AstIndex iterator;
  AstIndex body;
} AstFor;
typedef struct {
  u8 op_kind;
  AstIndex value;
} AstUnaryOp;
typedef struct {
  u8 op_kind;
  AstIndex lhs;
  AstIndex rhs;
} AstBinaryOp;
typedef struct {
  AstIndex base;
  AstIndex field;
} AstFieldAccess;
typedef struct {
  AstIndex callee;
  u32 count;
  AstIndex args[];
} AstCall;
typedef struct {
  AstIndex indexable;
  AstIndex index_at;
} AstIndexData;
typedef struct {
  AstIndex value;
} AstDefer;
typedef struct {
  u8 assign_kind;
  AstIndex lhs;
  AstIndex value;
} AstAssign;
typedef struct {
  AstIndex expr;
} AstConst;
typedef struct {
  AstIndex type_dst;
  AstIndex value;
} AstCast;
typedef struct {
  AstIndex type_dst;
  AstIndex value;
} AstAs;
typedef union {
  AstSource source;
  AstModSection mod_section;
  AstBlock block;
  AstBuiltin builtin;
  AstTypeFunction type_function;
  AstTypeSlice type_slice;
  AstTypeArray type_array;
  AstDeclaration declaration;
  AstAssign assign;
  AstLiteralSequence literal_sequence;
  TokenIndex literal_int;
  TokenIndex literal_string;
  TokenIndex identifier;
  AstFieldAccess access;
  AstCall call;
  AstIndexData index;
  AstUnaryOp unary_op;
  AstBinaryOp binary_op;
  AstFunction function;
  AstParam param;
  AstIfElse if_else;
  AstFor for_;
  AstDefer defer;
  AstConst const_;
  AstCast cast;
  AstAs as;
} AstNodeData;
typedef struct {
  TokenIndex start;
  TokenIndex end;
} SpanToken;
typedef struct {
  MessageSink *msg_sink;
  Arena *arena;
  Arena *scratch;
} ParseContext;
typedef struct {
  u32 count;
  u8 *kinds;
  SpanToken *spans;
  u32 *datas;
  void *extra;
} AstNodes;
void *ast_data(AstNodes *ast, AstIndex idx);
String ast_kind_string(u8 kind);
b32 parse(ParseContext *context, Tokens *tokens, AstNodes *ast);
enum SourceDeclarationKind {
  SourceDeclaration_root,
  SourceDeclaration_mod,
  SourceDeclaration_declaration,
};
typedef struct {
  u8 kind;
  AstIndex node;
  u32 child_count;
  u32 parent;
  StringIndex name;
} SourceDeclaration;
enum SourceStatus {
  SourceStatus_unprocessed,
  SourceStatus_failed_to_parse,
  SourceStatus_parsed,
};
struct Source {
  u32 status;
  SourceIndex idx;
  Arena arena;
  MessageList msg_list;
  MessageSink msg_sink;
  String filename;
  String text;
  Tokens tokens;
  AstNodes ast;
  u32 decl_tree_size;
  SourceDeclaration *decls;
  DeclarationIndex *decl_idxs;
};
void source_file_init(Source *source, SourceIndex idx, String filename);
void source_file_deinit(Source *source);
b32 source_read_file(Source *source);
b32 source_tokenize(Source *source, Arena *scratch);
b32 source_parse(Source *source, Arena *scratch);
void source_index_declarations(Source *source, StringInterner *strings);
void source_print_all_messages(Source *source);

typedef struct {
  Arena *perm;
  Arena *scratch;
  Common *common;
  MessageSink *msg_sink;
  StringInterner *strings;
  DeclarationInterner *decls;
  ValueStore *values;
} CodeGenContext;
b32 generate_code(CodeGenContext *context, Declaration *decl);
typedef struct {
  InstructionIndex start;
  InstructionIndex end;
} ScopeSpan;
typedef struct { ScopeSpan* data; u32 len; u32 cap; } ScopeStack;
typedef struct {
  u32 ok;
  InstructionIndex pc;
  IrChunk *chunk;
  ValueIndex *inst_map;
  ScopeStack scopes;
  ArenaSnapshot snapshot;
} CallFrame;
typedef struct { CallFrame* data; u32 len; u32 cap; } CallStack;
void frame_push(CallStack *call_stack, Arena *arena, IrChunk *chunk);
void frame_pop(CallStack *call_stack, Arena *arena, ValueStore *values);
typedef struct {
  Arena *perm;
  Arena *scratch;
  Common *common;
  MessageSink *msg_sink;
  DeclarationInterner *decls;
  ValueStore *values;
  TypeInterner *types;
} InterpretContext;
typedef struct {
  Arena *perm;
  Arena *scratch;
  MessageSink *msg_sink;
  DeclarationInterner *declarations;
  TypeInterner *types;
  ValueStore *values;
  Common *common;
} Interpreter;
void step(Interpreter *in, CallFrame *f);
u32 run_until(Interpreter *in, CallStack *stack, u32 idx);

typedef __builtin_va_list __gnuc_va_list;
typedef __builtin_va_list va_list;
static usize sources_cap(SourceList *list);
static Source *sources_push(SourceList *list, Arena *arena);
static Source sources_pop(SourceList *list);
static void sources_append(SourceList *list, Arena *arena, Source item);
static Source *sources_ptr_at_unchecked(SourceList *list, usize i);
static Source sources_at_unchecked(SourceList *list, usize i);
static void sources_copy_to_array(SourceList *list, Source *out);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
static usize segment_count_at_size(usize min_size_log2, usize size) {
  return bitwidth(size >> min_size_log2) + 1;
}
static usize segment_size(usize min_size_log2, usize si) {
  return (usize)1 << (min_size_log2 + si);
}
static usize segment_idx(usize min_size_log2, usize i) {
  return bitwidth((i >> min_size_log2) + 1) - 1;
}
static usize item_idx(usize min_size_log2, usize i, usize si) {
  return i + ((usize)1 << min_size_log2) - ((usize)1 << (min_size_log2 + si));
}
usize sources_cap(SourceList *list) {
  return (((usize)1 << list->segment_count) - 1) << 4;
}
static void sources__ensure_capacity(SourceList *list, Arena *arena, usize min_capacity) {
  usize cap = sources_cap(list);
  if (cap >= min_capacity) {
    return;
  }
  usize required_segment_count = segment_count_at_size(4, min_capacity);
  if (required_segment_count <= list->segment_count) {
    return;
  }
  for (usize i = list->segment_count; i < required_segment_count; i++) {
    usize size = segment_size(4, i);
    list->segments[i] = arena_push(arena, (size) * sizeof(Source), _Alignof(Source));
  }
  list->segment_count = required_segment_count;
}
static
Source *sources_push(SourceList *list, Arena *arena) {
  sources__ensure_capacity(list, arena, list->len + 1);
  Source *p = sources_ptr_at_unchecked(list, list->len);
  list->len += 1;
  return p;
}
static
Source sources_pop(SourceList *list) {
  (__builtin_expect(!(list->len > 0), 0) ? __assert_rtn(__func__, "segment_list.h", 113, "list->len > 0") : (void)0);
  Source res = sources_at_unchecked(list, list->len-1);
  list->len -= 1;
  return res;
}
static
void sources_append(SourceList *list, Arena *arena, Source item) {
  *sources_push(list, arena) = item;
}
static
Source *sources_ptr_at_unchecked(SourceList *list, usize idx) {
  usize si = segment_idx(4, idx);
  usize i = item_idx(4, idx, si);
  return &list->segments[si][i];
}
static
Source sources_at_unchecked(SourceList *list, usize idx) {
  return *sources_ptr_at_unchecked(list, idx);
}
static
void sources_copy_to_array(SourceList *list, Source *out) {
  if (list->len == 0) {
    return;
  }
  u32 offset = 0;
  u32 segment_count = segment_count_at_size(4, list->len);
  for (u32 i = 0; i < segment_count - 1; i++) {
    u32 size = segment_size(4, i);
    __builtin___memcpy_chk (out + offset, list->segments[i], size * sizeof(Source), __builtin_object_size (out + offset, 0));
    offset += size;
  }
  __builtin___memcpy_chk (out + offset, list->segments[segment_count-1], (list->len - offset) * sizeof(Source), __builtin_object_size (out + offset, 0));
}
#pragma clang diagnostic pop
static __inline __attribute__((unused)) __attribute__((const)) unsigned XXH_INLINE_XXH_versionNumber (void);
typedef enum {
    XXH_NAMESPACEXXH_OK = 0,
    XXH_NAMESPACEXXH_ERROR
} XXH_NAMESPACEXXH_errorcode;
    typedef uint32_t XXH32_hash_t;
static __inline __attribute__((unused)) __attribute__((pure)) XXH32_hash_t XXH_INLINE_XXH32 (const void* input, size_t length, XXH32_hash_t seed);
typedef struct XXH_NAMESPACEXXH32_state_s XXH_NAMESPACEXXH32_state_t;
static __inline __attribute__((unused)) __attribute__((malloc)) XXH_NAMESPACEXXH32_state_t* XXH_INLINE_XXH32_createState(void);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH32_freeState(XXH_NAMESPACEXXH32_state_t* statePtr);
static __inline __attribute__((unused)) void XXH_INLINE_XXH32_copyState(XXH_NAMESPACEXXH32_state_t* dst_state, const XXH_NAMESPACEXXH32_state_t* src_state);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH32_reset (XXH_NAMESPACEXXH32_state_t* statePtr, XXH32_hash_t seed);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH32_update (XXH_NAMESPACEXXH32_state_t* statePtr, const void* input, size_t length);
static __inline __attribute__((unused)) __attribute__((pure)) XXH32_hash_t XXH_INLINE_XXH32_digest (const XXH_NAMESPACEXXH32_state_t* statePtr);
typedef struct {
    unsigned char digest[4];
} XXH_NAMESPACEXXH32_canonical_t;
static __inline __attribute__((unused)) void XXH_INLINE_XXH32_canonicalFromHash(XXH_NAMESPACEXXH32_canonical_t* dst, XXH32_hash_t hash);
static __inline __attribute__((unused)) __attribute__((pure)) XXH32_hash_t XXH_INLINE_XXH32_hashFromCanonical(const XXH_NAMESPACEXXH32_canonical_t* src);
   typedef uint64_t XXH64_hash_t;
static __inline __attribute__((unused)) __attribute__((pure)) XXH64_hash_t XXH_INLINE_XXH64(__attribute__((noescape)) const void* input, size_t length, XXH64_hash_t seed);
typedef struct XXH_NAMESPACEXXH64_state_s XXH_NAMESPACEXXH64_state_t;
static __inline __attribute__((unused)) __attribute__((malloc)) XXH_NAMESPACEXXH64_state_t* XXH_INLINE_XXH64_createState(void);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH64_freeState(XXH_NAMESPACEXXH64_state_t* statePtr);
static __inline __attribute__((unused)) void XXH_INLINE_XXH64_copyState(__attribute__((noescape)) XXH_NAMESPACEXXH64_state_t* dst_state, const XXH_NAMESPACEXXH64_state_t* src_state);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH64_reset (__attribute__((noescape)) XXH_NAMESPACEXXH64_state_t* statePtr, XXH64_hash_t seed);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH64_update (__attribute__((noescape)) XXH_NAMESPACEXXH64_state_t* statePtr, __attribute__((noescape)) const void* input, size_t length);
static __inline __attribute__((unused)) __attribute__((pure)) XXH64_hash_t XXH_INLINE_XXH64_digest (__attribute__((noescape)) const XXH_NAMESPACEXXH64_state_t* statePtr);
typedef struct { unsigned char digest[sizeof(XXH64_hash_t)]; } XXH_NAMESPACEXXH64_canonical_t;
static __inline __attribute__((unused)) void XXH_INLINE_XXH64_canonicalFromHash(__attribute__((noescape)) XXH_NAMESPACEXXH64_canonical_t* dst, XXH64_hash_t hash);
static __inline __attribute__((unused)) __attribute__((pure)) XXH64_hash_t XXH_INLINE_XXH64_hashFromCanonical(__attribute__((noescape)) const XXH_NAMESPACEXXH64_canonical_t* src);
static __inline __attribute__((unused)) __attribute__((pure)) XXH64_hash_t XXH_INLINE_XXH3_64bits(__attribute__((noescape)) const void* input, size_t length);
static __inline __attribute__((unused)) __attribute__((pure)) XXH64_hash_t XXH_INLINE_XXH3_64bits_withSeed(__attribute__((noescape)) const void* input, size_t length, XXH64_hash_t seed);
static __inline __attribute__((unused)) __attribute__((pure)) XXH64_hash_t XXH_INLINE_XXH3_64bits_withSecret(__attribute__((noescape)) const void* data, size_t len, __attribute__((noescape)) const void* secret, size_t secretSize);
typedef struct XXH_NAMESPACEXXH3_state_s XXH_NAMESPACEXXH3_state_t;
static __inline __attribute__((unused)) __attribute__((malloc)) XXH_NAMESPACEXXH3_state_t* XXH_INLINE_XXH3_createState(void);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH3_freeState(XXH_NAMESPACEXXH3_state_t* statePtr);
static __inline __attribute__((unused)) void XXH_INLINE_XXH3_copyState(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* dst_state, __attribute__((noescape)) const XXH_NAMESPACEXXH3_state_t* src_state);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH3_64bits_reset(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH3_64bits_reset_withSeed(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr, XXH64_hash_t seed);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH3_64bits_reset_withSecret(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr, __attribute__((noescape)) const void* secret, size_t secretSize);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH3_64bits_update (__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr, __attribute__((noescape)) const void* input, size_t length);
static __inline __attribute__((unused)) __attribute__((pure)) XXH64_hash_t XXH_INLINE_XXH3_64bits_digest (__attribute__((noescape)) const XXH_NAMESPACEXXH3_state_t* statePtr);
typedef struct {
    XXH64_hash_t low64;
    XXH64_hash_t high64;
} XXH_NAMESPACEXXH128_hash_t;
static __inline __attribute__((unused)) __attribute__((pure)) XXH_NAMESPACEXXH128_hash_t XXH_INLINE_XXH3_128bits(__attribute__((noescape)) const void* data, size_t len);
static __inline __attribute__((unused)) __attribute__((pure)) XXH_NAMESPACEXXH128_hash_t XXH_INLINE_XXH3_128bits_withSeed(__attribute__((noescape)) const void* data, size_t len, XXH64_hash_t seed);
static __inline __attribute__((unused)) __attribute__((pure)) XXH_NAMESPACEXXH128_hash_t XXH_INLINE_XXH3_128bits_withSecret(__attribute__((noescape)) const void* data, size_t len, __attribute__((noescape)) const void* secret, size_t secretSize);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH3_128bits_reset(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH3_128bits_reset_withSeed(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr, XXH64_hash_t seed);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH3_128bits_reset_withSecret(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr, __attribute__((noescape)) const void* secret, size_t secretSize);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH3_128bits_update (__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr, __attribute__((noescape)) const void* input, size_t length);
static __inline __attribute__((unused)) __attribute__((pure)) XXH_NAMESPACEXXH128_hash_t XXH_INLINE_XXH3_128bits_digest (__attribute__((noescape)) const XXH_NAMESPACEXXH3_state_t* statePtr);
static __inline __attribute__((unused)) __attribute__((pure)) int XXH_INLINE_XXH128_isEqual(XXH_NAMESPACEXXH128_hash_t h1, XXH_NAMESPACEXXH128_hash_t h2);
static __inline __attribute__((unused)) __attribute__((pure)) int XXH_INLINE_XXH128_cmp(__attribute__((noescape)) const void* h128_1, __attribute__((noescape)) const void* h128_2);
typedef struct { unsigned char digest[sizeof(XXH_NAMESPACEXXH128_hash_t)]; } XXH_NAMESPACEXXH128_canonical_t;
static __inline __attribute__((unused)) void XXH_INLINE_XXH128_canonicalFromHash(__attribute__((noescape)) XXH_NAMESPACEXXH128_canonical_t* dst, XXH_NAMESPACEXXH128_hash_t hash);
static __inline __attribute__((unused)) __attribute__((pure)) XXH_NAMESPACEXXH128_hash_t XXH_INLINE_XXH128_hashFromCanonical(__attribute__((noescape)) const XXH_NAMESPACEXXH128_canonical_t* src);
struct XXH_NAMESPACEXXH32_state_s {
   XXH32_hash_t total_len_32;
   XXH32_hash_t large_len;
   XXH32_hash_t v[4];
   XXH32_hash_t mem32[4];
   XXH32_hash_t memsize;
   XXH32_hash_t reserved;
};
struct XXH_NAMESPACEXXH64_state_s {
   XXH64_hash_t total_len;
   XXH64_hash_t v[4];
   XXH64_hash_t mem64[4];
   XXH32_hash_t memsize;
   XXH32_hash_t reserved32;
   XXH64_hash_t reserved64;
};
struct XXH_NAMESPACEXXH3_state_s {
   _Alignas(64) XXH64_hash_t acc[8];
   _Alignas(64) unsigned char customSecret[192];
   _Alignas(64) unsigned char buffer[256];
   XXH32_hash_t bufferedSize;
   XXH32_hash_t useSeed;
   size_t nbStripesSoFar;
   XXH64_hash_t totalLen;
   size_t nbStripesPerBlock;
   size_t secretLimit;
   XXH64_hash_t seed;
   XXH64_hash_t reserved64;
   const unsigned char* extSecret;
};
static __inline __attribute__((unused)) __attribute__((pure)) XXH_NAMESPACEXXH128_hash_t XXH_INLINE_XXH128(__attribute__((noescape)) const void* data, size_t len, XXH64_hash_t seed);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH3_generateSecret(__attribute__((noescape)) void* secretBuffer, size_t secretSize, __attribute__((noescape)) const void* customSeed, size_t customSeedSize);
static __inline __attribute__((unused)) void XXH_INLINE_XXH3_generateSecret_fromSeed(__attribute__((noescape)) void* secretBuffer, XXH64_hash_t seed);
static __inline __attribute__((unused)) __attribute__((pure)) XXH64_hash_t
XXH_INLINE_XXH3_64bits_withSecretandSeed(__attribute__((noescape)) const void* data, size_t len,
                              __attribute__((noescape)) const void* secret, size_t secretSize,
                              XXH64_hash_t seed);
static __inline __attribute__((unused)) __attribute__((pure)) XXH_NAMESPACEXXH128_hash_t
XXH_INLINE_XXH3_128bits_withSecretandSeed(__attribute__((noescape)) const void* input, size_t length,
                               __attribute__((noescape)) const void* secret, size_t secretSize,
                               XXH64_hash_t seed64);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH_INLINE_XXH3_64bits_reset_withSecretandSeed(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr,
                                    __attribute__((noescape)) const void* secret, size_t secretSize,
                                    XXH64_hash_t seed64);
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH_INLINE_XXH3_128bits_reset_withSecretandSeed(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr,
                                     __attribute__((noescape)) const void* secret, size_t secretSize,
                                     XXH64_hash_t seed64);
static __attribute__((malloc)) void* XXH_malloc(size_t s) { return malloc(s); }
static void XXH_free(void* p) { free(p); }
static void* XXH_memcpy(void* dest, const void* src, size_t size)
{
    return __builtin___memcpy_chk (dest, src,size, __builtin_object_size (dest, 0));
}
 typedef uint8_t xxh_u8;
typedef XXH32_hash_t xxh_u32;
static xxh_u32 XXH_read32(const void* ptr)
{
    typedef __attribute__((aligned(1))) xxh_u32 xxh_unalign32;
    return *((const xxh_unalign32*)ptr);
}
static xxh_u32 XXH_swap32 (xxh_u32 x)
{
    return ((x << 24) & 0xff000000 ) |
            ((x << 8) & 0x00ff0000 ) |
            ((x >> 8) & 0x0000ff00 ) |
            ((x >> 24) & 0x000000ff );
}
typedef enum {
    XXH_aligned,
    XXH_unaligned
} XXH_alignment;
static __attribute__((unused)) xxh_u32 XXH_readLE32(const void* ptr)
{
    return 1 ? XXH_read32(ptr) : XXH_swap32(XXH_read32(ptr));
}
static xxh_u32 XXH_readBE32(const void* ptr)
{
    return 1 ? XXH_swap32(XXH_read32(ptr)) : XXH_read32(ptr);
}
static __attribute__((unused)) xxh_u32
XXH_readLE32_align(const void* ptr, XXH_alignment align)
{
    if (align==XXH_unaligned) {
        return XXH_readLE32(ptr);
    } else {
        return 1 ? *(const xxh_u32*)ptr : XXH_swap32(*(const xxh_u32*)ptr);
    }
}
static __inline __attribute__((unused)) unsigned XXH_INLINE_XXH_versionNumber (void) { return (0 *100*100 + 8 *100 + 2); }
static xxh_u32 XXH32_round(xxh_u32 acc, xxh_u32 input)
{
    acc += input * 0x85EBCA77U;
    acc = __builtin_rotateleft32(acc, 13);
    acc *= 0x9E3779B1U;
    __asm__("" : "+r" (acc));
    return acc;
}
static xxh_u32 XXH32_avalanche(xxh_u32 hash)
{
    hash ^= hash >> 15;
    hash *= 0x85EBCA77U;
    hash ^= hash >> 13;
    hash *= 0xC2B2AE3DU;
    hash ^= hash >> 16;
    return hash;
}
static __attribute__((pure)) xxh_u32
XXH32_finalize(xxh_u32 hash, const xxh_u8* ptr, size_t len, XXH_alignment align)
{
    if (ptr==((void*)0)) __builtin_assume(len == 0);
    if (!0) {
        len &= 15;
        while (len >= 4) {
            do { hash += XXH_readLE32_align(ptr, align) * 0xC2B2AE3DU; ptr += 4; hash = __builtin_rotateleft32(hash, 17) * 0x27D4EB2FU; } while (0);
            len -= 4;
        }
        while (len > 0) {
            do { hash += (*ptr++) * 0x165667B1U; hash = __builtin_rotateleft32(hash, 11) * 0x9E3779B1U; } while (0);
            --len;
        }
        return XXH32_avalanche(hash);
    } else {
         switch(len&15) {
           case 12: do { hash += XXH_readLE32_align(ptr, align) * 0xC2B2AE3DU; ptr += 4; hash = __builtin_rotateleft32(hash, 17) * 0x27D4EB2FU; } while (0);
                         __attribute__ ((__fallthrough__));
           case 8: do { hash += XXH_readLE32_align(ptr, align) * 0xC2B2AE3DU; ptr += 4; hash = __builtin_rotateleft32(hash, 17) * 0x27D4EB2FU; } while (0);
                         __attribute__ ((__fallthrough__));
           case 4: do { hash += XXH_readLE32_align(ptr, align) * 0xC2B2AE3DU; ptr += 4; hash = __builtin_rotateleft32(hash, 17) * 0x27D4EB2FU; } while (0);
                         return XXH32_avalanche(hash);
           case 13: do { hash += XXH_readLE32_align(ptr, align) * 0xC2B2AE3DU; ptr += 4; hash = __builtin_rotateleft32(hash, 17) * 0x27D4EB2FU; } while (0);
                         __attribute__ ((__fallthrough__));
           case 9: do { hash += XXH_readLE32_align(ptr, align) * 0xC2B2AE3DU; ptr += 4; hash = __builtin_rotateleft32(hash, 17) * 0x27D4EB2FU; } while (0);
                         __attribute__ ((__fallthrough__));
           case 5: do { hash += XXH_readLE32_align(ptr, align) * 0xC2B2AE3DU; ptr += 4; hash = __builtin_rotateleft32(hash, 17) * 0x27D4EB2FU; } while (0);
                         do { hash += (*ptr++) * 0x165667B1U; hash = __builtin_rotateleft32(hash, 11) * 0x9E3779B1U; } while (0);
                         return XXH32_avalanche(hash);
           case 14: do { hash += XXH_readLE32_align(ptr, align) * 0xC2B2AE3DU; ptr += 4; hash = __builtin_rotateleft32(hash, 17) * 0x27D4EB2FU; } while (0);
                         __attribute__ ((__fallthrough__));
           case 10: do { hash += XXH_readLE32_align(ptr, align) * 0xC2B2AE3DU; ptr += 4; hash = __builtin_rotateleft32(hash, 17) * 0x27D4EB2FU; } while (0);
                         __attribute__ ((__fallthrough__));
           case 6: do { hash += XXH_readLE32_align(ptr, align) * 0xC2B2AE3DU; ptr += 4; hash = __builtin_rotateleft32(hash, 17) * 0x27D4EB2FU; } while (0);
                         do { hash += (*ptr++) * 0x165667B1U; hash = __builtin_rotateleft32(hash, 11) * 0x9E3779B1U; } while (0);
                         do { hash += (*ptr++) * 0x165667B1U; hash = __builtin_rotateleft32(hash, 11) * 0x9E3779B1U; } while (0);
                         return XXH32_avalanche(hash);
           case 15: do { hash += XXH_readLE32_align(ptr, align) * 0xC2B2AE3DU; ptr += 4; hash = __builtin_rotateleft32(hash, 17) * 0x27D4EB2FU; } while (0);
                         __attribute__ ((__fallthrough__));
           case 11: do { hash += XXH_readLE32_align(ptr, align) * 0xC2B2AE3DU; ptr += 4; hash = __builtin_rotateleft32(hash, 17) * 0x27D4EB2FU; } while (0);
                         __attribute__ ((__fallthrough__));
           case 7: do { hash += XXH_readLE32_align(ptr, align) * 0xC2B2AE3DU; ptr += 4; hash = __builtin_rotateleft32(hash, 17) * 0x27D4EB2FU; } while (0);
                         __attribute__ ((__fallthrough__));
           case 3: do { hash += (*ptr++) * 0x165667B1U; hash = __builtin_rotateleft32(hash, 11) * 0x9E3779B1U; } while (0);
                         __attribute__ ((__fallthrough__));
           case 2: do { hash += (*ptr++) * 0x165667B1U; hash = __builtin_rotateleft32(hash, 11) * 0x9E3779B1U; } while (0);
                         __attribute__ ((__fallthrough__));
           case 1: do { hash += (*ptr++) * 0x165667B1U; hash = __builtin_rotateleft32(hash, 11) * 0x9E3779B1U; } while (0);
                         __attribute__ ((__fallthrough__));
           case 0: return XXH32_avalanche(hash);
        }
        __builtin_assume(0);
        return hash;
    }
}
static __attribute__((unused)) __attribute__((pure)) xxh_u32
XXH32_endian_align(const xxh_u8* input, size_t len, xxh_u32 seed, XXH_alignment align)
{
    xxh_u32 h32;
    if (input==((void*)0)) __builtin_assume(len == 0);
    if (len>=16) {
        const xxh_u8* const bEnd = input + len;
        const xxh_u8* const limit = bEnd - 15;
        xxh_u32 v1 = seed + 0x9E3779B1U + 0x85EBCA77U;
        xxh_u32 v2 = seed + 0x85EBCA77U;
        xxh_u32 v3 = seed + 0;
        xxh_u32 v4 = seed - 0x9E3779B1U;
        do {
            v1 = XXH32_round(v1, XXH_readLE32_align(input, align)); input += 4;
            v2 = XXH32_round(v2, XXH_readLE32_align(input, align)); input += 4;
            v3 = XXH32_round(v3, XXH_readLE32_align(input, align)); input += 4;
            v4 = XXH32_round(v4, XXH_readLE32_align(input, align)); input += 4;
        } while (input < limit);
        h32 = __builtin_rotateleft32(v1, 1) + __builtin_rotateleft32(v2, 7)
            + __builtin_rotateleft32(v3, 12) + __builtin_rotateleft32(v4, 18);
    } else {
        h32 = seed + 0x165667B1U;
    }
    h32 += (xxh_u32)len;
    return XXH32_finalize(h32, input, len&15, align);
}
static __inline __attribute__((unused)) XXH32_hash_t XXH_INLINE_XXH32 (const void* input, size_t len, XXH32_hash_t seed)
{
    if (0) {
        if ((((size_t)input) & 3) == 0) {
            return XXH32_endian_align((const xxh_u8*)input, len, seed, XXH_aligned);
    } }
    return XXH32_endian_align((const xxh_u8*)input, len, seed, XXH_unaligned);
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH32_state_t* XXH_INLINE_XXH32_createState(void)
{
    return (XXH_NAMESPACEXXH32_state_t*)XXH_malloc(sizeof(XXH_NAMESPACEXXH32_state_t));
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH32_freeState(XXH_NAMESPACEXXH32_state_t* statePtr)
{
    XXH_free(statePtr);
    return XXH_NAMESPACEXXH_OK;
}
static __inline __attribute__((unused)) void XXH_INLINE_XXH32_copyState(XXH_NAMESPACEXXH32_state_t* dstState, const XXH_NAMESPACEXXH32_state_t* srcState)
{
    XXH_memcpy(dstState, srcState, sizeof(*dstState));
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH32_reset(XXH_NAMESPACEXXH32_state_t* statePtr, XXH32_hash_t seed)
{
    __builtin_assume(statePtr != ((void*)0));
    __builtin___memset_chk (statePtr, 0, sizeof(*statePtr), __builtin_object_size (statePtr, 0));
    statePtr->v[0] = seed + 0x9E3779B1U + 0x85EBCA77U;
    statePtr->v[1] = seed + 0x85EBCA77U;
    statePtr->v[2] = seed + 0;
    statePtr->v[3] = seed - 0x9E3779B1U;
    return XXH_NAMESPACEXXH_OK;
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH_INLINE_XXH32_update(XXH_NAMESPACEXXH32_state_t* state, const void* input, size_t len)
{
    if (input==((void*)0)) {
        __builtin_assume(len == 0);
        return XXH_NAMESPACEXXH_OK;
    }
    { const xxh_u8* p = (const xxh_u8*)input;
        const xxh_u8* const bEnd = p + len;
        state->total_len_32 += (XXH32_hash_t)len;
        state->large_len |= (XXH32_hash_t)((len>=16) | (state->total_len_32>=16));
        if (state->memsize + len < 16) {
            XXH_memcpy((xxh_u8*)(state->mem32) + state->memsize, input, len);
            state->memsize += (XXH32_hash_t)len;
            return XXH_NAMESPACEXXH_OK;
        }
        if (state->memsize) {
            XXH_memcpy((xxh_u8*)(state->mem32) + state->memsize, input, 16-state->memsize);
            { const xxh_u32* p32 = state->mem32;
                state->v[0] = XXH32_round(state->v[0], XXH_readLE32(p32)); p32++;
                state->v[1] = XXH32_round(state->v[1], XXH_readLE32(p32)); p32++;
                state->v[2] = XXH32_round(state->v[2], XXH_readLE32(p32)); p32++;
                state->v[3] = XXH32_round(state->v[3], XXH_readLE32(p32));
            }
            p += 16-state->memsize;
            state->memsize = 0;
        }
        if (p <= bEnd-16) {
            const xxh_u8* const limit = bEnd - 16;
            do {
                state->v[0] = XXH32_round(state->v[0], XXH_readLE32(p)); p+=4;
                state->v[1] = XXH32_round(state->v[1], XXH_readLE32(p)); p+=4;
                state->v[2] = XXH32_round(state->v[2], XXH_readLE32(p)); p+=4;
                state->v[3] = XXH32_round(state->v[3], XXH_readLE32(p)); p+=4;
            } while (p<=limit);
        }
        if (p < bEnd) {
            XXH_memcpy(state->mem32, p, (size_t)(bEnd-p));
            state->memsize = (unsigned)(bEnd-p);
        }
    }
    return XXH_NAMESPACEXXH_OK;
}
static __inline __attribute__((unused)) XXH32_hash_t XXH_INLINE_XXH32_digest(const XXH_NAMESPACEXXH32_state_t* state)
{
    xxh_u32 h32;
    if (state->large_len) {
        h32 = __builtin_rotateleft32(state->v[0], 1)
            + __builtin_rotateleft32(state->v[1], 7)
            + __builtin_rotateleft32(state->v[2], 12)
            + __builtin_rotateleft32(state->v[3], 18);
    } else {
        h32 = state->v[2] + 0x165667B1U;
    }
    h32 += state->total_len_32;
    return XXH32_finalize(h32, (const xxh_u8*)state->mem32, state->memsize, XXH_aligned);
}
static __inline __attribute__((unused)) void XXH_INLINE_XXH32_canonicalFromHash(XXH_NAMESPACEXXH32_canonical_t* dst, XXH32_hash_t hash)
{
    do { _Static_assert(((sizeof(XXH_NAMESPACEXXH32_canonical_t) == sizeof(XXH32_hash_t))),"sizeof(XXH32_canonical_t) == sizeof(XXH32_hash_t)"); } while(0);
    if (1) hash = XXH_swap32(hash);
    XXH_memcpy(dst, &hash, sizeof(*dst));
}
static __inline __attribute__((unused)) XXH32_hash_t XXH_INLINE_XXH32_hashFromCanonical(const XXH_NAMESPACEXXH32_canonical_t* src)
{
    return XXH_readBE32(src);
}
typedef XXH64_hash_t xxh_u64;
static xxh_u64 XXH_read64(const void* ptr)
{
    typedef __attribute__((aligned(1))) xxh_u64 xxh_unalign64;
    return *((const xxh_unalign64*)ptr);
}
static xxh_u64 XXH_swap64(xxh_u64 x)
{
    return ((x << 56) & 0xff00000000000000ULL) |
            ((x << 40) & 0x00ff000000000000ULL) |
            ((x << 24) & 0x0000ff0000000000ULL) |
            ((x << 8) & 0x000000ff00000000ULL) |
            ((x >> 8) & 0x00000000ff000000ULL) |
            ((x >> 24) & 0x0000000000ff0000ULL) |
            ((x >> 40) & 0x000000000000ff00ULL) |
            ((x >> 56) & 0x00000000000000ffULL);
}
static __attribute__((unused)) xxh_u64 XXH_readLE64(const void* ptr)
{
    return 1 ? XXH_read64(ptr) : XXH_swap64(XXH_read64(ptr));
}
static xxh_u64 XXH_readBE64(const void* ptr)
{
    return 1 ? XXH_swap64(XXH_read64(ptr)) : XXH_read64(ptr);
}
static __attribute__((unused)) xxh_u64
XXH_readLE64_align(const void* ptr, XXH_alignment align)
{
    if (align==XXH_unaligned)
        return XXH_readLE64(ptr);
    else
        return 1 ? *(const xxh_u64*)ptr : XXH_swap64(*(const xxh_u64*)ptr);
}
static xxh_u64 XXH64_round(xxh_u64 acc, xxh_u64 input)
{
    acc += input * 0xC2B2AE3D27D4EB4FULL;
    acc = __builtin_rotateleft64(acc, 31);
    acc *= 0x9E3779B185EBCA87ULL;
    return acc;
}
static xxh_u64 XXH64_mergeRound(xxh_u64 acc, xxh_u64 val)
{
    val = XXH64_round(0, val);
    acc ^= val;
    acc = acc * 0x9E3779B185EBCA87ULL + 0x85EBCA77C2B2AE63ULL;
    return acc;
}
static xxh_u64 XXH64_avalanche(xxh_u64 hash)
{
    hash ^= hash >> 33;
    hash *= 0xC2B2AE3D27D4EB4FULL;
    hash ^= hash >> 29;
    hash *= 0x165667B19E3779F9ULL;
    hash ^= hash >> 32;
    return hash;
}
static __attribute__((pure)) xxh_u64
XXH64_finalize(xxh_u64 hash, const xxh_u8* ptr, size_t len, XXH_alignment align)
{
    if (ptr==((void*)0)) __builtin_assume(len == 0);
    len &= 31;
    while (len >= 8) {
        xxh_u64 const k1 = XXH64_round(0, XXH_readLE64_align(ptr, align));
        ptr += 8;
        hash ^= k1;
        hash = __builtin_rotateleft64(hash,27) * 0x9E3779B185EBCA87ULL + 0x85EBCA77C2B2AE63ULL;
        len -= 8;
    }
    if (len >= 4) {
        hash ^= (xxh_u64)(XXH_readLE32_align(ptr, align)) * 0x9E3779B185EBCA87ULL;
        ptr += 4;
        hash = __builtin_rotateleft64(hash, 23) * 0xC2B2AE3D27D4EB4FULL + 0x165667B19E3779F9ULL;
        len -= 4;
    }
    while (len > 0) {
        hash ^= (*ptr++) * 0x27D4EB2F165667C5ULL;
        hash = __builtin_rotateleft64(hash, 11) * 0x9E3779B185EBCA87ULL;
        --len;
    }
    return XXH64_avalanche(hash);
}
static __attribute__((unused)) __attribute__((pure)) xxh_u64
XXH64_endian_align(const xxh_u8* input, size_t len, xxh_u64 seed, XXH_alignment align)
{
    xxh_u64 h64;
    if (input==((void*)0)) __builtin_assume(len == 0);
    if (len>=32) {
        const xxh_u8* const bEnd = input + len;
        const xxh_u8* const limit = bEnd - 31;
        xxh_u64 v1 = seed + 0x9E3779B185EBCA87ULL + 0xC2B2AE3D27D4EB4FULL;
        xxh_u64 v2 = seed + 0xC2B2AE3D27D4EB4FULL;
        xxh_u64 v3 = seed + 0;
        xxh_u64 v4 = seed - 0x9E3779B185EBCA87ULL;
        do {
            v1 = XXH64_round(v1, XXH_readLE64_align(input, align)); input+=8;
            v2 = XXH64_round(v2, XXH_readLE64_align(input, align)); input+=8;
            v3 = XXH64_round(v3, XXH_readLE64_align(input, align)); input+=8;
            v4 = XXH64_round(v4, XXH_readLE64_align(input, align)); input+=8;
        } while (input<limit);
        h64 = __builtin_rotateleft64(v1, 1) + __builtin_rotateleft64(v2, 7) + __builtin_rotateleft64(v3, 12) + __builtin_rotateleft64(v4, 18);
        h64 = XXH64_mergeRound(h64, v1);
        h64 = XXH64_mergeRound(h64, v2);
        h64 = XXH64_mergeRound(h64, v3);
        h64 = XXH64_mergeRound(h64, v4);
    } else {
        h64 = seed + 0x27D4EB2F165667C5ULL;
    }
    h64 += (xxh_u64) len;
    return XXH64_finalize(h64, input, len, align);
}
static __inline __attribute__((unused)) XXH64_hash_t XXH_INLINE_XXH64 (__attribute__((noescape)) const void* input, size_t len, XXH64_hash_t seed)
{
    if (0) {
        if ((((size_t)input) & 7)==0) {
            return XXH64_endian_align((const xxh_u8*)input, len, seed, XXH_aligned);
    } }
    return XXH64_endian_align((const xxh_u8*)input, len, seed, XXH_unaligned);
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH64_state_t* XXH_INLINE_XXH64_createState(void)
{
    return (XXH_NAMESPACEXXH64_state_t*)XXH_malloc(sizeof(XXH_NAMESPACEXXH64_state_t));
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH64_freeState(XXH_NAMESPACEXXH64_state_t* statePtr)
{
    XXH_free(statePtr);
    return XXH_NAMESPACEXXH_OK;
}
static __inline __attribute__((unused)) void XXH_INLINE_XXH64_copyState(__attribute__((noescape)) XXH_NAMESPACEXXH64_state_t* dstState, const XXH_NAMESPACEXXH64_state_t* srcState)
{
    XXH_memcpy(dstState, srcState, sizeof(*dstState));
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH64_reset(__attribute__((noescape)) XXH_NAMESPACEXXH64_state_t* statePtr, XXH64_hash_t seed)
{
    __builtin_assume(statePtr != ((void*)0));
    __builtin___memset_chk (statePtr, 0, sizeof(*statePtr), __builtin_object_size (statePtr, 0));
    statePtr->v[0] = seed + 0x9E3779B185EBCA87ULL + 0xC2B2AE3D27D4EB4FULL;
    statePtr->v[1] = seed + 0xC2B2AE3D27D4EB4FULL;
    statePtr->v[2] = seed + 0;
    statePtr->v[3] = seed - 0x9E3779B185EBCA87ULL;
    return XXH_NAMESPACEXXH_OK;
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH_INLINE_XXH64_update (__attribute__((noescape)) XXH_NAMESPACEXXH64_state_t* state, __attribute__((noescape)) const void* input, size_t len)
{
    if (input==((void*)0)) {
        __builtin_assume(len == 0);
        return XXH_NAMESPACEXXH_OK;
    }
    { const xxh_u8* p = (const xxh_u8*)input;
        const xxh_u8* const bEnd = p + len;
        state->total_len += len;
        if (state->memsize + len < 32) {
            XXH_memcpy(((xxh_u8*)state->mem64) + state->memsize, input, len);
            state->memsize += (xxh_u32)len;
            return XXH_NAMESPACEXXH_OK;
        }
        if (state->memsize) {
            XXH_memcpy(((xxh_u8*)state->mem64) + state->memsize, input, 32-state->memsize);
            state->v[0] = XXH64_round(state->v[0], XXH_readLE64(state->mem64+0));
            state->v[1] = XXH64_round(state->v[1], XXH_readLE64(state->mem64+1));
            state->v[2] = XXH64_round(state->v[2], XXH_readLE64(state->mem64+2));
            state->v[3] = XXH64_round(state->v[3], XXH_readLE64(state->mem64+3));
            p += 32 - state->memsize;
            state->memsize = 0;
        }
        if (p+32 <= bEnd) {
            const xxh_u8* const limit = bEnd - 32;
            do {
                state->v[0] = XXH64_round(state->v[0], XXH_readLE64(p)); p+=8;
                state->v[1] = XXH64_round(state->v[1], XXH_readLE64(p)); p+=8;
                state->v[2] = XXH64_round(state->v[2], XXH_readLE64(p)); p+=8;
                state->v[3] = XXH64_round(state->v[3], XXH_readLE64(p)); p+=8;
            } while (p<=limit);
        }
        if (p < bEnd) {
            XXH_memcpy(state->mem64, p, (size_t)(bEnd-p));
            state->memsize = (unsigned)(bEnd-p);
        }
    }
    return XXH_NAMESPACEXXH_OK;
}
static __inline __attribute__((unused)) XXH64_hash_t XXH_INLINE_XXH64_digest(__attribute__((noescape)) const XXH_NAMESPACEXXH64_state_t* state)
{
    xxh_u64 h64;
    if (state->total_len >= 32) {
        h64 = __builtin_rotateleft64(state->v[0], 1) + __builtin_rotateleft64(state->v[1], 7) + __builtin_rotateleft64(state->v[2], 12) + __builtin_rotateleft64(state->v[3], 18);
        h64 = XXH64_mergeRound(h64, state->v[0]);
        h64 = XXH64_mergeRound(h64, state->v[1]);
        h64 = XXH64_mergeRound(h64, state->v[2]);
        h64 = XXH64_mergeRound(h64, state->v[3]);
    } else {
        h64 = state->v[2] + 0x27D4EB2F165667C5ULL;
    }
    h64 += (xxh_u64) state->total_len;
    return XXH64_finalize(h64, (const xxh_u8*)state->mem64, (size_t)state->total_len, XXH_aligned);
}
static __inline __attribute__((unused)) void XXH_INLINE_XXH64_canonicalFromHash(__attribute__((noescape)) XXH_NAMESPACEXXH64_canonical_t* dst, XXH64_hash_t hash)
{
    do { _Static_assert(((sizeof(XXH_NAMESPACEXXH64_canonical_t) == sizeof(XXH64_hash_t))),"sizeof(XXH64_canonical_t) == sizeof(XXH64_hash_t)"); } while(0);
    if (1) hash = XXH_swap64(hash);
    XXH_memcpy(dst, &hash, sizeof(*dst));
}
static __inline __attribute__((unused)) XXH64_hash_t XXH_INLINE_XXH64_hashFromCanonical(__attribute__((noescape)) const XXH_NAMESPACEXXH64_canonical_t* src)
{
    return XXH_readBE64(src);
}
typedef long long __m64 __attribute__((__vector_size__(8), __aligned__(8)));
typedef long long __v1di __attribute__((__vector_size__(8)));
typedef int __v2si __attribute__((__vector_size__(8)));
typedef short __v4hi __attribute__((__vector_size__(8)));
typedef char __v8qi __attribute__((__vector_size__(8)));
static __inline__ void __attribute__((__always_inline__, __nodebug__,
                                      __target__("mmx,no-evex512")))
_mm_empty(void) {
  __builtin_ia32_emms();
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_cvtsi32_si64(int __i)
{
    return (__m64)__builtin_ia32_vec_init_v2si(__i, 0);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_cvtsi64_si32(__m64 __m)
{
    return __builtin_ia32_vec_ext_v2si((__v2si)__m, 0);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_cvtsi64_m64(long long __i)
{
    return (__m64)__i;
}
static __inline__ long long __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_cvtm64_si64(__m64 __m)
{
    return (long long)__m;
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_packs_pi16(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_packsswb((__v4hi)__m1, (__v4hi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_packs_pi32(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_packssdw((__v2si)__m1, (__v2si)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_packs_pu16(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_packuswb((__v4hi)__m1, (__v4hi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_unpackhi_pi8(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_punpckhbw((__v8qi)__m1, (__v8qi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_unpackhi_pi16(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_punpckhwd((__v4hi)__m1, (__v4hi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_unpackhi_pi32(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_punpckhdq((__v2si)__m1, (__v2si)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_unpacklo_pi8(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_punpcklbw((__v8qi)__m1, (__v8qi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_unpacklo_pi16(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_punpcklwd((__v4hi)__m1, (__v4hi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_unpacklo_pi32(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_punpckldq((__v2si)__m1, (__v2si)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_add_pi8(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_paddb((__v8qi)__m1, (__v8qi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_add_pi16(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_paddw((__v4hi)__m1, (__v4hi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_add_pi32(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_paddd((__v2si)__m1, (__v2si)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_adds_pi8(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_paddsb((__v8qi)__m1, (__v8qi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_adds_pi16(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_paddsw((__v4hi)__m1, (__v4hi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_adds_pu8(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_paddusb((__v8qi)__m1, (__v8qi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_adds_pu16(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_paddusw((__v4hi)__m1, (__v4hi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_sub_pi8(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_psubb((__v8qi)__m1, (__v8qi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_sub_pi16(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_psubw((__v4hi)__m1, (__v4hi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_sub_pi32(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_psubd((__v2si)__m1, (__v2si)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_subs_pi8(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_psubsb((__v8qi)__m1, (__v8qi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_subs_pi16(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_psubsw((__v4hi)__m1, (__v4hi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_subs_pu8(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_psubusb((__v8qi)__m1, (__v8qi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_subs_pu16(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_psubusw((__v4hi)__m1, (__v4hi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_madd_pi16(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_pmaddwd((__v4hi)__m1, (__v4hi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_mulhi_pi16(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_pmulhw((__v4hi)__m1, (__v4hi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_mullo_pi16(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_pmullw((__v4hi)__m1, (__v4hi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_sll_pi16(__m64 __m, __m64 __count)
{
    return (__m64)__builtin_ia32_psllw((__v4hi)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_slli_pi16(__m64 __m, int __count)
{
    return (__m64)__builtin_ia32_psllwi((__v4hi)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_sll_pi32(__m64 __m, __m64 __count)
{
    return (__m64)__builtin_ia32_pslld((__v2si)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_slli_pi32(__m64 __m, int __count)
{
    return (__m64)__builtin_ia32_pslldi((__v2si)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_sll_si64(__m64 __m, __m64 __count)
{
    return (__m64)__builtin_ia32_psllq((__v1di)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_slli_si64(__m64 __m, int __count)
{
    return (__m64)__builtin_ia32_psllqi((__v1di)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_sra_pi16(__m64 __m, __m64 __count)
{
    return (__m64)__builtin_ia32_psraw((__v4hi)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_srai_pi16(__m64 __m, int __count)
{
    return (__m64)__builtin_ia32_psrawi((__v4hi)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_sra_pi32(__m64 __m, __m64 __count)
{
    return (__m64)__builtin_ia32_psrad((__v2si)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_srai_pi32(__m64 __m, int __count)
{
    return (__m64)__builtin_ia32_psradi((__v2si)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_srl_pi16(__m64 __m, __m64 __count)
{
    return (__m64)__builtin_ia32_psrlw((__v4hi)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_srli_pi16(__m64 __m, int __count)
{
    return (__m64)__builtin_ia32_psrlwi((__v4hi)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_srl_pi32(__m64 __m, __m64 __count)
{
    return (__m64)__builtin_ia32_psrld((__v2si)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_srli_pi32(__m64 __m, int __count)
{
    return (__m64)__builtin_ia32_psrldi((__v2si)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_srl_si64(__m64 __m, __m64 __count)
{
    return (__m64)__builtin_ia32_psrlq((__v1di)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_srli_si64(__m64 __m, int __count)
{
    return (__m64)__builtin_ia32_psrlqi((__v1di)__m, __count);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_and_si64(__m64 __m1, __m64 __m2)
{
    return __builtin_ia32_pand((__v1di)__m1, (__v1di)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_andnot_si64(__m64 __m1, __m64 __m2)
{
    return __builtin_ia32_pandn((__v1di)__m1, (__v1di)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_or_si64(__m64 __m1, __m64 __m2)
{
    return __builtin_ia32_por((__v1di)__m1, (__v1di)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_xor_si64(__m64 __m1, __m64 __m2)
{
    return __builtin_ia32_pxor((__v1di)__m1, (__v1di)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_cmpeq_pi8(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_pcmpeqb((__v8qi)__m1, (__v8qi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_cmpeq_pi16(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_pcmpeqw((__v4hi)__m1, (__v4hi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_cmpeq_pi32(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_pcmpeqd((__v2si)__m1, (__v2si)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_cmpgt_pi8(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_pcmpgtb((__v8qi)__m1, (__v8qi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_cmpgt_pi16(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_pcmpgtw((__v4hi)__m1, (__v4hi)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_cmpgt_pi32(__m64 __m1, __m64 __m2)
{
    return (__m64)__builtin_ia32_pcmpgtd((__v2si)__m1, (__v2si)__m2);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_setzero_si64(void)
{
    return __extension__ (__m64){ 0LL };
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_set_pi32(int __i1, int __i0)
{
    return (__m64)__builtin_ia32_vec_init_v2si(__i0, __i1);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_set_pi16(short __s3, short __s2, short __s1, short __s0)
{
    return (__m64)__builtin_ia32_vec_init_v4hi(__s0, __s1, __s2, __s3);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_set_pi8(char __b7, char __b6, char __b5, char __b4, char __b3, char __b2,
            char __b1, char __b0)
{
    return (__m64)__builtin_ia32_vec_init_v8qi(__b0, __b1, __b2, __b3,
                                               __b4, __b5, __b6, __b7);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_set1_pi32(int __i)
{
    return _mm_set_pi32(__i, __i);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_set1_pi16(short __w)
{
    return _mm_set_pi16(__w, __w, __w, __w);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_set1_pi8(char __b)
{
    return _mm_set_pi8(__b, __b, __b, __b, __b, __b, __b, __b);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_setr_pi32(int __i0, int __i1)
{
    return _mm_set_pi32(__i1, __i0);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_setr_pi16(short __w0, short __w1, short __w2, short __w3)
{
    return _mm_set_pi16(__w3, __w2, __w1, __w0);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,no-evex512"), __min_vector_width__(64)))
_mm_setr_pi8(char __b0, char __b1, char __b2, char __b3, char __b4, char __b5,
             char __b6, char __b7)
{
    return _mm_set_pi8(__b7, __b6, __b5, __b4, __b3, __b2, __b1, __b0);
}

typedef int __v4si __attribute__((__vector_size__(16)));
typedef float __v4sf __attribute__((__vector_size__(16)));
typedef float __m128 __attribute__((__vector_size__(16), __aligned__(16)));
typedef float __m128_u __attribute__((__vector_size__(16), __aligned__(1)));
typedef unsigned int __v4su __attribute__((__vector_size__(16)));
extern int posix_memalign(void **__memptr, size_t __alignment, size_t __size);
static __inline__ void *__attribute__((__always_inline__, __nodebug__,
                                       __malloc__, __alloc_size__(1),
                                       __alloc_align__(2)))
_mm_malloc(size_t __size, size_t __align) {
  if (__align == 1) {
    return malloc(__size);
  }
  if (!(__align & (__align - 1)) && __align < sizeof(void *))
    __align = sizeof(void *);
  void *__mallocedMemory;
  if (posix_memalign(&__mallocedMemory, __align, __size))
    return 0;
  return __mallocedMemory;
}
static __inline__ void __attribute__((__always_inline__, __nodebug__))
_mm_free(void *__p)
{
  free(__p);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_add_ss(__m128 __a, __m128 __b)
{
  __a[0] += __b[0];
  return __a;
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_add_ps(__m128 __a, __m128 __b)
{
  return (__m128)((__v4sf)__a + (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_sub_ss(__m128 __a, __m128 __b)
{
  __a[0] -= __b[0];
  return __a;
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_sub_ps(__m128 __a, __m128 __b)
{
  return (__m128)((__v4sf)__a - (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_mul_ss(__m128 __a, __m128 __b)
{
  __a[0] *= __b[0];
  return __a;
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_mul_ps(__m128 __a, __m128 __b)
{
  return (__m128)((__v4sf)__a * (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_div_ss(__m128 __a, __m128 __b)
{
  __a[0] /= __b[0];
  return __a;
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_div_ps(__m128 __a, __m128 __b)
{
  return (__m128)((__v4sf)__a / (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_sqrt_ss(__m128 __a)
{
  return (__m128)__builtin_ia32_sqrtss((__v4sf)__a);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_sqrt_ps(__m128 __a)
{
  return __builtin_ia32_sqrtps((__v4sf)__a);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_rcp_ss(__m128 __a)
{
  return (__m128)__builtin_ia32_rcpss((__v4sf)__a);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_rcp_ps(__m128 __a)
{
  return (__m128)__builtin_ia32_rcpps((__v4sf)__a);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_rsqrt_ss(__m128 __a)
{
  return __builtin_ia32_rsqrtss((__v4sf)__a);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_rsqrt_ps(__m128 __a)
{
  return __builtin_ia32_rsqrtps((__v4sf)__a);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_min_ss(__m128 __a, __m128 __b)
{
  return __builtin_ia32_minss((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_min_ps(__m128 __a, __m128 __b)
{
  return __builtin_ia32_minps((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_max_ss(__m128 __a, __m128 __b)
{
  return __builtin_ia32_maxss((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_max_ps(__m128 __a, __m128 __b)
{
  return __builtin_ia32_maxps((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_and_ps(__m128 __a, __m128 __b)
{
  return (__m128)((__v4su)__a & (__v4su)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_andnot_ps(__m128 __a, __m128 __b)
{
  return (__m128)(~(__v4su)__a & (__v4su)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_or_ps(__m128 __a, __m128 __b)
{
  return (__m128)((__v4su)__a | (__v4su)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_xor_ps(__m128 __a, __m128 __b)
{
  return (__m128)((__v4su)__a ^ (__v4su)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpeq_ss(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpeqss((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpeq_ps(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpeqps((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmplt_ss(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpltss((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmplt_ps(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpltps((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmple_ss(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpless((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmple_ps(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpleps((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpgt_ss(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_shufflevector((__v4sf)__a,
                                         (__v4sf)__builtin_ia32_cmpltss((__v4sf)__b, (__v4sf)__a),
                                         4, 1, 2, 3);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpgt_ps(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpltps((__v4sf)__b, (__v4sf)__a);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpge_ss(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_shufflevector((__v4sf)__a,
                                         (__v4sf)__builtin_ia32_cmpless((__v4sf)__b, (__v4sf)__a),
                                         4, 1, 2, 3);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpge_ps(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpleps((__v4sf)__b, (__v4sf)__a);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpneq_ss(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpneqss((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpneq_ps(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpneqps((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpnlt_ss(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpnltss((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpnlt_ps(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpnltps((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpnle_ss(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpnless((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpnle_ps(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpnleps((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpngt_ss(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_shufflevector((__v4sf)__a,
                                         (__v4sf)__builtin_ia32_cmpnltss((__v4sf)__b, (__v4sf)__a),
                                         4, 1, 2, 3);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpngt_ps(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpnltps((__v4sf)__b, (__v4sf)__a);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpnge_ss(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_shufflevector((__v4sf)__a,
                                         (__v4sf)__builtin_ia32_cmpnless((__v4sf)__b, (__v4sf)__a),
                                         4, 1, 2, 3);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpnge_ps(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpnleps((__v4sf)__b, (__v4sf)__a);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpord_ss(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpordss((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpord_ps(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpordps((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpunord_ss(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpunordss((__v4sf)__a, (__v4sf)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cmpunord_ps(__m128 __a, __m128 __b)
{
  return (__m128)__builtin_ia32_cmpunordps((__v4sf)__a, (__v4sf)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_comieq_ss(__m128 __a, __m128 __b)
{
  return __builtin_ia32_comieq((__v4sf)__a, (__v4sf)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_comilt_ss(__m128 __a, __m128 __b)
{
  return __builtin_ia32_comilt((__v4sf)__a, (__v4sf)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_comile_ss(__m128 __a, __m128 __b)
{
  return __builtin_ia32_comile((__v4sf)__a, (__v4sf)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_comigt_ss(__m128 __a, __m128 __b)
{
  return __builtin_ia32_comigt((__v4sf)__a, (__v4sf)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_comige_ss(__m128 __a, __m128 __b)
{
  return __builtin_ia32_comige((__v4sf)__a, (__v4sf)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_comineq_ss(__m128 __a, __m128 __b)
{
  return __builtin_ia32_comineq((__v4sf)__a, (__v4sf)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_ucomieq_ss(__m128 __a, __m128 __b)
{
  return __builtin_ia32_ucomieq((__v4sf)__a, (__v4sf)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_ucomilt_ss(__m128 __a, __m128 __b)
{
  return __builtin_ia32_ucomilt((__v4sf)__a, (__v4sf)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_ucomile_ss(__m128 __a, __m128 __b)
{
  return __builtin_ia32_ucomile((__v4sf)__a, (__v4sf)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_ucomigt_ss(__m128 __a, __m128 __b)
{
  return __builtin_ia32_ucomigt((__v4sf)__a, (__v4sf)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_ucomige_ss(__m128 __a, __m128 __b)
{
  return __builtin_ia32_ucomige((__v4sf)__a, (__v4sf)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_ucomineq_ss(__m128 __a, __m128 __b)
{
  return __builtin_ia32_ucomineq((__v4sf)__a, (__v4sf)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cvtss_si32(__m128 __a)
{
  return __builtin_ia32_cvtss2si((__v4sf)__a);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cvt_ss2si(__m128 __a)
{
  return _mm_cvtss_si32(__a);
}
static __inline__ long long __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cvtss_si64(__m128 __a)
{
  return __builtin_ia32_cvtss2si64((__v4sf)__a);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_cvtps_pi32(__m128 __a)
{
  return (__m64)__builtin_ia32_cvtps2pi((__v4sf)__a);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_cvt_ps2pi(__m128 __a)
{
  return _mm_cvtps_pi32(__a);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cvttss_si32(__m128 __a)
{
  return __builtin_ia32_cvttss2si((__v4sf)__a);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cvtt_ss2si(__m128 __a)
{
  return _mm_cvttss_si32(__a);
}
static __inline__ long long __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cvttss_si64(__m128 __a)
{
  return __builtin_ia32_cvttss2si64((__v4sf)__a);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_cvttps_pi32(__m128 __a)
{
  return (__m64)__builtin_ia32_cvttps2pi((__v4sf)__a);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_cvtt_ps2pi(__m128 __a)
{
  return _mm_cvttps_pi32(__a);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cvtsi32_ss(__m128 __a, int __b)
{
  __a[0] = __b;
  return __a;
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cvt_si2ss(__m128 __a, int __b)
{
  return _mm_cvtsi32_ss(__a, __b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cvtsi64_ss(__m128 __a, long long __b)
{
  __a[0] = __b;
  return __a;
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_cvtpi32_ps(__m128 __a, __m64 __b)
{
  return __builtin_ia32_cvtpi2ps((__v4sf)__a, (__v2si)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_cvt_pi2ps(__m128 __a, __m64 __b)
{
  return _mm_cvtpi32_ps(__a, __b);
}
static __inline__ float __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_cvtss_f32(__m128 __a)
{
  return __a[0];
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_loadh_pi(__m128 __a, const __m64 *__p)
{
  typedef float __mm_loadh_pi_v2f32 __attribute__((__vector_size__(8)));
  struct __mm_loadh_pi_struct {
    __mm_loadh_pi_v2f32 __u;
  } __attribute__((__packed__, __may_alias__));
  __mm_loadh_pi_v2f32 __b = ((const struct __mm_loadh_pi_struct*)__p)->__u;
  __m128 __bb = __builtin_shufflevector(__b, __b, 0, 1, 0, 1);
  return __builtin_shufflevector(__a, __bb, 0, 1, 4, 5);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_loadl_pi(__m128 __a, const __m64 *__p)
{
  typedef float __mm_loadl_pi_v2f32 __attribute__((__vector_size__(8)));
  struct __mm_loadl_pi_struct {
    __mm_loadl_pi_v2f32 __u;
  } __attribute__((__packed__, __may_alias__));
  __mm_loadl_pi_v2f32 __b = ((const struct __mm_loadl_pi_struct*)__p)->__u;
  __m128 __bb = __builtin_shufflevector(__b, __b, 0, 1, 0, 1);
  return __builtin_shufflevector(__a, __bb, 4, 5, 2, 3);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_load_ss(const float *__p)
{
  struct __mm_load_ss_struct {
    float __u;
  } __attribute__((__packed__, __may_alias__));
  float __u = ((const struct __mm_load_ss_struct*)__p)->__u;
  return __extension__ (__m128){ __u, 0, 0, 0 };
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_load1_ps(const float *__p)
{
  struct __mm_load1_ps_struct {
    float __u;
  } __attribute__((__packed__, __may_alias__));
  float __u = ((const struct __mm_load1_ps_struct*)__p)->__u;
  return __extension__ (__m128){ __u, __u, __u, __u };
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_load_ps(const float *__p)
{
  return *(const __m128*)__p;
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_loadu_ps(const float *__p)
{
  struct __loadu_ps {
    __m128_u __v;
  } __attribute__((__packed__, __may_alias__));
  return ((const struct __loadu_ps*)__p)->__v;
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_loadr_ps(const float *__p)
{
  __m128 __a = _mm_load_ps(__p);
  return __builtin_shufflevector((__v4sf)__a, (__v4sf)__a, 3, 2, 1, 0);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_undefined_ps(void)
{
  return (__m128)__builtin_ia32_undef128();
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_set_ss(float __w)
{
  return __extension__ (__m128){ __w, 0.0f, 0.0f, 0.0f };
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_set1_ps(float __w)
{
  return __extension__ (__m128){ __w, __w, __w, __w };
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_set_ps1(float __w)
{
    return _mm_set1_ps(__w);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_set_ps(float __z, float __y, float __x, float __w)
{
  return __extension__ (__m128){ __w, __x, __y, __z };
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_setr_ps(float __z, float __y, float __x, float __w)
{
  return __extension__ (__m128){ __z, __y, __x, __w };
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_setzero_ps(void)
{
  return __extension__ (__m128){ 0.0f, 0.0f, 0.0f, 0.0f };
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_storeh_pi(__m64 *__p, __m128 __a)
{
  typedef float __mm_storeh_pi_v2f32 __attribute__((__vector_size__(8)));
  struct __mm_storeh_pi_struct {
    __mm_storeh_pi_v2f32 __u;
  } __attribute__((__packed__, __may_alias__));
  ((struct __mm_storeh_pi_struct*)__p)->__u = __builtin_shufflevector(__a, __a, 2, 3);
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_storel_pi(__m64 *__p, __m128 __a)
{
  typedef float __mm_storeh_pi_v2f32 __attribute__((__vector_size__(8)));
  struct __mm_storeh_pi_struct {
    __mm_storeh_pi_v2f32 __u;
  } __attribute__((__packed__, __may_alias__));
  ((struct __mm_storeh_pi_struct*)__p)->__u = __builtin_shufflevector(__a, __a, 0, 1);
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_store_ss(float *__p, __m128 __a)
{
  struct __mm_store_ss_struct {
    float __u;
  } __attribute__((__packed__, __may_alias__));
  ((struct __mm_store_ss_struct*)__p)->__u = __a[0];
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_storeu_ps(float *__p, __m128 __a)
{
  struct __storeu_ps {
    __m128_u __v;
  } __attribute__((__packed__, __may_alias__));
  ((struct __storeu_ps*)__p)->__v = __a;
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_store_ps(float *__p, __m128 __a)
{
  *(__m128*)__p = __a;
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_store1_ps(float *__p, __m128 __a)
{
  __a = __builtin_shufflevector((__v4sf)__a, (__v4sf)__a, 0, 0, 0, 0);
  _mm_store_ps(__p, __a);
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_store_ps1(float *__p, __m128 __a)
{
  _mm_store1_ps(__p, __a);
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_storer_ps(float *__p, __m128 __a)
{
  __a = __builtin_shufflevector((__v4sf)__a, (__v4sf)__a, 3, 2, 1, 0);
  _mm_store_ps(__p, __a);
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_stream_pi(void *__p, __m64 __a)
{
  __builtin_ia32_movntq((__m64 *)__p, __a);
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_stream_ps(void *__p, __m128 __a)
{
  __builtin_nontemporal_store((__v4sf)__a, (__v4sf*)__p);
}
void _mm_sfence(void);
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_max_pi16(__m64 __a, __m64 __b)
{
  return (__m64)__builtin_ia32_pmaxsw((__v4hi)__a, (__v4hi)__b);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_max_pu8(__m64 __a, __m64 __b)
{
  return (__m64)__builtin_ia32_pmaxub((__v8qi)__a, (__v8qi)__b);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_min_pi16(__m64 __a, __m64 __b)
{
  return (__m64)__builtin_ia32_pminsw((__v4hi)__a, (__v4hi)__b);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_min_pu8(__m64 __a, __m64 __b)
{
  return (__m64)__builtin_ia32_pminub((__v8qi)__a, (__v8qi)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_movemask_pi8(__m64 __a)
{
  return __builtin_ia32_pmovmskb((__v8qi)__a);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_mulhi_pu16(__m64 __a, __m64 __b)
{
  return (__m64)__builtin_ia32_pmulhuw((__v4hi)__a, (__v4hi)__b);
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_maskmove_si64(__m64 __d, __m64 __n, char *__p)
{
  __builtin_ia32_maskmovq((__v8qi)__d, (__v8qi)__n, __p);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_avg_pu8(__m64 __a, __m64 __b)
{
  return (__m64)__builtin_ia32_pavgb((__v8qi)__a, (__v8qi)__b);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_avg_pu16(__m64 __a, __m64 __b)
{
  return (__m64)__builtin_ia32_pavgw((__v4hi)__a, (__v4hi)__b);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_sad_pu8(__m64 __a, __m64 __b)
{
  return (__m64)__builtin_ia32_psadbw((__v8qi)__a, (__v8qi)__b);
}
unsigned int _mm_getcsr(void);
void _mm_setcsr(unsigned int __i);
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_unpackhi_ps(__m128 __a, __m128 __b)
{
  return __builtin_shufflevector((__v4sf)__a, (__v4sf)__b, 2, 6, 3, 7);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_unpacklo_ps(__m128 __a, __m128 __b)
{
  return __builtin_shufflevector((__v4sf)__a, (__v4sf)__b, 0, 4, 1, 5);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_move_ss(__m128 __a, __m128 __b)
{
  __a[0] = __b[0];
  return __a;
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_movehl_ps(__m128 __a, __m128 __b)
{
  return __builtin_shufflevector((__v4sf)__a, (__v4sf)__b, 6, 7, 2, 3);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_movelh_ps(__m128 __a, __m128 __b)
{
  return __builtin_shufflevector((__v4sf)__a, (__v4sf)__b, 0, 1, 4, 5);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_cvtpi16_ps(__m64 __a)
{
  __m64 __b, __c;
  __m128 __r;
  __b = _mm_setzero_si64();
  __b = _mm_cmpgt_pi16(__b, __a);
  __c = _mm_unpackhi_pi16(__a, __b);
  __r = _mm_setzero_ps();
  __r = _mm_cvtpi32_ps(__r, __c);
  __r = _mm_movelh_ps(__r, __r);
  __c = _mm_unpacklo_pi16(__a, __b);
  __r = _mm_cvtpi32_ps(__r, __c);
  return __r;
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_cvtpu16_ps(__m64 __a)
{
  __m64 __b, __c;
  __m128 __r;
  __b = _mm_setzero_si64();
  __c = _mm_unpackhi_pi16(__a, __b);
  __r = _mm_setzero_ps();
  __r = _mm_cvtpi32_ps(__r, __c);
  __r = _mm_movelh_ps(__r, __r);
  __c = _mm_unpacklo_pi16(__a, __b);
  __r = _mm_cvtpi32_ps(__r, __c);
  return __r;
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_cvtpi8_ps(__m64 __a)
{
  __m64 __b;
  __b = _mm_setzero_si64();
  __b = _mm_cmpgt_pi8(__b, __a);
  __b = _mm_unpacklo_pi8(__a, __b);
  return _mm_cvtpi16_ps(__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_cvtpu8_ps(__m64 __a)
{
  __m64 __b;
  __b = _mm_setzero_si64();
  __b = _mm_unpacklo_pi8(__a, __b);
  return _mm_cvtpi16_ps(__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_cvtpi32x2_ps(__m64 __a, __m64 __b)
{
  __m128 __c;
  __c = _mm_setzero_ps();
  __c = _mm_cvtpi32_ps(__c, __b);
  __c = _mm_movelh_ps(__c, __c);
  return _mm_cvtpi32_ps(__c, __a);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_cvtps_pi16(__m128 __a)
{
  __m64 __b, __c;
  __b = _mm_cvtps_pi32(__a);
  __a = _mm_movehl_ps(__a, __a);
  __c = _mm_cvtps_pi32(__a);
  return _mm_packs_pi32(__b, __c);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse,no-evex512"), __min_vector_width__(64)))
_mm_cvtps_pi8(__m128 __a)
{
  __m64 __b, __c;
  __b = _mm_cvtps_pi16(__a);
  __c = _mm_setzero_si64();
  return _mm_packs_pi16(__b, __c);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse,no-evex512"), __min_vector_width__(128)))
_mm_movemask_ps(__m128 __a)
{
  return __builtin_ia32_movmskps((__v4sf)__a);
}

typedef double __m128d __attribute__((__vector_size__(16), __aligned__(16)));
typedef long long __m128i __attribute__((__vector_size__(16), __aligned__(16)));
typedef double __m128d_u __attribute__((__vector_size__(16), __aligned__(1)));
typedef long long __m128i_u
    __attribute__((__vector_size__(16), __aligned__(1)));
typedef double __v2df __attribute__((__vector_size__(16)));
typedef long long __v2di __attribute__((__vector_size__(16)));
typedef short __v8hi __attribute__((__vector_size__(16)));
typedef char __v16qi __attribute__((__vector_size__(16)));
typedef unsigned long long __v2du __attribute__((__vector_size__(16)));
typedef unsigned short __v8hu __attribute__((__vector_size__(16)));
typedef unsigned char __v16qu __attribute__((__vector_size__(16)));
typedef signed char __v16qs __attribute__((__vector_size__(16)));
typedef _Float16 __v8hf __attribute__((__vector_size__(16), __aligned__(16)));
typedef _Float16 __m128h __attribute__((__vector_size__(16), __aligned__(16)));
typedef _Float16 __m128h_u __attribute__((__vector_size__(16), __aligned__(1)));
typedef __bf16 __v8bf __attribute__((__vector_size__(16), __aligned__(16)));
typedef __bf16 __m128bh __attribute__((__vector_size__(16), __aligned__(16)));
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_add_sd(__m128d __a,
                                                        __m128d __b) {
  __a[0] += __b[0];
  return __a;
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_add_pd(__m128d __a,
                                                        __m128d __b) {
  return (__m128d)((__v2df)__a + (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_sub_sd(__m128d __a,
                                                        __m128d __b) {
  __a[0] -= __b[0];
  return __a;
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_sub_pd(__m128d __a,
                                                        __m128d __b) {
  return (__m128d)((__v2df)__a - (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_mul_sd(__m128d __a,
                                                        __m128d __b) {
  __a[0] *= __b[0];
  return __a;
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_mul_pd(__m128d __a,
                                                        __m128d __b) {
  return (__m128d)((__v2df)__a * (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_div_sd(__m128d __a,
                                                        __m128d __b) {
  __a[0] /= __b[0];
  return __a;
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_div_pd(__m128d __a,
                                                        __m128d __b) {
  return (__m128d)((__v2df)__a / (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_sqrt_sd(__m128d __a,
                                                         __m128d __b) {
  __m128d __c = __builtin_ia32_sqrtsd((__v2df)__b);
  return __extension__(__m128d){__c[0], __a[1]};
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_sqrt_pd(__m128d __a) {
  return __builtin_ia32_sqrtpd((__v2df)__a);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_min_sd(__m128d __a,
                                                        __m128d __b) {
  return __builtin_ia32_minsd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_min_pd(__m128d __a,
                                                        __m128d __b) {
  return __builtin_ia32_minpd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_max_sd(__m128d __a,
                                                        __m128d __b) {
  return __builtin_ia32_maxsd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_max_pd(__m128d __a,
                                                        __m128d __b) {
  return __builtin_ia32_maxpd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_and_pd(__m128d __a,
                                                        __m128d __b) {
  return (__m128d)((__v2du)__a & (__v2du)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_andnot_pd(__m128d __a,
                                                           __m128d __b) {
  return (__m128d)(~(__v2du)__a & (__v2du)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_or_pd(__m128d __a,
                                                       __m128d __b) {
  return (__m128d)((__v2du)__a | (__v2du)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_xor_pd(__m128d __a,
                                                        __m128d __b) {
  return (__m128d)((__v2du)__a ^ (__v2du)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpeq_pd(__m128d __a,
                                                          __m128d __b) {
  return (__m128d)__builtin_ia32_cmpeqpd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmplt_pd(__m128d __a,
                                                          __m128d __b) {
  return (__m128d)__builtin_ia32_cmpltpd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmple_pd(__m128d __a,
                                                          __m128d __b) {
  return (__m128d)__builtin_ia32_cmplepd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpgt_pd(__m128d __a,
                                                          __m128d __b) {
  return (__m128d)__builtin_ia32_cmpltpd((__v2df)__b, (__v2df)__a);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpge_pd(__m128d __a,
                                                          __m128d __b) {
  return (__m128d)__builtin_ia32_cmplepd((__v2df)__b, (__v2df)__a);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpord_pd(__m128d __a,
                                                           __m128d __b) {
  return (__m128d)__builtin_ia32_cmpordpd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpunord_pd(__m128d __a,
                                                             __m128d __b) {
  return (__m128d)__builtin_ia32_cmpunordpd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpneq_pd(__m128d __a,
                                                           __m128d __b) {
  return (__m128d)__builtin_ia32_cmpneqpd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpnlt_pd(__m128d __a,
                                                           __m128d __b) {
  return (__m128d)__builtin_ia32_cmpnltpd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpnle_pd(__m128d __a,
                                                           __m128d __b) {
  return (__m128d)__builtin_ia32_cmpnlepd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpngt_pd(__m128d __a,
                                                           __m128d __b) {
  return (__m128d)__builtin_ia32_cmpnltpd((__v2df)__b, (__v2df)__a);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpnge_pd(__m128d __a,
                                                           __m128d __b) {
  return (__m128d)__builtin_ia32_cmpnlepd((__v2df)__b, (__v2df)__a);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpeq_sd(__m128d __a,
                                                          __m128d __b) {
  return (__m128d)__builtin_ia32_cmpeqsd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmplt_sd(__m128d __a,
                                                          __m128d __b) {
  return (__m128d)__builtin_ia32_cmpltsd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmple_sd(__m128d __a,
                                                          __m128d __b) {
  return (__m128d)__builtin_ia32_cmplesd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpgt_sd(__m128d __a,
                                                          __m128d __b) {
  __m128d __c = __builtin_ia32_cmpltsd((__v2df)__b, (__v2df)__a);
  return __extension__(__m128d){__c[0], __a[1]};
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpge_sd(__m128d __a,
                                                          __m128d __b) {
  __m128d __c = __builtin_ia32_cmplesd((__v2df)__b, (__v2df)__a);
  return __extension__(__m128d){__c[0], __a[1]};
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpord_sd(__m128d __a,
                                                           __m128d __b) {
  return (__m128d)__builtin_ia32_cmpordsd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpunord_sd(__m128d __a,
                                                             __m128d __b) {
  return (__m128d)__builtin_ia32_cmpunordsd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpneq_sd(__m128d __a,
                                                           __m128d __b) {
  return (__m128d)__builtin_ia32_cmpneqsd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpnlt_sd(__m128d __a,
                                                           __m128d __b) {
  return (__m128d)__builtin_ia32_cmpnltsd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpnle_sd(__m128d __a,
                                                           __m128d __b) {
  return (__m128d)__builtin_ia32_cmpnlesd((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpngt_sd(__m128d __a,
                                                           __m128d __b) {
  __m128d __c = __builtin_ia32_cmpnltsd((__v2df)__b, (__v2df)__a);
  return __extension__(__m128d){__c[0], __a[1]};
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpnge_sd(__m128d __a,
                                                           __m128d __b) {
  __m128d __c = __builtin_ia32_cmpnlesd((__v2df)__b, (__v2df)__a);
  return __extension__(__m128d){__c[0], __a[1]};
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_comieq_sd(__m128d __a,
                                                       __m128d __b) {
  return __builtin_ia32_comisdeq((__v2df)__a, (__v2df)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_comilt_sd(__m128d __a,
                                                       __m128d __b) {
  return __builtin_ia32_comisdlt((__v2df)__a, (__v2df)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_comile_sd(__m128d __a,
                                                       __m128d __b) {
  return __builtin_ia32_comisdle((__v2df)__a, (__v2df)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_comigt_sd(__m128d __a,
                                                       __m128d __b) {
  return __builtin_ia32_comisdgt((__v2df)__a, (__v2df)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_comige_sd(__m128d __a,
                                                       __m128d __b) {
  return __builtin_ia32_comisdge((__v2df)__a, (__v2df)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_comineq_sd(__m128d __a,
                                                        __m128d __b) {
  return __builtin_ia32_comisdneq((__v2df)__a, (__v2df)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_ucomieq_sd(__m128d __a,
                                                        __m128d __b) {
  return __builtin_ia32_ucomisdeq((__v2df)__a, (__v2df)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_ucomilt_sd(__m128d __a,
                                                        __m128d __b) {
  return __builtin_ia32_ucomisdlt((__v2df)__a, (__v2df)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_ucomile_sd(__m128d __a,
                                                        __m128d __b) {
  return __builtin_ia32_ucomisdle((__v2df)__a, (__v2df)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_ucomigt_sd(__m128d __a,
                                                        __m128d __b) {
  return __builtin_ia32_ucomisdgt((__v2df)__a, (__v2df)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_ucomige_sd(__m128d __a,
                                                        __m128d __b) {
  return __builtin_ia32_ucomisdge((__v2df)__a, (__v2df)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_ucomineq_sd(__m128d __a,
                                                         __m128d __b) {
  return __builtin_ia32_ucomisdneq((__v2df)__a, (__v2df)__b);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtpd_ps(__m128d __a) {
  return __builtin_ia32_cvtpd2ps((__v2df)__a);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtps_pd(__m128 __a) {
  return (__m128d) __builtin_convertvector(
      __builtin_shufflevector((__v4sf)__a, (__v4sf)__a, 0, 1), __v2df);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtepi32_pd(__m128i __a) {
  return (__m128d) __builtin_convertvector(
      __builtin_shufflevector((__v4si)__a, (__v4si)__a, 0, 1), __v2df);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtpd_epi32(__m128d __a) {
  return __builtin_ia32_cvtpd2dq((__v2df)__a);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtsd_si32(__m128d __a) {
  return __builtin_ia32_cvtsd2si((__v2df)__a);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtsd_ss(__m128 __a,
                                                         __m128d __b) {
  return (__m128)__builtin_ia32_cvtsd2ss((__v4sf)__a, (__v2df)__b);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtsi32_sd(__m128d __a,
                                                            int __b) {
  __a[0] = __b;
  return __a;
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtss_sd(__m128d __a,
                                                          __m128 __b) {
  __a[0] = __b[0];
  return __a;
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvttpd_epi32(__m128d __a) {
  return (__m128i)__builtin_ia32_cvttpd2dq((__v2df)__a);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvttsd_si32(__m128d __a) {
  return __builtin_ia32_cvttsd2si((__v2df)__a);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse2,no-evex512"), __min_vector_width__(64))) _mm_cvtpd_pi32(__m128d __a) {
  return (__m64)__builtin_ia32_cvtpd2pi((__v2df)__a);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse2,no-evex512"), __min_vector_width__(64))) _mm_cvttpd_pi32(__m128d __a) {
  return (__m64)__builtin_ia32_cvttpd2pi((__v2df)__a);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse2,no-evex512"), __min_vector_width__(64))) _mm_cvtpi32_pd(__m64 __a) {
  return __builtin_ia32_cvtpi2pd((__v2si)__a);
}
static __inline__ double __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtsd_f64(__m128d __a) {
  return __a[0];
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_load_pd(double const *__dp) {
  return *(const __m128d *)__dp;
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_load1_pd(double const *__dp) {
  struct __mm_load1_pd_struct {
    double __u;
  } __attribute__((__packed__, __may_alias__));
  double __u = ((const struct __mm_load1_pd_struct *)__dp)->__u;
  return __extension__(__m128d){__u, __u};
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_loadr_pd(double const *__dp) {
  __m128d __u = *(const __m128d *)__dp;
  return __builtin_shufflevector((__v2df)__u, (__v2df)__u, 1, 0);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_loadu_pd(double const *__dp) {
  struct __loadu_pd {
    __m128d_u __v;
  } __attribute__((__packed__, __may_alias__));
  return ((const struct __loadu_pd *)__dp)->__v;
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_loadu_si64(void const *__a) {
  struct __loadu_si64 {
    long long __v;
  } __attribute__((__packed__, __may_alias__));
  long long __u = ((const struct __loadu_si64 *)__a)->__v;
  return __extension__(__m128i)(__v2di){__u, 0LL};
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_loadu_si32(void const *__a) {
  struct __loadu_si32 {
    int __v;
  } __attribute__((__packed__, __may_alias__));
  int __u = ((const struct __loadu_si32 *)__a)->__v;
  return __extension__(__m128i)(__v4si){__u, 0, 0, 0};
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_loadu_si16(void const *__a) {
  struct __loadu_si16 {
    short __v;
  } __attribute__((__packed__, __may_alias__));
  short __u = ((const struct __loadu_si16 *)__a)->__v;
  return __extension__(__m128i)(__v8hi){__u, 0, 0, 0, 0, 0, 0, 0};
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_load_sd(double const *__dp) {
  struct __mm_load_sd_struct {
    double __u;
  } __attribute__((__packed__, __may_alias__));
  double __u = ((const struct __mm_load_sd_struct *)__dp)->__u;
  return __extension__(__m128d){__u, 0};
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_loadh_pd(__m128d __a,
                                                          double const *__dp) {
  struct __mm_loadh_pd_struct {
    double __u;
  } __attribute__((__packed__, __may_alias__));
  double __u = ((const struct __mm_loadh_pd_struct *)__dp)->__u;
  return __extension__(__m128d){__a[0], __u};
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_loadl_pd(__m128d __a,
                                                          double const *__dp) {
  struct __mm_loadl_pd_struct {
    double __u;
  } __attribute__((__packed__, __may_alias__));
  double __u = ((const struct __mm_loadl_pd_struct *)__dp)->__u;
  return __extension__(__m128d){__u, __a[1]};
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_undefined_pd(void) {
  return (__m128d)__builtin_ia32_undef128();
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_set_sd(double __w) {
  return __extension__(__m128d){__w, 0.0};
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_set1_pd(double __w) {
  return __extension__(__m128d){__w, __w};
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_set_pd1(double __w) {
  return _mm_set1_pd(__w);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_set_pd(double __w,
                                                        double __x) {
  return __extension__(__m128d){__x, __w};
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_setr_pd(double __w,
                                                         double __x) {
  return __extension__(__m128d){__w, __x};
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_setzero_pd(void) {
  return __extension__(__m128d){0.0, 0.0};
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_move_sd(__m128d __a,
                                                         __m128d __b) {
  __a[0] = __b[0];
  return __a;
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_store_sd(double *__dp,
                                                       __m128d __a) {
  struct __mm_store_sd_struct {
    double __u;
  } __attribute__((__packed__, __may_alias__));
  ((struct __mm_store_sd_struct *)__dp)->__u = __a[0];
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_store_pd(double *__dp,
                                                       __m128d __a) {
  *(__m128d *)__dp = __a;
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_store1_pd(double *__dp,
                                                        __m128d __a) {
  __a = __builtin_shufflevector((__v2df)__a, (__v2df)__a, 0, 0);
  _mm_store_pd(__dp, __a);
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_store_pd1(double *__dp,
                                                        __m128d __a) {
  _mm_store1_pd(__dp, __a);
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_storeu_pd(double *__dp,
                                                        __m128d __a) {
  struct __storeu_pd {
    __m128d_u __v;
  } __attribute__((__packed__, __may_alias__));
  ((struct __storeu_pd *)__dp)->__v = __a;
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_storer_pd(double *__dp,
                                                        __m128d __a) {
  __a = __builtin_shufflevector((__v2df)__a, (__v2df)__a, 1, 0);
  *(__m128d *)__dp = __a;
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_storeh_pd(double *__dp,
                                                        __m128d __a) {
  struct __mm_storeh_pd_struct {
    double __u;
  } __attribute__((__packed__, __may_alias__));
  ((struct __mm_storeh_pd_struct *)__dp)->__u = __a[1];
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_storel_pd(double *__dp,
                                                        __m128d __a) {
  struct __mm_storeh_pd_struct {
    double __u;
  } __attribute__((__packed__, __may_alias__));
  ((struct __mm_storeh_pd_struct *)__dp)->__u = __a[0];
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_add_epi8(__m128i __a,
                                                          __m128i __b) {
  return (__m128i)((__v16qu)__a + (__v16qu)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_add_epi16(__m128i __a,
                                                           __m128i __b) {
  return (__m128i)((__v8hu)__a + (__v8hu)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_add_epi32(__m128i __a,
                                                           __m128i __b) {
  return (__m128i)((__v4su)__a + (__v4su)__b);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse2,no-evex512"), __min_vector_width__(64))) _mm_add_si64(__m64 __a,
                                                            __m64 __b) {
  return (__m64)__builtin_ia32_paddq((__v1di)__a, (__v1di)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_add_epi64(__m128i __a,
                                                           __m128i __b) {
  return (__m128i)((__v2du)__a + (__v2du)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_adds_epi8(__m128i __a,
                                                           __m128i __b) {
  return (__m128i)__builtin_elementwise_add_sat((__v16qs)__a, (__v16qs)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_adds_epi16(__m128i __a,
                                                            __m128i __b) {
  return (__m128i)__builtin_elementwise_add_sat((__v8hi)__a, (__v8hi)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_adds_epu8(__m128i __a,
                                                           __m128i __b) {
  return (__m128i)__builtin_elementwise_add_sat((__v16qu)__a, (__v16qu)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_adds_epu16(__m128i __a,
                                                            __m128i __b) {
  return (__m128i)__builtin_elementwise_add_sat((__v8hu)__a, (__v8hu)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_avg_epu8(__m128i __a,
                                                          __m128i __b) {
  return (__m128i)__builtin_ia32_pavgb128((__v16qi)__a, (__v16qi)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_avg_epu16(__m128i __a,
                                                           __m128i __b) {
  return (__m128i)__builtin_ia32_pavgw128((__v8hi)__a, (__v8hi)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_madd_epi16(__m128i __a,
                                                            __m128i __b) {
  return (__m128i)__builtin_ia32_pmaddwd128((__v8hi)__a, (__v8hi)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_max_epi16(__m128i __a,
                                                           __m128i __b) {
  return (__m128i)__builtin_elementwise_max((__v8hi)__a, (__v8hi)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_max_epu8(__m128i __a,
                                                          __m128i __b) {
  return (__m128i)__builtin_elementwise_max((__v16qu)__a, (__v16qu)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_min_epi16(__m128i __a,
                                                           __m128i __b) {
  return (__m128i)__builtin_elementwise_min((__v8hi)__a, (__v8hi)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_min_epu8(__m128i __a,
                                                          __m128i __b) {
  return (__m128i)__builtin_elementwise_min((__v16qu)__a, (__v16qu)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_mulhi_epi16(__m128i __a,
                                                             __m128i __b) {
  return (__m128i)__builtin_ia32_pmulhw128((__v8hi)__a, (__v8hi)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_mulhi_epu16(__m128i __a,
                                                             __m128i __b) {
  return (__m128i)__builtin_ia32_pmulhuw128((__v8hi)__a, (__v8hi)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_mullo_epi16(__m128i __a,
                                                             __m128i __b) {
  return (__m128i)((__v8hu)__a * (__v8hu)__b);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse2,no-evex512"), __min_vector_width__(64))) _mm_mul_su32(__m64 __a,
                                                            __m64 __b) {
  return __builtin_ia32_pmuludq((__v2si)__a, (__v2si)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_mul_epu32(__m128i __a,
                                                           __m128i __b) {
  return __builtin_ia32_pmuludq128((__v4si)__a, (__v4si)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_sad_epu8(__m128i __a,
                                                          __m128i __b) {
  return __builtin_ia32_psadbw128((__v16qi)__a, (__v16qi)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_sub_epi8(__m128i __a,
                                                          __m128i __b) {
  return (__m128i)((__v16qu)__a - (__v16qu)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_sub_epi16(__m128i __a,
                                                           __m128i __b) {
  return (__m128i)((__v8hu)__a - (__v8hu)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_sub_epi32(__m128i __a,
                                                           __m128i __b) {
  return (__m128i)((__v4su)__a - (__v4su)__b);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("mmx,sse2,no-evex512"), __min_vector_width__(64))) _mm_sub_si64(__m64 __a,
                                                            __m64 __b) {
  return (__m64)__builtin_ia32_psubq((__v1di)__a, (__v1di)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_sub_epi64(__m128i __a,
                                                           __m128i __b) {
  return (__m128i)((__v2du)__a - (__v2du)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_subs_epi8(__m128i __a,
                                                           __m128i __b) {
  return (__m128i)__builtin_elementwise_sub_sat((__v16qs)__a, (__v16qs)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_subs_epi16(__m128i __a,
                                                            __m128i __b) {
  return (__m128i)__builtin_elementwise_sub_sat((__v8hi)__a, (__v8hi)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_subs_epu8(__m128i __a,
                                                           __m128i __b) {
  return (__m128i)__builtin_elementwise_sub_sat((__v16qu)__a, (__v16qu)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_subs_epu16(__m128i __a,
                                                            __m128i __b) {
  return (__m128i)__builtin_elementwise_sub_sat((__v8hu)__a, (__v8hu)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_and_si128(__m128i __a,
                                                           __m128i __b) {
  return (__m128i)((__v2du)__a & (__v2du)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_andnot_si128(__m128i __a,
                                                              __m128i __b) {
  return (__m128i)(~(__v2du)__a & (__v2du)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_or_si128(__m128i __a,
                                                          __m128i __b) {
  return (__m128i)((__v2du)__a | (__v2du)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_xor_si128(__m128i __a,
                                                           __m128i __b) {
  return (__m128i)((__v2du)__a ^ (__v2du)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_slli_epi16(__m128i __a,
                                                            int __count) {
  return (__m128i)__builtin_ia32_psllwi128((__v8hi)__a, __count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_sll_epi16(__m128i __a,
                                                           __m128i __count) {
  return (__m128i)__builtin_ia32_psllw128((__v8hi)__a, (__v8hi)__count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_slli_epi32(__m128i __a,
                                                            int __count) {
  return (__m128i)__builtin_ia32_pslldi128((__v4si)__a, __count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_sll_epi32(__m128i __a,
                                                           __m128i __count) {
  return (__m128i)__builtin_ia32_pslld128((__v4si)__a, (__v4si)__count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_slli_epi64(__m128i __a,
                                                            int __count) {
  return __builtin_ia32_psllqi128((__v2di)__a, __count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_sll_epi64(__m128i __a,
                                                           __m128i __count) {
  return __builtin_ia32_psllq128((__v2di)__a, (__v2di)__count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_srai_epi16(__m128i __a,
                                                            int __count) {
  return (__m128i)__builtin_ia32_psrawi128((__v8hi)__a, __count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_sra_epi16(__m128i __a,
                                                           __m128i __count) {
  return (__m128i)__builtin_ia32_psraw128((__v8hi)__a, (__v8hi)__count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_srai_epi32(__m128i __a,
                                                            int __count) {
  return (__m128i)__builtin_ia32_psradi128((__v4si)__a, __count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_sra_epi32(__m128i __a,
                                                           __m128i __count) {
  return (__m128i)__builtin_ia32_psrad128((__v4si)__a, (__v4si)__count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_srli_epi16(__m128i __a,
                                                            int __count) {
  return (__m128i)__builtin_ia32_psrlwi128((__v8hi)__a, __count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_srl_epi16(__m128i __a,
                                                           __m128i __count) {
  return (__m128i)__builtin_ia32_psrlw128((__v8hi)__a, (__v8hi)__count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_srli_epi32(__m128i __a,
                                                            int __count) {
  return (__m128i)__builtin_ia32_psrldi128((__v4si)__a, __count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_srl_epi32(__m128i __a,
                                                           __m128i __count) {
  return (__m128i)__builtin_ia32_psrld128((__v4si)__a, (__v4si)__count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_srli_epi64(__m128i __a,
                                                            int __count) {
  return __builtin_ia32_psrlqi128((__v2di)__a, __count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_srl_epi64(__m128i __a,
                                                           __m128i __count) {
  return __builtin_ia32_psrlq128((__v2di)__a, (__v2di)__count);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpeq_epi8(__m128i __a,
                                                            __m128i __b) {
  return (__m128i)((__v16qi)__a == (__v16qi)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpeq_epi16(__m128i __a,
                                                             __m128i __b) {
  return (__m128i)((__v8hi)__a == (__v8hi)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpeq_epi32(__m128i __a,
                                                             __m128i __b) {
  return (__m128i)((__v4si)__a == (__v4si)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpgt_epi8(__m128i __a,
                                                            __m128i __b) {
  return (__m128i)((__v16qs)__a > (__v16qs)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpgt_epi16(__m128i __a,
                                                             __m128i __b) {
  return (__m128i)((__v8hi)__a > (__v8hi)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmpgt_epi32(__m128i __a,
                                                             __m128i __b) {
  return (__m128i)((__v4si)__a > (__v4si)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmplt_epi8(__m128i __a,
                                                            __m128i __b) {
  return _mm_cmpgt_epi8(__b, __a);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmplt_epi16(__m128i __a,
                                                             __m128i __b) {
  return _mm_cmpgt_epi16(__b, __a);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cmplt_epi32(__m128i __a,
                                                             __m128i __b) {
  return _mm_cmpgt_epi32(__b, __a);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtsi64_sd(__m128d __a,
                                                            long long __b) {
  __a[0] = __b;
  return __a;
}
static __inline__ long long __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtsd_si64(__m128d __a) {
  return __builtin_ia32_cvtsd2si64((__v2df)__a);
}
static __inline__ long long __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvttsd_si64(__m128d __a) {
  return __builtin_ia32_cvttsd2si64((__v2df)__a);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtepi32_ps(__m128i __a) {
  return (__m128) __builtin_convertvector((__v4si)__a, __v4sf);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtps_epi32(__m128 __a) {
  return (__m128i)__builtin_ia32_cvtps2dq((__v4sf)__a);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvttps_epi32(__m128 __a) {
  return (__m128i)__builtin_ia32_cvttps2dq((__v4sf)__a);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtsi32_si128(int __a) {
  return __extension__(__m128i)(__v4si){__a, 0, 0, 0};
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtsi64_si128(long long __a) {
  return __extension__(__m128i)(__v2di){__a, 0};
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtsi128_si32(__m128i __a) {
  __v4si __b = (__v4si)__a;
  return __b[0];
}
static __inline__ long long __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_cvtsi128_si64(__m128i __a) {
  return __a[0];
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128)))
_mm_load_si128(__m128i const *__p) {
  return *__p;
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128)))
_mm_loadu_si128(__m128i_u const *__p) {
  struct __loadu_si128 {
    __m128i_u __v;
  } __attribute__((__packed__, __may_alias__));
  return ((const struct __loadu_si128 *)__p)->__v;
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128)))
_mm_loadl_epi64(__m128i_u const *__p) {
  struct __mm_loadl_epi64_struct {
    long long __u;
  } __attribute__((__packed__, __may_alias__));
  return __extension__(__m128i){
      ((const struct __mm_loadl_epi64_struct *)__p)->__u, 0};
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_undefined_si128(void) {
  return (__m128i)__builtin_ia32_undef128();
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_set_epi64x(long long __q1,
                                                            long long __q0) {
  return __extension__(__m128i)(__v2di){__q0, __q1};
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_set_epi64(__m64 __q1,
                                                           __m64 __q0) {
  return _mm_set_epi64x((long long)__q1, (long long)__q0);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_set_epi32(int __i3, int __i2,
                                                           int __i1, int __i0) {
  return __extension__(__m128i)(__v4si){__i0, __i1, __i2, __i3};
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128)))
_mm_set_epi16(short __w7, short __w6, short __w5, short __w4, short __w3,
              short __w2, short __w1, short __w0) {
  return __extension__(__m128i)(__v8hi){__w0, __w1, __w2, __w3,
                                        __w4, __w5, __w6, __w7};
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128)))
_mm_set_epi8(char __b15, char __b14, char __b13, char __b12, char __b11,
             char __b10, char __b9, char __b8, char __b7, char __b6, char __b5,
             char __b4, char __b3, char __b2, char __b1, char __b0) {
  return __extension__(__m128i)(__v16qi){
      __b0, __b1, __b2, __b3, __b4, __b5, __b6, __b7,
      __b8, __b9, __b10, __b11, __b12, __b13, __b14, __b15};
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_set1_epi64x(long long __q) {
  return _mm_set_epi64x(__q, __q);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_set1_epi64(__m64 __q) {
  return _mm_set_epi64(__q, __q);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_set1_epi32(int __i) {
  return _mm_set_epi32(__i, __i, __i, __i);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_set1_epi16(short __w) {
  return _mm_set_epi16(__w, __w, __w, __w, __w, __w, __w, __w);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_set1_epi8(char __b) {
  return _mm_set_epi8(__b, __b, __b, __b, __b, __b, __b, __b, __b, __b, __b,
                      __b, __b, __b, __b, __b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_setr_epi64(__m64 __q0,
                                                            __m64 __q1) {
  return _mm_set_epi64(__q1, __q0);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_setr_epi32(int __i0, int __i1,
                                                            int __i2,
                                                            int __i3) {
  return _mm_set_epi32(__i3, __i2, __i1, __i0);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128)))
_mm_setr_epi16(short __w0, short __w1, short __w2, short __w3, short __w4,
               short __w5, short __w6, short __w7) {
  return _mm_set_epi16(__w7, __w6, __w5, __w4, __w3, __w2, __w1, __w0);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128)))
_mm_setr_epi8(char __b0, char __b1, char __b2, char __b3, char __b4, char __b5,
              char __b6, char __b7, char __b8, char __b9, char __b10,
              char __b11, char __b12, char __b13, char __b14, char __b15) {
  return _mm_set_epi8(__b15, __b14, __b13, __b12, __b11, __b10, __b9, __b8,
                      __b7, __b6, __b5, __b4, __b3, __b2, __b1, __b0);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_setzero_si128(void) {
  return __extension__(__m128i)(__v2di){0LL, 0LL};
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_store_si128(__m128i *__p,
                                                          __m128i __b) {
  *__p = __b;
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_storeu_si128(__m128i_u *__p,
                                                           __m128i __b) {
  struct __storeu_si128 {
    __m128i_u __v;
  } __attribute__((__packed__, __may_alias__));
  ((struct __storeu_si128 *)__p)->__v = __b;
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_storeu_si64(void *__p,
                                                          __m128i __b) {
  struct __storeu_si64 {
    long long __v;
  } __attribute__((__packed__, __may_alias__));
  ((struct __storeu_si64 *)__p)->__v = ((__v2di)__b)[0];
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_storeu_si32(void *__p,
                                                          __m128i __b) {
  struct __storeu_si32 {
    int __v;
  } __attribute__((__packed__, __may_alias__));
  ((struct __storeu_si32 *)__p)->__v = ((__v4si)__b)[0];
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_storeu_si16(void *__p,
                                                          __m128i __b) {
  struct __storeu_si16 {
    short __v;
  } __attribute__((__packed__, __may_alias__));
  ((struct __storeu_si16 *)__p)->__v = ((__v8hi)__b)[0];
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_maskmoveu_si128(__m128i __d,
                                                              __m128i __n,
                                                              char *__p) {
  __builtin_ia32_maskmovdqu((__v16qi)__d, (__v16qi)__n, __p);
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_storel_epi64(__m128i_u *__p,
                                                           __m128i __a) {
  struct __mm_storel_epi64_struct {
    long long __u;
  } __attribute__((__packed__, __may_alias__));
  ((struct __mm_storel_epi64_struct *)__p)->__u = __a[0];
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_stream_pd(void *__p,
                                                        __m128d __a) {
  __builtin_nontemporal_store((__v2df)__a, (__v2df *)__p);
}
static __inline__ void __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_stream_si128(void *__p,
                                                           __m128i __a) {
  __builtin_nontemporal_store((__v2di)__a, (__v2di *)__p);
}
static __inline__ void
    __attribute__((__always_inline__, __nodebug__, __target__("sse2")))
    _mm_stream_si32(void *__p, int __a) {
  __builtin_ia32_movnti((int *)__p, __a);
}
static __inline__ void
    __attribute__((__always_inline__, __nodebug__, __target__("sse2")))
    _mm_stream_si64(void *__p, long long __a) {
  __builtin_ia32_movnti64((long long *)__p, __a);
}
void _mm_clflush(void const *__p);
void _mm_lfence(void);
void _mm_mfence(void);
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_packs_epi16(__m128i __a,
                                                             __m128i __b) {
  return (__m128i)__builtin_ia32_packsswb128((__v8hi)__a, (__v8hi)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_packs_epi32(__m128i __a,
                                                             __m128i __b) {
  return (__m128i)__builtin_ia32_packssdw128((__v4si)__a, (__v4si)__b);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_packus_epi16(__m128i __a,
                                                              __m128i __b) {
  return (__m128i)__builtin_ia32_packuswb128((__v8hi)__a, (__v8hi)__b);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_movemask_epi8(__m128i __a) {
  return __builtin_ia32_pmovmskb128((__v16qi)__a);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_unpackhi_epi8(__m128i __a,
                                                               __m128i __b) {
  return (__m128i)__builtin_shufflevector(
      (__v16qi)__a, (__v16qi)__b, 8, 16 + 8, 9, 16 + 9, 10, 16 + 10, 11,
      16 + 11, 12, 16 + 12, 13, 16 + 13, 14, 16 + 14, 15, 16 + 15);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_unpackhi_epi16(__m128i __a,
                                                                __m128i __b) {
  return (__m128i)__builtin_shufflevector((__v8hi)__a, (__v8hi)__b, 4, 8 + 4, 5,
                                          8 + 5, 6, 8 + 6, 7, 8 + 7);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_unpackhi_epi32(__m128i __a,
                                                                __m128i __b) {
  return (__m128i)__builtin_shufflevector((__v4si)__a, (__v4si)__b, 2, 4 + 2, 3,
                                          4 + 3);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_unpackhi_epi64(__m128i __a,
                                                                __m128i __b) {
  return (__m128i)__builtin_shufflevector((__v2di)__a, (__v2di)__b, 1, 2 + 1);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_unpacklo_epi8(__m128i __a,
                                                               __m128i __b) {
  return (__m128i)__builtin_shufflevector(
      (__v16qi)__a, (__v16qi)__b, 0, 16 + 0, 1, 16 + 1, 2, 16 + 2, 3, 16 + 3, 4,
      16 + 4, 5, 16 + 5, 6, 16 + 6, 7, 16 + 7);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_unpacklo_epi16(__m128i __a,
                                                                __m128i __b) {
  return (__m128i)__builtin_shufflevector((__v8hi)__a, (__v8hi)__b, 0, 8 + 0, 1,
                                          8 + 1, 2, 8 + 2, 3, 8 + 3);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_unpacklo_epi32(__m128i __a,
                                                                __m128i __b) {
  return (__m128i)__builtin_shufflevector((__v4si)__a, (__v4si)__b, 0, 4 + 0, 1,
                                          4 + 1);
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_unpacklo_epi64(__m128i __a,
                                                                __m128i __b) {
  return (__m128i)__builtin_shufflevector((__v2di)__a, (__v2di)__b, 0, 2 + 0);
}
static __inline__ __m64 __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_movepi64_pi64(__m128i __a) {
  return (__m64)__a[0];
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_movpi64_epi64(__m64 __a) {
  return __extension__(__m128i)(__v2di){(long long)__a, 0};
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_move_epi64(__m128i __a) {
  return __builtin_shufflevector((__v2di)__a, _mm_setzero_si128(), 0, 2);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_unpackhi_pd(__m128d __a,
                                                             __m128d __b) {
  return __builtin_shufflevector((__v2df)__a, (__v2df)__b, 1, 2 + 1);
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_unpacklo_pd(__m128d __a,
                                                             __m128d __b) {
  return __builtin_shufflevector((__v2df)__a, (__v2df)__b, 0, 2 + 0);
}
static __inline__ int __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_movemask_pd(__m128d __a) {
  return __builtin_ia32_movmskpd((__v2df)__a);
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_castpd_ps(__m128d __a) {
  return (__m128)__a;
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_castpd_si128(__m128d __a) {
  return (__m128i)__a;
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_castps_pd(__m128 __a) {
  return (__m128d)__a;
}
static __inline__ __m128i __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_castps_si128(__m128 __a) {
  return (__m128i)__a;
}
static __inline__ __m128 __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_castsi128_ps(__m128i __a) {
  return (__m128)__a;
}
static __inline__ __m128d __attribute__((__always_inline__, __nodebug__, __target__("sse2,no-evex512"), __min_vector_width__(128))) _mm_castsi128_pd(__m128i __a) {
  return (__m128d)__a;
}
void _mm_pause(void);
_Alignas(64) static const xxh_u8 XXH3_kSecret[192] = {
    0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81, 0x2c, 0xf7, 0x21, 0xad, 0x1c,
    0xde, 0xd4, 0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb, 0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f,
    0xcb, 0x79, 0xe6, 0x4e, 0xcc, 0xc0, 0xe5, 0x78, 0x82, 0x5a, 0xd0, 0x7d, 0xcc, 0xff, 0x72, 0x21,
    0xb8, 0x08, 0x46, 0x74, 0xf7, 0x43, 0x24, 0x8e, 0xe0, 0x35, 0x90, 0xe6, 0x81, 0x3a, 0x26, 0x4c,
    0x3c, 0x28, 0x52, 0xbb, 0x91, 0xc3, 0x00, 0xcb, 0x88, 0xd0, 0x65, 0x8b, 0x1b, 0x53, 0x2e, 0xa3,
    0x71, 0x64, 0x48, 0x97, 0xa2, 0x0d, 0xf9, 0x4e, 0x38, 0x19, 0xef, 0x46, 0xa9, 0xde, 0xac, 0xd8,
    0xa8, 0xfa, 0x76, 0x3f, 0xe3, 0x9c, 0x34, 0x3f, 0xf9, 0xdc, 0xbb, 0xc7, 0xc7, 0x0b, 0x4f, 0x1d,
    0x8a, 0x51, 0xe0, 0x4b, 0xcd, 0xb4, 0x59, 0x31, 0xc8, 0x9f, 0x7e, 0xc9, 0xd9, 0x78, 0x73, 0x64,
    0xea, 0xc5, 0xac, 0x83, 0x34, 0xd3, 0xeb, 0xc3, 0xc5, 0x81, 0xa0, 0xff, 0xfa, 0x13, 0x63, 0xeb,
    0x17, 0x0d, 0xdd, 0x51, 0xb7, 0xf0, 0xda, 0x49, 0xd3, 0x16, 0x55, 0x26, 0x29, 0xd4, 0x68, 0x9e,
    0x2b, 0x16, 0xbe, 0x58, 0x7d, 0x47, 0xa1, 0xfc, 0x8f, 0xf8, 0xb8, 0xd1, 0x7a, 0xd0, 0x31, 0xce,
    0x45, 0xcb, 0x3a, 0x8f, 0x95, 0x16, 0x04, 0x28, 0xaf, 0xd7, 0xfb, 0xca, 0xbb, 0x4b, 0x40, 0x7e,
};
static const xxh_u64 PRIME_MX1 = 0x165667919E3779F9ULL;
static const xxh_u64 PRIME_MX2 = 0x9FB21C651E98DF25ULL;
static XXH_NAMESPACEXXH128_hash_t
XXH_mult64to128(xxh_u64 lhs, xxh_u64 rhs)
{
    __uint128_t const product = (__uint128_t)lhs * (__uint128_t)rhs;
    XXH_NAMESPACEXXH128_hash_t r128;
    r128.low64 = (xxh_u64)(product);
    r128.high64 = (xxh_u64)(product >> 64);
    return r128;
}
static xxh_u64
XXH3_mul128_fold64(xxh_u64 lhs, xxh_u64 rhs)
{
    XXH_NAMESPACEXXH128_hash_t product = XXH_mult64to128(lhs, rhs);
    return product.low64 ^ product.high64;
}
static __attribute__((unused)) __attribute__((const)) xxh_u64 XXH_xorshift64(xxh_u64 v64, int shift)
{
    __builtin_assume(0 <= shift && shift < 64);
    return v64 ^ (v64 >> shift);
}
static XXH64_hash_t XXH3_avalanche(xxh_u64 h64)
{
    h64 = XXH_xorshift64(h64, 37);
    h64 *= PRIME_MX1;
    h64 = XXH_xorshift64(h64, 32);
    return h64;
}
static XXH64_hash_t XXH3_rrmxmx(xxh_u64 h64, xxh_u64 len)
{
    h64 ^= __builtin_rotateleft64(h64, 49) ^ __builtin_rotateleft64(h64, 24);
    h64 *= PRIME_MX2;
    h64 ^= (h64 >> 35) + len ;
    h64 *= PRIME_MX2;
    return XXH_xorshift64(h64, 28);
}
static __attribute__((unused)) __attribute__((pure)) XXH64_hash_t
XXH3_len_1to3_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    __builtin_assume(input != ((void*)0));
    __builtin_assume(1 <= len && len <= 3);
    __builtin_assume(secret != ((void*)0));
    { xxh_u8 const c1 = input[0];
        xxh_u8 const c2 = input[len >> 1];
        xxh_u8 const c3 = input[len - 1];
        xxh_u32 const combined = ((xxh_u32)c1 << 16) | ((xxh_u32)c2 << 24)
                               | ((xxh_u32)c3 << 0) | ((xxh_u32)len << 8);
        xxh_u64 const bitflip = (XXH_readLE32(secret) ^ XXH_readLE32(secret+4)) + seed;
        xxh_u64 const keyed = (xxh_u64)combined ^ bitflip;
        return XXH64_avalanche(keyed);
    }
}
static __attribute__((unused)) __attribute__((pure)) XXH64_hash_t
XXH3_len_4to8_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    __builtin_assume(input != ((void*)0));
    __builtin_assume(secret != ((void*)0));
    __builtin_assume(4 <= len && len <= 8);
    seed ^= (xxh_u64)XXH_swap32((xxh_u32)seed) << 32;
    { xxh_u32 const input1 = XXH_readLE32(input);
        xxh_u32 const input2 = XXH_readLE32(input + len - 4);
        xxh_u64 const bitflip = (XXH_readLE64(secret+8) ^ XXH_readLE64(secret+16)) - seed;
        xxh_u64 const input64 = input2 + (((xxh_u64)input1) << 32);
        xxh_u64 const keyed = input64 ^ bitflip;
        return XXH3_rrmxmx(keyed, len);
    }
}
static __attribute__((unused)) __attribute__((pure)) XXH64_hash_t
XXH3_len_9to16_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    __builtin_assume(input != ((void*)0));
    __builtin_assume(secret != ((void*)0));
    __builtin_assume(9 <= len && len <= 16);
    { xxh_u64 const bitflip1 = (XXH_readLE64(secret+24) ^ XXH_readLE64(secret+32)) + seed;
        xxh_u64 const bitflip2 = (XXH_readLE64(secret+40) ^ XXH_readLE64(secret+48)) - seed;
        xxh_u64 const input_lo = XXH_readLE64(input) ^ bitflip1;
        xxh_u64 const input_hi = XXH_readLE64(input + len - 8) ^ bitflip2;
        xxh_u64 const acc = len
                          + XXH_swap64(input_lo) + input_hi
                          + XXH3_mul128_fold64(input_lo, input_hi);
        return XXH3_avalanche(acc);
    }
}
static __attribute__((unused)) __attribute__((pure)) XXH64_hash_t
XXH3_len_0to16_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    __builtin_assume(len <= 16);
    { if (__builtin_expect(len > 8, 1)) return XXH3_len_9to16_64b(input, len, secret, seed);
        if (__builtin_expect(len >= 4, 1)) return XXH3_len_4to8_64b(input, len, secret, seed);
        if (len) return XXH3_len_1to3_64b(input, len, secret, seed);
        return XXH64_avalanche(seed ^ (XXH_readLE64(secret+56) ^ XXH_readLE64(secret+64)));
    }
}
static __attribute__((unused)) xxh_u64 XXH3_mix16B(const xxh_u8* restrict input,
                                     const xxh_u8* restrict secret, xxh_u64 seed64)
{
    { xxh_u64 const input_lo = XXH_readLE64(input);
        xxh_u64 const input_hi = XXH_readLE64(input+8);
        return XXH3_mul128_fold64(
            input_lo ^ (XXH_readLE64(secret) + seed64),
            input_hi ^ (XXH_readLE64(secret+8) - seed64)
        );
    }
}
static __attribute__((unused)) __attribute__((pure)) XXH64_hash_t
XXH3_len_17to128_64b(const xxh_u8* restrict input, size_t len,
                     const xxh_u8* restrict secret, size_t secretSize,
                     XXH64_hash_t seed)
{
    __builtin_assume(secretSize >= 136); (void)secretSize;
    __builtin_assume(16 < len && len <= 128);
    { xxh_u64 acc = len * 0x9E3779B185EBCA87ULL;
        if (len > 32) {
            if (len > 64) {
                if (len > 96) {
                    acc += XXH3_mix16B(input+48, secret+96, seed);
                    acc += XXH3_mix16B(input+len-64, secret+112, seed);
                }
                acc += XXH3_mix16B(input+32, secret+64, seed);
                acc += XXH3_mix16B(input+len-48, secret+80, seed);
            }
            acc += XXH3_mix16B(input+16, secret+32, seed);
            acc += XXH3_mix16B(input+len-32, secret+48, seed);
        }
        acc += XXH3_mix16B(input+0, secret+0, seed);
        acc += XXH3_mix16B(input+len-16, secret+16, seed);
        return XXH3_avalanche(acc);
    }
}
static __attribute__((pure)) XXH64_hash_t
XXH3_len_129to240_64b(const xxh_u8* restrict input, size_t len,
                      const xxh_u8* restrict secret, size_t secretSize,
                      XXH64_hash_t seed)
{
    __builtin_assume(secretSize >= 136); (void)secretSize;
    __builtin_assume(128 < len && len <= 240);
    { xxh_u64 acc = len * 0x9E3779B185EBCA87ULL;
        xxh_u64 acc_end;
        unsigned int const nbRounds = (unsigned int)len / 16;
        unsigned int i;
        __builtin_assume(128 < len && len <= 240);
        for (i=0; i<8; i++) {
            acc += XXH3_mix16B(input+(16*i), secret+(16*i), seed);
        }
        acc_end = XXH3_mix16B(input + len - 16, secret + 136 - 17, seed);
        __builtin_assume(nbRounds >= 8);
        acc = XXH3_avalanche(acc);
        for (i=8 ; i < nbRounds; i++) {
            __asm__("" : "+r" (acc));
            acc_end += XXH3_mix16B(input+(16*i), secret+(16*(i-8)) + 3, seed);
        }
        return XXH3_avalanche(acc + acc_end);
    }
}
static __attribute__((unused)) void XXH_writeLE64(void* dst, xxh_u64 v64)
{
    if (!1) v64 = XXH_swap64(v64);
    XXH_memcpy(dst, &v64, sizeof(v64));
}
    typedef int64_t xxh_i64;
static __attribute__((unused)) void
XXH3_accumulate_512_sse2( void* restrict acc,
                    const void* restrict input,
                    const void* restrict secret)
{
    __builtin_assume((((size_t)acc) & 15) == 0);
    { __m128i* const xacc = (__m128i *) acc;
        const __m128i* const xinput = (const __m128i *) input;
        const __m128i* const xsecret = (const __m128i *) secret;
        size_t i;
        for (i=0; i < 64/sizeof(__m128i); i++) {
            __m128i const data_vec = _mm_loadu_si128 (xinput+i);
            __m128i const key_vec = _mm_loadu_si128 (xsecret+i);
            __m128i const data_key = _mm_xor_si128 (data_vec, key_vec);
            __m128i const data_key_lo = ((__m128i)__builtin_ia32_pshufd((__v4si)(__m128i)(data_key), (int)((((0) << 6) | ((3) << 4) | ((0) << 2) | (1)))));
            __m128i const product = _mm_mul_epu32 (data_key, data_key_lo);
            __m128i const data_swap = ((__m128i)__builtin_ia32_pshufd((__v4si)(__m128i)(data_vec), (int)((((1) << 6) | ((0) << 4) | ((3) << 2) | (2)))));
            __m128i const sum = _mm_add_epi64(xacc[i], data_swap);
            xacc[i] = _mm_add_epi64(product, sum);
    } }
}
static __attribute__((unused)) void XXH3_accumulate_sse2(xxh_u64* restrict acc, const xxh_u8* restrict input, const xxh_u8* restrict secret, size_t nbStripes) { size_t n; for (n = 0; n < nbStripes; n++ ) { const xxh_u8* const in = input + n*64; __builtin_prefetch((in + 320), 0 , 3 ); XXH3_accumulate_512_sse2( acc, in, secret + n*8); } }
static __attribute__((unused)) void
XXH3_scrambleAcc_sse2(void* restrict acc, const void* restrict secret)
{
    __builtin_assume((((size_t)acc) & 15) == 0);
    { __m128i* const xacc = (__m128i*) acc;
        const __m128i* const xsecret = (const __m128i *) secret;
        const __m128i prime32 = _mm_set1_epi32((int)0x9E3779B1U);
        size_t i;
        for (i=0; i < 64/sizeof(__m128i); i++) {
            __m128i const acc_vec = xacc[i];
            __m128i const shifted = _mm_srli_epi64 (acc_vec, 47);
            __m128i const data_vec = _mm_xor_si128 (acc_vec, shifted);
            __m128i const key_vec = _mm_loadu_si128 (xsecret+i);
            __m128i const data_key = _mm_xor_si128 (data_vec, key_vec);
            __m128i const data_key_hi = ((__m128i)__builtin_ia32_pshufd((__v4si)(__m128i)(data_key), (int)((((0) << 6) | ((3) << 4) | ((0) << 2) | (1)))));
            __m128i const prod_lo = _mm_mul_epu32 (data_key, prime32);
            __m128i const prod_hi = _mm_mul_epu32 (data_key_hi, prime32);
            xacc[i] = _mm_add_epi64(prod_lo, _mm_slli_epi64(prod_hi, 32));
        }
    }
}
static __attribute__((unused)) void XXH3_initCustomSecret_sse2(void* restrict customSecret, xxh_u64 seed64)
{
    do { _Static_assert((((192 & 15) == 0)),"(XXH_SECRET_DEFAULT_SIZE & 15) == 0"); } while(0);
    (void)(&XXH_writeLE64);
    { int const nbRounds = 192 / sizeof(__m128i);
        __m128i const seed = _mm_set_epi64x((xxh_i64)(0U - seed64), (xxh_i64)seed64);
        int i;
        const void* const src16 = XXH3_kSecret;
        __m128i* dst16 = (__m128i*) customSecret;
        __asm__("" : "+r" (dst16));
        __builtin_assume(((size_t)src16 & 15) == 0);
        __builtin_assume(((size_t)dst16 & 15) == 0);
        for (i=0; i < nbRounds; ++i) {
            dst16[i] = _mm_add_epi64(_mm_load_si128((const __m128i *)src16+i), seed);
    } }
}
static __attribute__((unused)) xxh_u64
XXH_mult32to64_add64(xxh_u64 lhs, xxh_u64 rhs, xxh_u64 acc)
{
    return ((xxh_u64)(xxh_u32)((xxh_u32)lhs) * (xxh_u64)(xxh_u32)((xxh_u32)rhs)) + acc;
}
static __attribute__((unused)) void
XXH3_scalarRound(void* restrict acc,
                 void const* restrict input,
                 void const* restrict secret,
                 size_t lane)
{
    xxh_u64* xacc = (xxh_u64*) acc;
    xxh_u8 const* xinput = (xxh_u8 const*) input;
    xxh_u8 const* xsecret = (xxh_u8 const*) secret;
    __builtin_assume(lane < (64 / sizeof(xxh_u64)));
    __builtin_assume(((size_t)acc & (16 -1)) == 0);
    {
        xxh_u64 const data_val = XXH_readLE64(xinput + lane * 8);
        xxh_u64 const data_key = data_val ^ XXH_readLE64(xsecret + lane * 8);
        xacc[lane ^ 1] += data_val;
        xacc[lane] = XXH_mult32to64_add64(data_key , data_key >> 32, xacc[lane]);
    }
}
static __attribute__((unused)) void
XXH3_accumulate_512_scalar(void* restrict acc,
                     const void* restrict input,
                     const void* restrict secret)
{
    size_t i;
    for (i=0; i < (64 / sizeof(xxh_u64)); i++) {
        XXH3_scalarRound(acc, input, secret, i);
    }
}
static __attribute__((unused)) void XXH3_accumulate_scalar(xxh_u64* restrict acc, const xxh_u8* restrict input, const xxh_u8* restrict secret, size_t nbStripes) { size_t n; for (n = 0; n < nbStripes; n++ ) { const xxh_u8* const in = input + n*64; __builtin_prefetch((in + 320), 0 , 3 ); XXH3_accumulate_512_scalar( acc, in, secret + n*8); } }
static __attribute__((unused)) void
XXH3_scalarScrambleRound(void* restrict acc,
                         void const* restrict secret,
                         size_t lane)
{
    xxh_u64* const xacc = (xxh_u64*) acc;
    const xxh_u8* const xsecret = (const xxh_u8*) secret;
    __builtin_assume((((size_t)acc) & (16 -1)) == 0);
    __builtin_assume(lane < (64 / sizeof(xxh_u64)));
    {
        xxh_u64 const key64 = XXH_readLE64(xsecret + lane * 8);
        xxh_u64 acc64 = xacc[lane];
        acc64 = XXH_xorshift64(acc64, 47);
        acc64 ^= key64;
        acc64 *= 0x9E3779B1U;
        xacc[lane] = acc64;
    }
}
static __attribute__((unused)) void
XXH3_scrambleAcc_scalar(void* restrict acc, const void* restrict secret)
{
    size_t i;
    for (i=0; i < (64 / sizeof(xxh_u64)); i++) {
        XXH3_scalarScrambleRound(acc, secret, i);
    }
}
static __attribute__((unused)) void
XXH3_initCustomSecret_scalar(void* restrict customSecret, xxh_u64 seed64)
{
    const xxh_u8* kSecretPtr = XXH3_kSecret;
    do { _Static_assert((((192 & 15) == 0)),"(XXH_SECRET_DEFAULT_SIZE & 15) == 0"); } while(0);
    { int const nbRounds = 192 / 16;
        int i;
        for (i=0; i < nbRounds; i++) {
            xxh_u64 lo = XXH_readLE64(kSecretPtr + 16*i) + seed64;
            xxh_u64 hi = XXH_readLE64(kSecretPtr + 16*i + 8) - seed64;
            XXH_writeLE64((xxh_u8*)customSecret + 16*i, lo);
            XXH_writeLE64((xxh_u8*)customSecret + 16*i + 8, hi);
    } }
}
typedef void (*XXH3_f_accumulate)(xxh_u64* restrict, const xxh_u8* restrict, const xxh_u8* restrict, size_t);
typedef void (*XXH3_f_scrambleAcc)(void* restrict, const void*);
typedef void (*XXH3_f_initCustomSecret)(void* restrict, xxh_u64);
static __attribute__((unused)) void
XXH3_hashLong_internal_loop(xxh_u64* restrict acc,
                      const xxh_u8* restrict input, size_t len,
                      const xxh_u8* restrict secret, size_t secretSize,
                            XXH3_f_accumulate f_acc,
                            XXH3_f_scrambleAcc f_scramble)
{
    size_t const nbStripesPerBlock = (secretSize - 64) / 8;
    size_t const block_len = 64 * nbStripesPerBlock;
    size_t const nb_blocks = (len - 1) / block_len;
    size_t n;
    __builtin_assume(secretSize >= 136);
    for (n = 0; n < nb_blocks; n++) {
        f_acc(acc, input + n*block_len, secret, nbStripesPerBlock);
        f_scramble(acc, secret + secretSize - 64);
    }
    __builtin_assume(len > 64);
    { size_t const nbStripes = ((len - 1) - (block_len * nb_blocks)) / 64;
        __builtin_assume(nbStripes <= (secretSize / 8));
        f_acc(acc, input + nb_blocks*block_len, secret, nbStripes);
        { const xxh_u8* const p = input + len - 64;
            XXH3_accumulate_512_sse2(acc, p, secret + secretSize - 64 - 7);
    } }
}
static __attribute__((unused)) xxh_u64
XXH3_mix2Accs(const xxh_u64* restrict acc, const xxh_u8* restrict secret)
{
    return XXH3_mul128_fold64(
               acc[0] ^ XXH_readLE64(secret),
               acc[1] ^ XXH_readLE64(secret+8) );
}
static XXH64_hash_t
XXH3_mergeAccs(const xxh_u64* restrict acc, const xxh_u8* restrict secret, xxh_u64 start)
{
    xxh_u64 result64 = start;
    size_t i = 0;
    for (i = 0; i < 4; i++) {
        result64 += XXH3_mix2Accs(acc+2*i, secret + 16*i);
    }
    return XXH3_avalanche(result64);
}
static __attribute__((unused)) XXH64_hash_t
XXH3_hashLong_64b_internal(const void* restrict input, size_t len,
                           const void* restrict secret, size_t secretSize,
                           XXH3_f_accumulate f_acc,
                           XXH3_f_scrambleAcc f_scramble)
{
    _Alignas(16) xxh_u64 acc[(64 / sizeof(xxh_u64))] = { 0xC2B2AE3DU, 0x9E3779B185EBCA87ULL, 0xC2B2AE3D27D4EB4FULL, 0x165667B19E3779F9ULL, 0x85EBCA77C2B2AE63ULL, 0x85EBCA77U, 0x27D4EB2F165667C5ULL, 0x9E3779B1U };
    XXH3_hashLong_internal_loop(acc, (const xxh_u8*)input, len, (const xxh_u8*)secret, secretSize, f_acc, f_scramble);
    do { _Static_assert(((sizeof(acc) == 64)),"sizeof(acc) == 64"); } while(0);
    __builtin_assume(secretSize >= sizeof(acc) + 11);
    return XXH3_mergeAccs(acc, (const xxh_u8*)secret + 11, (xxh_u64)len * 0x9E3779B185EBCA87ULL);
}
static __attribute__((unused)) XXH64_hash_t
XXH3_hashLong_64b_withSecret(const void* restrict input, size_t len,
                             XXH64_hash_t seed64, const xxh_u8* restrict secret, size_t secretLen)
{
    (void)seed64;
    return XXH3_hashLong_64b_internal(input, len, secret, secretLen, XXH3_accumulate_sse2, XXH3_scrambleAcc_sse2);
}
static __attribute__((pure)) XXH64_hash_t
XXH3_hashLong_64b_default(const void* restrict input, size_t len,
                          XXH64_hash_t seed64, const xxh_u8* restrict secret, size_t secretLen)
{
    (void)seed64; (void)secret; (void)secretLen;
    return XXH3_hashLong_64b_internal(input, len, XXH3_kSecret, sizeof(XXH3_kSecret), XXH3_accumulate_sse2, XXH3_scrambleAcc_sse2);
}
static __attribute__((unused)) XXH64_hash_t
XXH3_hashLong_64b_withSeed_internal(const void* input, size_t len,
                                    XXH64_hash_t seed,
                                    XXH3_f_accumulate f_acc,
                                    XXH3_f_scrambleAcc f_scramble,
                                    XXH3_f_initCustomSecret f_initSec)
{
    if (seed == 0)
        return XXH3_hashLong_64b_internal(input, len,
                                          XXH3_kSecret, sizeof(XXH3_kSecret),
                                          f_acc, f_scramble);
    { _Alignas(16) xxh_u8 secret[192];
        f_initSec(secret, seed);
        return XXH3_hashLong_64b_internal(input, len, secret, sizeof(secret),
                                          f_acc, f_scramble);
    }
}
static XXH64_hash_t
XXH3_hashLong_64b_withSeed(const void* restrict input, size_t len,
                           XXH64_hash_t seed, const xxh_u8* restrict secret, size_t secretLen)
{
    (void)secret; (void)secretLen;
    return XXH3_hashLong_64b_withSeed_internal(input, len, seed,
                XXH3_accumulate_sse2, XXH3_scrambleAcc_sse2, XXH3_initCustomSecret_sse2);
}
typedef XXH64_hash_t (*XXH3_hashLong64_f)(const void* restrict, size_t,
                                          XXH64_hash_t, const xxh_u8* restrict, size_t);
static __attribute__((unused)) XXH64_hash_t
XXH3_64bits_internal(const void* restrict input, size_t len,
                     XXH64_hash_t seed64, const void* restrict secret, size_t secretLen,
                     XXH3_hashLong64_f f_hashLong)
{
    __builtin_assume(secretLen >= 136);
    if (len <= 16)
        return XXH3_len_0to16_64b((const xxh_u8*)input, len, (const xxh_u8*)secret, seed64);
    if (len <= 128)
        return XXH3_len_17to128_64b((const xxh_u8*)input, len, (const xxh_u8*)secret, secretLen, seed64);
    if (len <= 240)
        return XXH3_len_129to240_64b((const xxh_u8*)input, len, (const xxh_u8*)secret, secretLen, seed64);
    return f_hashLong(input, len, seed64, (const xxh_u8*)secret, secretLen);
}
static __inline __attribute__((unused)) XXH64_hash_t XXH_INLINE_XXH3_64bits(__attribute__((noescape)) const void* input, size_t length)
{
    return XXH3_64bits_internal(input, length, 0, XXH3_kSecret, sizeof(XXH3_kSecret), XXH3_hashLong_64b_default);
}
static __inline __attribute__((unused)) XXH64_hash_t
XXH_INLINE_XXH3_64bits_withSecret(__attribute__((noescape)) const void* input, size_t length, __attribute__((noescape)) const void* secret, size_t secretSize)
{
    return XXH3_64bits_internal(input, length, 0, secret, secretSize, XXH3_hashLong_64b_withSecret);
}
static __inline __attribute__((unused)) XXH64_hash_t
XXH_INLINE_XXH3_64bits_withSeed(__attribute__((noescape)) const void* input, size_t length, XXH64_hash_t seed)
{
    return XXH3_64bits_internal(input, length, seed, XXH3_kSecret, sizeof(XXH3_kSecret), XXH3_hashLong_64b_withSeed);
}
static __inline __attribute__((unused)) XXH64_hash_t
XXH_INLINE_XXH3_64bits_withSecretandSeed(__attribute__((noescape)) const void* input, size_t length, __attribute__((noescape)) const void* secret, size_t secretSize, XXH64_hash_t seed)
{
    if (length <= 240)
        return XXH3_64bits_internal(input, length, seed, XXH3_kSecret, sizeof(XXH3_kSecret), ((void*)0));
    return XXH3_hashLong_64b_withSecret(input, length, seed, (const xxh_u8*)secret, secretSize);
}
static __attribute__((malloc)) void* XXH_alignedMalloc(size_t s, size_t align)
{
    __builtin_assume(align <= 128 && align >= 8);
    __builtin_assume((align & (align-1)) == 0);
    __builtin_assume(s != 0 && s < (s + align));
    {
        xxh_u8* base = (xxh_u8*)XXH_malloc(s + align);
        if (base != ((void*)0)) {
            size_t offset = align - ((size_t)base & (align - 1));
            xxh_u8* ptr = base + offset;
            __builtin_assume((size_t)ptr % align == 0);
            ptr[-1] = (xxh_u8)offset;
            return ptr;
        }
        return ((void*)0);
    }
}
static void XXH_alignedFree(void* p)
{
    if (p != ((void*)0)) {
        xxh_u8* ptr = (xxh_u8*)p;
        xxh_u8 offset = ptr[-1];
        xxh_u8* base = ptr - offset;
        XXH_free(base);
    }
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH3_state_t* XXH_INLINE_XXH3_createState(void)
{
    XXH_NAMESPACEXXH3_state_t* const state = (XXH_NAMESPACEXXH3_state_t*)XXH_alignedMalloc(sizeof(XXH_NAMESPACEXXH3_state_t), 64);
    if (state==((void*)0)) return ((void*)0);
    do { XXH_NAMESPACEXXH3_state_t* tmp_xxh3_state_ptr = (state); tmp_xxh3_state_ptr->seed = 0; tmp_xxh3_state_ptr->extSecret = ((void*)0); } while(0);
    return state;
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode XXH_INLINE_XXH3_freeState(XXH_NAMESPACEXXH3_state_t* statePtr)
{
    XXH_alignedFree(statePtr);
    return XXH_NAMESPACEXXH_OK;
}
static __inline __attribute__((unused)) void
XXH_INLINE_XXH3_copyState(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* dst_state, __attribute__((noescape)) const XXH_NAMESPACEXXH3_state_t* src_state)
{
    XXH_memcpy(dst_state, src_state, sizeof(*dst_state));
}
static void
XXH3_reset_internal(XXH_NAMESPACEXXH3_state_t* statePtr,
                    XXH64_hash_t seed,
                    const void* secret, size_t secretSize)
{
    size_t const initStart = __builtin_offsetof(XXH_NAMESPACEXXH3_state_t, bufferedSize);
    size_t const initLength = __builtin_offsetof(XXH_NAMESPACEXXH3_state_t, nbStripesPerBlock) - initStart;
    __builtin_assume(__builtin_offsetof(XXH_NAMESPACEXXH3_state_t, nbStripesPerBlock) > initStart);
    __builtin_assume(statePtr != ((void*)0));
    __builtin___memset_chk ((char*)statePtr + initStart, 0, initLength, __builtin_object_size ((char*)statePtr + initStart, 0));
    statePtr->acc[0] = 0xC2B2AE3DU;
    statePtr->acc[1] = 0x9E3779B185EBCA87ULL;
    statePtr->acc[2] = 0xC2B2AE3D27D4EB4FULL;
    statePtr->acc[3] = 0x165667B19E3779F9ULL;
    statePtr->acc[4] = 0x85EBCA77C2B2AE63ULL;
    statePtr->acc[5] = 0x85EBCA77U;
    statePtr->acc[6] = 0x27D4EB2F165667C5ULL;
    statePtr->acc[7] = 0x9E3779B1U;
    statePtr->seed = seed;
    statePtr->useSeed = (seed != 0);
    statePtr->extSecret = (const unsigned char*)secret;
    __builtin_assume(secretSize >= 136);
    statePtr->secretLimit = secretSize - 64;
    statePtr->nbStripesPerBlock = statePtr->secretLimit / 8;
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH_INLINE_XXH3_64bits_reset(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr)
{
    if (statePtr == ((void*)0)) return XXH_NAMESPACEXXH_ERROR;
    XXH3_reset_internal(statePtr, 0, XXH3_kSecret, 192);
    return XXH_NAMESPACEXXH_OK;
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH_INLINE_XXH3_64bits_reset_withSecret(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr, __attribute__((noescape)) const void* secret, size_t secretSize)
{
    if (statePtr == ((void*)0)) return XXH_NAMESPACEXXH_ERROR;
    XXH3_reset_internal(statePtr, 0, secret, secretSize);
    if (secret == ((void*)0)) return XXH_NAMESPACEXXH_ERROR;
    if (secretSize < 136) return XXH_NAMESPACEXXH_ERROR;
    return XXH_NAMESPACEXXH_OK;
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH_INLINE_XXH3_64bits_reset_withSeed(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr, XXH64_hash_t seed)
{
    if (statePtr == ((void*)0)) return XXH_NAMESPACEXXH_ERROR;
    if (seed==0) return XXH_INLINE_XXH3_64bits_reset(statePtr);
    if ((seed != statePtr->seed) || (statePtr->extSecret != ((void*)0)))
        XXH3_initCustomSecret_sse2(statePtr->customSecret, seed);
    XXH3_reset_internal(statePtr, seed, ((void*)0), 192);
    return XXH_NAMESPACEXXH_OK;
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH_INLINE_XXH3_64bits_reset_withSecretandSeed(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr, __attribute__((noescape)) const void* secret, size_t secretSize, XXH64_hash_t seed64)
{
    if (statePtr == ((void*)0)) return XXH_NAMESPACEXXH_ERROR;
    if (secret == ((void*)0)) return XXH_NAMESPACEXXH_ERROR;
    if (secretSize < 136) return XXH_NAMESPACEXXH_ERROR;
    XXH3_reset_internal(statePtr, seed64, secret, secretSize);
    statePtr->useSeed = 1;
    return XXH_NAMESPACEXXH_OK;
}
static __attribute__((unused)) const xxh_u8 *
XXH3_consumeStripes(xxh_u64* restrict acc,
                    size_t* restrict nbStripesSoFarPtr, size_t nbStripesPerBlock,
                    const xxh_u8* restrict input, size_t nbStripes,
                    const xxh_u8* restrict secret, size_t secretLimit,
                    XXH3_f_accumulate f_acc,
                    XXH3_f_scrambleAcc f_scramble)
{
    const xxh_u8* initialSecret = secret + *nbStripesSoFarPtr * 8;
    if (nbStripes >= (nbStripesPerBlock - *nbStripesSoFarPtr)) {
        size_t nbStripesThisIter = nbStripesPerBlock - *nbStripesSoFarPtr;
        do {
            f_acc(acc, input, initialSecret, nbStripesThisIter);
            f_scramble(acc, secret + secretLimit);
            input += nbStripesThisIter * 64;
            nbStripes -= nbStripesThisIter;
            nbStripesThisIter = nbStripesPerBlock;
            initialSecret = secret;
        } while (nbStripes >= nbStripesPerBlock);
        *nbStripesSoFarPtr = 0;
    }
    if (nbStripes > 0) {
        f_acc(acc, input, initialSecret, nbStripes);
        input += nbStripes * 64;
        *nbStripesSoFarPtr += nbStripes;
    }
    return input;
}
static __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH3_update(XXH_NAMESPACEXXH3_state_t* restrict const state,
            const xxh_u8* restrict input, size_t len,
            XXH3_f_accumulate f_acc,
            XXH3_f_scrambleAcc f_scramble)
{
    if (input==((void*)0)) {
        __builtin_assume(len == 0);
        return XXH_NAMESPACEXXH_OK;
    }
    __builtin_assume(state != ((void*)0));
    { const xxh_u8* const bEnd = input + len;
        const unsigned char* const secret = (state->extSecret == ((void*)0)) ? state->customSecret : state->extSecret;
        xxh_u64* restrict const acc = state->acc;
        state->totalLen += len;
        __builtin_assume(state->bufferedSize <= 256);
        if (len <= 256 - state->bufferedSize) {
            XXH_memcpy(state->buffer + state->bufferedSize, input, len);
            state->bufferedSize += (XXH32_hash_t)len;
            return XXH_NAMESPACEXXH_OK;
        }
        do { _Static_assert(((256 % 64 == 0)),"XXH3_INTERNALBUFFER_SIZE % XXH_STRIPE_LEN == 0"); } while(0);
        if (state->bufferedSize) {
            size_t const loadSize = 256 - state->bufferedSize;
            XXH_memcpy(state->buffer + state->bufferedSize, input, loadSize);
            input += loadSize;
            XXH3_consumeStripes(acc,
                               &state->nbStripesSoFar, state->nbStripesPerBlock,
                                state->buffer, (256 / 64),
                                secret, state->secretLimit,
                                f_acc, f_scramble);
            state->bufferedSize = 0;
        }
        __builtin_assume(input < bEnd);
        if (bEnd - input > 256) {
            size_t nbStripes = (size_t)(bEnd - 1 - input) / 64;
            input = XXH3_consumeStripes(acc,
                                       &state->nbStripesSoFar, state->nbStripesPerBlock,
                                       input, nbStripes,
                                       secret, state->secretLimit,
                                       f_acc, f_scramble);
            XXH_memcpy(state->buffer + sizeof(state->buffer) - 64, input - 64, 64);
        }
        __builtin_assume(input < bEnd);
        __builtin_assume(bEnd - input <= 256);
        __builtin_assume(state->bufferedSize == 0);
        XXH_memcpy(state->buffer, input, (size_t)(bEnd-input));
        state->bufferedSize = (XXH32_hash_t)(bEnd-input);
    }
    return XXH_NAMESPACEXXH_OK;
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH_INLINE_XXH3_64bits_update(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* state, __attribute__((noescape)) const void* input, size_t len)
{
    return XXH3_update(state, (const xxh_u8*)input, len,
                       XXH3_accumulate_sse2, XXH3_scrambleAcc_sse2);
}
static __attribute__((unused)) void
XXH3_digest_long (XXH64_hash_t* acc,
                  const XXH_NAMESPACEXXH3_state_t* state,
                  const unsigned char* secret)
{
    xxh_u8 lastStripe[64];
    const xxh_u8* lastStripePtr;
    XXH_memcpy(acc, state->acc, sizeof(state->acc));
    if (state->bufferedSize >= 64) {
        size_t const nbStripes = (state->bufferedSize - 1) / 64;
        size_t nbStripesSoFar = state->nbStripesSoFar;
        XXH3_consumeStripes(acc,
                           &nbStripesSoFar, state->nbStripesPerBlock,
                            state->buffer, nbStripes,
                            secret, state->secretLimit,
                            XXH3_accumulate_sse2, XXH3_scrambleAcc_sse2);
        lastStripePtr = state->buffer + state->bufferedSize - 64;
    } else {
        size_t const catchupSize = 64 - state->bufferedSize;
        __builtin_assume(state->bufferedSize > 0);
        XXH_memcpy(lastStripe, state->buffer + sizeof(state->buffer) - catchupSize, catchupSize);
        XXH_memcpy(lastStripe + catchupSize, state->buffer, state->bufferedSize);
        lastStripePtr = lastStripe;
    }
    XXH3_accumulate_512_sse2(acc,
                        lastStripePtr,
                        secret + state->secretLimit - 7);
}
static __inline __attribute__((unused)) XXH64_hash_t XXH_INLINE_XXH3_64bits_digest (__attribute__((noescape)) const XXH_NAMESPACEXXH3_state_t* state)
{
    const unsigned char* const secret = (state->extSecret == ((void*)0)) ? state->customSecret : state->extSecret;
    if (state->totalLen > 240) {
        _Alignas(16) XXH64_hash_t acc[(64 / sizeof(xxh_u64))];
        XXH3_digest_long(acc, state, secret);
        return XXH3_mergeAccs(acc,
                              secret + 11,
                              (xxh_u64)state->totalLen * 0x9E3779B185EBCA87ULL);
    }
    if (state->useSeed)
        return XXH_INLINE_XXH3_64bits_withSeed(state->buffer, (size_t)state->totalLen, state->seed);
    return XXH_INLINE_XXH3_64bits_withSecret(state->buffer, (size_t)(state->totalLen),
                                  secret, state->secretLimit + 64);
}
static __attribute__((unused)) __attribute__((pure)) XXH_NAMESPACEXXH128_hash_t
XXH3_len_1to3_128b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    __builtin_assume(input != ((void*)0));
    __builtin_assume(1 <= len && len <= 3);
    __builtin_assume(secret != ((void*)0));
    { xxh_u8 const c1 = input[0];
        xxh_u8 const c2 = input[len >> 1];
        xxh_u8 const c3 = input[len - 1];
        xxh_u32 const combinedl = ((xxh_u32)c1 <<16) | ((xxh_u32)c2 << 24)
                                | ((xxh_u32)c3 << 0) | ((xxh_u32)len << 8);
        xxh_u32 const combinedh = __builtin_rotateleft32(XXH_swap32(combinedl), 13);
        xxh_u64 const bitflipl = (XXH_readLE32(secret) ^ XXH_readLE32(secret+4)) + seed;
        xxh_u64 const bitfliph = (XXH_readLE32(secret+8) ^ XXH_readLE32(secret+12)) - seed;
        xxh_u64 const keyed_lo = (xxh_u64)combinedl ^ bitflipl;
        xxh_u64 const keyed_hi = (xxh_u64)combinedh ^ bitfliph;
        XXH_NAMESPACEXXH128_hash_t h128;
        h128.low64 = XXH64_avalanche(keyed_lo);
        h128.high64 = XXH64_avalanche(keyed_hi);
        return h128;
    }
}
static __attribute__((unused)) __attribute__((pure)) XXH_NAMESPACEXXH128_hash_t
XXH3_len_4to8_128b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    __builtin_assume(input != ((void*)0));
    __builtin_assume(secret != ((void*)0));
    __builtin_assume(4 <= len && len <= 8);
    seed ^= (xxh_u64)XXH_swap32((xxh_u32)seed) << 32;
    { xxh_u32 const input_lo = XXH_readLE32(input);
        xxh_u32 const input_hi = XXH_readLE32(input + len - 4);
        xxh_u64 const input_64 = input_lo + ((xxh_u64)input_hi << 32);
        xxh_u64 const bitflip = (XXH_readLE64(secret+16) ^ XXH_readLE64(secret+24)) + seed;
        xxh_u64 const keyed = input_64 ^ bitflip;
        XXH_NAMESPACEXXH128_hash_t m128 = XXH_mult64to128(keyed, 0x9E3779B185EBCA87ULL + (len << 2));
        m128.high64 += (m128.low64 << 1);
        m128.low64 ^= (m128.high64 >> 3);
        m128.low64 = XXH_xorshift64(m128.low64, 35);
        m128.low64 *= PRIME_MX2;
        m128.low64 = XXH_xorshift64(m128.low64, 28);
        m128.high64 = XXH3_avalanche(m128.high64);
        return m128;
    }
}
static __attribute__((unused)) __attribute__((pure)) XXH_NAMESPACEXXH128_hash_t
XXH3_len_9to16_128b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    __builtin_assume(input != ((void*)0));
    __builtin_assume(secret != ((void*)0));
    __builtin_assume(9 <= len && len <= 16);
    { xxh_u64 const bitflipl = (XXH_readLE64(secret+32) ^ XXH_readLE64(secret+40)) - seed;
        xxh_u64 const bitfliph = (XXH_readLE64(secret+48) ^ XXH_readLE64(secret+56)) + seed;
        xxh_u64 const input_lo = XXH_readLE64(input);
        xxh_u64 input_hi = XXH_readLE64(input + len - 8);
        XXH_NAMESPACEXXH128_hash_t m128 = XXH_mult64to128(input_lo ^ input_hi ^ bitflipl, 0x9E3779B185EBCA87ULL);
        m128.low64 += (xxh_u64)(len - 1) << 54;
        input_hi ^= bitfliph;
        if (sizeof(void *) < sizeof(xxh_u64)) {
            m128.high64 += (input_hi & 0xFFFFFFFF00000000ULL) + ((xxh_u64)(xxh_u32)((xxh_u32)input_hi) * (xxh_u64)(xxh_u32)(0x85EBCA77U));
        } else {
            m128.high64 += input_hi + ((xxh_u64)(xxh_u32)((xxh_u32)input_hi) * (xxh_u64)(xxh_u32)(0x85EBCA77U - 1));
        }
        m128.low64 ^= XXH_swap64(m128.high64);
        {
            XXH_NAMESPACEXXH128_hash_t h128 = XXH_mult64to128(m128.low64, 0xC2B2AE3D27D4EB4FULL);
            h128.high64 += m128.high64 * 0xC2B2AE3D27D4EB4FULL;
            h128.low64 = XXH3_avalanche(h128.low64);
            h128.high64 = XXH3_avalanche(h128.high64);
            return h128;
    } }
}
static __attribute__((unused)) __attribute__((pure)) XXH_NAMESPACEXXH128_hash_t
XXH3_len_0to16_128b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    __builtin_assume(len <= 16);
    { if (len > 8) return XXH3_len_9to16_128b(input, len, secret, seed);
        if (len >= 4) return XXH3_len_4to8_128b(input, len, secret, seed);
        if (len) return XXH3_len_1to3_128b(input, len, secret, seed);
        { XXH_NAMESPACEXXH128_hash_t h128;
            xxh_u64 const bitflipl = XXH_readLE64(secret+64) ^ XXH_readLE64(secret+72);
            xxh_u64 const bitfliph = XXH_readLE64(secret+80) ^ XXH_readLE64(secret+88);
            h128.low64 = XXH64_avalanche(seed ^ bitflipl);
            h128.high64 = XXH64_avalanche( seed ^ bitfliph);
            return h128;
    } }
}
static __attribute__((unused)) XXH_NAMESPACEXXH128_hash_t
XXH128_mix32B(XXH_NAMESPACEXXH128_hash_t acc, const xxh_u8* input_1, const xxh_u8* input_2,
              const xxh_u8* secret, XXH64_hash_t seed)
{
    acc.low64 += XXH3_mix16B (input_1, secret+0, seed);
    acc.low64 ^= XXH_readLE64(input_2) + XXH_readLE64(input_2 + 8);
    acc.high64 += XXH3_mix16B (input_2, secret+16, seed);
    acc.high64 ^= XXH_readLE64(input_1) + XXH_readLE64(input_1 + 8);
    return acc;
}
static __attribute__((unused)) __attribute__((pure)) XXH_NAMESPACEXXH128_hash_t
XXH3_len_17to128_128b(const xxh_u8* restrict input, size_t len,
                      const xxh_u8* restrict secret, size_t secretSize,
                      XXH64_hash_t seed)
{
    __builtin_assume(secretSize >= 136); (void)secretSize;
    __builtin_assume(16 < len && len <= 128);
    { XXH_NAMESPACEXXH128_hash_t acc;
        acc.low64 = len * 0x9E3779B185EBCA87ULL;
        acc.high64 = 0;
        if (len > 32) {
            if (len > 64) {
                if (len > 96) {
                    acc = XXH128_mix32B(acc, input+48, input+len-64, secret+96, seed);
                }
                acc = XXH128_mix32B(acc, input+32, input+len-48, secret+64, seed);
            }
            acc = XXH128_mix32B(acc, input+16, input+len-32, secret+32, seed);
        }
        acc = XXH128_mix32B(acc, input, input+len-16, secret, seed);
        { XXH_NAMESPACEXXH128_hash_t h128;
            h128.low64 = acc.low64 + acc.high64;
            h128.high64 = (acc.low64 * 0x9E3779B185EBCA87ULL)
                        + (acc.high64 * 0x85EBCA77C2B2AE63ULL)
                        + ((len - seed) * 0xC2B2AE3D27D4EB4FULL);
            h128.low64 = XXH3_avalanche(h128.low64);
            h128.high64 = (XXH64_hash_t)0 - XXH3_avalanche(h128.high64);
            return h128;
        }
    }
}
static __attribute__((pure)) XXH_NAMESPACEXXH128_hash_t
XXH3_len_129to240_128b(const xxh_u8* restrict input, size_t len,
                       const xxh_u8* restrict secret, size_t secretSize,
                       XXH64_hash_t seed)
{
    __builtin_assume(secretSize >= 136); (void)secretSize;
    __builtin_assume(128 < len && len <= 240);
    { XXH_NAMESPACEXXH128_hash_t acc;
        unsigned i;
        acc.low64 = len * 0x9E3779B185EBCA87ULL;
        acc.high64 = 0;
        for (i = 32; i < 160; i += 32) {
            acc = XXH128_mix32B(acc,
                                input + i - 32,
                                input + i - 16,
                                secret + i - 32,
                                seed);
        }
        acc.low64 = XXH3_avalanche(acc.low64);
        acc.high64 = XXH3_avalanche(acc.high64);
        for (i=160; i <= len; i += 32) {
            acc = XXH128_mix32B(acc,
                                input + i - 32,
                                input + i - 16,
                                secret + 3 + i - 160,
                                seed);
        }
        acc = XXH128_mix32B(acc,
                            input + len - 16,
                            input + len - 32,
                            secret + 136 - 17 - 16,
                            (XXH64_hash_t)0 - seed);
        { XXH_NAMESPACEXXH128_hash_t h128;
            h128.low64 = acc.low64 + acc.high64;
            h128.high64 = (acc.low64 * 0x9E3779B185EBCA87ULL)
                        + (acc.high64 * 0x85EBCA77C2B2AE63ULL)
                        + ((len - seed) * 0xC2B2AE3D27D4EB4FULL);
            h128.low64 = XXH3_avalanche(h128.low64);
            h128.high64 = (XXH64_hash_t)0 - XXH3_avalanche(h128.high64);
            return h128;
        }
    }
}
static __attribute__((unused)) XXH_NAMESPACEXXH128_hash_t
XXH3_hashLong_128b_internal(const void* restrict input, size_t len,
                            const xxh_u8* restrict secret, size_t secretSize,
                            XXH3_f_accumulate f_acc,
                            XXH3_f_scrambleAcc f_scramble)
{
    _Alignas(16) xxh_u64 acc[(64 / sizeof(xxh_u64))] = { 0xC2B2AE3DU, 0x9E3779B185EBCA87ULL, 0xC2B2AE3D27D4EB4FULL, 0x165667B19E3779F9ULL, 0x85EBCA77C2B2AE63ULL, 0x85EBCA77U, 0x27D4EB2F165667C5ULL, 0x9E3779B1U };
    XXH3_hashLong_internal_loop(acc, (const xxh_u8*)input, len, secret, secretSize, f_acc, f_scramble);
    do { _Static_assert(((sizeof(acc) == 64)),"sizeof(acc) == 64"); } while(0);
    __builtin_assume(secretSize >= sizeof(acc) + 11);
    { XXH_NAMESPACEXXH128_hash_t h128;
        h128.low64 = XXH3_mergeAccs(acc,
                                     secret + 11,
                                     (xxh_u64)len * 0x9E3779B185EBCA87ULL);
        h128.high64 = XXH3_mergeAccs(acc,
                                     secret + secretSize
                                            - sizeof(acc) - 11,
                                     ~((xxh_u64)len * 0xC2B2AE3D27D4EB4FULL));
        return h128;
    }
}
static __attribute__((pure)) XXH_NAMESPACEXXH128_hash_t
XXH3_hashLong_128b_default(const void* restrict input, size_t len,
                           XXH64_hash_t seed64,
                           const void* restrict secret, size_t secretLen)
{
    (void)seed64; (void)secret; (void)secretLen;
    return XXH3_hashLong_128b_internal(input, len, XXH3_kSecret, sizeof(XXH3_kSecret),
                                       XXH3_accumulate_sse2, XXH3_scrambleAcc_sse2);
}
static __attribute__((unused)) XXH_NAMESPACEXXH128_hash_t
XXH3_hashLong_128b_withSecret(const void* restrict input, size_t len,
                              XXH64_hash_t seed64,
                              const void* restrict secret, size_t secretLen)
{
    (void)seed64;
    return XXH3_hashLong_128b_internal(input, len, (const xxh_u8*)secret, secretLen,
                                       XXH3_accumulate_sse2, XXH3_scrambleAcc_sse2);
}
static __attribute__((unused)) XXH_NAMESPACEXXH128_hash_t
XXH3_hashLong_128b_withSeed_internal(const void* restrict input, size_t len,
                                XXH64_hash_t seed64,
                                XXH3_f_accumulate f_acc,
                                XXH3_f_scrambleAcc f_scramble,
                                XXH3_f_initCustomSecret f_initSec)
{
    if (seed64 == 0)
        return XXH3_hashLong_128b_internal(input, len,
                                           XXH3_kSecret, sizeof(XXH3_kSecret),
                                           f_acc, f_scramble);
    { _Alignas(16) xxh_u8 secret[192];
        f_initSec(secret, seed64);
        return XXH3_hashLong_128b_internal(input, len, (const xxh_u8*)secret, sizeof(secret),
                                           f_acc, f_scramble);
    }
}
static XXH_NAMESPACEXXH128_hash_t
XXH3_hashLong_128b_withSeed(const void* input, size_t len,
                            XXH64_hash_t seed64, const void* restrict secret, size_t secretLen)
{
    (void)secret; (void)secretLen;
    return XXH3_hashLong_128b_withSeed_internal(input, len, seed64,
                XXH3_accumulate_sse2, XXH3_scrambleAcc_sse2, XXH3_initCustomSecret_sse2);
}
typedef XXH_NAMESPACEXXH128_hash_t (*XXH3_hashLong128_f)(const void* restrict, size_t,
                                            XXH64_hash_t, const void* restrict, size_t);
static __attribute__((unused)) XXH_NAMESPACEXXH128_hash_t
XXH3_128bits_internal(const void* input, size_t len,
                      XXH64_hash_t seed64, const void* restrict secret, size_t secretLen,
                      XXH3_hashLong128_f f_hl128)
{
    __builtin_assume(secretLen >= 136);
    if (len <= 16)
        return XXH3_len_0to16_128b((const xxh_u8*)input, len, (const xxh_u8*)secret, seed64);
    if (len <= 128)
        return XXH3_len_17to128_128b((const xxh_u8*)input, len, (const xxh_u8*)secret, secretLen, seed64);
    if (len <= 240)
        return XXH3_len_129to240_128b((const xxh_u8*)input, len, (const xxh_u8*)secret, secretLen, seed64);
    return f_hl128(input, len, seed64, secret, secretLen);
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH128_hash_t XXH_INLINE_XXH3_128bits(__attribute__((noescape)) const void* input, size_t len)
{
    return XXH3_128bits_internal(input, len, 0,
                                 XXH3_kSecret, sizeof(XXH3_kSecret),
                                 XXH3_hashLong_128b_default);
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH128_hash_t
XXH_INLINE_XXH3_128bits_withSecret(__attribute__((noescape)) const void* input, size_t len, __attribute__((noescape)) const void* secret, size_t secretSize)
{
    return XXH3_128bits_internal(input, len, 0,
                                 (const xxh_u8*)secret, secretSize,
                                 XXH3_hashLong_128b_withSecret);
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH128_hash_t
XXH_INLINE_XXH3_128bits_withSeed(__attribute__((noescape)) const void* input, size_t len, XXH64_hash_t seed)
{
    return XXH3_128bits_internal(input, len, seed,
                                 XXH3_kSecret, sizeof(XXH3_kSecret),
                                 XXH3_hashLong_128b_withSeed);
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH128_hash_t
XXH_INLINE_XXH3_128bits_withSecretandSeed(__attribute__((noescape)) const void* input, size_t len, __attribute__((noescape)) const void* secret, size_t secretSize, XXH64_hash_t seed)
{
    if (len <= 240)
        return XXH3_128bits_internal(input, len, seed, XXH3_kSecret, sizeof(XXH3_kSecret), ((void*)0));
    return XXH3_hashLong_128b_withSecret(input, len, seed, secret, secretSize);
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH128_hash_t
XXH_INLINE_XXH128(__attribute__((noescape)) const void* input, size_t len, XXH64_hash_t seed)
{
    return XXH_INLINE_XXH3_128bits_withSeed(input, len, seed);
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH_INLINE_XXH3_128bits_reset(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr)
{
    return XXH_INLINE_XXH3_64bits_reset(statePtr);
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH_INLINE_XXH3_128bits_reset_withSecret(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr, __attribute__((noescape)) const void* secret, size_t secretSize)
{
    return XXH_INLINE_XXH3_64bits_reset_withSecret(statePtr, secret, secretSize);
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH_INLINE_XXH3_128bits_reset_withSeed(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr, XXH64_hash_t seed)
{
    return XXH_INLINE_XXH3_64bits_reset_withSeed(statePtr, seed);
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH_INLINE_XXH3_128bits_reset_withSecretandSeed(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* statePtr, __attribute__((noescape)) const void* secret, size_t secretSize, XXH64_hash_t seed)
{
    return XXH_INLINE_XXH3_64bits_reset_withSecretandSeed(statePtr, secret, secretSize, seed);
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH_INLINE_XXH3_128bits_update(__attribute__((noescape)) XXH_NAMESPACEXXH3_state_t* state, __attribute__((noescape)) const void* input, size_t len)
{
    return XXH_INLINE_XXH3_64bits_update(state, input, len);
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH128_hash_t XXH_INLINE_XXH3_128bits_digest (__attribute__((noescape)) const XXH_NAMESPACEXXH3_state_t* state)
{
    const unsigned char* const secret = (state->extSecret == ((void*)0)) ? state->customSecret : state->extSecret;
    if (state->totalLen > 240) {
        _Alignas(16) XXH64_hash_t acc[(64 / sizeof(xxh_u64))];
        XXH3_digest_long(acc, state, secret);
        __builtin_assume(state->secretLimit + 64 >= sizeof(acc) + 11);
        { XXH_NAMESPACEXXH128_hash_t h128;
            h128.low64 = XXH3_mergeAccs(acc,
                                         secret + 11,
                                         (xxh_u64)state->totalLen * 0x9E3779B185EBCA87ULL);
            h128.high64 = XXH3_mergeAccs(acc,
                                         secret + state->secretLimit + 64
                                                - sizeof(acc) - 11,
                                         ~((xxh_u64)state->totalLen * 0xC2B2AE3D27D4EB4FULL));
            return h128;
        }
    }
    if (state->seed)
        return XXH_INLINE_XXH3_128bits_withSeed(state->buffer, (size_t)state->totalLen, state->seed);
    return XXH_INLINE_XXH3_128bits_withSecret(state->buffer, (size_t)(state->totalLen),
                                   secret, state->secretLimit + 64);
}
static __inline __attribute__((unused)) int XXH_INLINE_XXH128_isEqual(XXH_NAMESPACEXXH128_hash_t h1, XXH_NAMESPACEXXH128_hash_t h2)
{
    return !(memcmp(&h1, &h2, sizeof(h1)));
}
static __inline __attribute__((unused)) int XXH_INLINE_XXH128_cmp(__attribute__((noescape)) const void* h128_1, __attribute__((noescape)) const void* h128_2)
{
    XXH_NAMESPACEXXH128_hash_t const h1 = *(const XXH_NAMESPACEXXH128_hash_t*)h128_1;
    XXH_NAMESPACEXXH128_hash_t const h2 = *(const XXH_NAMESPACEXXH128_hash_t*)h128_2;
    int const hcmp = (h1.high64 > h2.high64) - (h2.high64 > h1.high64);
    if (hcmp) return hcmp;
    return (h1.low64 > h2.low64) - (h2.low64 > h1.low64);
}
static __inline __attribute__((unused)) void
XXH_INLINE_XXH128_canonicalFromHash(__attribute__((noescape)) XXH_NAMESPACEXXH128_canonical_t* dst, XXH_NAMESPACEXXH128_hash_t hash)
{
    do { _Static_assert(((sizeof(XXH_NAMESPACEXXH128_canonical_t) == sizeof(XXH_NAMESPACEXXH128_hash_t))),"sizeof(XXH128_canonical_t) == sizeof(XXH128_hash_t)"); } while(0);
    if (1) {
        hash.high64 = XXH_swap64(hash.high64);
        hash.low64 = XXH_swap64(hash.low64);
    }
    XXH_memcpy(dst, &hash.high64, sizeof(hash.high64));
    XXH_memcpy((char*)dst + sizeof(hash.high64), &hash.low64, sizeof(hash.low64));
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH128_hash_t
XXH_INLINE_XXH128_hashFromCanonical(__attribute__((noescape)) const XXH_NAMESPACEXXH128_canonical_t* src)
{
    XXH_NAMESPACEXXH128_hash_t h;
    h.high64 = XXH_readBE64(src);
    h.low64 = XXH_readBE64(src->digest + 8);
    return h;
}
static __attribute__((unused)) void XXH3_combine16(void* dst, XXH_NAMESPACEXXH128_hash_t h128)
{
    XXH_writeLE64( dst, XXH_readLE64(dst) ^ h128.low64 );
    XXH_writeLE64( (char*)dst+8, XXH_readLE64((char*)dst+8) ^ h128.high64 );
}
static __inline __attribute__((unused)) XXH_NAMESPACEXXH_errorcode
XXH_INLINE_XXH3_generateSecret(__attribute__((noescape)) void* secretBuffer, size_t secretSize, __attribute__((noescape)) const void* customSeed, size_t customSeedSize)
{
    if (secretBuffer == ((void*)0)) return XXH_NAMESPACEXXH_ERROR;
    if (secretSize < 136) return XXH_NAMESPACEXXH_ERROR;
    if (customSeedSize == 0) {
        customSeed = XXH3_kSecret;
        customSeedSize = 192;
    }
    if (customSeed == ((void*)0)) return XXH_NAMESPACEXXH_ERROR;
    { size_t pos = 0;
        while (pos < secretSize) {
            size_t const toCopy = ((((secretSize - pos)) > (customSeedSize)) ? (customSeedSize) : ((secretSize - pos)));
            __builtin___memcpy_chk ((char*)secretBuffer + pos, customSeed, toCopy, __builtin_object_size ((char*)secretBuffer + pos, 0));
            pos += toCopy;
    } }
    { size_t const nbSeg16 = secretSize / 16;
        size_t n;
        XXH_NAMESPACEXXH128_canonical_t scrambler;
        XXH_INLINE_XXH128_canonicalFromHash(&scrambler, XXH_INLINE_XXH128(customSeed, customSeedSize, 0));
        for (n=0; n<nbSeg16; n++) {
            XXH_NAMESPACEXXH128_hash_t const h128 = XXH_INLINE_XXH128(&scrambler, sizeof(scrambler), n);
            XXH3_combine16((char*)secretBuffer + n*16, h128);
        }
        XXH3_combine16((char*)secretBuffer + secretSize - 16, XXH_INLINE_XXH128_hashFromCanonical(&scrambler));
    }
    return XXH_NAMESPACEXXH_OK;
}
static __inline __attribute__((unused)) void
XXH_INLINE_XXH3_generateSecret_fromSeed(__attribute__((noescape)) void* secretBuffer, XXH64_hash_t seed)
{
    _Alignas(16) xxh_u8 secret[192];
    XXH3_initCustomSecret_sse2(secret, seed);
    __builtin_assume(secretBuffer != ((void*)0));
    __builtin___memcpy_chk (secretBuffer, secret, 192, __builtin_object_size (secretBuffer, 0));
}

u32 hash_decl_key(void *context, DeclarationKey key) {
  (void)(context);
  return XXH_INLINE_XXH32(&key, sizeof(DeclarationKey), 0);
}
b32 cmp_decl_key(void *context, DeclarationKey a, DeclarationKey b) {
  (void)(context);
  return a.parent == b.parent && a.name == b.name;
}
 void decls_init(DeclarationInterner *interner, InternerOptions *options);
                 void decls_deinit(DeclarationInterner *interner);
                 DeclarationIndex decls_add(DeclarationInterner *interner, DeclarationKey item);
                 DeclarationIndex decls_add_checked(DeclarationInterner *interner, DeclarationKey item, b32 *already_present);
                 b32 decls_find(DeclarationInterner *interner, DeclarationKey item, DeclarationIndex *idx);
                 DeclarationKey decls_get(DeclarationInterner *interner, DeclarationIndex idx);
                 Declaration decls_get_extra(DeclarationInterner *interner, DeclarationIndex idx);
                 Declaration *decls_extra_get_ptr(DeclarationInterner *interner, DeclarationIndex idx);
                 void decls_set_extra(DeclarationInterner *interner, DeclarationIndex idx, Declaration);
static usize decls__list_cap(DeclarationInternerList *list);
static DeclarationInternerExtra *decls__list_push(DeclarationInternerList *list, Arena *arena);
static DeclarationInternerExtra decls__list_pop(DeclarationInternerList *list);
static void decls__list_append(DeclarationInternerList *list, Arena *arena, DeclarationInternerExtra item);
static DeclarationInternerExtra *decls__list_ptr_at_unchecked(DeclarationInternerList *list, usize i);
static DeclarationInternerExtra decls__list_at_unchecked(DeclarationInternerList *list, usize i);
static void decls__list_copy_to_array(DeclarationInternerList *list, DeclarationInternerExtra *out);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
usize decls__list_cap(DeclarationInternerList *list) {
  return (((usize)1 << list->segment_count) - 1) << 6;
}
static void decls__list__ensure_capacity(DeclarationInternerList *list, Arena *arena, usize min_capacity) {
  usize cap = decls__list_cap(list);
  if (cap >= min_capacity) {
    return;
  }
  usize required_segment_count = segment_count_at_size(6, min_capacity);
  if (required_segment_count <= list->segment_count) {
    return;
  }
  for (usize i = list->segment_count; i < required_segment_count; i++) {
    usize size = segment_size(6, i);
    list->segments[i] = arena_push(arena, (size) * sizeof(DeclarationInternerExtra), _Alignof(DeclarationInternerExtra));
  }
  list->segment_count = required_segment_count;
}
static
DeclarationInternerExtra *decls__list_push(DeclarationInternerList *list, Arena *arena) {
  decls__list__ensure_capacity(list, arena, list->len + 1);
  DeclarationInternerExtra *p = decls__list_ptr_at_unchecked(list, list->len);
  list->len += 1;
  return p;
}
static
DeclarationInternerExtra decls__list_pop(DeclarationInternerList *list) {
  (__builtin_expect(!(list->len > 0), 0) ? __assert_rtn(__func__, "segment_list.h", 113, "list->len > 0") : (void)0);
  DeclarationInternerExtra res = decls__list_at_unchecked(list, list->len-1);
  list->len -= 1;
  return res;
}
static
void decls__list_append(DeclarationInternerList *list, Arena *arena, DeclarationInternerExtra item) {
  *decls__list_push(list, arena) = item;
}
static
DeclarationInternerExtra *decls__list_ptr_at_unchecked(DeclarationInternerList *list, usize idx) {
  usize si = segment_idx(6, idx);
  usize i = item_idx(6, idx, si);
  return &list->segments[si][i];
}
static
DeclarationInternerExtra decls__list_at_unchecked(DeclarationInternerList *list, usize idx) {
  return *decls__list_ptr_at_unchecked(list, idx);
}
static
void decls__list_copy_to_array(DeclarationInternerList *list, DeclarationInternerExtra *out) {
  if (list->len == 0) {
    return;
  }
  u32 offset = 0;
  u32 segment_count = segment_count_at_size(6, list->len);
  for (u32 i = 0; i < segment_count - 1; i++) {
    u32 size = segment_size(6, i);
    __builtin___memcpy_chk (out + offset, list->segments[i], size * sizeof(DeclarationInternerExtra), __builtin_object_size (out + offset, 0));
    offset += size;
  }
  __builtin___memcpy_chk (out + offset, list->segments[segment_count-1], (list->len - offset) * sizeof(DeclarationInternerExtra), __builtin_object_size (out + offset, 0));
}
#pragma clang diagnostic pop
static void decls__map_init(DeclarationInternerMap *map, HashMapOptions *options);
static void decls__map_deinit(DeclarationInternerMap *map);
static u32 decls__map_cap(DeclarationInternerMap *map);
static DeclarationInternerMapBucket *decls__map_insert_key_and_get_bucket(DeclarationInternerMap *map, DeclarationKey key, b32 *was_occupied);
static DeclarationInternerMapBucket *decls__map_remove_key_and_get_bucket(DeclarationInternerMap *map, DeclarationKey key);
static DeclarationIndex *decls__map_find(DeclarationInternerMap *map, DeclarationKey key);
static b32 decls__map_insert(DeclarationInternerMap *map, DeclarationKey key, DeclarationIndex value);
static b32 decls__map_remove(DeclarationInternerMap *map, DeclarationKey key);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
static __attribute__((always_inline)) inline b32 slot_is_empty(u32 meta) { return meta == 0; }
static __attribute__((always_inline)) inline b32 slot_is_occupied(u32 meta) { return (meta & 0x00000001) != 0; }
static __attribute__((always_inline)) inline b32 slot_is_tombstone(u32 meta) { return (meta & 0x00000002) != 0; }
static __attribute__((always_inline)) inline b32 slot_is_stale(u32 meta) { return (meta & 0x00000003) == 0x00000003; }
static __attribute__((always_inline)) inline u32 read_fingerprint(u32 x) { return x & 0xfffffffc; }
static void decls__map__grow_and_rehash(DeclarationInternerMap *map);
static u32 decls__map__find_occupied_index(DeclarationInternerMap *map, DeclarationKey key);
static u32 decls__map__find_insert_index(DeclarationInternerMap *map, u32 hash, DeclarationKey key);
static b32 decls__map__rehash(DeclarationInternerMap *map, void *mem, u32 cap, u32 size);
static
void decls__map_init(DeclarationInternerMap *map, HashMapOptions *options) {
  u32 size = (((options->initial_size)>(4))?(options->initial_size):(4));
  (__builtin_expect(!(is_zero_or_power_of_two(size)), 0) ? __assert_rtn(__func__, "hashmap.h", 121, "is_zero_or_power_of_two(size)") : (void)0);
  u32 byte_size = size * (sizeof(u32) + sizeof(DeclarationInternerMapBucket));
  DeclarationInternerMapBucket *buckets = (options->allocator).fn((options->allocator).ctx, ((void*)0), 0, byte_size, _Alignof(DeclarationInternerMapBucket));
  u32 *meta = ((u32*)(buckets + size));
  __builtin___memset_chk (meta, 0, size * sizeof(u32), __builtin_object_size (meta, 0));
  map->allocator = options->allocator;
  map->buckets = buckets;
  map->meta = meta;
  map->context = options->context;
  map->mask = size - 1;
  map->item_count = 0;
}
static
void decls__map_deinit(DeclarationInternerMap *map) {
  if (!((map->buckets) == ((void*)0))) {
    u32 cap = decls__map_cap(map);
    u32 byte_size = cap * (sizeof(DeclarationInternerMapBucket) + sizeof(u32));
    (map->allocator).fn((map->allocator).ctx, map->buckets, byte_size, 0, 0);
  }
  __builtin___memset_chk (map, 0, sizeof(*map), __builtin_object_size (map, 0));
}
static
u32 decls__map_cap(DeclarationInternerMap *map) {
  return map->mask + 1;
}
static
DeclarationInternerMapBucket *decls__map_insert_key_and_get_bucket(DeclarationInternerMap *map, DeclarationKey key, b32 *was_occupied) {
  u32 cap = decls__map_cap(map);
  if (map->item_count > cap * 0.8) {
    decls__map__grow_and_rehash(map);
  }
  u32 hash = hash_decl_key(map->context, key);
  u32 idx = decls__map__find_insert_index(map, hash, key);
  while (idx == 4294967295U) {
    decls__map__grow_and_rehash(map);
    idx = decls__map__find_insert_index(map, hash, key);
  }
  b32 is_occupied = slot_is_occupied(map->meta[idx]);
  if (!is_occupied) {
    map->item_count += 1;
    map->meta[idx] = read_fingerprint(hash) | 0x00000001;
    map->buckets[idx].key = key;
  }
  *was_occupied = is_occupied;
  return &map->buckets[idx];
}
static
DeclarationInternerMapBucket *decls__map_remove_key_and_get_bucket(DeclarationInternerMap *map, DeclarationKey key) {
  u32 idx = decls__map__find_occupied_index(map, key);
  if (idx == 4294967295U) {
    return ((void*)0);
  }
  map->item_count -= 1;
  map->meta[idx] = 0x00000002;
  return &map->buckets[idx];
}
static
DeclarationIndex *decls__map_find(DeclarationInternerMap *map, DeclarationKey key) {
  u32 idx = decls__map__find_occupied_index(map, key);
  if (idx == 4294967295U) {
    return ((void*)0);
  }
  return &map->buckets[idx].val;
}
static
b32 decls__map_insert(DeclarationInternerMap *map, DeclarationKey key, DeclarationIndex value) {
  b32 was_occupied;
  DeclarationInternerMapBucket *bucket = decls__map_insert_key_and_get_bucket(map, key, &was_occupied);
  bucket->val = value;
  return was_occupied;
}
static
b32 decls__map_remove(DeclarationInternerMap *map, DeclarationKey key) {
  DeclarationInternerMapBucket *bucket = decls__map_remove_key_and_get_bucket(map, key);
  return !((bucket) == ((void*)0));
}
static void decls__map__grow_and_rehash(DeclarationInternerMap *map) {
  u32 cap = decls__map_cap(map);
  u32 size = cap * 2;
  while (1) {
    if (size == 0) { do { fprintf(__stderrp, "panic in %s at %s:%u\n", __func__, "src/hashmap.h", 233); abort(); } while (0); }
    usize new_byte_size = size * (sizeof(u32) + sizeof(DeclarationInternerMapBucket));
    void *mem = (map->allocator).fn((map->allocator).ctx, ((void*)0), 0, new_byte_size, _Alignof(DeclarationInternerMapBucket));
    b32 ok = decls__map__rehash(map, mem, cap, size);
    if (ok) {
      usize old_byte_size = cap * (sizeof(u32) + sizeof(DeclarationInternerMapBucket));
      (map->allocator).fn((map->allocator).ctx, map->buckets, old_byte_size, 0, 0);
      DeclarationInternerMapBucket *buckets = mem;
      u32 *meta = ((u32*)(buckets + size));
      map->mask = size - 1;
      map->meta = meta;
      map->buckets = buckets;
      break;
    }
    (map->allocator).fn((map->allocator).ctx, mem, new_byte_size, 0, 0);
    size *= 2;
  }
}
static b32 decls__map__rehash(DeclarationInternerMap *map, void *mem, u32 cap, u32 size) {
  DeclarationInternerMapBucket *buckets = mem;
  u32 *meta = ((u32*)(buckets + size));
  __builtin___memcpy_chk (buckets, map->buckets, cap * sizeof(DeclarationInternerMapBucket), __builtin_object_size (buckets, 0));
  __builtin___memcpy_chk (meta, map->meta, cap * sizeof(u32), __builtin_object_size (meta, 0));
  __builtin___memset_chk (meta + cap, 0, (size - cap) * sizeof(u32), __builtin_object_size (meta + cap, 0));
  for (u32 i = 0; i < cap; i++) {
    if (slot_is_occupied(meta[i])) {
      meta[i] |= 0x00000003;
    } else if (slot_is_tombstone(meta[i])) {
      meta[i] = 0;
    }
  }
  u32 mask = size - 1;
  for (u32 i = 0; i < cap; i++) {
    if (!slot_is_stale(meta[i])) {
      continue;
    }
    DeclarationInternerMapBucket bi = buckets[i];
    u32 hash = hash_decl_key(map->context, bi.key);
    u32 start = hash & mask;
    u32 idx = 4294967295U;
    for (u32 k = 0; k < 32; k++) {
      u32 j = (start + k) & mask;
      if (slot_is_empty(meta[j]) || slot_is_stale(meta[j])) {
        idx = j;
        break;
      }
    }
    if (idx == 4294967295U) {
      return 0;
    }
    if (i == idx) {
      meta[idx] = read_fingerprint(hash) | 0x00000001;
      continue;
    }
    meta[i] = 0;
    if (slot_is_empty(meta[idx])) {
      meta[idx] = read_fingerprint(hash) | 0x00000001;
      buckets[idx] = bi;
      continue;
    }
    if (slot_is_stale(meta[idx])) {
      meta[idx] = read_fingerprint(hash) | 0x00000001;
      do { DeclarationInternerMapBucket tmp_ = (bi); bi = (buckets[idx]); buckets[idx] = tmp_; } while (0);
    }
    for (u32 j = 0; j < cap; j++) {
      u32 hash = hash_decl_key(map->context, bi.key);
      u32 start = hash & mask;
      for (u32 k = 0; k < 32; k++) {
        u32 idx = (start + k) & mask;
        if (slot_is_empty(meta[idx])) {
          meta[idx] = read_fingerprint(hash) | 0x00000001;
          buckets[idx] = bi;
          goto next;
        }
        if (slot_is_stale(meta[idx])) {
          meta[idx] = read_fingerprint(hash) | 0x00000001;
          do { DeclarationInternerMapBucket tmp_ = (bi); bi = (buckets[idx]); buckets[idx] = tmp_; } while (0);
          goto place_next_bucket_in_chain;
        }
      }
      return 0;
    place_next_bucket_in_chain:
      continue;
    }
  next:
    continue;
  }
  return 1;
}
static u32 decls__map__find_occupied_index(DeclarationInternerMap *map, DeclarationKey key) {
  u32 hash = hash_decl_key(map->context, key);
  u32 fingerprint = read_fingerprint(hash);
  u32 start_idx = hash & map->mask;
  for (u32 i = 0; i < 32; i++) {
    u32 idx = (start_idx + i) & map->mask;
    if (slot_is_empty(map->meta[idx])) {
      return 4294967295U;
    }
    if (slot_is_occupied(map->meta[idx]) && read_fingerprint(map->meta[idx]) == fingerprint) {
      if (cmp_decl_key(map->context, key, map->buckets[idx].key)) {
        return idx;
      }
    }
  }
  return 4294967295U;
}
static u32 decls__map__find_insert_index(DeclarationInternerMap *map, u32 hash, DeclarationKey key) {
  u32 fingerprint = read_fingerprint(hash);
  u32 start_idx = hash & map->mask;
  u32 tombstone_idx = 4294967295U;
  for (u32 i = 0; i < 32; i++) {
    u32 idx = (start_idx + i) & map->mask;
    if (slot_is_empty(map->meta[idx])) {
      if (tombstone_idx != 4294967295U) {
        return tombstone_idx;
      } else {
        return idx;
      }
    }
    if (slot_is_tombstone(map->meta[idx]) && tombstone_idx == 4294967295U) {
      tombstone_idx = idx;
    }
    if (slot_is_occupied(map->meta[idx]) && read_fingerprint(map->meta[idx]) == fingerprint) {
      if (cmp_decl_key(map->context, key, map->buckets[idx].key)) {
        return idx;
      }
    }
  }
  return tombstone_idx;
}
#pragma clang diagnostic pop
void decls_init(DeclarationInterner *interner, InternerOptions *options) {
  interner->arena = options->arena;
  __builtin___memset_chk (&interner->list, 0, sizeof(DeclarationInternerList), __builtin_object_size (&interner->list, 0));
  decls__map_init(&interner->map, &(HashMapOptions){
    .allocator = options->map_allocator,
    .initial_size = options->map_initial_size,
    .context = options->context,
  });
}
void decls_deinit(DeclarationInterner *interner) {
  decls__map_deinit(&interner->map);
  __builtin___memset_chk (interner, 0, sizeof(DeclarationInterner), __builtin_object_size (interner, 0));
}
DeclarationIndex decls_add(DeclarationInterner *interner, DeclarationKey item) {
  b32 ignore;
  return decls_add_checked(interner, item, &ignore);
}
                 DeclarationIndex decls_add_checked(DeclarationInterner *interner, DeclarationKey item, b32 *already_present) {
  b32 was_occupied;
  DeclarationInternerMapBucket *bucket = decls__map_insert_key_and_get_bucket(&interner->map, item, &was_occupied);
  if (was_occupied) {
    *already_present = 1;
    return bucket->val;
  }
  DeclarationKey intern = item;
  DeclarationIndex idx = ((DeclarationIndex)(interner->list.len));
  *bucket = (DeclarationInternerMapBucket){ .key = intern, .val = idx, };
  *already_present = 0;
  decls__list_append(&interner->list, interner->arena, (DeclarationInternerExtra){ .key = intern });
  return idx;
}
b32 decls_find(DeclarationInterner *interner, DeclarationKey item, DeclarationIndex *idx) {
  DeclarationIndex *v = decls__map_find(&interner->map, item);
  if (v) {
    *idx = *v;
    return 1;
  }
  return 0;
}
DeclarationKey decls_get(DeclarationInterner *interner, DeclarationIndex idx) {
  return decls__list_at_unchecked(&interner->list, idx).key;
}
Declaration decls_get_extra(DeclarationInterner *interner, DeclarationIndex idx) {
  return decls__list_at_unchecked(&interner->list, idx).extra;
}
Declaration *decls_extra_get_ptr(DeclarationInterner *interner, DeclarationIndex idx) {
  return &decls__list_ptr_at_unchecked(&interner->list, idx)->extra;
}
void decls_set_extra(DeclarationInterner *interner, DeclarationIndex idx, Declaration val) {
  decls__list_ptr_at_unchecked(&interner->list, idx)->extra = val;
}

static void *cstd_alloc_fn(void *ctx, void *p, usize old_byte_size, usize new_byte_size, u32 align) {
  (void)(ctx); (void)(old_byte_size); (void)(align);
  if (!((p) == ((void*)0)) && new_byte_size == 0) {
    free(p);
    return ((void*)0);
  }
  return realloc(p, new_byte_size);
}
Allocator const cstd_allocator = { .fn = cstd_alloc_fn, };
static void compiler_add_message(void *user, u8 severity, SourceIndex idx, MessageLocation location, String format, ...) {
  Compiler *compiler = user;
  u32 arg_count = message_format_arg_count(format);
  Message *msg = arena_push(&compiler->arena, sizeof(Message) + arg_count * sizeof(MessageArg), _Alignof(Message));
  msg->severity = severity;
  msg->source = idx;
  msg->location = location;
  msg->format = arena_copy_string(&compiler->arena, format);
  va_list vl;
  __builtin_va_start(vl, format);
  for (u32 i = 0; i < arg_count; i++) {
    msg->args[i] = __builtin_va_arg(vl, MessageArg);
  }
  __builtin_va_end(vl);
  msglist_append(&compiler->msg_list, &compiler->arena, msg);
}
void compiler_init(Compiler *compiler) {
  __builtin___memset_chk (compiler, 0, sizeof(Compiler), __builtin_object_size (compiler, 0));
  arena_init(&compiler->arena, &(ArenaOptions){
    .reserve_size = (((u64)(16)) << 20),
    .initial_commit_size = (((u64)(1)) << 20),
  });
  arena_init(&compiler->scratch, &(ArenaOptions){
    .reserve_size = (((u64)(16)) << 20),
    .initial_commit_size = (((u64)(1)) << 20),
  });
  compiler->msg_sink = (MessageSink){
    .user = compiler,
    .add_message = compiler_add_message,
  };
  strings_init(&compiler->strings, &(InternerOptions){
    .arena = &compiler->arena,
    .map_allocator = cstd_allocator,
    .map_initial_size = 32,
  });
  types_init(&compiler->types, &(InternerOptions){
    .arena = &compiler->arena,
    .map_allocator = cstd_allocator,
    .map_initial_size = 32,
    .context = &compiler->scratch,
  });
  values_init(&compiler->values, &(ValueStoreOptions){
    .arena = &compiler->arena,
    .payload_allocator = cstd_allocator,
  });
  decls_init(&compiler->decls, &(InternerOptions){
    .arena = &compiler->arena,
    .map_allocator = cstd_allocator,
    .map_initial_size = 32,
  });
  compiler->common.type.comptime_int = types_add(&compiler->types, &(Type){ .kind = Type_comptime_int });
  compiler->common.type.nil = types_add(&compiler->types, &(Type){ .kind = Type_nil });
  compiler->common.type.type = types_add(&compiler->types, &(Type){ .kind = Type_type });
  compiler->common.type.i32 = types_add(&compiler->types, &(Type){ .kind = Type_integer, .data.integer = { .signedness = Signed, .bitwidth = 32 } });
  {
    Value *v;
    compiler->common.val.nil = values_alloc(&compiler->values, &v);
    TypeIndex *data = values_alloc_data(&compiler->values, sizeof(TypeIndex), _Alignof(TypeIndex));
    *data = compiler->common.type.nil;
    *v = (Value){
      .type = compiler->common.type.type,
      .data_size = sizeof(TypeIndex),
      .data = data,
    };
  }
  {
    Value *v;
    compiler->common.val.type = values_alloc(&compiler->values, &v);
    TypeIndex *data = values_alloc_data(&compiler->values, sizeof(TypeIndex), _Alignof(TypeIndex));
    *data = compiler->common.type.type;
    *v = (Value){
      .type = compiler->common.type.type,
      .data_size = sizeof(TypeIndex),
      .data = data,
    };
  }
  {
    Value *v;
    compiler->common.val.i32 = values_alloc(&compiler->values, &v);
    TypeIndex *data = values_alloc_data(&compiler->values, sizeof(TypeIndex), _Alignof(TypeIndex));
    *data = compiler->common.type.i32;
    *v = (Value){
      .type = compiler->common.type.type,
      .data_size = sizeof(TypeIndex),
      .data = data,
    };
  }
  decls_add(&compiler->decls, (DeclarationKey){ .parent = 4294967295U, .name = 4294967295U });
  {
    DeclarationIndex decl_i32 = decls_add(
      &compiler->decls,
      (DeclarationKey){ .parent = 0, .name = strings_add(&compiler->strings,
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpointer-sign"
                                                                             ((String){ .str = "i32", .len = (sizeof("i32") - 1), })
#pragma clang diagnostic pop
                                                                                              ) }
    );
    decls_set_extra(
      &compiler->decls,
      decl_i32,
      (Declaration){ .kind = Declaration_primitive, .data.primitive = compiler->common.val.i32 }
    );
  }
}
void compiler_deinit(Compiler *compiler) {
  arena_deinit(&compiler->arena);
  arena_deinit(&compiler->scratch);
}
void compiler_add_sourcefile(Compiler *compiler, String filename) {
  SourceIndex idx = compiler->sources.len;
  Source *source = sources_push(&compiler->sources, &compiler->arena);
  source_file_init(source, idx, filename);
}
void compiler_print_all_messages(Compiler *compiler) {
  for (u32 i = 0; i < compiler->sources.len; i++) {
    Source *source = sources_ptr_at_unchecked(&compiler->sources, i);
    source_print_all_messages(source);
  }
  u32 count = compiler->msg_list.len;
  for (u32 i = 0; i < count; i++) {
    Message *msg = msglist_at_unchecked(&compiler->msg_list, i);
    Source *source = sources_ptr_at_unchecked(&compiler->sources, msg->source);
    print_message(msg, source);
  }
}
b32 lookup_identifier(DeclarationInterner *decl_keys, DeclarationIndex *mods, u32 mod_count, StringIndex name, DeclarationIndex *out) {
  for (u32 i = 0; i < mod_count; i++) {
    DeclarationKey key = {
      .parent = mods[i],
      .name = name,
    };
    DeclarationIndex idx;
    b32 found = decls_find(decl_keys, key, &idx);
    if (found) {
      *out = idx;
      return 1;
    }
  }
  return 0;
}
typedef enum {
  Run_ok,
  Run_resolve_declaration_type,
  Run_resolve_declaration_value,
} RunResult;
typedef struct {
  Declaration *decl;
  CallStack *call_stack;
  b8 is_fresh;
  u8 min_required_resolve_status;
} ResolveEntry;
typedef struct {
  Arena *scratch;
  u32 declaration_count;
  Declaration *declarations;
  Interpreter *in;
  struct { ResolveEntry* data; u32 len; u32 cap; } resolve_stack;
} Resolver;
static CallStack alloc_callstack(Arena *arena) {
  CallStack stack;
  do { (&stack)->data = (arena_push(arena, (128) * sizeof(CallFrame), _Alignof(CallFrame))); (&stack)->len = 0; (&stack)->cap = (128); } while (0);
  return stack;
}
static void resolver_init(Resolver *resolver) {
  do { fprintf(__stderrp, "TODO -> "); do { fprintf(__stderrp, "panic in %s at %s:%u\n", __func__, "src/compiler.c", 250); abort(); } while (0); } while (0);
}
static b32 resolve_run_entry_until(Resolver *resolver, ResolveEntry *entry, u32 end) {
  u32 err = run_until(resolver->in, entry->call_stack, end);
  if (err == Run_resolve_declaration_type || err == Run_resolve_declaration_value) {
    CallFrame *f = (&((entry->call_stack)->data[(entry->call_stack)->len-1]));
    DeclarationIndex idx = chunk_data(f->chunk, f->pc);
    Declaration *decl = &resolver->declarations[idx];
    CallStack call_stack = alloc_callstack(resolver->scratch);
    frame_push(&call_stack, resolver->scratch, decl->data.decl.chunk);
    u8 min_required_resolve_status;
    switch (err) {
    case Run_resolve_declaration_type: min_required_resolve_status = ResolveStatus_type_resolved; break;
    case Run_resolve_declaration_value: min_required_resolve_status = ResolveStatus_fully_resolved; break;
    default: __builtin_unreachable();
    }
    do { if ((&resolver->resolve_stack)->len == (&resolver->resolve_stack)->cap) { do { fprintf(__stderrp, "panic in %s at %s:%u\n", __func__, "src/compiler.c", 280); abort(); } while (0); } ((&resolver->resolve_stack)->data[(&resolver->resolve_stack)->len++] = (((ResolveEntry){ .decl = decl, .call_stack = call_stack, .is_fresh = 1, .min_required_resolve_status = min_required_resolve_status, }))); } while (0);
    entry->is_fresh = 0;
    return 1;
  }
  return 0;
}
b32 resolve_declarations(Resolver *resolver) {
  for (u32 i = 0; i < resolver->declaration_count; i++) {
    {
      Declaration *decl = &resolver->declarations[i];
      u8 resolve_status = decl->resolve_status;
      if (resolve_status == ResolveStatus_fully_resolved) {
        continue;
      }
      (__builtin_expect(!(resolve_status == ResolveStatus_unresolved || resolve_status == ResolveStatus_type_resolved), 0) ? __assert_rtn(__func__, "compiler.c", 301, "resolve_status == ResolveStatus_unresolved || resolve_status == ResolveStatus_type_resolved") : (void)0);
      CallStack call_stack = alloc_callstack(resolver->scratch);
      frame_push(&call_stack, resolver->scratch, decl->data.decl.chunk);
      (__builtin_expect(!((&resolver->resolve_stack)->len < (&resolver->resolve_stack)->cap), 0) ? __assert_rtn(__func__, "compiler.c", 314, "(&resolver->resolve_stack)->len < (&resolver->resolve_stack)->cap") : (void)0), do { if (((&resolver->resolve_stack))->len == ((&resolver->resolve_stack))->cap) { do { fprintf(__stderrp, "panic in %s at %s:%u\n", __func__, "src/compiler.c", 314); abort(); } while (0); } (((&resolver->resolve_stack))->data[((&resolver->resolve_stack))->len++] = ((((ResolveEntry){ .decl = decl, .call_stack = call_stack, .is_fresh = 1, .min_required_resolve_status = min_required_resolve_status, })))); } while (0);
    }
    while (!((&resolver->resolve_stack)->len == 0)) {
      ResolveEntry *entry = (&((&resolver->resolve_stack)->data[(&resolver->resolve_stack)->len-1]));
      Declaration *decl = entry->decl;
      CallStack *call_stack = entry->call_stack;
      u8 resolve_status = decl->resolve_status;
      if (resolve_status >= entry->min_required_resolve_status) {
        ((&resolver->resolve_stack)->data[--(&resolver->resolve_stack)->len]);
        continue;
      }
      if (entry->is_fresh && resolve_status == ResolveStatus_resolving_type) {
        do { fprintf(__stderrp, "panic in %s at %s:%u\n", __func__, "src/compiler.c", 330); abort(); } while (0);
      }
      if (entry->is_fresh && resolve_status == ResolveStatus_resolving_value) {
        do { fprintf(__stderrp, "panic in %s at %s:%u\n", __func__, "src/compiler.c", 334); abort(); } while (0);
      }
      entry->is_fresh = 0;
      if (resolve_status < ResolveStatus_type_resolved) {
        decl->resolve_status = ResolveStatus_resolving_type;
        b32 has_pushed = resolve_run_entry_until(resolver, entry, decl->data.decl.typecheck_end);
        if (has_pushed) {
          continue;
        }
        decl->resolve_status = ResolveStatus_type_resolved;
      }
      if (resolve_status < ResolveStatus_fully_resolved) {
        decl->resolve_status = ResolveStatus_resolving_value;
        b32 has_pushed = resolve_run_entry_until(resolver, entry, decl->data.decl.chunk->opcode_count);
        if (has_pushed) {
          continue;
        }
        decl->resolve_status = ResolveStatus_fully_resolved;
        ((&resolver->resolve_stack)->data[--(&resolver->resolve_stack)->len]);
      }
    }
  }
}
typedef struct {
  DeclarationIndex mod;
  u32 n;
} DeclFrame;
b32 compile(Compiler *compiler) {
  b32 is_ok = 1;
  for (u32 i = 0; i < compiler->sources.len; i++) {
    Source *source = sources_ptr_at_unchecked(&compiler->sources, i);
    b32 ok;
    ok = source_read_file(source);
    if (!ok) { is_ok = 0; source->status = SourceStatus_failed_to_parse; continue; }
    ok = source_tokenize(source, &compiler->scratch);
    if (!ok) { is_ok = 0; source->status = SourceStatus_failed_to_parse; continue; }
    ok = source_parse(source, &compiler->scratch);
    if (!ok) { is_ok = 0; source->status = SourceStatus_failed_to_parse; continue; }
    source_index_declarations(source, &compiler->strings);
    source->status = SourceStatus_parsed;
  }
  if (!is_ok) {
    return 0;
  }
  for (u32 i = 0; i < compiler->sources.len; i++) {
    Source *source = sources_ptr_at_unchecked(&compiler->sources, i);
    DeclFrame stackmem[4];
    struct { DeclFrame* data; u32 len; u32 cap; } stack;
    do { (&stack)->data = (stackmem); (&stack)->len = 0; (&stack)->cap = (4); } while (0);
    (__builtin_expect(!((&stack)->len < (&stack)->cap), 0) ? __assert_rtn(__func__, "compiler.c", 405, "(&stack)->len < (&stack)->cap") : (void)0), do { if (((&stack))->len == ((&stack))->cap) { do { fprintf(__stderrp, "panic in %s at %s:%u\n", __func__, "src/compiler.c", 405); abort(); } while (0); } (((&stack))->data[((&stack))->len++] = ((((DeclFrame){ .mod = 0, .n = source->decls[0].child_count })))); } while (0);
    source->decl_idxs[0] = 0;
    u32 offset = 1;
    while (!((&stack)->len == 0)) {
      DeclFrame *top = (&((&stack)->data[(&stack)->len-1]));
      if (top->n == 0) {
        ((&stack)->data[--(&stack)->len]);
        continue;
      }
      top->n -= 1;
      SourceDeclaration const *decl = &source->decls[offset];
      if (decl->kind == SourceDeclaration_mod) {
        b32 already_exists;
        DeclarationIndex idx = decls_add_checked(
          &compiler->decls,
          (DeclarationKey){ .parent = top->mod, .name = decl->name },
          &already_exists
        );
        if (already_exists) {
          Declaration val = decls_get_extra(&compiler->decls, idx);
          if (val.kind != Declaration_mod) {
            is_ok = 0;
            (&compiler->msg_sink)->add_message((&compiler->msg_sink)->user, Severity_Error, source->idx, (MessageLocation){ .kind = MessageLocation_ast_index, .data.ast_index = decl->node },
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpointer-sign"
            ((String){ .str = "Declaration already exists.", .len = (sizeof("Declaration already exists.") - 1), })
#pragma clang diagnostic pop
            );
            goto next_iter;
          }
        }
        (__builtin_expect(!((&stack)->len < (&stack)->cap), 0) ? __assert_rtn(__func__, "compiler.c", 445, "(&stack)->len < (&stack)->cap") : (void)0), do { if (((&stack))->len == ((&stack))->cap) { do { fprintf(__stderrp, "panic in %s at %s:%u\n", __func__, "src/compiler.c", 445); abort(); } while (0); } (((&stack))->data[((&stack))->len++] = ((((DeclFrame){ .mod = idx, .n = decl->child_count })))); } while (0);
        source->decl_idxs[offset] = idx;
        decls_set_extra(
          &compiler->decls,
          idx,
          (Declaration){ .kind = Declaration_mod, .data.loc = { .source = source->idx, .source_decl_idx = offset }}
        );
      } else if (decl->kind == SourceDeclaration_declaration) {
        b32 already_exists;
        DeclarationIndex idx = decls_add_checked(
          &compiler->decls,
          (DeclarationKey){ .parent = top->mod, .name = decl->name },
          &already_exists
        );
        if (already_exists) {
          is_ok = 0;
          (&compiler->msg_sink)->add_message((&compiler->msg_sink)->user, Severity_Error, source->idx, (MessageLocation){ .kind = MessageLocation_ast_index, .data.ast_index = decl->node },
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpointer-sign"
          ((String){ .str = "Declaration already exists.", .len = (sizeof("Declaration already exists.") - 1), })
#pragma clang diagnostic pop
          );
          goto next_iter;
        }
        source->decl_idxs[offset] = idx;
        decls_set_extra(&compiler->decls, idx, (Declaration){
          .kind = Declaration_decl,
          .data.loc = { .source = source->idx, .source_decl_idx = offset },
        });
      }
next_iter:
      offset += 1;
    }
  }
  {
    CodeGenContext context = {
      .arena = &compiler->arena,
      .scratch = &compiler->scratch,
      .common = &compiler->common,
      .msg_sink = &compiler->msg_sink,
      .strings = &compiler->strings,
      .decls = &compiler->decls,
      .values = &compiler->values,
    };
    for (u32 i = 0; i < compiler->sources.len; i++) {
      Source *source = sources_ptr_at_unchecked(&compiler->sources, i);
      for (u32 j = 0; j < source->decl_count; j++) {
        is_ok &= source_generate_code(&context, source, j);
        ir_chunk_print(__stdoutp, &source->ir_chunks[j], &compiler->types, &compiler->values);
      }
    }
  }
  {
    for (u32 i = 0; i < compiler->sources.len; i++) {
      Source *source = sources_ptr_at_unchecked(&compiler->sources, i);
      InterpretContext context = {
        .perm = &source->arena,
        .scratch = &compiler->scratch,
        .common = &compiler->common,
        .msg_sink = &compiler->msg_sink,
        .decls = &compiler->decls,
        .values = &compiler->values,
        .types = &compiler->types,
      };
      for (u32 j = 0; j < source->decl_count; j++) {
        is_ok &= source_interpret_declaration(&context, source, j);
        ir_chunk_print(__stdoutp, &source->runtime_chunks[j], &compiler->types, &compiler->values);
      }
    }
  }
  return is_ok;
}
