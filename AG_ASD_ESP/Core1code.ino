//Core1:ASD Core
void Core1code( void * pvParameters ){
  //Serial.print("Task1 running on core ");
  //Serial.println(xPortGetCoreID());


//References
void ASD_Host_Sect_Submit(byte newSect[]);

  sectionOut[0] = 0x03; 
  sectionOut[1] = 0;
  sectionOut[2] = 0;
  sectionOut[3] = 0x80;
   
for(;;){
  
 if(gotInit == false){
    firstInit = true;
    if ((millis() - lastInitTime) > 1000){
      ASD_Host_Init_Request();
      DbgSerial.println("Waiting for Response from ASD Client"); 
     }
    
  }
 
 else{
  if (firstInit == true){
    ASD_Host_Rate_Request();
    delay(50);
    ASD_Host_Init_Config();
    delay(100);
    ASD_Host_Sect_Request();
    firstInit = false;
   }
   
  if (((millis() - lastReqTime) >500) && gotInit == true){
      switch (steps){
        case 0:
        case 1:
          ASD_Host_Rate_Request();
          break;
        case 2:
          ASD_Host_Sect_Request();
          break;
        case 3:
          ASD_Host_Rate_Request();
          break;
        case 4:  
          ASD_Host_Rate_Request();
          steps = 0; 
          break;
        default:
          steps = 0; 
          break;
       }// end switch
       steps++;
    }
  if (sectChange == true){
    //Save previous state
    sectionSent[0] = sectionOut[0];
    sectionSent[1] = sectionOut[1];
    sectionSent[2] = sectionOut[2];
    sectionSent[3] = sectionOut[3];
    
     ASD_Host_Sect_Submit(sectionOut);
     
     for(int temp=0; temp < 2; temp++){
       ASD_Host_Sect_Request();
       delay(50); 
     }
   }
  if (lastSectReqTime > lastSectTime) sectChange = false; // proof if Sections arrived
  
  for(int temp=0; temp < 4; temp++){
     byte dd = sectionOverride[temp]; 
     sectionOverride[temp] = sectionSent[temp] ^ sectionReturn[temp];  //xor  
     if (dd > sectionOverride[temp] ) {
      //Serial.println(" OR down");
      sectionOverrideOld[temp] = dd;
      gotBackSect=true;
     }
   }
  
  
  if (((millis() - lastInitTime) > 3600) && gotInit == true ){  
    ASD_Host_Init_Request();
   }
  if ((millis() - lastInitResponse) > 10000) gotInit = false;
 
 }//end else
 
  //------------------------------------------------------------------------------------------
  //Read serial ASD data

  while (Serial1.available()) 
    { 
     prevbyte=c; 
     c = Serial1.read();
     //Serial.print(c,HEX);
     //Serial.print(" ");
     
     if ((c == startByte) && (prevbyte != escMask) )      //if start character is $=0x02 start new sentence
       {
         newSentence = true; // only store sentence, if time is over
         i = 0;
       }
    
     if ((c == stopByte) && (prevbyte != escMask) && newSentence)   //if character is stopbyte 
       { 
         ASDBuffer[i++] = stopByte;  //last-1 byte
         
         switch (ASDBuffer[1]){
          case 0x00:                                  //Init Response
            if (ASDBuffer[2] == 0x03) {
               gotInit = true;
               lastInitResponse = millis();
            }
            
            break;
          case 0x55:                                  //Section Response
            if (ASDBuffer[2] == 0x01){
              //section Bytes = Pos 7,8,9,10  
              byte Pos = 6;
              byte Prev =0;
              //Serial.print("Got TB:  ");
              
              for (int temp = 0; temp < 4; temp++){
                if (ASDBuffer[Pos] == escMask) Pos++; 
                //sectionRetOld[temp] = sectionReturn[temp]; //save previous Value
                sectionReturn[temp] = ASDBuffer[Pos];   
                //Serial.print(ASDBuffer[Pos],HEX);
                //Serial.print(" ");
                Pos++;
              } 
              ASD_CalcRate();
            }
            break;
          case 0x20:{                                  //Rate Response
            //rate Bytes = Pos 9,10,11
            byte Pos = 8;
            for (int temp = 0; temp < 3; temp++){
              if (ASDBuffer[Pos] == escMask) Pos++; 
              rateAct[temp] = ASDBuffer[Pos];   
              Pos++;
              }
            break;}
         default:
            DbgSerial.print("Got unknown response from Client: ");
            for( int temp = 0; temp < i; temp++){  
              DbgSerial.print(ASDBuffer[temp],HEX);
              DbgSerial.print(" ");
             }
            DbgSerial.println();
            break;
         }
        i = 0;
        newSentence = false;
      }
   
     if (newSentence && i < 20) 
       {
        ASDBuffer[i++] = c;
       }
   } // end serial avail.

  delay(1); //wdt
 } // End of (main core1)
} // End of core1code
  

