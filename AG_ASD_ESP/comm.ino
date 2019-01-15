void transmit_AOG(){
  //Send to agopenGPS, 5Hz per second
    
    SC_Serial.print(rateAppliedLPMLeft); //100 x actual!
    SC_Serial.print(",");
    SC_Serial.print(rateAppliedLPMRight); //100 x actual!
    SC_Serial.print(",");
    SC_Serial.print((int)((float)accumulatedCountsLeft / (float)flowmeterCalFactorLeft +
      (float)accumulatedCountsRight / (float)flowmeterCalFactorRight));
    
    SC_Serial.print(",");
    SC_Serial.print(RelayToAOG[1]);
    SC_Serial.print(",");
    SC_Serial.print(RelayToAOG[0]);
    SC_Serial.print(",");
    SC_Serial.print(SectSWOffToAOG[1]);
    SC_Serial.print(",");
    SC_Serial.print(SectSWOffToAOG[0]);
    SC_Serial.print(",");
    SC_Serial.print(SectMainToAOG);
    SC_Serial.println();
    // flush out buffer
    SC_Serial.flush();
    lastAOG_Transmit = millis();
}

//******************************************************************
void receive_AOG(){
  if (SC_Serial.available() > 0 && !isDataFound && !isSettingFound) //find the header,
  { 
    byte temp = SC_Serial.read();
    //Serial.print(temp);
    
    header = tempHeader << 8 | temp;               //high,low bytes to make int
    tempHeader = temp;                             //save for next time
    if (header == 32762) isDataFound = true;     //Do we have a match?
    if (header == 32760) isSettingFound = true;     //Do we have a match?
  }

  //DATA Header has been found, so the next 4 bytes are the data -- 127H + 250L = 32762
  if (SC_Serial.available() > 0 && isDataFound)
  { //Serial.print("shit1");
    isDataFound = false;
    relayHi = SC_Serial.read();   // read relay control from AgOpenGPS
    relayLo = SC_Serial.read();   // read relay control from AgOpenGPS
    groundSpeed = SC_Serial.read() >> 2;  //actual speed times 4, single byte

    // sent as 100 times value in liters per minute
    rateSetPointLeft = (float)(SC_Serial.read() << 8 | SC_Serial.read());   //high,low bytes
    rateSetPointLeft *= 0.01;
    rateSetPointRight = (float)(SC_Serial.read() << 8 | SC_Serial.read());   //high,low bytes
    rateSetPointRight *= 0.01;

    //UTurn byte
    uTurn = SC_Serial.read();

    //reset watchdog as we just heard from AgOpenGPS
    watchdogTimer = 0;
    lastAOG_Receive = millis();
  }

  //SETTINGS Header has been found,  6 bytes are the settings -- 127H + 248L = 32760
  if (SC_Serial.available() > 0 && isSettingFound)
  {
    isSettingFound = false;  //reset the flag

    //accumulated volume, 0 it if 32700 is sent
    float tempf = (float)(SC_Serial.read() << 8 | SC_Serial.read());   //high,low bytes
    if (tempf == 32700)
    {
      accumulatedCountsLeft = 0;
      accumulatedCountsRight = 0;
    }

    //flow meter cal factor in counts per Liter
    flowmeterCalFactorLeft = ((float)(SC_Serial.read() << 8 | SC_Serial.read()));   //high,low bytes
    flowmeterCalFactorRight = ((float)(SC_Serial.read() << 8 | SC_Serial.read()));   //high,low bytes
  }
}

//******************************************************************
void transmit_Cerea(){
  //SC_Serial.print("@BOOMBOX;");
  //SC_Serial.print("@SDOSE;");// Speed
  //if(sectionReturn[0] && 0x01 == 1){
  //  SC_Serial.print("1;");  
  //}else SC_Serial.print("0;"); //TB1
  //if(sectionReturn[0] && 0x02 == 2){
  //  SC_Serial.print("1;");  
  //}else SC_Serial.print("0;"); //TB1
  
  //SC_Serial.println("END");
 // delay(100);
  SC_Serial.print("@APLICADO;");  // Ausbringmenge
  SC_Serial.print(rateAppliedLPMLeft);
  SC_Serial.println(";END");
}

//******************************************************************
// Example: @CEREA;12;200;0;0;1;1;1;0;0;1;1;0;0;1;0;1;0;1;END
//          @HIDRAULIC;0;END @HIDRAULIC;1;END
void receive_Cerea(){
  while (SC_Serial.available()) 
    { 
     char ch = SC_Serial.read();
     //DbgSerial.print(ch);
     //DbgSerial.print(" ");
     //DbgSerial.print(temp);
     if (ch == '@')     //if start character is $=0x02 start new sentence
       { 
         newSet = true; // start new sentence
         in = 0;
         CereaBuf="";
         //reset watchdog as we just heard from Cerea
         watchdogTimer = 0;
       }
     if (ch == 0x0D && newSet)   //if character is CR, build UDP send buffer
       { 
         if (CereaBuf.startsWith("@CEREA") ){
           int first = CereaBuf.indexOf(';');   // first Semik
           int second = CereaBuf.indexOf(';', first + 1 ); 
           CereaBuf.remove(0, second + 1); // remove Command+speed
           first = CereaBuf.indexOf(';');   // third Semik
           String Dose = CereaBuf.substring(0, first);
           int Rate = Dose.toInt();
           CereaBuf.remove(0, first + 1);
           int last = CereaBuf.indexOf('END'); //serch for "end"
           int sectCnt = (last-2) / 2;
           if (sectCnt > 31) sectCnt = 31;
           unsigned long bitidx=1;
           Sections=0;
           for (i=0; i < sectCnt * 2; i+=2){
             if (CereaBuf.substring(i, i+1) == "1") Sections |= bitidx;
             bitidx *=2;
            }
           //DbgSerial.print("SectCnt: ");
           //DbgSerial.println(sectCnt);
           
           
          }
         if (CereaBuf.startsWith("@HIDRAULIC")){
           int first = CereaBuf.indexOf(';');   // erstes Semik
           CereaBuf.remove(0, first + 1);
           bool Hydraulics;
           if (CereaBuf.substring(0, 1) == "1") Hydraulics = 1;
           else Hydraulics = 0;
           DbgSerial.print("Cerea Hidra Out: ");
           DbgSerial.println(Hydraulics);
          }
         in = 0;
         newSet = false;
        }
      if (newSet && in < 40) 
        {
         CereaBuf += ch;
        }
    }
}
//******************************************************************
void flush_serial(){     //clean out serial buffer
      while (SC_Serial.available() > 0) char t = SC_Serial.read();
      watchdogTimer = 0;
    }
