/* src/conf.h.  Generated automatically by configure.  */
/* src/conf.h.in.  Generated automatically from configure.in by autoheader.  */

#ifndef _CONF_H_
#define _CONF_H_

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if we're compiling CircleMUD under any type of UNIX system.  */
#define CIRCLE_UNIX 1

/* Define if the system is capable of using crypt() to encrypt.  */
#define CIRCLE_CRYPT 1

/* Define if we don't have proper support for the system's crypt().  */
/* #undef HAVE_UNSAFE_CRYPT */

/* Define is the system has struct in_addr.  */
#define HAVE_STRUCT_IN_ADDR 1

/* Define to `int' if <sys/socket.h> doesn't define.  */
/* #undef socklen_t */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef ssize_t */

/* Define if you have the gettimeofday function.  */
#define HAVE_GETTIMEOFDAY 1

/* Define if you have the inet_addr function.  */
#define HAVE_INET_ADDR 1

/* Define if you have the inet_aton function.  */
#define HAVE_INET_ATON 1

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the snprintf function.  */
#define HAVE_SNPRINTF 1

/* Define if you have the strcasecmp function.  */
#define HAVE_STRCASECMP 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the stricmp function.  */
/* #undef HAVE_STRICMP */

/* Define if you have the strlcpy function.  */
#define HAVE_STRLCPY 1

/* Define if you have the strncasecmp function.  */
#define HAVE_STRNCASECMP 1

/* Define if you have the strnicmp function.  */
/* #undef HAVE_STRNICMP */

/* Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/* Define if you have the vsnprintf function.  */
#define HAVE_VSNPRINTF 1

/* Define if you have the <arpa/inet.h> header file.  */
#define HAVE_ARPA_INET_H 1

/* Define if you have the <arpa/telnet.h> header file.  */
#define HAVE_ARPA_TELNET_H 1

/* Define if you have the <assert.h> header file.  */
#define HAVE_ASSERT_H 1

/* Define if you have the <crypt.h> header file.  */
#define HAVE_CRYPT_H 1

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <mcheck.h> header file.  */
#define HAVE_MCHECK_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <net/errno.h> header file.  */
/* #undef HAVE_NET_ERRNO_H */

/* Define if you have the <netdb.h> header file.  */
#define HAVE_NETDB_H 1

/* Define if you have the <netinet/in.h> header file.  */
#define HAVE_NETINET_IN_H 1

/* Define if you have the <signal.h> header file.  */
#define HAVE_SIGNAL_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H 1

/* Define if you have the <sys/fcntl.h> header file.  */
#define HAVE_SYS_FCNTL_H 1

/* Define if you have the <sys/resource.h> header file.  */
#define HAVE_SYS_RESOURCE_H 1

/* Define if you have the <sys/select.h> header file.  */
#define HAVE_SYS_SELECT_H 1

/* Define if you have the <sys/socket.h> header file.  */
#define HAVE_SYS_SOCKET_H 1

/* Define if you have the <sys/stat.h> header file.  */
#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/types.h> header file.  */
#define HAVE_SYS_TYPES_H 1

/* Define if you have the <sys/uio.h> header file.  */
#define HAVE_SYS_UIO_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the malloc library (-lmalloc).  */
/* #undef HAVE_LIBMALLOC */

/* Check for a prototype to accept. */
/* #undef NEED_ACCEPT_PROTO */

/* Check for a prototype to atoi. */
/* #undef NEED_ATOI_PROTO */

/* Check for a prototype to atol. */
/* #undef NEED_ATOL_PROTO */

/* Check for a prototype to bind. */
/* #undef NEED_BIND_PROTO */

/* Check for a prototype to bzero. */
/* #undef NEED_BZERO_PROTO */

/* Check for a prototype to chdir. */
/* #undef NEED_CHDIR_PROTO */

/* Check for a prototype to close. */
/* #undef NEED_CLOSE_PROTO */

/* Check for a prototype to crypt. */
/* #undef NEED_CRYPT_PROTO */

/* Check for a prototype to fclose. */
/* #undef NEED_FCLOSE_PROTO */

/* Check for a prototype to fcntl. */
/* #undef NEED_FCNTL_PROTO */

/* Check for a prototype to fflush. */
/* #undef NEED_FFLUSH_PROTO */

