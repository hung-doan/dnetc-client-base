/*
 * Copyright distributed.net 1999-2008 - All Rights Reserved
 * For use in distributed.net projects only.
 * Any other distribution or use of this source violates copyright.
 *
 * $Id: ogr-ng.cpp,v 1.1 2008/02/10 00:07:41 kakace Exp $
 */
#include <string.h>   /* memset */

/* #define OGR_DEBUG           */ /* turn on miscellaneous debug information */ 
/* #define OGR_TEST_FIRSTBLANK */ /* test firstblank logic (table or asm)    */

#if defined(OGR_DEBUG) || defined(OGR_TEST_FIRSTBLANK)
#include <stdio.h>  /* printf for debugging */
#endif

/* --- various optimization option overrides ----------------------------- */

#if defined(NO_OGR_OPTIMIZATION) || defined(GIMME_BASELINE_OGR_CPP)
  /*
  ** baseline/reference == ogr.cpp without optimization == ~old ogr.cpp 
  */
  #define OGROPT_HAVE_FIND_FIRST_ZERO_BIT_ASM   0 /* 0-2 - default is hw dependant */
  #define OGROPT_CYCLE_CACHE_ALIGN              0 /* 0/1 - default is 0 ('no')  */
  #define OGROPT_ALTERNATE_CYCLE                0 /* 0/1 - default is 0 ('no')  */
  #define OGROPT_ALTERNATE_COMP_LEFT_LIST_RIGHT 0 /* 0/1 - default is 0         */
#endif

/* -- various optimization option defaults ------------------------------- */

/* ogr_cycle() method selection.
   OGROPT_ALTERNATE_CYCLE == 0 - Default (GARSP) ogr_cycle().
   OGROPT_ALTERNATE_CYCLE == 1 - Fully customized ogr_cycle(). Implementors must
                                 provide all necessary macros.
*/
#ifndef OGROPT_ALTERNATE_CYCLE
#define OGROPT_ALTERNATE_CYCLE 0    /* 0 (FLEGE) or 1 */
#endif


/* These are alternatives for the COMP_LEFT_LIST_RIGHT macro to "shift the
   list to add or extend the first mark".

   OGROPT_ALTERNATE_COMP_LEFT_LIST_RIGHT == 0 -> default COMP_LEFT_LIST_RIGHT
   OGROPT_ALTERNATE_COMP_LEFT_LIST_RIGHT == 1 -> Implementors may provide
      platform-specific or assembly versions of the bitmap manipulation macros
      that correspond to the OGROPT_ALTERNATE_CYCLE setting. Missing macros
      are still defined to reasonable defaults.
*/
#ifndef OGROPT_ALTERNATE_COMP_LEFT_LIST_RIGHT
#define OGROPT_ALTERNATE_COMP_LEFT_LIST_RIGHT 0   /* 0 (no opt) or 1 */
#endif


/* OGROPT_IGNORE_TIME_CONSTRAINT_ARG: By default, ogr_cycle() treats the 
   nodes_to_do argument as a hint rather than a precise number. This is a 
   BadThing(TM) in non-preemptive or real-time environments that are running 
   the cruncher in time contraints, so ogr_cycle() also gets passed a 
   'with_time_constraints' argument that tells the it whether such an 
   environment is present or not.
   However, on most platforms this will never be true, and under those 
   circumstances, testing the 'with_time_constraints' flag is an uneccessary
   waste of time. OGROPT_IGNORE_TIME_CONSTRAINT_ARG serves to disable the
   test. 
*/
#define OGROPT_IGNORE_TIME_CONSTRAINT_ARG
#if defined(macintosh) || defined(__riscos)         \
    || defined(__NETWARE__) || defined(NETWARE)     \
    || defined(__WINDOWS386__) /* 16bit windows */  \
    || (defined(ASM_X86) && defined(GENERATE_ASM))
    /* ASM_X86: the ogr core used by all x86 platforms is a hand optimized */
    /* .S/.asm version of this core - If we're compiling for asm_x86 then */
    /* we're either generating an .S for later optimization, or compiling */
    /* for comparison with an existing .S */
  #undef OGROPT_IGNORE_TIME_CONSTRAINT_ARG
#endif  


/* Some cpus benefit from having the top of the main ogr_cycle() loop being
   aligned on a specific memory boundary, for optimum performance during
   cache line reads.  Your compiler may well already align the code optimumly,
   but notably gcc for 68k, and PPC, does not.  If you turn this option on,
   you'll need to supply suitable code for the __BALIGN macro.
*/
#ifndef OGROPT_CYCLE_CACHE_ALIGN
#define OGROPT_CYCLE_CACHE_ALIGN 0      /* the default is "no" */
#endif

