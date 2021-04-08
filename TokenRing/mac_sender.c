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
	uint8_t tokenFrameBuffer[17];				// Token frame buffer
	uint8_t * msg;
	uint8_t * qPtr;
	uint8_t * bPtr;
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
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);					
				bPtr = queueMsgBuffer.anyPtr;
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
					tPtr = queueAllMsg.anyPtr;
					queueMsgBuffer.type = TO_PHY;
					//------------------------------------------------------------------------
					// QUEUE SEND	-- send the message to phy
					//------------------------------------------------------------------------
					retCode = osMessageQueuePut(
						queue_phyS_id,
						&queueMsgBuffer,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);								
				}
				break;
				
			case DATABACK:
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
				break;
			case DATA_IND:
				break;
			default:
				break;
		}
	}
}
