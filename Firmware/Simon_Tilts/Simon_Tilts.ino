/*
*  Simon Tilts
*  Originally written by Pete Lewis January 2014
*  SparkFun Electronics
*  Beerware License. Feel free to use/tweak this, but if you meet me at a bar, you can buy me a beer.
*  
*  This code is meant to be used with SFE's Simon Tilts thru-hole soldering kit.
*  It can be found here: https://www.sparkfun.com/products/12634
*  
*  It is an educational kit that is not only meant to teach thru-hole soldering,
*  but it also challenges the player to practice memory and orientation skills.
*  
*  This is intended to create a pattern game, similar to the Simon Says, that involves motion.
*  It uses 3 IR tilt sensors to sense the position of the PCB in relation to gravity.
*  All 6 sides of the PCB has an LED on it, these are used to indicate the next tilt direction of the pattern.
*  The buzzer is also used to associate a tone with each position.
*
*  To start the game, the user must tilt the PCB in any direction. Then repeat the pattern you see.
*  Remember, tilting an LED upward is the equivalent of "pressing that button".
*/

#include "pitches.h" // This header file defines the values needed to make "scaled" tones on the buzzer

//------------------------------------------------------------------------------------------------
// HARDWARE PIN DECLARATIONS

// Define an array to include the pins for all six LEDs.
  // They are used during gameplay to blink a specific pattern that the player must follow
  // They are hardwired in the PCB. This means they will never change, so they can be a constant int
const int led_pin[6] = {4,2,7,6,5,3}; 

// Define the pin for the GND return on all six LEDs
  // All six LEDs are wired to return on this one single pin: "led_gnd_pin"
  // Most of the time, we will be turning on only one LED at a time
  // To turn on a specific LED, we must turn that LED pin HIGH, and this led_gnd_pin LOW
  // We chose pin 10 because it can do PWM. This allows us to do PWM on all six LEDs
const int led_gnd_pin = 10;

// Define an array to include the pins connected to the tilt sensors
  // Really, they are connected to the infrared detector next to each tilt sensor
  // But because their ultimate purpose is to sense tilt position, we named them "tilt_pin"
  // If we do an analogRead() on these pins, they will return a different value depending on
  // where the BB is in the tilt arm sensor. See explanation below for more info.
const int tilt_pin[3] = {14,15,16};

//------------------------------------------------------------------------------------------------
// GAMEPLAY VARIABLE DECLARATIONS

// Define an array to include six NOTES that will be used during gameplay             
  // These notes will be played on the buzzer each time an LED is lite up
  // Each LED gets a specific tone. 
  // This array of notes corresponds with the previous array of LEDs (lede_pin[])
  // These particular notes are a pentatonic scale in D                                      
const int notes[6] = {NOTE_D4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_C5, NOTE_D5}; 

// Define a boolean to know whether we are in waiting mode or not
  // Simon tilts effectively has three modes (so far): waiting, play, and nightlight
  // If nightlight mode is not entered during setup(), then it can be in "waiting" or "not waiting"
boolean waiting_mode = true;

// Define a variable to keep track of when a player makes a mistake (aka fails)
  // For example, they timed out or rolled to the incorrect position
  // "False" means there is no failure, we will initialize it this way
boolean fail = false;
                  
//------------------------------------------------------------------------------------------------
// LEVEL SPECIFIC VARIABLE DECLARATIONS

// Define a variable to keep track of which level the player is in
  // With each level, the Simon Tilts gets a little more difficult to play
  // The player starts on level 1 and increments up as they complete each level
  // See function set_level_variables() for more info
int level = 1;
               
// Define playback_delay_time variable (MILLISECONDS)
  // This affects the overall speed at which the pattern is shown to the player
  // The pattern may be as little as one (and as many as 20) "steps"
  // This variable is the amount of milliseconds delay in between each step's blink
  // In level one, the delay time is 400 ms (pretty slow). It will decrease more and more in higher levels
  // See function set_level_variables() for more info
int playback_delay_time; 

// Define a variable to keep track of how many moves it takes to win the current level
  // It starts with 5, then it get's longer with each level
  // See function set_level_variables() for more info
int moves_to_win; 

