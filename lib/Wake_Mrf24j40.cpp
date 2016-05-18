#ifndef _WAKE_MRF24J40_CPP_
#define _WAKE_MRF24J40_CPP_

#include "Wake_Mrf24j40.hpp"


namespace wake{
	// Default constructor, sets Spi to chipslect 0, mode 0 , BPW 8, clock 4 Mhz, delay 5us
	mrf24j40::mrf24j40(): Spi(0,0,8,4000000,5){
		//setup spi bus using chip select 0 (cs0), then check if successful
		if(!begin(0)){
			// an error occured while setting up spi bus
			//print error to std::cerr
			std::cerr<<"ERROR!!!!"<<std::endl;
		}

		// setup gpio
		// set reset pin to gpio25
		reset_pin=25;
		// export gpio to FS
		gpio.exportGPIO(reset_pin);
		// set reset gpio direction to output
		gpio.pinMode(reset_pin,"out");
		// set reset gpio to HIGH (reset pin on mrf24j40 is active low)
		gpio.digitalWrite(reset_pin,1);

		// set interrupt pin to gpio 24
		interrupt_pin=24;
		// export gpio to FS
		gpio.exportGPIO(interrupt_pin);
		// set interrupt gpio direction to input
		gpio.pinMode(interrupt_pin,"in");
		// set interrupt gpio to HIGH (INT pin edge is configurable,
		//assumed rising to support epoll() with no lockup)
		gpio.setEdge(interrupt_pin,RISING);
		txBusy=false;

	}

	mrf24j40::mrf24j40(uint8_t channel): Spi(0,0,8,4000000,5){

	}
	//default deconstructor
	mrf24j40::~mrf24j40(){}

	/* read register in low address bank 0x00 to 0xFF
	 *  arguments: 8bit address of register (uint8_t)
	 *  effects: none
	 *  return: returns the value in the register at address (uint8_t)
	 *
	 */
	uint8_t mrf24j40::readShortAddress(uint8_t address){

		spiBuffer[0] = 0x7E & (address << 1) ;
		spiBuffer[1] = 0x00;
		if(0 > byteTransfer (0, spiBuffer, 2, false)){
				perror("ReadShortAddress(): ");

			}
		return spiBuffer[1];
	}

	/* write register in low address bank 0x00 to 0xFF
	 *  arguments: 8bit address of register (uint8_t) , value to be written to register(uint8_t)
	 *  effects: writes register at address to value
	 *  return: none
	 *
	 */
	void mrf24j40::writeShortAddress(uint8_t address, uint8_t value){
		spiBuffer[0] = 0x7E & (address << 1) ;
		spiBuffer[0] |= 0x01;
		spiBuffer[1] = value;
		if(0 > byteTransfer (0, spiBuffer, 2, false)){
			perror("WriteShortAddress(): ");

		}
	}


	uint8_t mrf24j40::readLongAddress(uint16_t address){
		spiBuffer[0] = (address>>3) |0x80;
		spiBuffer[1] = address<<5;
		spiBuffer[2] = 0x00;
		byteTransfer (0, spiBuffer, 3, false);
	    return spiBuffer[2];
	}


	void mrf24j40::writeLongAddress(uint16_t address, uint8_t value){
		spiBuffer[0] = (address>>3) |0x80;
		spiBuffer[1] = (address<<5) & 0xE0|0x10;
		spiBuffer[2] = value;
		byteTransfer (0, spiBuffer, 3, false);
	}


    void mrf24j40::setPanId(uint16_t panId){
    	PANID.val= panId;
    	writeShortAddress(MRF_PANIDL,PANID.bytes.PANIDL);
    	writeShortAddress(MRF_PANIDH,PANID.bytes.PANIDH);
    }

    uint16_t mrf24j40::getPanId(){
    	PANID.bytes.PANIDL=readShortAddress(MRF_PANIDL);
    	PANID.bytes.PANIDH=readShortAddress(MRF_PANIDH);
    	return PANID.val;
    }


    void mrf24j40::setNodeShortAddress(uint16_t address){
    	SADR.val=address;
    	writeShortAddress(MRF_SADRL,SADR.bytes.SADRL);
    	writeShortAddress(MRF_SADRH,SADR.bytes.SADRH);
    }

    uint16_t mrf24j40::getNodeShortAddress(){
    	SADR.bytes.SADRL=readShortAddress(MRF_SADRL);
    	SADR.bytes.SADRH=readShortAddress(MRF_SADRH);
    	return SADR.val;
    }

