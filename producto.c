#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>


int main(int argc,char *argv[]){

char *fichA,*fichB,*fichC,*aux,**aux2,a,c='\n';
float **matrizA,**matrizB,**matrizC,f;
int id1,id2,id3,xA,yA,xB,yB,i,j,k;

fichA=(char *) malloc((strlen(argv[2])+3)*sizeof(char));
strcpy(fichA,"."); 
strcat(fichA,"/");
strcat(fichA,argv[2]);
if ((id1=open(fichA,O_RDONLY,0777))==-1) {
						printf("No se encuentra el archivo %s\n",argv[2]);
						exit(0);
					}	


fichB=(char *) malloc((strlen(argv[3])+3)*sizeof(char));
strcpy(fichB,"."); 
strcat(fichB,"/");
strcat(fichB,argv[3]);
if ((id2=open(fichB,O_RDONLY,0777))==-1) {
						printf("No se encuentra el archivo %s\n",argv[3]);
						exit(0);
					}	
					
aux=(char *) malloc(2*sizeof(char));
for (i=0;i<2;i++) {
			read(id1,&aux[i],1);
			if (aux[i]=='\n') break;
		}
xA=atoi(aux);

for (i=0;i<2;i++) {
			read(id1,&aux[i],1);
			if (aux[i]=='\n') break;
		}
yA=atoi(aux);

for (i=0;i<2;i++) {
			read(id2,&aux[i],1);
			if (aux[i]=='\n') break;
		}
yB=atoi(aux);

for (i=0;i<2;i++) {
			read(id2,&aux[i],1);
			if (aux[i]=='\n') break;
		}
xB=atoi(aux);

if (yA!=yB) {
		printf("No se pueden multiplicar las matrices\n");
		exit(0);
	}

matrizA=(float **) malloc(yA*sizeof(float));
for (i=0;i<yA;i++) matrizA[i]=(float *) malloc(xA*sizeof(float));

matrizB=(float **) malloc(xB*sizeof(float));
for (i=0;i<xB;i++) matrizB[i]=(float *) malloc(yB*sizeof(float));

aux=(char *) malloc((yA+9)*sizeof(char));
aux2=(char **) malloc((yA+1)*sizeof(char));
for (i=0;i<yA;i++) aux2[i]=(char *) malloc(9*sizeof(char));


for (i=0;i<xA;i++) {
			j=0;
			while(1){
						read(id1,&aux[j],1);
						if (aux[j]=='\n') break;
						j++;
					}
			aux2[0] = strtok(aux," ");
  			j=1; 
  			while(aux2[j++]=strtok(NULL," "));
  			for (j=0;j<yA;j++) matrizA[i][j]=atof(aux2[j]);
  		}
  		
for (j=0;j<yB;j++) {
			i=0;
			while(1){
						read(id2,&aux[i],1);
						if (aux[i]=='\n') break;
						i++;
					}
			aux2[0] = strtok(aux," ");
  			i=1; 
  			while(aux2[i++]=strtok(NULL," "));
  			for (i=0;i<xB;i++) matrizB[i][j]=atof(aux2[i]);
  		}
  		
 		 
fichC=(char *) malloc((strlen(argv[4])+3)*sizeof(char));
strcpy(fichC,"."); 
strcat(fichC,"/");
strcat(fichC,argv[4]);
if ((id3=open(fichC,O_WRONLY|O_CREAT|O_TRUNC,0777))==-1) {
								printf("codifica: ERROR: No tiene permiso para crear un fichero en este directorio\n");
								close(id1);
								close(id2);
								exit(0);
							}					
							
matrizC=(float **) malloc(xB*sizeof(float));
for (i=0;i<xB;i++) matrizC[i]=(float *) malloc(xA*sizeof(float));

for (i=0;i<xA;i++) {
			for (j=0;j<yA;j++) {
								f=0;
								for (k=0;k<yB;k++) f=f+matrizA[i][k]*matrizB[j][k];
								matrizC[i][j]=f;
							}
			} 
for (i=0;i<xA;i++) {
			for (j=0;j<yB;j++) {
								sprintf(aux,"%f ",matrizC[i][j]);
								for (k=0;k<strlen(aux);k++)	write(id3,&aux[k],1);
								printf("%f ",matrizC[i][j]);
							}
			write(id3,&c,1);
			printf("\n");
		}
							
if (!(strcmp(argv[1],"-c"))) {
						
}
	else {
		
	}
	
}
	