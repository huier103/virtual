set(0, 'DefaultLineMarkerSize', 10);
set(0, 'Defaultaxesfontsize', 14);
set(0, 'DefaultLineLineWidth', 2);

%%
root_path = '';

plot_style = {'b-*';'r-*';'g-*';'y-*';'k-*';...
    'b-o';'r-o';'g-o';'y-o';'k-o';...
    'b-s';'r-s';'g-s';'y-s';'k-s';...
    'b-d';'r-d';'g-d';'y-d';'k-d';...
    'b-x';'r-x';'g-x';'y-x';'k-x';...
    'b-+';'r-+';'g-+';'y-+';'k-+'};

h_fig=figure(1)
%hold off
legend_str = {};
a=[16 6];
qq={'AWGN' 'EVA' 'EPA'};
for t=0:1:0
i=1;
file = fullfile(root_path,sprintf('bler_chan%d_nframes10000.csv',a(t+1)));
data = dlmread(file,',',1,0);
snr = data(:,1);
bler = data(:,2);
snr_tmp = snr(bler<1);
bler_tmp=bler(bler<1);
semilogy(snr_tmp,bler_tmp,plot_style{t+1});
hold on
legend_str{i} = sprintf('%s',qq{t*5+1});
i=i+1;
end
grid on

%%
h_leg = legend(legend_str,3);
set(h_leg,'FontSize',10);
title('PBCH SISO Performance Curves for LTE 5 MhZ,10000 frames');
ylabel 'BLER'
xlabel 'SNR'
ylim([0.001 1])
grid on


