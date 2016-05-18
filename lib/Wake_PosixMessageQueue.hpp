#ifndef _WAKE_POSIX_MESSAGE_QUEUE_HPP_
#define _WAKE_POSIX_MESSAGE_QUEUE_HPP_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <mqueue.h>



namespace wake{

	class MessageQ{
		private:
			char* mq_name;
			uint8_t* buffer;
			mqd_t mq;
			struct mq_attr attr;

		public:
			MessageQ(const char* queueName, int flags, int maxQueueSize, int maxMsgSize);
			~MessageQ();
			mqd_t getQueueDiscriptor();
			bool updateAttr();
			int getSize();
			ssize_t receive(uint8_t* msgBuffer);
			bool send(uint8_t* newMsg,size_t len,int priority);
	};

}
#endif
