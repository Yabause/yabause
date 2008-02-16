// xsw_ctype.h
// This is intended as a prototype for files using the global/ctype.cpp file.

#if !defined(__FreeBSD__) && !defined(__NetBSD__)
# if defined(__cplusplus) || defined(c_plusplus)

/* Most systems should have this defined.
#ifndef isblank
extern bool isblank(int c);
#endif
 */

#else

/* Most systems should have this defined.

#ifndef isblank
extern int isblank( int );
#endif
 */

# endif	/* __cplusplus || c_plusplus */
#endif	/* __FreeBSD__ */


extern void ctype_dummy_func();
