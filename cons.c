#include <pvm3.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>

#define PROG "/home/i/infjpf02/pvm3/bin/LINUX/cons"
#define FICHA "/home/i/infjpf02/pvm3/bin/LINUX/matriza.dat"
#define FICHB "/home/i/infjpf02/pvm3/bin/LINUX/matrizb.dat"
#define FICHC "/home/i/infjpf02/pvm3/bin/LINUX/resultado"

int fila(int tid,int *tids,int celx,int cely) {
  int i;

  for (i=0;i<celx*cely;i++) if (tid==tids[i]) return(i/celx);
  return(-1);
}

int columna(int tid,int *tids,int celx,int cely) {
  int i;

  for (i=0;i<celx*cely;i++) if (tid==tids[i]) return(i/cely);
  return(-1);
}

int main(int argc,char *argv[]){

//char *fichA,*fichB,*fichC,
char *aux,**aux2,a,c='\n';
double **matrizA,**matrizB,**matrizC;
int *tids,*tids_t,tid,tidpadre,nodo,celx,cely,mA,nA,mB,nB;
int fil,col,base,desp,filrec,colrec,faltan,idx,idy,id1,id2,id3,M,N,NB,P,i,j,k;

tid=pvm_mytid();
tidpadre=pvm_parent();

if (tidpadre<0) {

if ((cely=atoi(argv[1]))==0) {
					printf("Las dimensiones de la malla deben ser enteros mayores que 0\n");
					pvm_exit();
					exit(0);
				}

if ((celx=atoi(argv[2]))==0) {
					printf("Las dimensiones de la malla deben ser enteros mayores que 0\n");
					pvm_exit();
					exit(0);
				}
/*
fichA=(char *) malloc((strlen(argv[3])+43)*sizeof(char));
strcpy(fichA,"/home/i/infjpf02/pvm3/bin/LINUX/");
strcat(fichA,argv[3]);
*/
if ((id1=open(FICHA,O_RDONLY,0777))==-1) {
						printf("No se encuentra el archivo %s\n",FICHA);
						pvm_exit();
						exit(0);
					}	

/*
fichB=(char *) malloc((strlen(argv[4])+43)*sizeof(char));
strcpy(fichB,"/home/i/infjpf02/pvm3/bin/LINUX/"); 
strcat(fichB,argv[4]);
*/
if ((id2=open(FICHB,O_RDONLY,0777))==-1) {
						printf("No se encuentra el archivo %s\n",FICHB);
						pvm_exit();
						exit(0);
					}	
					
/*
aux=(char *) malloc(2*sizeof(char));
for (i=0;i<2;i++) {
			read(id1,&aux[i],1);
			if (aux[i]=='\n') break;
		}
M=atoi(aux);

for (i=0;i<2;i++) {
			read(id1,&aux[i],1);
			if (aux[i]=='\n') break;
		}
N=atoi(aux);

for (i=0;i<2;i++) {
			read(id2,&aux[i],1);
			if (aux[i]=='\n') break;
		}
NB=atoi(aux);

for (i=0;i<2;i++) {
			read(id2,&aux[i],1);
			if (aux[i]=='\n') break;
		}
P=atoi(aux);

*/
M=10;
N=8;
NB=8;
P=12;

/*
printf("%d %d %d %d\n",M,N,NB,P);

if (N!=NB) {
		printf("No se pueden multiplicar las matrices\n");
		pvm_exit();
		exit(0);
	}
*/
matrizA=(double **) malloc(N*sizeof(double));
for (i=0;i<N;i++) matrizA[i]=(double *) malloc(M*sizeof(double));

matrizB=(double **) malloc(N*sizeof(double));
for (i=0;i<N;i++) matrizB[i]=(double *) malloc(P*sizeof(double));

aux=(char *) malloc((N+9)*sizeof(char));
aux2=(char **) malloc((N+1)*sizeof(char));
for (i=0;i<N;i++) aux2[i]=(char *) malloc(9*sizeof(char));


for (i=0;i<M;i++) {
			j=0;
			while(1){
						read(id1,&aux[j],1);
						if (aux[j]=='\n') break;
						j++;
					}
			aux2[0]=strtok(aux," ");
  			j=1; 
  			while(aux2[j++]=strtok(NULL," "));
  			for (j=0;j<N;j++) matrizA[i][j]=atof(aux2[j]);
  		}
  		
for (j=0;j<N;j++) {
			i=0;
			while(1){
						read(id2,&aux[i],1);
						if (aux[i]=='\n') break;
						i++;
					}
			aux2[0]=strtok(aux," ");
  			i=1; 
  			while(aux2[i++]=strtok(NULL," "));
  			for (i=0;i<P;i++) matrizB[i][j]=atof(aux2[i]);
  		}
  		
/* 		 
fichC=(char *) malloc((strlen(argv[5])+43)*sizeof(char));
strcpy(fichC,"/home/i/infjpf02/pvm3/bin/LINUX/"); 
strcat(fichC,argv[5]);
*/
if ((id3=open(FICHC,O_WRONLY|O_CREAT|O_TRUNC,0777))==-1) {
								printf("No tiene permiso para crear un fichero en este directorio\n");
								close(id1);
								close(id2);
								pvm_exit();
								exit(0);
							}					
							
matrizC=(double **) malloc(P*sizeof(double));
for (i=0;i<P;i++) matrizC[i]=(double *) malloc(M*sizeof(double));

for (i=0;i<M;i++) for (j=0;j<P;j++) matrizC[i][j]=0;
							
tids=(int*)malloc(celx*cely*sizeof(int));
tids[0]=tid;

/*     
aux=(char *) malloc(40*sizeof(char));
strcpy("/home/i/infjpf02/pvm3/bin/LINUX/",aux);
strcat(argv[0],aux);
*/
if (celx*cely!=1) pvm_spawn(PROG,(char **) 0,0,"",celx*cely-1,&tids[1]);

tids_t=(int*)malloc(celx*cely*sizeof(int));
j=0;
for (k=0;k<celx;k++) for (i=k;i<celx*cely;i=i+celx) tids_t[j++]=tids[i];
						
mA=(int)ceil(M*1.0/cely);
nA=(int)ceil(N*1.0/celx);
mB=(int)ceil(P*1.0/celx);
nB=(int)ceil(N*1.0/cely);
		
//Parte de matriz del padre
for (i=0;i<mA;i++) for (j=0;j<mB;j++) for (k=0;k<N;k++) matrizC[i][j]=matrizC[i][j]+matrizA[i][k]*matrizB[j][k];

filrec=0; //Nodos que recibieron filas
colrec=0; //Nodos que recibieron columnas	

if ((celx*cely-1)!=0) {

	//Primero se envian las filas de A a las filas de la malla

	for (i=0;i<cely;i++) {
		if (pvm_initsend(PvmDataDefault)==-1) {
							printf("Error al inicializar el buffer de envio\n");
							pvm_exit();
							exit(0);
						}	
 
		
		if (i == 0) { //Se envian mA filas de A a la fila del nodo 0
				faltan=M-mA;
				pvm_pkint(&mA,1,1);
				pvm_pkint(&N,1,1);
				for (j=0;j<mA;j++) pvm_pkdouble(&matrizA[j][0],N,1);
				pvm_mcast(&tids[1],celx-1,0);
				filrec++;
			}
			else if (faltan>=mA) { //Se envian mA filas de A
				 	faltan=faltan-mA;
					pvm_pkint(&mA,1,1);
					pvm_pkint(&N,1,1);
					for (j=i*mA;j<(i+1)*mA;j++) pvm_pkdouble(&matrizA[j][0],N,1);
					pvm_mcast(&tids[i*celx],celx,0);
					filrec++;
				}
				else if (faltan>0) { //Se envian el resto de filas
							pvm_pkint(&faltan,1,1);
	  						pvm_pkint(&N,1,1);
 							for (j=i*mA;j<i*mA+faltan;j++) pvm_pkdouble(&matrizA[j][0],N,1);
	  						faltan=0;
	  						pvm_mcast(&tids[i*celx],celx,0);
	  						filrec++;
						}
					else {
	  					pvm_pkint(&faltan,1,1); //No se envian filas
	  					pvm_mcast(&tids[i*celx], celx, 0);
					}
	}

	//Ahora se envian las filas traspuestas de B a las columnas de la malla

	for (i=0;i<celx;i++) {
		if (pvm_initsend(PvmDataDefault)==-1) {
					printf("Error al inicializar el buffer de envio\n");
	  				pvm_exit();
	  				exit(0);
				}
		if (i==0) { //Se envian mB filas de B traspuesta a la columna del nodo 0
				faltan=P-mB;
	  			pvm_pkint(&mB,1,1);
	  			for (j=0;j<mB;j++) pvm_pkdouble(&matrizB[j][0],N,1);
	  			pvm_mcast(&tids_t[1],cely-1,1);
	  			colrec++;
			}
			else if (faltan>=mB) { //Se envian mB filas de B traspuesta
	  					faltan=faltan-mB;
	  					pvm_pkint(&mB,1,1);
	  					for (j=i*mB;j<(i+1)*mB;j++) pvm_pkdouble(&matrizB[j][0],N,1);
	  					pvm_mcast(&tids_t[i*cely],cely,1);
	  					colrec++;
					}
				else if (faltan>0) { //Se envian el resto de filas
	  						pvm_pkint(&faltan,1,1);
	  						for (j=i*mB;j<i*mB+faltan;j++) pvm_pkdouble(&matrizB[j][0],N,1);
	  						faltan=0;
	  						pvm_mcast(&tids_t[i*cely],cely,1);
	  						colrec++;
						}
					else {
	  					pvm_pkint(&faltan,1,1); //No se envian filas
	  					pvm_mcast(&tids_t[i*cely],cely,1);
					}
	}						

	//Ahora se recibe la respuesta de los hijos

	for (i=0;i<(filrec*colrec)-1;i++) {
				pvm_recv(-1,2);
				pvm_upkint(&nodo,1,1);
				pvm_upkint(&fil,1,1);
				pvm_upkint(&col,1,1);
				if (fil*col!=0) {
	  				base=fila(nodo,tids,celx,cely)*mA;
	  				desp=columna(nodo,tids_t,celx,cely)*mB;
	  				if (base==-1) {
	    					printf("Error en la recepcion de datos\n");
	    					pvm_exit();
	    					exit(0);
	  				}
	  				for (j=0;j<fil;j++) pvm_upkdouble(&matrizC[base+j][desp],col,1);
				}
      			}
}

//Ahora se escribe la matriz

for (i=0;i<M;i++) {
			for (j=0;j<P;j++) {
						sprintf(aux,"%f ",matrizC[i][j]);
						for (k=0;k<strlen(aux);k++) write(id3,&aux[k],1);
						printf("%f ");
					}
			write(id3,&c,1);
			printf("\n");
		}
	
pvm_exit();
exit(0);

}

else { //Hijos

	if (pvm_initsend(PvmDataDefault)==-1) {
      		printf("Error al inicializar el buffer de envio\n");
      		pvm_exit();
      		exit(0);
    	}
    
    	//Se reciben las filas de A
    	
	pvm_recv(tidpadre,0);
    	pvm_upkint(&fil,1,1);
    	if (fil==0) { //El nodo no trabaja
      		pvm_exit();
      		exit(0);
    	}
    	pvm_upkint(&N,1,1);
    	
   	matrizA=(double **) malloc(N*sizeof(double));
	for (i=0;i<N;i++) matrizA[i]=(double *) malloc(fil*sizeof(double));
    
        for (j=0;j<fil;j++) pvm_upkdouble(&matrizA[j][0],N,1);
        
    	//Se reciben las filas de B tras
