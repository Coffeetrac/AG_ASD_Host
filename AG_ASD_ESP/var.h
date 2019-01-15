 //loop time variables in microseconds
  const unsigned int LOOP_TIME = 500; //00hz 
  unsigned int lastTime = LOOP_TIME;
  unsigned int currentTime = LOOP_TIME;
  unsigned int dT = 50000;
  byte count = 0;
  byte watchdogTimer = 0;


  byte SectMainToAOG = 0;  // output the Switches to AOG
  byte SectSWOffToAOG[]={0,0};
  byte RelayToAOG[]={0,0};
  bool autoMode=0,autoModeold=0;
 //program flow
  bool isDataFound = false, isSettingFound = false;
  int header = 0, tempHeader = 0;

 //bit 0 is section 0
  byte relayHi = 0, relayLo = 0;
  byte uTurn = 0;
  float rateSetPointLeft = 0.0;
  float rateSetPointRight = 0.0;

 //the ISR counter
  volatile unsigned long pulseCountLeft = 0, pulseDurationLeft;
  volatile unsigned long pulseCountRight = 0, pulseDurationRight;
 //Actual Applied rates
  int rateAppliedLPMLeft = 0;
  int rateAppliedLPMRight = 0;
  float groundSpeed = 0; //speed from AgOpenGPS is multiplied by 4

 //used to calculate delivered volume
  unsigned long accumulatedCountsLeft = 0;
  unsigned long accumulatedCountsRight = 0;
  float pulseAverageLeft = 0;
  float pulseAverageRight = 0;
  float flowmeterCalFactorLeft = 500;
  float flowmeterCalFactorRight = 500;
