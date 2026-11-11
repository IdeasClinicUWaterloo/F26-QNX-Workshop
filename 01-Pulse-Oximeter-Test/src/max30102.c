/******************************************************************************* * 
 * @note        CREDIT & DERIVATION NOTICE:
 *              This code is a port and modification of original work created by:
 *              Original Author: DFRobot
 *              Original Source: https://github.com/DFRobot/DFRobot_MAX30102
 * 
 *              The original implementation was designed for the Arduino platform.
 *              It has been ported, refactored, and adapted for 
 *              the QNX Neutrino RTOS running on a Raspberry Pi.
 * 
 * @platform    QNX Neutrino RTOS (Raspberry Pi)
 * @original    Arduino Architecture
 *******************************************************************************/

#include <max30102.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <max30102_hr.h>
#include <rpi_i2c.h>
#include <rpi_gpio.h>
#include <math.h>

#define min(x,y) ((x) < (y) ? (x) : (y))
#define FreqS 25    //sampling frequency
#define BUFFER_SIZE (FreqS * 4) 
#define MA4_SIZE 4 // DONOT CHANGE
#define MAX30102_GPIO_INT_PIN GPIO17
#define MIN_CONSECT_SAMPLES 3

typedef struct {
    uint32_t red[MAX30102_SENSE_BUF_SIZE];
    uint32_t IR[MAX30102_SENSE_BUF_SIZE];
    uint8_t head;
    uint8_t tail;
} sSenseBuf_t;

// Define possible event types for the system
enum sample_event_t{
    EVENT_HEARTRATE_SPO2, // Event triggered by button press
}; 

//uch_spo2_table is approximated as  -45.060*ratioAverage* ratioAverage + 30.354 *ratioAverage + 94.845 ;
const uint8_t uch_spo2_table[184]={ 95, 95, 95, 96, 96, 96, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98, 99, 99, 99, 99, 
              99, 99, 99, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 
              100, 100, 100, 100, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 98, 98, 98, 97, 97, 
              97, 97, 96, 96, 96, 96, 95, 95, 95, 94, 94, 94, 93, 93, 93, 92, 92, 92, 91, 91, 
              90, 90, 89, 89, 89, 88, 88, 87, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81, 
              80, 80, 79, 78, 78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 70, 69, 69, 68, 67, 
              66, 66, 65, 64, 63, 62, 62, 61, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50, 
              49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 31, 30, 29, 
              28, 27, 26, 25, 23, 22, 21, 20, 19, 17, 16, 15, 14, 12, 11, 10, 9, 7, 6, 5, 
              3, 2, 1 } ;

static  int32_t an_x[ BUFFER_SIZE]; //ir
static  int32_t an_y[ BUFFER_SIZE]; //red

sSenseBuf_t senseBuf; //Buffer for storing multiple groups of array
uint8_t _activeLEDs;

static int chid;  // Channel ID
static int coid;  // Connection ID

MAX30102_t max30102_heartbeat = {0};

bool writeReg(uint8_t reg, const uint8_t *pBuf, uint8_t size){
    if (pBuf == NULL || size == 0) {
        perror("Invalid data");
        return false;
    }

    int response;
    if (size == 1) {
        response = smbus_write_byte_data(BUS, MAX30102_IIC_ADDRESS, reg, *pBuf );
    } else {
        response = smbus_write_block_data(BUS, MAX30102_IIC_ADDRESS, reg, pBuf, size);
    }

    if (response != I2C_SUCCESS) {
        perror("I2C failed");
        return false;
    }

    return true;
}

bool readReg(uint8_t reg, uint8_t *pBuf, uint8_t size){
    if (pBuf == NULL || size == 0) {
        perror("Invalid data");
        return false;
    }

    int response;
    if (size == 1) {
        response = smbus_read_byte_data(BUS, MAX30102_IIC_ADDRESS, reg, pBuf);
    } else {
        response = smbus_read_block_data( BUS, MAX30102_IIC_ADDRESS, reg, pBuf, size);
    }

    if (response != I2C_SUCCESS) {
        perror("I2C failed");
        return false;
    }

    return true;
}

