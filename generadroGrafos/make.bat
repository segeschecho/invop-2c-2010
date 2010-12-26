@echo off
::con 70% de densidad
set densidad=80
set inicio=60
set fin=70
set paso=5
::        start, step, end
for /l %%X in (%inicio%, %paso%, %fin%) do generador.py %%X %densidad% > test%%X-%densidad%.txt
::con 80%
set densidad=90
for /l %%X in (%inicio%, %paso%, %fin%) do generador.py %%X %densidad% > test%%X-%densidad%.txt
::con 30%
set densidad=30
for /l %%X in (%inicio%, %paso%, %fin%) do generador.py %%X %densidad% > test%%X-%densidad%.txt
::con 40%
set densidad=40
for /l %%X in (%inicio%, %paso%, %fin%) do generador.py %%X %densidad% > test%%X-%densidad%.txt
