/* 
 * Copyright distributed.net 1997-1999 - All Rights Reserved
 * For use in distributed.net projects only.
 * Any other distribution or use of this source violates copyright.
 *
 * -------------------------------------------------------------------
 * program (pro'-gram) [vi]: To engage in a pastime similar to banging
 * one's head against a wall but with fewer opportunities for reward.
 * -------------------------------------------------------------------
 */
const char *selcore_cpp(void) {
return "@(#)$Id: selcore-conflict.cpp,v 1.59 1999/11/28 07:05:15 sampo Exp $"; }


#include "cputypes.h"
#include "client.h"    // MAXCPUS, Packet, FileHeader, Client class, etc
#include "baseincs.h"  // basic (even if port-specific) #includes
#include "problem.h"   // problem class
#include "cpucheck.h"  // cpu selection, GetTimesliceBaseline()
#include "logstuff.h"  // Log()/LogScreen()/LogScreenPercent()/LogFlush()
#include "clicdata.h"  // GetContestNameFromID()
#include "bench.h"     // TBenchmark()
#include "selftest.h"  // SelfTest()
#include "selcore.h"   // keep prototypes in sync

/* ------------------------------------------------------------------------ */

static const char **__corenames_for_contest( unsigned int cont_i )
{
  /* 
   When selecting corenames, use names that describe how (what optimization)
   they are different from their predecessor(s). If only one core,
   use the obvious "MIPS optimized" or similar.
  */
  #define LARGEST_SUBLIST 7 /* including the terminating null */
  static const char *corenames_table[CONTEST_COUNT][LARGEST_SUBLIST]= 
  #undef LARGEST_SUBLIST
  {
  #if (CLIENT_CPU == CPU_X86)
    { /* RC5 */
      /* we should be using names that tell us how the cores are different
         (just like "bryd" and "movzx bryd")
      */
      "RG/BRF class 5", /* P5/Am486/Cx486 - may become P5MMX at runtime*/
      "RG class 3/4",   /* 386/486 - may become SMC at runtime */
      "RG class 6",     /* PPro/II/III */
      "RG Cx re-pair",  /* Cyrix 6x86[MX]/M2, AMD K7 */
      "RG RISC-rotate I", /* K5 */
      "RG RISC-rotate II", /* K6 - may become mmx-k6-2 core at runtime */
      NULL
    },
    { /* DES */
      "byte Bryd",
      "movzx Bryd",
      #if defined(MMX_BITSLICER) || defined(CLIENT_SUPPORTS_SMP) 
      "Kwan/Bitslice", /* may become MMX bitslice at runtime */
      #endif
      NULL
    },
  #elif (CLIENT_CPU == CPU_ARM)
    { /* RC5 */
      "Series A core", /* (autofor for ARM 3/6xx/7xxx) "ARM 3, 610, 700, 7500, 7500FE" */
      "Series B core", /* (autofor ARM 8xx/StrongARM) "ARM 810, StrongARM 110" */
      "Series C core", /* (autofor ARM 2xx) "ARM 2, 250" */
      NULL             /* "ARM 710" */
    },
    { /* DES */
      "Standard ARM core", /* "ARM 3, 610, 700, 7500, 7500FE" or  "ARM 710" */
      "StrongARM optimized core", /* "ARM 810, StrongARM 110" or "ARM 2, 250" */
      NULL
    },
  #elif (CLIENT_CPU == CPU_68K)
    { /* RC5 */
      "Motorola 68000", "Motorola 68010", "Motorola 68020", "Motorola 68030",
      "Motorola 68040", "Motorola 68060", /* will never change :) */
      NULL
    },
    { /* DES */
      "Generic DES core", 
      NULL
    },
  #elif (CLIENT_CPU == CPU_ALPHA) 
    { /* RC5 */
      #if (CLIENT_OS == OS_DEC_UNIX)
      "ev3 and ev4 optimized",
      "ev5 and ev6 optimized",
      #elif (CLIENT_OS == OS_WIN32)
      "michmarch series A",  /* :) */
      #else
      "axp bmeyer",
      #endif
      NULL
    },
    { /* DES */
      #if (CLIENT_OS == OS_DEC_UNIX)
      "ev3 and ev4 optimized",
      "ev5 and ev6 optimized",
      #else
      "dworz/amazing core",
      #endif
      NULL
    },
  #elif (CLIENT_CPU == CPU_POWERPC)
    { /* RC5 */
      #if (CLIENT_OS == OS_WIN32)
      "RG ansi2",
      #else
        #if (CLIENT_OS == OS_AIX)
        "RG AIXALL (Power CPU)",
        #endif
        "allitnil", /* aka rc5_unit_func_g1() wrapper */
        "lintilla", /* aka rc5_unit_func_g2_g3() wrapper */
      #endif
      NULL
    },
    { /* DES */
      "Generic DES core", 
      NULL
    },
  #else
    { /* RC5 */
      "Generic RC5 core",
      NULL
    },
    { /* DES */
      "Generic DES core",
      NULL
    },
  #endif  
    { /* OGR */
      "Standard OGR core",
      NULL
    },
    { /* CSC */
      "6 bit - inline", 
      "6 bit - called", 
      "1 key - inline", 
      "1 key - called",
      NULL
    }
  };  
  static int fixed_up = -1;
  if (fixed_up < 0)
  {
    #if (CLIENT_CPU == CPU_X86)
    {
      long det = GetProcessorType(1);
      #ifdef SMC /* actually only for the first thread */
      corenames_table[RC5][1] = "RG self-modifying";
      #endif
      if (det >= 0 && (det & 0x100)!=0) /* ismmx */
      {
        #if defined(MMX_RC5)
        corenames_table[RC5][0] = "jasonp P5/MMX"; /* slower on a PII/MMX */
        #endif
        #if defined(MMX_RC5_AMD)
        corenames_table[RC5][5] = "BRF Kx/MMX"; /* is this k6-2 only? */
        #endif
        #ifdef MMX_BITSLICER
        corenames_table[DES][2] = "MMX bitslice"; 
        #endif
      }
    }
    #endif
    #if (CLIENT_CPU == CPU_POWERPC) && (CLIENT_OS == OS_MACOS) // G4's are only in macs
    {
    	long det = GetProcessorType(1);
		if(det == 2) {    	
        	corenames_table[RC5][2] = "crunch-vec"; /* aka rc5_unit_func_vec() wrapper */
        	corenames_table[RC5][3] = NULL;
        }
    }
    #endif
    fixed_up = 1;  
  }
  if (cont_i < CONTEST_COUNT)
  {
    return (const char **)(&(corenames_table[cont_i][0]));
  }
  return ((const char **)0);
}  

