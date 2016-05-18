/*
 * Wake_IO.cpp
 *
 *  Created on: 06 Jul 2014
 *      Author: ampm
 */

#ifndef _WAKE_IO_CPP
#define _WAKE_IO_CPP
#include "Wake_IO.h"
namespace wake{
Wake_IO::Wake_IO() {
	/*	Initialises the system into GPIO Pin mode and uses the
	 *	memory mapped hardware directly.
	 */

	// Check if User has root privileges
	if (geteuid () != 0){
		fprintf (stderr, "Program need to be executed as root due to direct memory access required\n") ;
		exit (EXIT_FAILURE) ;
	}

	// Open the master /dev/memory device
	int fd;
	if ((fd = open ("/dev/mem", O_RDWR | O_SYNC) ) < 0){
		int serr = errno ;
		fprintf (stderr, "Unable to open /dev/mem: %s\n", strerror (errno)) ;
		exit(EXIT_FAILURE) ;
	}

	// Setting up GPIO memory map:
	gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, GPIO_BASE) ;
	if ((int32_t)gpio == -1){
		int serr = errno ;
		fprintf (stderr, "GPIO memory map: %s\n", strerror (errno)) ;
		exit(EXIT_FAILURE) ;
	}

	//Setting up PAD memory map:
	pads = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, GPIO_PADS) ;
	if ((int32_t)pads == -1){
		int serr = errno;
		fprintf (stderr, "PADS memory map failed: %s\n", strerror (errno)) ;
		errno = serr;
		exit(EXIT_FAILURE);
	}


}

Wake_IO::~Wake_IO() {
	// TODO Auto-generated destructor stub
}

void Wake_IO::digitalWrite (int pin, int value){	// digitalWriteWPi()
	int fd;
	char buf[MAX_BUF];
	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", pin);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		fprintf (stderr, "digitalWrite : Error unable set value\n") ;
	}
	if(value == 1){
		write(fd, "1", 1);
	}
	else if(value == 0){
		write(fd, "0", 1);
	}
	close(fd);
}

void Wake_IO::setPadDrive (int group, int value){
	uint32_t wrVal ;
	if ((group < 0) || (group > 2)){
		return ;
	}
	wrVal = BCM_PASSWORD | 0x18 | (value & 7) ;
	*(pads + group + 11) = wrVal ;
}

bool Wake_IO::digitalRead (int pin){
	if(pin < 32){
		if((*(gpio + 13) & (1 << (pin & 31))) != 0){
			return false;
		}else{
			return true;
		}
	}else{
		if((*(gpio + 14) & (1 << (pin & 31))) != 0){
			return false;
		}else{
			return true;
		}
	}
}

void Wake_IO::setPullUps(int pin, int pud){
	uint8_t pudCLK = 38;
	if(pin >31){
		pudCLK = 39;
	}

	pud &=  3 ;
	*(gpio + GPPUD) = pud;
	usleep(5);
	*(gpio + pudCLK) = 1 << (pin & 31);
	usleep(5);
	*(gpio + GPPUD) = 0 ;
	usleep(5);
	*(gpio + pudCLK) = 0 ;
	usleep(5);
}

void Wake_IO::pinMode (int pin, const char* mode){
		int fd;
		char buf[MAX_BUF];
		snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/direction", pin);
		fd = open(buf, O_WRONLY);
		if (fd < 0) {
			fprintf (stderr, "pinMode : Error unable set direction\n") ;

		}
		write(fd, mode, strlen(mode) + 1);
		close(fd);


}

int Wake_IO::exportGPIO(unsigned int pin){
	char buf[MAX_BUF];
	int fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if (fd < 0){
		fprintf (stderr, "Export GPIO: Error unable export\n") ;
		return fd;
	}
	int len = snprintf(buf, sizeof(buf), "%d", pin);
	write(fd, buf, len);
	close(fd);
	return 0;
}