bool init_channel(void){
    // Create a private communication channel
    chid = ChannelCreate(_NTO_CHF_PRIVATE);
    if (chid == -1){
        perror("ChannelCreate");
        return false;
    }

    // Attach a connection to the created channel
    coid = ConnectAttach(0, 0, chid, _NTO_SIDE_CHANNEL, 0);
    if (coid == -1){
        perror("ConnectAttach");
        return false;
    }

    return true;
}

bool MAX30102_softReset(void){
    uint8_t resetValue = MAX30102_RESET; // 0x40

    if (!writeReg(MAX30102_MODECONFIG, &resetValue, 1)) return false;

    uint8_t mode = 0;
    for (int i = 0; i < 100; i++) {
        if (!readReg(MAX30102_MODECONFIG, &mode, 1))
            return false;

        if ((mode & MAX30102_RESET) == 0)
            return true;

        usleep(1000);
    }

    perror("Sensor timeout");
    return false;
}

bool MAX30102_wakeUp(void){
    uint8_t modeConfigValue = 0;

    if (!readReg(MAX30102_MODECONFIG, &modeConfigValue, 1)) return false;

    // Clear the shutdown bit (bit 7).
    modeConfigValue &= 0x7F;
    return writeReg(MAX30102_MODECONFIG, &modeConfigValue, 1);
}

bool MAX30102_initGPIO(void){
    if (rpi_gpio_setup_pull(MAX30102_GPIO_INT_PIN,GPIO_IN,GPIO_PUD_UP) != GPIO_SUCCESS) {
        perror("Failed to configure interrupt pin: GPIO 17\n");
        return false;
    }

    return true;
}
bool MAX30102_enablePPGInterrupt(void){
    uint8_t config = (1u << 6);
    return writeReg(MAX30102_INTENABLE1, &config, 1);
}

bool MAX30102_begin(void){
    if (!MAX30102_softReset()) {
        perror("Error: failed to reset sensor.\n");
        return 0;
    }

    if (!MAX30102_wakeUp()) {
        perror("Error: failed to wake up sensor.\n");
        return 0;
    }

    if(!init_channel()){
        perror("Error: failed to initialize channel.\n");
        return 0;
    }

    if(!MAX30102_initGPIO()){
        perror("Error: failed to configure interrupt pin.\n");
        return 0; 
    }

    if (rpi_gpio_add_event_detect(MAX30102_GPIO_INT_PIN, coid, GPIO_FALLING, EVENT_HEARTRATE_SPO2) != GPIO_SUCCESS){
        perror("rpi_gpio_add_event_detect");
        return 0;
    }


    return 1;
}

bool MAX30102_resetFIFO(void) {
    uint8_t zero = 0x00;

    if (!writeReg(MAX30102_FIFOWRITEPTR, &zero, 1))
        return false;

    if (!writeReg(MAX30102_FIFOOVERFLOW, &zero, 1))
        return false;

    if (!writeReg(MAX30102_FIFOREADPTR, &zero, 1))
        return false;

    return true;
}

bool MAX30102_setFIFOAverage(uint8_t numberOfSamples) {
    if (numberOfSamples > MAX30102_SAMPLEAVG_32) return false;

    uint8_t config = 0;
    if (!readReg(MAX30102_FIFOCONFIG, &config, sizeof(config)))  return false;

    // Clear bits 7:5
    config &= 0x1F;
    config |= (numberOfSamples << 5);

    return writeReg(MAX30102_FIFOCONFIG, &config, sizeof(config));
}

bool MAX30102_setPulseAmplitudeRed(uint8_t amplitude) {
    return writeReg(MAX30102_LED1_PULSEAMP, &amplitude, sizeof(amplitude));
}

bool MAX30102_setPulseAmplitudeIR(uint8_t amplitude) {
    return writeReg(MAX30102_LED2_PULSEAMP,&amplitude, sizeof(amplitude));
}