    void mrf24j40::setChannel(uint8_t channel){

         if (channel < 11 || channel > 26)
        {
            return;
        }

        writeLongAddress(MRF_RFCON0, ((channel - 11) << 4) | 0x03);
        writeShortAddress(MRF_RFCTL,0x04);
        writeShortAddress(MRF_RFCTL,0x00);
    				//Reset RF State machine
    	usleep(192);

    }

    uint8_t mrf24j40::getChannel(){

    	RFCON0.val = readLongAddress(MRF_RFCON0);
    	currentChannel = RFCON0.val >> 4;
    	currentChannel += 11;

    	return currentChannel;
    }


    uint8_t mrf24j40::channelAssessment(uint8_t channel){
        setChannel(channel);

        // calculate RSSI for firmware request
        BBREG6.val = 0x00;
        BBREG6.bits.RSSIMODE = 2;
        writeShortAddress(MRF_BBREG6, BBREG6.val);

        // Firmware Request the RSSI
        BBREG6.val = readShortAddress(MRF_BBREG6);
        while ((BBREG6.val  & 0x01) != 0x01)
        {
        	BBREG6.val  = readShortAddress(MRF_BBREG6);
        }

        // read the RSSI
        RSSI.val = readLongAddress(MRF_RSSI);

        // enable RSSI attached to received packet again after
        // the energy scan is finished
        BBREG6.val = 0x00;
        BBREG6.bits.RSSIMODE = 1;
        writeShortAddress(MRF_BBREG6, BBREG6.val);
        return RSSI.val;
    }





    void mrf24j40::reset(void) {
        uint8_t i=0xff;
        gpio.digitalWrite(reset_pin,0);
        usleep(300);
        gpio.digitalWrite(reset_pin,1);
        usleep(2000);
        /* do a soft reset */

        writeShortAddress(MRF_SOFTRST,0x07);
        do
        {
            i = readShortAddress(MRF_SOFTRST);
        } while ((i & 0x07) != (uint8_t) 0x00);



    }


    void mrf24j40::clearRxFifo(void){
    	RXFLUSH.val = readShortAddress(MRF_RXFLUSH);
    	RXFLUSH.bits.RXFLUSH = 1;
        writeShortAddress(MRF_RXFLUSH,RXFLUSH.val);
    }

    void mrf24j40::makeCoord(){
    	RXMCR.val = readShortAddress(MRF_RXMCR);
    	RXMCR.bits.COORD =1;
        writeShortAddress(MRF_RXMCR,RXMCR.val);
        TXMCR.val = readShortAddress(MRF_TXMCR);
        TXMCR.bits.SLOTTED =0;
        writeShortAddress(MRF_TXMCR, TXMCR.val);
    }

    void mrf24j40::makePANCoord(){
        RXMCR.val = readShortAddress(MRF_RXMCR);
        RXMCR.bits.PANCOORD =1;
        writeShortAddress(MRF_RXMCR,RXMCR.val);

        TXMCR.val = readShortAddress(MRF_TXMCR);
        TXMCR.bits.SLOTTED =0;
        writeShortAddress(MRF_TXMCR, TXMCR.val);

        ORDER.val=readShortAddress(MRF_ORDER);
		ORDER.bits.BO = 0xF;
        ORDER.bits.SO = 0xF;
        writeShortAddress(MRF_ORDER,ORDER.val);

    }

    void mrf24j40::enableAutoAcknowledge(){
    	RXMCR.val = readShortAddress(MRF_RXMCR);
    	RXMCR.bits.NOACKRSP =0;
    	writeShortAddress(MRF_RXMCR,RXMCR.val);
    }

    // enables promiscuousMode (Accepts all Packets received))
    void mrf24j40::setPromiscuousMode(){
    	RXMCR.val = readShortAddress(MRF_RXMCR);
    	RXMCR.bits.PROMI =1;
    	writeShortAddress(MRF_RXMCR,RXMCR.val);
    }

    void mrf24j40::enableBatteryMonitor(){

        //TODO: Accept argument for battery voltage threshold
        writeLongAddress(MRF_RFCON5,0x80); // Battery voltage low threshold set to 2.5V
        RFCON6.val = readLongAddress(MRF_RFCON6);
        RFCON6.bits.BATEN=1;
        writeLongAddress(MRF_RFCON6,RFCON6.val);
    }