#if (OGROPT_CYCLE_CACHE_ALIGN == 1) && defined(__BALIGN)
  #define OGR_CYCLE_CACHE_ALIGN __BALIGN
#else
  #define OGR_CYCLE_CACHE_ALIGN { }
#endif


/* Select the implementation for the LOOKUP_FIRSTBLANK macro.
   0 -> No hardware support
   1 -> Have ASM code, but still need the lookup table. You'll have to supply
        suitable code for the __CNTLZ_ARRAY_BASED(bitmap,bitarray) macro.
   2 -> Full featured ASM code. You'll have to supply code suitable for the
        __CNTLZ(bitmap) macro.
  
  NOTE : These macros shall arrange to return the number of leading 0 of
         "~bitmap" (i.e. the number of leading 1 of the "bitmap" argument) plus
         one. Since the bitmap argument is a 32-bit value, the valid range is
         [1; 33]. Said otherwise :
         __CNTLZ(0xFFFFFFFF) == 33     <== YOU'VE BEEN WARNED !
         __CNTLZ(0xFFFFFFFE) == 32
         __CNTLZ(0xFFFFA427) == 18
         __CNTLZ(0x00000000) ==  1

  TEST : Define OGR_TEST_FIRSTBLANK to test the code at run time.
*/
#ifndef OGROPT_HAVE_FIND_FIRST_ZERO_BIT_ASM
#define OGROPT_HAVE_FIND_FIRST_ZERO_BIT_ASM 0
#endif


/* ----------------------------------------------------------------------- */

#include "ogr-ng.h"

#if defined(__cplusplus)
extern "C" {
#endif


static const
int OGR[] = {
  /*  1 */    0,   1,   3,   6,  11,  17,  25,  34,  44,  55,
  /* 11 */   72,  85, 106, 127, 151, 177, 199, 216, 246, 283,
  /* 21 */  333, 356, 372, 425, 480, 492, 553, 585, 623
};


/* ----------------------------------------------------------------------- */

#ifndef __MRC__
static int ogr_init(void);
static int ogr_getresult(void *state, void *result, int resultlen);
static int ogr_destroy(void *state);
static int ogr_cleanup(void);
static int ogr_cycle_entry(void *state, int *pnodes, int with_time_constraints);
static int ogr_create(void *input, int inputlen, void *state, int statelen,
                      int maxlen);
static int found_one(const struct OgrNgState *oState);


#ifndef OGR_NG_GET_DISPATCH_TABLE_FXN
  #define OGR_NG_GET_DISPATCH_TABLE_FXN ogr_ng_get_dispatch_table
#endif
extern CoreDispatchTable * OGR_NG_GET_DISPATCH_TABLE_FXN (void);


#ifdef OGR_DEBUG
  /*
  ** Use DUMP_BITMAPS(depth, lev) or DUMP_RULER(state, depth) macros
  ** instead of calling the following functions directly, unless you
  ** want unpredictable results...
  */
  static void __dump(const struct OgrNgLevel *lev, int depth);
  static void __dump_ruler(const struct OgrNgState *oState, int depth);

  /*
  ** Debugging macros
  */
  #if !defined(DUMP_BITMAPS)
    #define DUMP_BITMAPS(lev,depth)   \
      SAVE_FINAL_STATE(lev);          \
      __dump(lev, depth);
  #endif

  #if !defined(DUMP_RULER)
    #define DUMP_RULER(state,depth)   \
      __dump_ruler(state, depth);
  #endif
#else
  #undef  DUMP_BITMAPS
  #define DUMP_BITMAPS(foo,bar)
  #undef  DUMP_RULER
  #define DUMP_RULER(foo,bar)
#endif  /* OGR_DEBUG */


#ifdef OGR_TEST_FIRSTBLANK
  static int  __testFirstBlank(void);
#endif

#endif  /* __MRC__ */

#if defined(__cplusplus)
}
#endif

/* ----------------------------------------------------------------------- */
/***************************************************************************
 * The following macros define the BITLIST CLASS (bitmaps manipulation)
 * The variables defined here should only be manipulated within these class
 * macros.
 ***************************************************************************/

#if (OGROPT_ALTERNATE_CYCLE == 0) && (OGROPT_ALTERNATE_COMP_LEFT_LIST_RIGHT == 0)
  /* Enforce the use of default code */
  #undef SETUP_TOP_STATE
  #undef COMP_LEFT_LIST_RIGHT
  #undef COMP_LEFT_LIST_RIGHT_32
  #undef PUSH_LEVEL_UPDATE_STATE
  #undef POP_LEVEL
  #undef SAVE_FINAL_STATE
