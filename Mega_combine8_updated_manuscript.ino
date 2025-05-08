#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
LiquidCrystal_PCF8574 lcd(0x27);  
byte show = 0;
//----------------------------------------------------
#include <DS3231.h>
DS3231 myRTC;
bool century = false;
bool h12Flag ;
bool pmFlag ;
//----------------------------------------------------
#include <SPI.h>
#include <SD.h>
File myFile;
char fileName[] = "LTM.csv";
//----------------------------------------------------
#include <RoxMux.h>
#define MUX_TOTAL1 39           
Rox74HC165 <MUX_TOTAL1> mux1;
#define MUX_TOTAL2 39            
Rox74HC165 <MUX_TOTAL2> mux2;
#define MUX_TOTAL3 39             
Rox74HC165 <MUX_TOTAL3> mux3;
#define MUX_TOTAL4 33             
Rox74HC165 <MUX_TOTAL4> mux4;
unsigned int n1, n2, n3, n4;
// pins for 74HC165
#define PIN_MISO1 22 
#define PIN_CS1   23 
#define PIN_SCK1  24 

#define PIN_MISO2 25 
#define PIN_CS2   26 
#define PIN_SCK2  27 

#define PIN_MISO3 28 
#define PIN_CS3   29 
#define PIN_SCK3  30 

#define PIN_MISO4 31 
#define PIN_CS4   32 
#define PIN_SCK4  33 

#define green_lamp 37  //in4
#define yellow_lamp 36 //in3
#define red_lamp 35  //in2
#define speaker 34   //in1

int LED_DRIVER[20] ={38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 18, 17, 16, 15, 14, 5, 6, 7, 8, 9};

bool en_print_uart_data = 0;

bool pinState1[MUX_TOTAL1*8];  //intermediate variable
bool pinState2[MUX_TOTAL2*8];  
bool pinState3[MUX_TOTAL3*8];  
bool pinState4[MUX_TOTAL4*8];  
float state_sum[1200]; 
bool first_record_error[1200];
int open_pin_error, short_pin_error;  
bool first_check =0;
bool en_save_sdcard =0;
unsigned long Ts = millis();

bool pinStateTotal[1200];     //fix following total pín
unsigned int ith_test_of_error_led[1200];

//control button, joystick variables 
#define VRx_pin A0
#define VRy_pin A1
#define SW_pin 2

bool SW_state = 1;
int  VRx, VRx_pre;
int  VRy, VRy_pre;

unsigned int units_number = 200;
unsigned int test_time = 120;
unsigned int on_time = 5;
unsigned int off_time = 1;
unsigned int n_test;
byte led_mode =3;

unsigned int work_percent = 0;  //percent unit (%)
bool en_adj_parameters = 0;
bool clear_lcd_bit = 0;

#define on_time_max 30
#define on_time_min 1

#define off_time_max 3
#define off_time_min 1