// Define an array to keep track of the game pattern
  // The game pattern is the sequence of tilt positions that the user must repeat
  // It is randomly chosen each time the user enters a new level
  // Each level, it gets longer and longer.
  // This means it could get as long as 23 on level 10.
  // It is *really* hard to complete level 3, so I'd be very excited to see someone win level 10 and break this array
  // For most players they will only be using the first 9 spots of this array and then failing :)
int game_pattern[23] = {};

// Define a variable to keep track of the length of the current game
  // This is the number of tilt positions in the pattern the user must repeat in order to advance each "partial-level"
  // It is incremented each time there is a new random position added to game_pattern[]
  // It is also compared with moves_to_win each time the player tilts correctly.
  // This is how we know if the player has one the level yet, or if they are still building to moves_to_win
int game_length = 0;                      

// Define an array to include the next possible positions that could be added to the game pattern
  // This is necessary, so that when we randomly choose a pattern, it doesn't choose two of the same positions in a row
  // Because this is a tilt motion game, it doesn't make sense if the pattern repeats on the current position
  // We also use this array in combination with the function, lookup_possible_positions(), in order to 
  // make the game pattern more user friendly for levels 1 and 2.
  // We can ensure that the game pattern always moves from one position to an adjacent position,
  // and not an "across" position (i.e. if you're on the top, then immediately blinking the bottom)
  // See lookup_possible_positions() for more info
int next_possible_positions[4] = {};

//------------------------------------------------------------------------------------------------
// TILT SENSOR SPECIFIC VARIABLE DECLARATIONS

// Infrared tilt sensor explanation
// The tilt sensors are little tubes with small metal ball bearings (aka "BB"s) in them
// Each BB can fall to either end of the tube, and is pulled by gravity
// On one end of the tube (closest to the PCB) there is a "look through" hole
// With an IR emitter and detector on either side of the "look through" hole,
// we can see if the BB is present or not.
// Combining three of these tilt sensors positioned carefully, you can sense 6 positions.
// TOP, BOTTOM, LEFT, RIGHT, FRONT, BACK (these are defined while holding the game so that the 
// silk "SIMON TILTS" reads left to right, and is facing upwards)

// The readings from each IR detector are as follows for each position
// Each infrared detector is defined by it's corresponding pin on the micro (it's "tilt_pin")

//   POSITION     tilt_pin[0]    tilt_pin[1]    tilt_pin[2]       DECIMAL
//------------------------------------------------------------------------
//   TOP          1              1              1                 7 
//   BOTTOM       0              0              0                 0
//   LEFT         1              0              1                 5
//   RIGHT        0              1              0                 2
//   FRONT        1              0              0                 4
//   BACK         0              1              1                 3

// Define an array of "good" readings from the tilt sensors
  // Notice how 5 and 6 are not included as possibilities in the table above
  // This is because of the particular angle and placement of the tilt arms
  // 5 and 6 are impossible, so in an effort to debounce the data, we create the following array
  // The order of this array is important, because it corresponds with the led_pin[] array.
const byte correct_tilt_byte[6] = {7,4,5,3,2,0};

// Define a variable used to store the actual current physical position
  // This is determined by which tilt position the player is currently holding the game
  // It is updated every time we call read_position_w_debounce()
  // It is also used in listen_for_pattern() to determine gameplay
int current_tilt_position;

void setup()
{
  Serial.begin(57600); // For debugging
  randomSeed(analogRead(A7)); // We use random() to create the unique game pattern for each level
                              // This seed helps make it "more" random
  // LED SETUP 
    // Turn all six to OUTPUTS
  for(int i = 0;i<6;i++)
  { 
    pinMode(led_pin[i], OUTPUT);
  }
  
  // Turn the GND return for all six leds to output, and low
  pinMode(led_gnd_pin, OUTPUT);
  digitalWrite(led_gnd_pin, LOW);
  // Blink all Leds 3 times to show that they are working 
  blink_all(100);
  blink_all(100);
  blink_all(100);
  
  //  BUZZER SETUP
    // The Buzzer is connected to pins 12 and 13
    // Turn both to outputs
  pinMode(12,OUTPUT);
  pinMode(13,OUTPUT);
  // Write the "pin 13 side" LOW
  // We will use the "pin 12 side" to buzz the tone
  digitalWrite(13,LOW);

  // EASTER EGG!!! -- CHECK FOR NIGHTLIGHT MODE --
    // If the user turns on the game while holding it upside down, 
    // then it will go into a special mode called "Nightlight mode"
    // This is in setup() because we only want it to check once (right after powering up)
    // If the game is held in any other position, then it will continue into the main loop 
    // and play default game mode
    // To enter night light mode, follow these three steps:
    // 1. Turn game off
    // 2. Hold game upside down
    // 3. While continuing to hold game upside down, turn the game back on again and wait a moment.
  if(read_position_once() == 0) // read_position_once() will return a "0" if the game is held upside down
  {
    while(1) // Stay in nightlight mode forever (or at least until you cyle power) 
    {
      nightlight_fade(random(0,6)); 
    }
  }
}

