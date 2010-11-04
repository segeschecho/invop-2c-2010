/************************************************/
/*  Ejemplo IO                                  */
/************************************************/

#include "C:\ILOG\CPLEX_Studio_AcademicResearch122\cplex\include\ilcplex\cplex.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <iostream.h>

/********* Declaracion de variables globales *********/
float machineEps;

/********* Declaracion de funciones ******************/
static int CPXPUBLIC
   cortes(CPXCENVptr env, void *cbdata, int wherefrom,
                  void *cbhandle, int *useraction_p);

int main (int argc, char *argv[]){
   int status = 0;
   double init,end;

   CPXENVptr env = NULL;
   CPXLPptr  lp = NULL;
   machineEps = 0.000001;

   env = CPXopenCPLEX (&status);
   if ( env == NULL ) {
      cout << "Problemas al abrir en entorno\n";
      goto TERMINATE;
   }

   /*Parametros para que no preprocese*/
   CPXsetintparam (env, CPX_PARAM_REDUCE,CPX_OFF);
   CPXsetintparam (env, CPX_PARAM_PRELINEAR, 0);
   CPXsetintparam (env, CPX_PARAM_MIPCBREDLP, CPX_OFF); 

   /*Parametros para salida por pantalla*/
   CPXsetintparam (env, CPX_PARAM_SCRIND, CPX_ON);
   CPXsetintparam (env, CPX_PARAM_MIPINTERVAL, 1);
   CPXsetintparam (env, CPX_PARAM_MIPDISPLAY, 3);

   status = CPXsetcutcallbackfunc (env, cortes,NULL);
   if ( status ) {
      cout << "Problemas al setear rutina de separacion - %d.\n", status;
      goto TERMINATE;
   }

   lp = CPXcreateprob (env, &status, "pp.lp");
   if ( lp == NULL ) {
      cout << "Problemas al crear el LP.\n";
      goto TERMINATE;
   }

   status = CPXreadcopyprob (env, lp, argv[1], NULL);
   if ( status ) {
      cout << "Problemas al leer el problema - " << status << endl;
      goto TERMINATE;
   }

   CPXgettime(env,&init);
   status = CPXmipopt (env, lp);
   CPXgettime(env,&end); 

   if ( status ) {
      cout << "Ver status - " << status << endl;
      goto TERMINATE;
   }   

   status = CPXgetstat (env, lp);
   if(status==CPX_STAT_UNBOUNDED) {
      cout << "Modelo no acotado\n";
   } else if(status==CPXMIP_INFEASIBLE){
	cout << "Problema Infactible\n";
   } else if(status==CPXMIP_TIME_LIM_FEAS){
      cout << "Se fue de tiempo\n";
   } else {
	double objval;
	int numcols = CPXgetnumcols (env, lp);
      int nodos = CPXgetnodecnt(env, lp);
	double x[numcols];
	status = CPXgetmipobjval (env, lp, &objval) || CPXgetmipx (env, lp,x,0,numcols-1);
      if ( status ) {
		cout << "Problemas al pedir la solucion - " << status << endl;
            goto TERMINATE;
      }

	cout << endl << "Valor objetivo: " << objval << endl;
	cout << "Nodos: " << nodos << endl;
      cout << "Tiempo: " << end-init << endl;
	cout << "\nSolucion:\n";
	for(int i=0;i<numcols;i++) cout << "Col " << i << ": " << x[i] << endl;
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


