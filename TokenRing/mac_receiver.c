//////////////////////////////////////////////////////////////////////////////////
/// \file mac_receiver.c
/// \brief MAC receiver thread
/// \author Pascal Sartoretti (sap at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"
#include <stdio.h>
#include <string.h>
#include "main.h"

//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacReceiver(void *argument)
{
    struct queueMsg_t queueMsg;        // queue message
		struct queueMsg_t queueAppMsg;        // queue message app
    uint8_t tokenFrameBuffer[17];    // Token frame buffer
    uint8_t addressSource;
    uint8_t sapiSource;    
    uint8_t addressDest;
		uint32_t dataLength;
    uint8_t sapiDest;
    uint8_t * msg;
    uint8_t * qPtr;
    size_t    size;
    osStatus_t retCode;
    for(;;)
    {
        //----------------------------------------------------------------------------
        // QUEUE READ                                        
        //----------------------------------------------------------------------------
        retCode = osMessageQueueGet(     
            queue_macR_id,
            &queueMsg,
            NULL,
            osWaitForever);     
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);                
        qPtr = queueMsg.anyPtr;
        
        if(qPtr[0] == TOKEN_TAG)
        {
            //------------------------------------------------------------------------
            // QUEUE SEND    (send received frame to Mac Sender)
            //------------------------------------------------------------------------
            queueMsg.type = TOKEN;
            queueMsg.anyPtr = qPtr;
            retCode = osMessageQueuePut(
                    queue_macS_id,
                    &queueMsg,
                    osPriorityNormal,
                    0);
            CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);                
        }
        else //Frame is a data
        {
            sapiSource = qPtr[0] & 7; //binary to keep only the 3 LSB which define the SAPI
            addressSource = (qPtr[0] & 120) >> 3; //binary to keep the address bits
            sapiDest = qPtr[1] & 7; //binary to keep only the 3 LSB which define the SAPI
            addressDest = (qPtr[1] & 120) >> 3; //binary to keep the address bits
            dataLength = qPtr[2];
            if((addressDest == gTokenInterface.myAddress)||(addressDest==15)){
              if((qPtr[3+dataLength]&0xFC)>>2 == (doChecksum(qPtr,qPtr[2]+3) & 0x3F))
							{	
								qPtr[3+dataLength] = qPtr[3+dataLength]|3;	//ack bit = 1	read bit = 1
								
                queueMsg.sapi = sapiSource;
                queueMsg.sapi = addressSource;
                  
								
								msg = osMemoryPoolAlloc(memPool,osWaitForever);
								memcpy(msg,&qPtr[3],dataLength);
								msg[dataLength] = '\0';
								queueAppMsg.type = DATA_IND;
								queueAppMsg.anyPtr = msg;
								queueAppMsg.addr = addressSource;
								queueAppMsg.sapi = sapiSource;
								
								// VERIFY IF IS FOR CHAT OR TIME WITH DEST SAPI
								if(sapiSource == CHAT_SAPI)
								{
									//------------------------------------------------------------------------
									// QUEUE CHATr 
									//------------------------------------------------------------------------
									retCode = osMessageQueuePut(
													queue_chatR_id,
													&queueAppMsg,
													osPriorityNormal,
													osWaitForever);
									CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);   
								}
								
								//------------------------------------------------------------------------
								//  TIMEr SEND    
								//------------------------------------------------------------------------
								if(sapiSource == TIME_SAPI)
								{
									retCode = osMessageQueuePut(
												queue_timeR_id,
												&queueAppMsg,
												osPriorityNormal,
												osWaitForever);
								CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);    	
								}

								if(addressSource == gTokenInterface.myAddress)
								{
									queueMsg.type = DATABACK;
									//--------------------------------------------------------------------
									// QUEUE SEND	(send message to mac sender)
									//--------------------------------------------------------------------
									retCode = osMessageQueuePut(
										queue_macS_id,
										&queueMsg,
										osPriorityNormal,
										osWaitForever);
									CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
								}
								else
								{
									//------------------------------------------------------------------------
									// QUEUE PHYs SEND    (send ptr on received frame to MAC Sender)
									//------------------------------------------------------------------------
									queueMsg.type = TO_PHY;
									queueMsg.anyPtr = qPtr;
									retCode = osMessageQueuePut(
                        queue_phyS_id,
                        &queueMsg,
                        osPriorityNormal,
                        0);
									CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
								}
							}
							else
							{ //checksum is wrong
								//ACK bit = 0
								//READ bit = 1
								qPtr[3+dataLength] = qPtr[3+dataLength]|2;	//ack bit = 1	
								if(addressSource == gTokenInterface.myAddress){
									//send DataBack
									//------------------------------------------------------------------------
									// QUEUE DataBack SEND
									//------------------------------------------------------------------------
									queueMsg.type = DATABACK;
									queueMsg.anyPtr = qPtr;
									queueMsg.sapi = sapiSource;
									queueMsg.sapi = addressSource;
									retCode = osMessageQueuePut(
													queue_macS_id,
													&queueMsg,
													osPriorityNormal,
													0);
									CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);        
									//----------------------------------------------------------------------------
								}
								else{
									//-------------------------------------------------------------------------
									// QUEUE PHYs SEND    (send ptr on received frame to PHY Sender)
									//------------------------------------------------------------------------
									queueMsg.type = TO_PHY;
									queueMsg.anyPtr = qPtr;
									retCode = osMessageQueuePut(
													queue_phyS_id,
													&queueMsg,
													osPriorityNormal,
													0);
									CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);        
									//----------------------------------------------------------------------------
								}
							}
            }
            else{	//TO_PHY , message is not for us					

							if(addressSource == gTokenInterface.myAddress){
								//send DataBack
							//------------------------------------------------------------------------
							// QUEUE DataBack SEND    (send ptr on received frame to MAC Sender)
							//------------------------------------------------------------------------
							queueMsg.type = DATABACK;
							queueMsg.anyPtr = qPtr;
							queueMsg.sapi = sapiSource;
							queueMsg.sapi = addressSource;
							retCode = osMessageQueuePut(
											queue_macS_id,
											&queueMsg,
											osPriorityNormal,
											0);
							CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
							}
							else
							{
								//------------------------------------------------------------------------
								// QUEUE PHYs SEND    (send ptr on received frame to PHY Sender)
								//------------------------------------------------------------------------
								queueMsg.type = TO_PHY;
								queueMsg.anyPtr = qPtr;
								retCode = osMessageQueuePut(
												queue_phyS_id,
												&queueMsg,
												osPriorityNormal,
												0);
								CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);        
								//----------------------------------------------------------------------------
							}
						}						
        }            
    }
}

