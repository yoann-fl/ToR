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
    uint8_t tokenFrameBuffer[17];    // Token frame buffer
    uint8_t addressSource;
    uint8_t sapiSource;    
    uint8_t addressDest;
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
            addressSource = qPtr[0] & 120; //binary to keep the address bits
            sapiDest = qPtr[1] & 7; //binary to keep only the 3 LSB which define the SAPI
            addressDest = qPtr[1] & 120; //binary to keep the address bits
            
            if(addressSource == gTokenInterface.myAddress){
                //DATABACK
                
                //------------------------------------------------------------------------
                // QUEUE PHYs SEND    (send ptr on received frame to PHY Sender)
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
            else if(addressDest == gTokenInterface.myAddress){
                
            //read bit = 1
            //qPtr[4] = qPtr[4] or 2;
            qPtr[4] = qPtr[4] | 2;
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
                

 

            //------------------------------------------------------------------------
            // QUEUE CHATr & TIMEr SEND    
            //------------------------------------------------------------------------
            queueMsg.type = DATA_IND;
            queueMsg.anyPtr = qPtr;
            queueMsg.addr = addressSource;
            queueMsg.sapi = sapiSource;
            retCode = osMessageQueuePut(
                    queue_chatR_id,
                    &queueMsg,
                    osPriorityNormal,
                    0);
            CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);    
            retCode = osMessageQueuePut(
                    queue_timeR_id,
                    &queueMsg,
                    osPriorityNormal,
                    0);
            CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);        
            //-------------------------------------------------------------------------
            
            //check le checksum
            
            //ACK bit = 1 
            qPtr[4] = qPtr[4] | 1;

 

            }
        }            
    }
}