/************************************************/
/*  Ejemplo IO                                  */
/************************************************/

#include "C:\Program Files\CPLEX_Studio_AcademicResearch122\cplex\include\ilcplex\cplex.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <iostream>

using namespace std;

#define BUFFERSIZE 256

/********* Declaracion de variables globales *********/
double machineEps;

/********* Declaracion de funciones ******************/
static int CPXPUBLIC cortes(CPXCENVptr env, void *cbdata, int wherefrom, void *cbhandle, int *useraction_p);

void colToLp(const char* colFileName, const char* lpFileName)
{
    FILE* finput;
    FILE* foutput;

    fopen_s(&finput, colFileName, "r");
    if( finput == NULL )
    {
        printf("No se pudo leer el archivo %s", colFileName);
        return;
    }

    fopen_s(&foutput, lpFileName, "w");
    if( foutput == NULL )
    {
        printf("No se pudo escribir el archivo %s", lpFileName);
        return;
    }

    char c;
    char* format = new char[BUFFERSIZE];
    int nodes = 0;
    int edges = 0;
    while( (c = getc(finput)) != EOF )
    {
        switch(c)
        {
        case 'c':
            fprintf_s(foutput, "\\");
            while( (c = getc(finput)) != '\n' )
                fprintf_s(foutput, "%c", c);

            fprintf_s(foutput, "\n");
            break;

        case 'p':
            getc(finput); // skip space
            int i;
            for( i = 0; i < BUFFERSIZE && (c = getc(finput)) != ' '; i++ ) 
                format[i] = c;
            format[i] = '\0';
            fscanf_s(finput, "%d %d\n", &nodes, &edges);

            // begin constructing output file
            fprintf_s(foutput, "Minimize\n obj:");
            if(nodes > 0)
                fprintf_s(foutput, " w0");
            for( i = 1; i < nodes; i++ )
                fprintf_s(foutput, " + w%d",i);


            fprintf_s(foutput, "\nSubject To");
            for( i = 0; i < nodes; i++ )
            {
                fprintf_s(foutput, "\n soloUnColorNodo%d: x%d0", i, i);

                for( int j = 1; j < nodes; j++ )
                    fprintf_s(foutput, " + x%d%d", i, j);

                fprintf_s(foutput, " = 1");
            }

            break;

        case 'e':
            getc(finput); // skip space
            int node1, node2;
            fscanf_s(finput, "%d %d\n", &node1, &node2);
            node1--;
            node2--;
            for(int j = 0; j < nodes; j++)
                fprintf_s(foutput, "\n adyDeDistintoColorNodo%dNodo%dColor%d: x%d%d + x%d%d - w%d <= 0",
                          node1, node2, j, node1, j, node2, j, j);
            break;

        default:
            break;
        }
    }

    fprintf_s(foutput, "\nBinary\n");
    for(int j = 0; j < nodes; j++)
        fprintf_s(foutput, " w%d\n", j);

    for(int i = 0; i < nodes; i++)
        for(int j = 0; j < nodes; j++)
            fprintf_s(foutput, " x%d%d\n", i, j);

    fprintf_s(foutput, "\nEnd");

    fclose(foutput);
    fclose(finput);
}

int main (int argc, char *argv[]){
    int status = 0;
    double init,end;
    string fileInput;
    size_t extensionOffset;

    CPXENVptr env = NULL;
    CPXLPptr  lp = NULL;
    machineEps = 0.000001;

    env = CPXopenCPLEX (&status);
    if ( env == NULL )
    {
        printf("Problemas al abrir en entorno\n");
        goto TERMINATE;
    }

    /*Parametros para que no preprocese*/
/*    CPXsetintparam (env, CPX_PARAM_REDUCE,CPX_OFF);
    CPXsetintparam (env, CPX_PARAM_PRELINEAR, 0);
    CPXsetintparam (env, CPX_PARAM_MIPCBREDLP, CPX_OFF); 
*/
    /*Parametros para salida por pantalla*/
    CPXsetintparam (env, CPX_PARAM_SCRIND, CPX_ON);
    CPXsetintparam (env, CPX_PARAM_MIPINTERVAL, 1);
    CPXsetintparam (env, CPX_PARAM_MIPDISPLAY, 3);
/*
    status = CPXsetcutcallbackfunc (env, cortes, NULL);
    if ( status )
    {
        printf("Problemas al setear rutina de separacion - %d.\n", status);
        goto TERMINATE;
    }
*/
    lp = CPXcreateprob (env, &status, "pp.lp");
    if ( lp == NULL )
    {
        printf("Problemas al crear el LP.\n");
        goto TERMINATE;
    }

    if( argv[1] != 0 )
        fileInput = argv[1];
    else
        fileInput = "myciel4.col";

    extensionOffset = fileInput.find_last_of(".");
    if( fileInput.find_last_of(".col") )
    {
        string fileOutput = fileInput.substr(0, extensionOffset);
        fileOutput += ".lp";
        colToLp(fileInput.c_str(), fileOutput.c_str());
        fileInput = fileOutput;
    }

    status = CPXreadcopyprob (env, lp, fileInput.c_str(), NULL);
    if ( status )
    {
        printf("Problemas al leer el problema - %d\n", status);
        goto TERMINATE;
    }

    CPXgettime(env,&init);
    status = CPXmipopt (env, lp);
    CPXgettime(env,&end);

    if ( status ) {
        printf("Ver status - %d\n", status);
        goto TERMINATE;
    }

    status = CPXgetstat (env, lp);
    if(status==CPX_STAT_UNBOUNDED) {
        printf("Modelo no acotado\n");
    } else if(status==CPXMIP_INFEASIBLE){
	    printf("Problema Infactible\n");
    } else if(status==CPXMIP_TIME_LIM_FEAS){
        printf("Se fue de tiempo\n");
    } else {
	    double objval;
	    int numcols = CPXgetnumcols (env, lp);
        int nodos = CPXgetnodecnt(env, lp);
	    double* x;
        x = new double[numcols];
	    status = CPXgetmipobjval (env, lp, &objval) || CPXgetmipx (env, lp,x,0,numcols-1);
        if ( status ) {
		    printf("Problemas al pedir la solucion - %d\n", status);
                goto TERMINATE;
          }

	    printf("\nValor objetivo: %lf\n", objval);
	    printf("Nodos: %d\n", nodos);
        printf("Tiempo: %lf\n", end-init);
	    printf("\nSolucion:\n");
	    for(int i = 0; i < numcols; i++)
            printf("Col %d: %lf\n", i, x[i]);
    }

 TERMINATE:
    if ( lp != NULL ) {
        status = CPXfreeprob (env, &lp);
        if ( status ) {
            fprintf (stderr, "CPXfreeprob %d.\n",status);
        }
    }
    if ( env != NULL ) {
        status = CPXcloseCPLEX (&env);
        if ( status ) {
            printf ("CPXclose %d\n",status);
        }
    }

    system("PAUSE");
    return 0;
}

static int CPXPUBLIC cortes (CPXCENVptr env,
               void       *cbdata,
               int        wherefrom,
               void       *cbhandle,
               int        *useraction_p){
  
   *useraction_p = CPX_CALLBACK_DEFAULT;

   //if(cantCortes > 0) *useraction_p = CPX_CALLBACK_SET;  
  return 0;
}


