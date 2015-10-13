Nsc=12;
Ns=14;
Nrb=25;
Tsub=0.001;
Tframe=0.01;
Tcp=1024/(15000*2048)*20;
F=(Tframe-Tcp)/Tframe*(Nsc*Ns/2-4)/(Nsc*Ns/2);
B=Nsc*Ns*Nrb/Tsub/1000000;
SNR=1:1:25;
C=F*B*log2(1+SNR);
plot(SNR,C,'r-^');