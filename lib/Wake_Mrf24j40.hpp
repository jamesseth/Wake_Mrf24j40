#ifndef _WAKE_MRF24J40_HPP_
#define _WAKE_MRF24J40_HPP_

#include <unistd.h>
#include <stdint.h>
#include <iostream>
#include <sstream>
#include <fstream>

#include "Wake_Spi.cpp"
#include "Wake_IO.cpp"
#include "Wake_Mrf24j40_Registers.hpp"
#include "Wake_Epoll2.cpp"
#include "Wake_PosixMessageQueue.cpp"



namespace wake{
    Wake_IO gpio;
    MessageQ rxQueue("/mrf24j40_rxqueue",O_WRONLY | O_CREAT , 10, 144);
    MessageQ txQueue("/mrf24j40_txqueue",O_RDONLY | O_CREAT, 10, 128);



	class mrf24j40: public Spi{
	private:
    	int reset_pin;
    	int interrupt_pin;
		uint8_t spiBuffer[256];
		uint8_t Rx_Fifo_Buffer[144];
		uint8_t Tx_Normal_Fifo_Buffer[128];
		uint8_t Tx_Beacon_Fifo_Buffer[128];
		uint8_t Tx_GTS1_Fifo_Buffer[128];
		uint8_t Tx_GTS2_Fifo_Buffer[128];
		uint8_t currentChannel;
		bool txBusy;
		uint8_t secKey[0x0F];



		//



		struct _packet{
			bool processed;
			uint8_t frameLength;
			uint8_t headerLength;
			uint8_t payloadLength;
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
			uint8_t sequenceNumber;
			uint16_t destPandId;
			uint16_t srcPanId;
			uint16_t srcAddress;
			uint16_t destAddress;
			uint8_t srcAddresslong[8];
			uint8_t destAddresslong[8];
			uint8_t framePayload[144];
			union _fcs_{
				uint16_t val;
				struct{
					uint8_t lsb;
					uint8_t msb;
				}bytes;
			}FCS;
			union _rssilqi_{
				uint16_t val;
				struct{
					uint16_t LQI	:8;
					uint16_t RSSI	:8;
				}bytes;
			}linkQuality;

		}PACKET;

/* ---------------------------------------------------------------------------*/
		union _rxmcr_{
			uint8_t val;
			struct{
				uint8_t PROMI     : 1;	//Promiscuous Mode bit
				uint8_t ERRPKT    : 1;	//Packet Error Mode bit
				uint8_t COORD     : 1;	//Coordinator bit
				uint8_t PANCOORD  : 1;	//PAN Coordinator bit
				uint8_t _reserved : 1;  //Reserved Maintain as '0'
				uint8_t NOACKRSP  : 1;  //Auto Acknowledge bit (1 : disabled 0: enabled)
				uint8_t reserved  : 2;  //Reserved maintain as '0'
			}bits;
		}RXMCR;

/*------------------------------------------------------------------------------------------*/
		union _panid_{
		        uint16_t val;
		        struct{
		        	uint8_t PANIDL     : 8;  //PANID LSB
		        	uint8_t PANIDH     : 8;  //PANID MSB
		        }bytes;
		}PANID;
/*------------------------------------------------------------------------------------------*/
		union _sadr_{
		        uint16_t val;
		        struct{
		                uint16_t SADRL     : 8;  //SADR LSB
		                uint16_t SADRH     : 8;  //SADR MSB

		        }bytes;
		}SADR;
/*------------------------------------------------------------------------------------------*/
		union _eadr_{
			uint64_t val;
			struct{
				uint64_t EADR0	:8;
				uint64_t EADR1  :8;
				uint64_t EADR2  :8;
				uint64_t EADR3  :8;
				uint64_t EADR4  :8;
				uint64_t EADR5  :8;
				uint64_t EADR6  :8;
				uint64_t EADR7  :8;
			}bytes;
		}EADR;