    uint8_t mrf24j40::checkBatteryVoltage(){
    	RXSR.val = readShortAddress(MRF_RXSR);
        return RXSR.bits.BATIND;

    }

    void mrf24j40::setupInterrupts(){
    	INTCON.val=0xFF;
    	INTCON.bits.TXNIE=0;
    	INTCON.bits.RXIE=0;
    	writeShortAddress(MRF_INTCON,INTCON.val);
    }

    void mrf24j40::enableFifo(bool onOff){
    	PACON2.val = readShortAddress(MRF_PACON2);
    	if(onOff == true){
    		PACON2.bits.FIFOEN = 1;
    	}else{
    		PACON2.bits.FIFOEN = 0;
    	}
    	writeShortAddress(MRF_PACON2,PACON2.val);
    }

    void mrf24j40::setTxOnTimeSymbolBits(int useconds){
    	int calc = useconds / 16;
    	PACON2.val = readShortAddress(MRF_PACON2);
    	if(calc >= 1 || calc <= 0x0F){
    		PACON2.bits.TXONTS = calc;
    		writeShortAddress(MRF_PACON2,PACON2.val);
    	}
    }

    void mrf24j40::init(){
        uint8_t i;
        reset();
        clearRxFifo();

        /* Program the short MAC Address, 0xffff */
      //  MRF24J40_setNodeShortAddress(0xFFFF);
      //  MRF24J40_setPANID(0xFFFF);


        writeShortAddress(MRF_PACON2,0x98); //Initialize FIFOEN = 1 and TXONTS = 0x6.
        writeShortAddress(MRF_TXSTBL,0x95); //Initialize RFSTBL = 0x9

     //   while((MRF25J40_readShortAddress(MRF_RFSTATE)&0xA0) != 0xA0);	// wait till RF state machine in RX mode
        writeLongAddress(MRF_RFCON0,0x03); //Initialize RFOPT = 0x03.
        writeLongAddress(MRF_RFCON1,0x01); //Initialize VCOOPT = 0x02.
        writeLongAddress(MRF_RFCON2,0x80); //Enable PLL (PLLEN = 1).
        writeLongAddress(MRF_RFCON3, 0x00); // Tx power set to 0dBm
        writeLongAddress(MRF_RFCON6,0x90); //Initialize TXFIL = 1 and 20MRECVR = 1.
        writeLongAddress(MRF_RFCON7,0x80); //Initialize SLPCLKSEL = 0x2 (100 kHz Internal oscillator).
        writeLongAddress(MRF_RFCON8,0x10); //Initialize RFVCO = 1.
        writeLongAddress(MRF_SLPCON1,0x21);

        writeShortAddress(MRF_BBREG2,0x80); //Set CCA mode to ED.
        writeShortAddress(MRF_CCAEDTH,0x60); //Set CCA ED threshold
        writeShortAddress(MRF_BBREG6,0x40); //Set appended RSSI value to RXFIFO.
      // MRF25J40_writeShortAddress(MRF_INTCON,0xF6); // enable only RXIF and TXIF

        setChannel(11);      //Set channel to 11

        writeShortAddress(MRF_RFCTL, 0x04); // Reset RF state machine.
        writeShortAddress(MRF_RFCTL, 0x00); // part 2
        usleep(200); // delay at least 192usec
        do
        {
            i = readLongAddress(MRF_RFSTATE);
        }
        while((i&0xA0) != 0xA0);
        setupInterrupts();	// INTCON, enabled=0. RXIE and TXNIE only enabled
        //writeLongAddress(MRF_SLPCON0,0x02);
        enableAutoAcknowledge();
        writeShortAddress(MRF_TXNCON,0x0C); // Rqst Ack, no packets pending, indirect communication enabled
        RXFLUSH.val = readShortAddress(MRF_RXFLUSH);

//        RXFLUSH.bits.DATAONLY = 1;
//        writeShortAddress(MRF_RXFLUSH,RXFLUSH.val);
    }


