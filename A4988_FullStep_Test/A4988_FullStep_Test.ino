int stp = 11;  //connect pin 13 to step
int dir = 12;  // connect pin 12 to dir

void setup()
{               
  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT);  
}


void loop()
{
  //digitalWrite(dir, HIGH);    //alternate these to make the motor move back and forth.
  digitalWrite(dir, LOW);

  digitalWrite(stp, HIGH);   
  delay(10);                   //alter these delay()s to change speeds. The A4988 accepts down to 500us.
  digitalWrite(stp, LOW); 
  delay(10);

}

