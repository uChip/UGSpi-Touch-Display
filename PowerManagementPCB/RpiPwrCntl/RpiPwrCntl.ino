

// Define hardware connections
//   Arduino definition of a ATtiny85
//     p1 - Reset (active low) <-- pulled high
//     p2 - A3 or 3          <-- Rpi signals shutdown in progress active high
//     p3 - A2 or 4          <-- Power control pin active low
//     p4 - GND
//     p5 - 0 (pwm capable)  <-- LED output
//     p6 - 1 (pwm capable)  <-- Request Rpi to shutdown active low
//     p7 - A1 or 2          <-- Button input (closure to ground)
//     p8 - Vcc

// This hardware is wired as follows
#define BUTTONpin 2
#define SD_ACKpin 3
#define SD_REQpin 1
#define LEDpin 0
#define POWERpin 4

//Button Events
#define NOEVENT 0
#define SHORTPRESS 1
#define LONGPRESS 2

//Button state
#define UP true
#define DOWN false

//LED modes
#define LED_ON 1
#define LED_OFF 2
#define LED_BLINK 3
#define LED_BREATH 4
#define FULLON 255
#define FULLOFF 0

//Controller State
#define SLEEP 1
#define PWRON 2
#define REQSHUTDOWN 3
#define PWROFF 4

//Long press timer
#define BTIMEOUT 60


int LEDMode;
int State;
int ButtonEvent;
int lastButton;
int thisButton;

long ButtonTimer;
long LEDTimer;
int LEDval;

int breatharray[16] = {
  0,
  36,
  73,
  109,
  
  145,
  181,
  227,
  255,
  
  255,
  227,
  181,
  145,
  
  109,
  73,
  36,
  0,
};
int breathcount;

int myCheckButton() {
  lastButton = thisButton;
  thisButton = digitalRead(BUTTONpin);

  //Falling Edge
  if (lastButton == UP && thisButton == DOWN) {
    ButtonTimer = 0;
    return NOEVENT;
  }

  //Rising Edge
  if (lastButton == DOWN && thisButton == UP) {
    if (ButtonTimer < BTIMEOUT) return SHORTPRESS;
    else return LONGPRESS;
  }

  //Still Down
  if (lastButton == DOWN && thisButton == DOWN) {
    ButtonTimer++;
    if (ButtonTimer >= BTIMEOUT) return (LONGPRESS);
    else return NOEVENT;
  }

  return NOEVENT;
}


void UpdateLED() {
  LEDTimer++;
  if (LEDTimer > 4) {
    LEDTimer = 0;
    switch (LEDMode) {
      case LED_BLINK:
        if (LEDval > FULLOFF) LEDval = FULLOFF;
        else LEDval = FULLON;
        break;
      case LED_OFF:
        LEDval = FULLOFF;
        break;
      case LED_ON:
        LEDval = FULLON;
        break;
      case LED_BREATH:
        breathcount++;
        if (breathcount > 15) breathcount = 0;
        LEDval = breatharray[breathcount];
        break;
      default: break;
    }
    analogWrite(LEDpin, LEDval);
  }
}


void setup() {
  pinMode(BUTTONpin, INPUT_PULLUP);
  pinMode(SD_ACKpin, INPUT);
  pinMode(SD_REQpin, INPUT);
  pinMode(LEDpin, OUTPUT);
  pinMode(POWERpin, OUTPUT);

  digitalWrite(SD_REQpin, LOW);
  analogWrite(LEDpin, FULLOFF);
  digitalWrite(POWERpin, false);

  LEDMode = LED_OFF;
  State = SLEEP;

}

void loop() {
  ButtonEvent = myCheckButton();  // Returns NOEVENT, SHORTPRESS, or LONGPRESS
  UpdateLED();

  switch (State) {
    case SLEEP: {
        digitalWrite(POWERpin, false); //turn power OFF
        LEDMode = LED_BREATH;
        // digitalWrite(SD_REQpin, false); // no shutdown req
        pinMode(SD_REQpin, INPUT);  //turn off shutdown req, go from output LOW to hiZ
        if (ButtonEvent == SHORTPRESS) State = PWRON;
        if (ButtonEvent == LONGPRESS) State = PWROFF;
        break;
      }
    case PWRON: {
        digitalWrite(POWERpin, true); //turn power ON
        LEDMode = LED_ON;
        // digitalWrite(SD_REQpin, false); // no shutdown req
        pinMode(SD_REQpin, INPUT);  //turn off shutdown req, go from output LOW to hiZ
        if (ButtonEvent == SHORTPRESS) State = REQSHUTDOWN;
        if (ButtonEvent == LONGPRESS) State = PWROFF;
        if (digitalRead(SD_ACKpin) == true) {
          delay(100);
          State = PWROFF;
        }
        break;
      }
    case REQSHUTDOWN: {
        digitalWrite(POWERpin, true); //turn power ON
        LEDMode = LED_BLINK;
        // digitalWrite(SD_REQpin, LOW); //Request Shutdown
        pinMode(SD_REQpin, OUTPUT);  //Request Shutdown, go from hiZ to output LOW
        //if(ButtonEvent == SHORTPRESS) State = REQSHUTDOWN;
        if (ButtonEvent == LONGPRESS) State = PWROFF;
        if (digitalRead(SD_ACKpin) == true) {
          delay(100);
          State = PWROFF;
        }
        break;
      }
    case PWROFF: {
        digitalWrite(POWERpin, false); //turn power OFF
        LEDMode = LED_OFF;
        //digitalWrite(SD_REQpin, false); // no shutdown req
        pinMode(SD_REQpin, INPUT);  //turn off shutdown req, go from output LOW to hiZ
        if (ButtonEvent == LONGPRESS) {
          State = PWROFF;
        }
        else State = SLEEP;
        break;
      }
    default: State = SLEEP;
  }

  delay(50);
}
