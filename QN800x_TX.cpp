/*
QN8007 & QN8006 I2C FM Transmitter Library

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Credits: 

This library ist based on previous work by Evgeniy Gichev in 2021
Creation of this library was greatly supported by Bernhard45
(Google "github BM45/iRadio").
Improvements by Norbert W.

For in depth information about RDS look for IEC 62106:1999 document

Author: Semir Nouri, December 2024


*/

#include <Arduino.h>
#include <QN800x_TX.h>
#include <Wire.h>


QN800x_TX::QN800x_TX()
{
    I2C_address = 0x2B; // QN8007 & QN8006 I2C address
    uint8_t buf[32]; // Buffer for data sent to I2C registers
        
    rdsPI_h = 0b11010000; // RDS PI code high byte*
    rdsPI_l = 0b00100010; // RDS PI code low byte*

    /* PI Code two bytes explanation: 
     * First nibble = country code. Germany = 1 
     * Second nibble is program geographic type 2 = National
     * Second byte program number here it was set to 34
     * These settings are almost arbitrary for a Small local TX
     * But they need to be made
     */

    pstxC = 0; // Completed PS transmissions counter
    rttxC = 0; // Completed RT transmissions counter


}

//-------------------Begin Functions Area-----------------------

void QN800x_TX::SendData_(uint8_t len) 
{
  Wire.beginTransmission(I2C_address);
  for (uint8_t i = 0; i < len; i++) { Wire.write(buf[i]);}
  Wire.endTransmission();
}

void QN800x_TX::swReset(bool reset) // Reset the QN800x
{
	buf[0] = SYSTEM1;  // Start sub address SYSTEM1 address: 01h
	
	if (reset) bitSet (buf[1], 7); // Set reset bit	
	
    SendData_(2);
}

void QN800x_TX::bbReset(uint8_t gap)
{    
	buf[0] = ANACTL1; // ANACTL1 address: 03h 
    uint8_t rbuf[8]; // Read buffer for QN800x Register data

    Wire.requestFrom(I2C_address, 8); 
    for (uint8_t i = 0; i < 8; i++) {rbuf[i] = Wire.read ();} // rbuf = Read Buffer for register values read from QN8007
    	    
    buf[1] = bitClear (rbuf[5], 5);   // Reset active
	  
	SendData_(2);

    delay(gap);

    Wire.requestFrom(I2C_address, 8); 
    for (uint8_t i = 0; i < 8; i++) {rbuf[i] = Wire.read ();} // rbuf = Read Buffer for register values read from QN8007

    buf[1] = bitSet (rbuf[5], 5); // Deactivate reset

    SendData_(2);
}

void QN800x_TX::setExtClk(bool ext) // Choose to operate on external clock
{
	buf[0] = REG_XLT3; // Start sub address REG_XLT3
	
	if (ext)  buf[1] = 0b00010100; // Set bit for external XCLK input pin 16
	if (!ext) buf[1] = 0b00000100; // Use internal crystal oscillator
	
	SendData_(2);
}

void QN800x_TX::setXtal(uint8_t xtal) // Set up frequency of used clock crystal
{  
 /*  0 = 11.289MHz,  1 = 12.000MHz,  2 = 12.288MHz,  3 = 13.000MHz,  4 = 16.367MHz
  *  5 = 18.414MHz,  6 = 19.200MHz,  8 = 22.579MHz,  9 = 24.000MHz, 10 = 24.576MHz
  * 11 = 26.000MHz, 12 = 32.734MHZ, 13 = 36.828MHz, 14 = 38.400MHz, 15 = 7.600MHz
  *
  *             11 is the default use with this software!
  */	  
  
  buf[0] = ANACTL1;  // ANACTL1 Address: 03h 
  
    if (((xtal >= 0) && (xtal < 7)) || ((xtal > 7) && (xtal <= 15))) 
    {buf[1] = 0b00100000 | xtal;}
    else buf[1] = 0b00101011;      // Default frequency 26 MHz

  SendData_(2);
}

