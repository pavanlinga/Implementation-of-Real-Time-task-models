CC = i586-poky-linux-gcc
ARCH = x86
CROSS_COMPILE = i586-poky-linux-


IOT_HOME = /opt/iot-devkit/1.7.2/sysroots


PATH := $(PATH):$(IOT_HOME)/x86_64-pokysdk-linux/usr/bin/i586-poky-linux

SROOT=$(IOT_HOME)/i586-poky-linux


all:
	i586-poky-linux-gcc -Wall -I /opt/iot-devkit/1.7.2/sysroots/i586-poky-linux/usr/include -L /opt/iot-devkit/1.7.2/sysroots/i586-poky-linux/usr/lib -pthread -o run_me main_fread.c --sysroot=$(SROOT)

clean:
	rm -f *.o

host:
	gcc -g -Wall -pthread -o run_me main_fread.c