    uint8_t mrf24j40::recieve(){
    	//get size of packet in fifo + 2 bytes for the RSSI & LQI bytes appended to packet
    	uint8_t nBytes = 2 + readLongAddress(MRF_RXFIFO);
    	//Set RXDECINV to avoid receiving a packet while reading RX fifo.
    	writeShortAddress(MRF_BBREG1, 0x04);

    	// Read rx fifo normal
    	for(uint8_t i =0; i < nBytes;i++){
    	   	Rx_Fifo_Buffer[i]= readLongAddress(MRF_RXFIFO + i);
    	}
    	//Clear the RxFifo
    	clearRxFifo();
    	//Clear RXDECINV to receive packets again.
    	writeShortAddress(MRF_BBREG1, 0x00);
    	//put packet on rx queue
    	rxQueue.send(Rx_Fifo_Buffer,nBytes,1);

    	return 0;
    }


    uint8_t mrf24j40::recieveOld(){

    	uint16_t Fifo_Index = 0x300; // Rx fifo starts at address 0x300

        //Set RXDECINV to avoid receiving a packet while reading RX fifo.
        writeShortAddress(MRF_BBREG1, 0x04);

        // Read first byte in RXFIFO (0x300) to get number of bytes in packet.
        PACKET.frameLength = readLongAddress(Fifo_Index++); // Framelength = Header length + Payload + FCS + *LQI + *RSSI  (* if set to be appended)
#ifdef _DEBUG_
        /* If _DEBUG_ is define the entire packet is read from the Rx fifo and stored in the Rx_Fifo_Buffer*/
        for(uint8_t i =0; i < PACKET.frameLength + 2;i++){
        	Rx_Fifo_Buffer[i]= readLongAddress(MRF_RXFIFO + i);
        }
#endif
        // Read Frame Control from rx fifo
        PACKET.frameControl.bytes.lsb = readLongAddress(Fifo_Index++);
        PACKET.frameControl.bytes.msb =readLongAddress(Fifo_Index++);
        //read sequence number from rx fifo
        PACKET.sequenceNumber = readLongAddress(Fifo_Index++);

        // Check if the packet is encrypted. the Security enable bit will be set in the control header
        // If the security bit is set decrypt the packet   TODO: implement security features
        if(PACKET.frameControl.bits.securityEnabled == 1){
            //TODO: Decode etc..
        }

    // ACK packet type is handled in hardware on the MRF24J40-----------------------------------------------------------------------------
    // Get PANID   --> ACK frames do not have a PANID
    //    if(MRF24J40_controlFrame.bits.frameType != 2){
    //        MRF24J40_packet.DestinationPANID = (MRF24J40_readLongAddress(MRF_RXFIFO + MRF24J40_packet.HeaderLength + 1) <<8) | MRF24J40_readLongAddress(MRF_RXFIFO + MRF24J40_packet.HeaderLength);
    //        MRF24J40_packet.HeaderLength+=2;
    //    }destPandId
    //------------------------------------------------------------------------------------------------------------------------------------

        // read destination PANID from rxFifo
        PACKET.destPandId = (readLongAddress(Fifo_Index++) <<8) | readLongAddress(Fifo_Index++);

        // Read destination address from RXFIFO
        // Check what destination address type is used.
        if(PACKET.frameControl.bits.destAddressingMode == 2){
        	//16bit short addressing is used read the two bytes from rx fifo
            PACKET.destAddress = (readLongAddress(Fifo_Index++) <<8) | readLongAddress(Fifo_Index++);

        }else if(PACKET.frameControl.bits.destAddressingMode == 3){
        	//64bit short addressing is used read the eight bytes from rx fifo
            for(uint8_t i =0; i < 8; i++){
            	PACKET.destAddresslong[i] = readLongAddress(Fifo_Index++);

            }
        }

        // read source PANID from rxFifo
        PACKET.srcPanId = (readLongAddress(Fifo_Index++) <<8) | readLongAddress(Fifo_Index++);

         // Read source address from RXFIFO
         // Check what destination address type is used.
         if(PACKET.frameControl.bits.sourceAddressingMode == 2){
        	 //16bit short addressing is used read the two bytes from rx fifo
            PACKET.srcAddress = (readLongAddress(Fifo_Index++) <<8) | readLongAddress(Fifo_Index++);
         }else if(PACKET.frameControl.bits.sourceAddressingMode == 3){
        	 //64bit short addressing is used read the eight bytes from rx fifo
             for(uint8_t i =0; i < 8; i++){
            	 PACKET.srcAddresslong[i] = readLongAddress(Fifo_Index++);

            }
         }




        // Calculate payload length is bytes
        PACKET.payloadLength = (PACKET.frameLength - 4) - (Fifo_Index - 0x300 - 1);

        // Read Payload from rx fifo
        for(uint8_t i = 0; i <= PACKET.payloadLength; i++ ){
             PACKET.framePayload[i] = readLongAddress(Fifo_Index++);
        }

        //Read FCS from rx fifo (2 bytes)
        PACKET.FCS.bytes.lsb=readLongAddress( Fifo_Index++);
        PACKET.FCS.bytes.msb=readLongAddress( Fifo_Index++);

        //Read Link quality from rx fifo (1 byte)
        PACKET.linkQuality.bytes.LQI = readLongAddress( Fifo_Index++);
        //Read RSSI from rx fifo (1 byte)
        PACKET.linkQuality.bytes.RSSI = readLongAddress( Fifo_Index++);

        //Clear the RxFifo
        clearRxFifo();

        // Set processed flag to False ("new data")
        PACKET.processed = false;
        //Clear RXDECINV recieve packets again.
        writeShortAddress(MRF_BBREG1, 0x00);

        //return the number of bytes read from rx fifo
        return Fifo_Index;
    }