void loop()
{
  // Each time we come around to the beginning of the main loop, check to see if we want to be in
  // waiting mode. If waiting mode is engaged, then we will enter the following while loop and wait 
  // for the player to move the game in any way, to indicate they want to start a new game.
  // When "waiting_mode" is set to false, then this while loop is ignored and normal game play happens.
  // "waiting_mode" is only changed to false in wait_for_user_input_change()
  while(waiting_mode) wait_for_user_input_change();
  
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
  
  listen_for_pattern(); // Listen for correct pattern (handles timeout and failures)
                        // because gamelength and moves_to_win are global, it knows when you've won or lost
}

// Define a function to take lots of readings from the tilt sensors and debounce them into one usable position reading #
  // This is a bit different than your standard averaging debounce of streaming data.
  // In fact, it doesn't do any averaging at all.
  // We make an array to store all the streaming readings.
  // We then starting filling the array with one reading at a time.
  // There are two important things going on with this function:
  // (1) it compares each new reading with the previous reading,
  // If they are the same, then it stores the active reading and moves on to take another
  // If it's different, then it doesn't take it as a good reading, and assumes it's noise.
  // The for loop ends and the reading doesn't change current_tilt_position. So essentially, this reading failed (it was too noisy).
  // This makes the player actually hold the game still for 50 readings (which is actually only a few mili seconds.
  // (2) It compares each reading to correct_tilt_byte[] array.
  // This makes sure that the BBs are settled in the tilt arm and not causing an erroneous data.
  // It converts the raw byte returned from read_position_once() into a number from 0 to 6. 
  // This will be the ultimate position #. It is more useful this way, because we can use it to call 
  // the corresponding LED from led_pin[] array
void read_position_w_debounce()
{
  int pos_array[100]; // used to store position readings in a row. We want to make sure that the position has stayed there for at least 50 readings before we make it a new reading.
  int valid_reading = 0;
  for(int j = 0;j<50;j++)
  {
    byte raw_tilt_input = read_position_once(); // take a single reading from the tilt sensors
    for(int i = 0;i<6;i++) // compare the raw reading to the "known goods" in the correct_tilt_byte[] array.
                           // Do it in a for loop so we can check all six.
    {
      if(raw_tilt_input == correct_tilt_byte[i]) 
      {
        pos_array[j] = i; // convert to a number from 0 to 6 (i from this for loop will be 0-6), this will be our ultimate # to know the position
        Serial.print(pos_array[j]);
        break;
      }
    }
    if(j == 0) valid_reading += 1; // we will assume the first reading is always a valid reading, this keeps the total count correct
    else{
      if(pos_array[j] == pos_array[j-1]) valid_reading += 1; // check new reading against previous, if it's the same, increment valid_reading
      else{
        delay(100);
        j=50;
      }
    }
  }
  Serial.print("--");
  Serial.println(valid_reading);
  
  if(valid_reading == 50) current_tilt_position = pos_array[0]; // only if we get 50 valid readings (all the same), do we take this as a solid good reading
                                                                // and finally... set the current_tilt_position to the first reading in the pos_array[0]
}


// Define a function that listens for the correct pattern to be rotated by the player
  // This is basically a for loop that goes through the current gamelength,
  // and listens for the player to tumble the correct pattern
  // It includes a timeout by rapping all of the "if's" in a while(timeout<50)
  // In each of the IF statements it checks 
  // (1) Are they on the correct next position?
  // (2) Is it time to timeout?
  // (3) Are they still on the previous position?
  // (4) Did the player rotate to the wrong position? 
  // (5) Has the player stayed on a wrong position for 15 counts?
