#include <Bounce2.h>

/*
 * Created May 2016
 * 
 * Keypad system to replace ACIOY lock on the door leading to the second room in Mayan.
 * There are 12 buttons, each associated with a unique letter. 
 * There are 5 leds to indicate to the player the current state of the input sequence.
 * Only 1 led is on at a time, upon transitioning from the last state to the first, the leds dance
 * if correct the sequence was correct, otherwise the last led is lit for 1+ second.
 * 
 * Reset by holding the buttons O and C at the same time for three seconds.
 * Once this door has been unlocked, the keypad is "disabled" for the players and the door 
 * remains unlocked until a staff reset.
 *
 *
 * Mapping of letters to pin numbers
 * A, 7    O: 10
 * T:      C: 8
 * I: 9    K
 * L       R
 * U       M
 * Y: 11   N
 * 
 * All letters not part of the correct sequence share the last pin: 12 (i.e they are in paralell).
 * 
 * June 2, 2016 Mods
 * Added more conditions for the reset to decrease the chance of players locking themselves in
 * As part of fire safety conventions, the magnetic lock must be unlocked on boot up, and failures
 * Normally Open convention will be used instead
 * 
 */


#define RELAY_PIN 13
#define NUM_LEDS 5
#define NUM_BUTTON_PINS 6 // Uno does not have that many pins, so all invalid buttons share a pin
#define SEQUENCE_LENGTH 5
#define LOCKOUT_TIME 300
#define DEBOUNCE_TIME 100
#define PLAYER_LOCKOUT_TIME 1000 //  time (s) the last led is displayed for 
#define RESET_BUTTON 1 // index of the button to be held
#define OTHER_RESET_BUTTON 3 // other index of the button to be held for the full reset
#define RESET_BUTTON_HOLD_TIME 3000 // time (ms) that the reset buttons must be held for
#define FLICKER 100
#define NUM_FLICKERS 5

const int ledPins[NUM_LEDS] = {2,3,4,5,6};
const int buttonPins[NUM_BUTTON_PINS] = {7,8,9,10,11,12};
const int correctSequence[SEQUENCE_LENGTH] = {7,8,9,10,11}; // refers to the pin number

int currentSequence[SEQUENCE_LENGTH] = {0,0,0,0,0};
int sequenceCounter = 0;

Bounce debounce[NUM_BUTTON_PINS];

int long previousTime = 0;

int ledFlags[NUM_LEDS] = {0,0,0,0,0};

bool isDoorUnlocked = true; // adhere to fire safety
 
void setup() {

  
  for (int i = 0; i < NUM_BUTTON_PINS; i++){
    debounce[i] = Bounce();
    debounce[i].attach(buttonPins[i]);
    debounce[i].interval(DEBOUNCE_TIME);   
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  for (int i = 0; i < NUM_LEDS; i++){
     pinMode(ledPins[i], OUTPUT);
     digitalWrite(ledPins[i], LOW);
  }

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // on boot up the magnet must be OFF, (normally open convention)

  Serial.begin(9600);

}

void loop() {

  for (int i = 0; i < NUM_BUTTON_PINS; i++){
    debounce[i].update();   
  }
  
  if (isDoorUnlocked){  
    if (resetCondition()){
      Serial.println("O, C Being held");
      delay(RESET_BUTTON_HOLD_TIME);
      if (resetCondition()){
        reset();
        previousTime = millis(); // temporarily ignores input so button isn't accidently pressed on reset 
      }
    }
    return; 
  }

  if (millis() - previousTime < LOCKOUT_TIME){
      return; 
  }

  // poll the state of the buttons for whichever was pushed
  for (int i = 0; i < NUM_BUTTON_PINS; i++){
    if (debounce[i].read() == LOW){
      pushButton(buttonPins[i]);
      previousTime = millis();
      break;
    }
  } 

  // light up the leds according to the flags
  for (int i = 0; i < NUM_LEDS; i++){
    digitalWrite(ledPins[i], ledFlags[i]);  
  }

}

boolean checkSequence(){
  for (int i = 0; i < SEQUENCE_LENGTH; i++){
    if (currentSequence[i] != correctSequence[i]){
      return false;
    }
  }
  return true;
}

/*
 * Input: pin number of the button that was pushed (7 to 12) 
 * Stores the integer in the current sequence 
 * Adjusts the led flags
 * On the 5th button press, check the validity of the sequence and 
 * call the win or reset function
 */
 
void pushButton(int i){ 
  
  currentSequence[sequenceCounter] = i;
  Serial.print(i);  
    
  if (sequenceCounter == NUM_LEDS - 1){
      if (checkSequence()){
        Serial.println("Correct Sequence");
        win();
      }else{
        Serial.println("Incorrect Sequence");
        digitalWrite(ledPins[NUM_LEDS -2], LOW);
        digitalWrite(ledPins[NUM_LEDS -1], HIGH);
        delay(PLAYER_LOCKOUT_TIME);
        reset();
      }
  }else{    
      for (int i = 0; i < NUM_LEDS; i++){ // just turn them all off first
        ledFlags[i] = LOW;
      }   
      ledFlags[sequenceCounter] = HIGH;
      sequenceCounter++; 
  }
}

/*
 * Reset the ledflags, the current sequence input and other variables
 * Turn on the magnetic lock
 */
 
void reset(){
  for (int i = 0; i < SEQUENCE_LENGTH; i++){
    currentSequence[i] = 0;
    ledFlags[i] = LOW; 
  }
  sequenceCounter = 0;
  isDoorUnlocked = false;
  digitalWrite(RELAY_PIN, LOW); // turns ON the magnet
  delay(PLAYER_LOCKOUT_TIME);
}

/*
 * Unlock the magnetic lock and make the leds dance, then reset the led flags
 */
 
void win(){
  dance();
  for (int i = 0; i < NUM_LEDS; i++){
    ledFlags[i] = LOW; 
  }
  isDoorUnlocked = true;
  digitalWrite(RELAY_PIN, HIGH); // turn OFF the magnet
}

/*
 * Only one led can be on at a time so just light them up one at a time in sequence
 */
void dance(){
  for (int i = 0; i < NUM_FLICKERS; i++){
    digitalWrite(ledPins[NUM_LEDS -1], LOW);
    digitalWrite(ledPins[0], HIGH);
    delay(FLICKER); 
    for (int i = 1; i < NUM_LEDS; i++){ 
      digitalWrite(ledPins[i-1], LOW);
      digitalWrite(ledPins[i],HIGH);
      delay(FLICKER);
    }
  }
}


bool resetCondition(){

  // this part may not be neccesary 
  for (int i = 0; i < NUM_BUTTON_PINS; i++){
    debounce[i].update();   
  }

   // this could be done in 5 lines LOL, for future builds
  for (int i = 0; i < NUM_BUTTON_PINS; i++){
    if (i == RESET_BUTTON){
      if (debounce[RESET_BUTTON].read() == HIGH){ // 
        return false;  
      }
    }else if (i == OTHER_RESET_BUTTON){
      if (debounce[OTHER_RESET_BUTTON].read() == HIGH){
        return false;
      }
    }else{
      if (debounce[i].read() == LOW){
        return false;
      }
    }
  }
  return true;
}

