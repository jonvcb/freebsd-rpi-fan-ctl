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

Feel free to use and adapt to your own needs.

### To build: 

Just type:

# make

Or use:

# cc rpi-fan-ctl.c -lgpio -o rpi-fan-ctl

### How to use: 

This tool is to control a fan using PWM. You need a fan with 3 pins, 2 for 
power and 1 for PWM control. If your fan only has 2 wires (power) 
you need an extra circuit to create a third one; look on the internet for
how to build such circuits, there is plenty on that. 

Do not try to connect your 2 wire fan to a GPIO pin as the pi doesn't supply
enough power to run a fan on these ports and this will likely damage it. 

#### GPIO pin

By default this tool is configured to use the PIO pin 14 (connector pin 8)
next to ground (connector pin 6) and to +5V (connector pin 4).

If you want to use a different GPI port you can, just specify it with the 
-g option and a pin number from 0 to 53. 

This tool is pre-configured to connect to GPI controller 0 i.e. /dev/gpioc0

e.g. $ rpi-fan-ctl -g 1 ....

#### Verbose mode

Use -v to enable verbosity (in foreground mode only). Use -v -v to also log
pin transitions. 

####: Reading the fan state:

Use the -s option to display the current fan state; 1 means it's running and 0 means it's not running (or powered). 

e.g. $ rpi-fan-ctl -s 
 
1

#### Reading the current CPU temperature: 

Use the -c option to display the temperature: 

e.g. $ rpi-fan-ctl -c

55.955

#### Running the fan so that the CPU temperature stays below a threshold: 

To keep the cpu temperature below a certain temperature, use the -t option. For example to keep the temp below 60 degrees celcius you can type: 

$ rpi-fan-ctl -t 60

This mode doesn't use PWM. Instead, it simply turns the fan to full power 
when the temperature goes above the threshold. It then turns the fan off when 
the temperature drops below the threshold minus 5 degrees, i.e. 55 in the above
example.

This mode enters a loop and the tool runs forever until it is interrupted 
with a SIGINT (or CTRL-C). 

#### Running the fan at a certain power percentage using PWM:

You can run the fan at a certain power percentage (of the supplied 5V and its 
required current) by specifiying the -p option. This causes PWM to control the 
fan so that it only runs (is powered) for X% of the time. 

e.g. to run the fan at 50%: 

$ rpi-fan-ctl -p 50

To run at 70% with verbosity: 

$ rpi-fan-ctl -v -p 70

This mode enters a loop and the tool runs forever until it is interrupted 
with a SIGINT (or CTRL-C). 

This mode can also be used to just turn the fan OFF or ON in which case
it doesn't run in a loop. To do so just pass the percentages 0 for always OFF
or 100 

#### Running the fan in order to keep the CPU at a certain temperature: 

This is the most complex but nicest mode; it uses PWM to keep the CPU around a 
certain temperature at all times, providing it is possible. 

This mode also enters a loop and the tool runs forever until it is interrupted 
with a SIGINT (or CTRL-C). 

By specifying a desired temperature with the -w option, in this mode the tool
starts off by running the fan at 50% and then keeps adjusting the fan speed 
using PWM to up or down in order to achieve the desired temperature. It then 
tries to stay there and keeps adjusting it as temperature changes are detected. 

Naturally this is only possible if the temperature can be achieved. If you 
specify a temperature that is too low to achieve, the fan will stay always ON. 
If you specify a temperature too high, the fan will converge to always OFF as 
the actual temperature is lower. 

You should try with this value to see what best attains the results you need
for the level of noise that is acceptable. A low value is desired as long as 
the temperature can be achieved. A high value will cause the fan to spin faster
and thus make more noise. 

To run CPU at 60 degrees and control on GPIO port 12 for example, use: 

$ rpi-fan-ctl -g 12 -w 60 

#### PWM frequency:

For most cases you shouldn't need to change this; the default frequency is 
25Hz which means that when using PWM, per second the fan will transition from 
OFF to ON and then to OFF 25 times per second. 

The duration of the OFF and ON states depends on the percentage being achieved
as well as the frequency. So if you run the fan at 40% with a frequency of 10, 
the fan will be ON for 0.04 seconds and OFF for 0.06 seconds, 10 times per
second. If you run with a frequency of 1, the fan will be ON for 0.4 seconds 
and OFF for 0.6 seconds. 

You can change the frequency with the -f option to any value between 1 and 50. 
For optimal accuracy 10000 should be a multiple of that value. This option
only has effect in PWM modes, i.e. if used with conjunction with the -p or 
-w option. 

For example: 

$ rpi-fan-ctl -w 60 -f 10 

Runs at 10Hz attempting to achieve 60 degrees of CPU temperature. 

#### Running in background mode:

The last option is the background mode; when used it cases the tool to
detach and to run forever in background. You can then terminate the process
by sending it a KILL -SIGINT if wished. 

It is only available for modes that run in loop (-t, -p and -w) and is 
enabled with the -d switch as in the following example: 

$ rpi-fan-ctl -w 60 -d 

---

That's it for now; anything feel free to contact me. 

Enjoy the new silence :)))

