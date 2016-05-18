/*
 * Wake_Spi.h
 *
 *  Created on: 06 Jul 2014
 *      Author: James Vlok
 */

#ifndef WAKE_SPI_H_
#define WAKE_SPI_H_


#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

namespace wake{

	class Spi {
	private:
		const char* spiDev0 = "/dev/spidev0.0";
		const char* spiDev1 = "/dev/spidev0.1";
		uint8_t spiMode[2];
		uint8_t spiBPW[2];
		uint32_t spiDelay[2];
		uint32_t spiSpeeds[2];
		int spiFds[2];
		bool busy;

	public:
		Spi();
		Spi(uint8_t chipSelect, uint8_t mode, uint8_t bitsPerWord, uint32_t clockSpeed,uint32_t delayus = 0);
		Spi(const char* setupFileName);
		~Spi();
		bool begin(int channel);
		int byteTransfer (int channel, uint8_t *data, int len, int csToggle);
		long getNumber(char* stringNumber);
		bool setupCommands(uint8_t channel,char* line);
		bool setupFromFile(const char* fileName);
	};
}
#endif /* WAKE_SPI_H_ */
