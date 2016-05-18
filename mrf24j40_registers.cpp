#define _NDEBUG_

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <pthread.h>

#include "lib/Wake_Spi.cpp"
#include "lib/Wake_Mrf24j40.cpp"
#include "lib/Wake_Epoll2.cpp"

Wake_Epoll irqEpoll;

wake::mrf24j40 txrx("conf/mrf24j40");
#ifdef _DEBUG_
wake::MessageQ testQueue("/mrf24j40_rxqueue",O_RDONLY | O_CREAT, 10, 144);
#endif
uint8_t txbuffer[128];

void Interrupt_Handler(int fd){
	txrx.Interrupt_Handler(fd);
/*				read(fd,NULL,1);
 *				txrx.readShortAddress(MRF_INTSTAT);
 *				txrx.recieve();
 *
 *				//txrx.writeShortAddress(MRF_BBREG1, 0x04);
 */
}

//void sendpacket(){
//
//
//	uint8_t nbytes=2;
//
//	txbuffer[nbytes++] = 0x21;
//	txbuffer[nbytes++] = 0x88;
//	txbuffer[nbytes++] = 1;
//	txbuffer[nbytes++] = 0xCC;
//	txbuffer[nbytes++] = 0xAA;
//	txbuffer[nbytes++] = 0x56;
//	txbuffer[nbytes++] = 0xAA;
//	txbuffer[nbytes++] = 0xCC;
//	txbuffer[nbytes++] = 0xAA;
//	txbuffer[nbytes++] = 0x55;
//	txbuffer[nbytes++] = 0xAA;
//	txbuffer[0] = nbytes-2;
//	txbuffer[1] = nbytes-1;
//	txbuffer[nbytes++] = 'A';
//	txrx.sendData(txbuffer);
//
//
//
//
//
//}









void transmit(int fd){
	uint8_t tmp[144];

	while(wake::txQueue.getSize() > 0){
		//std::cout<<"-------------------------------"<<std::endl;
		wake::txQueue.receive(tmp);
		txrx.sendData(tmp);

	}
}






//template <class T>
//T getConf(const char* fileName, std::string key, T defaultValue){
//	T value;
//	try{
//		// Open file
//			std::ifstream infile( fileName );
//			// check if file was successfully opened
//			if(!infile){
//				//could not open file throw exception
//				throw(std::runtime_error( "Cannot open Config file. "));
//			}
//			// declare string buffer for line to be read from file
//			std::string line;
//
//			// read line from file
//			while( std::getline(infile, line)){
//				//pass line read from file into string stream
//				std::istringstream is_line(line);
//				// extract value from string stream
//				std::string buff;
//				if(is_line >> buff >> value){
//#ifdef _DEBUG_
//					std::cout<<"#---"<<buff <<" -> "<< value<<std::endl;
//#endif
//
//					// Check if key matches buffer
//					if(buff == key){
//						//check if file is open and close file if open before returning
//						if(infile){
//							infile.close();
//						}
//#ifdef _DEBUG_
//						std::cout<<key <<" -> "<< value<<std::endl;
//#endif
//						return value;
//					}
//
//
//				}
//
//			}
//			if(infile){
//				infile.close();
//			}
//			throw(std::runtime_error( "Error getting integer from conf file. "));
//	}catch(const std::exception& ex){
//		std::cerr << ex.what() << "\nFatal error" << std::endl;
//
//	}
//	return defaultValue;
//}



void* IRQ_Handler(void* arg){

	irqEpoll.addEvent(wake::txQueue.getQueueDiscriptor(), EPOLLPRI | EPOLLIN,transmit);
	while(1){

		irqEpoll.waitForEvent(-1);
	//	sleep(5);
	//	std::cout<<"tx"<<std::endl;
		//	sendpacket();
	}
	pthread_exit((void*)"IRQ_Handler Stopped");

}












int main(void){
	pthread_t IRQ_thread;
	void *IRQ_thread_Results;
	pthread_create(&IRQ_thread, NULL, IRQ_Handler, IRQ_thread_Results);
	uint16_t buff = txrx.getPanId();
	std::cout<<"PanID: "<<std::hex<<(int)buff<<std::endl;
	buff = txrx.getNodeShortAddress();
	std::cout<<"Short Address: "<<std::hex<<(int)buff<<std::endl;
	std::cout<<"Channel: "<<std::dec<<(int)txrx.getChannel()<<std::endl;


	char c;
	int count = 10;



			int fd = -1;
			fd = open("/sys/class/gpio/gpio24/value",O_RDONLY);



			if( fd != -1){
				read(fd,NULL,256);

				irqEpoll.addEvent(fd,EPOLLET | EPOLLIN,Interrupt_Handler);

			}else{
				std::cerr<<"Unable to open gpio24"<<std::endl;
			}

			while(1){
				//read(fd,NULL,1);
				//TODO: start pThreads for txQueue
				irqEpoll.waitForEvent(-1);
#ifdef _DEBUG_
				while(testQueue.getSize() > 0){
					uint8_t * temp;
					uint8_t nbs = testQueue.receive(temp);
					for(uint8_t i =0; i < nbs; i++){
						std::cout<<(int)temp[i];
					}std::cout<<std::endl;
				}
#endif
				// waiting for interrupt
				//txrx.display();
			}
//		interfd = open("/sys/class/gpio/gpio24/value",O_RDONLY);
//		if(interfd == -1){
//			perror("Interrupt gpio value file could not be opened: ");
//			return -1;
//		}
//		read(interfd,&c,1);
//	//	std::cout<<"C = "<<(char)c<<std::endl;
//		if(c == '1'){
//			txrx.readShortAddress(MRF_INTSTAT);
//			//txrx.writeShortAddress(MRF_BBREG1, 0x04);
//			txrx.recieve();
//			txrx.display();
			//txrx.clearRxFifo();
			//txrx.writeShortAddress(MRF_BBREG1, 0x00);
			//count--;

//		}
//		close(interfd);
//		usleep(100);
//	}


}

