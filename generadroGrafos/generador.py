import sys
import random

#-----------------------------funciones--------------------------------#
#obtiene un vecino que no sea el mismo nodo
def getVecino(nodo, nodos):
    vecino = random.randint(0, nodos - 1)
    while vecino == nodo:
        vecino = random.randint(0, nodos - 1)
    return vecino

#-------------------------------main-----------------------------------#

if(len(sys.argv) == 3):
    nodos = int(sys.argv[1])
    densidad = int(sys.argv[2])
else:
    print "Modo de uso: %s numNodos densidad" % sys.argv[0]
    print "densidad = 0% - 100 %"
    sys.exit()

#ejes en base a la entrada
maxEjes = nodos * (nodos - 1) / 2
minEjes = nodos - 1
ejesACrear = int(densidad * maxEjes / 100)

if ejesACrear < minEjes:
    print "No se puede crear un grafo sin nodos aislados con esa densidad"
    sys.exit()

#lista de adyacentes
l = []
#diccionario para evitar duplicados
d = {}

#agrego un eje a cada nodo de manera que el grafo sea conexo
for nodo in range(nodos):
    vecino = getVecino(nodo, nodos)
    if d.has_key(nodo):
        l.append([])
        continue
    l.append([vecino])
    d[nodo] = {vecino: True}
    if(d.has_key(vecino)):
        d[vecino][nodo] = True
    else:
        d[vecino] = {nodo: True}
    ejesACrear -= 1

if ejesACrear > 0:
    for nodo in range(nodos):
        cantVecinos = len(d[nodo].keys())
        nuevosVecinos = random.randint(0, ejesACrear - 1)
        #agrego K vecinos al nodo (si se puede)
        while cantVecinos < nodos - 1 and nuevosVecinos > 0:
            #puedo agregar vecinos
            vecino = getVecino(nodo, nodos)
            while d.get(nodo).has_key(vecino):
                vecino = getVecino(nodo, nodos)            
            l[nodo].append(vecino)
            d[nodo][vecino] = True
            d[vecino][nodo] = True
            
            cantVecinos += 1
            nuevosVecinos -= 1
            ejesACrear -= 1

#muestro el grafo
print "p edge %i %i" % (nodos, int(densidad * maxEjes / 100) - ejesACrear)
for n in range(nodos):
    for v in l[n]:
        print "e %i %i" % (n+1, v+1)