//------------------------------------------------------
void setup(){
  Serial.begin(115200);    Serial.println("begin...");
  while (!Serial)
    ;
  Serial.println("Probing for PCF8574 on address 0x27...");
  Wire.begin();
  Wire.beginTransmission(0x27);
  int error;
  error = Wire.endTransmission();
  Serial.print("Error: ");   Serial.print(error);
if (error == 0) {
    Serial.print(": LCD found:");
    show = 0;
    Serial.println(show);
    lcd.begin(20, 4);  // initialize the lcd
    lcd.setBacklight(255);
    lcd.home();
    lcd.clear();
  } else {
    Serial.println(": LCD not found.");
  }  

  mux1.begin(PIN_MISO1, PIN_CS1, PIN_SCK1);
  mux2.begin(PIN_MISO2, PIN_CS2, PIN_SCK2);
  mux3.begin(PIN_MISO3, PIN_CS3, PIN_SCK3);
  mux4.begin(PIN_MISO4, PIN_CS4, PIN_SCK4);
  
  //----------show information of time and save to sdcard------------
  get_time_temp_info();
  //
  if (!SD.begin(53)) {    //pin 53 ->CS 
    Serial.println("Boot failed!");
    lcd.setCursor(0, 0);
    lcd.print("Boot failed!");
    lcd.setCursor(0, 1);
    lcd.print("Check sdcard!");

    while (1);
  }
  Serial.println("initialization done.");

  myFile = SD.open(fileName, FILE_WRITE);
  if (myFile) {
    //writeTimetoSdcard();
    myFile.println("#Led Test Machine#");
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening LTM.csv");
  }

  for (byte i = 0; i<20; i++)
  {
    pinMode(LED_DRIVER[i], OUTPUT);
    digitalWrite(LED_DRIVER[i], LOW);
  }

  pinMode(green_lamp, OUTPUT);   //operating status lamp 
  pinMode(yellow_lamp, OUTPUT);
  pinMode(red_lamp, OUTPUT);
  pinMode(speaker,OUTPUT);
  
   digitalWrite(green_lamp,!LOW);
   digitalWrite(yellow_lamp,!LOW);
   digitalWrite(red_lamp,!LOW);
   digitalWrite(speaker,!LOW);

  digitalWrite(speaker,LOW);

  for (unsigned int i = 0; i<1200; i++)
    {
    first_record_error[i] =0;
    }
  pinMode(SW_pin,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SW_pin), button_interrup, FALLING);
  digitalWrite(yellow_lamp,!HIGH);
}
//--------------------------------------------------
void loop()
{
if (SW_state == 0)
  {
  while(digitalRead(SW_pin) ==0) {  //reject disturbance
      delay(10); }

  //-----------------------------------------------------

 Serial.print("SW on"); 
 Serial.print(" show:"); 
 Serial.println(show); 
 SW_state = 1;

  if (show ==01) {
        show=20;     
        clear_lcd_bit = 1; 
  } else if ((show >=20) && (show <=24)) {
        saveParameterstoSdcard();
        show=00;
        clear_lcd_bit = 1; 
  } else if (show ==0)   {
        show=10;      //testing screen     
        first_check = 1;   
        clear_lcd_bit = 1; 
  } else if (show ==2) {
        show=30;      //testing screen
        clear_lcd_bit = 1;
        first_check=1; 
  } else if (show ==30) {
        show=00;      //testing screen
        clear_lcd_bit = 1;
  } else if (show ==10) {
        show=00;      //return after testing process
        clear_lcd_bit = 1;  
  } 
  }

  VRx = analogRead(VRx_pin);
  VRy = analogRead(VRy_pin);

  //enable to flash parameters
  if (abs(VRx-VRx_pre) > 200)
  {  
  Serial.print("VRx:");
  Serial.print(VRx);
  Serial.print(" show:"); 
  Serial.println(show); 

  VRx_pre = VRx;
  }

//--------------------------------------------------------------
if ((VRx<300) && (show>=0) && (show<=2))  //select one of rows
  { 
    if (show>=2)
       show = 0;   
    else
       show++;   
  }
else if ((VRx>900) && (show>=0) && (show<=2)) 
  {
    if (show<=0)
       show = 2;
    else 
       show--;              
  }
//---------------------------------------------------------------
if ((VRx<300) && (show>=20) && (show<=24))  //select one of rows
  { 
    show++;
      if (show>24)
        show = 20;      
  }
else if ((VRx>900) && (show>=20) && (show<=24)) 
  {
    show--;
      if (show<20)
        show = 24;              
  }
//enable to increase or decrease parameters
if ((abs(VRy-VRy_pre) > 200))
  {  
  Serial.print("VRy:");
  Serial.print(VRy);
  Serial.print(" show:"); 
  Serial.println(show); 

  VRy_pre = VRy;
  }

//-> enable to vary continuously
  if (show ==20)
    {
      if (VRy>900) 
      { 
        units_number++;
        if (units_number>200)    units_number =200;              
      }
      else if (VRy<300)
          {
            units_number--;
            if (units_number<1)    units_number =1;   
          }
    }
  else if (show ==21)
    {
    if (VRy>900) 
      { 
        led_mode++;
        if (led_mode>3)    led_mode=3;              
      }
      else if (VRy<300)
          {
          led_mode--;
          if (led_mode<1)    led_mode=1; 
          }
    }
  else if (show ==22)
    {
    if (VRy>900) 
      { 
        test_time++;
        if (test_time>120)    test_time =120;              
      }
      else if (VRy<300)
          {
            test_time--;
            if (test_time< (on_time+off_time))    test_time = (on_time+off_time);   
          }
    }
  else if (show ==23)
    {
    if (VRy>900) 
      { 
        on_time++;
        //if (on_time>on_time_max)    on_time =on_time_max;
        if (on_time>min(on_time_max,test_time-off_time))    on_time =min(on_time_max,test_time-off_time);               
      }
      else if (VRy<300)
          {
            on_time--;
            if (on_time < on_time_min)    on_time =on_time_min;   //be at least >1 times
          }
    }
  else if (show ==24)    
    {
    if (VRy>900) 
      { 
        off_time++;
        if (off_time>off_time_max)    off_time =off_time_max;              
      }
      else if (VRy<300)
          {
            off_time--;
            if (off_time < off_time_min)    off_time =off_time_min;   //be at least >1 times
          }
    }


 //------------------------------------------------------
 if (Serial.available()) //enter show value from pc keyboard
 {
  show = Serial.parseInt();
  lcd.setCursor(0, 0);
  lcd.clear();
 } 
  displaylcd();

 //-----start testing process----------------------------
 if ((show ==10) && (first_check ==1))
  {
    lcd.setCursor(15, 0);
    lcd.print("    "); 
    Serial.print("Testing..........");
    digitalWrite(red_lamp,!LOW);
    digitalWrite(yellow_lamp,!LOW);
    digitalWrite(green_lamp,!HIGH);
    test_process();     //->>dang bi loi
    digitalWrite(green_lamp,!LOW);
    digitalWrite(yellow_lamp,!HIGH);  

    first_check =0;  
    show = 0;
  }
 //-----calibrate in/out---------------------------------
 if ((show ==30) && (first_check ==1))
  {
    en_save_sdcard = 1;  //enable to save sdcard
    myFile = SD.open(fileName, FILE_WRITE);
  if (myFile) {
    writeTimetoSdcard();
    myFile.println("Result of in/out pins calibration:");

    calibrate_in_out_pins();     
    myFile.println();
    myFile.print("Total of Open error pins: "); myFile.println(open_pin_error); 
    myFile.print("Total of Shorted error pins:"); myFile.println(short_pin_error);
    en_save_sdcard =0;
    // close the file:
    myFile.close();    
  } else {
    Serial.println("error opening LTM.csv to save calibration result");
  }
    first_check =0;  //
  }

  if ((show == 99)) 
  {
    Serial.print("Reading a file!");
    readFromFile(); //print to pc sdcard data
    Serial.println(" Done!");
    show = 00;
    
  }

  if ((show == 77))
  {
    if (SD.exists(fileName)) 
    {
    Serial.print("Removing LTM.csv.");
    SD.remove(fileName);
    Serial.println(" Done!");
    } else {
    Serial.println("Error delete, file does not exist!");
    }

    show = 00;     
  }

 //-------------------------------
 en_print_uart_data=0;   //kiem tra trang thai IN/OUT 
//------------------------------------------------------
mux1.update();                           //combine pins into a single port
  for(uint16_t i=0, n1=mux1.getLength(); i < n1 ; i++){
    pinStateTotal[i] = mux1.read(i);
    //Serial.println(i);
    }
 //input module 2
    mux2.update();
  for(uint16_t i=0, n2=mux2.getLength(); i < n2 ; i++){
    pinStateTotal[i + MUX_TOTAL1*8] = mux2.read(i);   
    //Serial.println(i+MUX_TOTAL1*8);    
    }
 //input module 3
    mux3.update();
  for(uint16_t i=0, n3=mux3.getLength(); i < n3 ; i++){
    pinStateTotal[i+ (MUX_TOTAL1 + MUX_TOTAL2)*8] = mux3.read(i);
    //Serial.println(i+ (MUX_TOTAL1+MUX_TOTAL2)*8); 
    }
 //input module 4
    mux4.update();    
  for(uint16_t i=0, n4=mux4.getLength(); i < n4 ; i++){
    pinStateTotal[i+ (MUX_TOTAL1 + MUX_TOTAL2 + MUX_TOTAL3)*8] = mux4.read(i);
    //Serial.println(i+ (MUX_TOTAL1+MUX_TOTAL2+MUX_TOTAL3)*8); 
    }
  for (unsigned int i=0;i<units_number*6;i++)     //save errors to sdcard      
    {
      if((pinStateTotal[i] ==1) && (en_print_uart_data==1))
      {
      Serial.print(" P");
      Serial.print(i);
      Serial.print(":");
      Serial.print(pinStateTotal[i]);    
      }
    }
  if(en_print_uart_data == 1)
  {
    Serial.print(" Ts:");
    Serial.println(millis()-Ts);
    Ts = millis();
    
  }
}