bool MAX30102_setLEDMode(uint8_t ledMode) {
    if(!writeReg(MAX30102_MODECONFIG, &ledMode, sizeof(ledMode))) return 0; 
    
    if(ledMode == MAX30102_MODE_REDONLY) _activeLEDs = 1;
        
    if(ledMode == MAX30102_MODE_RED_IR  || ledMode == MAX30102_MODE_MULTILED) _activeLEDs = 2;

    return 1;
}
  
bool MAX30102_enableFIFORollover(void) {
    // Read current FIFO configuration
    uint8_t config = 0;
    if (!readReg(MAX30102_FIFOCONFIG,&config, sizeof(config))) return 0;

    config |= (1 << 4);
    return writeReg(MAX30102_FIFOCONFIG,&config, sizeof(config));
}

  /*!
   *@brief Use macro definition to configure sensor 
   *@param ledBrightness LED brightness, default value: 0x1F（6.4mA), Range: 0~255（0=Off, 255=50mA）
   *@param sampleAverage Average multiple samples then draw once, reduce data throughput, default 4 samples average
   *@param ledMode LED mode, default to use red light and IR at the same time 
   *@param MAX30102_SAMPLERATE Sampling rate, default 400 samples every second 
   *@param pulseWidth Pulse width: the longer the pulse width, the wider the detection range. Default to be Max range
   *@param adcRange ADC Measurement Range, default 4096 (nA), 15.63(pA) per LSB
   */

bool MAX30102_sensorConfiguration(uint8_t ledBrightness, uint8_t sampleAverage, uint8_t ledMode, uint8_t sampleRate, uint8_t pulseWidth, uint8_t adcRange){    
    if(!MAX30102_setFIFOAverage(sampleAverage)){
        perror("Error: failed to set fifo avg.\n");
        return 0;
    }

    uint8_t config = (adcRange << 5) | (sampleRate << 2) | pulseWidth;
    if (!writeReg(MAX30102_PARTICLECONFIG, &config, sizeof(config))){
        perror("Error: failed to configure particle sensor.");
        return 0;
    }

    bool red_resp = MAX30102_setPulseAmplitudeRed(ledBrightness);
    bool ir_resp = MAX30102_setPulseAmplitudeIR(ledBrightness);
    if(!red_resp || !ir_resp){
        perror("Error: failed to set current for red and/or IR LED.\n");
        return 0;
    }

    if(!MAX30102_enableFIFORollover()){
        perror("Error: failed to enable fifo rollover.\n");
        return 0;
    }

    if(!MAX30102_resetFIFO()){
        perror("Error: failed to reset fifo.\n");
        return 0;
    }

    if(!MAX30102_setLEDMode(ledMode)){
        perror("Error: failed to start measuring.\n");
        return 0;
    }

    // Clear any pending interrupts from startup/configuration
    uint8_t status = 0;

    if (!readReg(MAX30102_INTSTAT1, &status, 1)) {
        perror("Failed to clear pending interrupts");
        return 0;
    }

    // Enable PPG_RDY interrupt
    if (!MAX30102_enablePPGInterrupt()) {
        perror("Failed to enable PPG_RDY");
        return 0;
    }

    uint8_t enable = 0;
    if (!readReg(MAX30102_INTENABLE1, &enable, 1))
        return false;

    return 1; 
}

bool MAX30102_waitForInterrupt(void){
    struct _pulse pulse;
    for (;;) {
        if (MsgReceivePulse(chid, &pulse, sizeof(pulse), NULL) == -1){
            perror("MsgReceivePulse()");
            return 0;
        }

        // Interrupt triggered 
        if (pulse.code == _PULSE_CODE_MINAVAIL && pulse.value.sival_int == EVENT_HEARTRATE_SPO2) {
            return 1;
        } 
        
        perror("Unexpected fault");
        return 0; 
    }
}