/* -------------------------------------------------------------------- */

static unsigned int __corecount_for_contest( unsigned int cont_i )
{
  const char **cnames = __corenames_for_contest( cont_i );
  if (cnames)
  {
    cont_i = 0;
    while (cnames[cont_i])
      cont_i++;
    return cont_i;
  }
  return 0;  
}

/* ===================================================================== */

void selcoreEnumerateWide( int (*proc)(
                            const char **corenames, int idx, void *udata ),
                       void *userdata )
{
  if (proc)
  {
    unsigned int corenum;
    for (corenum = 0;;corenum++)
    {
      const char *carray[CONTEST_COUNT];
      int have_one = 0;
      unsigned int cont_i;
      for (cont_i = 0; cont_i < CONTEST_COUNT;cont_i++)
      {
        carray[cont_i] = (const char *)0;
        if (corenum < __corecount_for_contest( cont_i ))
        {
          const char **names = __corenames_for_contest( cont_i );
          carray[cont_i] = names[corenum];
          have_one++;
        }
      }
      if (!have_one)
        break;
      if (! ((*proc)( &carray[0], (int)corenum, userdata )) )
        break;
    }
  }  
  return;
}
  
/* ---------------------------------------------------------------------- */

void selcoreEnumerate( int (*proc)(unsigned int cont, 
                            const char *corename, int idx, void *udata ),
                       void *userdata )
{
  if (proc)
  {
    int stoploop = 0;
    unsigned int cont_i;
    for (cont_i = 0; !stoploop && cont_i < CONTEST_COUNT; cont_i++)
    {
      unsigned int corecount = __corecount_for_contest( cont_i );
      if (corecount)
      {
        unsigned int coreindex;
        const char **corenames = __corenames_for_contest(cont_i);
        for (coreindex = 0; !stoploop && coreindex < corecount; coreindex++)
          stoploop = (! ((*proc)(cont_i, 
                      corenames[coreindex], (int)coreindex, userdata )) );
      }
    }
  }
  return;
}  

