# QN8007

QN8007 FM transmitter IC simple library for ARDUINO

The QN8007 is the transmitter only version of the QN8006. Since all registers and the device ID are identical this library will work for both ICs in transmitter mode.

Please take note that each of these ICs has "L" and non "L" versions. Only the non-"L" versions support RDS!
They are not marked, so you need to use the software to read out register 0x06 and check the CIDs. I learned the hard way that a bunch of ICs bought in China were "L" and do not support RDS. They work well as non-RDS FM transmitters nevertheless.

This library supports the following functionality:

- Setting up transmission with analog or digital I2S inputs
- Setting  frequency between 76 and 108 MHz
- Changing RF settings like deviation, pilot deviation and RF output power
- Tuning the crystal frequency to get a precise output frequency
- Choice of different crystal frequencies for the main passive crystal...However:

Please take note that I have not been able to run the chip on any other frequency than the default 26MHz. Other crystals give unpredictable results like totally wrong output frequency.

- Selection of an external clock input instead of the crystal. This is useful when running more than one IC in the same circuit
- RDS PS, PTY and Radio Text support
- RDS deviation setup. This may need to be increased to 8-10 from the default 6.

I have found that some newer radios seem to have issues with PS type 0B blocks. The default is 0A even if the alternative frequencies setup is not used in this mode.

This IC is ideal for use with an internet radio program running on a ESP32 such as Karadio. Due to the I2S output being processed directly in digital format the resulting FM audio signal quality is excellent.

Have fun

Disclaimer:

Please observe regulations related to RF emissions in your region!

