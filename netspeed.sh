#!/bin/bash

INTERVAL="1"  # update interval in seconds

if [ -z "$1" ]; then
        echo
        echo usage: $0 [network-interface]
        echo
        echo e.g. $0 eth0
        echo
        exit
fi

IF=$1

while true
do
        R1=`cat /sys/class/net/$1/statistics/rx_bytes`
        T1=`cat /sys/class/net/$1/statistics/tx_bytes`
	R1_rx_length_errors=`cat /sys/class/net/$1/statistics/rx_length_errors`
	R1_rx_missed_errors=`cat /sys/class/net/$1/statistics/rx_missed_errors`
	R1_rx_over_errors=`cat /sys/class/net/$1/statistics/rx_over_errors`
	R1_rx_crc_errors=`cat /sys/class/net/$1/statistics/rx_crc_errors`
	R1_rx_frame_errors=`cat /sys/class/net/$1/statistics/rx_frame_errors`
	R1_rx_fifo_errors=`cat /sys/class/net/$1/statistics/rx_fifo_errors`
	R1_rx_errors=`cat /sys/class/net/$1/statistics/rx_errors`
	echo "intrrupts_start:"
	cat /proc/interrupts  | grep  "ice-enp1s0f1-TxRx-11"
        #sleep $INTERVAL
	sleep 1
	echo "intrrupts_end:"
	cat /proc/interrupts  | grep  "ice-enp1s0f1-TxRx-11"
        R2=`cat /sys/class/net/$1/statistics/rx_bytes`
        T2=`cat /sys/class/net/$1/statistics/tx_bytes`
	R2_rx_length_errors=`cat /sys/class/net/$1/statistics/rx_length_errors`
	R2_rx_missed_errors=`cat /sys/class/net/$1/statistics/rx_missed_errors`
	R2_rx_over_errors=`cat /sys/class/net/$1/statistics/rx_over_errors`
	R2_rx_crc_errors=`cat /sys/class/net/$1/statistics/rx_crc_errors`
	R2_rx_frame_errors=`cat /sys/class/net/$1/statistics/rx_frame_errors`
	R2_rx_fifo_errors=`cat /sys/class/net/$1/statistics/rx_fifo_errors`
	R2_rx_errors=`cat /sys/class/net/$1/statistics/rx_errors`	

        TBPS=`expr $T2 - $T1`
        RBPS=`expr $R2 - $R1`
        TKBPS=`expr $TBPS / 1 `
        RKBPS=`expr $RBPS / 1 `
	rx_length_errors_BPS=`expr $R2_rx_length_errors - $R1_rx_length_errors`
	rx_missed_errors_BPS=`expr $R2_rx_missed_errors - $R1_rx_missed_errors`
	rx_over_errors_BPS=`expr $R2_rx_over_errors - $R1_rx_over_errors`
	rx_crc_errors_BPS=`expr $R2_rx_crc_errors - $R1_rx_crc_errors`
	rx_frame_errors_BPS=`expr $R2_rx_frame_errors - $R1_rx_frame_errors`
	rx_fifo_errors_BPS=`expr $R2_rx_fifo_errors - $R1_rx_fifo_errors`
	rx_errors_BPS=`expr $R2_rx_errors - $R1_rx_errors`
        echo "TX $1: $TKBPS bytes/s RX $1: $RKBPS bytes/s"
        echo " rx_length_errors_BPS:$rx_length_errors_BPS bytes/s"
	echo " rx_missed_errors_BPS:$rx_missed_errors_BPS bytes/s"
	echo "   rx_over_errors_BPS:$rx_over_errors_BPS bytes/s"
        echo "    rx_crc_errors_BPS:$rx_crc_errors_BPS bytes/s"
	echo "  rx_frame_errors_BPS:$rx_frame_errors_BPS bytes/s"
	echo "   rx_fifo_errors_BPS:$rx_fifo_errors_BPS bytes/s"
	echo "        rx_errors_BPS:$rx_errors_BPS bytes/s"
done
