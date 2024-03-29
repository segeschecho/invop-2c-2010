/************************************************/
/*  Ejemplo IO                                  */
/************************************************/

#include "include.h"
using namespace std;

/********* Declaracion de constantes *********/

#define EPSILON 0.000001
#define BUFFERSIZE 256
#define min(a, b) (a) < (b) ? (a) : (b)
#define max(a, b) (a) > (b) ? (a) : (b)

/********* Declaracion de tipos *********/

typedef int Node;
typedef pair<int, int> Edge;
typedef unsigned int uint;

/********* Declaracion de variables globales *********/

bool g_bUseCPLEXInitialHeuristics;
bool g_bUseUserInitialHeuristics;
bool g_bUseUserCuts;
bool g_bUseCliqueCuts;
bool g_bUseOddCycleCuts;
bool g_bUseCPLEXCuts;
vector< vector< bool > > g_vvbGraphTable;
vector< vector< Edge > > g_vvEGraphAdjList;
pair< int, int > g_nHeuristicColorBounds;
int g_nNodes;
int g_nEdges;
int g_nColorColumns;
int g_nColumns;
uint g_nCortesClique;
uint g_nCortesOddHole;


int g_nMaxIterationsLowerBoundHeuristic;
int g_nMaxIterationsUpperBoundHeuristic;
int g_nMaxOddCycleCutIterations;
int g_nMaxOddCycleCutVerify;

/********* Implementacion de Funciones *********/

typedef struct _columnValue {
    int nColumn;
    double nValue;
} ColumnValue;

inline int nodeToColumn( Node nNode )
{
    return g_nColorColumns + nNode*g_nHeuristicColorBounds.second;
}

inline Node columnToNode( int nColumn )
{
    return (nColumn - g_nColorColumns) / g_nHeuristicColorBounds.second;
}

inline int comparePtrDouble (const void * a, const void * b)
{
    double nVal = (**(double**)b) - (**(double**)a);
    if( nVal > 0 )
        return 1;
    if( nVal < 0 )
        return -1;
    return 0;
}

int tryToAddOddCycleCut(CPXCENVptr env,
                        void       *cbdata,
                        int        wherefrom,
                        double*    x,
                        int        nCycleSize,
                        Node       nTreeNode1,
                        Node       nTreeNode2,
                        Node       nRootNode,
                        Node*      pNodeParent)
{
    int status = 0;

    int nColorsCount      = g_nHeuristicColorBounds.second;
    int* arrCutInd        = new int[ nCycleSize + 1 ];
    double* arrCutCoefVal = new double[ nCycleSize + 1 ];
    Node nCurrNode        = nTreeNode1;
    int nArrIdx           = 0;

    // agregamos los nodos a los arreglos
    while( nCurrNode != nRootNode )
    {
        int nColumn = nodeToColumn(nCurrNode);
        arrCutInd[nArrIdx] = nColumn;
        arrCutCoefVal[nArrIdx] = 1;

        nCurrNode = pNodeParent[nCurrNode];
        nArrIdx++;
    }

    nCurrNode = nTreeNode2;
    while( nCurrNode != nRootNode )
    {
        int nColumn = nodeToColumn(nCurrNode);
        arrCutInd[nArrIdx] = nColumn;
        arrCutCoefVal[nArrIdx] = 1;

        nCurrNode = pNodeParent[nCurrNode];
        nArrIdx++;
    }

    // agrego la raiz
    int nColumn            = nodeToColumn(nRootNode);
    arrCutInd[nArrIdx]     = nColumn;
    arrCutCoefVal[nArrIdx] = 1;

    // inicializo la columna y el coeficiente del color
    nArrIdx++;
    arrCutInd[nArrIdx]     = 0;
    arrCutCoefVal[nArrIdx] = -1;
    
    // seteamos el color para todo color no fijo
    for( int nColor = 0; nColor < g_nColorColumns && !status; nColor++ )
    {
        double nColumnValueSum = 0;
        int nzcnt = nCycleSize + 1;
        for( int i = 0; i < nCycleSize; i++ )
            nColumnValueSum += x[ arrCutInd[i] ];

        double nColorValue = x[nColor];
        if( nColumnValueSum > ((nCycleSize - 1) / 2)*nColorValue + EPSILON )
        {
            status = CPXcutcallbackadd(  env, cbdata, wherefrom,
                                         nzcnt, 0, 'L', arrCutInd,
                                         arrCutCoefVal, 0 );
            g_nCortesOddHole++;
        }

        // cambio el color
        for( int i = 0; i < nzcnt; i++ )
            arrCutInd[i]++;
    }

    // seteamos el color para todo color fijo
    for( int nColor = g_nColorColumns; nColor < nColorsCount && !status; nColor++ )
    {
        double nColumnValueSum = 0;
        int nzcnt = nCycleSize;
        for( int i = 0; i < nCycleSize; i++ )
            nColumnValueSum += x[ arrCutInd[i] ];

        if( nColumnValueSum > ((nCycleSize - 1) / 2) + EPSILON )
        {
            status = CPXcutcallbackadd(  env, cbdata, wherefrom,
                                         nzcnt, 1, 'L', arrCutInd,
                                         arrCutCoefVal, 0 );
            g_nCortesOddHole++;
        }

        // cambio el color
        for( int i = 0; i < nzcnt; i++ )
            arrCutInd[i]++;
    }

    if ( status )
        printf ("Failed to add odd cycle cut.\n");

    delete [] arrCutInd;
    delete [] arrCutCoefVal;
    return status;
}

