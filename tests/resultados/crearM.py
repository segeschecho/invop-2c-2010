import sys

if(sys.argv > 1):
    TIPOSTEST = int(sys.argv[1])
else:
    print "Falta el parametro de la cantidad de tipos de test"
    sys.exit()
 
#tipos = range(1, 13)
tipos = range(1, TIPOSTEST)
resTiempos = ""
resNodos = ""

for t in tipos:
    #seteo el directorio
    path = "tipo"+str(t)
    #genero la linea de tiempos
    f = open(path + "\graficoTiempo.txt", "r")
    linea = "t" + str(t) + " = " + f.readline()
    resTiempos += linea + "\n"
    f.close()
    #genero la linea de nodos
    f = open(path + "\graficoNodos.txt", "r")
    linea = "t" + str(t) + " = " + f.readline()
    resNodos += linea + "\n"
    f.close()
#escribo los datos en el archivo de matlab
fresTiempos = open("tiempos.m", "w")
fresNodos = open("nodos.m", "w")

fresTiempos.write("x = [3:13]\n")
fresTiempos.write(resTiempos + "\n")

fresNodos.write("x = [3:13]\n")
fresNodos.write(resNodos + "\n")
#pongo el codigo para que haga el plot de todos los graficos
for t in tipos:
    fresTiempos.write("plot(x, t" + str(t) + ")\n")
    fresTiempos.write("legend('tipo"+str(t)+"')\n")
    fresTiempos.write("hold all\n")
    
    fresNodos.write("plot(x, t" + str(t) + ")\n")
    fresNodos.write("legend('tipo"+str(t)+"')\n")
    fresNodos.write("hold all\n")
l = "legend("
for t in range(1, TIPOSTEST):
    l += "'tipo" + str(t) + "',"

l += "'tipo" + str(tipos[-1]) + "')"
fresTiempos.write(l)
fresNodos.write(l)

fresTiempos.close()
fresNodos.close()