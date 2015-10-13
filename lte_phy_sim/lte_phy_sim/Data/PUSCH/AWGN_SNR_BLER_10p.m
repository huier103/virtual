set(0, 'DefaultLineMarkerSize', 10);
set(0, 'Defaultaxesfontsize', 14);
set(0, 'DefaultLineLineWidth', 2);

%%
%root_path = 'awgn_1000_2012_8_11';
%root_path ='AWGN_10000_2012_8_11';
root_path='AWGN_2012_8_14';
mcs_small=0;
mcs_big=24;
frames=1000;
plot_style = {'b-*';'r-*';'g-*';'y-*';'k-*';...
    'b-o';'r-o';'g-o';'y-o';'k-o';...
    'b-s';'r-s';'g-s';'y-s';'k-s';...
    'b-d';'r-d';'g-d';'y-d';'k-d';...
    'b-x';'r-x';'g-x';'y-x';'k-x';...
    'b-+';'r-+';'g-+';'y-+';'k-+'};
lengtt ={'mcs0(QPSK)','mcs1(QPSK)','mcs2(QPSK)','mcs3(QPSK)','mcs4(QPSK)',...
       'mcs5(QPSK)','mcs6(QPSK)','mcs7(QPSK)','mcs8(QPSK)','mcs9(QPSK)',...
       'mcs10(16QAM)','mcs11(16QAM)','mcs12(16QAM)','mcs13(16QAM)',...
       'mcs14(16QAM)','mcs15(16QAM)','mcs16(16QAM)','mcs17(64QAM)',...
       'mcs18(64QAM)','mcs19(64QAM)','mcs20(64QAM)','mcs21(64QAM)',...
       'mcs22(64QAM)','mcs23(64QAM)','mcs24(64QAM)'};
 mcs_frames(1:1:25)=1000;
%  mcs_frames(18)=10000;
%  mcs_frames(19)=1000;
 %mcs_frames(20)
h_fig=figure(1);
hold off
legend_str = {};
i=1;
for mcs=mcs_small:mcs_big
    file = fullfile(root_path,sprintf('ber_bler_mcs%d_chan16_nframes%d.csv',mcs,mcs_frames(mcs+1)));
    data = dlmread(file,',',1,0);
    snr = data(:,1);
    bler = data(:,3)./data(:,4); % round 1
    %snr_tmp=snr(bler<=1&bler>=1e-2);
    %bler_tmp=bler(bler<=1&bler>=1e-2);
    throughput_withoutharq=data(:,15);
    throughput=data(:,16);
    figure(1)
    %semilogy(snr_tmp,bler_tmp,plot_style{mcs+1});
    semilogy(snr,bler(:,1),plot_style{mcs+1});
    hold on
    legend_str{i} = sprintf(lengtt{mcs+1});
    i=i+1;
end

%%
figure(1);
h_leg = legend(legend_str,'location','eastoutside');
set(h_leg,'FontSize',10);
title('AWGN Performance Curves for LTE 5 MhZ');
ylabel 'BLER'
xlabel 'SNR'
ylim([0.001 1])
grid on
file2=fullfile(root_path,sprintf('siso_awgn.eps'));
saveas(h_fig,file2,'epsc2');
h_fig2=figure(2);
i=1;
for mcs=mcs_small:mcs_big
    file = fullfile(root_path,sprintf('ber_bler_mcs%d_chan16_nframes%d.csv',mcs,frames));
    data = dlmread(file,',',1,0);
    snr = data(:,1);
    throughput_withoutharq=data(:,15);
    throughput=data(:,16);
    figure(2);
    plot(snr,throughput,plot_style{mcs+1});
    hold on
    legend_str{i} = sprintf(lengtt{mcs+1});
    i=i+1;
end
figure(2);
grid on
h_leg2 = legend(legend_str,'location','eastoutside');
set(h_leg2,'FontSize',10);
title('AWGN Performance Curves for LTE 5 MhZ');
ylabel 'Throughput(Mbps)'
xlabel 'SNR'
file3=fullfile(root_path,sprintf('Throughput_siso_awgn.eps'));
saveas(h_fig2,file3,'epsc2');