bool MAX30102_getWritePointer(uint8_t * byteTemp){
  if(!readReg(MAX30102_FIFOWRITEPTR, byteTemp, 1)){
    perror("Error: Failed to get writer pointer.\n");
    return 0; 
  }

  return 1;
}

bool MAX30102_getReadPointer(uint8_t * byteTemp){
    if(!readReg(MAX30102_FIFOREADPTR, byteTemp, 1)){
        perror("Error: Failed to get writer pointer.\n");
        return 0; 
    }

    return 1;
}

bool MAX30102_getNewData(void) {
    uint8_t readPointer = 0;
    uint8_t writePointer = 0;

    if(!MAX30102_getReadPointer(&readPointer)) {
        return 0; 
    }

    if(!MAX30102_getWritePointer(&writePointer)) {
        return 0; 
    }

    if (readPointer == writePointer) {
        return true; // No data available right now, not a hard failure
    }

    int32_t numberOfSamples = (writePointer - readPointer + 32) % 32;
    
    for (int s = 0; s < numberOfSamples; s++) {
        uint8_t temp[6] = {0};
        uint8_t bytes = (_activeLEDs > 1) ? 6 : 3;

        if (!readReg(MAX30102_FIFODATA, temp, bytes))
            return false;

        uint32_t red_val = ((uint32_t)temp[0] << 16) |((uint32_t)temp[1] << 8) |temp[2];
        uint32_t ir_val = 0;

        if (_activeLEDs > 1) {
            ir_val =((uint32_t)temp[3] << 16) |((uint32_t)temp[4] << 8) | temp[5];
        }

        uint8_t next = (senseBuf.head + 1) % MAX30102_SENSE_BUF_SIZE;

        // If software buffer is full, discard oldest sample
        if (next == senseBuf.tail) {
            senseBuf.tail = (senseBuf.tail + 1) % MAX30102_SENSE_BUF_SIZE;
        }

        senseBuf.red[senseBuf.head] = red_val & 0x3FFFF;
        senseBuf.IR[senseBuf.head]  = ir_val & 0x3FFFF;

        senseBuf.head = next;
    }

    return true;
}

/*********************************************************************
                 ALGORTHIM FOR HEART RATE & SPO2 
*********************************************************************/