#endif


/*
** OGROPT_ALTERNATE_CYCLE == 0
** Initialize top state
*/
#if !defined(SETUP_TOP_STATE)
 #define SETUP_TOP_STATE(lev)  \
   U comp0 = lev->comp[0];     \
   U dist0 = lev->dist[0];     \
   int newbit = 1;
#endif

/*
** OGROPT_ALTERNATE_CYCLE == 0
** Shift COMP and LIST bitmaps
*/
#if !defined(COMP_LEFT_LIST_RIGHT)
 #define COMP_LEFT_LIST_RIGHT(lev, s) {            \
   U temp1, temp2;                                 \
   register int ss = 32 - (s);                     \
   temp1 = newbit << ss;                           \
   temp2 = lev->list[0] << ss;                     \
   lev->list[0] = (lev->list[0] >> (s)) | temp1;   \
   temp1 = lev->list[1] << ss;                     \
   lev->list[1] = (lev->list[1] >> (s)) | temp2;   \
   temp2 = lev->list[2] << ss;                     \
   lev->list[2] = (lev->list[2] >> (s)) | temp1;   \
   temp1 = lev->list[3] << ss;                     \
   lev->list[3] = (lev->list[3] >> (s)) | temp2;   \
   temp2 = lev->list[4] << ss;                     \
   lev->list[4] = (lev->list[4] >> (s)) | temp1;   \
   temp1 = lev->list[5] << ss;                     \
   lev->list[5] = (lev->list[5] >> (s)) | temp2;   \
   temp2 = lev->list[6] << ss;                     \
   lev->list[6] = (lev->list[6] >> (s)) | temp1;   \
   temp1 = lev->comp[1] >> ss;                     \
   lev->list[7] = (lev->list[7] >> (s)) | temp2;   \
   temp2 = lev->comp[2] >> ss;                     \
   comp0 = (lev->comp[0] << (s)) | temp1;          \
   lev->comp[0] = comp0;                           \
   temp1 = lev->comp[3] >> ss;                     \
   lev->comp[1] = (lev->comp[1] << (s)) | temp2;   \
   temp2 = lev->comp[4] >> ss;                     \
   lev->comp[2] = (lev->comp[2] << (s)) | temp1;   \
   temp1 = lev->comp[5] >> ss;                     \
   lev->comp[3] = (lev->comp[3] << (s)) | temp2;   \
   temp2 = lev->comp[6] >> ss;                     \
   lev->comp[4] = (lev->comp[4] << (s)) | temp1;   \
   temp1 = lev->comp[7] >> ss;                     \
   lev->comp[5] = (lev->comp[5] << (s)) | temp2;   \
   lev->comp[7] = (lev->comp[7] << (s));           \
   lev->comp[6] = (lev->comp[6] << (s)) | temp1;   \
   newbit = 0;                                     \
 }
#endif

/*
** OGROPT_ALTERNATE_CYCLE == 0
** Shift COMP and LIST bitmaps by 32
*/
#if !defined(COMP_LEFT_LIST_RIGHT_32)
 #define COMP_LEFT_LIST_RIGHT_32(lev)  \
   lev->list[7] = lev->list[6];        \
   lev->list[6] = lev->list[5];        \
   lev->list[5] = lev->list[4];        \
   lev->list[4] = lev->list[3];        \
   lev->list[3] = lev->list[2];        \
   lev->list[2] = lev->list[1];        \
   lev->list[1] = lev->list[0];        \
   lev->list[0] = newbit;              \
   comp0 = lev->comp[1];               \
   lev->comp[0] = comp0;               \
   lev->comp[1] = lev->comp[2];        \
   lev->comp[2] = lev->comp[3];        \
   lev->comp[3] = lev->comp[4];        \
   lev->comp[4] = lev->comp[5];        \
   lev->comp[5] = lev->comp[6];        \
   lev->comp[6] = lev->comp[7];        \
   lev->comp[7] = 0;                   \
   newbit = 0;
#endif

