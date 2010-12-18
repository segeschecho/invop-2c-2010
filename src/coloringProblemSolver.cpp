/************************************************/
/*  Ejemplo IO                                  */
/************************************************/

#include "include.h"
using namespace std;

/********* Declaracion de constantes *********/

#define BUFFERSIZE 256
#define LOWER_BOUND_HEURISTIC_MAX_ITERATIONS 1
#define min(a, b) (a) < (b) ? (a) : (b)
#define max(a, b) (a) > (b) ? (a) : (b)

/********* Declaracion de tipos *********/

typedef int Node;
typedef pair<int, int> Edge;

struct cutinfo {
   CPXLPptr lp;
   int      nColumns;
};
typedef struct cutinfo CUTINFO, *CUTINFOptr;

/********* Declaracion de variables globales *********/
double machineEps;
vector< vector< bool > > g_vvbGraphTable;
vector< vector< Edge > > g_vvEGraphAdjList;
pair< int, int > g_nHeuristicColorBounds;
int g_nNodes;
int g_nEdges;

/********* Implementacion de Funciones *********/

typedef struct _columnValue {
    int nColumn;
    double nValue;
} ColumnValue;

inline int comparePtrDouble (const void * a, const void * b)
{
    return ( (**(double**)a) - (**(double**)b) );
}

static int CPXPUBLIC cortes (CPXCENVptr env,
                             void       *cbdata,
                             int        wherefrom,
                             void       *cbhandle,
                             int        *useraction_p)
{
    int status = 0;

    CUTINFOptr cutinfo = (CUTINFOptr) cbhandle;
    int nColumns = cutinfo->nColumns;
    double* x = new double[nColumns];

    *useraction_p = CPX_CALLBACK_DEFAULT;

    status = CPXgetcallbacknodex (env, cbdata, wherefrom, x, 0, nColumns - 1);
    if ( status )
    {
        printf("Failed to get node solution.\n");
        return status;
    }

    // corte por clique:
    double** pValues = new double*[nColumns];
    int nFractionalsLowerTo1 = 0;
    for (int col = 0; col < nColumns; col++)
    {
        if( x[col] < 1.0 )
            nFractionalsLowerTo1++;
        pValues[col] = &x[col];
    }
    // solo para debuguear:
    qsort(pValues, nColumns, sizeof(double*), comparePtrDouble);

    // para sacar el nodo correspondiente a ese valor se puede hacer
    // ((long)pValues[col] - (long)x) / sizeof(double)

    *useraction_p = CPX_CALLBACK_SET;
    return 0;
}

//////////////////////////////////////////////////////////////////////////

