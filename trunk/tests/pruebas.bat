::ejecuta el tp para una serie de tests
set densidad=70
set inicio=15
set fin=50
set paso=5
set tipos=6

for /l %%T in (1,1,%tipos%) do (
for /l %%X in (%inicio%, %paso%, %fin%) do ..\src\tp%%T.exe test%%X-%densidad%.txt > resultados\tipo%%T\res-test%%X-%densidad%.txt
)