#if !defined(PUSH_LEVEL_UPDATE_STATE)
 #define PUSH_LEVEL_UPDATE_STATE(lev) {            \
   U temp1, temp2;                                 \
   struct OgrNgLevel *lev2 = lev + 1;              \
   temp1 = (lev2->list[0] = lev->list[0]);         \
   temp2 = (lev2->list[1] = lev->list[1]);         \
   dist0 = (lev2->dist[0] = lev->dist[0] | temp1); \
   temp2 = (lev2->dist[1] = lev->dist[1] | temp2); \
   comp0 = lev->comp[0] | dist0;                   \
   lev2->comp[0] = comp0;                          \
   lev2->comp[1] = lev->comp[1] | temp2;           \
   temp1 = (lev2->list[2] = lev->list[2]);         \
   temp2 = (lev2->list[3] = lev->list[3]);         \
   temp1 = (lev2->dist[2] = lev->dist[2] | temp1); \
   temp2 = (lev2->dist[3] = lev->dist[3] | temp2); \
   lev2->comp[2] = lev->comp[2] | temp1;           \
   lev2->comp[3] = lev->comp[3] | temp2;           \
   temp1 = (lev2->list[4] = lev->list[4]);         \
   temp2 = (lev2->list[5] = lev->list[5]);         \
   temp1 = (lev2->dist[4] = lev->dist[4] | temp1); \
   temp2 = (lev2->dist[5] = lev->dist[5] | temp2); \
   lev2->comp[4] = lev->comp[4] | temp1;           \
   lev2->comp[5] = lev->comp[5] | temp2;           \
   temp1 = (lev2->list[6] = lev->list[6]);         \
   temp2 = (lev2->list[7] = lev->list[7]);         \
   temp1 = (lev2->dist[6] = lev->dist[6] | temp1); \
   temp2 = (lev2->dist[7] = lev->dist[7] | temp2); \
   lev2->comp[6] = lev->comp[6] | temp1;           \
   lev2->comp[7] = lev->comp[7] | temp2;           \
   newbit = 1;                                     \
 }
#endif

/*
** OGROPT_ALTERNATE_CYCLE == 0
** Pop level state (all bitmaps).
*/
#if !defined(POP_LEVEL)
 #define POP_LEVEL(lev)  \
   comp0 = lev->comp[0]; \
   dist0 = lev->dist[0]; \
   newbit = 0;
#endif

/*
** OGROPT_ALTERNATE_CYCLE == 0
** Save final state (all bitmaps)
*/
#if !defined(SAVE_FINAL_STATE)
 #define SAVE_FINAL_STATE(lev) \
   /* nothing */
#endif


/*-------------------------------------------------------------------------*/

#if defined(BYTE_ORDER) && (BYTE_ORDER == LITTLE_ENDIAN)
  #define FP_CLZ_LITTLEEND 1
#endif

#if (OGROPT_HAVE_FIND_FIRST_ZERO_BIT_ASM == 2) && defined(__CNTLZ)
  #define LOOKUP_FIRSTBLANK(x) __CNTLZ(x)
#elif (OGROPT_HAVE_FIND_FIRST_ZERO_BIT_ASM == 0) && defined(FP_CLZ_LITTLEEND)
  /*
  ** using the exponent in floating point double format
  ** Relies upon IEEE format (Little Endian)
  */
  static inline int LOOKUP_FIRSTBLANK(register unsigned int input)
  {
    unsigned int i;
    union {
      double d;
      int i[2];
    } u;
    i = ~input;
    u.d = i;
    return i == 0 ? 33 : 1055 - (u.i[1] >> 20);
  }
#else
  static const char ogr_first_blank_8bit[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 8, 9
  };


  #if (OGROPT_HAVE_FIND_FIRST_ZERO_BIT_ASM == 1) && defined(__CNTLZ_ARRAY_BASED)
    #define LOOKUP_FIRSTBLANK(x) __CNTLZ_ARRAY_BASED(x,ogr_first_blank_8bit)
  #else /* C code, no asm */
    static inline int LOOKUP_FIRSTBLANK(register unsigned int input)
    {
      register int result = 0;
      if (input >= 0xffff0000) {
        input <<= 16;
        result += 16;
      }
      if (input >= 0xff000000) {
        input <<= 8;
        result += 8;
      }
      result += ogr_first_blank_8bit[input>>24];
      return result;
    }
  #endif
#endif


/*-------------------------------------------------------------------------*/
/*
** found_one() - Assert the ruler is Golomb.
** This function is *REALLY* seldom used and it has no impact at all on
** benchmarks or runtime node rates.
** However, some compilers may choose to inline this function into ogr_cycle()
** thus causing a noticeable slow down. The fix is then to prevent the
** compiler from inlining this function, *NOT* to optimize it.
**
** NOTE : Principle #1 states that we don't have to track distances larger
**    than half the length of the ruler.
*/