//-------------------interrupt function--------------------
void button_interrup() {
  SW_state = 0;
}
//---------------------------------------
void displaylcd()
{
    if (show == 00) {
    if (clear_lcd_bit ==1)
      {
        lcd.setCursor(0, 0);
        lcd.clear();
        clear_lcd_bit =0;
      }
    lcd.setCursor(0, 0);
    lcd.print("Start to run");
    delay(150);
    lcd.setCursor(0, 0);
    lcd.print("                    ");
    delay(150);
    lcd.setCursor(0, 1);
    lcd.print("Adjust parameters");
    lcd.setCursor(0, 2);
    lcd.print("Calibrate in/out");
    lcd.setCursor(0, 3);
    lcd.print("<Push OK to confirm>");

  } else if (show == 01) {
    lcd.setCursor(0, 0);
    lcd.print("Start to run");
    lcd.setCursor(0, 1);
    lcd.print("Adjust parameters");
    delay(150);
    lcd.setCursor(0, 1);
    lcd.print("                    ");
    delay(150);
    lcd.setCursor(0, 2);
    lcd.print("Calibrate in/out");
    
} else if (show == 02) {
    lcd.setCursor(0, 0);
    lcd.print("Start to run");
    lcd.setCursor(0, 1);
    lcd.print("Adjust parameters");
    lcd.setCursor(0, 2);
    lcd.print("Calibrate in/out");
    delay(150);
    lcd.setCursor(0, 2);
    lcd.print("                    ");
    delay(150);
  
//...........................................
} else if (show == 10) {
    if (clear_lcd_bit ==1)
      {
        lcd.setCursor(0, 0);
        lcd.clear();
        clear_lcd_bit =0;
      }
    lcd.setCursor(0, 0);
    lcd.print("Testing(%)... :   ");
    // Display the temperature
    lcd.setCursor(0, 1);
    lcd.print("Temperature(C):");
    lcd.setCursor(15,1);
  	lcd.print(myRTC.getTemperature(), 1);
    lcd.setCursor(15, 0);
    lcd.print(work_percent); 
   
    
//...........................................
//Display adjusted parameters
  } else if (show == 20) {
    if (clear_lcd_bit ==1)
      {
        lcd.setCursor(0, 0);
        lcd.clear();
        clear_lcd_bit =0;
      }
    lcd.setCursor(0, 0);
    lcd.print("No.Units:"); 
    
    lcd.setCursor(10, 0);
    lcd.print("   ");
    delay(150);
    lcd.setCursor(10, 0);
    lcd.print(units_number);
    delay(150);

    lcd.setCursor(14, 0);
    lcd.print("Mode:");
    lcd.setCursor(19, 0);
    lcd.print(led_mode);

    lcd.setCursor(0, 1);
    lcd.print("Test time(min):");
    lcd.setCursor(15, 1);
    lcd.print(test_time);

    lcd.setCursor(0, 2);
    lcd.print("T_on:    T_off:"); 
    lcd.setCursor(5, 2);
    lcd.print(on_time);
    lcd.setCursor(15, 2);
    lcd.print(off_time);

    lcd.setCursor(0, 3);
    lcd.print("<Push OK to confirm>");
    
} else if (show == 21) {
    lcd.setCursor(19, 0);
    lcd.print(" ");
    delay(150);
    lcd.setCursor(19, 0);
    lcd.print(led_mode);
    delay(150);

  } else if (show == 22) {
    lcd.setCursor(15, 1);
    lcd.print("   ");
    delay(150);
    lcd.setCursor(15, 1);
    lcd.print(test_time);
    delay(150);

  } else if (show == 23) {
    lcd.setCursor(5, 2);
    lcd.print("   ");
    delay(150);
    lcd.setCursor(5, 2);
    lcd.print(on_time);
    delay(150);

  } else if (show == 24) {
    lcd.setCursor(15, 2);
    lcd.print("   ");
    delay(150);
    lcd.setCursor(15, 2);
    lcd.print(off_time);
    delay(150);
//...........................................
  } else if (show == 30) {
     if (clear_lcd_bit ==1)
      {
        lcd.setCursor(0, 0);
        lcd.clear();
        clear_lcd_bit =0;
      }
    lcd.setCursor(0, 0);
    lcd.print("Calibrate in/out:");
    // Display the temperature
        
    lcd.setCursor(0, 1);  lcd.print("OP number(F):");lcd.setCursor(15,1); lcd.print(open_pin_error); 
    lcd.setCursor(0, 2);  lcd.print("SP number(F):");lcd.setCursor(15,2); lcd.print(short_pin_error);
    lcd.setCursor(0, 3);  lcd.print("<Push OK to return>");  
   }   

}
//---------------------------------------



