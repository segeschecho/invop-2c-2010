::ejecuta el tp para una serie de tests
set densidad=70
set inicio=3
set fin=13
set paso=1

::parametros tipo 1
set tipo=1
for /l %%X in (%inicio%, %paso%, %fin%) do ..\src\tp%tipo%.exe test%%X-%densidad%.txt > resultados\tipo%tipo%\res-test%%X-%densidad%.txt

::parametros tipo 2
set tipo=2
for /l %%X in (%inicio%, %paso%, %fin%) do ..\src\tp%tipo%.exe test%%X-%densidad%.txt > resultados\tipo%tipo%\res-test%%X-%densidad%.txt

::parametros tipo 3
set tipo=3
for /l %%X in (%inicio%, %paso%, %fin%) do ..\src\tp%tipo%.exe test%%X-%densidad%.txt > resultados\tipo%tipo%\res-test%%X-%densidad%.txt

::parametros tipo 4
set tipo=4
for /l %%X in (%inicio%, %paso%, %fin%) do ..\src\tp%tipo%.exe test%%X-%densidad%.txt > resultados\tipo%tipo%\res-test%%X-%densidad%.txt

::parametros tipo 5
set tipo=5
for /l %%X in (%inicio%, %paso%, %fin%) do ..\src\tp%tipo%.exe test%%X-%densidad%.txt > resultados\tipo%tipo%\res-test%%X-%densidad%.txt

::parametros tipo 6
set tipo=6
for /l %%X in (%inicio%, %paso%, %fin%) do ..\src\tp%tipo%.exe test%%X-%densidad%.txt > resultados\tipo%tipo%\res-test%%X-%densidad%.txt

::parametros tipo 7
set tipo=7
for /l %%X in (%inicio%, %paso%, %fin%) do ..\src\tp%tipo%.exe test%%X-%densidad%.txt > resultados\tipo%tipo%\res-test%%X-%densidad%.txt

::parametros tipo 8
set tipo=8
for /l %%X in (%inicio%, %paso%, %fin%) do ..\src\tp%tipo%.exe test%%X-%densidad%.txt > resultados\tipo%tipo%\res-test%%X-%densidad%.txt

::parametros tipo 9
set tipo=9
for /l %%X in (%inicio%, %paso%, %fin%) do ..\src\tp%tipo%.exe test%%X-%densidad%.txt > resultados\tipo%tipo%\res-test%%X-%densidad%.txt

::parametros tipo 10
set tipo=10
for /l %%X in (%inicio%, %paso%, %fin%) do ..\src\tp%tipo%.exe test%%X-%densidad%.txt > resultados\tipo%tipo%\res-test%%X-%densidad%.txt

::parametros tipo 11
set tipo=11
for /l %%X in (%inicio%, %paso%, %fin%) do ..\src\tp%tipo%.exe test%%X-%densidad%.txt > resultados\tipo%tipo%\res-test%%X-%densidad%.txt

::parametros tipo 12
set tipo=12
for /l %%X in (%inicio%, %paso%, %fin%) do ..\src\tp%tipo%.exe test%%X-%densidad%.txt > resultados\tipo%tipo%\res-test%%X-%densidad%.txt