void listen_for_pattern()
{
  for(int i = 1; i<=game_length;i++)
  {
    int timeout = 0;
    int wrong_positions = 0;
    while(timeout<50)
    {
      read_position_w_debounce(); // Take in a reading of position with debounce
      
      if(current_tilt_position == game_pattern[i]) // (1) Are they on the correct next position?
        {
          blink_led_w_fade(current_tilt_position); // Blink current position once to indicate they did it correctly
          break;
        }
      else if ((timeout == 49)) // (2) how long have they been trying (or thinking), is it time to timeout?
      {
        Serial.print("Timed out on position ");
        Serial.println(i);
        blink_player_timed_out();
        waiting_mode = true;
        game_length = 0; // this gets us out of listen_for_pattern()
        level = 1;
        fail = 1;
        break;
      }
      else if(current_tilt_position == game_pattern[i-1]); // (3) Are they still on the previous position? If so, do nothing and increment timeout (happens below)
      else if(current_tilt_position != game_pattern[i]) wrong_positions += 1; // (4) Did the player rotate to the wrong position? 
                                                                            //  If so, keep track of how long by incrementing "wrong_positions"
      
      if(wrong_positions > 15) // (5) Has the player stayed on a wrong position for 15 counts?
      {
        Serial.print("you tumbled wrong:");
        Serial.println(current_tilt_position);
        blink_led(current_tilt_position, 50); // Blink quickly to show the incorrect tilt position
        blink_all(50);
        blink_led(game_pattern[i], 100); // Blink where the player should have rolled to
        delay(10);
        blink_led(game_pattern[i], 100); // Blink where the player should have rolled to (again)
        delay(10);
        blink_led(game_pattern[i], 400); // Blink where the player should have rolled to (and  a 3rd time)
        game_length = 0; // this gets us out of listen_for_pattern()
        level = 1;
        fail = 1;
        waiting_mode = true;
        break;
      }
      timeout += 1;   // Increment timeout
   } // End of timeout while loop
  
   if((i == game_length) && (fail == 0))
   {
     Serial.println("current gamelength complete!");
     Serial.println(current_tilt_position);
     delay(250);
     // Check to see if the player has reached "moves_to_win"
     if(game_length == moves_to_win)
     { 
       show_winner_blinkies(); // you've won!!
       game_length = 0; // this gets us out of listen_for_pattern()
       fail = 1;
       level += 1; // go to next level
     }
     else{
       delay(1000); // wait a sec to give the player time to be ready for playback
     }
    }
  } // end of game pattern for loop
}


// Define a function to blink one LED for a set amount of time with sound
  // Send this an LED number (0-5) and a delay time (milliseconds)
  // This function is only used in listen_for_pattern()
  // It is used when a player rotates to the incorrect position
  // and Simon Tilts then wants to show you where you should have rotated to.
void blink_led(int num, int delay_time)
{
  tone(12, notes[num]);
  digitalWrite(led_gnd_pin, LOW);
  digitalWrite(led_pin[num], HIGH);
  delay(delay_time);
  noTone(12);
  digitalWrite(led_gnd_pin, HIGH);
  digitalWrite(led_pin[num], LOW);
  delay(delay_time);
}


// Define a function to "blink" an led with quick fading and sound
  // This is used during playback of the game sequence
  // Send it a number and it will fade in and out quickly on that LED
void blink_led_w_fade(int num)
{
  tone(12, notes[num]);
  digitalWrite(led_gnd_pin, HIGH);
  digitalWrite(led_pin[num], HIGH);
  
  // fade out from max to min in increments of 40 points:
  for(int fadeValue = 255 ; fadeValue >= 0; fadeValue -=40) 
  {
    analogWrite(led_gnd_pin, fadeValue); // sets the value (range from 0 to 255):  
    delay(30); // wait for 30 milliseconds to see the dimming effect   
  } 
     
  for(int fadeValue = 0 ; fadeValue <= 255; fadeValue +=70) 
  { 
    analogWrite(led_gnd_pin, fadeValue); // sets the value (range from 0 to 255):      
    delay(30); // wait for 30 milliseconds to see the dimming effect                           
  } 
  digitalWrite(led_pin[num], LOW);
  digitalWrite(led_gnd_pin, LOW);
  noTone(12);
}


