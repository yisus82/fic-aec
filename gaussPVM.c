#include <stdio.h>
#include <malloc.h>
#include "pvm3.h"
//directorio de donde lee la matriz A y el vector b, y guardar x
#define DIRECTORIO "/home/i/infjps00/pvm3/bin/LINUX/"
#define GRUPO "MAJADEROS" /*grupo global al que perteneces*/
#define PROGRAMA "gaussPVM"
#define TAMSTR 256 /*Tamanho de los strings */

/*Casos posibles a lo largo de las iteraciones*/
#define CASO_1 1
#define CASO_2 2
#define CASO_3 3
#define CASO_4 4

/*
 Un punto es simplemente un par de enteros, para simplificar el paso
 de parametros.
 */
typedef struct p
{
    int x;
    int y;
}punto;

/*-------------------------------------------------------------------------*/
//Variables globales
double **a=NULL;int p,q;//matriz de coeficientes pxq
int n;//dimension de la matriz originaria nxn
double *b=NULL;//vector de p constantes
//nombres de los ficheros
char fichero_a[TAMSTR], fichero_b[TAMSTR], fichero_x[TAMSTR];
char grupoX[TAMSTR],grupoY[TAMSTR];
int msgtagX, msgtagY;/* los msgtag cuando envias a tus grupos*/
//informacion de pvm
punto dim;//tamnho de la malla de procesadores
punto pos;//mi posicion en la malla de procesadores
int mitid, *tids = NULL;//informacion con los tids
int yo;//numero dentro de GRUPO, en teoria tids[yo]=mitid si mantiene el orden
int nprocs;//para calcularlo una sola vez

/*-------------------------------------------------------------------------*/
//declaracion de las funciones
double calculo(double **, int, int, int, double, int);
int gauss();

int calculaP(int, int, int);
int calculaQ(int, int, int);
void reservaMatriz(double ***, int, int);
void reservaVector(double **, int);
void liberaMatriz(double **, int, int);
void liberaVector(double *, int);
int leeMatriz(char *, double ***, int *, int *, punto, punto);
void leeVector(char *, double **, int *, punto, punto);

void imprimeMatriz(double **, int,int );
void imprimeVector(double *, int );

// 5/5/03
void calculaPos(punto, int , punto*);
void enviaDatos(char *, char *, char *, punto, int *, int);
void recibeDatos(char *, char *, char *, punto *);

// 12/5/03
void uneGrupos(punto, char *, char *);
//14/5/03
int determinaCaso(int,punto,punto);
void calculoCaso1(int);
void calculoCaso2(int);
void calculoCaso3(int);
void calculoCaso4(int);

//19/5/03
void inicializaGauss(int nargs, char *argv[]);
void escribeSolucion();


/*------------------------------------------------------------------------*/

main(int nargs,char *argv[])
{//main
    int paux;

    //inicializacion del programa
    inicializaGauss(nargs, argv);

    calculaPos(dim,yo,&pos);//calculamos nuestra pos actual con yo

    n=leeMatriz(fichero_a,&a,&p,&q,dim,pos);
    leeVector(fichero_b,&b,&paux,dim,pos);

    if(p!=paux)
    {
	printf("Error: dimensiones de la matriz A y del vector B distintas.\n");
	printf("%i != %i \n",p,paux);fflush(stdout);
	liberaVector(b,paux);liberaMatriz(a,p,q);
        exit(0);
    }

    uneGrupos(pos,grupoX,grupoY);fflush(stdout);

    pvm_barrier(GRUPO,nprocs);/*Para que todos esten en los grupos */

    if(gauss())
    {
	printf("yo=%i ha podido resolver su subsistema(ver %s)",yo,fichero_x);
        fflush(stdout);
	/*solo la primera columna hace la escritura paralela*/
	if(pos.x==0)
	{
	    escribeSolucion();
	}
    }
    else
    {
        printf("No se ha podido resolver el sistema.\n");
    }

    liberaVector(b,p);
    liberaMatriz(a,p,q);

    pvm_exit();
}//main

/***********************************************************************/
/******************** DEFINICION DE LAS FUNCIONES **********************/
/***********************************************************************/

/*
 Crea los subprocesos, se incorporan cada uno al grupo global y
 reciben los nombres de archivos y otras informaciones adicionales.
 */
