/*
*    SPMD example using PVM 3.0
*/

#define NPROC 4

#include <sys/types.h>
#include <pvm3.h>

main()
{
    int mytid;                  /* my task id */
    int tids[NPROC];            /* array of task id */
    int me;                     /* my process number */
    int i;

    /* enroll in pvm */
    mytid = pvm_mytid();

    /* find out if I am parent or child */
    tids[0] = pvm_parent();

    printf ("\nEl valor del padre es %d",tids[0]);
    if( tids[0] < 0 )       /* then I am the parent */
    {
       tids[0] = mytid;
       me = 0;
       /* start up copies of myself */
       pvm_spawn("spmd", (char**)0, 0, "", NPROC-1, &tids[1]);

       /* multicast tids array to children */
       pvm_initsend( PvmDataDefault );
       pvm_pkint(tids, NPROC, 1);
       pvm_mcast(&tids[1], NPROC-1, 0);
    }
    else    /* I am a child */
    {
       /* receive tids array */
       pvm_recv(tids[0], 0);
       pvm_upkint(tids, NPROC, 1);
       for( i=1; i<NPROC ; i++ )
          if( mytid == tids[i] ){ me = i; break; }
     }
/*-----------------------------------------------------------
 * all NPROC tasks are equal now
 * and can address each other by tids[0] thru tids[NPROC-1]
 * for each process me => process number [0-(NPROC-1)]
 *-----------------------------------------------------------
 */
     printf("me = %d mytid = %d\n",me,mytid);
     dowork( me, tids, NPROC );

     /* program finished exit pvm */
     pvm_exit();
     exit(1);
}

/* Simple example passes a token around a ring */

dowork( me, tids, nproc )
     int me;
     int *tids;
     int nproc;
{
     int token;
     int dest;
     int count  = 1;
     int stride = 1;
     int msgtag = 4;

     if( me == 0 )
     { 
        token = tids[0];
        pvm_initsend( PvmDataDefault );
        pvm_pkint( &token, count, stride );
        pvm_send( tids[me+1], msgtag );
        pvm_recv( tids[nproc-1], msgtag );
        printf("token ring done\n");
     }
     else
     {
        pvm_recv( tids[me-1], msgtag );
        pvm_upkint( &token, count, stride );
        pvm_initsend( PvmDataDefault );
        pvm_pkint( &token, count, stride );
        dest = (me == nproc-1)? tids[0] : tids[me+1] ;
        pvm_send( dest, msgtag );
     }
}
