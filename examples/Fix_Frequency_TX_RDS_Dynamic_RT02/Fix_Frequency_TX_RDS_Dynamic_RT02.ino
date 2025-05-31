/* QN8007/QN8006 Example for simple FM Transmitter with RDS ans dynamic RT
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Credits: 
 *
 * This library ist based on previous work by Evgeniy Gichev in 2021
 * Creation of this library was greatly supported by Bernhard45
 * (Google "github BM45/iRadio").

 * For in depth information about RDS look for IEC 62106:1999 document

 * Author: Semir Nouri, December 2024
 *
 */


#include <QN800x_TX.h>

QN800x_TX tx = QN800x_TX();

bool bSnew[] = {1, 1, 1, 1, 1, 1, 1, 1,}; // Button States for 8 buttons
bool bSold[] = {0, 0, 0, 0, 0, 0, 0, 0,}; // Previous button states for 8 buttons

     
uint16_t myPTY = 10;      // PTY, programme type number, change to suit your taste

uint8_t txCount = 0;      // Counter for number of full RDS group transmissions
uint8_t RTsel = 1; 
uint8_t lastRT = 4;       // Maximum number for RT text instances
bool clr = 1;             // Text A/B toggle of RDS TX algo, clears the text buffer in receiver

char myPS[] =" *Lucy* ";  // Put your station name PS here
char myText[64];          // Array place holder for RT text to be transmitted

// Character >1234566789-1234566789-1234566789-1234566789-12345667890-12345667890-1234< last character
String rt0 = "Semir's library for QN8006 and QN8006 TX functions";
String rt1 = "FM Transmitter using QN800x IC from Quintic with RDS Service";
String rt2 = "RDS Radio Text test for Arduino Software";
String rt3 = "These ICs are supported: QN8006, QN8006L, QN8007 and QN8007L";
String rt4 = "Note QN8006L and QN8007L do NOT support RDS, all TX functions OK";
String rt5 = "test";

String currentRT = rt0; // Selected string for transmission

/* Connections:
 * Arduino port A4 = SDA
 * Arduino port A5 = SCL
 * Arduino ports D2-D9 should be connected to buttons that switch pins to ground.
 * QN800x has maximum VCC of 5V. This software was tested using 3.3V
 * Tests were performed using Arduino Nano and I2C level translator
 */

 /* Remark on output power: After power up or reset output power is ~130dBµV
  * When power setting is used maximum power drops to ~120dBµV
  */

void setup()
{ 
  Wire.begin();
  Wire.setClock(400000UL);
  //Serial.begin(9600);
   
  for (int i = 2; i < 10; i++){pinMode(i, INPUT_PULLUP);} // Set pins 2-9 to input mode
  pinMode(10, OUTPUT); pinMode(14, OUTPUT); // Pins for LEDs to Indicate PS & RT sending

  tx.swReset(true); // Reset all registers to default at software start
  delay(500);       // Wait for the QN8007 to settle after reset

  tx.setExtClk(false); // Set to "true" if an external clock is input to pin 16. 
    
  tx.tuneXtal_Rin(1, 2); 
  /* First value; Use 0...61 to get most precise output frequency, depends on crystal
   * Second value: Use 0-3 equals 10k, 20k, 40k, 80k input impedance
  */
  
  tx.setDeviation(108);     // Set deviation of multiplex signal 108 = 75kHz, Formula 0,69kHz*value
  tx.setDevPilot(9);        // Set Pilot deviation as percentage of main deviation 7%, 8%, 9%, 10% are possible
  tx.setDevRDS(8);          // Set RDS carrier deviation. Default is 6 Range is 0-127
  tx.initTransmit(1, 1, 1); // Start transmit mode, set Stereo, set I2S input, RDS active
  tx.setFrequency(977);     // Set default frequency  to 97.7MHz
}

