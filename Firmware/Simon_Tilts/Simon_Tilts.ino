/*
  Simon Tilts
  Originally written by Pete Lewis January 2014
  SparkFun Electronics
  Beerware Liscence. Feel free to use/tweak this, but if you meet me at a bar, you can buy me a beer.
  
  This code is meant to be used with SFE's Simon Tilts thru-hole soldering kit.
  It can be found here: https://www.sparkfun.com/products/12634
  
  It is an educational kit that is not only meant to teach thru-hole soldering,
  but it also challanges the player to practice memory and orientation skills.
  
  This is intended to create a pattern game, similar to the Simon Says, that involves motion.
  It uses 3 IR tilt sensors to sense the position of the PCB in relation to gravity.
  All 6 sides of the PCB an LED on it, these are used to indicate the next tumble direction of the pattern.
  The Buzzer is also used to assotiate a tone with each position.

  To start the game, the user must tilt the PCB in any direction. Then repeat the pattern you see.
  Remember, tilting an LED upward is the equivilant of "pressing that button".
*/

#include "pitches.h"

int led_pin[6] = {4,2,7,6,5,3};
int led_gnd_pin = 10; // used to control the GND return for all LEDs
int tilt_pin[3] = {14,15,16}; // three input pins for IR tilt sensors (detectors)
int tilt_byte; // incomoing raw byte from IR tilt sensors
int correct_tilt_byte[6] = {7,4,5,3,2,0};  //  IR sense tilt uses 3 sensors, and returns 0,1,3,4,6,7
int current_box_position = 0; // this is always the side that is facing up. 
                              // "0" is placeholder, it will change when user moves
int next_possible_positions[4] = {1,2,3,4}; // These #s are placeholders.
                                            // They will be updated each time we call 
int fail = 0;
int moves_to_win = 5; // start with 5, then it get's longer with each level (incremented at the end of listen_for_pattern())
int level = 1; // used to know what level you are in. As you increment levels, it gets harder
               // see function set_level_variables() for more info
int playback_delay_time = 50;
int game_pattern [13] = {0,1,2,3,4,5,1,2,3,4,5,1,2}; // this will be updated randomly each play cycle
int game_length = 0;
int notes[6] = {NOTE_D4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_C5, NOTE_D5}; // pentatonic scale in D
boolean waiting_mode = true; // variable used to determine to play or wait

void set_level_variables(int level);                
void read_position();

void setup(){
 Serial.begin(57600); 
 randomSeed(analogRead(A7));
 
// LEDs
   pinMode(led_gnd_pin, OUTPUT);
   digitalWrite(led_gnd_pin, LOW);
   for(int i = 0;i<6;i++)
   { 
     pinMode(led_pin[i], OUTPUT);
   }
   
  blink_all(100);
  blink_all(100);
  blink_all(100);
  
//  Buzzer setup
  pinMode(12,OUTPUT);
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);

// check for nightlight mode tilt position
  if(read_position_IR() == 0) // upside down
  {
    while(1) nightlight_fade(random(0,6)); 
  }
}

void loop(){
  
  // waiting mode
  while(waiting_mode)
  {
    int prev_pos = read_position_IR();
    blink_all(50);
    if(prev_pos != read_position_IR()) 
    {
      Serial.println("change detected... exiting waiting mode");
      delay(1000); 
      waiting_mode = false;
    }
    Serial.println(read_position_IR(), DEC);
  }
  
  set_level_variables(level); // this adjusts moves_to_win, game_length, playback_delay_time
  
  if(fail == 1){
    Serial.println("Starting Over...");
    randomSeed(analogRead(A7));
    fail = 0;
  }
  
  game_length++;
  Serial.print("game_length:");
  Serial.println(game_length);
  lookup_possible_positions(game_pattern[game_length-1]); // the "-1" means were looking at the last position added (not the one were trying to add).
  int previous_game_pattern_1 = game_pattern[1]; // remember the step 1 (aka the second step of pattern)
                                                 // We will use it below to ensure we never have the same first and second step
                                                 // This helps keep the pattern different each time.
  game_pattern[game_length] = next_possible_positions[random(0,4)];

  if(game_length == 1) // help avoid doing the same pattern as previous game
  { 
    while(game_pattern[game_length] == previous_game_pattern_1)
    {
      game_pattern[game_length] = next_possible_positions[random(0,4)]; // keep pulling new random positions, until it doesn't equal the previous pattern step 1
    }
  }
  
  if(level >= 3) // because we're at level 3+, add a second pattern, this means we get two added after each "user repeat"
  {
    game_length++;
    game_pattern[game_length] = random(0,6); // no rules, it can grab any next position it likes (i.e. the "across LED")
    if(game_pattern[game_length] == game_pattern[game_length-1])
    {
      while(game_pattern[game_length] == game_pattern[game_length-1]) game_pattern[game_length] = random(0,6); // prevents two of the same in a row
    }
  }
  
  if(game_length == 0) game_length = 1; // always have at least 1 in the length
  
  show_game_pattern();
  
  Serial.print("Get Ready, GO!!");
  
  listen_for_pattern(); // Listen for correct pattern (include timeout, for failure.
                        // because gamelength and moves_to_win are global, it knows when you've won or lost
}

