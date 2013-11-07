/*
Pete Lewis
6/28/2012
This is intended to create a pattern game, similar to the Simon, but involving remembering a sequence of "tumbles".
It uses 4 cheap tilt sensors to sense the position of the box in relation to gravity.
Each side of the box has an LED on it, these are used to indicate the next tumble direction of the pattern.
The Buzzer is used to indicate success and failures.

V021 - for green prototype board "v01"

To start the game, the user must tuble the box forward 4 times, stopping at each side. 
This sequence tells the box that it's time to begin.

RULES:
The pattern may not go in same direction more than two times in a row. This keeps it interesting.

There are 6 positions of the box. 

The box can tumble in 4 directions: LEFT, FORWARD, RIGHT and BACKWARD.



*/

#include "pitches.h"

int led_pin[6] = {4,2,7,6,5,3};
int led_gnd_pin = 10;

int tilt_pin[3] = {14,15,16};
int tilt_byte;
//int correct_tilt_byte[6] = {0,9,3,6,12,15}; //  position 0 (15) and 5 (0) are swapped between MAG and tilt sensor prototypes                                            
//int correct_tilt_byte[6] = {0,6,4,1,3,7};  //  Cap_tilt only uses 3 sensors, and returns 0,1,3,4,6,7
int correct_tilt_byte[6] = {7,4,5,3,2,0};  //  IR sense tilt only uses 3 sensors, and returns 0,1,3,4,6,7
int current_box_position = 0; // this is always the side that is facing up. The game starts on position "0".
int next_possible_positions[4] = {1,2,3,4};
int fail = 0;

int moves_to_win = 5; // start with 5, then it get's longer with each level (incremented at the end of listen_for_pattern())
int level = 1; // used to know what level you are in. As you increment levels, it gets harder
                // level 1: gamelength starts at 1, increments 1 at a time, playback starts slow then speeds up, moves_to_win = 5.
                // level 2: gamelength starts at 1, increments 1 at a time, playback fast al the time, moves_to_win = 10.
                // level 3: gamelength starts at 2, increments 2 at a time, play is even faster, moves_to_win = 15.
                
int game_length_increment = 1;
int playback_delay_time = 50;
void set_level_variables(int level);                

void read_position();

int game_pattern [13] = {0,1,2,3,4,5,1,2,3,4,5,1,2};
int game_length = 0;

int notes[6] = {NOTE_D4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_C5, NOTE_D5}; // pentatonic scale in D

void setup(){
 Serial.begin(57600); 
 randomSeed(analogRead(A7));
 
// LEDs
   pinMode(led_gnd_pin, OUTPUT);
   digitalWrite(led_gnd_pin, LOW);
   
   for(int i = 0;i<6;i++){ 
     pinMode(led_pin[i], OUTPUT);
  }
  blink_all(100);
  blink_all(100);
  blink_all(100);


// Pullup resistors on IR effect sensors
//  for(int i = 0;i<3;i++){ 
////    pinMode(tilt_pin[i], OUTPUT);
//
//   pinMode(tilt_pin[i], INPUT);
//   digitalWrite(tilt_pin[i], HIGH);
//   
//  }
  
//  Buzzer setup
  pinMode(12,OUTPUT);
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
  
//  for(int i=0;i<7;i++){
//   tone(12, notes[i]);
//   delay(50);
//   noTone(12);
//   delay(50);
//  }
  
  
  // check for nightlight mode tilt position
  if(read_position_IR() == 0) // upside down
  {
   while(1) nightlight_fade(random(0,6)); 
  }
  
}