		/* ---------------------------------------------------------------------------*/
		union _rxflush_{
			uint8_t val;
			struct{
				uint8_t RXFLUSH  : 1;
				uint8_t BCNONLY  : 1;
				uint8_t DATAONLY : 1;
				uint8_t CMDONLY  : 1;
				uint8_t res	 : 1;
				uint8_t WAKEPAD  : 1;
				uint8_t WAKEPOL  : 1;
				uint8_t res_	 : 1;
			}bits;
		}RXFLUSH;
		/* ---------------------------------------------------------------------------*/
		union _order_{
			uint8_t val;
			struct{
				uint8_t SO : 4;
				uint8_t BO : 4;
			}bits;
		}ORDER;
		/* ---------------------------------------------------------------------------*/
		union _txmcr_{
		        uint8_t val;
		        struct{
		                uint8_t CSMABF    : 3;
		                uint8_t MACMINBE  : 2;
		                uint8_t SLOTTED   : 1;
		                uint8_t BATLIFEXT : 1;
		                uint8_t NOCSMA    : 1;

		        }bits;
		}TXMCR;

		/* ---------------------------------------------------------------------------*/
		union _acktmout_{
			uint8_t val;
			struct{
				uint8_t MAWD    : 7;
				uint8_t DRPACK  : 1;

			}bits;
		}ACKTMOUT;
		/* ---------------------------------------------------------------------------*/
		union _eslotg1_{
			uint8_t val;
			struct{
				uint8_t CAP  : 4;
				uint8_t GTS  : 4;

			}bits;
		}ESLOTG1;

		/* ---------------------------------------------------------------------------*/
		union _symtick_{
			uint16_t val;
			struct{
				uint8_t TICKP  : 8;
				uint8_t TXONT  : 7;

			}bytes;
		}SYMTICK;
		/* ---------------------------------------------------------------------------*/
		union _pacon0_{
			uint8_t val;
			struct{
				uint8_t PAONT0_7  : 8;
			}bytes;
		}PACON0;
		/* ---------------------------------------------------------------------------*/
		union _pacon1_{
			uint8_t val;
			struct{
				uint8_t PAONT8  : 1;
				uint8_t PAONTS  : 4;
				uint8_t res_	: 3;
			}bits;
		}PACON1;
		/* ---------------------------------------------------------------------------*/
		union _pacon2_{
			uint8_t val;
			struct{
				uint8_t TXONT   : 2;
				uint8_t TXONTS  : 4;
				uint8_t res	    : 1;
				uint8_t FIFOEN  : 1;

			}bits;
		}PACON2;
		/* ---------------------------------------------------------------------------*/
		union _txbcon0_{
			uint8_t val;
			struct{
				uint8_t TXBTRIG   : 1;
				uint8_t TXBSECEN  : 1;
				uint8_t res	 	  : 6;

			}bits;
		}TXBCON0;

		/* ---------------------------------------------------------------------------*/
		union _txncon_{
			uint8_t val;
			struct{
				uint8_t TXNTRIG   : 1;
				uint8_t TXNSECEN   : 1;
				uint8_t TXNACKREQ : 1;
				uint8_t INDIRECT  : 1;
				uint8_t FPSTAT    : 1;
				uint8_t res	      : 2;

			}bits;
		}TXNCON;

		/* ---------------------------------------------------------------------------*/
		union _txg1con_{
			uint8_t val;
			struct{
				uint8_t TXG1TRIG   : 1;
				uint8_t TXG1SECEN  : 1;
				uint8_t TXG1ACKREQ : 1;
				uint8_t TXG1SLOT   : 3;
				uint8_t TXG1RETRY  : 2;

			}bits;
		}TXG1CON;