// Subs
//***********************************************************
void ASD_Host_Init_Request(){
 byte crc = 0;
 byte Init[5] = {0x01, 0x03, 0x02, 0x08, 0x01};  

 crc = 0xFF - Init[1] - Init[2] -Init[3] -Init[4];

 Serial1.write(startByte);
 for (int temp=0; temp<5; temp++){
   if (Init[temp] == startByte || Init[temp] == startByte || Init[temp] == escMask) Serial1.write(escMask);
   Serial1.write(Init[temp]);  
 }
 Serial1.write(crc);
 Serial1.write(stopByte);
 //DBG("      INIT Request",1);
 lastInitTime = millis();
}

//***********************************************************
void ASD_Host_Rate_Request(){
  byte crc = 0;
  // Rate Request  startByte, 0x20, escMask, 0x02, 0x03,   0xXX,0x00,   0x00, 0xXX, stopByte;
  //                Start     Rate?                        toolID       Rate! CRC   Stop

  crc= 0xFF - 0x24 - toolID[0] - toolID[1];
  Serial1.write(startByte);
  Serial1.write(0x20);  
  Serial1.write(escMask);
  Serial1.write(0x02);
  Serial1.write(0x03);
  if (toolID[0] == startByte || toolID[0] == stopByte || toolID[0] == escMask) Serial1.write(escMask);
  Serial1.write(toolID[0]);
  if (toolID[1] == startByte || toolID[1] == stopByte || toolID[1] == escMask) Serial1.write(escMask);
  Serial1.write(toolID[1]);
  Serial1.write(0x00);
  Serial1.write(crc);
  Serial1.write(stopByte);
  //DBG("     Rate Request",1);
  lastReqTime = millis();
}

//***********************************************************
void ASD_Host_Sect_Request(){
  byte crc = 0; 

  crc = 0xFF - 0x58 - toolID[0] - toolID[1];
 
  Serial1.write(startByte);
  Serial1.write(0x55);  
  Serial1.write(escMask);
  Serial1.write(0x02);
  Serial1.write(escMask);
  Serial1.write(0x02);
  if (toolID[0] == startByte || toolID[0] == stopByte || toolID[0] == escMask) Serial1.write(escMask);
  Serial1.write(toolID[0]);
  if (toolID[1] == startByte || toolID[1] == stopByte || toolID[1] == escMask) Serial1.write(escMask);
  Serial1.write(toolID[1]);
  Serial1.write(crc);
  Serial1.write(stopByte); 
  //DBG("     Section Request",1);
  lastReqTime = millis();
  lastSectReqTime = millis();
}

//***********************************************************
void ASD_Host_Rate_Submit(unsigned long newRate){
  byte crc = 0; 
  byte byte1, byte2, byte3;
  long temp;
  
 if (newRate < 16){
    byte1 = 0x00;
    byte2 = (newRate - 8) * 16;
    byte3 = 0x41;
  }
 else if (newRate >= 16 && newRate < 32){
    byte1 = 0x00;
    byte2 = newRate * 8;
    byte3 = 0x41;
  }
 else if (newRate >= 32 && newRate < 64){
    byte1 = 0x00;
    byte2 = (newRate - 32) * 4;
    byte3 = 0x42;
  }
 else if (newRate >= 64 && newRate < 128){
    byte1 = 0x00;
    byte2 = newRate * 2;
    byte3 = 0x42;
  }
 else if (newRate >= 128 && newRate < 256){
    byte1 = 0x00;
    byte2 = newRate - 128;
    byte3 = 0x42;
  }
 else if (newRate >= 256 && newRate < 512){
    byte1 = newRate % 2;
    byte2 = (newRate - byte1) >> 1;
    byte1 = byte1 * 0x80;
    byte3 = 0x43;
  }
 else if (newRate >= 512 && newRate < 1024){
    byte1 = newRate % 4;
    byte2 = ((newRate - byte1) - 512) >> 2;
    byte1 = byte1 * 0x40;
    byte3 = 0x44;
  }
 else if (newRate >= 1024 && newRate < 2048){
    byte1 = newRate % 8;
    byte2 = (((newRate - byte1) - 1024) >> 3) + 128;
    byte1 = byte1 * 0x20;
    byte3 = 0x44;
  }
 else if (newRate >= 2048){
    byte1 = newRate % 16;
    byte2 = ((newRate -byte1) - 2048) >> 4;
    byte1 = byte1 * 0x10;
    byte3 = 0x45;
  }

  crc = 0xFF - 0x27 - toolID[0] - toolID[1] - byte1 - byte2 -byte3;

  Serial1.write(startByte);
  Serial1.write(0x20);  
  Serial1.write(0x01);
  Serial1.write(0x07);
  if (toolID[0] == startByte || toolID[0] == stopByte || toolID[0] == escMask) Serial1.write(escMask);
  Serial1.write(toolID[0]);
  if (toolID[1] == startByte || toolID[1] == stopByte || toolID[1] == escMask) Serial1.write(escMask);
  Serial1.write(toolID[1]);
  Serial1.write(0x00);
  Serial1.write(0x00);
  if (byte1 == startByte || byte1 == stopByte || byte1 == escMask) Serial1.write(escMask);
  Serial1.write(byte1);
  if (byte2 == startByte || byte2 == stopByte || byte2 == escMask) Serial1.write(escMask);
  Serial1.write(byte2);
  Serial1.write(byte3);
  Serial1.write(crc);
  Serial1.write(stopByte); 
  //DBG(" Rate Submit",1);
}