void loop(){
  
  set_level_variables(level); // this adjusts moves_to_win, game_length, game_length_increment, playback_delay_time
  
// For debugging only
//  while(1){
//    //Serial.println(read_position_IR(), DEC);
//    read_position_IR();
//    delay(10);
//  }
  
    // choose first pattern step, add it to game_pattern.
//  lookup_possible_positions(game_pattern[game_length]);
//  Serial.println("next posible posistions:");
//  Serial.println(next_possible_positions[0]);
//  Serial.println(next_possible_positions[1]);
//  Serial.println(next_possible_positions[2]);
//  Serial.println(next_possible_positions[3]);
  
  if(fail == 1){
    Serial.println("Starting Over...");
    randomSeed(analogRead(A7));
//    game_length = 0;
    fail = 0;
//    for(int i=0;i<13;i++){ // reset the pattern to all zeros.
//      game_pattern[i] = 0;
//    }
//    lookup_possible_positions(game_pattern[0]); // make sure we lookup possible positions again because were starting at zero again.
  }
  

  
  
  
//  else{
    game_length++;
      Serial.print("game_length:");
  Serial.println(game_length);
  
  lookup_possible_positions(game_pattern[game_length-1]); // the "-1" means were looking at the last position added (not the one were trying to add).
//  Serial.print("game_pattern[game_length-1]:");
//  Serial.println(game_pattern[game_length-1]);
  
//  Serial.print("next posible posistions:");
//  Serial.print(next_possible_positions[0]);
//  Serial.print(next_possible_positions[1]);
//  Serial.print(next_possible_positions[2]);
//  Serial.println(next_possible_positions[3]);
  
//  delay(3000);

  int previous_game_pattern_1 = game_pattern[1]; // remember the step 1 (aka the second step of pattern)
                                                 // We will use it below to ensure we never have the same first and second step
                                                 // This helps keep the pattern different each time.
//  Serial.print("previous_game_pattern_1:");
//  Serial.println(previous_game_pattern_1);
//  delay(1000);
  
  game_pattern[game_length] = next_possible_positions[random(0,4)];
//  Serial.print("game_pattern[game_length]:");
//  Serial.println(game_pattern[game_length]);
//  delay(1000);

  // make sure it doesn't equal the step two previous. This prevents it from going back and forth and back and forth.
//  while((game_pattern[game_length] == game_pattern[game_length-2])){
//      game_pattern[game_length] = next_possible_positions[random(0,4)];
//    }
  if(game_length == 1){ // help avoid doing the same pattern as previous game
      while(game_pattern[game_length] == previous_game_pattern_1){
        game_pattern[game_length] = next_possible_positions[random(0,4)]; // keep pulling new random positions, until it doesn't equal the previous pattern step 1
      }
  }
//}
  //  game_pattern[game_length] = next_possible_positions[random(0,4)];
  
  if(level > 2)
  {
    game_length++;
//    game_pattern[game_length] = next_possible_positions[random(0,4)];
    game_pattern[game_length] = random(0,6);
    
    if(game_pattern[game_length] == game_pattern[game_length-1])
    {
//      while(game_pattern[game_length] == game_pattern[game_length-1]) game_pattern[game_length] = next_possible_positions[random(0,4)]; // prevents two in a row
        while(game_pattern[game_length] == game_pattern[game_length-1]) game_pattern[game_length] = random(0,6);
    }
  }
  //read_position();
  //Serial.println(current_box_position);
  
  
  //tilt_byte = (PINC & ~(1<<4) & ~(1<<5)& ~(1<<6)& ~(1<<7));
  //Serial.println(tilt_byte, DEC);
  
  
  //delay(10);
  
  // begin game
  // choose first pattern step, add it to game_pattern.
  // show pattern to user via terminal (LEDs later).
  // Listen for correct pattern (include timeout, for failure.
  // If they get it, play success tone
  // Then add a new pattern step to the end of game_pattern (using lookup_possible_positions)
  // show entire pattern to user.
  // Listen for correct pattern
  // once they do this 13 times, they win!!
  
  // begin game
  //Serial.print("game_length:");
  //Serial.println(game_length);
  
  if(game_length == 0) game_length = 1;
  
  show_game_pattern();

  //Serial.println("listening for you to tumble the pattern...");
  
  Serial.print("Get Ready, GO!!");
  delay(1000);
  
  listen_for_pattern();
  


  

  // Listen for correct pattern (include timeout, for failure.
  // If they get it, play success tone
  // Then add a new pattern step to the end of game_pattern (using lookup_possible_positions)
  // show entire pattern to user.
  // Listen for correct pattern
  // once they do this 13 times, they win!!
}

