// Hey, Emacs, this a -*-C++-*- file !

// Copyright distributed.net 1997-1998 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.
// 
// $Log: cputypes.h,v $
// Revision 1.27  1998/07/16 20:18:52  nordquist
// DYNIX port changes.
//
// Revision 1.26  1998/07/15 05:50:33  ziggyb
// removed the need for a fake bool when I upgraded my version of Watcom to version 11
//
// Revision 1.25  1998/07/01 09:06:36  daa
// add HPUX_M68
//
// Revision 1.24  1998/06/29 10:42:13  jlawson
// swapped OS_WIN32S and OS_WIN16 values, since Win32s clients were
// previously classified as win16
//
// Revision 1.23  1998/06/29 07:59:42  ziggyb
// Need Fake Bool on my older version of Watcom
//
// Revision 1.22  1998/06/29 06:58:00  jlawson
// added new platform OS_WIN32S to make code handling easier.
//
// Revision 1.21  1998/06/22 09:25:18  cyruspatel
// Added __WATCOMC__ check for bool support. 'true' was incorrectly defined
// as (1). Changed to be (!false). As of this writing, this should have no
// impact on the anything (I checked).
//
// Revision 1.20  1998/06/17 00:29:45  snake
//
//
// #ifdefs for OpenBSD on Sparc were in the Linux section instead of the
// OpenBSD section of 'configure'. Fixed by moving it to the right position.
//
// Revision 1.19  1998/06/15 00:13:09  skand
// define NetBSD/alpha
//
// Revision 1.18  1998/06/13 21:46:52  friedbait
// 'Log' keyword added such that we can easily track the change history
//
// 

#if ( !defined(_CPU_32BIT_) && !defined(_CPU_64BIT_) )
#define _CPU_32BIT_
#endif

#ifndef _CPUTYPES_H_
#define _CPUTYPES_H_


#if !defined(INTSIZES)
#define INTSIZES 442
#endif

#if (INTSIZES == 422)       // (16-bit DOS/WIN):  long=32, int=16, short=16
  typedef unsigned long u32;
  typedef signed long s32;
  typedef unsigned short u16;
  typedef signed short s16;
#elif (INTSIZES == 442)     // (typical):  long=32, int=32, short=16
  typedef unsigned long u32;
  typedef signed long s32;
  typedef unsigned short u16;
  typedef signed short s16;
#elif (INTSIZES == 842)     // (primarily Alphas):  long=64, int=32, short=16
  typedef unsigned int u32;
  typedef signed int s32;
  typedef unsigned short u16;
  typedef signed short s16;
#else
  #error "Invalid INTSIZES"
#endif

typedef unsigned char u8;
typedef signed char s8;
typedef double f64;
typedef float f32;

struct fake_u64 { u32 hi, lo; };
struct fake_s64 { s32 hi, lo; };

typedef struct fake_u64 u64;
typedef struct fake_s64 s64;

struct u128 { u64 hi, lo; };
struct s128 { s64 hi, lo; };

// Major CPU architectures, we don't need (or want) very fine resolution
#define CPU_UNKNOWN     0
#define CPU_X86         1
#define CPU_POWERPC     2
#define CPU_MIPS        3
#define CPU_ALPHA       4
#define CPU_PA_RISC     5
#define CPU_68K         6
#define CPU_SPARC       7
#define CPU_JAVA_VM     8
#define CPU_POWER       9
#define CPU_VAX         10
#define CPU_ARM         11
#define CPU_88K         12
#define CPU_KSR1        13
#define CPU_S390        14
#define CPU_MASPAR	15

