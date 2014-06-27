#include <LiquidCrystal.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7); 

int adcIn  = 0;

void setup(){
   lcd.begin(16, 2);
   
   lcd.setCursor(0,0);
   lcd.print("Hold each button");
   delay(1000);
}
 
void loop(){
  lcd.clear();
  lcd.setCursor(0,0);   
   
  adcIn = analogRead(0);
   
  lcd.print(adcIn);
   
  delay(400);


}