void QN800x_TX::tuneXtal_Rin(uint8_t xtune, uint8_t impedance) // Fine tune crystal frequency
{
    // Use 0...61 to get most precise output frequency, depends on crystal
    // use 0-3 equals 10k, 20k, 40k, 80k input impedance
	
    buf[0] = REG_VGA;                   // Register REG_VGA for setting analog input impedance and crystal cap
	
    uint8_t xtuneL = xtune & 0b0011111; // protect upper two bits
    uint8_t impH = impedance << 6;      // Shift value up by 6 bits so the highest two bits get altered
    buf[1] = impH | xtuneL;             // Add together the two settings and write to buffer	

	SendData_(2);
}

void QN800x_TX::initTransmit(bool stereo, bool I2S, bool RDS) // Set up transmitter mode
{
  	
  	buf[0] = SYSTEM0;       // Start sub address SYSTEM1 address: 00h
  	buf[1] = 0b01000001;    // Enter Transmitting mode. CH is determined by the content in CH[9:0].
  	buf[2] = 0b00000011;    // Set STEREO set pre-emphasis to 50µs & set IDLE to infinity
 
    if (stereo) bitClear (buf[2], 4);   // Set to stereo mode
   	if (!stereo) bitSet (buf[2], 4);    // Set to mono mode

	if (I2S) bitSet (buf[1], 2);        // Set to digital I2S input
   	if(!I2S) bitClear (buf[1], 2);      // Set to analog input	

	if (RDS) bitSet (buf[1], 1);        // Enable RDS
   	if (!RDS) bitClear (buf[1], 1);     // Disable RDS	

	SendData_(3);
}

void QN800x_TX::setFrequency(uint16_t  frequency) // Set transmitting frequency
{
  	// Calculate CH value. Frequency = 76 + CH*0.05 from data sheet
	// Change formula to CH = (freq-76)/0.05
	// Multiply all with 10/10 to use all integers instead of float yields CH = (frequency*10-7600)/5
	// Frequency has to be put in as integer e.g. 107.8MHz input is 1078

	uint16_t frequencyB = (frequency*10 - 7600)/5; 
	uint8_t frequencyH = frequencyB >> 8;   // Calculate two upper frequency bits for register 0x0B
	uint8_t frequencyL = frequencyB & 0xFF; // Calculate low 8 bits for register 0x08
  
    buf[0] = CH;                            // Start sub address 
    buf[1] = frequencyL;                    // Send lower 8 bits of frequency value
  
    SendData_(2);
  
    buf[0] = CH_STEP;                       // Start sub address 
    buf[1] = frequencyH;                    // Send upper 2 bits of frequency value
  
	SendData_(2);
}

void QN800x_TX::setPower(uint8_t power)   // Set RF output power
{	
	buf[0] = PAG_CAL;                       // Register for setting output power
	uint8_t pcon = 0b01000000;              // Disable power gain calibration functions
    uint8_t powerL = power & 0b00001111;    // lower 4 bits determine power setting
    buf[1] = pcon | powerL;	
	
	SendData_(2);
}

void QN800x_TX::setDeviation(uint8_t totaldev) // Set deviation of multiplex stereo signal
{
	buf[0] = TX_FDEV;                            // Register TX_FDEV for setting total deviation
	buf[1] = totaldev;

	SendData_(2);
}

void QN800x_TX::setDevRDS(uint8_t rdsdev) // Set deviation of RDS carrier
{
	uint8_t rds7 = rdsdev & 0b01111111;     // Protect the MSB bit if deviation value is over 127
	buf[0] = RDSFDEV;                       // Register RDSFDEV for setting RDS deviation
	buf[1] = rds7 | 0b10000000;             // Set MSB to 1 as in the default

	SendData_(2);
}

void QN800x_TX::setDevPilot(uint8_t pilotdev) // Set deviation of stereo pilot (percent of main deviation)
{	
	buf[0] = GAIN_TXPLT; // Register GAIN_TXPLT for setting Pilot deviation as a percentage of 75kHz
	
	switch (pilotdev) 
    {
	case 7:   buf[1] = 0b00011100; // Set pilot deviation to 7% of 75kHz
	break;
 	case 8:   buf[1] = 0b00100000; // Set pilot deviation to 8% of 75kHz
	break;
  	case 9:   buf[1] = 0b00100100; // Set pilot deviation to 9% of 75kHz
	break;
	case 10: buf[1] = 0b00101000; // Set pilot deviation to 10% of 75kHz
	break;
    }

	SendData_(2);
}