void buildGraphFromCol(const char* colFileName)
{
    FILE* finput;

    finput = fopen(colFileName, "r");
    if( finput == NULL )
    {
        printf("No se pudo leer el archivo %s\n", colFileName);
        return;
    }

    char c;
    char* format = new char[BUFFERSIZE];
    g_nNodes = 0;
    g_nEdges = 0;
    //leemos la informacion y armamos el grafo
    while( (c = getc(finput)) != EOF )
    {
        switch(c)
        {
        case 'c': //es un comentario
            while( (c = getc(finput)) != '\n' ){}
            break;

        case 'p': //descripcion del contenido
            getc(finput); // skip space
            int i;
            for( i = 0; i < BUFFERSIZE && (c = getc(finput)) != ' '; i++ )
                format[i] = c;
            format[i] = '\0';
            fscanf(finput, "%d %d\n", &g_nNodes, &g_nEdges);

            // initialize graph structures
            g_vvbGraphTable = vector< vector< bool > >( g_nNodes, vector< bool >(g_nNodes, false) );
            g_vvEGraphAdjList = vector< vector< Edge > >( g_nNodes, vector< Edge >() );
            break;

        case 'e': //es un eje
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

//////////////////////////////////////////////////////////////////////////

inline int getNonVisitedNode(bool* pbNonVisitedNodes)
{
    int i = 0;
    while (pbNonVisitedNodes[i] && i < g_nNodes)
        i++;

    if (i < g_nNodes)
        return i;

    return -1;
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
            minColor++;
    }

    return minColor + 1;
}

int heuristicBFSUpperBound()
{
    if ( g_nNodes == 0 )
        return 0;

    vector< int > vvnColoring( g_nNodes, 0 );
    list< Node > lNNodesQueue;
    bool *pbNonVisitedNodes = new bool[g_nNodes];
    Node nNVNode = -1;
    int nColors = 0;

    for(int i = 0; i < g_nNodes; i++)
        pbNonVisitedNodes[i] = false;

    // para cada nodo que no haya sido visitado hacemos bfs
    nNVNode =  getNonVisitedNode(pbNonVisitedNodes);
    while (nNVNode != -1)
    {
        lNNodesQueue.push_back(nNVNode);
        // coloreamos los nodos en orden bfs
        while ( !lNNodesQueue.empty() )
        {
            int nCurrentNode = lNNodesQueue.front();
            lNNodesQueue.pop_front();

            pbNonVisitedNodes[nCurrentNode] = true;

            int nodeColor = minAvailableColor( nCurrentNode, vvnColoring, nColors );
            vvnColoring[ nCurrentNode ] = nodeColor;
            if ( nodeColor > nColors )
                nColors = nodeColor;

            vector< Edge >& adj = g_vvEGraphAdjList[ nCurrentNode ];
            for ( int i = 0; i < adj.size(); i++ )
            {
                Node neighbor = adj[i].second;
                if ( !vvnColoring[ neighbor ] )
                    lNNodesQueue.push_back( neighbor );
            }
        }

        // buscamos el siguiente nodo no visitado
        nNVNode =  getNonVisitedNode(pbNonVisitedNodes);
    }

    return nColors;
}

int heuristicSequentialUpperBound()
{
    vector< int > vvnColoring( g_nNodes, 0 );
    int nColors = 0;

    if ( g_nNodes == 0 )
        return nColors;

    vector< pair<int, Node> > vnNodeDegree;
    for( Node nCurrentNode = 0; nCurrentNode < g_nNodes; nCurrentNode++ )
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

// New-Best-In algorithm
int maximalClique( Node startingNode )
{
    if ( g_nNodes == 0 )
        return 0;

    // Set V1 = V
    vector< bool > vbNodeIsAvailable( g_nNodes, true );
    int nAvailableNodes = g_nNodes;

    // Construct the vector of vertex degrees
    vector< int > vnVertexDegree;
    for ( Node node = 0; node < g_nNodes; node++ )
        vnVertexDegree.push_back( g_vvEGraphAdjList[node].size() );

    Node nCurrentNode = startingNode;
    Node nCurrentNodeDegree = vnVertexDegree[startingNode];

    // Set k = 1
    int nCliqueNodes = 0;

    while ( nAvailableNodes > 0 )
    {
        // Set Vk+1 = { v | v in Vk and v in N(vk) }.
        vector< bool > vbAux = vbNodeIsAvailable;
        for ( int node = 0; node < g_nNodes; node++ )
            vbAux[node] = vbNodeIsAvailable[node] && g_vvbGraphTable[nCurrentNode][node];

        for ( int node = 0; node < g_nNodes; node++ )
        {
            if( vbNodeIsAvailable[node] && ! vbAux[node] )
            {
                // Update degree(vk)
                nAvailableNodes--;
                vnVertexDegree[ node ] = -1;
                for( vector< Edge >::iterator it = g_vvEGraphAdjList[node].begin();
                                              it < g_vvEGraphAdjList[node].end();
                                              it++ )
                    vnVertexDegree[ it->second ]--;
            }
        }
        vbNodeIsAvailable = vbAux;
        nCliqueNodes++;

        nCurrentNode = -1;
        nCurrentNodeDegree = -1;
        // Choose a vertex vk from Vk such that degree(vk) is greatest
        for ( Node node = 0; node < g_nNodes; node++ )
        {
            if( vbNodeIsAvailable[ node ] )
            {
                int nNodeDegree = vnVertexDegree[ node ];
                if ( nNodeDegree > nCurrentNodeDegree )
                {
                    nCurrentNode = node;
                    nCurrentNodeDegree = nNodeDegree;
                }
            }
        }
    }

    return nCliqueNodes;
}

int heuristicCliqueLowerBound( int iterations )
{
    vector< pair<int, Node> > vnNodeDegree;
    for( Node nCurrentNode = 0; nCurrentNode < g_nNodes; nCurrentNode++ )
        vnNodeDegree.push_back( pair<int, Node>(g_vvEGraphAdjList[nCurrentNode].size(), nCurrentNode) );

    sort( vnNodeDegree.begin(), vnNodeDegree.end() );

    int maximalCliqueNodes = 0;

    for( int i = vnNodeDegree.size() - 1; i >= 0 && iterations > 0; i-- )
    {
        Node startingNode = vnNodeDegree[i].second;
        maximalCliqueNodes = max( maximalCliqueNodes, maximalClique(startingNode) );
        iterations--;
    }

    return maximalCliqueNodes;
}

pair<int, int> heuristicBounds(const char* colFileName)
{
    buildGraphFromCol(colFileName);
    pair<int, int> res( 0, g_nNodes );

    res.first = heuristicCliqueLowerBound(LOWER_BOUND_HEURISTIC_MAX_ITERATIONS);
    res.second = min(heuristicBFSUpperBound(), heuristicSequentialUpperBound());

    return res;
}

//////////////////////////////////////////////////////////////////////////

void colToLp(const char* colFileName, const char* lpFileName, int coloringLowerBound, int coloringUpperBound)
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
    g_nNodes = 0;
    g_nEdges = 0;
    int nColors = coloringUpperBound - coloringLowerBound;

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
            fscanf(finput, "%d %d\n", &g_nNodes, &g_nEdges);

            if( coloringUpperBound < 0 )
                coloringUpperBound = g_nNodes;
            // begin building output file
            fprintf(foutput, "Minimize\n obj:");
            if( g_nNodes > 0 )
                fprintf(foutput, " w0");
            for( i = 1; i < nColors; i++ )
                fprintf(foutput, " + w%d",i);

            fprintf(foutput, "\nSubject To");
            for( i = 0; i < g_nNodes; i++ )
            {
                fprintf(foutput, "\n soloUnColorNodo%d: x%d0", i, i);

                for( int j = 1; j < coloringUpperBound; j++ )
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
            for(int j = 0; j < nColors; j++)
                fprintf(foutput, "\n adyDeDistintoColorNodo%dNodo%dColor%d: x%d%d + x%d%d - w%d <= 0",
                          node1, node2, j, node1, j, node2, j, j);
            // no olvidar que tambien hay q agregar las restricciones de los colores que ya estan fijos en 1
            for(int j = nColors; j < coloringUpperBound; j++)
                fprintf(foutput, "\n adyDeDistintoColorNodo%dNodo%dColor%d: x%d%d + x%d%d <= 1",
                          node1, node2, j, node1, j, node2, j, j);

            break;

        default:
            break;
        }
    }

    fprintf(foutput, "\nBinary\n");
    for(int j = 0; j < coloringUpperBound; j++)
        fprintf(foutput, " w%d\n", j);

    for(int i = 0; i < g_nNodes; i++)
        for(int j = 0; j < coloringUpperBound; j++)
            fprintf(foutput, " x%d%d\n", i, j);

    fprintf(foutput, "\nEnd");

    fclose(foutput);
    fclose(finput);
}

int main (int argc, char *argv[]){
    int status = 0;
    double init,end;
    string fileInput;
    size_t extensionOffset;
    int numcols = 0;

    CPXENVptr env = NULL;
    CPXLPptr  lp = NULL;
    machineEps = 0.000001;
    g_nHeuristicColorBounds = pair<int, int>( -1, -1 );

    //Inicializamos el entorno
    env = CPXopenCPLEX (&status);
    if ( env == NULL )
    {
        printf("Problemas al abrir en entorno\n");
        goto TERMINATE;
    }

    ////Parametros para que no preprocese
    CPXsetintparam (env, CPX_PARAM_REDUCE,CPX_OFF);
    CPXsetintparam (env, CPX_PARAM_PRELINEAR, 0);
    CPXsetintparam (env, CPX_PARAM_MIPCBREDLP, CPX_OFF);

    //parametros para que no haga cortes
    CPXsetintparam(env,CPX_PARAM_CUTPASS,-1);             //Number of cutting plane passes
    CPXsetdblparam(env,CPX_PARAM_CUTSFACTOR,0.0);         //Row multiplier factor for cuts

    //Parametros para salida por pantalla
    CPXsetintparam (env, CPX_PARAM_SCRIND, CPX_OFF);      //para mostrar las iteraciones
    CPXsetintparam (env, CPX_PARAM_MIPINTERVAL, 1);       //para el log
    CPXsetintparam (env, CPX_PARAM_MIPDISPLAY, 3);        //muestra las soluciones y los cortes

    ///Parametros para elegir las estrategias de branching
    CPXsetintparam (env, CPX_PARAM_BRDIR, CPX_BRDIR_UP);            //CPX_BRDIR_DOWN o CPX_BRDIR_AUTO  o CPX_BRDIR_UP
    CPXsetintparam (env, CPX_PARAM_LBHEUR, CPX_ON);                 //local branching heuristic
    CPXsetintparam (env, CPX_PARAM_STRONGITLIM, 2);                 //MIP strong branching iterations limit 0(auto) o positivo
    CPXsetintparam (env, CPX_PARAM_ZEROHALFCUTS, 1);                // MIP zero-half cuts -1 0(auto) 1 2(agresivo)
    CPXsetintparam (env, CPX_PARAM_VARSEL, CPX_VARSEL_DEFAULT);     //variable selection, menos inviable, mas inviable
    CPXsetintparam (env, CPX_PARAM_NODESEL, CPX_NODESEL_BESTBOUND); //node selection strategy, BESTBOUND(default), DFS

    CUTINFO cutinfo;
    cutinfo.nColumns = 0;
    cutinfo.lp = 0;

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

    //Heuristicas iniciales
    g_nHeuristicColorBounds = heuristicBounds(fileInput.c_str());

    printf("Heuristicas: cota inf = %i, cota sup = %i \n", g_nHeuristicColorBounds.first, g_nHeuristicColorBounds.second);

    //si la extension no es .lp suponemos que es un archivo con formato .col
    extensionOffset = fileInput.find_last_of(".");
    if( fileInput.substr(extensionOffset, fileInput.size()).compare(".lp") != 0 )
    {
        string fileOutput = fileInput.substr(0, extensionOffset);
        fileOutput += ".lp";
        colToLp(fileInput.c_str(), fileOutput.c_str(), g_nHeuristicColorBounds.first, g_nHeuristicColorBounds.second);
        fileInput = fileOutput;
    }

    //leemos el problema
    status = CPXreadcopyprob (env, lp, fileInput.c_str(), NULL);
    if ( status )
    {
        printf("Problemas al leer el problema - %d\n", status);
        goto TERMINATE;
    }

    // inicializacion de info de cortes
    numcols = CPXgetnumcols (env, lp);
    cutinfo.lp = lp;
    cutinfo.nColumns = numcols;

    //asignamos la heuristica de separacion al problema
    status = CPXsetcutcallbackfunc (env, cortes, &cutinfo);
    if ( status )
    {
        printf("Problemas al setear rutina de separacion - %d.\n", status);
        goto TERMINATE;
    }

    CPXgettime(env,&init);
    status = CPXmipopt (env, lp);
    CPXgettime(env,&end);

    if ( status )
    {
        printf("Hubo un problema. Ver status - %d\n", status);
        goto TERMINATE;
    }

    status = CPXgetstat (env, lp);
    if (status == CPX_STAT_UNBOUNDED)        printf("Modelo no acotado\n");
    else if (status == CPXMIP_INFEASIBLE)    printf("Problema Infactible\n");
    else if (status == CPXMIP_TIME_LIM_FEAS) printf("Se fue de tiempo\n");
    else
    {
	    double objval;
        int nodos = CPXgetnodecnt(env, lp);
	    double* x;
        x = new double[numcols];
	    status =    CPXgetmipobjval (env, lp, &objval) ||
                    CPXgetmipx (env, lp, x, 0, numcols - 1);
        if ( status )
        {
            printf("Problemas al pedir la solucion - %d\n", status);
            goto TERMINATE;
        }

        // al valor del objetivo hay que sumarle el lowerBound de las heuristicas
        // de preprocesamiento, ya que le fijamos las variables de los colores
        // correspondientes a los que van de lowerBound a upperBound
	    printf("\Coloreo: %lf\n", objval + g_nHeuristicColorBounds.first);
	    printf("Nodos: %d\n", nodos);
        printf("Tiempo: %lf\n", end-init);
	    //printf("\nSolucion:\n");
	    //for(int i = 0; i < numcols; i++)
        //   printf("Col %d: %lf\n", i, x[i]);
    }

 TERMINATE:
    if ( lp != NULL ) {
        status = CPXfreeprob (env, &lp);
        if ( status )
            fprintf (stderr, "CPXfreeprob %d.\n",status);
    }
    if ( env != NULL ) {
        status = CPXcloseCPLEX (&env);
        if ( status )
            printf ("CPXclose %d\n",status);
    }

    return 0;
}