void test_process()
{ 
  unsigned int delay_coef = 59279;// =60000 ->1min //note: >10 
  unsigned int loop_count = 0;
  float measurement_threshold =0.55;
  unsigned int noise_reject_count[1200];
  work_percent = 0;
  bool total_error_bit = 0;
  //digitalWrite(yellow_lamp,HIGH);
  for (unsigned int  s = 0; s<units_number*6; s++)
  {
    noise_reject_count[s] =0;
  }
  
  myFile = SD.open(fileName, FILE_WRITE);
  if (myFile) {
    myFile.print("Date       ");myFile.print("Time        ");myFile.print("No.board        ");myFile.print("No.led       "); myFile.print("Status         "); myFile.println("Probability");   
    // close the file:
    myFile.close();
    } 
  
  else {
    // if the file didn't open, print an error:
    lcd.setCursor(0, 3);
    lcd.print("Insert a sdcard!");
    goto EXIT_PROCESS;
    }
  //...................................................
    
Ts = millis(); //start measuring time

n_test = test_time/(on_time+off_time);
//Serial.print("n_test:"); Serial.println(n_test);  //for debugging...

for (unsigned int k = 0; k < n_test; k++)                 //for n testing times 
  {
  lcd.setCursor(15,1);                  //update display
  lcd.print(myRTC.getTemperature(), 1);
  lcd.setCursor(15, 0);
  lcd.print(work_percent);      
  Serial.println(k);
  //Serial.println(work_percent);
  for (byte i=0; i<20; i++)
    {
     digitalWrite(LED_DRIVER[i],HIGH);
      delay(100);
    }
  //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  unsigned long delay_temp;
  for (byte d = 0;d<10; d++)     //read repeatly 10 times to reject noise
  {
  work_percent = (loop_count)*10/n_test; 
  lcd.setCursor(15,1);                  //update display
  lcd.print(myRTC.getTemperature(), 1);
  lcd.setCursor(15, 0);
  lcd.print(work_percent);   

   delay_temp = (unsigned long)on_time*(unsigned long)delay_coef/(unsigned long)10;
   delay(delay_temp);     //time of Led-on-status
   //Serial.println(on_time);
   //Serial.println(delay_temp);
   
  //input module 1
  mux1.update();                           //combine pins into a single port
  for(uint16_t i=0, n1=mux1.getLength(); i < n1 ; i++){
    pinStateTotal[i] = mux1.read(i);
    //Serial.println(i);
    }
 //input module 2
    mux2.update();
  for(uint16_t i=0, n2=mux2.getLength(); i < n2 ; i++){
    pinStateTotal[i + MUX_TOTAL1*8] = mux2.read(i);   
    //Serial.println(i+MUX_TOTAL1*8);    
    }
 //input module 3
    mux3.update();
  for(uint16_t i=0, n3=mux3.getLength(); i < n3 ; i++){
    pinStateTotal[i+ (MUX_TOTAL1 + MUX_TOTAL2)*8] = mux3.read(i);
    //Serial.println(i+ (MUX_TOTAL1+MUX_TOTAL2)*8); 
    }
 //input module 4
    mux4.update();    
  for(uint16_t i=0, n4=mux4.getLength(); i < n4 ; i++){
    pinStateTotal[i+ (MUX_TOTAL1 + MUX_TOTAL2 + MUX_TOTAL3)*8] = mux4.read(i);
    //Serial.println(i+ (MUX_TOTAL1+MUX_TOTAL2+MUX_TOTAL3)*8); 
    }
 
  loop_count++;
  Serial.print("loop_count:"); Serial.println(loop_count);

  for (unsigned int i=0;i<units_number*6;i++)     //save errors to sdcard      
    {
      noise_reject_count[i] = noise_reject_count[i] + pinStateTotal[i];    
      
    }
  }
 //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 for (byte i=0; i<20; i++)                 //Turn off each LED module one by one
    {
     digitalWrite(LED_DRIVER[i],LOW);  
     delay(100);
    }

  

delay_temp = (unsigned long)off_time*(unsigned long)delay_coef;        
delay(delay_temp);    //time of Led-OFF-status

SW_state = 1; //ignore interrup from SW button
Serial.print(" Ts:"); 
Serial.println(millis()-Ts);
Ts = millis();


}
for (unsigned int i=0;i<units_number*6;i++)     //save errors to sdcard      
    {
              
      float measurement_accuracy = float(noise_reject_count[i])/float(loop_count);  //noise_reject_count[i]/loop_count;   //12.5;//
      //Serial.println(measurement_accuracy);
      if (measurement_accuracy >= measurement_threshold) 
      {
      
      myFile = SD.open(fileName, FILE_WRITE);
      if (myFile) 
      {
        
            if (led_mode ==2)
              {
                if ((((i%6)/2+1) ==1 ) || (((i%6)/2+1) ==2) )                        //only get the first and second leds
                    {
                      myFile.print(myRTC.getDate(), DEC);               myFile.print("/");myFile.print(myRTC.getMonth(century), DEC); myFile.print("/"); myFile.print(myRTC.getYear(), DEC); myFile.print("    ");
                      myFile.print(myRTC.getHour(h12Flag, pmFlag), DEC);myFile.print(":");myFile.print(myRTC.getMinute(), DEC);       myFile.print("        ");    

                      myFile.print((i/6)+1);    myFile.print("              ");            //No. board---->very important 
                      myFile.print((i%6)/2+1);  myFile.print("            ");    //No.Led ---->very important 
                      if ((i%2) ==0) //open circuit error 
                        { 
                          myFile.print("Open");    myFile.print("           "); 
                        }
                      else  
                        {
                        myFile.print("Short");    myFile.print("          "); 
                        }
                      myFile.print(measurement_accuracy);    
                      myFile.println();   
                    }
              }
            else if ((led_mode ==3) || (led_mode ==1))
            {
              myFile.print(myRTC.getDate(), DEC);               myFile.print("/");myFile.print(myRTC.getMonth(century), DEC); myFile.print("/"); myFile.print(myRTC.getYear(), DEC); myFile.print("    ");
              myFile.print(myRTC.getHour(h12Flag, pmFlag), DEC);myFile.print(":");myFile.print(myRTC.getMinute(), DEC);       myFile.print("        ");    

              if (led_mode ==3)             //Mode: board 1 LED
              {
              myFile.print((i/6)+1);    myFile.print("              ");            //No. board---->very important 
              myFile.print((i%6)/2+1);  myFile.print("            ");    
              } else if (led_mode ==1)       //Mode: board 1 LED
              {
              myFile.print((i/2)+1);    myFile.print("              ");  
              myFile.print("1");        myFile.print("            ");  
              }

              if ((i%2) ==0) //open circuit error 
              { 
                myFile.print("Open");    myFile.print("           "); 
              }
              else  
              {
                myFile.print("Short");    myFile.print("          "); 
              }
            myFile.print(measurement_accuracy);    
            myFile.println(); 
            }
 
      }
      myFile.close();   
      total_error_bit = 1;      
      }
    }
  //...................................................
  
EXIT_PROCESS: 
  
//clear_lcd_bit ==1;
lcd.setCursor(0, 0);
lcd.clear(); 
if (total_error_bit ==1)
{
  digitalWrite(red_lamp,!HIGH);
}
digitalWrite(green_lamp,!LOW);
}