int corteCicloImpar(CPXCENVptr env,
                    void*      cbdata,
                    int        wherefrom,
                    void       *cbhandle,
                    double*    x,
                    double**   pOrdColumnValues,
                    int        nColumns )
{
    int status = 0;

    if ( g_nNodes == 0 )
        return status;

    int* pNodeLevel           = new int[g_nNodes];
    Node* pNodeRoot           = new Node[g_nNodes];
    Node* pNodesQueue         = new Node[g_nNodes];
    Node* pNodeParent         = new Node[g_nNodes];
    bool* pAlreadyStartedFrom = new bool[g_nNodes];

    for (int i = 0; i < g_nNodes; i++)
        pAlreadyStartedFrom[i] = false;

    int nMaxIterations = min( g_nMaxOddCycleCutIterations, nColumns );

    for( int nStartingNodeIdx = 0; nStartingNodeIdx < nMaxIterations; nStartingNodeIdx++ )
    {
        int nMaxCyclesVerification = g_nMaxOddCycleCutVerify;
        int nStartingColumn = ((long)pOrdColumnValues[nStartingNodeIdx] - (long)x) / sizeof(double);
        Node nStartingNode = columnToNode(nStartingColumn);
        if( pAlreadyStartedFrom[nStartingNode] )
            continue;

        pAlreadyStartedFrom[nStartingNode] = true;

        for (int i = 0; i < g_nNodes; i++)
        {
            pNodesQueue[i] = -1;
            pNodeLevel[i]  = -1;
            pNodeRoot[i]   = -1;
            pNodeParent[i] = -1;
        }

        size_t nQueueSize = 0;
        // inicializamos los arreglos para el nodo inicial
        pNodesQueue[nQueueSize++] = nStartingNode;
        pNodeLevel[nStartingNode] = 0;
        pNodeRoot[nStartingNode]  = nStartingNode;

        // inicializamos los arreglos para los
        // nodos vecinos al nodo inicial
        vector< Edge >& vAdjRootNode = g_vvEGraphAdjList[ nStartingNode ];
        size_t nAdjCount = vAdjRootNode.size();
        for ( uint i = 0; i < nAdjCount; i++)
        {
            Node nRootChild = vAdjRootNode[i].second;

            pNodesQueue[nQueueSize++] = nRootChild;
            pNodeLevel[nRootChild]    = 1;
            pNodeRoot[nRootChild]     = nRootChild;
            pNodeParent[nRootChild]   = nStartingNode;
        }

        uint nIdx = 1;
        int nLevel = 2;
        // hacemos bfs
        while (nIdx < nQueueSize && nMaxCyclesVerification > 0)
        {
            int nNewSize = nQueueSize;
            while (nIdx < nQueueSize && nMaxCyclesVerification > 0)
            {
                Node nParentNode = pNodesQueue[nIdx];
                vector< Edge >& vAdjCurrNode = g_vvEGraphAdjList[ nParentNode ];
                size_t nAdjCount = vAdjCurrNode.size();

                for (uint i = 0; i < nAdjCount && nMaxCyclesVerification > 0; i++)
                {
                    Node nChildNode = vAdjCurrNode[i].second;
                    
                    // si ya tiene un nivel asignado, el hijo ya fue visitado
                    if (pNodeLevel[nChildNode] > -1)
                    {
                        // si el hijo no es su padre
                        if ( pNodeParent[nParentNode] != nChildNode )
                        {
                            int nCycleSize = pNodeLevel[nChildNode] + nLevel;
                            // si tienen distintas raices y son una cantidad impar de nodos >= 5
                            if ( pNodeRoot[nChildNode] != pNodeRoot[nParentNode] && nCycleSize > 4 && nCycleSize % 2 == 1 )
                            {
                                // tenemos un ciclo impar! Intentamos agregar un corte
                                tryToAddOddCycleCut(env, cbdata, wherefrom, x, nCycleSize, nParentNode,
                                                    nChildNode, nStartingNode, pNodeParent);
                                nMaxCyclesVerification--;
                            }
                        }
                    }
                    else
                    {
                        // el nodo es nuevo, no explorado anteriormente
                        pNodesQueue[nNewSize++] = nChildNode;
                        pNodeLevel[nChildNode] = nLevel;
                        pNodeRoot[nChildNode]  = pNodeRoot[nParentNode];
                        pNodeParent[nChildNode]= nParentNode;
                    }
                }

                nIdx++;
            }

            nLevel++;
            nQueueSize = nNewSize;
        }
    }

    delete [] pNodeLevel;
    delete [] pNodeRoot;
    delete [] pNodesQueue;
    delete [] pNodeParent;
    delete [] pAlreadyStartedFrom;

    return status;
}

