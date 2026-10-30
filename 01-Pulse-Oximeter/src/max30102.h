
#include <stdbool.h>
#include <stdint.h>

#ifndef MAX30102_H
#define MAX30102_H

#define BUS 1
#define MAX30102_IIC_ADDRESS  0x57 //I2C Address
#define MAX30102_RESET 0x40

//Status Registers
#define MAX30102_INTSTAT1        0x00//Interrupt Status1
#define MAX30102_INTSTAT2        0x01//Interrupt Status2
#define MAX30102_INTENABLE1      0x02//Interrupt Enable1
#define MAX30102_INTENABLE2      0x03//Interrupt Enable2

//FIFO Registers
#define MAX30102_FIFOWRITEPTR    0x04//FIFO Write Pointer, The FIFO Write Pointer points to the location where the MAX30102 writes the next sample. This pointer advances for each sample pushed on to the FIFO. It can also be changed through the I2C interface when MODE[2:0] is 010, 011, or 111.
#define MAX30102_FIFOOVERFLOW    0x05//FIFO Overflow Counter, When the FIFO is full, samples are not pushed on to the FIFO, samples are lost. OVF_COUNTER counts the number of samples lost. It saturates at 0x1F. When a complete sample is “popped” (i.e., removal of old FIFO data and shifting the samples down) from the FIFO (when the read pointer advances), OVF_COUNTER is reset to zero.
#define MAX30102_FIFOREADPTR     0x06//FIFO Read Pointer, The FIFO Read Pointer points to the location from where the processor gets the next sample from the FIFO through the I2C interface. This advances each time a sample is popped from the FIFO. The processor can also write to this pointer after reading the samples to allow rereading samples from the FIFO if there is a data communication error.
#define MAX30102_FIFODATA        0x07//FIFO Data Register, The circular FIFO depth is 32 and can hold up to 32 samples of data. The sample size depends on the number of LED channels (a.k.a. channels) configured as active. As each channel signal is stored as a 3-byte data signal, the FIFO width can be 3 bytes or 6 bytes in size.

//Configuration Registers
#define MAX30102_FIFOCONFIG      0x08//FIFO Configuration
#define MAX30102_MODECONFIG      0x09//Mode Configuration
#define MAX30102_PARTICLECONFIG  0x0A//SpO2 Configuration
#define MAX30102_LED1_PULSEAMP   0x0C//LED1 Pulse Amplitude (Red LED)
#define MAX30102_LED2_PULSEAMP   0x0D//LED2 Pulse Amplitude (IR LED)
#define MAX30102_LED3_PULSEAMP   0x0E//RESERVED
#define MAX30102_LED_PROX_AMP    0x10//RESERVED
#define MAX30102_MULTILEDCONFIG1 0x11//Multi-LED Mode Control Registers
#define MAX30102_MULTILEDCONFIG2 0x12//Multi-LED Mode Control Registers
//Die Temperature Registers
#define MAX30102_DIETEMPINT      0x1F//Die Temp Integer
#define MAX30102_DIETEMPFRAC     0x20//Die Temp Fraction
#define MAX30102_DIETEMPCONFIG   0x21//Die Temperature Config
#define MAX30102_PROXINTTHRESH   0x30//RESERVED
//Part ID Registers
#define MAX30102_REVISIONID      0xFE//Revision ID
#define MAX30102_PARTID          0xFF//Part ID:0x15
#define MAX30102_EXPECTED_PARTID  0x15

//FIFO Configuration(Register address 0x08)
//sampleAverage(Table 3. Sample Averaging)
#define MAX30102_SAMPLEAVG_1     0
#define MAX30102_SAMPLEAVG_2     1
#define MAX30102_SAMPLEAVG_4     2
#define MAX30102_SAMPLEAVG_8     3
#define MAX30102_SAMPLEAVG_16    4
#define MAX30102_SAMPLEAVG_32    5

//ledMode(Table 4. Mode Control)
#define MAX30102_MODE_REDONLY    2
#define MAX30102_MODE_RED_IR     3
#define MAX30102_MODE_MULTILED   7

//pulseWidth(Table 7. LED Pulse Width Control)
#define MAX30102_PULSEWIDTH_69   0 
#define MAX30102_PULSEWIDTH_118  1
#define MAX30102_PULSEWIDTH_215  2
#define MAX30102_PULSEWIDTH_411  3

//Particle sensing configuration(Register address 0x0A)
//adcRange(Table 5. SpO2 ADC Range Control)

#define MAX30102_ADCRANGE_2048   0
#define MAX30102_ADCRANGE_4096   1
#define MAX30102_ADCRANGE_8192   2
#define MAX30102_ADCRANGE_16384  3

//sample rate (Table 6. SpO2 Sample Rate Control)
#define MAX30102_SAMPLERATE_50   0 
#define MAX30102_SAMPLERATE_100  1
#define MAX30102_SAMPLERATE_200  2
#define MAX30102_SAMPLERATE_400  3
#define MAX30102_SAMPLERATE_800  4
#define MAX30102_SAMPLERATE_1000 5
#define MAX30102_SAMPLERATE_1600 6
#define MAX30102_SAMPLERATE_3200 7
#define MAX30102_SENSE_BUF_SIZE  30

bool MAX30102_begin(void);
void MAX30102_int_test(void);
bool MAX30102_sensorConfiguration(uint8_t ledBrightness, uint8_t sampleAverage, uint8_t ledMode, uint8_t sampleRate, uint8_t pulseWidth, uint8_t adcRange);
void MAX30102_heartrateAndOxygenSaturation(int32_t *SPO2,int8_t *SPO2Valid,int32_t *heartRate,int8_t *heartRateValid);

#endif