// Major OS Architectures.
#define OS_UNKNOWN      0
#define OS_WIN32        1  // win95 + win98 + winnt
#define OS_DOS          2  // ms-dos, pc-dos, dr-dos, etc.
#define OS_FREEBSD      3
#define OS_LINUX        4
#define OS_BEOS         5
#define OS_MACOS        6
#define OS_IRIX         7
#define OS_VMS          8
#define OS_DEC_UNIX     9
#define OS_UNIXWARE     10
#define OS_OS2          11
#define OS_HPUX         12
#define OS_NETBSD       13
#define OS_SUNOS        14
#define OS_SOLARIS      15
#define OS_OS9          16
#define OS_JAVA_VM      17
#define OS_BSDI         18
#define OS_NEXTSTEP     19
#define OS_SCO          20
#define OS_QNX          21
#define OS_OSF1         22    // oldname for DEC UNIX
#define OS_MINIX        23
#define OS_MACH10       24
#define OS_AIX          25
#define OS_AUX          26
#define OS_RHAPSODY     27
#define OS_AMIGAOS      28
#define OS_OPENBSD      29
#define OS_NETWARE      30
#define OS_MVS          31
#define OS_ULTRIX       32
#define OS_NEWTON       33
#define OS_RISCOS       34
#define OS_DGUX         35
#define OS_WIN32S       36    // windows 3.1, 3.11, wfw (32-bit Win32s)
#define OS_SINIX        37
#define OS_DYNIX        38
#define OS_OS390        39
#define OS_MASPAR       40
#define OS_WIN16        41    // windows 3.1, 3.11, wfw (16-bit)

// determine current compiling platform
#if defined(WIN32) || defined(__WIN32__) || defined(_Windows) || defined(_WIN32)
  #if defined(NTALPHA)
    #define CLIENT_OS     OS_WIN32
    #define CLIENT_CPU    CPU_ALPHA
  #elif defined(ASM_PPC)
    #define CLIENT_OS     OS_WIN32
    #define CLIENT_CPU    CPU_POWERPC
  #elif !defined(WIN32) && !defined(__WIN32__) && !defined(_WIN32)
    // win16 gui
    #define CLIENT_OS     OS_WIN16
    #define CLIENT_CPU    CPU_X86
  #elif defined(NOMAIN) && !defined(MULTITHREAD)
    // win32s gui
    #define CLIENT_OS     OS_WIN32S
    #define CLIENT_CPU    CPU_X86
  #elif defined(_M_IX86)
    #define CLIENT_OS     OS_WIN32
    #define CLIENT_CPU    CPU_X86
  #endif
#elif defined(DJGPP) || defined(DOS4G) || defined(__MSDOS__)
  #define CLIENT_OS     OS_DOS
  #define CLIENT_CPU    CPU_X86
#elif defined(__NETWARE__)
  #if defined(_M_IX86)
    #define CLIENT_OS     OS_NETWARE
    #define CLIENT_CPU    CPU_X86
  #elif defined(_M_SPARC)
    #define CLIENT_OS     OS_NETWARE
    #define CLIENT_CPU    CPU_SPARC
  #elif defined(_M_ALPHA)
    #define CLIENT_OS     OS_NETWARE
    #define CLIENT_CPU    CPU_ALPHA
  #endif
#elif defined(__OS2__)
  #define CLIENT_OS     OS_OS2
  #define CLIENT_CPU    CPU_X86
#elif defined(linux)
  #if defined(ASM_ALPHA)
    #define CLIENT_OS     OS_LINUX
    #define CLIENT_CPU    CPU_ALPHA
  #elif defined(ASM_X86)
    #define CLIENT_OS     OS_LINUX
    #define CLIENT_CPU    CPU_X86
  #elif defined(ARM)
    #define CLIENT_OS     OS_LINUX
    #define CLIENT_CPU    CPU_ARM
  #elif defined(ASM_SPARC) || defined(SPARCLINUX)
    #define CLIENT_OS     OS_LINUX
    #define CLIENT_CPU    CPU_SPARC
  #elif defined(ASM_PPC)
    #define CLIENT_OS     OS_LINUX
    #define CLIENT_CPU    CPU_POWERPC
  #elif defined(ASM_68K)
    #define CLIENT_OS     OS_LINUX
    #define CLIENT_CPU    CPU_68K
  #endif