void QN800x_TX::setMute(bool mute) 
{  
/*	
  Wire.beginTransmission(I2C_address);
  Wire.write(ANACTL1);                       // ANACTL1 Address: 03h
  Wire.endTransmission();
  Wire.requestFrom(I2C_address, 1);          // Request one Byte
  buf[1] = Wire.read();
  if (mute)  buf[1] = buf[1] & 0b01111111;   // Mute
  if (!mute) buf[1] = buf[1] | 0b10000000;   // Unmute
  buf[0] = ANACTL1;                          // ANACTL1 Address: 03h
  SendData_(2);

*/

    uint8_t rbuf[32]; // Read buffer for QN800x Register data

    Wire.requestFrom(I2C_address, 8); 
    for (uint8_t i = 0; i < 8; i++) {rbuf[i] = Wire.read ();} // rbuf = Read Buffer for register values read from QN8007
    
    
	buf[0] = ANACTL1; // ANACTL1 address: 03h
	    
    if (mute)  buf[1] = bitSet (rbuf[5], 7);   // Mute
   	if (!mute) buf[1] = bitClear (rbuf[5], 7); // Unmute

    // Remark: On QN8007 the mute function only works with bit #5 set to 0 for mute
	  
	SendData_(2);
}


void QN800x_TX::setDAsrate(uint8_t samprate) // Master mode Audio sampling rate
{
	buf[0] = IIS; //Register for I2S setup

	// below configuration sets to 16Bit I2S protocol      
   	// Below setting will activate master mode
	// the QN8006/7 will supply BCK ad WCK as Master and expect data input

	switch (samprate) 
    {
	case 32: buf[1] = 0b01001001; // Set sample rate to 32kbps in Master mode
	break;
 	case 40: buf[1] = 0b01011001; // Set sample rate to 40kbps in Master mode
	break;
  	case 44: buf[1] = 0b01101001; // Set sample rate to 44.1kbps in Master mode
	break;
	case 48: buf[1] = 0b01111001; // Set sample rate to 48kbps in Master mode
	break;
    }
    	
	SendData_(2);
}

//------------------------RDS PS section-----------------------------------

