CC=cc
CP=cp

all: rpi-fan-ctl

install: rpi-fan-ctl
	$(CP) rpi-fan-ctl /usr/local/sbin

rpi-fan-ctl: rpi-fan-ctl.c
	$(CC) rpi-fan-ctl.c -lgpio -o rpi-fan-ctl
