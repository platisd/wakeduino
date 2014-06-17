/* buzzer constants */

const int c = 261;
const int d = 294;
const int e = 329;
const int f = 349;
const int g = 391;
const int gS = 415;
const int a = 440;
const int aS = 455;
const int b = 466;
const int cH = 523;
const int cSH = 554;
const int dH = 587;
const int dSH = 622;
const int eH = 659;
const int fH = 698;
const int fSH = 740;
const int gH = 784;
const int gSH = 830;
const int aH = 880;

const int firstSectionTones[20][2] = {
  {a, 500},
  {a, 500},    
  {a, 500},
  {f, 350},
  {cH, 150},  
  {a, 500},
  {f, 350},
  {cH, 150},
  {a, 650},
  {0,500},
  {eH, 500},
  {eH, 500},
  {eH, 500},  
  {fH, 350},
  {cH, 150},
  {gS, 500},
  {f, 350},
  {cH, 150},
  {a, 650},
  {0,500}

};


const int secondSectionTones[18][2] = {
  {aH, 500},
  {a, 300},
  {a, 150},
  {aH, 500},
  {gSH, 325},
  {gH, 175},
  {fSH, 125},
  {fH, 125},    
  {fSH, 250},
  {0,325},
  {aS, 250},
  {dSH, 500},
  {dH, 325},  
  {cSH, 175},  
  {cH, 125},  
  {b, 125},  
  {cH, 250},
   {0,350} 
  
};

const int variant1Tones [9][2] = {
  {f, 250},  
  {gS, 500},  
  {f, 350},  
  {a, 125},
  {cH, 500},
  {a, 375},  
  {cH, 125},
  {eH, 650},
  {0,500}
};

const int variant2Tones [9][2] = {
  {f, 250},  
  {gS, 500},  
  {f, 375},  
  {cH, 125},
  {a, 500},  
  {f, 375},  
  {cH, 125},
  {a, 650},  
  {0,650}  
  
};
/* custom cursors definition */
byte homeButton [8] = {
  B00100,
  B01010,
  B10001,
  B00100,
  B01010,
  B10001,
  B00000
};
byte plusMinus [7] = {
  B00100,
  B00100,
  B11111,
  B00100,
  B00100,
  B00000,
  B11111  
};
byte jCursor [8];
byte normalCursor [8] = { //has to be 8 positions array, otherwise some strange pixels show up at the bottom
  B00000,
  B00100,
  B00010,
  B11111,
  B00010,
  B00100,
  B00000  
};
byte selectCursor [8] = {
  B01000,
  B01100,
  B01110,
  B01111,
  B01110,
  B01100,
  B01000  
};
byte alarmBell [8] = {
  B00100,
  B01110,
  B01110,
  B11111,
  B11111,
  B00100,
  B00000
};