void inicializaGauss(int nargs, char *argv[])
{
    //nos incorporamos a pvm y al grupo
    mitid = pvm_mytid();
    yo= pvm_joingroup(GRUPO);
    if(yo<0)
    {
	printf("Error en el join group, yo=%i, mitid=%i.\n",yo,mitid);
        exit(0);
    }
    if(yo==0) //si es el primer proceso
    {

	//coger argumentos
	if(nargs!=6)
	{
	    printf("Error: numero de parametros incorrecto (%d). El formato es:\n", nargs);
	    printf("\t gauss fichero_a fichero_b fichero_salida dimx dimy\n");
	    exit(0);
	}
	else
	{
	    strcpy(fichero_a,DIRECTORIO);strcat(fichero_a,argv[1]);
	    strcpy(fichero_b,DIRECTORIO);strcat(fichero_b,argv[2]);
	    strcpy(fichero_x,DIRECTORIO);strcat(fichero_x,argv[3]);
	    dim.x= atoi(argv[4]);
	    dim.y= atoi(argv[5]);
	    if((dim.x<=0)&&(dim.y<=0))
	    {
		printf("Error: dimensiones de la malla incorrectas.\n");
                exit(0);
	    }

	}
	//crear los demas procesos
        nprocs=dim.x*dim.y;
	tids=(int *)calloc(nprocs, sizeof(int));

	pvm_spawn(PROGRAMA, (char **)0, 0, "", nprocs-1,&tids[1]);
	//completamos tids(por si acaso)
	tids[0]=pvm_gettid(GRUPO,0);

	//enviarles fichero_a,fichero_b,fichero_salida, y dim
        enviaDatos(fichero_a,fichero_b, fichero_x, dim, &tids[1],nprocs-1);
    }
    else//se reciben los datos iniciales
    {
	recibeDatos(fichero_a,fichero_b,fichero_x,&dim);
        nprocs=dim.x*dim.y;
    }

}/* inicializaGauss */
/***********************************************************************/
/**************** TRASLACION DE LAS FUNCIONES FORTRAN ******************/
/***********************************************************************/
double calculo(double **a, int i, int j, int ipv, double factor, int n)
{//calculo
    return  a[i][j]-factor*a[ipv][j];
}//calculo



/*
 Resuelve el sistema de ecuaciones lineales.
 O si no lo resuelve
 1 si lo resuelve
 Casos posibles en cada iteracion:
 1) yo tengo el pivote
 2) no lo tengo pero lo tiene uno de mi fila de procesadores
 3) no lo tengo pero lo tiene uno de mi columna de procesadores
 4) ni lo tengo yo, ni el de mi fila, ni el de mi columna

 la situacion se calcula mediante iteracion,dim y pos
 */
int gauss()
{//gauss
    int it=0;//iteracion actual
    int caso;//caso en el que nos encontramos

    for(it=0;it<n;it++)
    {
	caso=determinaCaso(it,dim,pos);
    	switch(caso)
	{
	case CASO_1: calculoCaso1(it);
	break;
	case CASO_2: calculoCaso2(it);
	break;
	case CASO_3: calculoCaso3(it);
	break;
	case CASO_4: calculoCaso4(it);
	break;
	}

    }
    return 1;
}//gauss


/*CASO_1
 1) Determinar el pivote.
 2) Dividir los elementos de esa fila original por el pivote.
 3) Restar esa subfila a todas las demas subfilas que se poseen,
    calculando el factor para cada subfila.
 4) Enviar el pivote a todos los de tu fila de procesadores + los
 factores correspondientes del resto de subfilas que poseen(las
 que no tienen el pivote).
 5.) Enviar la subfila que se esta tratando a todos los de tu
 columna de procesadores.+bf
*/
void calculoCaso1(int it)
{
    int i,j;
    int f,c;//en que fila,columna de tu submatriz esta el pivote
    double *factor;//array en que ir almacenando los factores
    double pivote;

    f = it/dim.y;
    c = it/dim.x;

    //1)
    pivote=a[f][c];
    if(pivote==0)//pivote 0
    {
	printf("Pivote mal.\n");
        exit(0);//<--- no es una buena solucion
    }
    //2)
    for(i=0;i<q;i++)
	a[f][i]/=pivote;
    b[f]/=pivote;


    //3)
    factor=(double *)calloc(p,sizeof(double));
    //La fila del pivote la ignoramos
    for(i=0;i<p;i++)//subfila a subfila de la submatriz
    {
	if(i!=f)
	{
	    factor[i] = a[i][c];//ahora en a[f][c] hay un 1
	    for(j=0;j<q;j++)//hacemos la resta
	    {
                a[i][j] -= factor[i]*a[f][j];
	    }
            b[i] -= factor[i]*b[f];
	}
    }
    //4)
    pvm_initsend(PvmDataDefault);
    pvm_pkdouble(&pivote,1,1);//direccion,elementos,stride
    pvm_pkdouble(factor,p,1);
    pvm_bcast(grupoY,msgtagY);

    pvm_initsend(PvmDataDefault);
    pvm_pkdouble(&a[f][0],q,1);
    pvm_pkdouble(&b[f],1,1);
    pvm_bcast(grupoX,msgtagX);
}//calculoCaso1

