/************************************************/
/*  Ejemplo IO                                  */
/************************************************/

#include "include.h"
using namespace std;

#define BUFFERSIZE 256
#define min(a, b) (a) < (b) ? (a) : (b)
#define max(a, b) (a) > (b) ? (a) : (b)

/********* Declaracion de tipos *********/

typedef int Node;
typedef pair<int, int> Edge;

/********* Declaracion de variables globales *********/
double machineEps;
vector< vector< bool > > g_vvbGraphTable;
vector< vector< Edge > > g_vvEGraphAdjList;
int g_nHeuristicColors;

/********* Implementacion de Funciones *********/

static int CPXPUBLIC cortes (CPXCENVptr env,
                             void       *cbdata,
                             int        wherefrom,
                             void       *cbhandle,
                             int        *useraction_p)
{
  
    *useraction_p = CPX_CALLBACK_DEFAULT;

    //if(cantCortes > 0) *useraction_p = CPX_CALLBACK_SET;  
    return 0;
}

void buildGraphFromCol(const char* colFileName)
{
    FILE* finput;

    finput = fopen(colFileName, "r");
    if( finput == NULL )
    {
        printf("No se pudo leer el archivo %s", colFileName);
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
            while( (c = getc(finput)) != '\n' ){}
            break;

        case 'p':
            getc(finput); // skip space
            int i;
            for( i = 0; i < BUFFERSIZE && (c = getc(finput)) != ' '; i++ ) 
                format[i] = c;
            format[i] = '\0';
            fscanf(finput, "%d %d\n", &nodes, &edges);

            // initialize graph structures
            g_vvbGraphTable = vector< vector< bool > >( nodes, vector< bool >(nodes, false) );
            g_vvEGraphAdjList = vector< vector< Edge > >( nodes, vector< Edge >() );

        case 'e':
            getc(finput); // skip space
            int node1, node2;
            fscanf(finput, "%d %d\n", &node1, &node2);
            --node1;
            --node2;
            g_vvbGraphTable[node1][node2] = true;
            g_vvbGraphTable[node2][node1] = true;
            g_vvEGraphAdjList[node1].push_back( Edge(node1, node2) );
            g_vvEGraphAdjList[node2].push_back( Edge(node2, node1) );
            break;

        default:
            break;
        }
    }

    fclose(finput);
}

int minAvailableColor( Node node, vector< int > coloring, int qtyColorsUsed )
{
    vector< bool > vbAdjColorUsed( qtyColorsUsed, false );
    vector< Edge >& vEAdj = g_vvEGraphAdjList[ node ];

    for ( int i = 0; i < vEAdj.size(); i++ )
    {
        int adjColor = coloring[ vEAdj[i].second ];
        if ( adjColor > 0 )
            vbAdjColorUsed[adjColor - 1] = true;
    }

    int minColor = 0;
    if( vbAdjColorUsed.size() > 0 )
    {
        while (vbAdjColorUsed[ minColor ])
        {
            minColor++;
        }
    }

    return minColor + 1;
}

int heuristicBFSUpperBound()
{
    int nNodes = g_vvbGraphTable.size();
    vector< int > vvnColoring( nNodes, 0 );
    list< Node > lNNodesStack;
    int nColors = 0;

    if ( nNodes == 0 )
        return nColors;

    lNNodesStack.push_back(0);
    while ( !lNNodesStack.empty() )
    {
        int nCurrentNode = lNNodesStack.front();
        lNNodesStack.pop_front();

        int nodeColor = minAvailableColor( nCurrentNode, vvnColoring, nColors );
        vvnColoring[ nCurrentNode ] = nodeColor;
        if ( nodeColor > nColors )
            nColors = nodeColor;

        vector< Edge >& adj = g_vvEGraphAdjList[ nCurrentNode ];
        for ( int i = 0; i < adj.size(); i++ )
        {
            Node neighbor = adj[i].second;
            if ( !vvnColoring[ neighbor ] )
                lNNodesStack.push_back( neighbor );
        }
    }

    return nColors;
}