#elif defined(__FreeBSD__)
  #if defined(ASM_X86)
    #define CLIENT_OS     OS_FREEBSD
    #define CLIENT_CPU    CPU_X86
  #endif
#elif defined(__NetBSD__)
  #if defined(ASM_X86)
    #define CLIENT_OS     OS_NETBSD
    #define CLIENT_CPU    CPU_X86
  #elif defined(ARM)
    #define CLIENT_OS     OS_NETBSD
    #define CLIENT_CPU    CPU_ARM
  #elif defined(ASM_ALPHA)
    #define CLIENT_OS     OS_NETBSD
    #define CLIENT_CPU    CPU_ALPHA
  #endif
#elif defined(__OpenBSD__) || defined(openbsd)
  #if defined(ASM_X86)
    #define CLIENT_OS     OS_OPENBSD
    #define CLIENT_CPU    CPU_X86
  #elif defined(ASM_ALPHA)
    #define CLIENT_OS     OS_OPENBSD
    #define CLIENT_CPU    CPU_ALPHA
  #elif defined(ASM_SPARC)
    #define CLIENT_OS     OS_OPENBSD
    #define CLIENT_CPU    CPU_SPARC
  #endif
#elif defined(__QNX__)
  #if defined(ASM_X86)
    #define CLIENT_OS     OS_QNX
    #define CLIENT_CPU    CPU_X86
  #endif
#elif defined(solaris)
  #if defined(ASM_X86)
    #define CLIENT_OS     OS_SOLARIS
    #define CLIENT_CPU    CPU_X86
  #elif defined(ASM_SPARC)
    #define CLIENT_OS     OS_SOLARIS
    #define CLIENT_CPU    CPU_SPARC
  #endif
#elif defined(_SUN68K_)
  #define CLIENT_OS         OS_SUNOS
  #define CLIENT_CPU        CPU_68K
#elif defined(bsdi)
  #if defined(ASM_X86)
    #define CLIENT_OS     OS_BSDI
    #define CLIENT_CPU    CPU_X86
  #endif
#elif defined(sco5)
  #if defined(ASM_X86)
    #define CLIENT_OS     OS_SCO
    #define CLIENT_CPU    CPU_X86
  #endif
#elif defined(__osf__)
  #if defined(__alpha)
    #define CLIENT_OS     OS_DEC_UNIX
    #define CLIENT_CPU    CPU_ALPHA
  #endif
#elif defined(sinix)
  #if defined(ASM_MIPS) || defined(__mips)
    #define CLIENT_OS     OS_SINIX
    #define CLIENT_CPU    CPU_MIPS
  #endif
#elif (defined(ASM_MIPS) || defined(__mips)) && !defined(sinix)
  #define CLIENT_OS     OS_IRIX
  #define CLIENT_CPU    CPU_MIPS
#elif defined(__VMS)
  #if defined(__ALPHA)
    #define CLIENT_OS     OS_VMS
    #define CLIENT_CPU    CPU_ALPHA
  #endif
  #if !defined(__VMS_UCX__) && !defined(NONETWORK) && !defined(MULTINET)
    #define MULTINET 1
  #endif
#elif defined(_HPUX)
  #if defined(ASM_HPPA)
    #define CLIENT_OS     OS_HPUX
    #define CLIENT_CPU    CPU_PA_RISC
  #endif
#elif defined(_HPUX_M68K)
  #define CLIENT_OS     OS_HPUX
  #define CLIENT_CPU    CPU_68K
#elif defined(_DGUX)
  #define CLIENT_OS     OS_DGUX
  #define CLIENT_CPU    CPU_88K
  #define PTHREAD_SCOPE_SYSTEM PTHREAD_SCOPE_GLOBAL
  #define pthread_sigmask(a,b,c)
#elif defined(_AIX)
  #if defined(_ARCH_PPC)
    #define CLIENT_OS     OS_AIX
    #define CLIENT_CPU    CPU_POWERPC
  #endif
