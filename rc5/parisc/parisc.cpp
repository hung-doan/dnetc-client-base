#include "problem.h"

#define P     0xB7E15163
#define Q     0x9E3779B9

#define S_not(n)      P+Q*n

class SnotN {
	static SnotN SnotN_;
	SnotN();
};

SnotN	SnotN::SnotN_;

unsigned long		S_not_n[26];

#if signal_debug_980508!=hamajima
#include <stdio.h>
#include <signal.h>

void catchSignal(int sig){
	fprintf(stderr,"catch signal : %d\n", sig);
}
#endif

SnotN::SnotN(){
#if signal_debug_980508!=hamajima
	signal(SIGINT,catchSignal);
	signal(SIGTERM,catchSignal);
	signal(SIGTSTP,catchSignal);
#endif
	S_not_n[0]=0xBF0A8B1D; // =ROTL3(S_not(0))
	S_not_n[1]=0x15235639; // =S_not(1)+S1[0]
	for( int i=2; i<26; i++ )
		S_not_n[i]=S_not(i);
}

extern u32	rc5_unit_func( RC5UnitWork* );
#define PIPELINE_COUNT 2

u32 rc5_parisc_unit_func ( RC5UnitWork * rc5unitwork, u32 iterations )
{                                
  u32 kiter = 0;
  int keycount = iterations;
  int pipeline_count = PIPELINE_COUNT;
  
  //fprintf (stderr, "rc5unitwork = %08X:%08X (%X)\n", rc5unitwork.L0.hi, rc5unitwork.L0.lo, keycount);
  while ( keycount-- ) // iterations ignores the number of pipelines
  {
    u32 result = rc5_unit_func( rc5unitwork );
    if ( result )
    {
      kiter += result-1;
      break;
    }
    else
    {
      /* note: we switch the order */  
      register u32 tempkeylo = rc5unitwork->L0.hi; 
      register u32 tempkeyhi = rc5unitwork->L0.lo;
      rc5unitwork->L0.lo =
        ((tempkeylo >> 24) & 0x000000FFL) |                               
        ((tempkeylo >>  8) & 0x0000FF00L) |                               
        ((tempkeylo <<  8) & 0x00FF0000L) |                               
        ((tempkeylo << 24) & 0xFF000000L);                                
      rc5unitwork->L0.hi = 
        ((tempkeyhi >> 24) & 0x000000FFL) |                               
        ((tempkeyhi >>  8) & 0x0000FF00L) |                               
        ((tempkeyhi <<  8) & 0x00FF0000L) |                               
        ((tempkeyhi << 24) & 0xFF000000L);                                
      rc5unitwork->L0.lo += pipeline_count;
      if (rc5unitwork->L0.lo < ((u32)pipeline_count))
        rc5unitwork->L0.hi++;
      tempkeylo = rc5unitwork->L0.hi; 
      tempkeyhi = rc5unitwork->L0.lo;
      rc5unitwork->L0.lo =
        ((tempkeylo >> 24) & 0x000000FFL) |                               
        ((tempkeylo >>  8) & 0x0000FF00L) |                               
        ((tempkeylo <<  8) & 0x00FF0000L) |                               
        ((tempkeylo << 24) & 0xFF000000L);                                
      rc5unitwork->L0.hi = 
        ((tempkeyhi >> 24) & 0x000000FFL) |                               
        ((tempkeyhi >>  8) & 0x0000FF00L) |                               
        ((tempkeyhi <<  8) & 0x00FF0000L) |                               
        ((tempkeyhi << 24) & 0xFF000000L);                                
      kiter += pipeline_count;
    }
  }
  return kiter;
}  