int corteClique(CPXCENVptr env,
                void       *cbdata,
                int        wherefrom,
                void       *cbhandle,
                double*    x,
                double**   pOrdColumnValues,
                int        nColumns )
{
    int status = 0;

    int nColumnsPerGraphNode = g_nHeuristicColorBounds.second;

    // iniciamos la busqueda de una clique
    int nColumn = ((long)pOrdColumnValues[0] - (long)x) / sizeof(double);
    // estas cuentas estan fuertemente respaldadas por el formato del .col generado
    int nColor = (nColumn - g_nColorColumns) % nColumnsPerGraphNode;
    double nColorValue = (nColor < g_nColorColumns) ? x[nColor] : 1;
    double nColumnValueSum = 0;

    vector<Node> vCliqueNodes;
    vector<int> vCliqueColumns;
    size_t nCurrentCliqueSize = 0;
    // vamos a ir agregando de a un nodo por vez, verificando siempre que el nodo
    // pertenezca a la clique y est� pintado del mismo color j, y que la sumatoria de los colores
    // sea menor o igual a wj + EPSILON
    for( int nIdx = 0; nIdx < nColumns && nColumnValueSum <= nColorValue + EPSILON; nIdx++ )
    {
        int nCurrentColumn = ((long)pOrdColumnValues[nIdx] - (long)x) / sizeof(double);
        int nCurrentColor = (nCurrentColumn - g_nColorColumns) % nColumnsPerGraphNode;
        if( nCurrentColor != nColor )
            continue;

        Node nCurrentNode = columnToNode(nCurrentColumn);

        bool bAddToClique = true;
        // verificamos que el nodo actual sea adyacente a todos los nodos de la clique
        for( uint i = 0; i < nCurrentCliqueSize && bAddToClique; i++ )
            bAddToClique = g_vvbGraphTable[ nCurrentNode ][ vCliqueNodes[i] ];

        if( bAddToClique )
        {
            // agregamos el nodo actual a la clique y actualizamos la sumatoria
            vCliqueNodes.push_back( nCurrentNode );
            vCliqueColumns.push_back( nCurrentColumn );

            nColumnValueSum += x[ nCurrentColumn ];
            nCurrentCliqueSize++;
        }
    }

    // si la sumatoria es mayor al valor del color + EPSILON,
    // agregar un corte por desigualdad clique
    if( nColumnValueSum > nColorValue + EPSILON )
    {
        int* cutInd = new int[ nCurrentCliqueSize + 1 ];
        double* cutCoefVal = new double[ nCurrentCliqueSize + 1 ];
        for(uint i = 0; i < nCurrentCliqueSize; i++)
        {
            cutInd[i]       = vCliqueColumns[i];
            cutCoefVal[i]   = 1;
        }

        int nzcnt = nCurrentCliqueSize;
        double rhs = 1;
        if (nColor < g_nColorColumns)
        {
            cutInd[nCurrentCliqueSize] = nColor;
            cutCoefVal[nCurrentCliqueSize] = -1;
            nzcnt++;
            rhs = 0;
        }

        status = CPXcutcallbackadd(  env, cbdata, wherefrom,
                                     nzcnt, rhs, 'L', cutInd,
                                     cutCoefVal, 0 );
        g_nCortesClique++;
        if ( status ) {
            printf ("Failed to add clique cut.\n");
            return status;
        }
      
        delete [] cutInd;
        delete [] cutCoefVal;
    }

    return status;
}

