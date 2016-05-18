#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

union _myunion_{
	uint16_t val;
	uint16_t lsb : 8;
	uint16_t msb : 8;
}panID;

int main(void){
	panID.val =0xAACC;
	printf("%02x\n",panID.msb);
}
