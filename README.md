SliderCam v0.2 Rob Taylor 2014 GPL 3.0
=========
Camera slider 16x2 GUI and stepper motion control for Arduino.
---------

See http://www.pololu.com/product/2128 for information and warnings on the A4988

See http://www.dfrobot.com/wiki/index.php/LCD_KeyPad_Shield_For_Arduino_SKU:_DFR0009 for info on the Amazon SainSmart shield (it's identical).

Minimum step count is one; ie. the entire motion is a single step. This is to avoid dividing by zero in the motion control calculations.

Remember that the unit splits up the total duration into motion time and shutter time. If the number of steps multiplied by the set shutter time (default at 2 seconds) exceeds the total duration set, the unit will not function as the delay period between motor steps ("pulseDelay") will become negative. It's generally a good idea to calculate the total time spent paused based on your input steps and preset shutter time prior to setting the total duration. This non-automation and lack of error-checking is flagged for future fixing.

Minimum pulseDelay is 1ms; the motor accepts a pulse this short. This gives a theoretical speed (processing overhead for remainder of motion loop notwithstanding) of 200mm/s. However, under load with a heavy camera, 2ms is a better option as 1ms can cause tooth skipping on the belt. This is again controlled by total time - paused time, so watch both sides of this equation.

For troubleshooting the motionControl() section, use serial feedback via USB immediately after the direction pin is set.

Serial.begin(9600);
Serial.println(pulseDelay);
Serial.println();
Serial.println();
Serial.println();
Serial.println();

Add other values and copy as necessary.  Calculated values tell a great deal if something is wrong.

Where "Stop!" used to be a polled feature of the motionControl() function, stoppage is now relegated to the hardware reset button, which improves speed and timing, and now a blinking "Stop!" indicates that a motion run has completed. Hit select to return to the main menu.