void maxim_heart_rate_and_oxygen_saturation(uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint32_t *pun_red_buffer, int32_t *pn_spo2, int8_t *pch_spo2_valid, 
                int32_t *pn_heart_rate, int8_t *pch_hr_valid){
  uint32_t un_ir_mean;
  int32_t k, n_i_ratio_count;
  int32_t i, n_exact_ir_valley_locs_count, n_middle_idx;
  int32_t n_th1, n_npks;   
  int32_t an_ir_valley_locs[15] ;
  int32_t n_peak_interval_sum;
  
  int32_t n_y_ac, n_x_ac;
  int32_t n_spo2_calc; 
  int32_t n_y_dc_max, n_x_dc_max; 
  int32_t n_y_dc_max_idx = 0;
  int32_t n_x_dc_max_idx = 0; 
  int32_t an_ratio[5], n_ratio_average; 
  int32_t n_nume, n_denom ;

  // calculates DC mean and subtract DC from ir
  un_ir_mean =0; 
  for (k=0 ; k<n_ir_buffer_length ; k++ ) un_ir_mean += pun_ir_buffer[k] ;
  un_ir_mean =un_ir_mean/n_ir_buffer_length ;
    
  // remove DC and invert signal so that we can use peak detector as valley detector
  for (k=0 ; k<n_ir_buffer_length ; k++ )  
    an_x[k] = -1*(pun_ir_buffer[k] - un_ir_mean) ; 
    
  // 4 pt Moving Average
  for(k=0; k< BUFFER_SIZE-MA4_SIZE; k++){
    an_x[k]=( an_x[k]+an_x[k+1]+ an_x[k+2]+ an_x[k+3])/(int32_t)4;        
  }
  // calculate threshold  
  n_th1=0; 
  for ( k=0 ; k<BUFFER_SIZE ;k++){
    n_th1 +=  an_x[k];
  }
  n_th1=  n_th1/ ( BUFFER_SIZE);
  if( n_th1<30) n_th1=30; // min allowed
  if( n_th1>60) n_th1=60; // max allowed

  for ( k=0 ; k<15;k++) an_ir_valley_locs[k]=0;
  // since we flipped signal, we use peak detector as valley detector
  maxim_find_peaks( an_ir_valley_locs, &n_npks, an_x, BUFFER_SIZE, n_th1, 4, 15 );//peak_height, peak_distance, max_num_peaks 
  n_peak_interval_sum =0;
  if (n_npks>=2){
    for (k=1; k<n_npks; k++) n_peak_interval_sum += (an_ir_valley_locs[k] -an_ir_valley_locs[k -1] ) ;
    n_peak_interval_sum =n_peak_interval_sum/(n_npks-1);
    *pn_heart_rate =(int32_t)( (FreqS*60)/ n_peak_interval_sum );
    *pch_hr_valid  = 1;
  }
  else  { 
    *pn_heart_rate = -999; // unable to calculate because # of peaks are too small
    *pch_hr_valid  = 0;
  }

  //  load raw value again for SPO2 calculation : RED(=y) and IR(=X)
  for (k=0 ; k<n_ir_buffer_length ; k++ )  {
      an_x[k] =  pun_ir_buffer[k] ; 
      an_y[k] =  pun_red_buffer[k] ; 
  }

  // find precise min near an_ir_valley_locs
  n_exact_ir_valley_locs_count =n_npks; 
  
  //using exact_ir_valley_locs , find ir-red DC andir-red AC for SPO2 calibration an_ratio
  //finding AC/DC maximum of raw

  n_ratio_average =0; 
  n_i_ratio_count = 0; 
  for(k=0; k< 5; k++) an_ratio[k]=0;
  for (k=0; k< n_exact_ir_valley_locs_count; k++){
    if (an_ir_valley_locs[k] > BUFFER_SIZE ){
      *pn_spo2 =  -999 ; // do not use SPO2 since valley loc is out of range
      *pch_spo2_valid  = 0; 
      return;
    }
  }
  // find max between two valley locations 
  // and use an_ratio betwen AC compoent of Ir & Red and DC compoent of Ir & Red for SPO2 
  for (k=0; k< n_exact_ir_valley_locs_count-1; k++){
    n_y_dc_max= -16777216 ; 
    n_x_dc_max= -16777216; 
    if (an_ir_valley_locs[k+1]-an_ir_valley_locs[k] >3){
        for (i=an_ir_valley_locs[k]; i< an_ir_valley_locs[k+1]; i++){
          if (an_x[i]> n_x_dc_max) {n_x_dc_max =an_x[i]; n_x_dc_max_idx=i;}
          if (an_y[i]> n_y_dc_max) {n_y_dc_max =an_y[i]; n_y_dc_max_idx=i;}
      }
      n_y_ac= (an_y[an_ir_valley_locs[k+1]] - an_y[an_ir_valley_locs[k] ] )*(n_y_dc_max_idx -an_ir_valley_locs[k]); //red
      n_y_ac=  an_y[an_ir_valley_locs[k]] + n_y_ac/ (an_ir_valley_locs[k+1] - an_ir_valley_locs[k])  ; 
      n_y_ac=  an_y[n_y_dc_max_idx] - n_y_ac;    // subracting linear DC compoenents from raw 
      n_x_ac= (an_x[an_ir_valley_locs[k+1]] - an_x[an_ir_valley_locs[k] ] )*(n_x_dc_max_idx -an_ir_valley_locs[k]); // ir
      n_x_ac=  an_x[an_ir_valley_locs[k]] + n_x_ac/ (an_ir_valley_locs[k+1] - an_ir_valley_locs[k]); 
      n_x_ac=  an_x[n_y_dc_max_idx] - n_x_ac;      // subracting linear DC compoenents from raw 
      n_nume=( n_y_ac *n_x_dc_max)>>7 ; //prepare X100 to preserve floating value
      n_denom= ( n_x_ac *n_y_dc_max)>>7;
      if (n_denom>0  && n_i_ratio_count <5 &&  n_nume != 0)
      {   
        an_ratio[n_i_ratio_count]= (n_nume*100)/n_denom ; //formular is ( n_y_ac *n_x_dc_max) / ( n_x_ac *n_y_dc_max) ;
        n_i_ratio_count++;
      }
    }
  }
  // choose median value since PPG signal may varies from beat to beat
  maxim_sort_ascend(an_ratio, n_i_ratio_count);
  n_middle_idx= n_i_ratio_count/2;

  if (n_middle_idx >1)
    n_ratio_average =( an_ratio[n_middle_idx-1] +an_ratio[n_middle_idx])/2; // use median
  else
    n_ratio_average = an_ratio[n_middle_idx ];

  if( n_ratio_average>2 && n_ratio_average <184){
    n_spo2_calc= uch_spo2_table[n_ratio_average] ;
    *pn_spo2 = n_spo2_calc ;
    *pch_spo2_valid  = 1;//  float_SPO2 =  -45.060*n_ratio_average* n_ratio_average/10000 + 30.354 *n_ratio_average/100 + 94.845 ;  // for comparison with table
  }
  else{
    *pn_spo2 =  -999 ; // do not use SPO2 since signal an_ratio is out of range
    *pch_spo2_valid  = 0; 
  }
}


