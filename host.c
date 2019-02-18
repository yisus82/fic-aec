#include <stdio.h>
#include <malloc.h>
#include "pvm3.h"

int main(int argc,int *argv[]) {

int tid,*tids,numiter,i,numproc,nums[2],cont=0;
double pi;

nums[0]=numproc=atoi(argv[1]);
nums[1]=numiter=atoi(argv[2]);
tid=pvm_mytid();
tids=(int *)malloc(numproc*sizeof(int));
pvm_joingroup("grupo_pi");
pvm_spawn("pi_nodos",(char **)0,0,"",numproc,tids);
pvm_initsend(PvmDataRaw);
pvm_pkint(nums,2,1);
pvm_mcast(tids,numproc,0);

pvm_barrier("grupo_pi",numproc+1); //para que el padre espere por los datos de los hijos
pvm_reduce(PvmSum,&cont,1,PVM_INT,0,"grupo_pi",0);
pvm_barrier("grupo_pi",numproc+1); // hijos esperando para no perder datos

pi=4.0*cont/(numproc*numiter);
printf("Valor global de PI: %f\n",pi);
pvm_exit();

}
