x = [1:7]
t1 = [8540, 1424, 10, 40, 9675, 13, 19]
t2 = [10057, 999, 10, 46, 7083, 14, 20]
t3 = [9, 305, 0, 20, 3841, 0, 95]
t4 = [11, 513, 0, 34, 4810, 27, 10]
t5 = [321, 0, 0, 0, 0, 0, 0]

plot(x, t1)
legend('tipo1')
hold all
plot(x, t2)
legend('tipo2')
hold all
plot(x, t3)
legend('tipo3')
hold all
plot(x, t4)
legend('tipo4')
hold all
plot(x, t5)
legend('tipo5')
hold all
legend('tipo1','tipo2','tipo3','tipo4','tipo5')