void read_position(int correct_next_pos){
  int pos_array[100]; // used to store position readings in a row. We want to make sure that the posisiton has stayed there for at least 10 readings before we make it a new reading.
  int valid_reading = 0;
  for(int j = 0;j<50;j++){
//    delay(5);
//    tilt_byte = (PINC & ~(1<<4) & ~(1<<5)& ~(1<<6)& ~(1<<7)); // for mag sensor version
    tilt_byte = read_position_IR();
    for(int i = 0;i<6;i++){
      if(tilt_byte == correct_tilt_byte[i]) {
        pos_array[j] = i;
        Serial.print(pos_array[j]);
//        delay(10);
        break;
      }
    }
//    if(pos_array[0] == pos_array[j]) valid_reading += 1; // check that all of the readings in the array are the same

//    if(pos_array[0] == correct_next_pos) break; // it only takes one correct reading to take it as a good reading.
    
    if(j == 0) valid_reading += 1;
    else{
      if(pos_array[j] == pos_array[j-1]) valid_reading += 1; // check that all of the readings in the array are the same
      else {
 //       Serial.println(pos_array[j]);
        delay(100);
        j=50;
      }
    }
  }
//  for(int i=0;i<50;i++){
//    Serial.print(pos_array[i]);
//  }
  Serial.print("--");
  Serial.println(valid_reading);
  
  if(valid_reading == 50) current_box_position = pos_array[0];
//  Serial.println(current_box_position);
//  blink_quick(current_box_position);
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

 for(int i = 0; i<=game_length;i++){
   int timeout = 0;
   int wrong_positions = 0;
   while(timeout<50){
     
    read_position(game_pattern[i]);
    
     if(current_box_position == game_pattern[i]) 
      {
//      Serial.print("you tumbled correct:");
//      Serial.println(current_box_position);
//      blink_fade_quick(current_box_position);
      blink_quick(current_box_position, 50);
      break;
      }
    
      else if ((timeout == 49)){
        Serial.print("Timed out on position ");
        Serial.println(i);
        blink_fail();
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
      break;
    }

  //  delay(5); 
   timeout += 1;   
   
//   Serial.println(timeout);
   //Serial.println(current_box_position);
   
   }
  
   if((i == game_length) && (fail == 0)){
     Serial.println("current gamelength complete!");
     Serial.println(current_box_position);
     delay(250);
        if(game_length == moves_to_win){ // you've won!!
         play_winner();
         game_length = 0; // this gets us out of listen_for_pattern()
         fail = 1; // this resets the game
         moves_to_win += 3; // each time you win, make it more difficult.
         level += 1; // go to next level
         }
         else{
//   blink_quick(current_box_position);
     digitalWrite(led_gnd_pin, LOW);
     digitalWrite(led_pin[current_box_position], HIGH);
     delay(2000);
     digitalWrite(led_pin[current_box_position], LOW);
     digitalWrite(led_gnd_pin, HIGH);
     delay(1000);
         }
   }
   
 }
}

void blink_fade_quick(int num){
  tone(12, notes[num]);
  digitalWrite(led_gnd_pin, HIGH);
  digitalWrite(led_pin[num], HIGH);
  
    // fade out from max to min in increments of 5 points:
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
//    long start = millis();

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
//    delay(5);                             // arbitrary delay to limit data to serial port 
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

void show_game_pattern(){
     // show pattern to user via terminal (and LEDs blinks).
  Serial.print("\n\r\n\rgame_pattern:");
  for(int i=0;i<=game_length;i++){
  Serial.print(game_pattern[i]);
  //BLINK LED pattern
     int led = game_pattern[i];
     blink_fade_quick(led);
//     int delay_time = (350 - (game_length*50)); // start slow, then increase playback time as the game_length gets longer
//     if(game_length >6) delay_time = 50; // after sequence 10, delaytime will always be 50 ms.
//     if(moves_to_win > 5) delay_time = 50; // if moves_to_win is greater than 5, this means you already one one round and playback will always be fast now
//     if(level>1) delay_time = playback_delay_time;
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
}

void set_level_variables(int level)
{
  if(level == 1)
  {
    moves_to_win = 4;
    playback_delay_time = (400 - (game_length*50)); // start slow, then increase playback time as the game_length gets longer
  }
  else if(level == 2)
  {
    moves_to_win = 6;
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
 
