#include <stdio.h>
#include <math.h>
#include "pvm3.h"
#define RAND_MAX 2147483647

int calculoPI(int tid,int numiter) {

double x,y;
int cont=0,i;

srand(tid+1);
for(i=0;i<numiter;i++){
			x=((double)rand())/((double)RAND_MAX);
			y=((double)rand())/((double)RAND_MAX);
			if((x*x+y*y)<=1) cont++;
 		}
return(cont);

}




main(){

int tid,numiter,numproc,nums[2],cont;
double pi=0;

tid=pvm_mytid();
pvm_joingroup("grupo_pi");
pvm_precv(-1,0,nums,2,PVM_INT,NULL,NULL,NULL);
numproc=nums[0];
numiter=nums[1];
cont=calculoPI(tid,numiter);
pi=4.0*cont/numiter;
printf("\nValor local de PI: %f\n",pi);
pvm_barrier("grupo_pi",numproc+1); //sincronizacion
pvm_reduce(PvmSum,&cont,1,PVM_INT,0,"grupo_pi",0);
pvm_barrier("grupo_pi",numproc+1); //seguridad
pvm_exit();

}
