/***********************************************************************
; Mini-Project C Source File - Fall 2013                    
;***********************************************************************
;	 	   			 		  			 		  		
; Project Name: < Catching Stars >
;
; Team Members:
;
; Teddy The
; Ryan Widjaja
; Joseph Sudibyo
;
;***********************************************************************
;
; The objective of this Mini-Project is to demonstrate how RTI, SPI and PWM
; works in a createive real product. RTI is used to perform interrupt for
; our 2 pushbuttons, SPI is used to scroll LED and shift characters to be
; displayed to the LCD, PWM is used to manipulate frequency sent to the 2
; speakers.
;
;***********************************************************************
;
; List of project-specific success criteria :
;
; 1. LED scrolling speed can be varied for each different level (hard,
;	 hardest, hardestest).
;
; 2. 2 pushbuttons can be used to navigate through the game with different
;    functionality depending on which menu the user is currently in.
;		- in level select menu, the 2 pushbuttons is used to cycle through
;		  the choices.
;		- in game mode, the 2 pushbuttons is used to play the game.
;		- in total score menu, the 2 pushbuttons is used to go back to the 
;		  level select menu.
;
; 3. The auto software reset is works everytime the device is turned on.
;
; 4. Speakers perform different music:
;		- music for the title as soon as the device is turned on.
;		- music after 5 trials of the game when the total score is displayed.
;
; 5. Works without cable (portable) using a 9V battery.
;
;
;***********************************************************************/
#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>


// All funtions after main should be initialiezed here
void shiftout(byte);	//shiftout characters
void lcdwait(void);		//wait 2ms
void send_byte(byte);	//sending byte
void send_i(byte);		//sending instructions
void chgline(byte);		//change line by moving cursor
void print_c(byte);		//print variable
void pmsglcd(char*);	//print message to LCD
void ledwait(void);		//led wait consist of looped LCD wait depending on level variable
void leveldisp(void);	//display level
void scoredisp(void);	//display score
void music(void);		//music for title
void music2(void);		//music for total score menu (when the game is over)


//  Variable declarations                                                                                                        
int leftpb      = 0;  // left pushbutton flag
int rghtpb      = 0;  // right pushbutton flag
int leftprevpb  = 0;  // previous pushbutton state
int rghtprevpb  = 0;  // pervious pushbutton state
int lcv;			  // counter for pmsglcd
int level = 0;		  // keep track what level the user choose to play
int game = 0;		  // game mode (0 for level select menu, 1 for in game mode, 2 for total score menu)
int loop = 0;		  // number of trials the user played
int score = 0;		  // store score the user get
int ledbit = 0;		  // counter to scroll LED 
int temp = 0;		  // counter
int ledpos = 1;		  // how many times LED has turned on
int lednum = 1;		  // LED position
int ui = 0;			  // counter 
int i;				  // counter

// LCD COMMUNICATION BIT MASKS
int RS = 0x04;     		//RS pin mask (PTT[2])
int RW = 0x08;     		//R/W pin mask (PTT[3])
int LCDCLK  = 0x10;     //LCD EN/CLK pin mask (PTT[4])

// LCD INSTRUCTION CHARACTERS
int LCDON = 0x0F;     //LCD initialization command
int LCDCLR = 0x01;    //LCD clear display command
int TWOLINE = 0x38;   //LCD 2-line enable command
int CURMOV = 0xFE;    //LCD cursor move instruction
int LINE1 = 0x80;     //LCD line 1 cursor position
int LINE2 = 0xC0;     //LCD line 2 cursor position
                                    