/*
CASO_2
 1.) Recibir el pivote+los factores para las subfilas que no son las
 mismas que las del pivote, que viene del grupo de tu misma fila.
 2.) Divide los elementos de la fila correspondiente de su submatriz
 por el pivote.
 3.) Restar esa subfila a todas las demas subfilas que se poseen usando
 los factores.
 4.) Enviar la subfila del pivote a todos los de tu columna
 de procesadores.+bf

*/
void calculoCaso2(int it)
{
    double *factor;//array donde metemos los factores
    double pivote;
    int f;//fila de nuestra submatriz en la que esta el pivote
    int i,j;

    f=it/dim.y;
    factor=(double *)calloc(p,sizeof(double));


    //1)recibimos de cualquier tid, nos basta que tenga el mstag del grupo
    pvm_recv(-1,msgtagY);
    pvm_upkdouble(&pivote,1,1);//direccion, n elementos, stride
    pvm_upkdouble(factor,p,1);

    //2)
    for(i=0;i<q;i++)
	a[f][i]/=pivote;
    b[f]/=pivote;

    //3)
    for(i=0;i<p;i++)//subfila a subfila de su submatriz
	if(i!=f)//no es la subfila del pivote
	{
	    for(j=0;j<q;j++)
		a[i][j]=a[i][j]-factor[i]*a[f][j];
            b[i]-=factor[i]*b[f];
	}
    //4)
    pvm_initsend(PvmDataDefault);
    pvm_pkdouble(&a[f][0],q,1); //direccion,num, stride
    pvm_pkdouble(&b[f],1,1);
    pvm_bcast(grupoX,msgtagX);
}//calculaCaso2


/*
CASO_3
 1.) Recibir la subfila de resta, de nuestra columna de procesadores+bf
 2.) Restarsela a todas las subfilas que se poseen calculando los factores.
 3.) Enviar esos factores a los de tu misma fila de procesadores.
*/
void calculoCaso3(int it)
{
    double *subfila;
    double *factor;
    int c;//columna de nuestra submatriz que corresponde con el pivote
    int i,j;
    double bf;//b de la fila del pivote

    c=it/dim.x;

    subfila = (double *)calloc(q,sizeof(double));
    factor = (double *)calloc(p,sizeof(double));

    //1)
    pvm_recv(-1,msgtagX);
    pvm_upkdouble(subfila,q,1);
    pvm_upkdouble(&bf,1,1);

    //2)
    for(i=0;i<p;i++)//subfila a subfila de nuestra submatriz
    {
	factor[i]=a[i][c];//factor es simplemente el elemento de la misma columna, a[f][c]=1

	for(j=0;j<q;j++)
	    a[i][j]-= factor[i]*subfila[j];

        b[i]-= factor[i]*bf;
    }

    //3)
    pvm_initsend(PvmDataDefault);
    pvm_pkdouble(factor,p,1);
    pvm_bcast(grupoY,msgtagY);
}//calculaCaso3


/*
 CASO_4
 1.) Recibir la subfila de resta(viene de tu columna).+bf
 2.) Recibir los factores  para la resta(viene de tu fila).
 3.) Restarse esa subfila a todas las subfilas que se poseen usando el factor.
*/
void calculoCaso4(int it)
{
    double *subfila,*factor;
    double bf;//b de la fila del pivote
    int i,j;

    subfila=(double *)calloc(q,sizeof(double));
    factor=(double *)calloc(p,sizeof(double));
    //1)
    pvm_recv(-1,msgtagX);
    pvm_upkdouble(subfila,q,1);
    pvm_upkdouble(&bf,1,1);
    //2)
    pvm_recv(-1,msgtagY);
    pvm_upkdouble(factor,p,1);

    //3)
    for(i=0;i<p;i++)
    {
	for(j=0;j<q;j++)
	    a[i][j]-=factor[i]*subfila[j];

        b[i]-=factor[i]*bf;
    }
}//calculaCaso4

