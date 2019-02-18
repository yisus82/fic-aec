#include <pvm3.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

//Calcula el minimo entre dos numeros
int min (int a, int b) {
  if (a < b) return a;
  else return b;
}

//Obtienen los indices locales dentro de una celda
int get_i (int I, int m) {
  return (I%m);
}

int get_j (int J, int n) {
  return (J%n);
}

//Obtienen los indices globales a partir de los locales de una celda
int get_I (int idy, int m, int i) {
  return (idy * m + i);
}

int get_J (int idx, int n, int j) {
  return (idx * n + j);
}

//Devuelve la fila de la malla en la que esta un cierto nodo
int get_idy(int tid, int * tids, int celx, int cely) {
  int i;

  for (i = 0; i < celx * cely; i++) {
    if (tid == tids[i]) return(i / celx);
  }

  return(-1);
}

//Devuelve la columna de la malla en la que esta un cierto nodo
int get_idx(int tid, int * tids, int celx, int cely) {
  int i;

  for (i = 0; i < celx * cely; i++) {
    if (tid == tids[i]) return(i / cely);
  }

  return(-1);
}

//Transpone el vector de tids
void transponer(int *tids, int *tids_t, int celx, int cely) {
  int i,j,k;

  j = 0;
  for (k = 0; k < celx; k++)
    for (i = k; i < celx * cely; i+= celx)
      tids_t[j++] = tids[i];
}

//Lee el tamaño de una matriz de un fichero
int leetamano(char *ruta, int *filas, int *columnas) {
  FILE *fichero;
  int i, j;

  fichero = fopen(ruta, "r");
  if (fichero == NULL) {
    perror("ERROR");
    return (-1);
  }
  fscanf(fichero, "%d", filas);
  fscanf(fichero, "%d", columnas);
  fclose(fichero);
  return 0;
}

//Lee la matriz de un fichero
//Si t = 0 la lectura se hace normalmente
//Si t = 1 la matriz se lee traspuesta (debe introducir filas y columnas al reves)
int leematriz(char *ruta, double **matriz, int t) {
  FILE *fichero;
  int i, j, filas, columnas;

  fichero = fopen(ruta, "r");
  if (fichero == NULL) return (-1);
  fscanf(fichero, "%d", &filas);
  fscanf(fichero, "%d", &columnas);
   if (t == 0) {
     for (i = 0; i < filas; i++) {
      for (j = 0; j < columnas; j++) {
	fscanf(fichero, "%lf", &matriz[i][j]);
      }
      printf("\n");
    }
  }
  else {
    for (i = 0; i < filas; i++) {
      for (j = 0; j < columnas; j++) {
	fscanf(fichero, "%lf", &matriz[j][i]);
      }
      printf("\n");
    }
  }
  fclose(fichero);
  return 0;
}

//Escribe una matriz en un fichero
int escribematriz(char *ruta, double **matriz, int filas, int columnas) {
  FILE *fichero;
  int i, j;

  fichero = fopen(ruta, "w+");
  if (fichero == NULL) return (-1);
  for (i = 0; i < filas; i++) {
      for (j = 0; j < columnas; j++) fprintf(fichero, "%lf ", matriz[i][j]);
      fprintf(fichero, "\n");
  }  
  fclose(fichero);
  return 0;
}