    void mrf24j40::display(){
    	SLPCON0.val=readLongAddress(MRF_SLPCON0);
    	std::cout<<"+-----------------------------------------------------------------------+"<<std::endl;
    	std::cout<<"Src: "<<std::hex<<(int)PACKET.srcAddress<<"\tSrcPANID: "<<std::hex<<(int)PACKET.srcPanId;
    	std::cout<<"\t Dest: "<<std::hex<<(int)PACKET.destAddress<<"\tDestPANID: "<<std::hex<<(int)PACKET.destPandId<<std::endl;
    	std::cout<<"Frame Length: "<< std::dec<<(int)PACKET.frameLength<<"\t Payload Length: "<<(int)PACKET.payloadLength<<std::endl;
    	std::cout<<"Seqence Number: "<<(int)PACKET.sequenceNumber<<std::endl;
    	std::cout<< "RSSI: "<<std::hex<<PACKET.linkQuality.bytes.RSSI<<"\tLQI: "<< std::hex<<PACKET.linkQuality.bytes.LQI<<std::endl;
    	std::cout<<"Payload:"<<std::endl;

    	for(uint8_t i =0; i < PACKET.payloadLength;i++){
    		std::cout<<(char)PACKET.framePayload[i];
    	}
    	std::cout<<std::endl<<"+-----------------------------------------------------------------------+"<<std::endl;
#ifdef _DEBUG_
    	for(uint8_t j=0; j < Rx_Fifo_Buffer[0] + 2; j++){
    		std::cout<<std::hex<<(int)Rx_Fifo_Buffer[j];
    	}
    	std::cout<<std::endl;
#endif
    }


    template <class T>
    T mrf24j40::getConf(const char* fileName, std::string key, T defaultValue){
    	T value;
    	try{
    		// Open file
    			std::ifstream infile( fileName );
    			// check if file was successfully opened
    			if(!infile){
    				//could not open file throw exception
    				throw(std::runtime_error( "Cannot open Config file. "));
    			}
    			// declare string buffer for line to be read from file
    			std::string line;

    			// read line from file
    			while( std::getline(infile, line)){
    				//pass line read from file into string stream
    				std::istringstream is_line(line);
    				// extract value from string stream
    				std::string buff;
    				if(is_line >> buff >> value){
    #ifdef _DEBUG_
    					std::cout<<"#---"<<buff <<" -> "<< value<<std::endl;
    #endif

    					// Check if key matches buffer
    					if(buff == key){
    						//check if file is open and close file if open before returning
    						if(infile){
    							infile.close();
    						}
    #ifdef _DEBUG_
    						std::cout<<key <<" -> "<< value<<std::endl;
    #endif
    						return value;
    					}


    				}

    			}
    			if(infile){
    				infile.close();
    			}
    			throw(std::runtime_error( "Error getting integer from conf file. "));
    	}catch(const std::exception& ex){
    		std::cerr << ex.what() << "\nFatal error" << std::endl;

    	}
    	return defaultValue;
    }












