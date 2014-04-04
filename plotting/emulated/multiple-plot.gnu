SEGSIZE=1460
reset
#set termoption dashed

######## CWND and rx rate ########

set output "cwnd_rx_rate-".x1."-".x2.".svg"
set terminal svg dashed size 700,700 fsize 14
set multiplot layout 2,1
set tmargin 3
set bmargin 3
set lmargin 10

set title "Congestion window (CWnd) of TCP CUBIC"
set key inside top right box
set xlabel "Time (s)"
#set xtic 10
set ylabel "CWnd (segment)"
#set y2label "Delay (ms)"
#set y2tics nomirror tc lt 2
#set yrange [0:200]
#set y2range [0:800]
set xrange [x1:x2]
set x2range [x1:x2]
#set y2tics nomirror tc lt 6
#set log y2

set arrow from 60,0 to 60,700 nohead lt 2 lc -1 lw 2




plot "cubic.dat" using 3:10 title "CWnd" with line lt 1 lc 1 lw 3,\
"cubic.dat"  using 3:14 title "Ssthreshold" with line lt 2 lc 2 lw 3
#"application-start.dat" using ($1+$5/1000000000):(50) title "TCP resumes" pt 1 lt 12
#"tcp_send-.dat" using 1:12 title "TCP delay" with lines lt 4 lw 2 axis x1y2
#"cubic.dat" using 3:($54*1000) title "rto" with lines lt 5 lw 2 axis x1y2


 
#set title "TCP goodput"
unset title 
set key inside bottom right 
#set xtic 10
set xrange [x1:x2]
set x2range [x1:x2]
#set y2label "a"
#set yrange [0:2000]
#set xrange [0:100]
#set yrange [0:20000]
#unset yrange
unset y2range
set ylabel "Goodput (kbps)"
unset y2tics
#set y2tics nomirror tc lt 6

set arrow from 60,0 to 60,700 nohead lt 2 lc -1 lw 2

plot "tcp_send.dat" using 1:3 title "TCP goodput" with lines lt 6 lw 3


unset multiplot


######

######## Timeout and delay ########
reset
set title "End-to-end delay and TCP RTO estimated value"
set key inside top right
set xlabel "Time (s)"
#set xtic 20
set ylabel "Delay (ms)"
#set y2label "Delay (ms)"
set output "delay-".x1."-".x2.".svg"
#set y2tics nomirror tc lt 2
#set y2range [0:800]
#set yrange [0:1200]
set xrange [x1:x2]
#set y2tics nomirror tc lt 2

set terminal svg

set arrow from 60,0 to 60,700 nohead lt 2 lc -1 lw 2

plot "tcp_send.dat" using 1:12 title "packet delay" with line lt 1 lc 8 lw 3
#"retrans--truth.dat" using 1:(0) title "Timeout" pt 1,\