static int CPXPUBLIC cortes (CPXCENVptr env,
                             void       *cbdata,
                             int        wherefrom,
                             void       *cbhandle,
                             int        *useraction_p)
{
    int status = 0;

    *useraction_p = CPX_CALLBACK_DEFAULT;

    double* x = new double[g_nColumns];
    status = CPXgetcallbacknodex(env, cbdata, wherefrom, x, 0, g_nColumns - 1);
    if ( status )
    {
        printf("Failed to get node solution.\n");
        return status;
    }

    int nNonColorVariables = g_nColumns - g_nColorColumns;

    int nValuesLowerTo1 = 0;
    for (int col = 0; col < nNonColorVariables; col++)
    {
        if( x[col + g_nColorColumns] < 1.0 )
            nValuesLowerTo1++;
    }

    double** pValues = new double*[nValuesLowerTo1];
    int nAux = 0;
    for (int col = 0; col < nNonColorVariables; col++)
    {
        if( x[col + g_nColorColumns] < 1.0 )
        {
            pValues[nAux] = &x[col + g_nColorColumns];
            nAux++;
        }
    }
    qsort(pValues, nValuesLowerTo1, sizeof(double*), comparePtrDouble);

    // para sacar el nodo correspondiente a ese valor se puede hacer
    // ((long)pValues[col] - (long)x) / sizeof(double)

    // corte por clique:
    if(g_bUseCliqueCuts)
        corteClique     (env, cbdata, wherefrom, cbhandle, x, pValues, nValuesLowerTo1);
    if(g_bUseOddCycleCuts)
        corteCicloImpar (env, cbdata, wherefrom, cbhandle, x, pValues, nValuesLowerTo1);

    delete [] pValues;
    delete [] x;

    *useraction_p = CPX_CALLBACK_SET;
    return status;
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
            if( !g_vvbGraphTable[node1][node2] && !g_vvbGraphTable[node2][node1] )
            {
                g_vvbGraphTable[node1][node2] = true;
                g_vvbGraphTable[node2][node1] = true;
                g_vvEGraphAdjList[node1].push_back( Edge(node1, node2) );
                g_vvEGraphAdjList[node2].push_back( Edge(node2, node1) );
            }
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

    int minColor;
    for ( minColor = 0; minColor < vbAdjColorUsed.size() && vbAdjColorUsed[ minColor ]; minColor++ ){}

    return minColor + 1;
}

int BFSColoring( Node nStartingNode )
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
    nNVNode =  nStartingNode;
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