/*
 Determina en que caso se esta:
 Casos posibles en cada iteracion:
 1) yo tengo el pivote
 2) no lo tengo pero lo tiene uno de mi fila de procesadores
 3) no lo tengo pero lo tiene uno de mi columna de procesadores
 4) ni lo tengo yo, ni el de mi fila, ni el de mi columna

 La it indica que elemento de la matriz estamos buscando:
 el (it,it)
 */
int determinaCaso(int it, punto dim,punto pos)
{//determinaCaso
    int condicion_fila= (it%dim.y==pos.y);/*lo tengo en mi fila*/
    int condicion_col=  (it%dim.x==pos.x);/*lo tengo en mi columna*/

    if(condicion_fila&&condicion_col) return CASO_1;/*en los dos*/
    if(condicion_fila) return CASO_2;
    if(condicion_col) return CASO_3;

    /* si se llega aqui es que no esta ni en mi fila ni en mi columna*/
    return CASO_4;
}//determinaCaso

/*************************************************************************/
/************* FUNCIONES DE RESERVA/LIBERACION DE MEMORIA ****************/
/*************************************************************************/
/*
 Reserva memoria para una matriz mat de dimensiones d1xd2
 */
void reservaMatriz(double ***mat, int d1, int d2)
{
    int i;
    *mat= (double **)calloc(d1, sizeof( double *));
    if(*mat==NULL)
	printf("Error: calloc no pudo reservar memoria.\n");

    for(i=0;i<d1;i++)
    {
	(*mat)[i]=(double *)calloc(d2, sizeof(double));
	if((*mat)[i]==NULL)
	    printf("Error: calloc no pudo reservar memoria.\n");

    }
}

/*
 Reserva memoria para un vector v de dimension d.
 */
void reservaVector(double **v, int d)
{
    *v= (double *)calloc(d, sizeof(double));
    if(*v==NULL)
	printf("Error: calloc no pudo reservar memoria.\n");

}

/*
 Libera memoria de una matriz mat de dimensiones d1xd2.
 */
void liberaMatriz(double **mat, int d1, int d2)
{
    int i;
    for(i=0;i<d1;i++)
    {
        free(mat[i]);
    }
    free(mat);
}

/*
 Libera memoria de un vector v de dimension d.
 */
void liberaVector(double *v, int d)
{
    free(v);
}


/*************************************************************************/
/************************* FUNCIONES DE E/S ******************************/
/*************************************************************************/
/*
 Lee del fichero nombre una matriz mat de dimensiones nxn ciclicamente,
 de manera que la matriz que devuelve es de dimension pxq. p y q depende
 de n , de la dimension de la malla de procesadores y la pos actual
 El "salto" depende de la dimension de la malla y la pos de este proceso
 mat y n lo lee del fichero, p y q lo calcula -> los devuelve
 nombre, dim y pos  son proporcionados por el programa -> no son cambiados

 Devuelve el tamnaho de la matriz originaria.
 */
int leeMatriz(char *nombre, double ***mat, int *p,int *q, punto dim,punto pos)
{//leeMatriz
    int f,c;//de acceso a la matriz de disco
    int auxf=0, auxc=0;//para acceder a nuestra matriz, para evitar calculos
    FILE *archivo=NULL;
    double dAux;
    int n;

    if ((archivo=fopen(nombre,"r"))==NULL)
    {
	printf("ERROR: imposible abrir fichero %s .\n",nombre);
        fflush(stdout);
        exit(0);
    }

    fscanf(archivo,"%i",&n);
    *p=calculaP(n, dim.y,pos.y);
    *q=calculaQ(n, dim.x,pos.x);

    reservaMatriz(mat, *p, *q);
    //leemos la informacion(siguen siendo de una matriz nxn)
    for(f=0;f<n;f++)
    {
	for(c=0;c<n;c++)
	{
	    fscanf(archivo,"%lf",&dAux);
	    if((f%dim.y==pos.y)&&(c%dim.x==pos.x))
		(*mat)[auxf][auxc++]=dAux;
	}
        if(f%dim.y==pos.y)//se ha leido de esta fila
	{auxc=0;auxf++;}
    }

    return n;
}//leeMatriz