		/* ---------------------------------------------------------------------------*/
		union _txg2con_{
			uint8_t val;
			struct{
				uint8_t TXG2TRIG   : 1;
				uint8_t TXG2SECEN  : 1;
				uint8_t TXG2ACKREQ : 1;
				uint8_t TXG2SLOT   : 3;
				uint8_t TXG2RETRY  : 2;

			}bits;
		}TXG2CON;
		/* ---------------------------------------------------------------------------*/
		union _eslotg23_{
			uint8_t val;
			struct{
				uint8_t GTS2	:4;
				uint8_t GTS3	:4;

			}bits;
		}ESLOTG23;

		/* ---------------------------------------------------------------------------*/
		union _eslotg45_{
			uint8_t val;
			struct{
				uint8_t GTS4	:4;
				uint8_t GTS5	:4;

			}bits;
		}ESLOTG45;
		/* ---------------------------------------------------------------------------*/
		union _eslotg67_{
			uint8_t val;
			struct{
				uint8_t GTS6	:4;
				uint8_t res		:4;

			}bits;
		}ESLOTG67;


		/* ---------------------------------------------------------------------------*/
		union _txpend_{
			uint8_t val;
			struct{
				uint8_t FPACK		:1;
				uint8_t GTSSWITCH	:1;
				uint8_t MLIFS		:6;

			}bits;
		}TXPEND;


		/* ---------------------------------------------------------------------------*/
		union _wakecon_{
			uint8_t val;
			struct{
				uint8_t INTL	:6;
				uint8_t REGWAKE	:1;
				uint8_t IMMWAKE	:1;
			}bits;
		}WAKECON;

		/* ---------------------------------------------------------------------------*/
		union _frmoffset_{
			uint8_t val;
			struct{
				uint8_t OFFSET	:8;
			}bits;
		}FTMOFFSET;

		/* ---------------------------------------------------------------------------*/
		union _txstat_{
			uint8_t val;
			struct{
				uint8_t TXNSTAT		:1;
				uint8_t TXG1STAT	:1;
				uint8_t TXG2STAT	:1;
				uint8_t TXG1FNT		:1;
				uint8_t TXG2FNT		:1;
				uint8_t CCAFAIL		:1;
				uint8_t TXNRETRY	:2;
			}bits;
		}TXSTAT;

		/* ---------------------------------------------------------------------------*/
		union _txbcon1_{
			uint8_t val;
			struct{
				uint8_t res			:4;
				uint8_t RSSINUM		:2;
				uint8_t WU_BCN		:1;
				uint8_t TXBMSK		:1;

			}bits;
		}TXBCON1;

		/* ---------------------------------------------------------------------------*/
		union _gateclk_{
			uint8_t val;
			struct{
				uint8_t res			:3;
				uint8_t GTSON		:1;
				uint8_t res_		:4;

			}bits;
		}GATECLK;

		/* ---------------------------------------------------------------------------*/
		union _txtime_{
			uint8_t val;
			struct{
				uint8_t res			:4;
				uint8_t TURNTIME	:4;
			}bits;
		}TXTIME;

		/* ---------------------------------------------------------------------------*/
		union _hsymtmr_{
			uint16_t val;
			struct{
				uint16_t HSYMTMRL	:8;
				uint16_t HSYMTMRH	:8;

			}bits;
		}HSYMTMR;
		/* ---------------------------------------------------------------------------*/
		union _softrst_{
			uint8_t val;
			struct{
				uint8_t RSTMAC		:1;
				uint8_t RSTBB		:1;
				uint8_t RSTPWR		:1;
				uint8_t res			:5;

			}bits;
		}SOFTRST;

		/* ---------------------------------------------------------------------------*/
		union _seccon0_{
			uint8_t val;
			struct{
				uint8_t TXNCIPHER	:3;
				uint8_t RXCIPHER	:3;
				uint8_t SECSTART	:1;
				uint8_t SECIGNORE	:1;

			}bits;
		}SECCON0;
		/* ---------------------------------------------------------------------------*/