    mrf24j40::mrf24j40(const char* fileName): Spi(0,0,8,4000000,5){
//    	for(uint8_t i =0x00; i < 0x0F;i++){
//    		secKey[i] = i;
//    	}
    	if(!begin(0)){
    					std::cout<<"ERROR!!!!"<<std::endl;
    			}
    			reset_pin=getConf<int>(fileName,"reset_pin",25);
    			interrupt_pin=getConf<int>(fileName,"interrupt_pin",24);

    			gpio.exportGPIO(reset_pin);
    			gpio.pinMode(reset_pin,"out");
    			gpio.digitalWrite(reset_pin,1);

    			init();

    			gpio.exportGPIO(interrupt_pin);
    			gpio.pinMode(interrupt_pin,"in");
    			if(getConf<std::string>(fileName,"interrupt_edge","rising") == "rising"){
    				std::cout<<"rising"<<std::endl;
    				gpio.setEdge(interrupt_pin,RISING);
    				writeLongAddress(MRF_SLPCON0,0x02);
    			}else{
    				gpio.setEdge(interrupt_pin,RISING);
    				SLPCON0.val=readLongAddress(MRF_SLPCON0);
    				SLPCON0.bits.INTEDGE=0;
    				writeLongAddress(MRF_SLPCON0,SLPCON0.val);
    			}


    			setChannel(getConf<int>(fileName,"channel",11));
    			std::string buff = getConf<std::string>(fileName,"pan_id","0xFFFF");
    			setPanId(std::stoul(buff, nullptr, 16));
    			buff = getConf<std::string>(fileName,"short_address","0xFFFF");
    			setNodeShortAddress(std::stoul(buff, nullptr, 16));

    			if("true" == getConf<std::string>(fileName,"pan_coord","true")){
    				makePANCoord();
    			}

    			if("true" == getConf<std::string>(fileName,"coord","true")){
    				makeCoord();
    			}

    			if("true" == getConf<std::string>(fileName,"auto_acknowledge","true")){
    				enableAutoAcknowledge();
    			}

    			buff = getConf<std::string>(fileName,"cca_mode","energy_detect");

    			BBREG2.val = readShortAddress(MRF_BBREG2);
    			if("energy_detect" == buff){
    				BBREG2.bits.CCAMODE = 0x2; //Set CCA mode to Energy Detect.
    				 writeShortAddress(MRF_BBREG2,BBREG2.val );
    			}else if("carrier_sense" == buff){
    				BBREG2.bits.CCAMODE = 0x1; //Set CCA mode to Carrier sense.
    				writeShortAddress(MRF_BBREG2,BBREG2.val );
    			}else{
    				BBREG2.bits.CCAMODE = 0x3; //Set CCA mode to Carrier sense & Energy Detect.
    				writeShortAddress(MRF_BBREG2,BBREG2.val );
    			}


    			buff = getConf<std::string>(fileName,"cca_carrier_threshold","0x0E");
    			BBREG2.val = readShortAddress(MRF_BBREG2);
    			BBREG2.bits.CCACSTH = std::stoul(buff, nullptr, 16);
    			writeShortAddress(MRF_BBREG2,BBREG2.val);

    			buff = getConf<std::string>(fileName,"cca_energy_threshold","0x60");
    			CCAEDTH.val = std::stoul(buff, nullptr, 16);
    			writeShortAddress(MRF_CCAEDTH,CCAEDTH.val);
    			txBusy=false;


// comment

    }

    void mrf24j40::Interrupt_Handler(int fd){
    	read(fd,NULL,1);
    	INTSTAT.val = readShortAddress(MRF_INTSTAT);
    	TXSTAT.val = readShortAddress(MRF_TXSTAT);
    	// check Recieve interrupt flag
    	if(INTSTAT.bits.RXIF){
    		recieve();
    	}
    	// Tx interrupt flag
    	if(INTSTAT.bits.TXNIF){
    		TXSTAT.val=readShortAddress(MRF_TXSTAT);
    		if(TXSTAT.bits.TXNSTAT == 1){
    			std::cerr<<"Transimission failed!"<<std::endl;
    			std::cerr<<"\tRetry Count : "<<std::dec<<(int)TXSTAT.bits.TXNRETRY<<std::endl;
    			std::cerr<<"\tCCA : ";
    			if(TXSTAT.bits.CCAFAIL == 1){
    				std::cerr<<"Channel Busy"<<std::endl;
    			}else{
    				std::cerr<<"Successful"<<std::endl;
    			}
    		}
    		txBusy=false;
    	}

    	//Security Interrupt flag
    	if(INTSTAT.bits.SECIF){

    	}

    	//Wake Interrupt
    	if(INTSTAT.bits.WAKEIF){

    	}

    	//tx GTS1 fifo Interrupt
    	if(INTSTAT.bits.TXG1IF){

    	}

    	//tx GTS2 Interrupt
    	if(INTSTAT.bits.TXG2IF){

    	}

    	// Half symbol Interrupt
    	if(INTSTAT.bits.HSYMTMRIF){

    	}

    	//Sleep interrupt
    	if(INTSTAT.bits.SLPIF){

    	}

    }