/*
 lee del archivo nombre un vector v de dimensiones n(que lee del disco)
 como trabajamos co una distribucion ciclica no leera todo el vector,
 a partir de n y de dim calculara su tamanho real p
 a partir de dim y de pos sabra que tiene que saltarse

 (tambien se encarga de la reserva de memoria de v)
 Aprovechamos para obtener de aqui n.
 */

void leeVector(char *nombre, double **v, int *p, punto dim, punto pos)
{//leeVector
    int i,n;
    FILE *archivo=NULL;
    double dAux;
    int aux;//para un manejo facil de los "saltos"


    if((archivo=fopen(nombre,"r"))==NULL)
    {
	printf("ERROR: imposible abrir fichero %s.\n",nombre);
        exit(0);
    }

    fscanf(archivo,"%i",&n);
    *p=calculaP(n, dim.y, pos.y);
    reservaVector(v,*p);
    aux=0;
    for(i=0;i<n;i++)
    {
	fscanf(archivo, "%lf", &dAux);
	if(i%dim.y==pos.y)
	    (*v)[aux++]=dAux;
    }

}//leeVector


/*ESTO PARA EL FINAL, PRIMERO QUE FUNCIONE*/
/*
 Escribe la solucion de gauss, que obtiene de las varibles globales.
 paralela con el resto de los procesos de nuestra columna de procesadores.
 */
void escribeSolucion()
{//escribeVector
    int i;
    FILE *archivo;
    int escritos=0;/* cuantos elementos de la solucion llevamos escritos*/
    int anterior_tid,siguiente_tid;
    /*tids de los elementos anterior
    y siguiente en tu columna de procesadores*/

    if(yo!=0)
    {
	if((archivo=fopen(fichero_x,"a+"))==NULL)
	{
	    printf("ERROR: imposible abrir fichero %s.\n",fichero_x);
	    exit(0);
	}
        fclose(archivo);
    }
    else/* el primero elimina lo que pudiera haber antes.*/
    {
	if((archivo=fopen(fichero_x,"w+"))==NULL)
	{
	    printf("ERROR: imposible abrir fichero %s.\n",fichero_x);
	    exit(0);
	}

	fprintf(archivo,"%i\n",n);/*tamanho del vector*/
        fclose(archivo);
    }


    if(yo==0) anterior_tid= pvm_gettid(GRUPO,dim.x*(dim.y-1));
    else  anterior_tid= pvm_gettid(GRUPO, yo-dim.x);

    if(yo==dim.x*(dim.y-1)) siguiente_tid= pvm_gettid(GRUPO,0);
    else siguiente_tid = pvm_gettid(GRUPO, yo+dim.x);

/*    printf("yo=%i, mitid=%i, siguiente=%i, anterior=%i\n",
	   yo,mitid,siguiente_tid,anterior_tid);
    fflush(stdout);*/

    if(yo==dim.x*(dim.y-1))/* lanza el mensaje de inicio del while siguiente*/
    {
	pvm_initsend(PvmDataDefault);
        pvm_send(siguiente_tid,1);
    }

    while(escritos<p)
    {
	pvm_recv(anterior_tid,1);//mensaje de que puede escribir

	printf("yo=%i,escritos=%i,p=%i,b[escritos]=%lf\n",
	       yo,escritos,p,b[escritos]);
	fflush(stdout);

        archivo=fopen(fichero_x,"a+");
	fprintf(archivo,"%lf \n",b[escritos++]);
        fclose(archivo);
	/*informamos al siguiente de que puede escribir*/
	pvm_initsend(PvmDataDefault);
        pvm_send(siguiente_tid, 1);
    }

//    fclose(archivo);
}//escribeSolucion

/*************************************************************************/
/******************* FUNCIONES PARA MANEJO DE PVM ************************/
/*************************************************************************/
/*
 Calcula el numero de filas que se deben leer de la original
 */
int calculaP(int filas, int dimy,int posy)
{
 return filas/dimy + (filas%dimy>posy);
}

/*
 Calcula el numero de columnas que se deben leer de la original
 */
int calculaQ(int columnas, int dimx,int posx)
{
 return calculaP(columnas,dimx,posx);
}

/*
 Calcula la pos dentro de la malla de procesadores, usando yo y dim, que es
 el numero dentro del grupo de procesos trabajando
 */
void calculaPos(punto dim, int yo, punto *pos)
{
    pos->y= yo/dim.x;
    pos->x= yo%dim.x;
}


/*
 Envia los datos iniciales a los elementos de tids
 Los datos que hay que enviar:
 fichero_a, fichero_b, fichero_c, dim
 */