int Wake_IO::unexportGPIO(unsigned int pin){
	int fd, len;
	char buf[MAX_BUF];
	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0) {
		fprintf (stderr, "Unexport GPIO: Error unable unexport\n") ;
		return fd;
	}
	len = snprintf(buf, sizeof(buf), "%d", pin);
	write(fd, buf, len);
	close(fd);
	return 0;
}

int Wake_IO::setEdge(unsigned int pin, const char* edge){

	//exportGPIO(pin);
	int fd;
	char buf[MAX_BUF];
	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", pin);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		fprintf (stderr, "Set Edge : Error unable set Edge\n") ;
		return fd;
	}
	write(fd, edge, strlen(edge) + 1);
	close(fd);
	return 0;
}

int Wake_IO::setupFromFile(const char* fName){
	FILE * fp;
	char * line = NULL;
	char * strPtr = NULL;
	size_t len = 0;
	ssize_t read;
	unsigned int prevPin=999;
	fp = fopen(fName, "r");
	if (fp == NULL)
		exit(EXIT_FAILURE);

	while ((read = getline(&line, &len, fp)) != -1) {
		//  printf("Retrieved line of length %zu :\n", read);
		//  printf("%s", line);
		strPtr = strstr(line, "GPIO");
		if( strPtr != NULL     &&   *(strPtr + 4) >= 0x30 && *(strPtr + 4) <= 0x39 && *(strPtr + 5) >= 0x30 && *(strPtr + 5) <= 0x39){
			char pinNumStr[3] = { *(strPtr + 4) , *(strPtr + 5), '\0'};
			unsigned int pinNum = atoi(pinNumStr) & 63;
			if(prevPin != pinNum){
				printf( "\n\e[0;44m\t\t\tGPIO %02d \t\t\t\e[0m\n",pinNum);
				prevPin = pinNum;
			}

			char gpiofName [64];
			sprintf(gpiofName, "/sys/class/gpio/gpio%d/value", pinNum) ;
			if( access( gpiofName, F_OK ) == -1 ) {
				exportGPIO(pinNum);
				printf( __CYAN "Exporting GPIO %d\n",pinNum);
			}

			strPtr = strstr(line, "DIR");
			if(strPtr != NULL){
				printf( "\e[0;47m");
				printf( __GREEN"Setting Direction\t\t");
				if(strstr(line, "input") != NULL){
					pinMode(pinNum,"in");
					printf("input");
				}
				else if(strstr(line, "output") != NULL){
					pinMode(pinNum,"out");
					printf("output");
				}
				printf( "\t\t\t" __RESET "\e[0m\n");
			}
			strPtr = strstr(line, "EDGE");
			if(strPtr != NULL){
				printf( "\e[0;47m");
				printf( __YELLOW "Setting Edge\t\t\t");
				if(strstr(line, "rising") != NULL){
					setEdge(pinNum,"rising");
					printf("rising");
				}else if(strstr(line, "falling") != NULL){
					setEdge(pinNum,"falling");
					printf("falling");
				}else if(strstr(line, "both") != NULL){
					setEdge(pinNum,"both");
					printf("both");
				}else{
					setEdge(pinNum,"none");
					printf("none");
				}
				printf("\t\t\t" __RESET "\e[0m\n");
			}

			strPtr = strstr(line, "PULL");
			if(strPtr != NULL){
				printf( "\e[0;47m");
				printf( __MAGENTA "Setting Pullups/downs\t\t");
				if(strstr(line, "down") != NULL){
					setPullUps(pinNum,PULLDOWN);
					printf("down");
				}else if(strstr(line, "up") != NULL){
					setPullUps(pinNum,PULLUP);
					printf("up");
				}else{
					setPullUps(pinNum,PULLOFF);
					printf("off");
				}
				printf("\t\t\t" __RESET "\e[0m\n");
			}

			strPtr = strstr(line, "unexport");
			if(strPtr != NULL){
				unexportGPIO(pinNum);
				printf("Unexporting GPIO %d\n",pinNum);
			}
		}
	}
	printf( __RESET );

	if(line){
		free(line);
	}
	return 0;
}
}
#endif
