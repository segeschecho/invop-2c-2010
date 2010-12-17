::ejecuta el tp para una serie de tests
set densidad=70
set inicio=3
set fin=13
set paso=1
set tipos=24

for /l %%T in (1,1,%tipos%) do (
for /l %%X in (%inicio%, %paso%, %fin%) do ..\src\tp%%T.exe test%%X-%densidad%.txt > resultados\tipo%%T\res-test%%X-%densidad%.txt
)