void QN800x_TX::setRDS_PS(char ps[], uint16_t newPTY, bool useB0, bool stereo) // Send PS using 0A or 0B group format
{

    uint8_t rbuf[7];   // Read Buffer for register values read from QN8007
    uint8_t pslc = 0;   // PS Letter Counter for PS twin letters

    Wire.requestFrom(I2C_address, 7); 
    for (uint8_t i = 0; i < 7; i++) {rbuf[i] = Wire.read ();}

    
        for (pslc = 0; pslc < 4; pslc++)   
        {
    
        uint16_t rdsGrp_0A = 0b0000000000001000; // raw group 0A data. Choose 0A when there are alternative frequencies

        /* Explanation for the 16 group 0A bits:
         * 0000->Group#, 1 = B0 -> B (0 = A), 0->TP, 00000->PTY, 0->TA, 0->M/S, 0->DI, xx->PS letter counter 00...11
         * 15-12         11                   10     9 - 5       4      3       2      10  Bit number MSB....LSB
         */
        
        uint16_t rdsGrp_0B = 0b0000100000001000; // raw group 0B data. Chose 0B if there are no alternative frequencies

        /* Explanation for the 16 group 0B bits:
         * 0000->Group#, 1 = B0 -> B (0 = A), 0->TP, 00000->PTY, 0->TA, 0->M/S, 0->DI, xx->PS letter counter 00...11
         * 15-12         11                   10     9 - 5       4      3       2      10  Bit number MSB....LSB
         */
        
        uint8_t af[] = {227, 102, 64, 203, 205, 205, 205, 205}; 

        /* Explanation: The above sets alternative frequencies (AF) for A0 group only.
         * The first item sets number of following AFs 225 = 1, 226 = 2... the first 
         * should be the frequency of this station, followed by several AF. Here two
         * have been added. Frequencies are coded by numbers:  1 = 87.6, 2 = 87.7
         * 3 = 87.8...204 = 107.9. 205 is a dummy code for "no frequency.
         */
                        
        uint16_t pty = newPTY << 5; // Move PTY bits to the correct position.

        

        if (pslc == 3 && stereo) {rdsGrp_0A = rdsGrp_0A | 0b0000000000000100;} // Set d0 Bit to 1 = stereo
        if (pslc == 3 && stereo) {rdsGrp_0B = rdsGrp_0B | 0b0000000000000100;} // Set d0 Bit to 1 = stereo

  
        rdsGrp_0B =  rdsGrp_0B + pslc + pty;       // Add Letter counter and PTY data to group
        rdsGrp_0A =  rdsGrp_0A + pslc + pty;       // Add Letter counter and PTY data to group
      
        buf[0] = RDS0;                             // First RDS register address 10h
        buf[1] = rdsPI_h;                          // Send data for Register 0x10 PI High Byte
        buf[2] = rdsPI_l;                          // Send data for Register 0x11 PI Low Byte

        if(useB0)
        {     					                   // Settings if B0 group is used
        buf[3] = ((uint8_t) ((rdsGrp_0B) >> 8));   // Send data for Register 0x12 Group High Byte
        buf[4] = ((uint8_t) ((rdsGrp_0B) & 0xff)); // Send data for Register 0x13 Group Low Byte
        buf[5] = rdsPI_h;                          // Send data for Register 0x14 PI High Byte repeat
        buf[6] = rdsPI_l;                          // Send data for Register 0x15 PI Low Byte repeat
        }
        
        if(!useB0)
        { 					                       // Settings if A0 group is used 
        buf[3] = ((uint8_t) ((rdsGrp_0A) >> 8));   // Send data for Register 0x12 Group High Byte
        buf[4] = ((uint8_t) ((rdsGrp_0A) & 0xff)); // Send data for Register 0x13 Group Low Byte
        buf[5] = af[2*pslc];                       // Send data for Register 0x14 Alternative Freq. list
        buf[6] = af[2*pslc+1];                     // Send data for Register 0x15 Alternative Freq. list
        }   

        buf[7] = ps[2*pslc];                       // Send data for Register 0x16 First letter
        buf[8] = ps[2*pslc+1];                     // Send data for Register 0x17 Second letter
        
        SendData_(9);

       rbuf[3] = rbuf[3] ^ 0b00000100;              // Toggle RDS Ready bit from register 0x01

        buf[0] = SYSTEM1;                            // Register SYSTEM1 contains the RDSReady toggle bit
        buf[1] = rbuf[3];                            // Send toggled bit to move RT data to QN8007 RDS tx data buffer
        
        SendData_(2);

        delay(100);                                   // Delay to allow the QN8007 to transmit the RDS data of one block

        /* The RDS bit rate is 1187.5 kb/s. One full group of 4 blocks has a total of 104 bits
         * 16 data bits + 10 bits for error correction hence 4 x 26 = 104 bits in a block.
         * it takes 104b / 1187kb/s = 0.0877s i.e. ~90 ms of transmit time for a group to complete
         * assuming a few ms of additional processing time at least 90 ms of delay needs to be 
         * added for an ATMEGa328 running @16MHz.
         */

        if  (pslc == 3) {pstxC++;} // increase PS completed transmission counter at the end of each 4 block RDS send

        }
            
}

uint8_t QN800x_TX::getPScomplete(bool reset) // Get PS counter value, look for counter reset
{
    if (reset) {pstxC = 0;}                    // Reset PS transmission counter to 0 when reset is true
    return pstxC;                              // Send current vale of PS tx counter to main program
}
    /* Explanation: The ratio between transmitting PS and RT can be controlled
     * PS should be transmitted 40% of the time, RT 15% of the time
     * Higher ratio in favor of PS will speed up station identification
     * But it will take longer for RT to appear on receiver display
     * In practice a ratio of 6:2 for PS:RT works fine
     * The value of this counter can be used to determine the number of
     * consecutive transmissions of the PS in the main programs loop
     * sending a reset = true initializes the counter
     */


