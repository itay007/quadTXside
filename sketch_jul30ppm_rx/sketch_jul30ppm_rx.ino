/*
   GND
   VCC 5V
   CE to Arduino pin 9
   CSN to Arduino pin 10
   SCK to Arduino pin 13
   MOSI to Arduino pin 11
   MISO to Arduino pin 12
 */
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN   9
#define CSN_PIN 10
/*PPM CONFIG*/
#define chanel_number 8  //set the number of chanels
#define default_servo_value 1500  //set the default servo value
#define PPM_FrLen  22440 //22500  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_PulseLen 400  //set the pulse length
#define onState 0 //set polarity of the pulses: 1 is positive, 0 is negative
#define sigPin 2  //set PPM signal output pin on the arduino

const uint64_t pipe = 0xE8E8F0F0E1LL; 
RF24 radio(CE_PIN, CSN_PIN);
int joystick[4]; 
int ppm[chanel_number];
int analogPin = 0;
float a_scale = 1.5;


void setup() 
{
  Serial.begin(9600);
  delay(500);
    for(int i=0; i<chanel_number; i++)
  {
    ppm[i]= default_servo_value;
  }
  delay(1000);
  Serial.println("Nrf24L01 Receiver Starting");
  radio.begin();
  radio.openReadingPipe(1,pipe);
  radio.startListening();
  pinMode(sigPin, OUTPUT);
  delay(500);
 digitalWrite(sigPin, !onState);  //set the PPM signal pin to the default state (off)
 cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  
  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();
}


void loop()   
{
  if ( radio.available() )
  {
    bool done = false;
    while (!done)
    {
      done = radio.read( joystick, sizeof(joystick) );
      /*
      Serial.print(" W = ");      
      Serial.println(joystick[3]);
      */
       ppm[4]=(1500-(525-joystick[0])*a_scale);
       ppm[5]=(1500-(525-joystick[1])*a_scale);
       ppm[6]=(1500-(525-joystick[2])*a_scale);
       ppm[7]=(1500-(525-joystick[3])*a_scale);
       //(1500-(525-joystick[0])*a_scale);
      }
  }
/*
  else
  {    
     ppm[4]=default_servo_value;
     ppm[5]=default_servo_value;
     ppm[6]=default_servo_value;
     ppm[7]=default_servo_value;
  }
*/
}

ISR(TIMER1_COMPA_vect)
{  //leave this alone
  static boolean state = true;     
   TCNT1 = 0;       
   if(state) 
   {  //start pulse
       digitalWrite(sigPin, onState);
       OCR1A = PPM_PulseLen * 2;
       state = false;
    }
     else
     {  //end pulse and calculate when to start the next pulse
         static byte cur_chan_numb;
         static unsigned int calc_rest;
         digitalWrite(sigPin, !onState);
         state = true;
         if(cur_chan_numb >= chanel_number)
         {
             cur_chan_numb = 0;
              calc_rest = calc_rest + PPM_PulseLen;// 
              OCR1A = (PPM_FrLen - calc_rest) * 2;
              calc_rest = 0;
          }
          else
           {
             OCR1A = (ppm[cur_chan_numb] - PPM_PulseLen) * 2;
             calc_rest = calc_rest + ppm[cur_chan_numb];
             cur_chan_numb++;
            }     
         }
}