/* Check for a prototype to fprintf. */
/* #undef NEED_FPRINTF_PROTO */

/* Check for a prototype to fputc. */
/* #undef NEED_FPUTC_PROTO */

/* Check for a prototype to fputs. */
/* #undef NEED_FPUTS_PROTO */

/* Check for a prototype to fread. */
/* #undef NEED_FREAD_PROTO */

/* Check for a prototype to fscanf. */
/* #undef NEED_FSCANF_PROTO */

/* Check for a prototype to fseek. */
/* #undef NEED_FSEEK_PROTO */

/* Check for a prototype to fwrite. */
/* #undef NEED_FWRITE_PROTO */

/* Check for a prototype to getpeername. */
/* #undef NEED_GETPEERNAME_PROTO */

/* Check for a prototype to getpid. */
/* #undef NEED_GETPID_PROTO */

/* Check for a prototype to getrlimit. */
/* #undef NEED_GETRLIMIT_PROTO */

/* Check for a prototype to getsockname. */
/* #undef NEED_GETSOCKNAME_PROTO */

/* Check for a prototype to gettimeofday. */
/* #undef NEED_GETTIMEOFDAY_PROTO */

/* Check for a prototype to htonl. */
/* #undef NEED_HTONL_PROTO */

/* Check for a prototype to htons. */
/* #undef NEED_HTONS_PROTO */

/* Check for a prototype to inet_addr. */
/* #undef NEED_INET_ADDR_PROTO */

/* Check for a prototype to inet_aton. */
/* #undef NEED_INET_ATON_PROTO */

/* Check for a prototype to inet_ntoa. */
/* #undef NEED_INET_NTOA_PROTO */

/* Check for a prototype to listen. */
/* #undef NEED_LISTEN_PROTO */

/* Check for a prototype to ntohl. */
/* #undef NEED_NTOHL_PROTO */

/* Check for a prototype to perror. */
/* #undef NEED_PERROR_PROTO */

/* Check for a prototype to printf. */
/* #undef NEED_PRINTF_PROTO */

/* Check for a prototype to qsort. */
/* #undef NEED_QSORT_PROTO */

/* Check for a prototype to read. */
/* #undef NEED_READ_PROTO */

/* Check for a prototype to remove. */
/* #undef NEED_REMOVE_PROTO */

/* Check for a prototype to rewind. */
/* #undef NEED_REWIND_PROTO */

/* Check for a prototype to select. */
/* #undef NEED_SELECT_PROTO */

/* Check for a prototype to setitimer. */
/* #undef NEED_SETITIMER_PROTO */

/* Check for a prototype to setrlimit. */
/* #undef NEED_SETRLIMIT_PROTO */

/* Check for a prototype to setsockopt. */
/* #undef NEED_SETSOCKOPT_PROTO */

/* Check for a prototype to snprintf. */
/* #undef NEED_SNPRINTF_PROTO */

/* Check for a prototype to socket. */
/* #undef NEED_SOCKET_PROTO */

/* Check for a prototype to sprintf. */
/* #undef NEED_SPRINTF_PROTO */

/* Check for a prototype to sscanf. */
/* #undef NEED_SSCANF_PROTO */

/* Check for a prototype to strcasecmp. */
/* #undef NEED_STRCASECMP_PROTO */

/* Check for a prototype to strdup. */
/* #undef NEED_STRDUP_PROTO */

/* Check for a prototype to strerror. */
/* #undef NEED_STRERROR_PROTO */

/* Check for a prototype to stricmp. */
#define NEED_STRICMP_PROTO

/* Check for a prototype to strlcpy. */
/* #undef NEED_STRLCPY_PROTO */

/* Check for a prototype to strncasecmp. */
/* #undef NEED_STRNCASECMP_PROTO */

/* Check for a prototype to strnicmp. */
#define NEED_STRNICMP_PROTO

/* Check for a prototype to system. */
/* #undef NEED_SYSTEM_PROTO */

/* Check for a prototype to time. */
/* #undef NEED_TIME_PROTO */

/* Check for a prototype to unlink. */
/* #undef NEED_UNLINK_PROTO */

/* Check for a prototype to vsnprintf. */
/* #undef NEED_VSNPRINTF_PROTO */

/* Check for a prototype to write. */
/* #undef NEED_WRITE_PROTO */

#endif /* _CONF_H_ */