//---------------------RDS RT Section---------------------------------

void QN800x_TX::setRDS_RT (char rt[], uint16_t newPTY, bool clrTXT)
{
  uint8_t rbuf[7]; // Read Buffer for register values read from QN8007
  uint8_t rtlc = 0; 

  Wire.requestFrom(I2C_address, 7);
  for (uint8_t i = 0; i < 7; i++) {rbuf[i] = Wire.read ();}
  
        for (rtlc = 0; rtlc < 16; rtlc++)   // rtlc = Radio Text Letter Counter for RT quad letters
        {    
  
            uint16_t rdsGrp_2A = 0b0010000000000000; // raw group 2A data. 2A offers 64 characters for RT
   
        /* Explanation for the 16 group 2A bits:
         * 0000->Group#, 1 = Bo -> B (0 = A), 0->TP, 00000->PTY, 0->T-A/B, xxxx-> RT letter counter 00...63 
         * 15-12         11                   10     9 - 5       4         3210    Bit number MSB....LSB
         */

        uint16_t pty = newPTY << 5; // Move PTY bits to the correct position.

        
  
        if (clrTXT) {rdsGrp_2A = rdsGrp_2A | 0b0000000000010000;} // Change A/B Text bit for new text 

        rdsGrp_2A = rdsGrp_2A  + pty + rtlc; // Add Letter counter and PTY data to group

      
        buf[0] = RDS0;                             // First RDS register address 10h
   	    buf[1] = rdsPI_h;                          // Send data for Register 0x10 PI High Byte
        buf[2] = rdsPI_l;                          // Send data for Register 0x11 PI Low Byte
        buf[3] = ((uint8_t) ((rdsGrp_2A) >> 8));   // Send data for Register 0x12 Group High Byte
        buf[4] = ((uint8_t) ((rdsGrp_2A) & 0xff)); // Send data for Register 0x13 Group Low Byte
        buf[5] = (rt[4*rtlc]);                     // Send data for Register 0x14 First letter
        buf[6] = (rt[4*rtlc+1]);                   // Send data for Register 0x15 Second letter
        buf[7] = (rt[4*rtlc+2]);                   // Send data for Register 0x16 Third letter
        buf[8] = (rt[4*rtlc+3]);                   // Send data for Register 0x17 Fourth letter
	  
        SendData_(9);

        rbuf[3] = rbuf[3] ^ 0b00000100;             // Toggle RDS Ready bit 
   
        buf[0] = SYSTEM1;                           // Register SYSTEM1 contains the RDSReady toggle bit
        buf[1] = rbuf[3];                           // Send toggled bit to move RT data to RDS tx buffer
	  
        SendData_(2);

        delay(100);                                  // Delay to allow the QN8007 to transmit the RDS data

        /* The RDS bit rate is 1187.5 kb/s. One full group of 4 blocks has a total of 104 bits
         * 16 data bits + 10 bits for error correction hence 4 x 26 = 104 bits in a block.
         * it takes 104b / 1187kb/s = 0.0877s i.e. ~90 ms of transmit time for a group to complete
         * assuming a few ms of additional processing time at least 90 ms of delay needs to be 
         * added for an ATMEGa328 running @16MHz.
         */

        if (rtlc == 15){rttxC++;}// Increment PS completed transmission counter at the end of transmission cycle      
        
        }

}

uint8_t QN800x_TX::getRTcomplete(bool reset)

{
    if (reset) {rttxC = 0;}     // Reset RT transmission counter to 0 when reset is true
    return rttxC;               // Send current value of RT tx counter to main program
}

    /* Explanation: The ratio between transmitting PS and RT can be controlled
     * PS should be transmitted 40% of the time, RT 15% of the time
     * Higher ratio in favor of PS will speed up station identification
     * But it will take longer for RT to appear on receiver display
     * In practice a ratio of 6:2 for PS:RT works fine
     * The value of this counter can be used to determine the number of
     * consecutive transmissions of the RT in the main programs loop
     * sending a reset = true initializes the counter
     */

