#include <stdio.h>
#include <malloc.h>
#include "pvm3.h"

#define RAND_MAX 2147483647
#define recibir(origen,entero) pvm_precv(origen,0,&entero,1,PVM_INT,NULL,NULL,NULL)
#define enviar(destino,entero) pvm_psend(destino,0,&entero,1,PVM_INT);
#define arriba(pos) pos-1
#define abajo(pos) pos+1
#define derecha(pos) pos+y
#define izquierda(pos) pos-y

int x,y,*tids,id;

int CalculoPI(int id,int numiter) {

double x, y;
int cont=0,i;
    
srand(id + 1);
for (i=0;i<numiter;i++) {
        			x=((double)rand())/((double)RAND_MAX);
        			y=((double)rand())/((double)RAND_MAX);
        			if ((x*x+y*y)<=1) cont++;
    			}
return (cont);

}

void malla(int tid){
  
int numproc;
pvm_recv(pvm_parent() ,0);
pvm_upkint(&x, 1, 1);
pvm_upkint(&y, 1, 1);
numproc=x*y;
tids=(int*)malloc(numproc*sizeof(int));
pvm_upkint(tids,numproc,1);
for(id=0;id<numproc;id++) if(tids[id]==tid) break;

}

main() {

int tid,numiter,cont,contrec=0,globalcont;
double pi;

tid = pvm_mytid();
malla(tid);

if(id==0) {
    recibir(pvm_parent(),numiter);
    if (y!=1) enviar(tids[abajo(id)],numiter);
  }

if (y!=1) {
	if((id > 0) && (id<(y-1))) {
    		recibir(tids[arriba(id)],numiter);
    		enviar(tids[abajo(id)],numiter);
  	}
  
	if(id==(y-1)) recibir(tids[arriba(id)],numiter);
}

if (x!=1) {  
	if((id/y)==0) enviar(tids[derecha(id)],numiter);

	if(((id/y)>0) && ((id/y)<(x-1))) {
    		recibir(tids[izquierda(id)],numiter);
    		enviar(tids[derecha(id)],numiter);
  	}
  
	if((id/y)==(x-1)) recibir(tids[izquierda(id)],numiter);
}

cont=CalculoPI(id,numiter);

if(id==0) {
    pi=(4.0*cont)/numiter;
    pvm_initsend(PvmDataDefault);
    pvm_psend(pvm_parent(),0,&pi,1,PVM_DOUBLE);
    if ((x==1) && (y==1)) pvm_psend(pvm_parent(), 0, &cont, 1, PVM_DOUBLE);  
}

if (x!=1) {
	if((id/y)==0) {
    		recibir(tids[derecha(id)],contrec);
    		cont=cont+contrec;
  	}
      
	if (y==1) enviar(pvm_parent(),cont);
}

if(((id/y)>0) && ((id/y)<(x-1))) {
    recibir(tids[derecha(id)],contrec);
    cont=cont+contrec;
    enviar(tids[izquierda(id)],cont);
  }
  
if((id/y)==(x-1)) enviar(tids[izquierda(id)],cont);
  
if (y!=1) {
	if(id==0) {
    		recibir(tids[abajo(id)],contrec);
    		globalcont=cont+contrec;
    		enviar(pvm_parent(),globalcont);
  	}
  
	if((id>0) && (id<(y-1))) {
    		recibir(tids[abajo(id)],contrec);
    		cont=cont+contrec;
    		enviar(tids[arriba(id)],cont);
  	}
  
	if(id==(y-1)) enviar(tids[arriba(id)],cont);
}

pvm_exit();

}