// Define a function to blink all six LEDs in sequence without sound
  // This basically uses a for loop to blink all six LEDs
  // It does NOT buzz the buzzer with each corresponding tone
void blink_all(int delay_time)
{
  digitalWrite(led_gnd_pin, LOW);
  for(int i = 0;i<6;i++)
  { 
    digitalWrite(led_pin[i], HIGH);
    delay(delay_time);
    digitalWrite(led_pin[i], LOW);
  }
}


// Define a function to indicate the the player has timed out of gameplay
  // This happens when they stay on a position for too long
  // The "timeout" variable is defined in listen_for_pattern()
  // This function plays a low tone and blinks all six LEDs in sequence
  // It is intended to be very different from winner_blinky() which is kind of "cheery"
void blink_player_timed_out(void)
{
  tone(12, 150);
  for(int i=0;i<5;i++)
  {
    blink_all(50);
  }
  noTone(12);
}


// Define a function that will show various blinky action and sound buzzing
  // This is used to control what the player sees and hears when they win each level
  // It's basically a way to call winner_blinky different amounts of times at different speeds
  // This way when you win each level, you see a unique display and it "sort of" shows you 
  // which level you just completed
void show_winner_blinkies()
{
  if(level == 1)
  {
    winner_blinky(100);
  }
  else if(level == 2)
  {
    winner_blinky(200);
    winner_blinky(200);
  }
  else if(level >= 3)
  {
    for(int i=1;i<=level;i++)
      {
         winner_blinky(400);
      }
  }
  delay(1000); // This gives the player a second to get ready for the next level
}


// Define a function to blink the six LEDs in sequence, while also playing tones
  // Send this function an integer, increment_val, to speed up or slow down the sequence
  // This function is basically comprised of two for loops.
  // One to blink LEDs and buzz tones that start at low pitches and then climb up to high pitches
  // And a second to start at high pitches and then decline down to low pitches
  // It's mostly just a fun thing to see and hear when you complete a level