/* --------------------------------------------------------------------- */

int selcoreValidateCoreIndex( unsigned int cont_i, int index )
{
  if (index >= 0 && index < ((int)__corecount_for_contest( cont_i )))
    return index;
  return -1;
}

/* --------------------------------------------------------------------- */

const char *selcoreGetDisplayName( unsigned int cont_i, int index )
{
  if (index >= 0 && index < ((int)__corecount_for_contest( cont_i )))
  {
     const char **names = __corenames_for_contest( cont_i );
     return names[index];
  }
  return "";
}

/* ===================================================================== */

static struct
{
  int user_cputype[CONTEST_COUNT]; /* what the user has in the ini */
  int corenum[CONTEST_COUNT]; /* what we map it to */
} selcorestatics;

/* ---------------------------------------------------------------------- */

int DeinitializeCoreTable( void ) { return 0; }

int InitializeCoreTable( int *coretypes ) /* ClientMain calls this */
{
  static int initialized = -1;
  unsigned int cont_i;
  if (initialized < 0)
  {
    for (cont_i = 0; cont_i < CONTEST_COUNT; cont_i++)
    {
      selcorestatics.user_cputype[cont_i] = -1;
      selcorestatics.corenum[cont_i] = -1;
    }
    initialized = 0;
  }
  if (coretypes)
  {
    for (cont_i = 0; cont_i < CONTEST_COUNT; cont_i++)
    {
      int index = 0;
      if (__corecount_for_contest( cont_i ) > 1)
        index = selcoreValidateCoreIndex( cont_i, coretypes[cont_i] );
      if (!initialized || index != selcorestatics.user_cputype[cont_i])
        selcorestatics.corenum[cont_i] = -1; /* got change */
      selcorestatics.user_cputype[cont_i] = index;
    }
    initialized = 1;
  }
  if (initialized > 0)
    return 0;
  return -1;
}  

/* ---------------------------------------------------------------------- */

static long __bench_or_test( int which, 
                            unsigned int cont_i, unsigned int benchsecs )
{
  long rc = -1;
  
  if (InitializeCoreTable(((int *)0)) < 0) /* ACK! selcoreInitialize() */
    return -1;                             /* hasn't been called */

  if (cont_i < CONTEST_COUNT)
  {
    /* save current state */
    int user_cputype = selcorestatics.user_cputype[cont_i]; 
    int corenum = selcorestatics.corenum[cont_i];
    unsigned int coreidx, corecount = __corecount_for_contest( cont_i );
    rc = 0;
    for (coreidx = 0; coreidx < corecount; coreidx++)
    {
      selcorestatics.user_cputype[cont_i] = coreidx; /* as if user set it */
      selcorestatics.corenum[cont_i] = -1; /* reset to show name */
      if (which == 's') /* bench */
      {
        int irc = SelfTest( cont_i );
        if (irc <= 0) /* failed or not supported */
        {
          rc = (long)irc;
          break; /* test failed. stop */
        }
      }
      else if ((rc = TBenchmark( cont_i, benchsecs, 0 )) <= 0)
        break; /* failed/not supported for this contest */
    }
    selcorestatics.user_cputype[cont_i] = user_cputype; 
    selcorestatics.corenum[cont_i] = corenum;
  }
  return rc;
}