//***********************************************************
void ASD_Host_Sect_Submit(byte newSect[]){
  byte crc = 0; 

  crc = 0xFF - 0x5B - toolID[0] - toolID[1] - newSect[0] - newSect[1] - newSect[2] - newSect[3];
 
  Serial1.write(startByte);
  Serial1.write(0x55);  
  Serial1.write(0x01);
  Serial1.write(0x06);
  if (toolID[0] == startByte || toolID[0] == stopByte || toolID[0] == escMask) Serial1.write(escMask);
  Serial1.write(toolID[0]);
  if (toolID[1] == startByte || toolID[1] == stopByte || toolID[1] == escMask) Serial1.write(escMask);
  Serial1.write(toolID[1]);
  for (int temp=0; temp<4; temp++){
    if (newSect[temp] == startByte || newSect[temp] == stopByte || newSect[temp] == escMask) Serial1.write(escMask);
    Serial1.write( newSect[temp] );
  }
  Serial1.write(crc);
  Serial1.write(stopByte); 
  //DBG("   Section Submit",1);
  lastSectTime = millis();
}

//***********************************************************
void ASD_CalcRate(){
  switch (rateAct[2]){
    case 0x41:
     if (rateAct[1] <= 0x7F){
       rateAct_i = (rateAct[1] >> 4) + 8;
      }
     else if (rateAct[1] <= 0xFF){
       rateAct_i = (rateAct[1] >> 3);
      }
    break;
    
    case 0x42:
     if (rateAct[1] <= 0x7F){
       rateAct_i = (rateAct[1] >> 2) + 32;
      }
     else if (rateAct[1] <= 0xFF){
       rateAct_i = (rateAct[1] >> 1);
      }
    break;
      
    case 0x43:
     if (rateAct[1] <= 0x7F){
       rateAct_i = (rateAct[1]) + 128 + (rateAct[0] >> 8);
      }
     else if (rateAct[1] <= 0xFF){
       rateAct_i = (rateAct[1] << 1) + (rateAct[0] >> 7);
      }
    break;
    
    case 0x44:
     if (rateAct[1] <= 0x7F){
       rateAct_i = (rateAct[1] << 2) + (rateAct[0] >> 6) + 512;
      }
     else if (rateAct[1] <= 0xFF){
       rateAct_i = (rateAct[1] << 3) + (rateAct[0] >> 5);
      }
    break;
    
    case 0x45:
     if (rateAct[1] <= 0x7F){
       rateAct_i = (rateAct[1] << 4) + (rateAct[0] >> 4) + 2048;
      }
     else if (rateAct[1] <= 0xFF){
       rateAct_i = (rateAct[1] << 5) + (rateAct[0] >> 3);
      }
    break;

    default:
      rateAct_i = 0;
    break;  
  }
  
}
//***********************************************************
void ASD_Host_Init_Config(){
  byte crc = 0;
 
  crc = 0xFF - 0x25 - 0x02 - 0x02 - toolID[0] - toolID[1];
  Serial1.write(startByte);
  Serial1.write(0x25);  
  Serial1.write(escMask);  
  Serial1.write(0x02);  
  Serial1.write(escMask);  
  if (toolID[0] == startByte || toolID[0] == stopByte || toolID[0] == escMask) Serial1.write(escMask);
  Serial1.write(toolID[0]);
  if (toolID[1] == startByte || toolID[1] == stopByte || toolID[1] == escMask) Serial1.write(escMask);
  Serial1.write(toolID[1]);
  Serial1.write(crc);
  Serial1.write(stopByte);

  delay(50);
  crc = 0xFF - 0x35 - 0x02 - 0x02 - toolID[0] - toolID[1];
  Serial1.write(startByte);
  Serial1.write(0x35);  
  Serial1.write(escMask);  
  Serial1.write(0x02);  
  Serial1.write(escMask);  
  if (toolID[0] == startByte || toolID[0] == stopByte || toolID[0] == escMask) Serial1.write(escMask);
  Serial1.write(toolID[0]);
  if (toolID[1] == startByte || toolID[1] == stopByte || toolID[1] == escMask) Serial1.write(escMask);
  Serial1.write(toolID[1]);
  Serial1.write(crc);
  Serial1.write(stopByte);
  //DBG("      INIT Config 1",1);

}
//***********************************************************