		union _secon1_{
			uint8_t val;
			struct{
				uint8_t DISENC		:1;
				uint8_t DISDEC		:1;
				uint8_t res			:2;
				uint8_t TXBCIPHER	:3;
				uint8_t res_		:1;

			}bits;
		}SECCON1;

		/* ---------------------------------------------------------------------------*/
		union _txstbl_{
			uint8_t val;
			struct{
				uint8_t MSIFS	:4;
				uint8_t RFSTBL	:4;

			}bits;
		}TXSTBL;
		/* ---------------------------------------------------------------------------*/

		union _rxsr_{
			uint8_t val;
			struct{
				uint8_t res			:2;
				uint8_t SECDECERR	:1;
				uint8_t res_		:2;
				uint8_t BATIND		:1;
				uint8_t UPSECERR	:1;
				uint8_t _res		:1;
			}bits;
		}RXSR;
		/* ---------------------------------------------------------------------------*/

		union _intstat_{
			uint8_t val;
			struct{
				uint8_t TXNIF		:1;
				uint8_t TXG1IF		:1;
				uint8_t TXG2IF		:1;
				uint8_t RXIF		:1;
				uint8_t SECIF		:1;
				uint8_t HSYMTMRIF	:1;
				uint8_t WAKEIF		:1;
				uint8_t SLPIF		:1;
			}bits;
		}INTSTAT;
		/* ---------------------------------------------------------------------------*/