int heuristicBFSUpperBound(int nIterations, vector< pair< int, Node > >& vnNodesOrderedByDegree)
{
    int nBFSColoringNumber = 0;
    if( vnNodesOrderedByDegree.size() > 0 )
        nBFSColoringNumber = BFSColoring(vnNodesOrderedByDegree[vnNodesOrderedByDegree.size() - 1].second);

    for( int i = vnNodesOrderedByDegree.size() - 2; i >= 0 && nIterations > 0; i-- )
    {
        Node startingNode = vnNodesOrderedByDegree[i].second;
        nBFSColoringNumber = min( nBFSColoringNumber, BFSColoring(startingNode) );
        nIterations--;
    }

    return nBFSColoringNumber;
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

int heuristicCliqueLowerBound( int iterations, vector< pair< int, Node > >& vnNodesOrderedByDegree )
{
    int maximalCliqueNodes = 0;

    for( int i = vnNodesOrderedByDegree.size() - 1; i >= 0 && iterations > 0; i-- )
    {
        Node startingNode = vnNodesOrderedByDegree[i].second;
        maximalCliqueNodes = max( maximalCliqueNodes, maximalClique(startingNode) );
        iterations--;
    }

    return maximalCliqueNodes;
}

void heuristicBounds()
{
    vector< pair<int, Node> > vnNodeDegree;
    for( Node nCurrentNode = 0; nCurrentNode < g_nNodes; nCurrentNode++ )
        vnNodeDegree.push_back( pair<int, Node>(g_vvEGraphAdjList[nCurrentNode].size(), nCurrentNode) );

    sort( vnNodeDegree.begin(), vnNodeDegree.end() );

    g_nHeuristicColorBounds.first = heuristicCliqueLowerBound(g_nMaxIterationsLowerBoundHeuristic, vnNodeDegree);
    g_nHeuristicColorBounds.second = min( heuristicBFSUpperBound(g_nMaxIterationsUpperBoundHeuristic, vnNodeDegree),
                                          heuristicSequentialUpperBound() );
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
    g_nColorColumns = coloringUpperBound - coloringLowerBound;

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
            if( g_nColorColumns > 0 )
                fprintf(foutput, " w0");
            for( i = 1; i < g_nColorColumns; i++ )
                fprintf(foutput, " + w%d",i);

            fprintf(foutput, "\nSubject To");
            for( i = 0; i < g_nNodes; i++ )
            {
                fprintf(foutput, "\n soloUnColorNodo%d: x%d_0", i, i);

                for( int j = 1; j < coloringUpperBound; j++ )
                    fprintf(foutput, " + x%d_%d", i, j);

                fprintf(foutput, " = 1");
            }

            break;

        case 'e':
            getc(finput); // skip space
            int node1, node2;
            fscanf(finput, "%d %d\n", &node1, &node2);
            node1--;
            node2--;
            for(int j = 0; j < g_nColorColumns; j++)
                fprintf(foutput, "\n adyDeDistintoColorNodo%dNodo%dColor%d: x%d_%d + x%d_%d - w%d <= 0",
                          node1, node2, j, node1, j, node2, j, j);
            // no olvidar que tambien hay q agregar las restricciones de los colores que ya estan fijos en 1
            for(int j = g_nColorColumns; j < coloringUpperBound; j++)
                fprintf(foutput, "\n adyDeDistintoColorNodo%dNodo%dColor%d: x%d_%d + x%d_%d <= 1",
                          node1, node2, j, node1, j, node2, j, j);

            break;

        default:
            break;
        }
    }

    fprintf(foutput, "\nBinary\n");
    for(int j = 0; j < g_nColorColumns; j++)
        fprintf(foutput, " w%d\n", j);

    for(int i = 0; i < g_nNodes; i++)
        for(int j = 0; j < coloringUpperBound; j++)
            fprintf(foutput, " x%d_%d\n", i, j);

    fprintf(foutput, "End");

    fclose(foutput);
    fclose(finput);
}

