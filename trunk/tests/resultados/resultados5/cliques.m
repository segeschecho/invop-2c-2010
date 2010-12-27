x = [1:1:7]
t3 = [0,128,2,51,876,6,76]
t4 = [0,156,2,32,738,32,4]

plot(x, t3)
legend('tipo3')
hold all
plot(x, t4)
legend('tipo4')
hold all
legend('tipo3','tipo4')