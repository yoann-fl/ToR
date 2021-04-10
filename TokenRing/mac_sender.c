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
	struct queueMsg_t queueAllMsg;		// queue message token
	struct queueMsg_t queueMsgBuffer;		// queue message storage
	uint8_t tokenFrameBuffer[17];	// Token frame buffer
	uint8_t * msg;
	uint8_t * qPtr;
	uint8_t * bPtr;
	size_t	size;
	osStatus_t retCodeToken;
	osStatus_t retCodeBuffer;
	
	for(;;)
	{
		//----------------------------------------------------------------------------
		// QUEUES READ										
		//----------------------------------------------------------------------------
		
		// Read the MAC Sender first queue
		retCodeToken = osMessageQueueGet( 	
			queue_macS_id,
			&queueAllMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCodeToken,__LINE__,__FILE__,CONTINUE);				
		qPtr = queueAllMsg.anyPtr;
		
		// Manage NEW_TOKEN Type
		if(queueAllMsg.type == NEW_TOKEN)
		{
			tokenFrameBuffer[0] = TOKEN_TAG;
			
			// Init ready list
			for(int i = 1; i < 16; i++)
			{
				tokenFrameBuffer[i] = 0;
			}
			queueAllMsg.anyPtr = tokenFrameBuffer;
		}
		
		// Manage DATABACK Type
		else if(queueAllMsg.type == DATABACK)
		{
			// Update ready list
			for(int i = 1; i < 16; i++)
			{
				gTokenInterface.station_list[i-1] = qPtr[i];
			}
			qPtr[MYADDRESS] = 10;		// Chat and time are ON
			queueAllMsg.anyPtr = qPtr;  //pk?
		}
		// Manage START Type
		else if(queueAllMsg.type == START)
		{
			qPtr[MYADDRESS] = 10;		// Turn on Chat (SAP_1 = 1) and Time (SAP_3 = 1)
			queueAllMsg.anyPtr = qPtr;
		}
		
		// Manage STOP Type
		else if(queueAllMsg.type == STOP)
		{
			qPtr[MYADDRESS] = 8;		// Turn off Chat (SAP_1 = 0) and time still ON
			queueAllMsg.anyPtr = qPtr;
		}
		
		// Put in the buffer if a message was get from the queue is not a Token data back or new token
		if((retCodeToken == osOK) && !(queueAllMsg.type == NEW_TOKEN || queueAllMsg.type == DATABACK))  
		{
			//------------------------------------------------------------------------
			// QUEUE SEND	(send received frame to MAC sender's buffer)
			//------------------------------------------------------------------------
			queueMsgBuffer.type = TO_BUFF;
			//queueMsgBuffer.anyPtr = queueAllMsg.anyPtr;  oui/non????
			retCodeBuffer = osMessageQueuePut(
				queue_macSBuffer_id,
				&queueMsgBuffer,
				osPriorityNormal,
				0);
			CheckRetCode(retCodeToken,__LINE__,__FILE__,CONTINUE);
		}
		
		else if((retCodeToken == osOK) && (queueAllMsg.type == NEW_TOKEN || queueAllMsg.type == DATABACK))
		{
			
			// Read the MAC Sender storage queue
			retCodeBuffer = osMessageQueueGet( 	
				queue_macSBuffer_id,
				&queueMsgBuffer,
				NULL,
				osWaitForever); 	
			CheckRetCode(retCodeToken,__LINE__,__FILE__,CONTINUE);				
			bPtr = queueMsgBuffer.anyPtr; //on fait quoi de bPtr ?
			
			//------------------------------------------------------------------------
			// QUEUE SEND	(send received frame to physical sender)
			//------------------------------------------------------------------------
			queueMsgBuffer.type = TO_PHY;
			retCodeToken = osMessageQueuePut(
				queue_phyS_id,
				&queueMsgBuffer,
				osPriorityNormal,
				0);
			CheckRetCode(retCodeToken,__LINE__,__FILE__,CONTINUE);
		}
	}
}