int solveProblem(string sFileName)
{
    int status = 0;
    double init,end;
    size_t extensionOffset;
    int numcols = 0;

    CPXENVptr env = NULL;
    CPXLPptr  lp = NULL;

    //Inicializamos el entorno
    env = CPXopenCPLEX (&status);
    if ( env == NULL )
    {
        printf("Problemas al abrir en entorno\n");
        goto TERMINATE;
    }

    // initializacion de varibales globales
    g_nNodes = 0;
    g_nEdges = 0;
    g_nColorColumns = 0;
    g_nColumns = 0;
    g_nCortesClique = 0;
    g_nCortesOddHole = 0;

    g_bUseCPLEXInitialHeuristics = false;
    g_bUseUserInitialHeuristics = true;
    g_bUseCPLEXCuts = true;
    g_bUseUserCuts = true;
    g_bUseOddCycleCuts = true;
    g_bUseCliqueCuts = true;
    g_nMaxIterationsLowerBoundHeuristic = 3;
    g_nMaxIterationsUpperBoundHeuristic = 3;
    g_nMaxOddCycleCutIterations = 1;
    g_nMaxOddCycleCutVerify = 25;

    if( !g_bUseCPLEXInitialHeuristics )
    {
        ////Parametros para que no preprocese
        CPXsetintparam (env, CPX_PARAM_REDUCE,CPX_OFF);
        CPXsetintparam (env, CPX_PARAM_PRELINEAR, 0);
        CPXsetintparam (env, CPX_PARAM_MIPCBREDLP, CPX_OFF);
    }
    
    //parametros para que no haga cortes
    if(!g_bUseCPLEXCuts)
    {
        CPXsetintparam(env, CPX_PARAM_DISJCUTS, -1);
        CPXsetintparam(env, CPX_PARAM_FLOWCOVERS, -1);
        CPXsetintparam(env, CPX_PARAM_FLOWPATHS, -1);
        CPXsetintparam(env, CPX_PARAM_FRACCUTS, -1);
        CPXsetintparam(env, CPX_PARAM_GUBCOVERS, -1);
        CPXsetintparam(env, CPX_PARAM_IMPLBD, -1);
        CPXsetintparam(env, CPX_PARAM_CLIQUES, -1);
        CPXsetintparam(env, CPX_PARAM_CUTPASS, -1);             //Number of cutting plane passes
        CPXsetdblparam(env, CPX_PARAM_CUTSFACTOR, 0.0);         //Row multiplier factor for cuts
    }

    //Parametros para salida por pantalla
    CPXsetintparam (env, CPX_PARAM_SCRIND, CPX_OFF);      //para mostrar las iteraciones
    CPXsetintparam (env, CPX_PARAM_MIPINTERVAL, 1);       //para el log
    CPXsetintparam (env, CPX_PARAM_MIPDISPLAY, 3);        //muestra las soluciones y los cortes

    ///Parametros para elegir las estrategias de branching
    CPXsetintparam (env, CPX_PARAM_BRDIR, CPX_BRDIR_UP);          //CPX_BRDIR_DOWN o CPX_BRDIR_AUTO  o CPX_BRDIR_UP
    CPXsetintparam (env, CPX_PARAM_LBHEUR, CPX_OFF);              //local branching heuristic OFF(default)
    CPXsetintparam (env, CPX_PARAM_STRONGITLIM, 1);               //MIP strong branching iterations limit 0(auto) o positivo
    CPXsetintparam (env, CPX_PARAM_VARSEL, CPX_VARSEL_DEFAULT);   //variable selection, menos inviable, mas inviable
    CPXsetintparam (env, CPX_PARAM_NODESEL, CPX_NODESEL_BESTEST); //node selection strategy, BESTBOUND(default), DFS, BESTEST
    CPXsetintparam (env, CPX_PARAM_ZEROHALFCUTS, -1);

    lp = CPXcreateprob (env, &status, "pp.lp");
    if ( lp == NULL )
    {
        printf("Problemas al crear el LP.\n");
        goto TERMINATE;
    }

    // Heuristicas iniciales
    CPXgettime(env,&init);
    buildGraphFromCol(sFileName.c_str());
    g_nHeuristicColorBounds = pair<int, int>( 0, g_nNodes );
    if (g_bUseUserInitialHeuristics)
    {
        heuristicBounds();
        printf( "Heuristicas: cota inf = %i, cota sup = %i \n",
                g_nHeuristicColorBounds.first,
                g_nHeuristicColorBounds.second );
    }

    g_nColorColumns = g_nHeuristicColorBounds.second - g_nHeuristicColorBounds.first;

    //si la extension no es .lp suponemos que es un archivo con formato .col
    extensionOffset = sFileName.find_last_of(".");
    if( sFileName.substr(extensionOffset, sFileName.size()).compare(".lp") != 0 )
    {
        string fileOutput = sFileName.substr(0, extensionOffset);
        fileOutput += ".lp";
        colToLp(sFileName.c_str(), fileOutput.c_str(), g_nHeuristicColorBounds.first, g_nHeuristicColorBounds.second);
        sFileName = fileOutput;
    }

    //leemos el problema
    status = CPXreadcopyprob (env, lp, sFileName.c_str(), NULL);
    if ( status )
    {
        printf("Problemas al leer el problema - %d\n", status);
        goto TERMINATE;
    }

    // inicializacion de info de cortes
    numcols = CPXgetnumcols (env, lp);
    g_nColumns = numcols;

    if(g_bUseUserCuts)
    {
        // asignamos la heuristica de separacion al problema
        status = CPXsetcutcallbackfunc (env, cortes, NULL);
        if ( status )
        {
            printf("Problemas al setear rutina de separacion - %d.\n", status);
            goto TERMINATE;
        }
    }

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
	    printf("Coloreo: %lf\n", objval + g_nHeuristicColorBounds.first);
	    printf("Nodos: %d\n", nodos);
        printf("Tiempo: %lf\n", end-init);
        printf("Cortes clique usuario: %d", g_nCortesClique);
        printf("Cortes odd-hole usuario: %d", g_nCortesOddHole);
	    //printf("\nSolucion:\n");
	    //for(int i = 0; i < numcols; i++)
        //   printf("Col %d: %lf\n", i, x[i]);
    }

 TERMINATE:
    if ( lp != NULL ) {
        status = CPXfreeprob (env, &lp);
        if ( status )
            printf ("CPXfreeprob %d.\n",status);
    }
    if ( env != NULL ) {
        status = CPXcloseCPLEX (&env);
        if ( status )
            printf ("CPXclose %d\n",status);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    string sFileInput;

    if( argv[1] != 0 )
        sFileInput = argv[1];
    else
        sFileInput = "miles500.col";

    return solveProblem(sFileInput);
}