void maxim_find_peaks( int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, int32_t n_size, int32_t n_min_height, int32_t n_min_distance, int32_t n_max_num )
/**
* \brief        Find peaks
* \par          Details
*               Find at most MAX_NUM peaks above MIN_HEIGHT separated by at least MIN_DISTANCE
*
* \retval       None
*/
{
  maxim_peaks_above_min_height( pn_locs, n_npks, pn_x, n_size, n_min_height );
  maxim_remove_close_peaks( pn_locs, n_npks, pn_x, n_min_distance );
  *n_npks = min( *n_npks, n_max_num );
}

void maxim_peaks_above_min_height( int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, int32_t n_size, int32_t n_min_height )
/**
* \brief        Find peaks above n_min_height
* \par          Details
*               Find all peaks above MIN_HEIGHT
*
* \retval       None
*/
{
  int32_t i = 1, n_width;
  *n_npks = 0;
  
  while (i < n_size-1){
    if (pn_x[i] > n_min_height && pn_x[i] > pn_x[i-1]){      // find left edge of potential peaks
      n_width = 1;
      while (i+n_width < n_size && pn_x[i] == pn_x[i+n_width])  // find flat peaks
        n_width++;
      if (i + n_width < n_size && pn_x[i] > pn_x[i+n_width] && (*n_npks) < 15) {      // find right edge of peaks
        pn_locs[(*n_npks)++] = i;    
        // for flat peaks, peak location is left edge
        i += n_width+1;
      }
      else
        i += n_width;
    }
    else
      i++;
  }
}

void maxim_remove_close_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_min_distance)
/**
* \brief        Remove peaks
* \par          Details
*               Remove peaks separated by less than MIN_DISTANCE
*
* \retval       None
*/
{
    
  int32_t i, j, n_old_npks, n_dist;
    
  /* Order peaks from large to small */
  maxim_sort_indices_descend( pn_x, pn_locs, *pn_npks );

  for ( i = -1; i < *pn_npks; i++ ){
    n_old_npks = *pn_npks;
    *pn_npks = i+1;
    for ( j = i+1; j < n_old_npks; j++ ){
      n_dist =  pn_locs[j] - ( i == -1 ? -1 : pn_locs[i] ); // lag-zero peak of autocorr is at index -1
      if ( n_dist > n_min_distance || n_dist < -n_min_distance )
        pn_locs[(*pn_npks)++] = pn_locs[j];
    }
  }

  // Resort indices int32_to ascending order
  maxim_sort_ascend( pn_locs, *pn_npks );
}

