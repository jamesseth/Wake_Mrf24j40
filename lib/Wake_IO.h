/*
 * Wake_IO.h
 *
 *  Created on: 06 Jul 2014
 *      Author: ampm
 */

#ifndef WAKE_IO_H_
#define WAKE_IO_H_

#include <cstdlib>
#include <cstdio>
#include <ctype.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <iostream>

#define __WHITE   "\x1b[00m"
#define __RED     "\x1b[31m"
#define __GREEN   "\x1b[32m"
#define __YELLOW  "\x1b[33m"
#define __BLUE    "\x1b[34m"
#define __MAGENTA "\x1b[35m"
#define __CYAN    "\x1b[36m"
#define __RESET   "\x1b[0m"





#define	INPUT	0
#define	OUTPUT	1
#define	PULLOFF			 0
#define	PULLDOWN		 1
#define	PULLUP			 2

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 	64

#define	BCM_PASSWORD		0x5A000000

/*
 * 	Function selection Values BCM2835
 */

#define	FSEL_INPT		0b000
#define	FSEL_OUTP		0b001
#define	FSEL_ALT0		0b100
#define	FSEL_ALT0		0b100
#define	FSEL_ALT1		0b101
#define	FSEL_ALT2		0b110
#define	FSEL_ALT3		0b111
#define	FSEL_ALT4		0b011
#define	FSEL_ALT5		0b010

/*
 *  ARM register address for BCM2835
 */

#define BCM2708_PERI_BASE	0x20000000
#define GPIO_PADS			(BCM2708_PERI_BASE + 0x00100000)
#define CLOCK_BASE			(BCM2708_PERI_BASE + 0x00101000)
#define GPIO_BASE			(BCM2708_PERI_BASE + 0x00200000)
#define GPIO_TIMER			(BCM2708_PERI_BASE + 0x0000B000)
#define GPIO_PWM			(BCM2708_PERI_BASE + 0x0020C000)

// GPPUD:
//	GPIO Pin pull up/down register

#define	GPPUD	37



#define	PAGE_SIZE		(4*1024)
#define	BLOCK_SIZE		(4*1024)

#define RISING	"rising"
#define FALLING	"falling"
#define BOTH	"both"


namespace wake{

static int sysFds [64] ;





class Wake_IO {
private:
	uint32_t* gpio;
	uint32_t* pads;

public:
	Wake_IO();
	~Wake_IO();
	void digitalWrite (int pin, int value);

	void setPadDrive (int group, int value);

	bool digitalRead (int pin);

	void setPullUps(int pin, int pud);

	void pinMode (int pin, const char* mode);

	int exportGPIO(unsigned int pin);

	int unexportGPIO(unsigned int pin);

	int setEdge(unsigned int pin, const char* edge);

	int setupFromFile(const char* fName);
};
}
#endif /* WAKE_IO_H_ */
