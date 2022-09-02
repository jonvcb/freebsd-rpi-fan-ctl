# rpi-fan-ctl
## Raspberry Pi PWM Fan Control Program

### Version History: 
  
2002.09.02 - Version 1.0 - Initial version

### Summary: 

I made this simple tool in C to control the speed of the CPU fan of my 
raspbery pi model 4. It should run on other models as well providing they 
have the GPIO interfaces. Made / tested on FreeBSD 13.1 but will likely 
compile on previous versions.  

It allows controlling a PWM fan in one of two modes; by setting a percentage
of power, or by setting a desired temperature. 

### To build: 

Just type:

# make

Or use:

# cc rpi-fan-ctl.c -lgpio -o rpi-fan-ctl

###: How to use: 

This tool is to control a fan using PWM. You need a fan with 3 pins, 2 for 
power and 1 for PWM control. If your fan only has 2 wires (power) 
you need an extra circuit to create a third one; look on the internet for
how to build such circuits, there is plenty on that. 

Do not try to connect your 2 wire fan to a GPIO pin as the pi doesn't supply
enough power to run a fan on these ports and this will likely damage it. 

####: GPIO pin

By default this tool is configured to use the PIO pin 14 (connector pin 8)
next to ground (connector pin 6) and to +5V (connector pin 4).

If you want to use a different GPI port you can, just specify it with the 
-g option and a pin number from 0 to 53. 

This tool is pre-configured to connect to GPI controller 0 i.e. /dev/gpioc0

e.g. # rpi-fan-ctl -g 1 ....

####: Verbose mode

Use -v to enable verbosity (in foreground mode only). Use -v -v to also log
pin transitions. 

####: Reading the fan state:

Use the -s option to display the current fan state; 1 means it's running and 0 means it's not running (or powered). 

e.g. # rpi-fan-ctl -s 
 
1

####: Reading the current CPU temperature: 

Use the -c option to display the temperature: 

e.g. # rpi-fan-ctl -c

55.955


