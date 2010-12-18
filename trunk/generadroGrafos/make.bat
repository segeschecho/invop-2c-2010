@echo off
::con 70% de densidad
set densidad=70
set inicio=50
set fin=50
set paso=1
::        start, step, end
for /l %%X in (%inicio%, %paso%, %fin%) do generador.py %%X %densidad% > test%%X-%densidad%.txt
::con 80%
set densidad=80
for /l %%X in (%inicio%, %paso%, %fin%) do generador.py %%X %densidad% > test%%X-%densidad%.txt
::con 90%
set densidad=90
for /l %%X in (%inicio%, %paso%, %fin%) do generador.py %%X %densidad% > test%%X-%densidad%.txt