    uint8_t mrf24j40::sendData(uint8_t * buffer){
    	struct _packet txPacket;

    	//Check if tx is busy
    	//read TXNCON reg
    	TXNCON.val = readShortAddress(MRF_TXNCON);  // no sure if this works correctly
    	//if bit TXNTRIG is 1 then tx is still busy
    	if(txBusy){
    		return 0;
    	}
    	txBusy = true;
    	//check if buffer is NULL
    	if(buffer == nullptr){
    		// buffer is NULL print error and return -1
    		std::cerr<<"mrf24j40::sendData(uint8_t*): recieved NULL buffer"<<std::endl;
    		return -1;
    	}

    	//get header length from buffer
    	txPacket.headerLength = buffer[0];
    	//get frame length from buffer
    	txPacket.frameLength = buffer[1];
    	//check if buffer is to large by check if the value of the FRAME_LENGTH byte is larger/ equal than 128
    	if(txPacket.frameLength >= 128){
    		// buffer is to large for txfifo print error and return -1
    		std::cerr<<"mrf24j40::sendData(uint8_t*): buffer to larger than txFifo"<<std::endl;
    		return -1;
    	}
    	//TX normal fifo transmission 		----> TODO: GTS implementation
    	//set TXCON bit to match frame control field
    	//get frame control from buffer
    	//lsb
    	txPacket.frameControl.bytes.lsb = buffer[2];
    	//msb
    	txPacket.frameControl.bytes.msb = buffer[3];

    	// If security is enabled encrypt packet
    	if(txPacket.frameControl.bits.securityEnabled == 1){
    		//load security key
    		setTxNormalFifoSecurityKey(secKey);
    		//select security suit
    		selectSecuritySuit(0x01);


    	}
    	//get security enable bit and encrypt packet if set to 1
    	TXNCON.bits.TXNSECEN = txPacket.frameControl.bits.securityEnabled;
    	//get acknowledge request bit
    	TXNCON.bits.TXNACKREQ = txPacket.frameControl.bits.ackRequest;
    	//get intra pan bit (is the transmission inside the same pan or to another pan)(inverse to IEEE802154)
    	TXNCON.bits.INDIRECT = !txPacket.frameControl.bits.intraPan;
    	//get frame pending bit
    	TXNCON.bits.FPSTAT = txPacket.frameControl.bits.framePending;

    	//TODO: Security Implementation

    	//write buffer to tx normal Fifo
    	for(uint8_t i =0; i < txPacket.frameLength+2;i++){
    			writeLongAddress(MRF_TXFIFO + i,buffer[i]);
    	}

    	//set Tx trigger to 1
    	TXNCON.bits.TXNTRIG = 1;

    	//start transmission by writing TXNCON reg
    	writeShortAddress(MRF_TXNCON,TXNCON.val);


    	return 0;




    }

    void mrf24j40::setTxNormalFifoSecurityKey(uint8_t* key){
    	//Security key for Tx Normal Fifo is from address 0x280 to 0x28F and is 16Bytes long
    	//load key into register
    	for(uint16_t i = 0x280; i <= 0x28F; i++){
    		writeLongAddress(i,i-0x280);
    	}
    }

    void mrf24j40::selectSecuritySuit(uint8_t suit){
    	//ensure suit is a valid selection  ( 0x00 to 0x07)
    	//0x00 None
    	//0x01 AES-CTR
    	//0x02 AES-CCM-128
    	//0x03 AES-CCM-64
    	//0x04 AES-CCM-32
    	//0x05 AES-CBC-MAC-128
    	//0x06 AES-CBC-MAC-64
    	//0x07 AES-CBC-MAC-32
    	suit &= 0x07;
    	SECCON0.val=readShortAddress(MRF_SECCON0);
    	SECCON0.bits.TXNCIPHER=suit;
    	writeShortAddress(MRF_SECCON0,SECCON0.val);
    }

}







#endif
