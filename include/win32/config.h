/* include/sphinx_config.h, defaults for Win32 */
/* sphinx_config.h: Externally visible configuration parameters for
 * SphinxBase.
 */

/* Default radix point for fixed-point */
/* #undef DEFAULT_RADIX */

/* Enable thread safety */
#define ENABLE_THREADS 

/* Use fixed-point computation */
/* #undef FIXED_POINT */

/* Enable matrix algebra with LAPACK */
#define WITH_LAPACK 1

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* We don't have popen, but we do have _popen */
/* #define HAVE_POPEN 1 */

/* We do have perror */
#define HAVE_PERROR 1

/* We have sys/stat.h */
#define HAVE_SYS_STAT_H 1

/* Extension for executables */
#define EXEEXT ".exe"
