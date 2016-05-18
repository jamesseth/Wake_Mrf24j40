#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

union _frameControl_{
			uint16_t val;
			struct{
				uint16_t lsb 					: 8;
				uint16_t msb					: 8;
		 	}bytes;
			struct{
				uint16_t frameType				:3;
				uint16_t securityEnabled 		:1;
				uint16_t framePending 			:1;
				uint16_t ackRequest 			:1;
				uint16_t intraPan				:1;
				uint16_t resevered				:3;
				uint16_t destAddressingMode		:2;
				uint16_t _reserved				:2;
				uint16_t sourceAddressingMode 	:2;
			}bits;
		}FRAMECONTROL;

int main(void){
	FRAMECONTROL.val =0xAACC;
	printf("%02x%02x\n",FRAMECONTROL.bytes.msb,FRAMECONTROL.bytes.lsb);
	printf("SourceAddressingMode: %02x\n",FRAMECONTROL.bits.sourceAddressingMode);
	printf("destAddressingMode: %02x\n",FRAMECONTROL.bits.destAddressingMode);

}

