
#include <Servo.h> // servo library
Servo s1;
int val = 0 ;

void setup()
{
    Serial.begin(9600);
    pinMode(7,INPUT);  // Rain sensor output pin connected
    s1.attach(6);      // Servo Motor   
}

void loop() 
{
   val = digitalRead(7);  //  Rain sensor output pin connected
  Serial.println(val);    // see the value in serial mpnitor in Arduino IDE
  delay(100);
  
  if(val == 0 )
  {
    s1.write(90);   // Servo Rotate 90 Degree
    delay(500);     // delay time
  } 

   if(val == 1 )
  {
    s1.write(0);     // Servo Rotate 0 Degree

   
  }
  }
