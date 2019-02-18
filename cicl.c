#include <pvm3.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>

#define PROG "/home/i/infjpf02/pvm3/bin/LINUX/cicl"
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

char *fichA,*fichB,*fichC,*aux,**aux2,c='\n';
double **matrizA,**matrizB,**matrizC;
double *matrizaux;
int *tids,*tids_t,tid,tidpadre,nodo,celx,cely;
int fil,col,base,desp,filrec,colrec,faltan,idx,idy,id1,id2,id3,M,N,NB,P,i,j,k,l;

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

fichA=(char *) malloc((strlen(argv[3])+43)*sizeof(char));
strcpy(fichA,"/home/i/infjpf02/pvm3/bin/LINUX/"); 
strcat(fichA,argv[3]);
if ((id1=open(fichA,O_RDONLY,0777))==-1) {
						printf("No se encuentra el archivo %s\n",argv[3]);
						pvm_exit();
						exit(0);
					}	


fichB=(char *) malloc((strlen(argv[4])+43)*sizeof(char));
strcpy(fichB,"/home/i/infjpf02/pvm3/bin/LINUX/"); 
strcat(fichB,argv[4]);
if ((id2=open(fichB,O_RDONLY,0777))==-1) {
						printf("No se encuentra el archivo %s\n",argv[4]);
						pvm_exit();
						exit(0);
					}	
					
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

if (N!=NB) {
		printf("No se pueden multiplicar las matrices\n");
		pvm_exit();
		exit(0);
	}

matrizA=(double **) malloc(N*sizeof(double));
for (i=0;i<N;i++) matrizA[i]=(double *) malloc(M*sizeof(double));

matrizB=(double **) malloc(N*sizeof(double));
for (i=0;i<N;i++) matrizB[i]=(double *) malloc(P*sizeof(double));

aux=(char *) malloc((9*N)*sizeof(char));
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
  		
 		 
fichC=(char *) malloc((strlen(argv[5])+43)*sizeof(char));
strcpy(fichC,"/home/i/infjpf02/pvm3/bin/LINUX/"); 
strcat(fichC,argv[5]);
if ((id3=open(fichC,O_WRONLY|O_CREAT|O_TRUNC,0777))==-1) {
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
    
aux=(char *) malloc(40*sizeof(char));
strcpy("./",aux);
strcat(argv[0],aux); 
if (celx*cely!=1) pvm_spawn(argv[0],(char **) 0,0,"",celx*cely-1,&tids[1]);

tids_t=(int*)malloc(celx*cely*sizeof(int));
j=0;
for (k=0;k<celx;k++) for (i=k;i<celx*cely;i=i+celx) tids_t[j++]=tids[i];
						
//Parte de matriz del padre

for (i=0;i<M;i=i+cely) for (j=0;j<P;j=j+celx) for (k=0;k<N;k++) matrizC[i][j]=matrizC[i][j]+matrizA[i][k]*matrizB[j][k];
      
filrec=cely;
if (cely>M) filrec=M;
 
colrec=celx;
if (celx>P) colrec=P; 	

if ((celx*cely-1)!=0) {
      
	//Primero se envian las filas de A a las filas de la malla
      
	for (i=0;i<cely;i++) { 
		if (pvm_initsend(PvmDataDefault)==-1) {
	  		printf("Error al inicializar el buffer de envio\n");
	  		pvm_exit();
	  		exit(0);
		}
		faltan=0;
		for (j=i;j<M;j=j+cely) faltan++;
	
		pvm_pkint(&faltan,1,1);
		pvm_pkint(&N,1,1);
		for (j=i;j<M;j=j+cely) pvm_pkdouble(&matrizA[j][0],N,1);
		pvm_mcast(&tids[i*celx],celx,0);
      }
      
	//Ahora se envian las filas traspuestas de B a las columnas de la malla

      	for (i=0;i<celx;i++) { 
		if (pvm_initsend(PvmDataDefault)==-1) {
	    		printf("Error al inicializar el buffer de envio\n");
	    		pvm_exit();
	    		exit(0);
		}
		faltan=0;
		for (j=i;j<P;j=j+celx) faltan++;
	
		pvm_pkint(&faltan,1,1);
		for (j=i;j<P;j=j+celx) pvm_pkdouble(&matrizB[j][0],N,1);
		pvm_mcast(&tids_t[i*cely],cely,1);
      	}
      
      
      	//Ahora se recibe la respuesta de los hijos
      
	for (i=0;i<(filrec*colrec)-1;i++) {
		pvm_recv(-1,2);
		pvm_upkint(&nodo,1,1);
	
		base=fila(nodo,tids,celx,cely);
		desp=columna(nodo,tids_t,celx,cely);	
	
		if (base==-1) {
	  		printf("Error en la recepcion de datos\n");
	  		pvm_exit();
	  		exit(0);
		}
	
		for (j=base;j<M;j=j+cely) for (k=desp;k<P;k=k+celx) pvm_upkdouble(&matrizC[j][k],1,1);
      }
}

//Ahora se escribe la matriz

for (i=0;i<M;i++) {
			for (j=0;j<P;j++) {
						sprintf(aux,"%f ",matrizC[i][j]);
						for (k=0;k<strlen(aux);k++) write(id3,&aux[k],1);
					}
			write(id3,&c,1);
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
    	if (fil==0) { //Este nodo no trabaja
      		pvm_exit();
      		exit(0);
    	}
    	pvm_upkint(&N,1,1);
       	
	matrizA=(double **) malloc(N*sizeof(double));
	for (i=0;i<N;i++) matrizA[i]=(double *) malloc(fil*sizeof(double));
    
        for (j=0;j<fil;j++) pvm_upkdouble(&matrizA[j][0],N,1);
    	
    	//Se reciben las filas de B traspuestas

    	pvm_recv(tidpadre,1);
    	pvm_upkint(&col,1,1);
    	if (col==0) { //Este nodo no trabaja
            	pvm_exit();
      		exit(0);
    	}

    	matrizB=(double **) malloc(N*sizeof(double));
	for (i=0;i<N;i++) matrizB[i]=(double *) malloc(col*sizeof(double));

    	for (j=0;j<col;j++) pvm_upkdouble(&matrizB[j][0],N,1);
    

    	matrizaux=(double *) malloc(fil*col*sizeof(double));
	
        for (i=0;i<fil*col;i++) matrizaux[i]=0;

        l=0;
    	for (i=0;i<fil;i++) for (j=0;j<col;j++) {  
					for (k=0;k<N;k++) matrizaux[l]=matrizaux[l]+matrizA[i][k]*matrizB[j][k];
					l++;
      				}

        pvm_pkint(&tid,1,1);    
        pvm_pkdouble(&matrizaux[0],fil*col,1);
    	pvm_send(tidpadre,2);
        pvm_exit();
    	exit(0);
  }
	
}
	
