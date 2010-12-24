x = [15:5:50]

sol = [8, 10, 0, 14, 17, 19, 22, 24 ]
inf = [8, 10, 0, 14, 17, 19, 22, 24 ]
sup = [8, 10, 0, 15, 18, 20, 22, 24 ]

plot(x, sol)
hold all

plot(x, inf)
hold all

plot(x, sup)
hold all

legend('Soluciones', 'Cota Inferior', 'Cota Superior')