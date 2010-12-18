::for /r resultados %%X in (res*.txt) do echo %%X

::para cada directorio tipo1, tipo2...
for /d %%X in (tipo*) do (
    :: obtengo los tiempos y los muestro en formato matlab
    del %%X\graficoTiempo.txt
    del %%X\graficoNodos.txt
    for %%Y in (%%X\res*) do extractor.py %%Y
    acomodarDatos.py %%X\graficoTiempo.txt
    acomodarDatos.py %%X\graficoNodos.txt
)
::Creo los archivos matlab de acuerdo a la cantidad de tipos de test
::(siempre uno mas de los que son, si son 12 tipos, poner 13)
crearM.py 13