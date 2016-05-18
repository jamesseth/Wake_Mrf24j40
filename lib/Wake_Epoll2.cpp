/*
 * working.cpp
 *
 *  Created on: 03 Sep 2014
 *      Author: ampm
 */
#ifndef WAKE_EPOLL_CPP_
#define WAKE_EPOLL_CPP_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <asm/types.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include  <sys/stat.h>


#define MAX_EVENTS 32

class Wake_Epoll{
private:
	int epfd;
	struct epoll_event *events;

	int funcPtrIndex;
	int fd_events[MAX_EVENTS];
	void (*functions[MAX_EVENTS])(int);

public:

	Wake_Epoll(){
		epfd = epoll_create1(0);
		if(epfd == -1){
			perror ("createEpoll()  :");
			return;
		}
		events = (struct epoll_event *)malloc(sizeof (struct epoll_event) * MAX_EVENTS);
		if (!events) {
			perror ("waitForEpollEvent() :    malloc  ");
			return;
		}

	}

	~Wake_Epoll(){
		if(events){
			free(events);
		}
		if(epfd >0){
			close(epfd);
		}
	}

	int addEvent(int fd, __u32 fdEvents,void (*functionPointer)(int)){
		struct epoll_event event;
		int ret;

		__u32 index;
		for(__u32 i =0; i < MAX_EVENTS; i++){
			if(functions[i] == NULL){
				index =i;
				i = MAX_EVENTS;
			}
		}
		functions[index]=functionPointer;
		fd_events[index]=fd;
		event.data.fd=index;
		event.events = fdEvents;

		//read(fd,NULL,4096);
		ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
		if(ret == -1){
			perror ("addEpollEvent():  ");
		}
		return ret;
	}


	int modEvent(int fd, __u32 fdEvents){
		struct epoll_event event;
		int ret;

		event.data.fd=fd;
		event.events = fdEvents;
		ret = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
		if(ret == -1){
			perror ("modEpollEvent():  ");
		}
		return ret;
	}

	int removeEvent(int fd, __u32 fdEvents){			// not complete need to remove function ptrs fd_events etc..
		struct epoll_event event;
		int ret;

		ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &event);
		if(ret == -1){
			perror ("removeEpollEvent():  ");
		}
		return ret;
	}

	int waitForEvent(int timeout =-1 ){

		int nr_events;
		if (!events) {
			perror ("waitForEpollEvent() :    malloc  ");
			return 1;
		}
		nr_events = epoll_wait (epfd, events, MAX_EVENTS, timeout);
		if (nr_events < 0) {
			perror ("waitForEpollEvent() :   epoll_wait   ");
			free (events);
			return 1;
		}

		for (__u8 i = 0; i < nr_events; i++) {
			//printf ("event=%ld on fd=%ld\n", (long int)events[i].events, (long int)events[i].data.fd);
			/*
			* We now can, per events[i].events, operate on
			* events[i].data.fd without blocking.
			*/
			if(events[i].events != 16){
			//	char buffer[65];
			//	int ret=0;
				functions[events[i].data.fd](fd_events[events[i].data.fd]);




			}else if(events[i].events == EPOLLHUP){
				//EPOLLHUP_Handler(events[i]);
				return -1;
			}
		}


		return -1;
	}

	bool EPOLLHUP_Handler(struct epoll_event oldevent){
		struct epoll_event temp;
		int fd = fd_events[oldevent.data.fd];
		temp.data.fd = oldevent.data.fd;
		temp.events = EPOLLIN | EPOLLET;

		char* buffer;
		char path[127];
		asprintf(&buffer, "/proc/self/fd/%d",fd);
		readlink(buffer, path, 127);
		removeEvent(fd, EPOLLIN | EPOLLET);
		close(fd);

		if(access(path,F_OK) != -1){
			fd = open(path,O_RDONLY | O_NONBLOCK);
			if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &temp) == -1){
				perror ("EPOLLHUP_Handle:  ");
				return false;
			}
			return true;
		}
		return false;

	}


};
#endif
