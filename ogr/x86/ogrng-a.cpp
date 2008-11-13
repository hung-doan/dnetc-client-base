#include "ansi/ogrng-32.h"

#include "x86/asm-x86.h"

#define OGROPT_HAVE_FIND_FIRST_ZERO_BIT_ASM   2 /* 0-2 - '100% asm'      */
#define OGROPT_ALTERNATE_CYCLE                1 /* 0/1 - 'yes'           */
#define OGR_NG_GET_DISPATCH_TABLE_FXN  ogrng_get_dispatch_table_asm1

#include "ansi/ogrng_codebase.cpp"

#include "ccoreio.h"       /* CDECL */
#include <stddef.h>        /* offsetof */

extern "C" int CDECL ogr_cycle_256_rt1(struct OgrState *oState, int *pnodes, const u16* pchoose);

static int ogr_cycle_256(struct OgrState *oState, int *pnodes, const u16* pchoose)
{
    /* Check structures layout and alignment to match assembly */

    STATIC_ASSERT(offsetof(struct OgrState, max)         == 0 );
    STATIC_ASSERT(offsetof(struct OgrState, maxdepthm1)  == 8 );
    STATIC_ASSERT(offsetof(struct OgrState, half_depth)  == 12);
    STATIC_ASSERT(offsetof(struct OgrState, half_depth2) == 16);
    STATIC_ASSERT(offsetof(struct OgrState, stopdepth)   == 24);
    STATIC_ASSERT(offsetof(struct OgrState, depth)       == 28);
    STATIC_ASSERT(offsetof(struct OgrState, Levels)      == 32);

    STATIC_ASSERT(sizeof(struct OgrLevel) == 104);
    STATIC_ASSERT(sizeof(oState->Levels)  == 104 * OGR_MAXDEPTH);

    STATIC_ASSERT(offsetof(struct OgrLevel, list)  ==   0);
    STATIC_ASSERT(offsetof(struct OgrLevel, dist)  ==  32);
    STATIC_ASSERT(offsetof(struct OgrLevel, comp)  ==  64);
    STATIC_ASSERT(offsetof(struct OgrLevel, mark)  ==  96);
    STATIC_ASSERT(offsetof(struct OgrLevel, limit) == 100);

    return ogr_cycle_256_rt1(oState, pnodes, pchoose);
}