static int found_one(const struct OgrNgState *oState)
{
  register int i, j;
  u32 diffs[((1024 - OGRNG_BITMAPS_LENGTH) + 31) / 32];
  register int max = oState->max;
  register int maxdepth = oState->maxdepth;
  const struct OgrNgLevel *levels = &oState->Levels[0];

  for (i = max >> (5+1); i >= 0; --i)
    diffs[i] = 0;

  for (i = 1; i < maxdepth; i++) {
    register int marks_i = levels[i].mark;

    for (j = 0; j < i; j++) {
      register int diff = marks_i - levels[j].mark;
      if (diff <= OGRNG_BITMAPS_LENGTH)
        break;

      if (diff+diff <= max) {       /* Principle #1 */
        register u32 mask = 1 << (diff & 31);
        diff = (diff >> 5) - (OGRNG_BITMAPS_LENGTH / 32);
        if ((diffs[diff] & mask) != 0)
          return 0;                 /* Distance already taken = not Golomb */

        diffs[diff] |= mask;
      }
    }
  }
  return -1;      /* Is golomb */
}


/* ----------------------------------------------------------------------- */

static int ogr_init(void)
{
  if (CHOOSE_DIST_BITS != ogr_ng_choose_bits || CHOOSE_MARKS != ogr_ng_choose_marks)
  {
    /* Incompatible CHOOSE array - Give up */
    return CORE_E_INTERNAL;
  }

  #if defined(OGR_TEST_FIRSTBLANK)
    if (__testFirstBlank())
      return CORE_E_INTERNAL;
  #endif

  return CORE_S_OK;
}


/* ----------------------------------------------------------------------- */

#define choose(dist,seg) pchoose[(dist) + ((seg) << CHOOSE_DIST_BITS)]

/*
** This function is called each time a stub is (re)started, so optimizations
** are useless and not welcomed.
*/

static int ogr_create(void *input, int inputlen, void *state, int statelen,
                      int stopDepth)
{
  struct OgrNgState *oState;
  struct WorkStub *workstub = (struct WorkStub *)input;
  int    midseg_size;


  if (input == NULL || inputlen != sizeof(struct WorkStub)
                    || (size_t)statelen < sizeof(struct OgrNgState)) {
    return CORE_E_INTERNAL;
  }

  if ( (oState = (struct OgrNgState *)state) == NULL) {
    return CORE_E_MEMORY;
  }

  if (stopDepth > workstub->stub.length) {
    return CORE_E_STUB;
  }

  memset(oState, 0, sizeof(struct OgrNgState));
  oState->maxdepth   = workstub->stub.marks;
  oState->maxdepthm1 = oState->maxdepth-1;
  oState->max        = OGR[oState->maxdepthm1];

  if (oState->maxdepth < OGR_NG_MIN || oState->maxdepth > OGR_NG_MAX) {
    return CORE_E_FORMAT;
  }  


  /* 
  ** Mid-segment reduction (one or two segments)
  */
  midseg_size         = 2 - (oState->maxdepthm1 & 1);
  oState->half_depth  = (oState->maxdepthm1 - midseg_size) / 2;
  oState->half_depth2 = oState->half_depth + midseg_size;

  {
    int n, limit;
    struct OgrNgLevel *lev = &oState->Levels[1];
    int mark = 0;
    int depth = 1;
    u16* pchoose = precomp_limits[oState->maxdepth - OGR_NG_MIN].choose_array;
    SETUP_TOP_STATE(lev);

    n = workstub->worklength;
    if (n < workstub->stub.length) {
      n = workstub->stub.length;
    }
    if (n > STUB_MAX) {
      return CORE_E_FORMAT;
    }

    limit = choose(0, depth);
    oState->Levels[depth].limit = limit;
    while (depth <= n) {
      int s = workstub->stub.diffs[depth-1];

      if ((mark += s) > limit) {
        return CORE_E_STUB;
      }

      while (s >= 32) {
        if (s == 32 && (comp0 & 1u)) {
          return CORE_E_STUB;         /* invalid location */
        }
        COMP_LEFT_LIST_RIGHT_32(lev);
        s -= 32;
      }

      if (s > 0) {
        if ( (comp0 & (1u << (32-s))) ) {
          return CORE_E_STUB;         /* invalid location */
        }
        COMP_LEFT_LIST_RIGHT(lev, s);
      }

      PUSH_LEVEL_UPDATE_STATE(lev);
      lev->mark = mark;
      lev++;
      depth++;

      /* Compute the maximum position for the current mark */
      limit = choose(dist0 >> ttmDISTBITS, depth);
      if (depth > oState->half_depth && depth <= oState->half_depth2) {
        int temp = oState->max - 1 - oState->Levels[oState->half_depth].mark;

        if (depth < oState->half_depth2) {
          /* Here's why LOOKUP_FIRSTBLANK(0xffffffff) must be equal to 33 ! */
          temp -= LOOKUP_FIRSTBLANK(dist0);
        }

        if (limit > temp) {
          limit = temp;
        }
      }

      /* Setup the next level */
      lev->limit = limit;
      lev->mark  = mark;
    }
    SAVE_FINAL_STATE(lev);
    oState->depth = depth - 1;
  }

  oState->startdepth = workstub->stub.length;
  oState->stopdepth  = (stopDepth != 0) ? stopDepth : oState->startdepth;
  return CORE_S_OK;
}