int heuristicSequentialUpperBound()
{
    int nNodes = g_vvbGraphTable.size();
    vector< int > vvnColoring( nNodes, 0 );
    int nColors = 0;

    if ( nNodes == 0 )
        return nColors;

    vector< pair<int, Node> > vnNodeDegree;
    for( Node nCurrentNode = 0; nCurrentNode < nNodes; nCurrentNode++ )
        vnNodeDegree.push_back( pair<int, Node>(g_vvEGraphAdjList[nCurrentNode].size(), nCurrentNode) );

    sort( vnNodeDegree.begin(), vnNodeDegree.end() );
    for( int nCurrentNode = vnNodeDegree.size() - 1; nCurrentNode >= 0; nCurrentNode-- )
    {
        int nodeColor = minAvailableColor( nCurrentNode, vvnColoring, nColors );
        vvnColoring[ nCurrentNode ] = nodeColor;
        if ( nodeColor > nColors )
            nColors = nodeColor;
    }

    return nColors;
}

void colToLp(const char* colFileName, const char* lpFileName, int qtyColorsUpperBound)
{
    FILE* finput;
    FILE* foutput;

    finput = fopen(colFileName, "r");
    if( finput == NULL )
    {
        printf("No se pudo leer el archivo %s", colFileName);
        return;
    }

    foutput = fopen(lpFileName, "w");
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
            fprintf(foutput, "\\");
            while( (c = getc(finput)) != '\n' )
                fprintf(foutput, "%c", c);

            fprintf(foutput, "\n");
            break;

        case 'p':
            getc(finput); // skip space
            int i;
            for( i = 0; i < BUFFERSIZE && (c = getc(finput)) != ' '; i++ ) 
                format[i] = c;
            format[i] = '\0';
            fscanf(finput, "%d %d\n", &nodes, &edges);

            if( qtyColorsUpperBound < 0 )
                qtyColorsUpperBound = nodes;
            // begin building output file
            fprintf(foutput, "Minimize\n obj:");
            if( nodes > 0 )
                fprintf(foutput, " w0");
            for( i = 1; i < qtyColorsUpperBound; i++ )
                fprintf(foutput, " + w%d",i);


            fprintf(foutput, "\nSubject To");
            for( i = 0; i < nodes; i++ )
            {
                fprintf(foutput, "\n soloUnColorNodo%d: x%d0", i, i);

                for( int j = 1; j < qtyColorsUpperBound; j++ )
                    fprintf(foutput, " + x%d%d", i, j);

                fprintf(foutput, " = 1");
            }

            break;

        case 'e':
            getc(finput); // skip space
            int node1, node2;
            fscanf(finput, "%d %d\n", &node1, &node2);
            node1--;
            node2--;
            for(int j = 0; j < qtyColorsUpperBound; j++)
                fprintf(foutput, "\n adyDeDistintoColorNodo%dNodo%dColor%d: x%d%d + x%d%d - w%d <= 0",
                          node1, node2, j, node1, j, node2, j, j);
            break;

        default:
            break;
        }
    }

    fprintf(foutput, "\nBinary\n");
    for(int j = 0; j < qtyColorsUpperBound; j++)
        fprintf(foutput, " w%d\n", j);

    for(int i = 0; i < nodes; i++)
        for(int j = 0; j < qtyColorsUpperBound; j++)
            fprintf(foutput, " x%d%d\n", i, j);

    fprintf(foutput, "\nEnd");

    fclose(foutput);
    fclose(finput);
}

int heuristicUpperBound(const char* colFileName)
{
    buildGraphFromCol(colFileName);
    return min(heuristicBFSUpperBound(), heuristicSequentialUpperBound());
}

int main (int argc, char *argv[]){
    int status = 0;
    double init,end;
    string fileInput;
    size_t extensionOffset;

    CPXENVptr env = NULL;
    CPXLPptr  lp = NULL;
    machineEps = 0.000001;
    g_nHeuristicColors = -1;

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

    g_nHeuristicColors = heuristicUpperBound(fileInput.c_str());

    extensionOffset = fileInput.find_last_of(".");
    if( fileInput.find_last_of(".col") )
    {
        string fileOutput = fileInput.substr(0, extensionOffset);
        fileOutput += ".lp";
        colToLp(fileInput.c_str(), fileOutput.c_str(), g_nHeuristicColors);
        fileInput = fileOutput;
    }

    // Start problem solving

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