		union _intcon_{
			uint8_t val;
			struct{
				uint8_t TXNIE		:1;
				uint8_t TXG1IE		:1;
				uint8_t TXG2IE		:1;
				uint8_t RXIE		:1;
				uint8_t SECIE		:1;
				uint8_t HSYMTMRIE	:1;
				uint8_t WAKEIE		:1;
				uint8_t SLPIE		:1;
			}bits;
		}INTCON;
		/* ---------------------------------------------------------------------------*/
		union _slpack_{
			uint8_t val;
			struct{
				uint8_t WAKECNT		:7;
				uint8_t SLPACK	:1;

			}bits;
		}SLPACK;
		/* ---------------------------------------------------------------------------*/
		union _rfctl_{
			uint8_t val;
			struct{
				uint8_t RFRXMODE	:1;
				uint8_t RFTXMODE	:1;
				uint8_t RFRST		:1;
				uint8_t WAKECNT		:2;
				uint8_t res		:3;

			}bits;
		}RFCTL;
		/* ---------------------------------------------------------------------------*/
		union _seccr2_{
			uint8_t val;
			struct{
				uint8_t TXG1CIPHER	:3;
				uint8_t TXG2CIPHER	:3;
				uint8_t UPENC		:1;
				uint8_t UPDEC		:1;

			}bits;
		}SECCR2;
		/* ---------------------------------------------------------------------------*/
		union _bbreg0_{
			uint8_t val;
			struct{
				uint8_t TURBO		:1;
				uint8_t res			:7;
			}bits;
		}BBREG0;
		/* ---------------------------------------------------------------------------*/
		union _bbreg1_{
			uint8_t val;
			struct{
				uint8_t res			:2;
				uint8_t RXDECINV	:1;
				uint8_t res_			:5;
			}bits;
		}BBREG1;
		/* ---------------------------------------------------------------------------*/
		union _bbreg2_{
			uint8_t val;
			struct{
				uint8_t res			:2;
				uint8_t CCACSTH		:4;
				uint8_t CCAMODE		:2;
			}bits;
		}BBREG2;
		/* ---------------------------------------------------------------------------*/
		union _bbreg3_{
			uint8_t val;
			struct{
				uint8_t res			:1;
				uint8_t PREDETTH	:3;
				uint8_t PREVALIDTH	:4;
			}bits;
		}BBREG3;
		/* ---------------------------------------------------------------------------*/
		union _bbreg4_{
			uint8_t val;
			struct{
				uint8_t res			:2;
				uint8_t PRECNT0		:3;
				uint8_t CSTH		:3;
			}bits;
		}BBREG4;
		/* ---------------------------------------------------------------------------*/
		union _bbreg6_{
			uint8_t val;
			struct{
				uint8_t RSSIRDY		:1;
				uint8_t res			:5;
				uint8_t RSSIMODE	:2;
			}bits;
		}BBREG6;
		/* ---------------------------------------------------------------------------*/
		union _ccaedth_{
			uint8_t val;
			struct{
				uint8_t CCAEDTH0_8			:8;

			}bits;
		}CCAEDTH;
		/* ---------------------------------------------------------------------------*/
		union _rfcon0_{
			uint8_t val;
			struct{
				uint8_t RFOPT		:4;
				uint8_t CHANNEL		:4;

			}bits;
		}RFCON0;
		/* ---------------------------------------------------------------------------*/
		union _rfcon1_{
			uint8_t val;
			struct{
				uint8_t VCOOPT		:8;

			}bits;
		}RFCON1;
		/* ---------------------------------------------------------------------------*/
		union _rfcon2_{
			uint8_t val;
			struct{
				uint8_t res			:7;
				uint8_t PLLEN		:1;

			}bits;
		}RFCON2;
		/* ---------------------------------------------------------------------------*/
		union _rfcon3_{
			uint8_t val;
			struct{
				uint8_t res			:3;
				uint8_t TXPWRS		:3;
				uint8_t TXPWRL		:2;

			}bits;
		}RFCON3;
		/* ---------------------------------------------------------------------------*/
		union _rfcon5_{
			uint8_t val;
			struct{
				uint8_t res			:4;
				uint8_t BATTH		:4;

			}bits;
		}RFCON5;
		/* ---------------------------------------------------------------------------*/
		union _rfcon6_{
			uint8_t val;
			struct{
				uint8_t res			:3;
				uint8_t BATEN		:1;
				uint8_t MRECVR		:1;
				uint8_t res_		:2;
				uint8_t TXFIL		:2;

			}bits;
		}RFCON6;
		/* ---------------------------------------------------------------------------*/
		union _rfcon7_{
			uint8_t val;
			struct{
				uint8_t res			:6;
				uint8_t SLPCLKSEL0	:2;

			}bits;
		}RFCON7;
		/* ---------------------------------------------------------------------------*/
		union _rfcon8_{
			uint8_t val;
			struct{
				uint8_t res			:4;
				uint8_t RFVCO		:1;
				uint8_t res_		:3;

			}bits;
		}RFCON8;
		/* ---------------------------------------------------------------------------*/
		union _slpcal0_{
			uint8_t val;
			struct{
				uint8_t SLPCAL		:8;

			}bits;
		}SLPCAL0;
		/* ---------------------------------------------------------------------------*/
		union _slpcal1_{
			uint8_t val;
			struct{
				uint8_t SLPCAL		:8;

			}bits;
		}SLPCAL1;
		/* ---------------------------------------------------------------------------*/
		union _slpcal2_{
			uint8_t val;
			struct{
				uint8_t SLPCAL		:4;
				uint8_t SLPCALEN	:1;
				uint8_t res			:2;
				uint8_t SLPCALRDY	:1;

			}bits;
		}SLPCAL2;
		/* ---------------------------------------------------------------------------*/
		union _rfstate_{
			uint8_t val;
			struct{
				uint8_t res			:5;
				uint8_t RFSATE  	:3;


			}bits;
		}RFSTATE;
		/* ---------------------------------------------------------------------------*/
		union _rssi_{
			uint8_t val;
			struct{
				uint8_t RSSI		:4;

			}bits;
		}RSSI;
		/* ---------------------------------------------------------------------------*/
		union _slpcon0_{
			uint8_t val;
			struct{
				uint8_t SLPCLKEN	:1;
				uint8_t INTEDGE		:1;
				uint8_t res			:6;


			}bits;
		}SLPCON0;
		/* ---------------------------------------------------------------------------*/
		union _slpcon1_{
			uint8_t val;
			struct{
				uint8_t SLPCLKDIV	:5;
				uint8_t CLKOUTEN	:1;
				uint8_t res			:2;


			}bits;
		}SLPCON1;
		/* ---------------------------------------------------------------------------*/
		union _waketime_{
			uint16_t val;
			struct{
				uint16_t WAKETIMEL	:8;
				uint16_t WAKETIMEH	:3;
				uint16_t res		:5;


			}bits;
		}WAKETIME;
		/* ---------------------------------------------------------------------------*/
		union _remcnt_{
			uint16_t val;
			struct{
				uint16_t REMCNTL	:8;
				uint16_t REMCNTH	:8;
			}bits;
		}REMCNT;
		/* ---------------------------------------------------------------------------*/
		union _maincnt_{
			uint32_t val;
			struct{
				uint32_t MAINCNT0	:8;
				uint32_t MAINCNT1	:8;
				uint32_t MAINCNT2	:8;
				uint32_t MAINCNT3	:2;
				uint32_t res		:5;
				uint32_t STARTCNT	:1;

			}bits;
		}MAINCNT;
		/* ---------------------------------------------------------------------------*/
		union _testmode_{
			uint8_t val;
			struct{
				uint8_t TESTMODE	:3;
				uint8_t RSSIWAIT	:2;
				uint8_t res			:3;
			}bits;
		}TESTMODE;
		/* ---------------------------------------------------------------------------*/
		union _assoeadr_{
			uint64_t val;
			struct{
				uint64_t ASSOEADR0	:8;
				uint64_t ASSOEADR1	:8;
				uint64_t ASSOEADR2	:8;
				uint64_t ASSOEADR3	:8;
				uint64_t ASSOEADR4	:8;
				uint64_t ASSOEADR5	:8;
				uint64_t ASSOEADR6	:8;
				uint64_t ASSOEADR7	:8;
			}bits;
		}ASSOEADR;
		/* ---------------------------------------------------------------------------*/
		union _assosadr_{
			uint16_t val;
			struct{
				uint16_t ASSOSADL	:8;
				uint16_t ASSOSADH	:8;

			}bits;
		}ASSOSADR;
		/* ---------------------------------------------------------------------------*/
	public:
		mrf24j40();
		mrf24j40(uint8_t channel);
		mrf24j40(const char* fileName);
		~mrf24j40();
		void writeShortAddress(uint8_t address, uint8_t value);
		uint8_t readShortAddress(uint8_t address);
		void  writeLongAddress(uint16_t address,uint8_t value);
		uint8_t readLongAddress(uint16_t address);
		void setPanId(uint16_t panId);
		uint16_t getPanId();
		void setNodeShortAddress(uint16_t address);
		uint16_t getNodeShortAddress();
		void setChannel(uint8_t channel);
		uint8_t getChannel();
		uint8_t channelAssessment(uint8_t channel);
		void reset();
		void clearRxFifo();
		void makeCoord();
		void makePANCoord();
		void enableAutoAcknowledge();
		void setPromiscuousMode();
		void enableBatteryMonitor();
		uint8_t checkBatteryVoltage();
		void setupInterrupts(); // TODO: finish interrupt setup member/function
		void enableFifo(bool onOff);
		void setTxOnTimeSymbolBits(int useconds);
		void init();
		uint8_t recieveOld();
		uint8_t recieve();
		void display();
		template <class T>
		T getConf(const char* fileName, std::string key, T defaultValue);
		void Interrupt_Handler(int fd);
		uint8_t sendData(uint8_t * buffer);

		void setTxNormalFifoSecurityKey(uint8_t* key);
		void selectSecuritySuit(uint8_t suit);






	};









}









#endif