/* ----------------------------------------------------------------------- */
/* WARNING : Buffers cannot store every marks of the ruler being checked. The
**  STUB_MAX macro determines how many "diffs" are stored and retrieved when
**  the core restarts a stub.
**  As a result, the core shall not be interrupted without care : if it's
**  interrupted beyond the mark that can be stored in the buffers, then the
**  node count becomes inacurate. This bug caused troubles on clients running
**  on non-preemptive OS and on clients using cores designed for such OSes.
**  The OGROPT_IGNORE_TIME_CONSTRAINTS_ARG macro should be defined whenever
**  possible. The sole purpose of the "with_time_constraints" argument is to
**  ensure responsiveness on non-preemptive OSes, especially on slow machines.
**
**  The fix makes use of a checkpoint to remember the node count upto the last
**  mark that can be recorded in the buffers. This node count is returned to
**  the caller, even if the core is interrupted beyond that mark. Thus, the
**  total node count maintained by the caller is kept in sync despite the
**  limitations of the buffers.
**  The number of extra nodes checked (beyond that mark) is stored into the
**  State data structure and it is used as the initial node count in case the
**  core is not interrupted (thus avoiding unecessary overhead).
*/

#if (OGROPT_ALTERNATE_CYCLE == 0)
static int ogr_cycle_256(struct OgrNgState *oState, int *pnodes,
                         const u16* pchoose, int with_time_constraints)
{
  int depth = oState->depth + 1;
  struct OgrNgLevel *lev = &oState->Levels[depth];
  int nodes = 0;
  int halfdepth  = oState->half_depth;
  int halfdepth2 = oState->half_depth2;

  #if !defined(OGROPT_IGNORE_TIME_CONSTRAINT_ARG)
    int checkpoint = 0;
    int checkpoint_depth = (1+STUB_MAX);
    *pnodes += oState->node_offset; /* force core to do some work */
    nodes = oState->node_offset;
    oState->node_offset = 0;
  #endif

  SETUP_TOP_STATE(lev);
  nodes++;

  do {
    int limit = lev->limit;
    int mark  = lev->mark;

    for (;;) {
      if (comp0 < 0xFFFFFFFE) {
        int s = LOOKUP_FIRSTBLANK(comp0);

        if ((mark += s) > limit) {
          break;
        }
        COMP_LEFT_LIST_RIGHT(lev, s);
      }
      else {         /* s >= 32 */
        if ((mark += 32) > limit) {
          break;
        }
        if (comp0 == ~0u) {
          COMP_LEFT_LIST_RIGHT_32(lev);
          continue;
        }
        COMP_LEFT_LIST_RIGHT_32(lev);
      }

      lev->mark = mark;
      if (depth == oState->maxdepthm1) {
        goto exit;
      }

      PUSH_LEVEL_UPDATE_STATE(lev);
      lev++;
      depth++;
      
      /* Compute the maximum position for the current mark */
      limit = choose(dist0 >> ttmDISTBITS, depth);
      lev->limit = limit;
      if (depth <= halfdepth) {
        if (nodes >= *pnodes) {
          lev->mark = mark;
          goto exit;
        }
      }
      else if (depth <= halfdepth2) {
        int temp = oState->max - 1 - oState->Levels[halfdepth].mark;

        if (depth < halfdepth2) {
          temp -= LOOKUP_FIRSTBLANK(dist0);
        }

        if (limit > temp) {
          lev->limit = limit = temp;
        }
      }

      if (with_time_constraints) { /* if (...) is optimized away if unused */
        #if !defined(OGROPT_IGNORE_TIME_CONSTRAINT_ARG)
        if (depth <= checkpoint_depth)
          checkpoint = nodes;

        if (nodes >= *pnodes) {
          oState->node_offset = nodes - checkpoint;
          lev->mark = mark;
          nodes = checkpoint;
          goto exit;
        }  
        #endif  
      }
      nodes++;
    } /* for (;;) */
    lev--;
    depth--;
    POP_LEVEL(lev);
  } while (depth > oState->stopdepth);

exit:
  SAVE_FINAL_STATE(lev);
  *pnodes = nodes;
  return depth;
}
#endif  /* OGROPT_ALTERNATE_CYCLE */


