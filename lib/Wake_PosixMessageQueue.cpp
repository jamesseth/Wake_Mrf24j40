#ifndef _WAKE_POSIX_MESSGE_QUEUE_CPP_
#define _WAKE_POSIX_MESSGE_QUEUE_CPP_

#include "Wake_PosixMessageQueue.hpp"
namespace wake{

	MessageQ::MessageQ(const char* queueName, int flags, int maxQueueSize, int maxMsgSize){
				mq_name = (char*)queueName;
				/* initialize the queue attributes */
				attr.mq_flags = flags;
				attr.mq_maxmsg = maxQueueSize;
				attr.mq_msgsize = maxMsgSize;
				attr.mq_curmsgs = 0;

				/* create the message queue */
				mq = mq_open(queueName,flags, 0644, &attr);
				if(mq == (mqd_t) -1){
					perror("Wake_MessageQ(char* queueName, int flags, int maxQueueSize, int maxMsgSize) : failed to create Message Queue\n");
			    	this->~MessageQ();
					return;
				}

				if(mq_getattr(mq, &attr) == -1){
					perror("Wake_MessageQ(char* queueName, int flags, int maxQueueSize, int maxMsgSize) : failed to get  Message Queue attributes\n");
					this->~MessageQ();
					return;
				}

				buffer = (uint8_t*)malloc(attr.mq_msgsize);
				if (buffer == NULL){
					perror("Wake_MessageQ(char* queueName, int flags, int maxQueueSize, int maxMsgSize) : failed allocate memory for Message Queue\n");
					this->~MessageQ();
					return;
				}

//				newPacket = (wakePacket*)malloc(sizeof(wakePacket));
//				if ( newPacket == NULL){
//					perror("Wake_MessageQ(char* queueName, int flags, int maxQueueSize, int maxMsgSize) : failed allocate memory for Message Queue\n");
//					this->~MessageQ();
//					return;
//			   }



		}

	MessageQ::~MessageQ(){
			if(buffer != NULL){
				free(buffer);
			}

			/* cleanup */
			while((mqd_t)-1 != mq_close(mq));
			while((mqd_t)-1 != mq_unlink((const char*)mq_name));
		}

	// updates the attr structure with the current attributes of the message queue
	bool MessageQ::updateAttr(){
		if (mq_getattr(mq, &attr) == -1){
			return false;
		}
		return true;
	}

	//gets the size of the message queue returns -1 if it fails to get size
	int MessageQ::getSize(){
		if(updateAttr()){
			return attr.mq_curmsgs;
		}
		return -1;
	}

	// get next message from queue and store it in msgBuffer. Returns nBytes in message read or -1 if not message
	ssize_t MessageQ::receive(uint8_t* msgBuffer){
		//get message from queue
		ssize_t nBytes =  mq_receive(mq, (char*)msgBuffer, attr.mq_msgsize, NULL);
		//copy message into buffer using memcpy
		//free message
		//return number of bytes
		return nBytes;
	}

	// puts message in queue return true if successful
	bool MessageQ::send(uint8_t* newMsg,size_t len,int priority =0){
		//allocate memory for packet
		//copy memory from newMsg to buffer
		//put message(buffer) on queue
		return mq_send(mq, (char*)newMsg,  len, priority);
	}

	mqd_t MessageQ::getQueueDiscriptor(){
		return mq;
	}
}
#endif