void read_position(int correct_next_pos){
  int pos_array[100]; // used to store position readings in a row. We want to make sure that the posisiton has stayed there for at least 10 readings before we make it a new reading.
  int valid_reading = 0;
  for(int j = 0;j<50;j++)
  {
    tilt_byte = read_position_IR();
    for(int i = 0;i<6;i++)
    {
      if(tilt_byte == correct_tilt_byte[i]) 
      {
        pos_array[j] = i;
        Serial.print(pos_array[j]);
        break;
      }
    }

    if(j == 0) valid_reading += 1;
    else{
      if(pos_array[j] == pos_array[j-1]) valid_reading += 1; // check that all of the readings in the array are the same
      else {
        delay(100);
        j=50;
      }
    }
  }
  Serial.print("--");
  Serial.println(valid_reading);
  
  if(valid_reading == 50) current_box_position = pos_array[0];
}

void lookup_possible_positions(int box_position){
 if(box_position == 0) 
 {
   next_possible_positions[0] = 1;
   next_possible_positions[1] = 2;
   next_possible_positions[2] = 3;
   next_possible_positions[3] = 4;
 }
 if(box_position == 1) 
 {
   next_possible_positions[0] = 5;
   next_possible_positions[1] = 2;
   next_possible_positions[2] = 4;
   next_possible_positions[3] = 0;
 }
  if(box_position == 2) 
 {
   next_possible_positions[0] = 1;
   next_possible_positions[1] = 5;
   next_possible_positions[2] = 3;
   next_possible_positions[3] = 0;
 }
  if(box_position == 3) 
 {
   next_possible_positions[0] = 2;
   next_possible_positions[1] = 5;
   next_possible_positions[2] = 4;
   next_possible_positions[3] = 0;
 }
  if(box_position == 4) 
 {
   next_possible_positions[0] = 3;
   next_possible_positions[1] = 5;
   next_possible_positions[2] = 1;
   next_possible_positions[3] = 0;
 }
  if(box_position == 5) 
 {
   next_possible_positions[0] = 3;
   next_possible_positions[1] = 2;
   next_possible_positions[2] = 1;
   next_possible_positions[3] = 4;
 }
}

void listen_for_pattern(){
  
  // hear the first position
  // if they are still on the first position, just wait
  // if they change to the wrong position, indicate failure!
  // if they move to the correct next step, then increment the game pattern and listen for the next position.

 for(int i = 1; i<=game_length;i++){
   int timeout = 0;
   int wrong_positions = 0;
   while(timeout<50){
     read_position(game_pattern[i]);
     if(current_box_position == game_pattern[i]) 
      {
        //Serial.print("you tumbled correct:");
        //Serial.println(current_box_position);
        blink_fade_quick(current_box_position);
        break;
      }
      else if ((timeout == 49)){
        Serial.print("Timed out on position ");
        Serial.println(i);
        blink_fail();
        waiting_mode = true;
        game_length = 0; // this gets us out of listen_for_pattern()
        level = 1;
        fail = 1;
        break;
      }
      else if(current_box_position == game_pattern[i-1]);
      else if(current_box_position != game_pattern[i]) wrong_positions += 1;
    
    if(wrong_positions > 15)
    {
      Serial.print("you tumbled WRONG:");
      Serial.println(current_box_position);
      blink_quick(current_box_position, 50);
      blink_all(50);
      blink_quick(game_pattern[i], 100);
      delay(10);
      blink_quick(game_pattern[i], 100);
      delay(10);
      blink_quick(game_pattern[i], 400);
      game_length = 0; // this gets us out of listen_for_pattern()
      level = 1;
      fail = 1;
      waiting_mode = true;
      break;
    }

   timeout += 1;   
   
   }
  
   if((i == game_length) && (fail == 0))
   {
     Serial.println("current gamelength complete!");
     Serial.println(current_box_position);
     delay(250);
     if(game_length == moves_to_win)
     { 
       play_winner(); // you've won!!
       game_length = 0; // this gets us out of listen_for_pattern()
       fail = 1; // this resets the game
       level += 1; // go to next level
     }
     else{
       delay(1000); // wait a sec to give the player time to be ready for playback
     }
   }
   
 }
}