void enviaDatos(char *fichero_a, char *fichero_b, char *fichero_x, punto dim,
		int *tids, int n)
{

    pvm_initsend(PvmDataDefault);
    pvm_pkstr(fichero_a);
    pvm_pkstr(fichero_b);
    pvm_pkstr(fichero_x);
    pvm_pkbyte((char *)&dim,sizeof(punto),1);

    pvm_mcast(tids, n, 1);

}

/*
 Recibe los datos iniciales a los elementos de tids
 Los datos que hay que recibir:
 fichero_a, fichero_b, fichero_c, dim
 */
void recibeDatos(char *fichero_a, char *fichero_b, char *fichero_x, punto *dim)
{
    pvm_recv(pvm_gettid(GRUPO,0),1);//recibimos del primer elemento del grupo
    pvm_upkstr(fichero_a);
    pvm_upkstr(fichero_b);
    pvm_upkstr(fichero_x);
    pvm_upkbyte((char *)dim,sizeof(punto),1);
}

/*
 Une el proceso, dado por pos, a los dos grupos correspondientes de
 filas y columnas de la malla de procesadores(ademas se encarga de formar
 los nombres)
 Ademas calcula msgtag.
 */
void uneGrupos(punto pos, char *grupoX, char *grupoY)
{
    int unidades,decenas,centenas;

    strcpy(grupoX,"X000");
    strcpy(grupoY,"Y000");

    //modificamos grupoX segun pos
    unidades=pos.x%10;
    decenas=(pos.x/10)%10;
    centenas=(pos.x/100)%10;
    grupoX[3]+=unidades;
    grupoX[2]+=decenas;
    grupoX[1]+=centenas;

    msgtagX= pos.x+1;
    //modificamos grupoY segun pos
    unidades=pos.y%10;
    decenas =(pos.y/10)%10;
    centenas=(pos.y/100)%10;
    grupoY[3]+=unidades;
    grupoY[2]+=decenas;
    grupoY[1]+=centenas;
    msgtagY=100+pos.y+1;

    if(pvm_joingroup(grupoX)<0)
    {
	printf("Error de yo=%i al unirse al grupo %s.\n", yo, grupoX);
        fflush(stdout);
    }

    if(pvm_joingroup(grupoY)<0)
    {
	printf("Error de yo=%i al unirse al grupo %s.\n", yo, grupoY);
        fflush(stdout);
    }

//    printf("despues de unirse al grupo, yo=%i\n",yo);fflush(stdout);

}
/*************************************************************************/
/******************* FUNCIONES PARA LA DEPURACION ************************/
/*************************************************************************/
void imprimeMatriz(double **m, int d1, int d2)
{
    int f,c;

    for(f=0;f<d1;f++)
    {
	for(c=0;c<d2;c++)
	    printf(" %8.3lf ",m[f][c]);
        printf("\n");
    }
}


void imprimeVector(double *v, int d)
{
    int i;

    for(i=0;i<d;i++)
	printf(" %8.3lf ", v[i]);
    printf("\n");
}











/*que tiene que hacer cada procesador en cada caso:
 CASO_1
 1) Determinar el pivote.
 2) Dividir los elementos de esa fila original por el pivote.
 3) Restar esa subfila a todas las demas subfilas que se poseen,
    calculando el factor para cada subfila.
 4) Enviar el pivote a todos los de tu fila de procesadores + los
 factores correspondientes del resto de subfilas que poseen(las
 que no tienen el pivote).
 5.) Enviar la subfila que se esta tratando  a todos los de tu
 columna de procesadores.


 CASO_2
 1.) Recibir el pivote+los factores para las subfilas que no son las
 mismas que las del pivote.
 2.) Divide los elementos de la fila correspondiente de su submatriz
 por el pivote.
 3.) Enviar la subfila del pivote a todos los de tu columna
 de procesadores.
 4.) Restar esa subfila a todas las demas subfilas que se poseen usando
 los factores.

 CASO_3
 1.) Recibir la subfila de resta.
 2.) Restarsela a todas las subfilas que se poseen calculando los factores.
 3.) Enviar esos factores a los de tu misma fila de procesadores.

 CASO_4
 1.) Recibir la subfila de resta(viene de tu columna).
 2.) Recibir los factores  para la resta(viene de tu fila).
 3.) Restarse esa subfila a todas las subfilas que se poseen usando el factor.

*/