void loop()
{
  // Section for test switches

  for (uint8_t j = 0; j < 8; j++) {bSnew[j] = digitalRead(j+2);} // Read and save all button states

  if (bSnew[0] == 0) tx.setFrequency(938); // Button #1: Switch to 93.8MHz
  if (bSnew[1] == 0) tx.setFrequency(947); // Button #2: Switch to 94.7MHz
  if (bSnew[2] == 0) tx.setFrequency(1078);// Button #3: Switch to 107.8MHz

  if (bSnew[3] == 1 && bSold[3] != bSnew[3]) {tx.initTransmit(1, 1, 1); bSold[3] = bSnew[3];} // Digital input
  if (bSnew[3] == 0 && bSold[3] != bSnew[3]) {tx.initTransmit(1, 0, 1); bSold[3] = bSnew[3];} // Analog input
  
  if (bSnew[4] == 1 && bSold[4] != bSnew[4]) {tx.setMute(0); bSold[4] = bSnew[4];} // No Mute
  if (bSnew[4] == 0 && bSold[4] != bSnew[4]) {tx.setMute(1); bSold[4] = bSnew[4];} // Mute

  if (bSnew[5] == 1 && bSold[5] != bSnew[5]) {tx.setPower(0); bSold[5] = bSnew[5];} // Max RF power
  if (bSnew[5] == 0 && bSold[5] != bSnew[5]) {tx.setPower(15); bSold[5] = bSnew[5];} // Min RF power
      
    
   
  // RDS Section 

  
  if (RTsel == 0){currentRT = rt0;} // Use rt0 Text when RTsel is 0
  if (RTsel == 1){currentRT = rt1;} // Use rt1 Text when RTsel is 1
  if (RTsel == 2){currentRT = rt2;} // Use rt2 Text when RTsel is 2
  if (RTsel == 3){currentRT = rt3;} // Use rt3 Text when RTsel is 3
  if (RTsel == 4){currentRT = rt4;} // Use rt4 Text when RTsel is 4
  
  

  while (currentRT.length() < 64) {currentRT = currentRT + " ";}     // Add blanks to the end to pad to 64 characters
  myText[currentRT.length()+1]; strcpy(myText, currentRT.c_str());   // Convert String to array


  /* The below sets the ratio of PS vs RT transmission times to 6:2.
   * digits after "myPTY "0" meams A0 Group is used, set to "1" for a B0 Group
   * Some receivers seem to ignore B0 groups, however.
   * Last digit "1" Means Stereo RDS flag set to "0" for Mono
   */
 
  while (tx.getPScomplete(0) < 6)                             // Transmit PI, PTY and PS data 6 times before moving on
  {
    tx.setRDS_PS (myPS, myPTY, 0, 1);
      if (tx.getPScomplete(0) == 1){digitalWrite (10, HIGH);} // Turn on PS LED
      if (tx.getPScomplete(0) == 6){digitalWrite (10, LOW);}  // Turn off PS LED  
  } 
    
  while (tx.getRTcomplete(false) < 2)                         // Now transmit PI, PTY and RT data.  
  {
    tx.setRDS_RT (myText, myPTY, clr);
      if (tx.getRTcomplete(0) == 1){digitalWrite (14, HIGH);} // Turn on RT LED
      if (tx.getRTcomplete(0) == 2){digitalWrite (14, LOW);}  // Turn off RT LED 
  }  
  
  tx.getPScomplete(1); tx.getRTcomplete(1); // Reset PS and RT counters

  txCount++;                                // Counts the number of times the RDS data transmission loop has run  

  if (bSnew[7] == 0 && txCount%9 == 0 ) tx.bbReset(0);  
  /* The above code is a patch. It briefly resets the QN800x Base Band registers.
   * It is needed to avoid RDS hang on QN8007 only!
   * The QN8006 works without this patch.
   */

  if (txCount%5 == 0 && bSnew[6] == 1) // Set RT change rate as multiple of full RDS transmissions e.g. 5
  {
    RTsel++;     // Increment RT selection after every 5th transmission of all groups
    clr =!clr;   // Toggle A/B Text bit to clear receiver RT buffer of old text
  }   
  
  if (RTsel > lastRT) {RTsel = 0;} // Limit selector for RT text to desired number of RT instances, 
                   
  
}