void winner_blinky(int increment_val)
{
  int num = 0; // starting led
  for(int pitch = 31; pitch <4900; pitch += increment_val)
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
  
  for(int pitch = 4900; pitch >31; pitch -= increment_val)
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


// Define a function to show current game pattern to user
  // This function blinks a sequence of LEDs (and also buzzes each corresponding tone)
  // It is basically a for loop that starts at the beginning, and then continues through the entire game length
  // Note, gamelength is incremented in the main loop.
  // As the player repeats the current "sub" pattern correctly, gamelength increments
  // This continues on until they win the current level
void show_game_pattern()
{
  Serial.print("\n\r\n\rgame_pattern:");
  for(int i=1;i<=game_length;i++) 
  {
    Serial.print(game_pattern[i]); // debug
    Serial.print(","); // debug
    blink_led_w_fade(game_pattern[i]);
    delay(playback_delay_time);
  }
  Serial.println(" "); // print a new line, this helps keep the debug serial window more legible
}


// Define a function to take a single reading of the tilt sensors
  // This function will read all three tilt sensors,
  // and return a byte value that represents the current read position.
  // Each infrared detectors will output a different voltages depending on where
  // the BB rests in it's corresponding tilt arm.
  // If we take an analogRead() on one of the infrared detectors, we can know
  // if the BB is present or not (that is, blocking the IR emitter).
  // Through some testing we found that when the BB is at the bottom of the tilt arm,
  // and blocking the IR, the analogRead will consistently return a value greater that 875. 
  // This is our "threshold" value.
byte read_position_once()
{
  byte result = 0;
  if(analogRead(tilt_pin[0]) > 875) result |= (1<<0);
  if(analogRead(tilt_pin[1]) > 875) result |= (1<<1);
  if(analogRead(tilt_pin[2]) > 875) result |= (1<<2);
  return result;
}


// Define a function to set level variables
  // Send this function the current level, which can be any integer >=1
  // It takes the level and then adjusts the gameplay variables accordingly
  // This gameplay variables are then used in listen_for_pattern() to control the difficulty of the game
void set_level_variables(int level)
{
  if(level == 1)
  {
    moves_to_win = 5;
    playback_delay_time = 400;
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


// Define a function to be used during "waiting mode"
  // This function basically blinks al six LEDs and waits for the player to tilt the game in any direction
  // It reads the current position, and then reads a second time,
  // and then compares the two readings to know if it's changed.
  // When this happens, it changes the global variable, "waiting_mode" to false, which ultimately kills waiting mode
void wait_for_user_input_change(void)
{
  byte prev_pos = read_position_once(); // store the current position so we can compare it later and notice a change.
  blink_all(50); // Blink to indicate we're in waiting mode
  if(prev_pos != read_position_once()) // if the player has rotated, then this new "fresh" reading will be different than "prev_pos"
  {
    delay(1000); // Delay to allow the user to return to the home position (with the silk "Simon Tilts" reading left to right)
    waiting_mode = false; // This ultimately will get us out of the waiting mode while loop (at the top of the main loop)
                          // and then move on to the gameplay (which lives in the remainder of the main loop)
  }
}


// Define a function to update the next possible positions
  // For use in creating a nice random game pattern
  // This function acts as a set of "rules" to ensure we don't have two of the same positions in a row for any game pattern
  // You send it the current position, and it will change the values in the array next_possible_positions[].
  // The new values will only include positions that are adjacent to the current position
void lookup_possible_positions(int tilt_pos)
{
  switch (tilt_pos)
  {
    case 0:
      next_possible_positions[0] = 1;
      next_possible_positions[1] = 2;
      next_possible_positions[2] = 3;
      next_possible_positions[3] = 4;
      break;
    case 1:
      next_possible_positions[0] = 5;
      next_possible_positions[1] = 2;
      next_possible_positions[2] = 4;
      next_possible_positions[3] = 0;
      break;
    case 2:
      next_possible_positions[0] = 1;
      next_possible_positions[1] = 5;
      next_possible_positions[2] = 3;
      next_possible_positions[3] = 0;
      break;
    case 3:
      next_possible_positions[0] = 2;
      next_possible_positions[1] = 5;
      next_possible_positions[2] = 4;
      next_possible_positions[3] = 0;
      break;
    case 4:
      next_possible_positions[0] = 3;
      next_possible_positions[1] = 5;
      next_possible_positions[2] = 1;
      next_possible_positions[3] = 0;
      break;
    case 5:
      next_possible_positions[0] = 3;
      next_possible_positions[1] = 2;
      next_possible_positions[2] = 1;
      next_possible_positions[3] = 4;
      break;
  }
}


// Define a function to do one single slow fade IN and OUT on one single LED
  // This is used to create nightlight mode.
  // You send it an integer to select which LED you'd like to fade
  // It will fade in and out slowly on "num"
  // "num" should be any number between 0 and 5, including 0.
  // These for loops are from ARDUINO>>EXAMPLES>>ANALOG>>FADING
  // the fadeValue increment changes the speed of the fade
void nightlight_fade(int num){
  digitalWrite(led_gnd_pin, HIGH); // Turn the GND return HIGH, which effectively turns all LEDs off
  digitalWrite(led_pin[num], HIGH); // Turn the LED we'd like to fade on to HIGH
  
  // Fade IN, note because we are using PWM on the GND return to control the fade,
  // we start at 255 (HIGH - or off) and then slowly decrease to 0 (LOW - full brightness)
  for(int fadeValue = 255 ; fadeValue >= 0; fadeValue -=1) { 
    analogWrite(led_gnd_pin, fadeValue); // sets the value (range from 0 to 255):    
    delay(60); // wait for 60 milliseconds to see the dimming effect    
     } 
     
  // Fade OUT, note because we are using PWM on the GND return to control the fade,
  // we start at 0 (LOW) and then slowly increase to 255 (HIGH)
  for(int fadeValue = 0 ; fadeValue <= 255; fadeValue +=1) { 
    analogWrite(led_gnd_pin, fadeValue);         
    delay(60);                            
  } 
  
  digitalWrite(led_pin[num], LOW); // Turn the specified LED off
}
 