void maxim_sort_ascend(int32_t  *pn_x, int32_t n_size) 
/**
* \brief        Sort array
* \par          Details
*               Sort array in ascending order (insertion sort algorithm)
*
* \retval       None
*/
{
  int32_t i, j, n_temp;
  for (i = 1; i < n_size; i++) {
    n_temp = pn_x[i];
    for (j = i; j > 0 && n_temp < pn_x[j-1]; j--)
        pn_x[j] = pn_x[j-1];
    pn_x[j] = n_temp;
  }
}

void maxim_sort_indices_descend(  int32_t  *pn_x, int32_t *pn_indx, int32_t n_size)
/**
* \brief        Sort indices
* \par          Details
*               Sort indices according to descending order (insertion sort algorithm)
*
* \retval       None
*/ 
{
  int32_t i, j, n_temp;
  for (i = 1; i < n_size; i++) {
    n_temp = pn_indx[i];
    for (j = i; j > 0 && pn_x[n_temp] > pn_x[pn_indx[j-1]]; j--)
      pn_indx[j] = pn_indx[j-1];
    pn_indx[j] = n_temp;
  }
}

void MAX30102_heartrateAndOxygenSaturation(
    int32_t *SPO2,
    bool *SPO2Valid,
    int32_t *heartRate,
    bool *heartRateValid
) {
    if (!SPO2 || !SPO2Valid || !heartRate || !heartRateValid)
        return;

    *SPO2 = -999;
    *SPO2Valid = false;
    *heartRate = -999;
    *heartRateValid = false;

    int32_t irBuffer[BUFFER_SIZE];
    uint32_t irMaximBuffer[BUFFER_SIZE];
    uint32_t redBuffer[BUFFER_SIZE];

    int sample_count = 0;
    int invalid_samples_count = 0; 

    // Collect 100 consecutive valid samples
    while (sample_count < BUFFER_SIZE) {

        while (senseBuf.tail != senseBuf.head &&
               sample_count < BUFFER_SIZE) {

            uint32_t ir = senseBuf.IR[senseBuf.tail];
            uint32_t red = senseBuf.red[senseBuf.tail];

            senseBuf.tail =
                (senseBuf.tail + 1) % MAX30102_SENSE_BUF_SIZE;

            // Finger removed: discard incomplete recording
            if (ir < IR_THRESHOLD) {
                invalid_samples_count++;

                // Confirm finger removal after consecutive invalid samples
                if (invalid_samples_count == MIN_CONSECT_SAMPLES) {
                    printf("No finger detected. Waiting...\n");
                }

                sample_count = 0;
                init_max3012_heartrate(&max30102_heartbeat);

                continue;
            }

            // Valid IR reading
            invalid_samples_count = 0;

            // Store valid sample
            irBuffer[sample_count] = (int32_t)ir;
            irMaximBuffer[sample_count] = ir;
            redBuffer[sample_count] = red;

            sample_count++;
        }

        if (sample_count == BUFFER_SIZE)
            break;

        if (!MAX30102_waitForInterrupt()) {
            init_max3012_heartrate(&max30102_heartbeat);
            return;
        }

        if (!MAX30102_getNewData()) {
            init_max3012_heartrate(&max30102_heartbeat);
            return;
        }
    }

    // Maxim algorithm (SpO2)
    int8_t spo2_valid = 0;
    int8_t maxim_hr_valid = 0;
    int32_t maxim_hr = -999;

    maxim_heart_rate_and_oxygen_saturation(
        irMaximBuffer,
        BUFFER_SIZE,
        redBuffer,
        SPO2,
        &spo2_valid,
        &maxim_hr,
        &maxim_hr_valid
    );

    *SPO2Valid = spo2_valid;

    max3012_heartrate(&max30102_heartbeat, irBuffer);
    double bpm = max30102_heartbeat.heartrate;
    if (isfinite(bpm) && bpm > 0) {
        *heartRate = (int32_t)(bpm + 0.5);
        *heartRateValid = true;
    }
}