/***********************************************************************
Initializations
***********************************************************************/
void  initializations(void) {
  
//; Set the PLL speed (bus clock = 24 MHz)
  CLKSEL = CLKSEL & 0x80; // disengage PLL from system
  PLLCTL = PLLCTL | 0x40; // turn on PLL
  SYNR = 0x02;            // set PLL multiplier
  REFDV = 0;              // set PLL divider
  while (!(CRGFLG & 0x08)){  }
  CLKSEL = CLKSEL | 0x80; // engage PLL

// Disable watchdog timer (COPCTL register)
  COPCTL = 0x40;    //COP off, RTI and COP stopped in BDM-mode
         
// Add additional port pin initializations here
  DDRAD = 0x00;
  ATDDIEN =0xC0;

//Intitialize the SPI to baud rate of 12 Mbps
  SPIBR = 0x00;
  SPICR1 = 0x50;
  SPICR2 = 0x00;
                                                                                                                          
// Initialize digital I/O port pins
  DDRT = 0xFF;
  DDRM_DDRM4 = 1;
  DDRM_DDRM5 = 1;  

  for(temp = 0; temp<9; temp++){
      PTT_PTT5 = 0;
      PTT_PTT6 = 1;
      PTT_PTT6 = 0;
    }
	
//Initialize the PWM 
  MODRR = 0x01; //PT0 used as PWM Ch 0 output
  PWME = 0x01; //Enable PWM Ch 0
  PWMPOL = 0x01; //set active high polarity
  PWMCTL = 0x00; //no concatenate
  PWMCAE = 0x00; //left aligned output
  PWMPER0 = 0xFF; //set max 8 bit period 
  PWMDTY0 = 0x00; //initially set 0% duty cycle  
  PWMCLK = 0x01; //select clock SA for Ch 0
  PWMSCLA = 0x00; //set clock SA scalar
  PWMPRCLK = 0x07; //set clock A


// Initialize LCD
  PTT_PTT4 = 1;     					// pull LCDCLK high (idle)
  PTT_PTT3 = 0;     					// pull R/W' low (write state)  
  send_i(LCDON);    					// turn on LCD (LCDON instruction)
  send_i(TWOLINE);  					// enable two-line mode (TWOLINE instruction)
  send_i(LCDCLR);   					// clear LCD (LCDCLR instruction)
  lcdwait();        					// wait for 2ms so that the LCD can wake up
  chgline(LINE1);   					// move cursor to line 1 of LCD
  pmsglcd("    Catching           ");	// print message "Catching"
  chgline(LINE2);						// move cursor to line 2 of LCD
  pmsglcd("     Stars         ");		// print message "Stars"
  music();    							// perform music for title
 
// Initialize RTI for 2.048 ms interrupt rate        
  RTICTL = 0x50;
  CRGINT = 0x80;
}
                                                                                      