int selcoreBenchmark( unsigned int cont_i, unsigned int secs )
{
  return __bench_or_test( 'b', cont_i, secs );
}

int selcoreSelfTest( unsigned int cont_i )
{
  return (int)__bench_or_test( 's', cont_i, 0 );
}

/* ---------------------------------------------------------------------- */

/* this is called from Problem::LoadState() */
int selcoreGetSelectedCoreForContest( unsigned int contestid )
{
  int corename_printed = 0;
  static long detected_type = -123;
  const char *contname = CliGetContestNameFromID(contestid);
  if (!contname) /* no such contest */
    return -1;

  if (InitializeCoreTable(((int *)0)) < 0) /* ACK! selcoreInitialize() */
    return -1;                             /* hasn't been called */

  if (__corecount_for_contest(contestid) == 1) /* only one core? */
    return 0;
  if (selcorestatics.corenum[contestid] >= 0) /* already selected one? */
    return selcorestatics.corenum[contestid];

  if (detected_type == -123) /* haven't autodetected yet? */
  {
    detected_type = GetProcessorType(1 /* quietly */);
    if (detected_type < 0)
      detected_type = -1;
    else
    {
      int quietly = 1;
      unsigned int cont_i;
      for (cont_i = 0; quietly && cont_i < CONTEST_COUNT; cont_i++)
      {
        if (__corecount_for_contest(cont_i) < 2)
          ; /* nothing */
        else if (selcorestatics.user_cputype[cont_i] < 0)
          quietly = 0;
      }
      if (!quietly)
        GetProcessorType(0);
    }
  }

  #if (CLIENT_CPU == CPU_ALPHA)
  if (contestid == RC5 || contestid == DES) /* old style */
  {
    selcorestatics.corenum[contestid] = selcorestatics.user_cputype[contestid];
    if (selcorestatics.corenum[contestid] < 0)
    {
      /* this is only useful if more than one core, which is currently
         only OSF/DEC-UNIX. If only one core, then that will have been 
         handled in the generic code above, but we play it safe anyway.
      */
      if (detected_type == 5 /*EV5*/ || detected_type == 7 /*EV56*/ ||
          detected_type == 8 /*EV6*/ || detected_type == 9 /*PCA56*/)
        selcorestatics.corenum[contestid] = 1;
      else /* ev3 and ev4 (EV4, EV4, LCA4, EV45) */
        selcorestatics.corenum[contestid] = 0;
    }    
    if (selcorestatics.corenum[contestid] < 0 || 
      selcorestatics.corenum[contestid] >= (int)__corecount_for_contest(contestid))
    {
      selcorestatics.corenum[contestid] = 0;
    }  
  }
  else if (contestid == CSC)
  {
    selcorestatics.corenum[CSC] = selcorestatics.user_cputype[CSC];
    if (selcorestatics.corenum[CSC] < 0)
    {
      ; // Who knows?  Just benchmark them all below
    }
  }
  #elif (CLIENT_CPU == CPU_68K)
  if (contestid == RC5 || contestid == DES) /* old style */
  {
    const char *corename = NULL;
    selcorestatics.corenum[DES] = 0;  /* only one DES core */
    selcorestatics.corenum[RC5] = selcorestatics.user_cputype[RC5];
    if (selcorestatics.corenum[RC5] < 0)
      selcorestatics.corenum[RC5] = detected_type;
    if (selcorestatics.corenum[RC5] < 0)
      selcorestatics.corenum[RC5] = 0;
    if (selcorestatics.corenum[RC5] == 4 || selcorestatics.corenum[RC5] == 5 ) 
      corename = "040/060";  // there is no 68050, so type5=060
    else //if (cputype == 0 || cputype == 1 || cputype == 2 || cputype == 3)
      corename = "000/010/020/030";
    LogScreen( "Selected code optimized for the Motorola 68%s.\n", corename );
    corename_printed = 1;
  }
  #elif (CLIENT_CPU == CPU_POWERPC)
  if (contestid == RC5 || contestid == DES) /* old style */
  {
    selcorestatics.corenum[DES] = 0; /* only one DES core */
    #if ((CLIENT_OS == OS_BEOS) || (CLIENT_OS == OS_AMIGAOS))
      // Be OS isn't supported on 601 machines
      // There is no 601 PPC board for the Amiga
      selcorestatics.corenum[RC5] = 1; //"PowerPC 603/604/750"
    #elif (CLIENT_OS == OS_WIN32)
      //actually win32/ppc isn't supported, but just in case
      selcorestatics.corenum[RC5] = 1;
    #elif (CLIENT_OS == OS_MACOS)
      switch (detected_type)
      {
      	case 0:
      		selcorestatics.corenum[RC5] = 0;	// PowerPC 601
      		selcorestatics.corenum[CSC] = 1;	// 16k L1 instruction cache
      		break;
      	case 1:
      		selcorestatics.corenum[RC5] = 1;	// PowerPC 603 through 750
      		//selcorestatics.corenum[CSC] = 1;	// L1 cache 16-64k
      		break;							   
      	case 2:
      		selcorestatics.corenum[RC5] = 2;	// PowerPC 7400
      		//selcorestatics.corenum[CSC] = 0;	// L1 cache 64k
      		break;
      }
    #endif
  }
  #elif (CLIENT_CPU == CPU_X86)
  {
    if (contestid == RC5)
    {
      selcorestatics.corenum[RC5] = selcorestatics.user_cputype[RC5];
      if (selcorestatics.corenum[RC5] < 0)
      {
        if (detected_type >= 0)
        {
          int cindex = -1; 
          switch ( detected_type & 0xff )
          {
            case 0: cindex = 0; break; // P5
            case 1: cindex = 1; break; // 386/486
            case 2: cindex = 2; break; // PII/PIII
            case 3: cindex = 3; break; // Cx6x86, AMD K7
            case 4: cindex = 4; break; // K5
            case 5: cindex = 5; break; // K6/K6-2/K6-3
            #if defined(SMC)    
            case 6: cindex = 1; break; // cyrix 486 uses SMC if available
            #else 
            case 6: cindex = 0; break; // else default to P5 (see /bugs/ #99)
            #endif
            case 7: cindex = 2; break; // castrated Celeron
            case 8: cindex = 2; break; // PPro
            //no default
          }
          selcorestatics.corenum[RC5] = cindex;
        }
      }
    }     
    else if (contestid == DES)
    {  
      selcorestatics.corenum[DES] = selcorestatics.user_cputype[DES];
      if (selcorestatics.corenum[DES] < 0)
      {
        if (detected_type >= 0)
        {
          int cindex = -1;
          if ((detected_type & 0x100) != 0 && /* have mmx */
             __corecount_for_contest(contestid) > 2) /* have mmx-bitslicer */
            cindex = 2; /* mmx bitslicer */
          else 
          {
            switch ( detected_type & 0xff )
            {
              case 0: cindex = 0; break; // P5             == standard Bryd
              case 1: cindex = 0; break; // 386/486        == standard Bryd
              case 2: cindex = 1; break; // PII/PIII       == movzx Bryd
              case 3: cindex = 1; break; // Cx6x86, AMD K7 == movzx Bryd
              case 4: cindex = 0; break; // K5             == standard Bryd
              case 5: cindex = 1; break; // K6             == movzx Bryd
              case 6: cindex = 0; break; // Cx486          == movzx Bryd
              case 7: cindex = 1; break; // orig Celeron   == movzx Bryd
              case 8: cindex = 1; break; // PPro           == movzx Bryd
              //no default
            }
          }
          selcorestatics.corenum[DES] = cindex;
        }
      }
    }
    else if (contestid == CSC)
    {
      selcorestatics.corenum[CSC] = selcorestatics.user_cputype[CSC];
      if (selcorestatics.corenum[CSC] < 0)
      {
        if (detected_type >= 0)
        {
          int cindex = -1; 
          // this is only valid for nasm'd cores or GCC 2.95 and up
          switch ( detected_type & 0xff )
          {
            case 0: cindex = 3; break; // P5             == 1key - called
            case 1: cindex = 3; break; // 386/486        == 1key - called
            case 2: cindex = 2; break; // PII/PIII       == 1key - inline
            case 3: cindex = 0; break; // Cx6x86, AMD K7 == 6bit - inline
            case 4: cindex = 2; break; // K5             == 1key - inline
            case 5: cindex = 0; break; // K6/K6-2/K6-3   == 6bit - inline
            case 6: cindex = 3; break; // Cyrix 486      == 1key - called
            case 7: cindex = 3; break; // orig Celeron   == 1key - called
            case 8: cindex = 3; break; // PPro           == 1key - called
            //no default
          }
          selcorestatics.corenum[CSC] = cindex;
        }
      }
    }
  }
  #elif (CLIENT_CPU == CPU_ARM)
  if (contestid == RC5 || contestid == DES)
  {
    selcorestatics.corenum[contestid] = selcorestatics.user_cputype[contestid];
    if (selcorestatics.corenum[contestid] < 0)
    {
      if (detected_type >= 0)
        selcorestatics.corenum[contestid] = (int)detected_type;
    }
    /* otherwise fall into bench */
  }
  #endif

  if (selcorestatics.corenum[contestid] < 0)
    selcorestatics.corenum[contestid] = selcorestatics.user_cputype[contestid];

  if (selcorestatics.corenum[contestid] >= 0) 
  { 
    if (!corename_printed)
    {
      LogScreen("%s: using core #%d (%s).\n", contname, 
         selcorestatics.corenum[contestid], 
         selcoreGetDisplayName(contestid, selcorestatics.corenum[contestid]) );
     }
  }
  else /* ok, bench it then */
  {
    int corecount = (int)__corecount_for_contest(contestid);
    selcorestatics.corenum[contestid] = 0;
    if (corecount > 0)
    {
      int whichcrunch;
      int saidmsg = 0, fastestcrunch = -1;
      unsigned long fasttime = 0;
      const u32 benchsize = 100000;
      Problem *problem = new Problem();

      for (whichcrunch = 0; whichcrunch < corecount; whichcrunch++)
      {
        ContestWork contestwork;
        unsigned long elapsed;
        selcorestatics.corenum[contestid] = whichcrunch;
        memset( (void *)&contestwork, 0, sizeof(contestwork));
        contestwork.crypto.iterations.lo = benchsize;
        if (problem->LoadState( &contestwork, contestid, 
                                benchsize, 0 /* unused */ ) == 0)
        {
          if (!saidmsg)
          {
            LogScreen("%s: Running micro-bench to select fastest core...\n", 
                      contname);
            saidmsg = 1;
          }                                
          problem->Run();
   
          elapsed = (((unsigned long)problem->runtime_sec) * 1000000UL)+
                    (((unsigned long)problem->runtime_usec));
          //printf("%s Core %d: %lu usec\n", contname,whichcrunch,elapsed);
    
          if (fastestcrunch < 0 || elapsed < fasttime)
          {
            fastestcrunch = whichcrunch; 
            fasttime = elapsed;
          }
        }
      }
      delete problem;

      if (fastestcrunch < 0) /* all failed */
      { 
        selcorestatics.corenum[contestid] = 0; /* don't bench again */
      }
      else
      {
        selcorestatics.corenum[contestid] = fastestcrunch;
        LogScreen("%s: selected core #%d (%s).\n", contname, fastestcrunch, 
                       selcoreGetDisplayName( contestid, fastestcrunch ) );
        corename_printed = 1;
      }
    }
  }
  
  return selcorestatics.corenum[contestid];
}

