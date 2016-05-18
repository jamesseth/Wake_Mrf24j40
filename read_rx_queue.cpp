#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <pthread.h>
#include <curses.h>


#include "lib/Wake_Epoll2.cpp"
#include "lib/Wake_PosixMessageQueue.cpp"


static int rxFd;


static uint8_t syncer = 0;
static long long errorpckt=0;
uint8_t presync=0;
static long long packet_cout =0;
uint16_t rssi=0;
wake::MessageQ rxQueue("/mrf24j40_rxqueue",O_RDONLY | O_CREAT, 10, 144);
wake::MessageQ txQueue("/mrf24j40_txqueue",O_WRONLY | O_CREAT , 10, 128);
Wake_Epoll irqEpoll;


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
	}frameControl;

void sendpacket(uint8_t* buffer, uint8_t nBytes){
	uint8_t txbuffer[144];



		txbuffer[0] = 11;
		txbuffer[1] = 11 + nBytes;
		txbuffer[2] = 0x21; //0x29
		txbuffer[3] = 0x88;
		txbuffer[4] = syncer++;
		txbuffer[5] = 0xCC;
		txbuffer[6] = 0xAA;
		txbuffer[7] = 0x56;
		txbuffer[8] = 0xAA;
		txbuffer[9] = 0xCC;
		txbuffer[10] = 0xAA;
		txbuffer[11] = 0x55;
		txbuffer[12] = 0xAA;

		for(uint8_t i =0; i < nBytes;i++){
			txbuffer[i + 13] = *(buffer + i);
		}
		if(txQueue.send(txbuffer,txbuffer[1]+2,1)==-1){
			std::cerr<<"Could not add to queue"<<std::endl;
		}
//		std::cout<<"tx: ";
//		for(uint8_t i =0; i < txbuffer[1]+2;i++){
////					if(i < 13){
//						std::cout<<std::hex<<(int)txbuffer[i]<<" , ";
////					}else{
////						std::cout<<(char)txbuffer[i];
////					}
//
//				}
//				std::cout<<std::endl;

}


void displayPacket(uint8_t * buffer){
	int maxX,maxY;
	WINDOW *infoWindow;

	initscr();
	erase();

	getmaxyx(stdscr,maxX,maxY);

	uint16_t winYUnits = maxY /5;
	uint16_t winXUnits = maxX /40;
	infoWindow = newwin(winYUnits ,maxY , 0, 0);


	start_color();
	init_pair(1, COLOR_WHITE,COLOR_RED);
	init_pair(2, COLOR_WHITE,COLOR_BLUE);
	init_pair(3, COLOR_WHITE,COLOR_BLACK);
	init_pair(4, COLOR_GREEN,COLOR_BLACK);
	init_pair(5, COLOR_BLACK,COLOR_GREEN);
	init_pair(6, COLOR_BLACK,COLOR_YELLOW);
	init_pair(7, COLOR_BLACK,COLOR_RED);
	werase(infoWindow);
	box(infoWindow, 0 , 0);

	wborder(infoWindow, '|', '|', '-', '-', '+', '+', '+', '+');
	mvwprintw(infoWindow,2,10,"Frame Length: %d", buffer[0]);
	mvwprintw(infoWindow,2,40,"LQI : %d ",buffer[buffer[0]+1]);
	mvwprintw(infoWindow,4,10,"frame Control: 0x%02x", (buffer[1] | (buffer[2]<<8)));
	rssi = (9*rssi) +  buffer[buffer[0]+2];
	rssi/=10;
	mvwprintw(infoWindow,4,40,"RSSI : %d ",rssi);
	mvwprintw(infoWindow,6,10,"Sequence Number: %d ",buffer[3]);
	if(packet_cout> 0 && buffer[3]-1 != presync ){
		if(buffer[3] != 0 && presync!=255){
			errorpckt++;
		}
	}
	presync=buffer[3];
	mvwprintw(infoWindow,6,40,"Pckt count: %u ",packet_cout);
	mvwprintw(infoWindow,8,10,"Dest PANID: 0x%02x ",(buffer[4] | (buffer[5]<<8)));
	mvwprintw(infoWindow,8,70,"Errors : %u",errorpckt);
	mvwprintw(infoWindow,10,10,"Dest #: 0x%02x ",(buffer[6] | (buffer[7]<<8)));
	mvwprintw(infoWindow,8,40,"Src PANID: 0x%02x ",(buffer[8] | (buffer[9]<<8)));
	mvwprintw(infoWindow,10,40,"Src #: 0x%02x ",(buffer[10] | (buffer[11]<<8)));

	for(uint8_t i = 0; i < buffer[0] -13;i++){
		int spacer =10;
		mvwprintw(infoWindow,12,spacer+ i,"%02x",buffer[i+12]);
		spacer+=4;
	}

	wrefresh(infoWindow);




}

