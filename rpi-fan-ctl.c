/*
 *  SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 * 
 * A program to control a raspberry pi's CPU fan using PWM on FreeBSD. 
 *
 * Copyright (c) 2022, Joao Cabral <jcnc@dhis.org)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/sysctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <libgpio.h>

#define	SYSCTL_NAME_CPUTEMP	"hw.cpufreq.temperature"
#define	GPIO_DEFAULT_PIN	14
#define	CPUTEMP_INVALID		-999
#define DEFAULT_PWM_FREQ 25

int mib[4];
size_t miblen = 0;
size_t datalen = 0;
gpio_handle_t handle;
int verbose = 0;
int pwmmode = 0;
int pwmfreq = DEFAULT_PWM_FREQ;

int gpiopin = GPIO_DEFAULT_PIN;

/* Handle exits softly */
void sig_term_handler(int s) {
  if(handle>=0) { gpio_close(handle); }
  if (verbose) { printf("SIGTERM received, exiting ...\n"); }
  exit(0);
}

void usage(char *s) {

  fprintf(stderr, "\nUsage: \n\n");

  fprintf(stderr, "  [-g pwm-gpio-pin] is used to specify the GPIO pin in any config:\n\n");
  fprintf(stderr, "  [-v] can be used to enable verbose mode and can be used multiple times.\n\n");
  fprintf(stderr, "  [-d] can be used to launch in daemon (background) mode and detach.\n\n");
  fprintf(stderr, "  -h shows this usage instructions.\n\n");

  fprintf(stderr, "  To run fan at XXX percent (use 0 to turn OFF and 100):\n");
  fprintf(stderr, "  %s [-g pwm-gpio-pin] [-v] [-d] -p <percentage>\n\n",s);

  fprintf(stderr, "  To keep CPU temperature between DD - 5 and DD degrees celcius:\n");
  fprintf(stderr, "  %s [-g pwm-gpio-pin] [-v] [-d] -t <max-temp>\n\n",s);

  fprintf(stderr, "  To keep CPU temperature at DD degrees celcius using PWM:\n");
  fprintf(stderr, "  %s [-g pwm-gpio-pin] [-v] [-d] -w <target-temp>\n\n",s);

  fprintf(stderr, "  To print fan (pin) running state, 0 or 1:\n");
  fprintf(stderr, "  %s [-g pwm-gpio-pin] -s\n\n",s);

  fprintf(stderr, "  To print CPU temperature:\n");
  fprintf(stderr, "  %s [-g pwm-gpio-pin] -c\n\n",s);

  fprintf(stderr, "  For PWM modes (-p and -w) the default PWM frequency is 25 which corresponds\n");
  fprintf(stderr, "  to the number of times per second that the power is toggled and cycled\n");
  fprintf(stderr, "  through ON and OFF. This frequency can be set to any number between 1 and 50.\n");
  fprintf(stderr, "  For example, a frequency of 1 and percentage of 40, causes power to be on\n");
  fprintf(stderr, "  for 0.4 seconds and off for 0.6 seconds continuously.\n");
  fprintf(stderr, "  Settig the frequency to 10 causes power to be on for 0.04 and off for 0.06,\n");
  fprintf(stderr, "  i.e. 10 full cycles per second.\n\n");
  fprintf(stderr, "  %s [-g pwm-gpio-pin] [-v] [-d] [f 1-50] -p <percentage>\n\n",s);
  fprintf(stderr, "  %s [-g pwm-gpio-pin] [-v] [-d] -w [-f 1-50] <target-temp>\n\n",s);

  exit(1);
}


/* Initialise MIB by finding the correct vector index and data size 
 * for the hardware cpu temperature sysctl data. */
void initialize_mib() {
  memset(mib,0,4);
  size_t len=4;
  if(!sysctlnametomib(SYSCTL_NAME_CPUTEMP,mib,&len))  {
    if(!sysctl(mib,(u_int)len,NULL,&datalen,NULL,0)) {
      if(datalen<=sizeof(int)) {
	miblen=len;
        return;
      }
    }
  }
  fprintf(stderr,"Couldn't find MIB for sysctl entry \"%s\". Exiting ...\n",
                 SYSCTL_NAME_CPUTEMP);
  exit(2);
}