void blink_fade_quick(int num){
  tone(12, notes[num]);
  digitalWrite(led_gnd_pin, HIGH);
  digitalWrite(led_pin[num], HIGH);
  
    // fade out from max to min in increments of 40 points:
  for(int fadeValue = 255 ; fadeValue >= 0; fadeValue -=40) { 
    // sets the value (range from 0 to 255):
    analogWrite(led_gnd_pin, fadeValue);         
    // wait for 30 milliseconds to see the dimming effect    
    delay(30); 
     } 
     
  for(int fadeValue = 0 ; fadeValue <= 255; fadeValue +=70) { 
    // sets the value (range from 0 to 255):
    analogWrite(led_gnd_pin, fadeValue);         
    // wait for 30 milliseconds to see the dimming effect    
    delay(30);                            
  } 
  digitalWrite(led_pin[num], LOW);
  digitalWrite(led_gnd_pin, LOW);
  noTone(12);
}

void blink_all(int delay_time){
     digitalWrite(led_gnd_pin, LOW);
    for(int i = 0;i<6;i++){ 
     digitalWrite(led_pin[i], HIGH);
     delay(delay_time);
     digitalWrite(led_pin[i], LOW);
  }
}

byte read_position_IR(){
    long total_1 =  analogRead(A0);
    long total_2 =  analogRead(A1);
    long total_3 =  analogRead(A2);
    byte result = 0;
    if(total_1 > 875) result |= (1<<0);
    if(total_2 > 875) result |= (1<<1);
    if(total_3 > 875) result |= (1<<2);
//    Serial.print(total_1); //1100 max
//    Serial.print("\t");    
//    Serial.print(total_2); //1000 max
//    Serial.print("\t");    
//    Serial.print(total_3); //400 max
//    Serial.print("\t"); 
//    Serial.println(result, DEC);
   return result;
}

void blink_quick(int num, int delay_time){
  tone(12, notes[num]);
  digitalWrite(led_gnd_pin, LOW);
  digitalWrite(led_pin[num], HIGH);
  delay(delay_time);
  noTone(12);
  digitalWrite(led_gnd_pin, HIGH);
  digitalWrite(led_pin[num], LOW);
  delay(delay_time);

}

void show_game_pattern(){ // show pattern to user via terminal and LEDs
  Serial.print("\n\r\n\rgame_pattern:");
  for(int i=1;i<=game_length;i++) 
  {
    Serial.print(game_pattern[i]);
    int led = game_pattern[i];
    blink_fade_quick(led);
    delay(playback_delay_time);
    Serial.print(",");
  }
  Serial.println(" ");
}

void blink_fail(void){
  tone(12, 150);
  for(int i=0;i<5;i++){
  blink_all(50);
  }
  noTone(12);
}

void play_winner(void){
  int num = 0; // starting led
  for(int pitch = 31; pitch <4900; pitch += 100)
    {
      digitalWrite(led_gnd_pin, LOW);
      digitalWrite(led_pin[num], HIGH);
      
      tone(12, pitch);
      delay(50);
      
      digitalWrite(led_gnd_pin, HIGH);
      digitalWrite(led_pin[num], LOW);
      
      num += 1; // increment to next led.
      if(num > 5) num = 0; // bring back around to led one.
    }
    
    for(int pitch = 4900; pitch >31; pitch -= 100)
    {
      digitalWrite(led_gnd_pin, LOW);
      digitalWrite(led_pin[num], HIGH);
      
      tone(12, pitch);
      delay(50);
      
      digitalWrite(led_gnd_pin, HIGH);
      digitalWrite(led_pin[num], LOW);
      
      num += 1; // increment to next led.
      if(num > 5) num = 0; // bring back around to led one.
    }
    
    noTone(12);
    delay(1000);
}

void set_level_variables(int level)
{
  if(level == 1)
  {
    moves_to_win = 5;
    playback_delay_time = 400; // start slow
  }
  else if(level == 2)
  {
    moves_to_win = 7;
    playback_delay_time = 100;
  }
  else if(level >= 3)
  {
    moves_to_win += 2;
    playback_delay_time = 50;
  }
}

void nightlight_fade(int num){
  digitalWrite(led_gnd_pin, HIGH);
  digitalWrite(led_pin[num], HIGH);
  
    // fade out from max to min in increments of 1 points:
  for(int fadeValue = 255 ; fadeValue >= 0; fadeValue -=1) { 
    // sets the value (range from 0 to 255):
    analogWrite(led_gnd_pin, fadeValue);         
    // wait for 60 milliseconds to see the dimming effect    
    delay(60); 
     } 
     
  for(int fadeValue = 0 ; fadeValue <= 255; fadeValue +=1) { 
    // sets the value (range from 0 to 255):
    analogWrite(led_gnd_pin, fadeValue);         
    // wait for 60 milliseconds to see the dimming effect    
    delay(60);                            
  } 
  digitalWrite(led_pin[num], LOW);
  digitalWrite(led_gnd_pin, LOW);
}
 