/* ----------------------------------------------------------------------- */

/* Note to porters :
** The new implementation pulls the call to found_one() out of the main loop.
** Optimizations shall focus on ogr_cycle_256() only.
*/
static int ogr_cycle_entry(void *state, int *pnodes, int with_time_constraints)
{
  int retval = CORE_S_CONTINUE;
  struct OgrNgState *oState = (struct OgrNgState *)state;
  u16* pchoose = precomp_limits[oState->maxdepth - OGR_NG_MIN].choose_array;
  int safesize = OGRNG_BITMAPS_LENGTH * 2;
  int depth;

  do {
    depth = ogr_cycle_256(oState, pnodes, pchoose, with_time_constraints);
  } while (oState->Levels[depth].mark > safesize && found_one(oState) == 0);

  if (depth == oState->maxdepthm1) {
    retval = CORE_S_SUCCESS;
  }
  else if (depth <= oState->stopdepth) {
    retval = CORE_S_OK;
  }

  oState->depth = depth - 1;
  return retval;
}

/* ----------------------------------------------------------------------- */

static int ogr_getnodeoffset(void *state)
{
  struct OgrNgState *oState = (struct OgrNgState *)state;

  return oState->node_offset;
}

/* ----------------------------------------------------------------------- */

static int ogr_getresult(void *state, void *result, int resultlen)
{
  struct OgrNgState *oState = (struct OgrNgState *)state;
  struct WorkStub *workstub = (struct WorkStub *)result;
  int i;

  if (resultlen != sizeof(struct WorkStub)) {
    return CORE_E_INTERNAL;
  }
  workstub->stub.marks = (u16)oState->maxdepth;
  workstub->stub.length = (u16)oState->startdepth;
  for (i = 0; i < STUB_MAX; i++) {
    workstub->stub.diffs[i] = (u16)(oState->Levels[i+1].mark - oState->Levels[i].mark);
  }
  workstub->worklength = oState->depth;
  if (workstub->worklength > STUB_MAX) {
    workstub->worklength = STUB_MAX;
  }
  return (ogr_check_cache(oState->maxdepth)) ? CORE_S_OK : CORE_E_INTERNAL;
}


/* ----------------------------------------------------------------------- */

static int ogr_destroy(void *state)
{
  struct OgrNgState *oState = (struct OgrNgState*) state;

  return (ogr_check_cache(oState->maxdepth)) ? CORE_S_OK : CORE_E_INTERNAL;
}


/* ----------------------------------------------------------------------- */

static int ogr_cleanup(void)
{
  /* NEVER CALLED */
  return CORE_S_OK;
}


/* ----------------------------------------------------------------------- */

CoreDispatchTable * OGR_NG_GET_DISPATCH_TABLE_FXN (void)
{
  static CoreDispatchTable dispatch_table;
  dispatch_table.init      = ogr_init;
  dispatch_table.create    = ogr_create;
  dispatch_table.cycle     = ogr_cycle_entry;
  dispatch_table.getresult = ogr_getresult;
  dispatch_table.destroy   = ogr_destroy;
  dispatch_table.cleanup   = ogr_cleanup;
  dispatch_table.getnodeoffset = ogr_getnodeoffset;
  return &dispatch_table;
}

/* ----------------------------------------------------------------------- */

