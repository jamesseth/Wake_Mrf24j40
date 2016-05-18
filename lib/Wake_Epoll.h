/*
 * Wake_Epoll.h
 *
 *  Created on: 06 Jul 2014
 *      Author: James Vlok
 */

#ifndef WAKE_EPOLL_H_
#define WAKE_EPOLL_H_

#include	<sys/epoll.h>
#include	<cstdio>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/ioctl.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<errno.h>
#include	<cstdlib>
#include	<string.h>




#define MAX_EVENTS 64

class Wake_Epoll {
private:
	struct epoll_event *events;
	struct epoll_event event;
	int epfd;
	bool events_created;
	int funcPtrIndex;
	void (*functions[MAX_EVENTS])();

public:
	Wake_Epoll();
	~Wake_Epoll();
	// Adds a file discriptor to poll , takes a file path and function pointer to a void functionName(void)
	bool addEvent(const char* fileName,void (*funcpointer)());
	bool addSocketEvent(int socketfd, void (*funcpointer)());
	bool addReadFileEvent(const char* fileName,void (*funcpointer)());
	// Starts the epoll on the add file discriptors returns the number of events and runs the function in the
	// corresponding function pointer.
	int startEpoll();
};

/*void myFunc(){
	std::cout<<"hello"<<std::endl;
}

int main(){
	Wake_Epoll myISR;
	myISR.addEvent("/sys/class/gpio/gpio17/value",myFunc);
	myISR.startEpoll();
	myISR.startEpoll();
	myISR.startEpoll();
	myISR.startEpoll();
	myISR.startEpoll();
}

*/





#endif /* WAKE_EPOLL_H_ */