/* Returns the CPU temperature as returned from the MIB in int format in mili 
 * degrees, i.e. 1000 corresponds to 1 degree C and 1500 corresponds to 1.5C.
 *  On error returns CPUTEMP_INVALID as defined above. */
int get_cpu_temperature() {
  int t;
  if(miblen==0) {
    initialize_mib();
  }
  if(sysctl(mib,miblen,&t,&datalen,NULL,0)) {
    return(CPUTEMP_INVALID);
  } 
  return t;
}

/* set_pin_value() sets the pin to a value of 0 or 1 */
void set_pin_value(int pin, int value) {
  int v=1;
  if (!value) v=0;
  if(verbose>=2) {
    printf("PIN %d => %d\n", pin, v);
  }
  if (gpio_pin_set(handle, pin, v) < 0) {
      fprintf(stderr, "Unable to set the value of pin %d. Exiting ...\n", pin);
      exit(5);
  }
}

/* Returns the GPIO pin value */
int get_pin_value(int pin) {
  int value = gpio_pin_get(handle, pin);
  if (value < 0) {
     fprintf(stderr, "Unable to get the value of pin %d. Exiting ...\n", pin);
      exit(5);
  }
  return value; 
}

/* Initializes the GPIO handle and pin */
void init_gpio(void) {
  gpio_config_t pin;
  handle = gpio_open(0);
  if(handle == GPIO_INVALID_HANDLE) {
    fprintf(stderr, "Unable to open GPIO controller 0. Exiting ...\n");
    exit(3);
  }

  /* Set pin as output if not already */
  pin.g_pin = gpiopin;
  pin.g_flags = GPIO_PIN_OUTPUT;
  if (gpio_pin_set_flags(handle, &pin) < 0) {
    fprintf(stderr, "Unable to set pin %d as output. Exiting ...\n", gpiopin);
    gpio_close(handle);
    exit(4);
  }
  return;
}
 
/* Runs fan at percentage or maxtemp, one of the two but not both. 
 * Specifying a percentage of 0 turns off the fan and returns straight
 * away. Speciying 100 turns the fan on and returns. Other values 
 * cause an infinite loop that adjusts the fan speed as needed. */
void run_fan_at(percentage, maxtemp) {

  if(percentage < 0 && !pwmmode) { /* Keep temperature below maxtemp  */

    float maxtk = (float)(maxtemp);
    float mintk = maxtk - 5.0;
    if (verbose) {
      printf("Attempting to keep CPU temperature below %.3f, currently it is %.3f\n", maxtk, (float)get_cpu_temperature() / 1000.0);
    }
    while(1) {
      int ti = get_cpu_temperature();
      float t = (float)ti; t /= 1000;
      int v = get_pin_value(gpiopin);
      if (t > maxtk && !v) {
	if(verbose) {
	  printf("CPU temperature %.3f is above %.3f, turning fan ON\n",t,maxtk);
	}
        set_pin_value(gpiopin,1);
      } else if (t < mintk && v) {
	if(verbose) {
	  printf("CPU temperature %.3f is below %.3f, turning fan OFF\n",t,mintk);
	}
        set_pin_value(gpiopin,0);
      } 
      sleep(1);
    }

  } else if(percentage < 0 && pwmmode) { /* Keep temperature at target using PWM  */
    if (verbose) {
      printf("Attempting to keep CPU temperature at %d C or below using PWM\n",maxtemp);
    }
    float ttemp = (float)(maxtemp);
    int perc = 50;
    int useccounter = 0;
    int sleepmode = 0;
    while(1) {

      if(!sleepmode) {
	if (perc == 100) {
	  if(!get_pin_value(gpiopin)) {
            set_pin_value(gpiopin, 1);
	  }
	  useccounter = 1000000;
	} else {
          useconds_t d1 = perc * (10000 / pwmfreq);
          useconds_t d2 = (100 - perc ) * (10000 / pwmfreq);
          set_pin_value(gpiopin, 1);
          usleep(d1);
          useccounter += d1;
          set_pin_value(gpiopin, 0);
          usleep(d2);
          useccounter += d2;
	}
      } else {
	sleep(1);
	useccounter = 1000000;
      }

      if (useccounter >= 1000000) {

	int ti = get_cpu_temperature();
      	float t = (float)ti; t /= 1000;

	if (t < ttemp - 5 && !sleepmode) {
	  if (verbose) {
	    printf("Temperature at %.3f is too low, below %.3f, entering sleep mode.\n", t, ttemp-5);
	  }
	  set_pin_value(gpiopin,0);
	  sleepmode = 1;
	} else if (t >= ttemp && sleepmode) {
	  if (verbose) {
	    printf("Temperature at %.3f is near target of %.3f, waking up from sleep mode.\n",t,ttemp);
          }
	  sleepmode = 0;
	} else if(!sleepmode) {
	  float rt = round(t);
	  if(rt > ttemp && perc < 100) {
	    perc += 1;
	    if(verbose) {
	      printf("Temperature at %.3f for target %.3f, adjusting fan to %d%c.\n",t,ttemp,perc,'%'); 
	    }
	  } else if (rt < ttemp && perc > 0) {
	    perc -= 1;
	    if(verbose) {
	      printf("Temperature at %.3f for target %.3f, adjusting fan to %d%c.\n",t,ttemp,perc,'%'); 
	    }
	  }
        }
	
	useccounter=0;
      }
     
    } /* End of while 1 */
    
  } else if(percentage == 0) { /* Simply turn off */
    if(verbose) {
      printf("Turning fan OFF (if ON)\n");
    }
    set_pin_value(gpiopin, 0);

  } else if (percentage == 100) { /* Simply turn on */
    if(verbose) {
      printf("Turning fan ON (if OFF)\n");
    }
    set_pin_value(gpiopin, 1);

  /* Run at a fixed percentage */
  } else if (percentage > 0 && percentage < 100) {
    if (verbose) {
      printf("Attempting to run fan at %d%c ...\n",percentage,'%');
    }
    useconds_t d1 = percentage * (10000 / pwmfreq);
    useconds_t d2 = (100 - percentage ) * (10000 / pwmfreq);
    while(1) {
      set_pin_value(gpiopin, 1);
      usleep(d1);
      set_pin_value(gpiopin, 0);
      usleep(d2);
    }
  }
}