//Programa principal
int main (int argc, char * argv[]) {
  int mitid, celx, cely, M, N, N2, P, mA, nA, mB, nB, idx, idy, i, j, k, I, J;
  int padre, nodo, filas, columnas, base, desplaz, rfilas, rcols, contador, error;
  double **matrizA, **matrizB, **matrizC; //Nota: matrizB es la trapuesta de B
  int *tids, *tids_t;
  char aux[40];
 
  mitid = pvm_mytid();
  padre = pvm_parent();
  
  if (padre < 0) {
    //Comprobacion de los argumentos
    if (argc != 6) {
      printf("ERROR: Numero de argumentos incorrecto\n");
      printf("Uso: %s cely celx\n", argv[0]);
      printf("donde cely y celx son las dimensiones de la malla de procesadores\n");
      pvm_exit();
      exit(-1);
    }
    
    //Obtencion del numero de procesadores de la malla
    cely = atoi(argv[1]);
    if (cely == 0) {
      printf("ERROR: tipo de argumentos incorrecto\n");
      printf("Uso: %s cely celx\n", argv[0]);
      printf("donde cely y celx son las dimensiones de la malla de procesadores\n");
      pvm_exit();
      exit(-1);
    }
    celx = atoi(argv[2]);
    if (celx == 0) {
      printf("ERROR: tipo de argumentos incorrecto\n");
      printf("Uso: %s cely celx\n", argv[0]);
      printf("donde cely y celx son las dimensiones de la malla de procesadores\n");
      pvm_exit();
      exit(-1);
    }
 
    //Leemos las dimensiones de los ficheros
    error = leetamano(argv[3], &M, &N);
    if (error == -1) {
      printf("Error al leer las dimensiones en el fichero %s\n", argv[3]);
      printf("El fichero no pudo ser leido correctamente\n");
      pvm_exit();
      exit(-1);
    }

    error = leetamano(argv[4], &N2, &P);
    if (error == -1) {
      printf("Error al leer las dimensiones en el fichero %s\n", argv[4]);
      printf("El fichero no pudo ser leido correctamente\n");
      pvm_exit();
      exit(-1);
    }
      
    if (M == 0 || N == 0 || P == 0) {
      printf("Error al leer las dimensiones en un fichero\n");
      printf("Alguna de las dimensiones de alguna matriz es 0\n");
      pvm_exit();
      exit(-1);
    }

    if (N != N2) {
      printf("Error: las dimensiones de las matrices no concuerdan\n");
      pvm_exit();
      exit(-1);
    }

    //Reservamos memoria para la matriz A
    matrizA = (double**)malloc(M * sizeof(double*));
    if (matrizA == NULL) {
      printf("ERROR: Falta de memoria\n");
      pvm_exit();
      exit(-1);
    }
    for (i = 0; i < M; i++) { 
      matrizA[i] = (double*)malloc(N * sizeof(double));
      if (matrizA[i] == NULL) {
	printf("ERROR: Falta de memoria\n");
	pvm_exit();
	exit(-1);
      }
    }

    //Reservamos memoria para la matriz B traspuesta
    matrizB = (double**)malloc(P * sizeof(double*));
    if (matrizB == NULL) {
      printf("ERROR: Falta de memoria\n");
      pvm_exit();
      exit(-1);
    }
    for (i = 0; i < P; i++) { 
      matrizB[i] = (double*)malloc(N * sizeof(double));
      if (matrizB[i] == NULL) {
	printf("ERROR: Falta de memoria\n");
	pvm_exit();
	exit(-1);
      }
    }

    //Reservamos memoria para la matriz C (resultado)
    matrizC = (double**)malloc(M * sizeof(double*));
    if (matrizC == NULL) {
      printf("ERROR: Falta de memoria\n");
      pvm_exit();
      exit(-1);
    }
    for (i = 0; i < M; i++) { 
      matrizC[i] = (double*)malloc(P * sizeof(double));
      if (matrizC[i] == NULL) {
	printf("ERROR: Falta de memoria\n");
	pvm_exit();
	exit(-1);
      }
    }
    
    for (i = 0; i < M; i++)
      for (j = 0; j < P; j++) 
	matrizC[i][j] = 0;

    //Leemos las matrices A y B
    error = leematriz(argv[3], matrizA, 0);
    if (error == -1) {
      printf("Error al leer la matriz en el fichero %s\n", argv[3]);
      printf("El fichero no pudo ser leido correctamente\n");
      pvm_exit();
      exit(-1);
    }

    error = leematriz(argv[4], matrizB, 1);
    if (error == -1) {
      printf("Error al leer la matriz en el fichero %s\n", argv[4]);
      printf("El fichero no pudo ser leido correctamente\n");
      pvm_exit();
      exit(-1);
    }

    //Creacion del vector de tids
    tids = (int*)malloc(celx*cely*sizeof(int));
    if (tids == NULL) {
      printf("ERROR: Falta de memoria\n");
      pvm_exit();
      exit(-1);
    }
    tids[0] = mitid;
    
    strcpy("/home/i/infjpf02/pvm3/bin/LINUX/",aux);
strcat(argv[0],aux);
if (celx*cely!=1) pvm_spawn(aux,(char **) 0,0,"",celx*cely-1,&tids[1]);
     
    if (celx * cely != 1)
      pvm_spawn(argv[0], (char**)0, 0, "", celx*cely-1, &tids[1]);
    
    //Creacion del vector de tids traspuestos
    tids_t = (int*)malloc(celx*cely*sizeof(int));
    if (tids_t == NULL) {
      printf("ERROR: Falta de memoria\n");
      pvm_exit();
      exit(-1);
    }
   
    transponer(tids,tids_t,celx,cely);

    printf("Vector de tids:\n");
    for (i = 0; i < cely; i++) {
      for (j = 0; j < celx; j++) printf("%d ",tids[i*celx + j]);
      printf("\n");
    }
    
    //printf("Vector de tids traspuesto:\n");
    //for (i = 0; i < celx*cely; i++) printf("%d ",tids_t[i]);
    //printf("\n");
        
    mA = (int)ceil(M*1.0/cely);
    nA = (int)ceil(N*1.0/celx);
    mB = (int)ceil(P*1.0/celx);
    nB = (int)ceil(N*1.0/cely);

    //Calculamos la parte de la matriz del padre.
    for (i = 0; i < mA; i++)
      for (j = 0; j < mB; j++) 
	for (k = 0; k < N; k++)
	  matrizC[i][j] += matrizA[i][k] * matrizB[j][k];
    
    rfilas = 0; //Num. de nodos que han recibido alguna fila
    rcols = 0;  //Num. de nodos que han recibido alguna columna. 

    if ((celx * cely - 1) != 0) {
      //Enviamos a cada fila de procesadores las filas de la
      //matriz A que les corresponde. "contador" mantiene el 
      //numero de filas de la matriz que aun no se han enviado.
      for (i = 0; i < cely; i++) { 
	//Preparacion del buffer de envio
	if (pvm_initsend(PvmDataDefault) == -1) {
	  printf("Error al inicializar el buffer de envio\n");
	  pvm_exit();
	  exit(-1);
	}	
	if (i == 0) { //Enviamos mA filas de A a la fila del nodo 0 (excluido).
	  contador = M - mA;
	  //printf("1: contador: %d, mA: %d, i: %d\n", contador, mA, i);
	  pvm_pkint(&mA,1,1);
	  pvm_pkint(&N,1,1);
	  
	  //pvm_pkdouble(&matrizA[0][0], N*mA, 1);
	  for (j = 0; j < mA; j++) pvm_pkdouble(&matrizA[j][0], N, 1);
	  pvm_mcast(&tids[1], celx - 1, 0);
	  rfilas++;
	}
	else if (contador >= mA) { //Enviamos mA filas de A
	  //printf("2: contador: %d, mA: %d, i: %d\n", contador, mA, i);
	  contador-=mA;
	  pvm_pkint(&mA,1,1);
	  pvm_pkint(&N,1,1);

	  //pvm_pkdouble(&matrizA[i*mA][0], N*mA, 1);
	  for (j = i*mA; j < (i + 1)*mA; j++) pvm_pkdouble(&matrizA[j][0], N, 1);
	  pvm_mcast(&tids[i*celx], celx, 0);
	  rfilas++;
	}
	else if (contador > 0 && contador < mA) { //Enviamos el resto de filas
	  //printf("3: contador: %d, mA: %d, i: %d\n", contador, mA, i);
	  pvm_pkint(&contador,1,1);
	  pvm_pkint(&N,1,1);
 
	  //pvm_pkdouble(&matrizA[i*mA][0], N*contador, 1);	  
	  for (j = i*mA; j < i*mA+contador; j++) pvm_pkdouble(&matrizA[j][0], N, 1);
	  contador = 0;
	  pvm_mcast(&tids[i*celx], celx, 0);
	  rfilas++;
	}
	else {
	  //printf("4: contador: %d, mA: %d, i: %d\n", contador, mA, i);
	  pvm_pkint(&contador,1,1); //No se le envia ninguna fila
	  pvm_mcast(&tids[i*celx], celx, 0);
	}
      }
      
      //Enviamos a cada columna de procesadores las filas de la
      //matriz B traspuesta que les corresponde. "contador" mantiene el
      //numero de columnas de la matriz que aun no se han enviado.
      for (i = 0; i < celx; i++) {
	//Preparacion del buffer de envio
	if (pvm_initsend(PvmDataDefault) == -1) {
	  printf("Error al inicializar el buffer de envio\n");
	  pvm_exit();
	  exit(-1);
	}
	if (i == 0) { //Enviamos mB filas de B' a la columna  del nodo 0 (excluido).
	  contador = P - mB;
	  //printf("1: contador: %d, mB: %d, i: %d\n", contador, mB, i);
	  pvm_pkint(&mB,1,1);
	  
	  //pvm_pkdouble(&matrizB[0][0], N*mB, 1);
	  for (j = 0; j < mB; j++) pvm_pkdouble(&matrizB[j][0], N, 1);
	  pvm_mcast(&tids_t[1], cely - 1, 1);
	  rcols++;
	}
	else if (contador >= mB) { //Enviamos mB filas de B traspuesta
	  //printf("2: contador: %d, mB: %d, i: %d\n", contador, mB, i);
	  contador-=mB;
	  pvm_pkint(&mB,1,1);
	  
	  //pvm_pkdouble(&matrizB[i*mB][0], N*mB, 1);
	  for (j = i*mB; j < (i+1)*mB; j++) pvm_pkdouble(&matrizB[j][0], N, 1);
	  pvm_mcast(&tids_t[i*cely], cely, 1);
	  rcols++;
	}
	else if (contador > 0 && contador < mB) { //Enviamos el resto de filas
	  //printf("3: contador: %d, mB: %d, i: %d\n", contador, mB, i);
	  pvm_pkint(&contador,1,1);
	  
	  //pvm_pkdouble(&matrizB[i*mB][0], N*contador, 1);
	  for (j = i*mB; j < i*mB+contador; j++) pvm_pkdouble(&matrizB[j][0], N, 1);
	  contador = 0;
	  pvm_mcast(&tids_t[i*cely], cely, 1);
	  rcols++;
	}
	else {
	  //printf("4: contador: %d, mB: %d, i: %d\n", contador, mB, i);
	  pvm_pkint(&contador,1,1); //No se le envia ninguna fila
	  pvm_mcast(&tids_t[i*cely], cely, 1);
	}
      }
      
      //Recibimos la respuesta de los hijos
      for (i = 0; i < (rfilas * rcols) - 1; i++) {
	pvm_recv(-1,2);
	pvm_upkint(&nodo,1,1);
	pvm_upkint(&filas,1,1);
	pvm_upkint(&columnas,1,1);
	if (filas * columnas != 0) {
	  base = get_idy(nodo, tids, celx, cely) * mA;
	  desplaz = get_idx(nodo, tids_t, celx, cely) * mB;
	  //printf("Nodo: %d Base: %d Desplaz: %d\n",nodo,base,desplaz);
	  if (base == -1) {
	    printf("Error en la recepcion de datos\n");
	    pvm_exit();
	    exit(-1);
	  }
	  for (j = 0; j < filas; j++) {
	    pvm_upkdouble(&matrizC[base + j][desplaz], columnas, 1);
	  }
	}
      }
    }
  
    printf("MATRIZ RESULTADO:\n");
    for (i = 0; i < M; i++) {
      for (j = 0; j < P; j++) printf("%f ", matrizC[i][j]);
      printf("\n");	
    }

    error = escribematriz(argv[5], matrizC, M, P);
    if (error == -1) {
      printf("Error al escribir en el fichero %s\n", argv[5]);
      printf("El fichero no pudo ser escrito correctamente\n");
      pvm_exit();
      exit(-1);
    }
    
    printf("Final del codigo del nodo (0,0)\n");
    pvm_exit();
    exit(0);
  }  
  else { //Codigo de los hijos.
    if (pvm_initsend(PvmDataDefault) == -1) {
      printf("Error al inicializar el buffer de envio\n");
      pvm_exit();
      exit(-1);
    }
    
    //Recibimos filas de la matriz A
    pvm_recv(padre,0);
    pvm_upkint(&filas,1,1);
    if (filas == 0) { //Este procesador no trabaja.
      printf("El nodo %d ha abandonado por falta de filas\n", mitid);
      pvm_exit();
      exit(0);
    }
    else printf("El nodo %d recibe %d filas\n", mitid, filas);
    pvm_upkint(&N,1,1);
    printf("Nodo %d recibe N: %d\n", mitid, N);
    //Reservamos memoria para la matriz A
    matrizA = (double**)malloc(filas * sizeof(double*));
    if (matrizA == NULL) {
      printf("ERROR: Falta de memoria\n");
      pvm_exit();
      exit(-1);
    }
    for (i = 0; i < filas; i++) { 
      matrizA[i] = (double*)malloc(N * sizeof(double));
      if (matrizA[i] == NULL) {
	printf("ERROR: Falta de memoria\n");
	pvm_exit();
	exit(-1);
      }
    }
    
    //pvm_upkdouble(&matrizA[0][0], N*filas, 1);
    for (j = 0; j < filas; j++) pvm_upkdouble(&matrizA[j][0], N, 1);
    printf("Nodo %d MATRIZ A:\n",mitid);
    for (j = 0; j < filas; j++) {
      for (i = 0; i < N; i++) printf("%f ", matrizA[j][i]);
      printf("\n");
    }
    printf("\n");

    //Recibimos filas de la matriz B', que son las columnas de B
    pvm_recv(padre,1);
    pvm_upkint(&columnas,1,1);
    if (columnas == 0) { //Este procesador no trabaja.
      printf("El nodo %d ha abandonado por falta de columnas\n", mitid);
      pvm_exit();
      exit(0);
    }
    else printf("El nodo %d ha recibido %d columnas\n", mitid, columnas);
    //Reservamos memoria para la matriz B'
    matrizB = (double**)malloc(columnas * sizeof(double*));
    if (matrizB == NULL) {
      printf("ERROR: Falta de memoria\n");
      pvm_exit();
      exit(-1);
    }
    for (i = 0; i < columnas; i++) { 
      matrizB[i] = (double*)malloc(N * sizeof(double));
      if (matrizB[i] == NULL) {
	printf("ERROR: Falta de memoria\n");
	pvm_exit();
	exit(-1);
      }
    }
    
    //  pvm_upkdouble(&matrizB[0][0], columnas*N, 1);
    for (j = 0; j < columnas; j++) pvm_upkdouble(&matrizB[j][0], N, 1);
    printf("Nodo %d MATRIZ B:\n",mitid);
    for (j = 0; j < columnas; j++) {
      for (i = 0; i < N; i++) printf("%f ", matrizB[j][i]);
      printf("\n");
    }
    printf("\n");

    //Reservamos memoria para la submatriz C
    matrizC = (double**)malloc(filas * sizeof(double*));
    if (matrizC == NULL) {
      printf("ERROR: Falta de memoria\n");
      pvm_exit();
      exit(-1);
    }
    for (i = 0; i < filas; i++) { 
      matrizC[i] = (double*)malloc(columnas * sizeof(double));
      if (matrizC[i] == NULL) {
	printf("ERROR: Falta de memoria\n");
	pvm_exit();
	exit(-1);
      }
    }

    //Inicializamos la matriz C
    for (i = 0; i < filas; i++)
      for (j = 0; j < columnas; j++) 
	matrizC[i][j] = 0;

    //Calculamos C.
    for (i = 0; i < filas; i++)
      for (j = 0; j < columnas; j++) 
	for (k = 0; k < N; k++)
	  matrizC[i][j] += matrizA[i][k] * matrizB[j][k];

    printf("Nodo %d MATRIZ C:\n",mitid);
    for (j = 0; j < filas; j++) {
      for (i = 0; i < columnas; i++) printf("%f ", matrizC[j][i]);
      printf("\n");
    }
    printf("\n");

    pvm_pkint(&mitid, 1, 1);
    pvm_pkint(&filas, 1, 1);
    pvm_pkint(&columnas, 1, 1);
    
    //pvm_pkdouble(&matrizC[0][0], filas*columnas, 1);
    for(j = 0; j < filas; j++) pvm_pkdouble(&matrizC[j][0], columnas, 1);
    pvm_send(padre, 2);
    printf("Final del codigo nodo hijo %d\n", mitid);
    pvm_exit();
    exit(0);
  }
  return 0;
}

