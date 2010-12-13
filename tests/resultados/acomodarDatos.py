#se usa para arreglar el orden en que se guardan los datos de resultado
#cuando se tiene ejmplo1,...,ejemplo10,ejemplo11, no lo ordena de esa forma
#sino que hace ejemplo1,ejemplo10,ejemplo11,ejemplo2,...
import sys

if(sys.argv > 1):
    nom = sys.argv[1]
else:
    print "falta el archivo por parametros"
    sys.exit()
    
f = open(nom, "r")
s = f.readline()
f.close()

#se que tengo que pasar siempre los primeros 4 valores al final
l = s.split(",")
l2 = l[5:12]
l2.extend(l[1:5])
#en l2 tengo los datos ordenados como string, con comillas
f = open(nom, "w")
f.write(str([float(i) for i in l2]))
f.close()