int main(int argc,char *argv[]) {

  int ch;
  extern char *optarg;
  int percentage  = -1;
  int maxtemp = -100;
  int statemode = 0;
  int tempmode = 0;
  int noopts = 0;
  int daemonmode = 0;
  while ((ch = getopt(argc, argv, "hf:dg:p:t:w:svc")) != -1) {
    switch(ch) {
      case 'v': 
		verbose += 1;
		break;
      case 'd': 
		daemonmode += 1;
		break;
      case 'f': 
		pwmfreq = atoi(optarg);
	        if (pwmfreq < 1 || pwmfreq > 50) usage(argv[0]);
		break;
      case 'h': 
		usage(argv[0]); 
		break;
      case 's': 
		statemode = 1;
		noopts++;
		break;
      case 'c': 
		tempmode = 1;
		noopts++;
		break;
      case 'g': 
		gpiopin = atoi(optarg); 
		break;
      case 'p': 
		if (percentage != -1) usage(argv[0]);
		percentage = atoi(optarg); 
		noopts++;
		break;
      case 't': 
		if (maxtemp != -100) usage(argv[0]);
		maxtemp = atoi(optarg); 
		noopts++;
		break;
      case 'w': 
		if (maxtemp != -100) usage(argv[0]);
		maxtemp = atoi(optarg); 
		pwmmode = 1;
		noopts++;
		break;
      default: 	break;
    }
  }

  /* Just some sanity checks */
  if(noopts!=1) usage(argv[0]);
  if(gpiopin < 0 || gpiopin > 53) usage(argv[0]);
  if(percentage < -1 || percentage > 100) usage(argv[0]);

  init_gpio();
  
  if (daemonmode) {
    if (verbose) {
      printf("Running in daemon / background mode, detaching...\n");
    }
    verbose = 0;
    if(fork()) exit(0);
    close(0);
    close(1);
    close(2);
  }

  signal(SIGTERM,sig_term_handler);
  
  if(statemode) {
    printf("%d\n",get_pin_value(gpiopin));
  } else if(tempmode) {
    int t = get_cpu_temperature();
    printf("%d.%d\n",t / 1000, t % 1000);
  } else {
    run_fan_at(percentage, maxtemp);
  }
  gpio_close(handle);
  return(0);
}