#elif defined(macintosh)
  #if GENERATINGPOWERPC
    #define CLIENT_OS     OS_MACOS
    #define CLIENT_CPU    CPU_POWERPC
  #elif GENERATING68K
    #define CLIENT_OS     OS_MACOS
    #define CLIENT_CPU    CPU_68K
  #endif
#elif defined(__dest_os) && defined(__be_os) && (__dest_os == __be_os)
  #if defined(__POWERPC__)
    #define CLIENT_OS     OS_BEOS
    #define CLIENT_CPU    CPU_POWERPC
  #elif defined(__INTEL__)
    #define CLIENT_OS     OS_BEOS
    #define CLIENT_CPU    CPU_X86
  #endif
#elif defined(AMIGA)
  #define CLIENT_OS     OS_AMIGAOS
  #ifdef __PPC__
    #define CLIENT_CPU    CPU_POWERPC
  #else
    #define CLIENT_CPU    CPU_68K
  #endif
#elif defined(__riscos)
  #define CLIENT_OS     OS_RISCOS
  #define CLIENT_CPU    CPU_ARM
#elif defined(_NeXT_)
  #if defined(ASM_X86)
    #define CLIENT_OS     OS_NEXTSTEP
    #define CLIENT_CPU    CPU_X86
  #elif defined(ASM_68K)
    #define CLIENT_OS     OS_NEXTSTEP
    #define CLIENT_CPU    CPU_68K
  #elif defined(ASM_HPPA)
    #define CLIENT_OS     OS_NEXTSTEP
    #define CLIENT_CPU    CPU_PA_RISC
  #elif defined(ASM_SPARC)
    #define CLIENT_OS     OS_NEXTSTEP
    #define CLIENT_CPU    CPU_SPARC
  #endif
#elif defined(__MVS__)
  #define CLIENT_OS     OS_OS390
  #define CLIENT_CPU    CPU_S390
#elif defined(_SEQUENT_)
  #if defined(ASM_X86)
    #define CLIENT_OS     OS_DYNIX
    #define CLIENT_CPU    CPU_X86
  #else
    #define CLIENT_OS     OS_DYNIX
    #define CLIENT_CPU    CPU_UNKNOWN
    #define IGNOREUNKNOWNCPUOS
  #endif
#endif

#if !defined(CLIENT_OS) || !defined(CLIENT_CPU)
  #define CLIENT_OS     OS_UNKNOWN
  #define CLIENT_CPU    CPU_UNKNOWN
#endif
#if (CLIENT_OS == OS_UNKNOWN) || (CLIENT_CPU == CPU_UNKNOWN)
  #if !defined(IGNOREUNKNOWNCPUOS)
    #error "Unknown CPU/OS detected in cputypes.h"
  #endif
#endif

// Some compilers/platforms don't yet support bool internally.
// When creating new rules here, please try to use compiler-specific macro tests
// since not all compilers on a specific platform (or even a newer version of
// your own compiler) may be missing bool.
//
#if defined(__VMS) || defined(__SUNPRO_CC) || defined(__DECCXX) || defined(__MVS__)
  #define NEED_FAKE_BOOL
#elif defined(_HPUX) || defined(_OLD_NEXT_)
  #define NEED_FAKE_BOOL
#elif defined(__WATCOMC__)
  //nothing - bool is defined
#elif defined(__xlc) || defined(__xlC) || defined(__xlC__) || defined(__XLC121__)
  #define NEED_FAKE_BOOL
#elif (defined(__mips) && __mips < 3 && !defined(__GNUC__))
  #define NEED_FAKE_BOOL
#elif (defined(__TURBOC__) && __TURBOC__ <= 0x400)
  #define NEED_FAKE_BOOL
#elif (defined(_MSC_VER) && _MSC_VER < 1100)
  #define NEED_FAKE_BOOL
#elif (defined(_SEQUENT_) && !defined(__GNUC__))
  #define NEED_FAKE_BOOL
#endif

#if defined(NEED_FAKE_BOOL)
    typedef char bool;
    #define true (!0)
    #define false (0)
#endif


#endif

