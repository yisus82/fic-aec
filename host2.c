#include <stdio.h>
#include <malloc.h>
#include "pvm3.h"


void malla(int *tids,int x, int y, int numproc)
{
pvm_initsend(PvmDataDefault);
pvm_pkint(&x,1,1);
pvm_pkint(&y,1,1);
pvm_pkint(tids,numproc,1);
pvm_mcast(tids,numproc,0);
}

int main(int argc, char *argv[]) {

int tid,*tids,numiter,i,cont,numproc,x,y;
double globalpi,pi;

tid=pvm_mytid();
x=atoi(argv[1]);
y=atoi(argv[2]);
numiter=atoi(argv[3]);
numproc=x*y;
tids=(int*)malloc(numproc*sizeof(int));
pvm_spawn("pi_nodos2",(char**)0, 0, "",numproc,tids);
malla(tids,x,y,numproc);
pvm_psend(tids[0],0,&numiter,1,PVM_INT);
pvm_precv(tids[0],0,&pi,1,PVM_DOUBLE,NULL,NULL,NULL);
printf("\nValor local de PI en el procesador 0: %f",pi);
pvm_precv(tids[0],0,&cont,1,PVM_INT,NULL,NULL,NULL);
globalpi=(4.0*cont)/(numproc*numiter);
printf("\nValor global de PI: %f\n",globalpi);
pvm_exit();

}