#if defined(OGR_DEBUG)
static void __dump(const struct OgrNgLevel *lev, int depth)
{
  printf("--- depth %d, limit %d\n", depth, lev->limit);

#if  defined(OGROPT_OGR_CYCLE_ALTIVEC) \
  && (defined(__VEC__) || defined(__ALTIVEC__))
    printf("list=%:08vlx %:08vlx\n", lev->listV0, lev->listV1);
    printf("dist=%:08vlx %:08vlx\n", lev->distV0, lev->distV1);
    printf("comp=%:08vlx %:08vlx\n", lev->compV0, lev->compV1);
#elif !defined(OGROPT_64BIT_IMPLEMENTATION)
  printf("list=%08x %08x %08x %08x %08x %08x %08x %08x\n",
      lev->list[0], lev->list[1], lev->list[2], lev->list[3],
      lev->list[4], lev->list[5], lev->list[6], lev->list[7]);
  printf("dist=%08x %08x %08x %08x %08x %08x %08x %08x\n",
      lev->dist[0], lev->dist[1], lev->dist[2], lev->dist[3],
      lev->dist[4], lev->dist[5], lev->dist[6], lev->dist[7]);
  printf("comp=%08x %08x %08x %08x %08x %08x %08x %08x\n",
      lev->comp[0], lev->comp[1], lev->comp[2], lev->comp[3],
      lev->comp[4], lev->comp[5], lev->comp[6], lev->comp[7]);

#elif (!defined(SIZEOF_SHORT) || (SIZEOF_SHORT < 8)) \
   && (!defined(SIZEOF_INT)   || (SIZEOF_INT   < 8)) \
   && (!defined(SIZEOF_LONG)  || (SIZEOF_LONG  < 8))
  // Assume ui64 is unsigned long long int
  printf("list=%016llx %016llx %016llx %016llx\n",
      lev->list[0], lev->list[1], lev->list[2], lev->list[3]);
  printf("dist=%016llx %016llx %016llx %016llx\n",
      lev->dist[0], lev->dist[1], lev->dist[2], lev->dist[3]);
  printf("comp=%016llx %016llx %016llx %016llx\n",
      lev->comp[0], lev->comp[1], lev->comp[2], lev->comp[3]);
#else
  printf("list=%016lx %016lx %016lx %016lx\n",
      lev->list[0], lev->list[1], lev->list[2], lev->list[3]);
  printf("dist=%016lx %016lx %016lx %016lx\n",
      lev->dist[0], lev->dist[1], lev->dist[2], lev->dist[3]);
  printf("comp=%016lx %016lx %016lx %016lx\n",
      lev->comp[0], lev->comp[1], lev->comp[2], lev->comp[3]);
#endif
}


static void __dump_ruler(const struct OgrNgState *oState, int depth)
{
  int i;
  printf("max %d ruler : ", oState->max);
  for (i = 1; i < depth; i++) {
    printf("%d ", oState->Levels[i].mark - oState->Levels[i-1].mark);
  }
  printf("\n");
}
#endif  /* OGR_DEBUG */


#if defined(OGR_TEST_FIRSTBLANK)
static int __testFirstBlank(void)
{
  static int done_test = 0;
  static char ogr_first_blank[65537]; /* first blank in 16 bit COMP bitmap, range: 1..16 */
  /* first zero bit in 16 bits */
  int i, j, k = 0, m = 0x8000;
  unsigned int err_count = 0;

  for (i = 1; i <= 16; i++) {
    for (j = k; j < k+m; j++) ogr_first_blank[j] = (char)i;
    k += m;
    m >>= 1;
  }
  ogr_first_blank[0xffff] = 17;     /* just in case we use it */
  if (done_test == 0)
  {
    unsigned int q, first_fail = 0xffffffff;
    int last_s1 = 0, last_s2 = 0;
    done_test = 1;                  /* test only once */
    printf("begin firstblank test\n"
           "(this may take a looooong time and requires a -KILL to stop)\n");
    for (q = 0; q <= 0xfffffffe; q++)
    {
      int s1 = ((q < 0xffff0000) ? \
        (ogr_first_blank[q>>16]) : (16 + ogr_first_blank[q - 0xffff0000]));
      int s2 = LOOKUP_FIRSTBLANK(q);
      int show_it = (q == 0xfffffffe || (q & 0xfffff) == 0xfffff);
      if (s1 != s2)
      {
        if (first_fail == 0xffffffff || (last_s1 != s1 || last_s2 != s2)) {
          printf("\n");
          first_fail = q;
          show_it = 1;
          last_s1 = s1;
          last_s2 = s2;
        }  
        if (show_it) {
          printf("\rfirstblank FAIL 0x%08x-0x%08x (should be %d, got %d) ", first_fail, q, s1, s2);
          fflush(stdout);
        }   
        err_count++;
      }
      else 
      {
        if (first_fail != 0xffffffff) {
          printf("\n");
          first_fail = 0xffffffff;
          show_it = 1;
        }  
        if (show_it) {
          printf("\rfirstblank [ok] 0x%08x-0x%08x ", q & 0xfff00000, q);
          fflush(stdout);
        }  
      }
    }
    if (LOOKUP_FIRSTBLANK(q) != 33) {  /* q = 0xffffffff */
      printf("firstblank FAIL (should be 33, got %d)", LOOKUP_FIRSTBLANK(q));
      err_count++;
    }
    printf("\nend firstblank test (%u errors)\n", err_count);
  }
  return (err_count) ? -1 : 0;
}
#endif