/***********************************************************************
Main
***********************************************************************/
void main(void) {

        PTT_PTT1 = 0;	// perform auto software reset as soon as the device is turned on
        lcdwait();
        PTT_PTT1 = 1;                                                                                           
       
        DisableInterrupts;
        initializations();                                                                              
        EnableInterrupts;

  while(1){   //forever loop

  /* write your code here */
    if (game == 0) {        //level select mode, game = 0
      chgline(LINE1);
      pmsglcd("  Select Level         ");
      leveldisp();
      
      if (rghtpb == 1 && leftpb == 0) {      //right push button to cycle through level choices
        rghtpb = 0;
        if (level == 0) {					 //if currently the user is at "Hard" mode, right pushbutton will change it to "Hardest"
          level++;
          leveldisp();
        }
        else if (level == 1) {				 //if currently the user is at "Hardest" mode, right pushbutton will change it to "Hardestest"
          level++;
          leveldisp();
        }
      }

      else if (leftpb == 1 && rghtpb == 0) {   //left push button to cycle through level choices
        leftpb = 0;
        if (level == 1) {					 //if currently the user is at "Hardest" mode, left pushbutton will change it to "Hard"
          level--;
          leveldisp();
        }
        else if (level == 2) {				 //if currently the user is at "Hardestest" mode, left pushbutton will change it to "Hardest"
          level--;
          leveldisp();
        }      
      }
       
      else if (rghtpb == 1 && leftpb == 1) { //both right and left push button are pressed to select the displayed level
              leftpb = 0;					 //clear left pushbutton flag	
              rghtpb = 0;					 //clear right pushbutton flag
              game = 1;     				 //set the game variable to 1 (in game mode)
      }
    }
 
     else if (game == 1) {             //game mode, game = 1
        send_i(LCDCLR);				   //clear LCD
        while (loop < 5) {			   //loop to keep track how many trials have the user played
          if (level == 0) {			   //print "Hard" to LCD if level == 0
            chgline(LINE1);
            pmsglcd("      Hard             ");
          }
          else if (level == 1) {	   //print "Hardest" to LCD if level == 1
            chgline(LINE1);
            pmsglcd("     Hardest                       ");
          }
          else if (level == 2) {	   //print "Hardestest" to LCD if level == 2
            chgline(LINE1);
            pmsglcd("   Hardestest             ");
          }  
            
          while(rghtpb == 0 && leftpb == 0 && loop < 5){		//while both pushbuttons have not been set and when trials is lees than 5
          //------------------------------------------------
              if(rghtpb == 0 && leftpb == 0){			//send data to external shift register for LED scrolling
                PTT_PTT5 = 1;			//data for LED
                PTT_PTT6 = 1;			//clock 1 for LED
                PTT_PTT6 = 0;			//clock 0 for LED
                ledwait();				//wait
                ledpos++;				//update how many times LED has turned on
              }
              if(ledbit < 8){
                for(temp = 0; temp<8; temp++){
                   if(rghtpb == 0 && leftpb == 0){
                     PTT_PTT5 = 0;		//data for LED
                     PTT_PTT6 = 1;		//clock 1 for LED
                     PTT_PTT6 = 0;		//clock 0 for LED
                     ledwait();			//wait
                     ledpos++;          //update how many times LED has turned on
                   }
                 }
               }
                                         
                                         
            if(rghtpb == 1 || leftpb == 1){			//if either right or left pushbutton is set
                for(i=0;i<10;i++){					//wait
                  ledwait();
                }
                for(temp = 0; temp<9; temp++){
                    PTT_PTT5 = 0;		//data for LED
                    PTT_PTT6 = 1;		//clock 1 for LED
                    PTT_PTT6 = 0;		//clock 0 for LED
                }
                rghtpb = 0;  			//clear right pushbutton flag
                leftpb = 0;  			//clear left pushbutton flag
                loop++;		 			//increase number of trials
                lednum = ledpos % 9;    //update led position (if the LED lights up 10 times, it means the LED2 is on)
                if(lednum == 0){		//zero means 8 in led position (lednum) variable
                    lednum = 8;
                } else{
                    lednum--;
                }      
          
                ledpos = 1;     		//set how many times LED is turned on to 1
              
                chgline(LINE2);			//move cursor to line 2 of LCD
                if ( lednum == 5 ) {	//print score of 100 if the GREEN LED (LED5) lights up
					score = score + 100;
					pmsglcd("    Fuel: 100                   ");
				}
				else if ( lednum == 4 || lednum == 6) {   //print score of 75 if the YELLOW LED (LED4 or LED6) lights up
					score = score + 75;
					pmsglcd("    Fuel: 75                    ");
				}
				else if ( lednum == 3 || lednum == 7) {   //print score of 50 if the RED LED (LED3 or LED7) lights up
					score = score + 50;
					pmsglcd("    Fuel: 50                    ");
				}			
				else if ( lednum == 2 || lednum == 8) {	  //print score of 25 if the RED LED (LED2 or LED8) lights up
					score = score + 25;
					pmsglcd("    Fuel: 25                    ");
				}
				else if ( lednum == 1 || lednum == 0) {   //print score of 0 if the RED LED (LED1 or LED9) lights up
					pmsglcd("    Fuel: 0                     ");
				}   
			}
          }//while  
        }
		game = 2;			//set the game variable to 2 (total score menu mode)
     }
     
     if (game == 2) {						//total score menu mode (game = 2)
      for(i=0;i<10;i++){					//wait
         ledwait();
      }
      music2();								//perform music for total score menu
      send_i(LCDCLR);						//LCD clear
      lcdwait();							//LCD wait
      chgline(LINE1);						//move cursor to line 1 of LCD
      pmsglcd("Total Score: ");				//print "Total Score: ");
      print_c((score/100)%10+0x30);			//print the oneth of total score
      print_c((score/10)%10+0x30);			//print the tenth of total score
      print_c((score/1)%10+0x30);			//print the hundreth of total score
      scoredisp();							//display score
      while(rghtpb == 0 && leftpb == 0){    //loop so that as soon as either right of left pushbutton is set, go back to level menu and reset everything
      }
      game = 0;				//clear game mode
      loop = 0;				//clear no of trials
      score = 0;			//clear score
      PWMDTY0 = 0x00;		//turns of music
     }
     
    _FEED_COP(); /* feeds the dog */
  
  }
}

