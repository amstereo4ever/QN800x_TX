/*
QN8007 & QN8006 I2C FM Radio Library

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

For indepth information about RDS look for IEC 62106:1999 document

Author: Semir Nouri, December 2024

*/

#include <Wire.h>

#ifndef QN800x_TX_h
#define QN800x_TX_h

#define SYSTEM0    0x00        /* Sets device modes                                   */
#define SYSTEM1    0x01        /* Sets device modes, resets                           */
#define ANACTL1    0x03        /* Analog control functions                            */
#define REG_VGA    0x04        /* TX mode input impedance, crystal cap load setting   */
#define IIS        0x07        /* Sets I2S parameters                                 */
#define CH         0x08        /* Lower 8 bits of 10-bit channel index                */
#define CH_STEP    0x0B        /* Channel scan frequency step. Highest 2 bits of channel indexes. */
#define TX_FDEV    0x0E        /* Specify total TX frequency deviation.                           */
#define GAIN_TXPLT 0x0F        /* Gain of TX pilot frequency deviation, I2S buffer clear.         */
#define RDS0       0x10        /* RDS data byte 0                                     */
#define RDS1       0x11        /* RDS data byte 1                                     */
#define RDS2       0x12        /* RDS data byte 2                                     */
#define RDS3       0x13        /* RDS data byte 3                                     */
#define RDS4       0x14        /* RDS data byte 4                                     */
#define RDS5       0x15        /* RDS data byte 5                                     */
#define RDS6       0x16        /* RDS data byte 6                                     */
#define RDS7       0x17        /* RDS data byte 7                                     */
#define RDSFDEV    0x18        /* Specify RDS frequency deviation, RDS mode selection */
#define REG_XLT3   0x49        /* XCLK pin control                                    */
#define PAG_CAL    0x5A        /* PA gain calibration                                 */

class QN800x_TX
{

public:
  
    QN800x_TX();
  
    void swReset(bool reset); // Reset the QN8007 via software
    void bbReset(uint8_t gap); // Reset the QN8007 audio Base Band (and it seems RDS) registers
    
    // General setup

    void setExtClk(bool ext);                            // Choose external clock source by setting to true
    void setXtal (uint8_t xtal);                         // Choose crystal frequency, default is "#11" or 26MHz
    void tuneXtal_Rin(uint8_t xtune, uint8_t impedance); // Range from 0-61 equals 10pF-30pF crystal capacitor load
                                                         // impedance Range from 0-4 equals 10k-80k

    // RF Setup

    void initTransmit(bool stereo, bool I2S, bool RDS);  // Innitial transmitter settings
    void setFrequency(uint16_t frequency);               // Range from 760-1080 equals 76MHz - 108MHz
    void setPower(uint8_t power);                        // Range from 0-15 equals 124-101.5 dBµV

    void setDeviation(uint8_t totaldev);                 // Range from 0-255 default is 108
    void setDevRDS(uint8_t rdsdev);                      // Range from 0-127
    void setDevPilot(uint8_t pilotdev);                  // Valid input 7, 8, 9 or 10 (7% - 10% of 75kHz)
    
    // Audio Setup

    void setMute(bool mute);           // Mute the audio. 
    void setDAsrate(uint8_t samprate); // Set Audio sample rate in QN8007 Master mode: 32, 40, 44.1, 48

    // RDS Setup
 
    void setRDS_PS (char ps[], uint16_t newPTY, bool useB0, bool stereo);   // Send your PS and PTY, chose group B0 or A0 for PS
    void setRDS_RT (char rt[], uint16_t newPTY, bool clrTXT);  // Send your Radio Text

    
    uint8_t getPScomplete(bool reset); // Get the number of PS transmissions in a sequnence, reset counter
    uint8_t getRTcomplete(bool reset); // Get the number of RT transmissions in a sequnence, reset counter
  
private:

	uint8_t I2C_address;        // I2C Address of the QN8007
    uint8_t pstxC;              // Counter for of transmitted PS groups A0 or B0
    uint8_t rttxC;              // Counter for of transmitted RT group A2
    uint8_t rdsPI_h;            // PI Code high byte
    uint8_t rdsPI_l;            // PI Code low byte
    
    uint8_t buf[32];             // Data buffer for I2C
    void SendData_(uint8_t len); // Function for sending data to registers

};


#endif
