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
			queue_macR_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
		qPtr = queueMsg.anyPtr;
		
		if(qPtr[0] == TOKEN_TAG)
		{
			//------------------------------------------------------------------------
			// QUEUE SEND	(send received frame to Mac Sender)
			//------------------------------------------------------------------------
			queueMsg.type = DATABACK;
			queueMsg.anyPtr = qPtr;
			retCode = osMessageQueuePut(
					queue_macS_id,
					&queueMsg,
					osPriorityNormal,
					0);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
		}
		
		//------------------------------------------------------------------------
		// QUEUE SEND	(send received frame to physical receiver)
		//------------------------------------------------------------------------
		/*queueMsg.type = TO_PHY;
		queueMsg.anyPtr = tokenFrameBuffer;
		retCode = osMessageQueuePut(
				queue_phyS_id,
				&queueMsg,
				osPriorityNormal,
				0);
		CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		*/								
	}
}