/***********************************************************************                       
; RTI interrupt service routine: RTI_ISR
;
;  Initialized for 2.048 ms interrupt rate
;
;  Samples state of pushbuttons (PAD7 = left, PAD6 = right)
;
;  If change in state from "high" to "low" detected, set pushbutton flag
;     leftpb (for PAD7 H -> L), rghtpb (for PAD6 H -> L)
;     Recall that pushbuttons are momentary contact closures to ground
;                                  
;***********************************************************************/
interrupt 7 void RTI_ISR(void)
{
          // set CRGFLG bit 
          CRGFLG = CRGFLG | 0x80; 


    // sample left pb
     if (PORTAD0_PTAD7 == 0) {
        if (leftprevpb == 1) {
          leftpb = 1;
        }
      }
    // sample right pb
     if (PORTAD0_PTAD6 == 0) {
        if (rghtprevpb == 1) {
          rghtpb = 1;
        }
      }
    leftprevpb = PORTAD0_PTAD7;
    rghtprevpb = PORTAD0_PTAD6; 
}

/***********************************************************************                              
;  shiftout: Transmits the contents of register A to external shift 
;            register using the SPI.  It should shift MSB first.  
;             
;            MISO = PM[4]
;            SCK  = PM[5]
;***********************************************************************/
void shiftout(byte data)
{
  while(SPISR_SPTEF == 0);      //read the SPTEF bit, continue if bit is 1
  SPIDR = data;        //write data to SPI data register
  //wait for 30 cycles for SPI data to shift out
        asm{
          pshx;
          psha;
          pshc; 
		  pulc;
		  pula;
		  pulx;
		  pshx;
          psha;
          pshc; 
		  pulc;
		  pula;
		  pulx;
        }
}

/***********************************************************************                              
  lcdwait: Delay for 2 ms
***********************************************************************/
void lcdwait()
{
   asm{
    ldd #$0FA0;

loopi:
    nop;
    nop;
    nop;
    nop;
    nop;
    nop;
    nop;
    nop;
    nop; 
    dbne  d,loopi;


   } 
}

/***********************************************************************                              
  send_byte: writes contents of register A to the LCD
***********************************************************************/
void send_byte(byte data)
{
     //Shift out character
     shiftout(data);
     //Pulse LCD clock line low->high
     PTT_PTT4 = 0;
     PTT_PTT4 = 1;
     //Wait 2 ms for LCD to process data
     lcdwait();
}

/***********************************************************************                              
  send_i: Sends instruction passed in register A to LCD  
***********************************************************************/
void send_i(byte inst)
{
      PTT_PTT2 = 0; //Set the register select line low (instruction data)
      send_byte(inst);  //Send byte
      PTT_PTT2 = 1;
}

/***********************************************************************                        
  chgline: Move LCD cursor to the cursor position passed in A
  NOTE: Cursor positions are encoded in the LINE1/LINE2 variables
***********************************************************************/
void chgline(byte pos)
{
     // send curmov instruction
     send_i(CURMOV);
     // send position byte
     send_i(pos); 
}

/***********************************************************************                       
;  print_c: Print character passed in register A on LCD ,            
***********************************************************************/
void print_c(byte data)
{
     send_byte(data);
}

/***********************************************************************                              
;  pmsglcd: pmsg, now for the LCD! Expect characters to be passed
;           by call. Registers should return unmodified. Should use
;           print_c to print characters.  
***********************************************************************/
void pmsglcd(char* line)
{
     lcv = 0;
     while(line[lcv] != '\0') {
      print_c(line[lcv]);
      lcv++;       
     }
}