//-----------------------------------------------------

  //---------------------------------
void calibrate_in_out_pins()
  {
  unsigned int calib_noise_reject_count[units_number*6];
  for (unsigned int i=0;i<units_number*6;i++)     //save errors to sdcard      
    {
      calib_noise_reject_count[i] = 0;            //initial value = 0;
    }

  float calib_measurement_threshold = 0.6;
  unsigned int calib_count = 0;
  open_pin_error=0;
  short_pin_error=0;

  for (byte i=0; i<20; i++)
    {
     digitalWrite(LED_DRIVER[i],HIGH);
      delay(100);
    }
  delay(200);  
  //........................................................   
  for (byte unc = 0;unc<20; unc++)     //read repeatly 10 times to reject noise
  {
  delay(100); //to read many times

  mux1.update();                           //combine pins into a single port
  for(uint16_t i=0, n1=mux1.getLength(); i < n1 ; i++){
    pinStateTotal[i] = mux1.read(i);
    //Serial.println(i);
    }
 //input module 2
    mux2.update();
  for(uint16_t i=0, n2=mux2.getLength(); i < n2 ; i++){
    pinStateTotal[i + MUX_TOTAL1*8] = mux2.read(i);   
    //Serial.println(i+MUX_TOTAL1*8);    
    }
 //input module 3
    mux3.update();
  for(uint16_t i=0, n3=mux3.getLength(); i < n3 ; i++){
    pinStateTotal[i+ (MUX_TOTAL1 + MUX_TOTAL2)*8] = mux3.read(i);
    //Serial.println(i+ (MUX_TOTAL1+MUX_TOTAL2)*8); 
    }
 //input module 4
    mux4.update();    
  for(uint16_t i=0, n4=mux4.getLength(); i < n4 ; i++){
    pinStateTotal[i+ (MUX_TOTAL1 + MUX_TOTAL2 + MUX_TOTAL3)*8] = mux4.read(i);
    //Serial.println(i+ (MUX_TOTAL1+MUX_TOTAL2+MUX_TOTAL3)*8); 
    }
  calib_count++;
  Serial.print("calib_count:"); Serial.println(calib_count);

  for (unsigned int i=0;i<units_number*6;i++)     //save errors to sdcard      
    {
      calib_noise_reject_count[i] = calib_noise_reject_count[i] + pinStateTotal[i];           
    }
  SW_state = 1; //ignore interrup from SW button  
 }
    for (byte i=0; i<20; i++)                 //Turn off each LED module one by one
    {
     digitalWrite(LED_DRIVER[i],LOW);  
     delay(100);
    }

  for (unsigned int i=0;i<units_number*6;i++)     //save errors to sdcard      
    {
      float calib_measurement_accuracy = float(calib_noise_reject_count[i])/float(calib_count); 
      if (calib_measurement_accuracy >= calib_measurement_threshold) {
          //pinStateTotal[i] = 1;  

      if(((i % 2) == 0) )  { //even value of i    ......................................................
      open_pin_error++;  
      if  (en_save_sdcard==1) {   //for debugging
        myFile.print(" OC-pin");
        myFile.print(i);
        myFile.print(": ");
        myFile.print(pinStateTotal[i]);   
        }  
      } else if (((i % 2) == 1) )  {  //odd value of i
      short_pin_error++;
        if  (en_save_sdcard==1) {   //for debugging
        myFile.print(" SC-pin");
        myFile.print(i);
        myFile.print(": ");
        myFile.print(pinStateTotal[i]);   
        }
      } 

     }
   
    }
}

