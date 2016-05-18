/*
 * Wake_Spi.cpp
 *
 *  Created on: 06 Jul 2014
 *      Author: James Vlok
 */
#ifndef _WAKE_SPI_CPP_
#define _WAKE_SPI_CPP_
#include "Wake_Spi.h"
namespace wake{
	Spi::Spi(){
		spiFds [0] = -1;
		spiFds [1] = -1;

	}

	Spi::Spi(uint8_t chipSelect, uint8_t mode, uint8_t bitsPerWord, uint32_t clockSpeed,uint32_t delayus){
				spiMode[chipSelect] = mode;
				spiBPW[chipSelect]= bitsPerWord;
				spiDelay[chipSelect]=delayus;
				spiSpeeds[chipSelect] = clockSpeed;

	}

	Spi::Spi(const char* setupFileName) {
		spiFds [0] = -1;
		spiFds [1] = -1;

		if(!setupFromFile(setupFileName)){
			printf("Error setting up from file : %s\n Reverting to Default SPI settings\n",setupFileName);
			spiMode[0] = 0;
			spiBPW[0]=8;
			spiDelay[0]=5;
			spiSpeeds[0] = 1000000;

			spiMode[1] = 0;
			spiBPW[1]=8;
			spiDelay[1]=5;
			spiSpeeds[1] = 1000000;
		}
		if(!begin(0)){
			printf("Could not init SPIO CS0\n");
		}
		if(!begin(1)){
			printf("Could not init SPIO CS1\n");
		}

	}

	Spi::~Spi() {
		if(spiFds[0]){
			close(spiFds[0]);
		}

		if(spiFds[1]){
			close(spiFds[1]);
		}
	}

	bool Spi::begin(int channel){

		int fd ;
		channel &= 1 ;

		if ((fd = open (channel == 0 ? spiDev0 : spiDev1, O_RDWR)) < 0){
			printf("Could not open SPI  file\n");
			return false;
		}
		spiFds[channel]=fd;
		uint32_t speed = spiSpeeds[channel];
		uint8_t newMode = spiMode[channel];
		uint8_t bitsPerWord = spiBPW[channel];

		if (ioctl (fd, SPI_IOC_WR_MODE, &spiMode[0])         < 0){
			printf("WR MODE ERROR\n");
			return false;
		}
		if (ioctl (fd, SPI_IOC_RD_MODE, &spiMode[0])         < 0){
			printf("RD MODE ERROR\n");
			return false;
		}

		if (ioctl (fd, SPI_IOC_WR_BITS_PER_WORD, &spiBPW[0]) < 0){
			printf("WR BPW ERROR\n");
			return false;
		}
		if (ioctl (fd, SPI_IOC_RD_BITS_PER_WORD, &spiBPW[0]) < 0){
			printf("MODE ERROR\n");
			return false;
		}
		busy=false;
		if (ioctl (fd, SPI_IOC_WR_MAX_SPEED_HZ, &spiSpeeds[channel])   < 0) return false;
		if (ioctl (fd, SPI_IOC_RD_MAX_SPEED_HZ, &spiSpeeds[channel])   < 0) return false;
		return true;

	}

	int Spi::byteTransfer (int channel, uint8_t *data, int len, int csToggle = true){
		while(busy);
		busy=true;
		struct spi_ioc_transfer tspi ;
		channel &= 1 ;
		memset (&tspi, 0, sizeof (tspi));
		tspi.tx_buf        = (unsigned long)data ;
		tspi.rx_buf        = (unsigned long)data ;
		tspi.len           = len ;
		tspi.cs_change     = csToggle;
		tspi.delay_usecs   = spiDelay[channel] ;
		tspi.speed_hz      = spiSpeeds [channel] ;
		tspi.bits_per_word = spiBPW [channel];
		busy=false;
		return ioctl (spiFds [channel], SPI_IOC_MESSAGE(1), &tspi) ;
	}



	long Spi::getNumber(char* stringNumber){
		char readBuffer[64]="\0";
		uint8_t ind = 0;
		//printf("Looking for Number\n");
		for(;stringNumber[ind] != '=' && stringNumber[ind] != '\n' && stringNumber[ind] != '\0'; ind++ ){
			if(stringNumber[ind] == '\n' ){
				return 0;
			}
		}
		//printf("Found the Number\n");
		ind++;

		for(uint8_t i =0; i <24 && stringNumber[ind + i] != '\n'; i++){
			readBuffer[i] = stringNumber[ind+i];
		}

		return atol(readBuffer);
	}

	bool Spi::setupCommands(uint8_t channel,char* line){
		char * strPtr=NULL;
		channel &=1;

		strPtr = strstr(line, "MODE");
		if( strPtr != NULL){
			spiMode[channel] = getNumber(strPtr+4);
			return true;
		}
		//--------------------------------------------------Bits Per Word
		strPtr = strstr(line, "BPW");
		if( strPtr != NULL){
			spiBPW[channel] = getNumber(strPtr+3);
			return true;
		}
		//-------------------------------------------------- Delay
		strPtr = strstr(line, "DELAY");
		if( strPtr != NULL){
			spiDelay[channel] = getNumber(strPtr+5);
			return true;
		}
		//-------------------------------------------------- Speed
		strPtr = strstr(line, "SPEED");
		if( strPtr != NULL){
			spiSpeeds[channel] = getNumber(strPtr+5);
			return true;
		}
		return false;
	}

	bool Spi::setupFromFile(const char* fileName){
		if(access(fileName, F_OK) == -1){
			printf("No Access to File: %s\n",fileName);
			return false;
		}

		uint8_t requiredNumber[2]={0,0};
		FILE * setupFile;
		char * line = NULL;

		size_t len = 0;
		ssize_t read;
		setupFile = fopen(fileName, "r");
		if (setupFile == NULL){
			printf("Invalid SPI setup file\n");
			return false;
		}

		while ((read = getline(&line, &len, setupFile)) != -1) {
			if(strstr(line, "SPI_CS0") != NULL){
				if(setupCommands(0,line + 7)){
					requiredNumber[0]++;
				}
			}else if(strstr(line, "SPI_CS1") !=NULL){
				//printf("%s",line);
				if(setupCommands(1,line + 7)){
					requiredNumber[1]++;
				}
			}
		}
		if (line){
			free(line);
		}

		if(requiredNumber[0] == 4 && requiredNumber[1]==4){
			printf("CS0 -- Mode:%d\tBPW:%d\tDelay:%d\tSpeed:%d\n",spiMode[0],spiBPW[0], spiDelay[0],spiSpeeds[0]);
			printf("CS1 -- Mode:%d\tBPW:%d\tDelay:%d\tSpeed:%d\n",spiMode[1],spiBPW[1], spiDelay[1],spiSpeeds[1]);
			return true;
		}
		return false;
	}
}
#endif