/***********************************************************************                              
;  ledwait: depends on level, wait for a couple seconds (looped lcdwait
; 			that is 2 ms long)   
***********************************************************************/
void ledwait()
{
   if(level == 0){
   for(ui = 0; ui<30; ui++) {
   lcdwait();
   }
   }
   
   if(level == 1){
   for(ui = 0; ui<20; ui++) {
   lcdwait();
   }
   }
   
   if(level == 2){
   for(ui = 0; ui<15; ui++) {
   lcdwait();
   }
   }
}

/***********************************************************************                              
;  leveldisp: display level in level select menu.
***********************************************************************/
void leveldisp() {
 if (level == 0) {
        chgline(LINE2);
        pmsglcd("      Hard     >                     ");
      }
      else if (level == 1) {
        chgline(LINE2);
        pmsglcd("<    Hardest   >                    ");
      }
      else {
        chgline(LINE2);
        pmsglcd("<  Hardestest                    ");
      }  
}

/***********************************************************************                              
;  scoredisp: display categorized score.
				450 ----> GODLIKE!
				350 ----> EINSTEINLIKE!
				250 ----> CHILDLIKE!
				150 ----> GRANDMALIKE!
				50  ----> SLOTHLIKE!
***********************************************************************/
void scoredisp(){
  if (score > 450) {
        chgline(LINE2);
        pmsglcd("    GODLIKE!              ");
        ledwait();
        ledwait();
        ledwait();
      }
      else if (score > 350) {
        chgline(LINE2);
        pmsglcd("  EINSTEINLIKE!               ");
        ledwait();
        ledwait();
        ledwait();
      }
      else if (score > 250) {
        chgline(LINE2);
        pmsglcd("   CHILDLIKE!                ");
        ledwait();
        ledwait();
        ledwait();
      }
      else if (score > 150) {
        chgline(LINE2);
        pmsglcd("  GRANDMALIKE!          ");
        ledwait();
        ledwait();
        ledwait();
      }
      else {
        chgline(LINE2);
        pmsglcd("   SLOTHLIKE!                ");
        ledwait();
        ledwait();
        ledwait();
      } 
}

/***********************************************************************                              
;  music: performs music for the title that is displayed as soon as the
;		  device is turned on.  
***********************************************************************/
void music(){
  PWMDTY0 = 0x7F;  //set the PWM duty cycle to 50%
  PWMSCLA = 0x04;  //set clock SA scalar to 4
  PWMPRCLK = 0x0E; //set the prescalar of clock A to 14
    for(i=0;i<5;i++){
		ledwait();
    }
  
  PWMSCLA = 0x29;  //set clock SA scalar to 41
  PWMPRCLK = 0x02; //set the prescalar of clock A to 2
    for(i=0;i<5;i++){
        ledwait();
    }
      
  PWMSCLA = 0x1E;  //set clock SA scalar to 30
  PWMPRCLK = 0x02; //set the prescalar of clock A to 2
    for(i=0;i<5;i++){
        ledwait();
    }
  PWMDTY0 = 0x00;  //turn the music off by setting the PWM duty cycle to 0%
}

/***********************************************************************                              
;  music2: performs music for the total score menu (when the game ends). 
***********************************************************************/
void music2(){
    PWMDTY0 = 0x7F;  //set the PWM duty cycle to 50%
    PWMSCLA = 0x29;  //set clock SA scalar to 41
    PWMPRCLK = 0x02; //set the prescalar of clock A to 2
      for(i=0;i<5;i++){
        ledwait();
      }
      
    PWMSCLA = 0x1E;  //set clock SA scalar to 30
    PWMPRCLK = 0x02; //set the prescalar of clock A to 2
      for(i=0;i<5;i++){
        ledwait();
      }
    PWMDTY0 = 0x00;  //turn the music off by setting the PWM duty cycle to 0%
}

/***********************************************************************
; ECE 362 - Mini-Project C Source File - Fall 2013                         
;***********************************************************************/