//--------------------------------
void get_time_temp_info()
  {
  Serial.print("DoW/Date/Month/Year/Hour/Minute/Second/12h-24h/temperature: ");
	Serial.print(myRTC.getDoW(), DEC);  
	Serial.print(" ");

	// then the date
	Serial.print(myRTC.getDate(), DEC);
	Serial.print(" ");

	// then the month
	Serial.print(myRTC.getMonth(century), DEC);
	Serial.print(" ");

  // Start with the year
	Serial.print(myRTC.getYear(), DEC);
	Serial.print(' ');
	
	// Finally the hour, minute, and second
	Serial.print(myRTC.getHour(h12Flag, pmFlag), DEC);
	Serial.print(" ");
	Serial.print(myRTC.getMinute(), DEC);
	Serial.print(" ");
	Serial.print(myRTC.getSecond(), DEC);
	// Add AM/PM indicator
	if (h12Flag) {
		if (pmFlag) {
			Serial.print(" PM ");
		} else {
			Serial.print(" AM ");
		}
	} else {
		Serial.print(" 24h ");
	}
  // Display the temperature
	Serial.print("T=");
	Serial.println(myRTC.getTemperature(), 2);
  }
  // ----------------------------------------------
  void writeTimetoSdcard()
  {
        myFile.print(myRTC.getDate(), DEC);               myFile.print("/");myFile.print(myRTC.getMonth(century), DEC); myFile.print("/"); myFile.print(myRTC.getYear(), DEC); myFile.print("    ");
        myFile.print(myRTC.getHour(h12Flag, pmFlag), DEC);myFile.print(":");myFile.print(myRTC.getMinute(), DEC);       myFile.print("        "); 
  // Display the temperature
	      myFile.print("T="); myFile.println(myRTC.getTemperature(), 2);
  }
//------------------------------------
void saveParameterstoSdcard()
  {
      myFile = SD.open(fileName, FILE_WRITE);
  if (myFile) {
      myFile.print("Number of LED units: ");  myFile.println(units_number);
      myFile.print("Test mode(leds/board): ");  myFile.println(led_mode);
      myFile.print("Total time of testing (minutes): ");myFile.println(test_time);
      myFile.print("Time of on Led (minutes): ");    myFile.println(on_time);
      myFile.print("Time of off Led (minutes): ");   myFile.println(off_time);
    // close the file:
    myFile.close();    
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening LTM.csv to save paramters");
  }      
  }
//------------------------------------
void readFromFile()  //read file from sd card and show on PC
{
   // re-open the file for reading:
  myFile = SD.open(fileName);
  if (myFile) {
    Serial.println(fileName);

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening LTM.csv");
  }
}
