//////////////////////////////////////////////////////////////////////////////////
/// \file mac_sender.c
/// \brief MAC sender thread
/// \author Pascal Sartoretti (pascal dot sartoretti at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "ext_led.h"


//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacSender(void *argument)
{
	struct queueMsg_t queueAllMsg;			// queue message token
	struct queueMsg_t queueMsgBuffer;		// queue message storage
	struct queueMsg_t msgList;					// queue meesage list to LCD
	uint8_t dataFrameLength;
	uint8_t tokenFrameBuffer[17];				// Token frame buffer
	uint8_t * originalMsg;
	uint8_t * msg;
	uint8_t * msgCpy = 0;
	uint8_t * qPtr;
	uint8_t * tPtr = 0;
	size_t	size;
	osStatus_t retCode;
	bool_t isListChanged;
	
	for(;;)
	{
		//----------------------------------------------------------------------------
		// QUEUES READ										
		//----------------------------------------------------------------------------
		
		// Read the MAC Sender first queue
		retCode = osMessageQueueGet( 	
			queue_macS_id,
			&queueAllMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
		qPtr = queueAllMsg.anyPtr;
		switch(queueAllMsg.type)
		{
			case NEW_TOKEN:
				tokenFrameBuffer[0] = TOKEN_TAG;
			
				// Init ready list
				for(int i = 1; i < 16; i++)
				{
					tokenFrameBuffer[i] = 0;
				}
				queueAllMsg.anyPtr = tokenFrameBuffer;
				queueAllMsg.type = TO_PHY;
				//--------------------------------------------------------------------------
				// QUEUE SEND	(send message to physical sender)
				//--------------------------------------------------------------------------
				retCode = osMessageQueuePut(
					queue_phyS_id,
					&queueAllMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				break;
				
			case START:
				gTokenInterface.connected = TRUE;
				gTokenInterface.station_list[MYADDRESS] = 10; // Turn on Chat (SAP_1 = 1) and Time (SAP_3 = 1)
				break;
			case STOP:
				gTokenInterface.connected = FALSE;
				gTokenInterface.station_list[MYADDRESS] = 8;		// Turn off Chat (SAP_1 = 0) and time still ON
				Ext_LED_Off(3);
				break;
			case TOKEN:
				isListChanged = FALSE;
				// Update ready list
				for(int i = 1; i < 16; i++)
				{
					if(gTokenInterface.station_list[i-1] != qPtr[i])
					{
						isListChanged = TRUE;
					}
					gTokenInterface.station_list[i-1] = qPtr[i];
				}
				if(gTokenInterface.connected == TRUE)
				{
					qPtr[MYADDRESS] = 10;		// Chat and time are ON
				}
				else
				{
					qPtr[MYADDRESS] = 8;		// Chat OFF and time ON
				}
				gTokenInterface.station_list[MYADDRESS] = qPtr[MYADDRESS+1];
				
				if(isListChanged != FALSE)
				{
					msgList.type = TOKEN_LIST;
					//------------------------------------------------------------------------
					// QUEUE SEND	-- send the message list to lcd
					//------------------------------------------------------------------------
					retCode = osMessageQueuePut(
						queue_lcd_id,
						&msgList,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
				}
				//--------------------------------------------------------------------------
				// QUEUE READ	-- Read the buffer of messages without wait									
				//--------------------------------------------------------------------------
				retCode = osMessageQueueGet( 	
					queue_macSBuffer_id,
					&queueMsgBuffer,
					NULL,
					0); 					
				qPtr = queueMsgBuffer.anyPtr;
				if(retCode != osOK)	// If there is no data, send the token
				{
					queueAllMsg.type = TO_PHY;
					//------------------------------------------------------------------------
					// QUEUE SEND	-- send the token to phy
					//------------------------------------------------------------------------
					retCode = osMessageQueuePut(
						queue_phyS_id,
						&queueAllMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
				}
				else
				{
					tPtr = queueAllMsg.anyPtr;		// Save token pointer
					originalMsg = qPtr;
					// Memory Alloc
					msgCpy = osMemoryPoolAlloc(memPool, osWaitForever);
					memcpy(msgCpy, qPtr, 4+qPtr[2]);
					queueMsgBuffer.anyPtr = msgCpy;
					//------------------------------------------------------------------------
					// QUEUE SEND	-- send the message to phy
					//------------------------------------------------------------------------
					queueMsgBuffer.type = TO_PHY;
					
					retCode = osMessageQueuePut(
						queue_phyS_id,
						&queueMsgBuffer,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);								
				}
				break;
				
			case DATABACK:
				
				dataFrameLength = qPtr[2];
				// is R bit = 1 ?
				if((qPtr[3+dataFrameLength] & 2) == 2)
				{
					// is A bit = 1 ?
					if(qPtr[3+dataFrameLength] & 1 == 1)
					{
						// Free message
						retCode = osMemoryPoolFree(memPool, msgCpy);
						CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
						
						// Free DATABACK
						retCode = osMemoryPoolFree(memPool, qPtr);
						CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
						
						//------------------------------------------------------------------------
						// QUEUE SEND	-- send the token again
						//------------------------------------------------------------------------

						queueAllMsg.anyPtr = tPtr;
						queueAllMsg.type = TO_PHY;
						retCode = osMessageQueuePut(
							queue_phyS_id,
							&queueAllMsg,
							osPriorityNormal,
							osWaitForever);
						CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
					}
					else	// R = 1, A = 0
					{
						// Free message
						retCode = osMemoryPoolFree(memPool, msgCpy);
						CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
						
						// Free DATABACK
						retCode = osMemoryPoolFree(memPool, qPtr);
						CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
						
						// Alloc Memory
						queueAllMsg.anyPtr = osMemoryPoolAlloc(memPool, osWaitForever);
						memcpy(queueAllMsg.anyPtr, msgCpy, 4+qPtr[2]);
						
						// Set R & A to 0
						uint8_t newStatus = qPtr[3+dataFrameLength] & 0xFC;						
						qPtr[3+dataFrameLength] = newStatus; 		// Ack = 0, Read = 0;
						
						// Send Data
						queueAllMsg.type = TO_PHY;
						retCode = osMessageQueuePut(
							queue_phyS_id,
							&queueAllMsg,
							osPriorityNormal,
							osWaitForever);
						CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
						
						//------------------------------------------------------------------------
						// QUEUE SEND	-- send the token again
						//------------------------------------------------------------------------

						queueAllMsg.anyPtr = tPtr;
						queueAllMsg.type = TO_PHY;
						retCode = osMessageQueuePut(
							queue_phyS_id,
							&queueAllMsg,
							osPriorityNormal,
							osWaitForever);
						CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
					}
				}
				else // R = 0, A = 0
				{
					// Create Error Message
					queueAllMsg.addr = qPtr[1] >> 3;
					queueAllMsg.type = MAC_ERROR;
					
					// Free DATABACK
					retCode = osMemoryPoolFree(memPool,qPtr);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
					sprintf((char*)msg, "Error in MAC Layer : Message was not read by the station : %d", queueAllMsg.addr+1);
					
					// Send Error to LCD
					queueAllMsg.anyPtr = msg;
					queueAllMsg.addr = gTokenInterface.myAddress;
						retCode = osMessageQueuePut(
							queue_lcd_id,
							&queueAllMsg,
							osPriorityNormal,
							osWaitForever);
						CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
					
					// Resend Token
					queueAllMsg.anyPtr = tPtr;
						queueAllMsg.type = TO_PHY;
						retCode = osMessageQueuePut(
							queue_phyS_id,
							&queueAllMsg,
							osPriorityNormal,
							osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				}
				
				break;
			case DATA_IND:
				//------------------------------------------------------------------------
				// DataBuffer SEND    -- send the Data in the waiting queue
				//------------------------------------------------------------------------
			
				// Memory Alloc
				msg = osMemoryPoolAlloc(memPool,osWaitForever);
			
				// Create data frame
				size = strlen(queueAllMsg.anyPtr);
				msg[0] = (gTokenInterface.myAddress << 3) | (queueAllMsg.sapi);
				msg[1] = ((queueAllMsg.addr) << 3) | queueAllMsg.sapi;
				msg[2] = size;
				memcpy(&msg[3], queueAllMsg.anyPtr, size);
			// to do Check Checksum for Broadcast => If add == 16, 2 lsb à 1, | 0x03. Else ->
				msg[3+size] = doChecksum(msg, size+3) << 2;
			
				// Free DATA_IND
				retCode = osMemoryPoolFree(memPool,queueAllMsg.anyPtr);
			
				// Put on buffer
				queueMsgBuffer.type = TO_BUFF;
				queueMsgBuffer.anyPtr = msg;
				retCode = osMessageQueuePut(
						queue_macSBuffer_id,
						&queueMsgBuffer,
						osPriorityNormal,
						0);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				break;
			default:
				break;
		}
	}
}