void rxFile(uint8_t* buff){
	if(rxFd != -1){
		buff[0] -=12;
		std::cout<<write(rxFd,buff+12,buff[0])<<std::endl;
	}

}


void addToRxQueue(int fd){
	uint8_t buffer[144];
		uint8_t nbytes=0;
	while(rxQueue.getSize() > 0){
		nbytes = rxQueue.receive(buffer);
		packet_cout++;
//		for(uint8_t i =0; i < buffer[0]+2;i++){
//			std::cout<<std::hex<<(int)buffer[i]<<" , ";
//		}
		if(rxFd != -1){
			std::cout<<(int)buffer[0]<<std::endl;
			buffer[0] =buffer[0] - 13;
			std::cout<<(int)buffer[0]<<std::endl;
				std::cout<<write(rxFd,&(buffer[12]),(int)buffer[0])<<std::endl;
			}

		//displayPacket(buffer);
//		for(uint8_t i =0; i < buffer[0]+2;i++){
//			if(i < 12 || i > buffer[0]-2){
//				std::cout<<std::hex<<(int)buffer[i]<<" , ";
//			}else if(i <= buffer[0]-2){
//				std::cout<<(char)buffer[i];
//			}
//
//		}
//		std::cout<<std::endl;
	}
}




void* IRQ_Handler(void* arg){

	irqEpoll.addEvent(rxQueue.getQueueDiscriptor(), EPOLLPRI | EPOLLIN,addToRxQueue);
	while(1){

			irqEpoll.waitForEvent(-1);
	}
	pthread_exit((void*)"IRQ_Handler Stopped");

}



int sendFile(const char* fileName){
	int fd = open(fileName,O_RDONLY);
	if(fd==-1){
	//	perror();
		std::cerr<<"Cannot open file"<<std::endl;
		return -1;
	}
	uint8_t filebuf[128];
	int nBytes =1;
	while(nBytes > 0){
		nBytes = read(fd,filebuf,112);
		if(nBytes > 0){
			//std::cout<<"nBytes:"<<nBytes<<std::endl;
		//	while(txQueue.getSize() >= 9);
			sendpacket(filebuf,nBytes);

			usleep(100000);
		}
	}

}







int main(){

	pthread_t IRQ_thread;
	void *IRQ_thread_Results;
	pthread_create(&IRQ_thread, NULL, IRQ_Handler, IRQ_thread_Results);

	rxFd = open("AFterTheFactFast3.JPG",O_WRONLY | O_CREAT);
	if(rxFd == -1){
		std::cerr<<"Could not open after.jpg";
		return -1;
	}

	//while(1){
		//std::cout<<"tx"<<std::endl;
		//sendpacket();
		//sendpacket((uint8_t*)"Hello",5);
		//sleep(2);
		sendFile("before.JPG");
		sleep(5);
		if(rxFd != -1){
			close(rxFd);
		}
		std::cout<<"Done"<<std::endl;

//	}
		while(1);

		endwin();
	pthread_join(IRQ_thread, &IRQ_thread_Results);

	return 0;
}
