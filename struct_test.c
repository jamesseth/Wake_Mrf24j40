#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

union myUnion{
	uint8_t reg;
	struct{
		uint8_t lsb: 4;
		uint8_t msb: 4;
	}myreg;

}hello;

int main(void){
//	myReg hello;
	hello.reg=0xF0;
	printf("reg is :%02x\n lsb is: %02x\n msb is: %02x ",hello.reg,hello.myreg.lsb,hello.myreg.msb);
	return 0;	
}
