::ejecuta el tp para una serie de tests

::grafos con 70% de densidad
for %%X in (test*70.txt) do ..\src\tp.exe %%X > resultados\%%X

::gafos con 80% de densidad