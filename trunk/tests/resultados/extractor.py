import sys

#tomo el nombre de archivo de la entrada
if sys.argv > 1:
    nom = sys.argv[1]
else:
    print "Falta el nombre del archivo como parametro"
    sys.exit()

#abro el archivo
f = open(nom, "r")
cont = f.read()
#busco la cantidad de nodos
inicioN = cont.find("Nodos:")
#busco el tiempo
inicioT = cont.find("Tiempo:")
nodos = cont[inicioN + 7 : inicioT - 1]
tiempo = cont[inicioT + 8 : len(cont) - 1]

#obtengo el directorio actual
directorio = nom[0:nom.find("\\")]

#guardo el tiempo y los nodos en archivois distintos
fresT = open(directorio+"\graficoTiempo.txt", "a")
fresT.write(","+tiempo)

fresN = open(directorio+"\graficoNodos.txt", "a")
fresN.write(","+nodos)

fresN.close()
fresT.close()
f.close()