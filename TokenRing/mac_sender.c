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
	struct queueMsg_t queueMsg;		// queue message
	uint8_t tokenFrameBuffer[17];	// Token frame buffer
	uint8_t * msg;
	uint8_t * qPtr;
	size_t	size;
	osStatus_t retCode;
	for(;;)
	{
		//----------------------------------------------------------------------------
		// QUEUE READ										
		//----------------------------------------------------------------------------
		retCode = osMessageQueueGet( 	
			queue_macS_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
		qPtr = queueMsg.anyPtr;
		
		if(queueMsg.type == NEW_TOKEN)
		{
			tokenFrameBuffer[0] = TOKEN_TAG;
			
			// Update ready List
			for(int i = 1; i < 16; i++)
			{
				tokenFrameBuffer[i] = 0;
			}
			queueMsg.anyPtr = tokenFrameBuffer;
		}
		else if(queueMsg.type == DATABACK)
		{
			for(int i = 1; i < 16; i++)
			{
				gTokenInterface.station_list[i-1] = qPtr[i];
			}
			qPtr[MYADDRESS] = 10;
			queueMsg.anyPtr = qPtr;
		}
		
		if(retCode == osOK)
		{
			//------------------------------------------------------------------------
			// QUEUE SEND	(send received frame to physical receiver)
			//------------------------------------------------------------------------
			queueMsg.type = TO_PHY;
			retCode = osMessageQueuePut(
				queue_phyS_id,
				&queueMsg,
				osPriorityNormal,
				0);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
		}										
	}
}