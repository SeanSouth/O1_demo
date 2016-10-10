/**
****************************************************************************************
*
* @file rf_ripple.c
*
* @brief Ripple radio initialization and specific functions
*
* Copyright (C) RivieraWaves 2009-2013
*
*
****************************************************************************************
*/

/**
****************************************************************************************
* @addtogroup RF_RIPPLE
* @ingroup RF
* @brief Ripple Radio Driver
*
* This is the driver block for Ripple radio
* @{
****************************************************************************************
*/

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"        // RW SW configuration

#ifdef RADIO_RIPPLE
#include <string.h>             // for memcpy
#include "co_utils.h"           // common utility definition
#include "co_math.h"            // common math functions
#include "co_endian.h"          // endian definitions
#include "rf.h"                 // RF interface
#include "plf.h"                // Platform functions

#include "rwip.h"               // for RF API structure definition
#include "arch.h"

#if (BLE_EMB_PRESENT)
#include "reg_blecore.h"        // ble core registers
#include "reg_ble_em_cs.h"      // control structure definitions
#endif //BLE_EMB_PRESENT

#if (BT_EMB_PRESENT)
#include "lc_epc.h"             // Enhanced Power Control definitions
#include "reg_btcore.h"         // bt core registers
#include "reg_bt_em_cs.h"       // control structure definitions
#endif //BT_EMB_PRESENT

// Ripple register definitions and access functions
 uint32_t rf_rpl_reg_rd (uint16_t addr);
 void rf_rpl_reg_wr (uint16_t addr, uint32_t value);

#define REG_RPL_RD                rf_rpl_reg_rd
#define REG_RPL_WR                rf_rpl_reg_wr

#include "reg_ripple.h"          // ripple register
#include "reg_modem.h"           // modem register

//#include "reg_plf.h"             // platform register

//@wik, includes added by wik
#include "global_io.h"
#include "em_map_ble_user.h"
#include "_reg_common_em_et.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#if (defined(CFG_BLE) && defined(CFG_EMB))
/// BLE read burst
__STATIC_INLINE void em_ble_burst_rd(void *sys_addr, uint16_t em_addr, uint16_t len)
{
    memcpy(sys_addr, (void *)(em_addr + REG_COMMON_EM_ET_BASE_ADDR), len);
}
/// BLE write burst
__STATIC_INLINE void em_ble_burst_wr(void const *sys_addr, uint16_t em_addr, uint16_t len)
{
    memcpy((void *)(em_addr + REG_COMMON_EM_ET_BASE_ADDR), sys_addr, len);
}
#endif // (CFG_BLE && CFG_EMB)

#define RPL_GAIN_TBL_SIZE           0x0F

/// Gain table
static const uint8_t RF_RPL_RX_GAIN_TBL[RPL_GAIN_TBL_SIZE] = {
        [0] = 43,
        [1] = 37,
        [2] = 31,
        [3] = 25,
        [4] = 19,
        [5] = 13,
        [6] = 7,
        [7] = 1};
#if defined(CFG_BT)
#define RF_EM_SPI_OFFSET        (EM_BT_RF_SPI_OFFSET)
#define RF_EM_SPI_ADRESS        (REG_BT_EM_CS_BASE_ADDR + RF_EM_SPI_OFFSET)
#elif defined(CFG_BLE)
#define RF_EM_SPI_OFFSET        (EM_BLE_RF_SPI_OFFSET)
#define RF_EM_SPI_ADRESS        (REG_BLE_EM_CS_BASE_ADDR + RF_EM_SPI_OFFSET)
#endif //CFG_BLE

#define RPL_SPIRD                   0x80
#define RPL_SPIWR                   0x00
#define RPL_RFPLL_TBL_SIZE          0x50
#define RPL_PWR_TBL_SIZE            0x0F

/* The offset value given below is the offset to add to the frequency table index to    */
/* get the value to be programmed in the radio for each channel                         */
#define RPL_FREQTAB_OFFSET           0             /* Offset for Ripple radio            */
#define RPL_RADIO_SKEW               14L           /* radio skew compensation           */

#define RFLOIF                      0x00

#define RPL_RSSI_20dB_THRHLD        -20
#define RPL_RSSI_45dB_THRHLD        -45
#define RPL_RSSI_48dB_THRHLD        -48
#define RPL_RSSI_55dB_THRHLD        -55
#define RPL_RSSI_60dB_THRHLD        -60
#define RPL_RSSI_70dB_THRHLD        -70

#if defined(CFG_BTCORE_30)
// Wake up delay
#define RPL_WK_UP_DELAY             2
#endif //CFG_BTCORE_30

// EDR Control value
#define RPL_EDRCNTL                 0x0D4


/// TX max power
#define RPL_POWER_MAX                0x06
#define RPL_POWER_MIN                0x01
#define RPL_POWER_MSK                0x07

#define RPL_NB_BOARDS                175


/*****************************************************************************************
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************/
extern bool rf_in_sleep;

uint32_t read_value;

/// PLL Lock Table
static uint8_t RFPLL_LOCK_TABLE[RPL_RFPLL_TBL_SIZE];

/// PLL VCOFC table
static uint8_t RFPLL_VCOFC_TABLE[RPL_RFPLL_TBL_SIZE];

/// PLL ICP table
static uint8_t RFPLL_ICP_TABLE[RPL_RFPLL_TBL_SIZE];


/// Power table
static const int8_t RF_RPL_TX_PW_CONV_TBL[RPL_PWR_TBL_SIZE] = {
        [0] = -23,
        [1] = -20,
        [2] = -17,
        [3] = -14,
        [4] = -11,
        [5] = -8,
        [6] = -5,
        [7] = -2};

/// Table storing TXCNTL1 tuning values for all RF boards
static const uint16_t RF_RPL_TXCNTL1_TBL[RPL_NB_BOARDS] =
{   // the index corresponds to the board number!
    [0]   = 0x0000,
    [1]   = 0x0000,
    [2]   = 0x0000,
    [3]   = 0x1C19, // gain 324
    [4]   = 0x0000,
    [5]   = 0x0000, // no dc correction needed
    [6]   = 0x0001, // changed value with new chip
    [7]   = 0x011D,
    [8]   = 0x1A1F, // gain 333
    [9]   = 0x011C,
    [10]  = 0x0000,
    [11]  = 0x021A,
    [12]  = 0x1D01,
    [13]  = 0x011F,
    [14]  = 0x1A1C,
    [15]  = 0x0000,
    [16]  = 0x0000, // no dc correction needed
    [17]  = 0x0400,
    [18]  = 0x1A03,
    [19]  = 0x1F1E,
    [20]  = 0x1E1D,
    [21]  = 0x1D01,
    [22]  = 0x031F,
    [23]  = 0x1F1E,
    [24]  = 0x031A,
    [25]  = 0x1A01,
    [26]  = 0x1B19,
    [27]  = 0x0000, // no dc correction needed
    [28]  = 0x1C1B,
    [29]  = 0x051A,
    [30]  = 0x1D1E,
    [31]  = 0x0000,
    [32]  = 0x0013,
    [33]  = 0x1F00,
    [34]  = 0x1C00,
    [35]  = 0x1A02,
    [36]  = 0x1900,
    [37]  = 0x011E,
    [38]  = 0x1C01,
    [39]  = 0x0000,
    [40]  = 0x011F,
    [41]  = 0x011F,
    [42]  = 0x0100,
    [43]  = 0x1900,
    [44]  = 0x1A19,
    [45]  = 0x1F00,
    [46]  = 0x031B,
    [47]  = 0x1805,
    [48]  = 0x0000,
    [49]  = 0x1B00,
    [50]  = 0x0501,
    [51]  = 0x011E,
    [52]  = 0x0000,
    [53]  = 0x1802,
    [54]  = 0x1A1A,
    [55]  = 0x1904,
    [56]  = 0x1F1B,
    [57]  = 0x1E1B,
    [58]  = 0x0401,
    [59]  = 0x1F19,
    [60]  = 0x1D00,
    [61]  = 0x1D1D,
    [62]  = 0x0100,
    [63]  = 0x1E02,
    [64]  = 0x011D,
    [65]  = 0x011C,
    [66]  = 0x1F1C,
    [67]  = 0x1A01,
    [68]  = 0x1C00,
    [69]  = 0x1F1C,
    [70]  = 0x0117,
    [71]  = 0x1A1D,
    [72]  = 0x0000,
    [73]  = 0x0000,
    [74]  = 0x0000,
    [75]  = 0x0000,
    [76]  = 0x1A19,
    [77]  = 0x0000,
    [78]  = 0x0000,
    [79]  = 0x0000,
    [80]  = 0x0000,
    [81]  = 0x1C01,
    [82]  = 0x0000,
    [83]  = 0x0000,
    [84]  = 0x0000,
    [85]  = 0x0000,
    [86]  = 0x0000,
    [87]  = 0x0000,
    [88]  = 0x0000,
    [89]  = 0x0000,
    [90]  = 0x0000,
    [91]  = 0x0000,
    [92]  = 0x0000,
    [93]  = 0x1C03,
    [94]  = 0x011D,
    [95]  = 0x0000,
    [96]  = 0x1A1D,
    [97]  = 0x191D,
    [98]  = 0x001E,
    [99]  = 0x0000,
    [100] = 0x0000,
    [101] = 0x011D,
    [102] = 0x0000,
    [103] = 0x0000,
    [104] = 0x1A1D,
    [105] = 0x1D1C,
    [106] = 0x011C,
    [107] = 0x0000,
    [108] = 0x0000,
    [109] = 0x0000,
    [110] = 0x0000,
    [111] = 0x0000,
    [112] = 0x0000,
    [113] = 0x0000,
    [114] = 0x0000,
    [115] = 0x0000,
    [116] = 0x0000,
    [117] = 0x0000,
    [118] = 0x0000,
    [119] = 0x0000,
    [120] = 0x0000,
    [121] = 0x0000,
    [122] = 0x0000,
    [123] = 0x0000,
    [124] = 0x0000,
    [125] = 0x0000,
    [126] = 0x0000,
    [127] = 0x0000,
    [128] = 0x0000,
    [129] = 0x0000,
    [130] = 0x0000,
    [131] = 0x0000,
    [132] = 0x0000,
    [133] = 0x0000,
    [134] = 0x0000,
    [135] = 0x1E1F,
    [136] = 0x1C1B,
    [137] = 0x0000,
    [138] = 0x0000,
    [139] = 0x0000,
    [140] = 0x0000,
    [141] = 0x1716,
    [142] = 0x0000,
    [143] = 0x031D,
    [144] = 0x0300,
    [145] = 0x0002,
    [146] = 0x0000,
    [147] = 0x0200,
    [148] = 0x0000,
    [149] = 0x1F1C,
    [150] = 0x0000,
    [151] = 0x0000,
    [152] = 0x1B01,
    [153] = 0x1E05,
    [154] = 0x0000,
    [155] = 0x1D03,
    [156] = 0x0000,
    [157] = 0x0000,
    [158] = 0x0000,
    [159] = 0x1E01,
    [160] = 0x0000,
    [161] = 0x1B1B,
    [162] = 0x0000,
    [163] = 0x0000,
    [164] = 0x071D,
    [165] = 0x1A1C,
    [166] = 0x0000,
    [167] = 0x1A1D,
    [168] = 0x1B00,
    [169] = 0x0000,
    [170] = 0x0000,
    [171] = 0x0000,
    [172] = 0x171F,
    [173] = 0x0006,
    [174] = 0x1E00
};

/// Table to set RSSI threshold depending on RF board
static const uint8_t RF_RPL_RSSI_THR_TBL[RPL_NB_BOARDS] =
{   // the index corresponds to the board number!
    [0]   = 0x29,
    [1]   = 0x29,
    [2]   = 0x29,
    [3]   = 0x29,
    [4]   = 0x00,
    [5]   = 0x29,
    [6]   = 0x29,
    [7]   = 0x29,
    [8]   = 0x29,
    [9]   = 0x29,
    [10]  = 0x00,
    [11]  = 0x29,
    [12]  = 0x29,
    [13]  = 0x29,
    [14]  = 0x29,
    [15]  = 0x29,
    [16]  = 0x29,
    [17]  = 0x29,
    [18]  = 0x29,
    [19]  = 0x29,
    [20]  = 0x29,
    [21]  = 0x29,
    [22]  = 0x29,
    [23]  = 0x29,
    [24]  = 0x29,
    [25]  = 0x29,
    [26]  = 0x29,
    [27]  = 0x29,
    [28]  = 0x1A,
    [29]  = 0x29,
    [30]  = 0x29,
    [31]  = 0x29,
    [32]  = 0x29,
    [33]  = 0x29,
    [34]  = 0x29,
    [35]  = 0x29,
    [36]  = 0x29,
    [37]  = 0x29,
    [38]  = 0x29,
    [39]  = 0x29,
    [40]  = 0x29,
    [41]  = 0x29,
    [42]  = 0x29,
    [43]  = 0x29,
    [44]  = 0x29,
    [45]  = 0x29,
    [46]  = 0x29,
    [47]  = 0x29,
    [48]  = 0x29,
    [49]  = 0x29,
    [50]  = 0x29,
    [51]  = 0x29,
    [52]  = 0x29,
    [53]  = 0x29,
    [54]  = 0x29,
    [55]  = 0x29,
    [56]  = 0x29,
    [57]  = 0x29,
    [58]  = 0x29,
    [59]  = 0x29,
    [60]  = 0x29,
    [61]  = 0x29,
    [62]  = 0x29,
    [63]  = 0x29,
    [64]  = 0x29,
    [65]  = 0x29,
    [66]  = 0x29,
    [67]  = 0x29,
    [68]  = 0x29,
    [69]  = 0x29,
    [70]  = 0x29,
    [71]  = 0x29,
    [72]  = 0x29,
    [73]  = 0x29,
    [74]  = 0x29,
    [75]  = 0x29,
    [76]  = 0x29,
    [77]  = 0x29,
    [78]  = 0x29,
    [79]  = 0x29,
    [80]  = 0x29,
    [81]  = 0x29,
    [82]  = 0x29,
    [83]  = 0x29,
    [84]  = 0x29,
    [85]  = 0x29,
    [86]  = 0x29,
    [87]  = 0x29,
    [88]  = 0x29,
    [89]  = 0x29,
    [90]  = 0x29,
    [91]  = 0x29,
    [92]  = 0x29,
    [93]  = 0x29,
    [94]  = 0x29,
    [95]  = 0x29,
    [96]  = 0x29,
    [97]  = 0x29,
    [98]  = 0x29,
    [99]  = 0x29,
    [100] = 0x29,
    [101] = 0x29,
    [102] = 0x29,
    [103] = 0x29,
    [104] = 0x29,
    [105] = 0x29,
    [106] = 0x29,
    [107] = 0x29,
    [108] = 0x29,
    [109] = 0x29,
    [110] = 0x29,
    [111] = 0x29,
    [112] = 0x29,
    [113] = 0x29,
    [114] = 0x29,
    [115] = 0x29,
    [116] = 0x29,
    [117] = 0x29,
    [118] = 0x29,
    [119] = 0x29,
    [120] = 0x29,
    [121] = 0x29,
    [122] = 0x29,
    [123] = 0x29,
    [124] = 0x29,
    [125] = 0x29,
    [126] = 0x29,
    [127] = 0x29,
    [128] = 0x29,
    [129] = 0x29,
    [130] = 0x29,
    [131] = 0x29,
    [132] = 0x29,
    [133] = 0x29,
    [134] = 0x29,
    [135] = 0x29,
    [136] = 0x29,
    [137] = 0x29,
    [138] = 0x29,
    [139] = 0x29,
    [140] = 0x29,
    [141] = 0x29,
    [142] = 0x29,
    [143] = 0x29,
    [144] = 0x29,
    [145] = 0x29,
    [146] = 0x29,
    [147] = 0x29,
    [148] = 0x29,
    [149] = 0x29,
    [150] = 0x29,
    [151] = 0x29,
    [152] = 0x29,
    [153] = 0x29,
    [154] = 0x29,
    [155] = 0x29,
    [156] = 0x29,
    [157] = 0x29,
    [158] = 0x29,
    [159] = 0x29,
    [160] = 0x29,
    [161] = 0x29,
    [162] = 0x29,
    [163] = 0x29,
    [164] = 0x29,
    [165] = 0x29,
    [166] = 0x29,
    [167] = 0x29,
    [168] = 0x29,
    [169] = 0x29,
    [170] = 0x29,
    [171] = 0x29,
    [172] = 0x29,
    [173] = 0x29,
    [174] = 0x29
};


/*****************************************************************************************
 * FUNCTION DEFINITIONS
 ****************************************************************************************/

/*****************************************************************************************
 * @brief SPI access
 ****************************************************************************************/
//@WIK, this function is not used, spi transfer is done in SW directly iso SPIGO bit,
static void rf_rpl_spi_tf(void)
{
    //launch SPI transfer 

    //Start spi transfer by  RADIOCNTL0.SPIGO
    //ble_spigo_setf(1);
   
    //Wait till spi ready by checking RADIOCNTL0.SPICOMP
    //while (!ble_spicomp_getf());
    
}


/*****************************************************************************************
 * @brief Ripple specific read access
 *
 * @param[in] addr    register address
 *
 * @return uint32_t value
 ****************************************************************************************
 */
//@WIK, this function is complete changed because of different spi activation.
// Is done in SW now iso SPIGO bit
 uint32_t rf_rpl_reg_rd (uint16_t address)
{

// spi format master out 		8bit address - 4bit  0xB - 36bit zero
// spi format master in         12bit unsed  - 32 bit read value - 4 bit unsed
    unsigned short byte_one;
    unsigned short byte_two;
    unsigned short byte_three;
    int a;
    uint32_t slave_value;

    byte_one=((address&0xFF)<<8)+0xB0;				// prepare words
    byte_two=0;
    byte_three=0;

    uint16_t reg = SPI2->SPI2_CTRL_REG;
    REG_SET_FIELD(SPI2, SPI2_CTRL_REG, SPI_WORD, reg, 1);       // set to 16bit mode
    REG_SET_FIELD(SPI2, SPI2_CTRL_REG, SPI_CLK, reg,  2);       // fastest clock
    SPI2->SPI2_CTRL_REG = reg;
    REG_SET_BIT(SPI2->SPI2_CTRL_REG, SPI_ON);                   // enable SPI block

    GPIO->P3_RESET_DATA_REG = 1<<1;				// set cs LOW

    SPI2->SPI2_RX_TX_REG0 = byte_one & 0xFFFF;   		// write TX_REG0, trigger to start
    do{
    }while (REG_GETF(SPI2, SPI2_CTRL_REG, SPI_INT_BIT) == 0);   // polling to wait for spi have data
    SPI2->SPI2_CLEAR_INT_REG = 1;                               // clear pending flag
    a= SPI2->SPI2_RX_TX_REG0;					// get spi data
    slave_value= ((a&0x000F)<<28);				// update return value			   	
    
    SPI2->SPI2_RX_TX_REG0 = byte_two & 0xFFFF;   		// write TX_REG0, trigger to start
    do{
    }while (REG_GETF(SPI2, SPI2_CTRL_REG, SPI_INT_BIT) == 0);   // polling to wait for spi have data
    SPI2->SPI2_CLEAR_INT_REG = 1;   				// clear pending flag
    a= SPI2->SPI2_RX_TX_REG0;					// get spi data
    slave_value=slave_value+ ((a&0xFFFF)<<12);			// update return value

    SPI2->SPI2_RX_TX_REG0 = byte_three & 0xFFFF;   		// write TX_REG0, trigger to start
    do{
    }while (REG_GETF(SPI2, SPI2_CTRL_REG, SPI_INT_BIT) == 0);   // polling to wait for spi have data
    SPI2->SPI2_CLEAR_INT_REG = 1;   				// clear pending flag
    a= SPI2->SPI2_RX_TX_REG0;				        // get spi data
    slave_value=slave_value+ ((a&0xFFF0)>>4);			// update return value
 
    GPIO->P3_SET_DATA_REG = 1<<1;                               // deactivate  cs

    REG_CLR_BIT(SPI2, SPI2_CTRL_REG, SPI_ON);                   // disable SPI block
 
    return slave_value;  
}

/*****************************************************************************************
 * @brief Ripple specific write access
 *
 * @param[in] addr    register address
 * @param[in] value   value to write
 *
 * @return uint32_t value
 ****************************************************************************************
 */
//@WIK, this function is complete changed because of different spi activation.
// Is done in SW now iso SPIGO bit
 void rf_rpl_reg_wr (uint16_t address, uint32_t data)
{
    unsigned short byte_one;
    unsigned short byte_two;
    unsigned short byte_three;
    
// spi format master out 		8bit address - 4bit  0x3 - 32bit data - 4bit 0xB
// spi format master in         46bit unsed  

    byte_one=((address&0xFF)<<8)+0x30+((data&0xF0000000)>>28);
    byte_two=((data&0x0FFFF000)>>12);
    byte_three=((data&0x0FFF)<<4)+0xB;

    uint16_t reg = SPI2->SPI2_CTRL_REG;
    REG_SET_FIELD(SPI2, SPI2_CTRL_REG, SPI_WORD, reg, 1);       // set to 16bit mode
    REG_SET_FIELD(SPI2, SPI2_CTRL_REG, SPI_CLK, reg,  2);       // fastest clock
    SPI2->SPI2_CTRL_REG = reg;
    REG_SET_BIT(SPI2->SPI2_CTRL_REG, SPI_ON);                   // enable SPI block

    GPIO->P3_RESET_DATA_REG = 1<<1;				// set cs LOW

    SPI2->SPI2_RX_TX_REG0 = byte_one & 0xFFFF;   		// write TX_REG0, trigger to start
    do{
    }while (REG_GETF(SPI2, SPI2_CTRL_REG, SPI_INT_BIT) == 0);   // polling to wait for spi have data
    SPI2->SPI2_CLEAR_INT_REG = 1;                               // clear pending flag
    
    SPI2->SPI2_RX_TX_REG0 = byte_two & 0xFFFF;                  // write TX_REG0, trigger to start
    do{
    }while (REG_GETF(SPI2, SPI2_CTRL_REG, SPI_INT_BIT) == 0);   // polling to wait for spi have data
    SPI2->SPI2_CLEAR_INT_REG = 1;				// clear pending flag
    
    SPI2->SPI2_RX_TX_REG0 = byte_three & 0xFFFF;   		// write TX_REG0, trigger to start
    do{
    }while (REG_GETF(SPI2, SPI2_CTRL_REG, SPI_INT_BIT) == 0);   // polling to wait for spi have data
    SPI2->SPI2_CLEAR_INT_REG = 1;   					// clear pending flag
    
    GPIO->P3_SET_DATA_REG = 1<<1;				// deactivate  cs

    REG_CLR_BIT(SPI2, SPI2_CTRL_REG, SPI_ON);                   // disable SPI block
 
}

/*****************************************************************************************
 * @brief Static function - Ripple TX CNTL1 by radio
 ****************************************************************************************
 */
static void rf_rpl_set_txcntl1(void)
{
    // Read board ID in platform
    //uint16_t id = plf_read_rf_board_id();
    uint16_t id = RIPPLE_ID; // Boards we currently have: 19, 49, 50

    // Check ID is valid
    if(id < RPL_NB_BOARDS)
    {
        // Program TXCNTL1 with dedicated value
        rpl_rftxcntl1_set(RPL_TXDC_FORCE_BIT | RF_RPL_TXCNTL1_TBL[id]);

        // Program RSSI threshold
        rpl_rssi_cfg_setf(RF_RPL_RSSI_THR_TBL[id]);
        ASSERT_ERR(rpl_rssi_cfg_getf() == RF_RPL_RSSI_THR_TBL[id]);
    }
    else
    {
        // Assert error
        ASSERT_ERR(0);
    }
}

/*****************************************************************************************
 * @brief Static function - Ripple RF Power up sequence (all on)
 ****************************************************************************************
 */
static void rf_rpl_pw_up(void)
{
    /* Set trimming values for LDO */
    rpl_ldo_rxtx_trim_setf(0x00);
    ASSERT_ERR(rpl_ldo_rxtx_trim_getf() == 0x00);
    rpl_ldo_vdig_trim_setf(0x00);
    ASSERT_ERR(rpl_ldo_vdig_trim_getf() == 0x00);
    rpl_ldo_adda_trim_setf(0x00);
    ASSERT_ERR(rpl_ldo_adda_trim_getf() == 0x00);
    rpl_ldo_vco_trim_setf(0x00);
    ASSERT_ERR(rpl_ldo_vco_trim_getf() == 0x00);
    rpl_ldo_xo_trim_setf(0x00);
    ASSERT_ERR(rpl_ldo_xo_trim_getf() == 0x00);
    rpl_ldo_pll_trim_setf(0x00);
    ASSERT_ERR(rpl_ldo_pll_trim_getf() == 0x00);
    rpl_ldo_bgap_trim_setf(0x04);
    ASSERT_ERR(rpl_ldo_bgap_trim_getf() == 0x04);

    /* Set behaviour vs. emable_rm low power mode */
    rpl_ldo_rxtx_lp_mode_setf(0x0);
    ASSERT_ERR(rpl_ldo_rxtx_lp_mode_getf() == 0x00);
    rpl_lda_adda_lp_mode_setf(0x0);
    ASSERT_ERR(rpl_lda_adda_lp_mode_getf() == 0x00);
    rpl_ldo_vco_lp_mode_setf(0x0);
    ASSERT_ERR(rpl_ldo_vco_lp_mode_getf() == 0x00);
    rpl_ldo_pll_lp_mode_setf(0x0);
    ASSERT_ERR(rpl_ldo_pll_lp_mode_getf() == 0x00);
    rpl_ldo_xo_lp_mode_setf(0x0);
    ASSERT_ERR(rpl_ldo_xo_lp_mode_getf() == 0x00);

    /* Power-up PLL */
    rpl_ldo_xo_on_setf(0x01);
    ASSERT_ERR(rpl_ldo_xo_on_getf() == 0x01);
    rpl_ldo_vco_on_setf(0x01);
    ASSERT_ERR(rpl_ldo_vco_on_getf() == 0x01);
    rpl_ldo_pll_on_setf(0x01);
    ASSERT_ERR(rpl_ldo_pll_on_getf() == 0x01);
    rpl_ldo_adda_on_setf(0x01);
    ASSERT_ERR(rpl_ldo_adda_on_getf() == 0x01);
    rpl_ldo_rxtx_on_setf(0x01);
    ASSERT_ERR(rpl_ldo_rxtx_on_getf() == 0x01);

    /* Ramp Gen Control is left as default */
    /* Other Power Settings */
    rpl_crm_cntl0_set(0x00000000);

    // Set clock to 52 MHz
    rpl_bb_clk_div_setf(0x00);
    ASSERT_ERR(rpl_bb_clk_div_getf() == 0x00);
}

/*****************************************************************************************
 * @brief Static function - Init modem for Ripple.
 ****************************************************************************************
 */
static void rf_rpl_mdm_init(void)
{
    mdm_rx_startupdel_set(0x00010003);
    mdm_tx_startupdel_set(0x00010003);
    mdm_pe_powerthr_set(0x00000018);

    mdm_fmtxen_setf(0x1);
    ASSERT_ERR(mdm_fmtxen_getf() == 0x1);

    /* new value for DFD KFACTOR */
    mdm_dfd_kfactor_set(0x0000006E);

    mdm_gsg_dphi_den_set(0x00050000);
    mdm_gsg_dphi_nom_set(0x00300001);
    mdm_mdm_cntl_set(0x00000002);
    mdm_tx_gfskmode_set(0x00000000);

    //Setting br/ble
    mdm_dsg_nom_set(0x01);
    mdm_dsg_den_set(0x00);
    mdm_gsg_den_set(0x00);
    mdm_gsg_nom_set(0x01);

}

/**
 ****************************************************************************************
 * @brief Static function - Measure Ripple VCO Frequency
 *
 * @param[in] vco_fc_value  VCO
 * @param[in] vco_freq      Pointer to frequency value.
 ****************************************************************************************
 */
static void rf_rpl_measure_vco_freq(uint8_t vco_fc_value, int * vco_freq)
{
    //Loop control
    uint8_t  exit_loop = 0x00;
    uint32_t prev_RPLLCNTL0 = 0x00000000;
    uint32_t prev_RPLLCNTL1 = 0x00000000;
    uint16_t timeout = 0x0000;

    //Register read value
    uint32_t reg_val = 0x00000000;
    exit_loop = 0x0;

    //Push registers values
    prev_RPLLCNTL0 = rpl_rpllcntl0_get();
    prev_RPLLCNTL1 = rpl_rpllcntl1_get();

    //VCO frequency meter timeout
    timeout = 20;

    //VCO frequency measurement
    rpl_force_rpll_ctrl_setf(0x1);
    rpl_xo_on_setf(0x1);
    rpl_rpll_ld_on_setf(0x1);
    rpl_rpll_cp_on_setf(0x1);
    rpl_rpll_div_on_setf(0x1);
    rpl_rpllcntl1_set(0x105401 | (vco_fc_value << RPL_RPLL_VCOFC_LSB));
    rpl_rpll_vco_on_setf(0x1);
    rpl_rpll_locnt_on_setf(0x1);

    while (exit_loop == 0)
    {
        reg_val = rpl_rpll_locnt_done_getf();
        timeout = timeout - 1;

        if (reg_val == 0x1)
        {
            exit_loop = 1;
        }
        else if (timeout == 0)
        {
            exit_loop = 1;
        }
    }

    //Compute VCO frequency
    *vco_freq = (uint16_t) (rpl_rpll_locnt_val_getf());

    //Pop register values
    rpl_rpllcntl0_set(prev_RPLLCNTL0);
    rpl_rpllcntl1_set(prev_RPLLCNTL1);
}

/**
 ****************************************************************************************
 * @brief Static function - for VCO Calibration
 *
 * @param[in] channel   channel
 * @param[in] vco_val   vco value
 ****************************************************************************************
 */
static void rf_rpl_calib_vco_fq(uint8_t channel, uint8_t *vco_val)
{
    //Target VCO frequency error
    int     target_freq_err, vco_freq, freq_err ;
    bool    exit_loop;
    uint8_t vco = 0x00;

    target_freq_err = 2402 - 7 + channel;
    exit_loop = 0;

    //Set channel
    rpl_rfdyncntl_set(channel << RPL_CHANNEL_LSB);

    //Starting VCO_FC Algorithm
    vco = 8;

    //Till not all the VCO_FC values not covered or error frequency is good enough
    while (exit_loop == 0)
    {
        //Measure VCO frequency
        rf_rpl_measure_vco_freq(vco, &vco_freq);

        //Compute frequency error
        freq_err = 2402 - 7 + channel - vco_freq;

        //Save the best settings
        if (co_abs(freq_err) < target_freq_err)
        {
            target_freq_err = co_abs(freq_err);

            *vco_val = vco;

            //Try next VCO_FC value
            if (freq_err < 0)
            {
                vco = vco + 1;
            }
            else
            {
                vco = vco - 1;
            }
        }
        else
        {
            exit_loop = 1;
        }
    }
}

/**
 ****************************************************************************************
 * @brief Static function for calibrating ICP value.
 *
 * @param[in] icp Pointer to value to calibrate.
 ****************************************************************************************
 */
static void rf_rpl_calib_icp(uint8_t channel,uint8_t * icp)
{
    *icp = 4;
}

/**
 ****************************************************************************************
 * @brief Static function for status lock.
 *
 * @param[in] chnl  channel
 * @param[in] icp   icp
 * @param[in] vco   vco value
 * @param[in] lock  pointer to lock
 ****************************************************************************************
 */
static void rf_rpl_status_lock(uint8_t chnl, uint8_t icp, uint8_t vco, uint8_t *lock)
{
    //Loop control
    bool exit_loop;

    //Push registers values
    uint32_t prev_RPLLCNTL0;
    uint32_t prev_RPLLCNTL1;
    int timeout;

    //Register read value
    uint32_t regval;

    exit_loop = 0;

    prev_RPLLCNTL0 = rpl_rpllcntl0_get();
    prev_RPLLCNTL1 = rpl_rpllcntl1_get();

    //PLL lock timeout
    timeout = 20;

    //Set chnl
    rpl_rfdyncntl_set(chnl << RPL_CHANNEL_LSB);

    //Try to lock PLL
    rpl_rpllcntl1_set(0x105000 | (icp << RPL_RPLL_ICP_LSB) | (vco << RPL_RPLL_VCOFC_LSB));
    // Turn on vcoref, ld, vco, cp div
    rpl_rpllcntl0_set(0x8000012F);

    //Get PLL lock status
    while (exit_loop == 0)
    {
        regval = rpl_rpll_locked_getf();

        timeout = timeout - 1;

        if(regval == 0x1)
        {
            *lock = 0;
            exit_loop = 1;
        }
        else if(timeout <= 0)
        {
            *lock = 1;
            exit_loop = 1;
        }
    }

    //Pop register values
    rpl_rpllcntl0_set(prev_RPLLCNTL0);
    rpl_rpllcntl1_set(prev_RPLLCNTL1);
}


/***************************************************************************************
 * @brief Static function for radio PLL auto-calibration.
 ****************************************************************************************
 */
static void rf_rpl_pll_autocalib(void)
{
    uint8_t  chnl;
    uint32_t wreg;

    for(chnl = 0; chnl < 80; chnl++)
    {
        //Calibrate VCOFC value
        rf_rpl_calib_vco_fq(chnl, &RFPLL_VCOFC_TABLE[chnl]);

        //Calibrate ICP value
        rf_rpl_calib_icp(chnl,&RFPLL_ICP_TABLE[chnl]);

        //Test if PLL Lock
        rf_rpl_status_lock(chnl, RFPLL_ICP_TABLE[chnl],
                                RFPLL_VCOFC_TABLE[chnl],
                               &RFPLL_LOCK_TABLE[chnl]);
    }

    for(chnl = 0; chnl < 80; chnl++)
    {
        //Write calibration table
        wreg = (uint32_t)((RFPLL_LOCK_TABLE[chnl] * 0x10000) |
                          (RFPLL_ICP_TABLE[chnl] * 0x1000)   |
                          (RFPLL_VCOFC_TABLE[chnl] * 0x100)  | 0x80 | chnl);

        rpl_vcofc_icp_calibcntl_set(wreg);
        rpl_vcofc_icp_calibcntl_set((uint32_t)chnl);

    }
}

/***************************************************************************************
 * @brief Static Ripple radio Calibration function.
 ***************************************************************************************
 */
static void rf_rpl_calib(void)
{
    rf_rpl_pll_autocalib();
}

/***************************************************************************************
 * @brief Static function - Sequencer settings Initialization for Ripple radio
 ****************************************************************************************
*/
static void rf_rpl_sequencers_init(void)
{
    rpl_seqcntl_set(0x02080208);
    ASSERT_ERR(rpl_seqcntl_get() == 0x02080208);
    /* RF TX UP Sequencer Settings */
    rpl_tx_up_dly0_setf(0x1D);
    ASSERT_ERR(rpl_tx_up_dly0_getf() == 0x1D);
    rpl_tx_up_dly1_setf(0xCA);
    ASSERT_ERR(rpl_tx_up_dly1_getf() == 0xCA);
    rpl_tx_up_dly2_setf(0x01);
    ASSERT_ERR(rpl_tx_up_dly2_getf() == 0x01);
    rpl_tx_up_dly3_setf(0xE7);
    ASSERT_ERR(rpl_tx_up_dly3_getf() == 0xE7);
    rpl_tx_up_dly4_setf(0x01);
    ASSERT_ERR(rpl_tx_up_dly4_getf() == 0x01);
    rpl_tx_up_dly5_setf(0x3A);
    ASSERT_ERR(rpl_tx_up_dly5_getf() == 0x3A);
    rpl_tx_up_dly6_setf(0x01);
    ASSERT_ERR(rpl_tx_up_dly6_getf() == 0x01);
    rpl_tx_up_dly7_setf(0x01);
    ASSERT_ERR(rpl_tx_up_dly7_getf() == 0x01);
    rpl_tx_up_dly8_setf(0x01);
    ASSERT_ERR(rpl_tx_up_dly8_getf() == 0x01);

    /* RF TX Down Sequencer timings */
    rpl_tx_dn_dly0_setf(0x01);
    ASSERT_ERR(rpl_tx_dn_dly0_getf() == 0x01);
    rpl_tx_dn_dly1_setf(0x01);
    ASSERT_ERR(rpl_tx_dn_dly1_getf() == 0x01);
    rpl_tx_dn_dly2_setf(0x01);
    ASSERT_ERR(rpl_tx_dn_dly2_getf() == 0x01);
    rpl_tx_dn_dly3_setf(0x01);
    ASSERT_ERR(rpl_tx_dn_dly3_getf() == 0x01);
    rpl_tx_dn_dly4_setf(0x01);
    ASSERT_ERR(rpl_tx_dn_dly4_getf() == 0x01);
    rpl_tx_dn_dly5_setf(0x01);
    ASSERT_ERR(rpl_tx_dn_dly5_getf() == 0x01);
    rpl_tx_dn_dly6_setf(0x01);
    ASSERT_ERR(rpl_tx_dn_dly6_getf() == 0x01);
    rpl_tx_dn_dly7_setf(0x01);
    ASSERT_ERR(rpl_tx_dn_dly7_getf() == 0x01);
    rpl_tx_dn_dly8_setf(0x01);
    ASSERT_ERR(rpl_tx_dn_dly8_getf() == 0x01);

     /*TX UP Sequencer Outputs*/
    rpl_tx_up_out0_setf(0x00);
    ASSERT_ERR(rpl_tx_up_out0_getf() == 0x00);
#if !RF_CLASS1

    rpl_tx_up_out1_setf(0x01);
    ASSERT_ERR(rpl_tx_up_out1_getf() == 0x01);
    rpl_tx_up_out2_setf(0x03);
    ASSERT_ERR(rpl_tx_up_out2_getf() == 0x03);
    rpl_tx_up_out3_setf(0x07);
    ASSERT_ERR(rpl_tx_up_out3_getf() == 0x07);
    rpl_tx_up_out4_setf(0x0F);
    ASSERT_ERR(rpl_tx_up_out4_getf() == 0x0F);
    rpl_tx_up_out5_setf(0x1F);
    ASSERT_ERR(rpl_tx_up_out5_getf() == 0x1F);
    rpl_tx_up_out6_setf(0x1F);
    ASSERT_ERR(rpl_tx_up_out6_getf() == 0x1F);
    rpl_tx_up_out7_setf(0x1F);
    ASSERT_ERR(rpl_tx_up_out7_getf() == 0x1F);
    rpl_tx_up_out8_setf(0x1F);
    ASSERT_ERR(rpl_tx_up_out8_getf() == 0x1F);

     /*TX DOWN Sequencer Outputs*/
    rpl_tx_dn_out0_setf(0x0F);
    ASSERT_ERR(rpl_tx_dn_out0_getf() == 0x0F);
    rpl_tx_dn_out1_setf(0x07);
    ASSERT_ERR(rpl_tx_dn_out1_getf() == 0x07);
    rpl_tx_dn_out2_setf(0x03);
    ASSERT_ERR(rpl_tx_dn_out2_getf() == 0x03);
    rpl_tx_dn_out3_setf(0x01);
    ASSERT_ERR(rpl_tx_dn_out3_getf() == 0x01);
#else
    rpl_tx_up_out1_setf(0x21);
    ASSERT_ERR(rpl_tx_up_out1_getf() == 0x21);
    rpl_tx_up_out2_setf(0x23);
    ASSERT_ERR(rpl_tx_up_out2_getf() == 0x23);
    rpl_tx_up_out3_setf(0x27);
    ASSERT_ERR(rpl_tx_up_out3_getf() == 0x27);
    rpl_tx_up_out4_setf(0x2F);
    ASSERT_ERR(rpl_tx_up_out4_getf() == 0x2F);
    rpl_tx_up_out5_setf(0x3F);
    ASSERT_ERR(rpl_tx_up_out5_getf() == 0x3F);
    rpl_tx_up_out6_setf(0x3F);
    ASSERT_ERR(rpl_tx_up_out6_getf() == 0x3F);
    rpl_tx_up_out7_setf(0x3F);
    ASSERT_ERR(rpl_tx_up_out7_getf() == 0x3F);
    rpl_tx_up_out8_setf(0x3F);
    ASSERT_ERR(rpl_tx_up_out8_getf() == 0x3F);

     /*TX DOWN Sequencer Outputs*/
    rpl_tx_dn_out0_setf(0x2F);
    ASSERT_ERR(rpl_tx_dn_out0_getf() == 0x2F);
    rpl_tx_dn_out1_setf(0x27);
    ASSERT_ERR(rpl_tx_dn_out1_getf() == 0x27);
    rpl_tx_dn_out2_setf(0x23);
    ASSERT_ERR(rpl_tx_dn_out2_getf() == 0x23);
    rpl_tx_dn_out3_setf(0x21);
    ASSERT_ERR(rpl_tx_dn_out3_getf() == 0x21);
#endif
    rpl_tx_dn_out4_setf(0x00);
    ASSERT_ERR(rpl_tx_dn_out4_getf() == 0x00);
    rpl_tx_dn_out5_setf(0x00);
    ASSERT_ERR(rpl_tx_dn_out5_getf() == 0x00);
    rpl_tx_dn_out6_setf(0x00);
    ASSERT_ERR(rpl_tx_dn_out6_getf() == 0x00);
    rpl_tx_dn_out7_setf(0x00);
    ASSERT_ERR(rpl_tx_dn_out7_getf() == 0x00);
    rpl_tx_dn_out8_setf(0x00);
    ASSERT_ERR(rpl_tx_dn_out8_getf() == 0x00);

    /* RX Sequencer */
    rpl_rx_up_dly0_setf(0x01);
    rpl_rx_up_dly1_setf(0x103);
    rpl_rx_up_dly2_setf(0x19);
    rpl_rx_up_dly3_setf(0x19);
    rpl_rx_up_dly4_setf(0x23);
    rpl_rx_up_dly5_setf(0x01);
    rpl_rx_up_dly6_setf(0x01);
    rpl_rx_up_dly7_setf(0x01);
    rpl_rx_up_dly8_setf(0x01);

    rpl_rx_dn_dly0_setf(0x01);
    rpl_rx_dn_dly1_setf(0x01);
    rpl_rx_dn_dly2_setf(0x01);
    rpl_rx_dn_dly3_setf(0x01);
    rpl_rx_dn_dly4_setf(0x01);
    rpl_rx_dn_dly5_setf(0x01);
    rpl_rx_dn_dly6_setf(0x01);
    rpl_rx_dn_dly7_setf(0x01);
    rpl_rx_dn_dly8_setf(0x01);

    /* Sequence with AGC */
    rpl_rx_up_out0_setf(0x00);
#if !RF_CLASS1    
    rpl_rx_up_out1_setf(0x01);
    rpl_rx_up_out2_setf(0x03);
    rpl_rx_up_out3_setf(0x07);
    rpl_rx_up_out4_setf(0x0F);
    rpl_rx_up_out5_setf(0x1F);
    rpl_rx_up_out6_setf(0x1F);
    rpl_rx_up_out7_setf(0x1F);
    rpl_rx_up_out8_setf(0x1F);

    rpl_rx_dn_out0_setf(0x0F);
    rpl_rx_dn_out1_setf(0x07);
    rpl_rx_dn_out2_setf(0x03);
    rpl_rx_dn_out3_setf(0x01);
#else
    rpl_rx_up_out1_setf(0x41);
    rpl_rx_up_out2_setf(0x43);
    rpl_rx_up_out3_setf(0x47);
    rpl_rx_up_out4_setf(0x4F);
    rpl_rx_up_out5_setf(0x5F);
    rpl_rx_up_out6_setf(0x5F);
    rpl_rx_up_out7_setf(0x5F);
    rpl_rx_up_out8_setf(0x5F);

    rpl_rx_dn_out0_setf(0x4F);
    rpl_rx_dn_out1_setf(0x47);
    rpl_rx_dn_out2_setf(0x43);
    rpl_rx_dn_out3_setf(0x41);
#endif
    rpl_rx_dn_out4_setf(0x00);
    rpl_rx_dn_out5_setf(0x00);
    rpl_rx_dn_out6_setf(0x00);
    rpl_rx_dn_out7_setf(0x00);
    rpl_rx_dn_out8_setf(0x00);
}

/***************************************************************************************
 * @brief Static function - Tx Gain tables settings
 ****************************************************************************************
 */
static void rf_rpl_txgain_set(void)
{
    rpl_rftx_gfsk_gain_table_set(0,0x00000003);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(0) == 0x00000003);
    rpl_rftx_gfsk_gain_table_set(1,0x00000013);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(1) == 0x00000013);
    rpl_rftx_gfsk_gain_table_set(2,0x00000023);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(2) == 0x00000023);
    rpl_rftx_gfsk_gain_table_set(3,0x00000033);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(3) == 0x00000033);
    rpl_rftx_gfsk_gain_table_set(4,0x00000133);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(4) == 0x00000133);
    rpl_rftx_gfsk_gain_table_set(5,0x00000233);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(5) == 0x00000233);
    rpl_rftx_gfsk_gain_table_set(6,0x00000333);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(6) == 0x00000333);
    rpl_rftx_gfsk_gain_table_set(7,0x00000433);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(7) == 0x00000433);
    rpl_rftx_gfsk_gain_table_set(8,0x00000324);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(8) == 0x00000324);
    rpl_rftx_gfsk_gain_table_set(9,0x00000324);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(9) == 0x00000324);
    rpl_rftx_gfsk_gain_table_set(10,0x00000324);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(10) == 0x00000324);
    rpl_rftx_gfsk_gain_table_set(11,0x00000324);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(11) == 0x00000324);
    rpl_rftx_gfsk_gain_table_set(12,0x00000324);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(12) == 0x00000324);
    rpl_rftx_gfsk_gain_table_set(13,0x00000324);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(13) == 0x00000324);
    rpl_rftx_gfsk_gain_table_set(14,0x00000324);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(14) == 0x00000324);
    rpl_rftx_gfsk_gain_table_set(15,0x00000324);
    ASSERT_ERR(rpl_rftx_gfsk_gain_table_get(15) == 0x00000324);

    /* EDR GAIN */
    rpl_rftx_edr_gain_table_set(8,0x00324324);
    ASSERT_ERR(rpl_rftx_edr_gain_table_get(8) == 0x00324324);
    rpl_rftx_edr_gain_table_set(9,0x00324324);
    ASSERT_ERR(rpl_rftx_edr_gain_table_get(9) == 0x00324324);
    rpl_rftx_edr_gain_table_set(10,0x00324324);
    ASSERT_ERR(rpl_rftx_edr_gain_table_get(10) == 0x00324324);
    rpl_rftx_edr_gain_table_set(11,0x00324324);
    ASSERT_ERR(rpl_rftx_edr_gain_table_get(11) == 0x00324324);
    rpl_rftx_edr_gain_table_set(12,0x00324324);
    ASSERT_ERR(rpl_rftx_edr_gain_table_get(12) == 0x00324324);
    rpl_rftx_edr_gain_table_set(13,0x00324324);
    ASSERT_ERR(rpl_rftx_edr_gain_table_get(13) == 0x00324324);
    rpl_rftx_edr_gain_table_set(14,0x00324324);
    ASSERT_ERR(rpl_rftx_edr_gain_table_get(14) == 0x00324324);
    rpl_rftx_edr_gain_table_set(15,0x00324324);
    ASSERT_ERR(rpl_rftx_edr_gain_table_get(15) == 0x00324324);
}

/***************************************************************************************
 * @brief Static function - Initialization sequence for Ripple radio
 ****************************************************************************************
 */
static void rf_rpl_init_seq(void)
{
    /* Note: All settings are enforced in case reset to the original settings
     * is required */
    /* RF PLL Settings */
    rpl_rpllcntl0_set(0x00000100);
    ASSERT_ERR(rpl_rpllcntl0_get() == 0x00000100);
    rpl_rpllcntl1_set(0x0010740C);
    ASSERT_ERR(rpl_rpllcntl1_get() == 0x0010740C);
    rpl_rpllcntl2_set(0x00080346);
    ASSERT_ERR(rpl_rpllcntl2_get() == 0x00080346);

    // Gain for FM
    rpl_fmtx_trim_setf(2);

    /* RF Sequencer Settings */
    rf_rpl_sequencers_init();

    /* **********************
     * RF Tx Chain Settings *
     * **********************/
    rpl_rampgencntl_set(0x10000202);
    ASSERT_ERR(rpl_rampgencntl_get() == 0x10000202);

    rpl_txctrl_force_setf(0x0);
    ASSERT_ERR(rpl_txctrl_force_getf() == 0x0);
    /* DC calibration disabled */
    rpl_txdc_cal_en_setf(0x0);
    ASSERT_ERR(rpl_txdc_cal_en_getf() == 0x0);

    /* RF TX Gain Table Settings */
    rf_rpl_txgain_set();
    /* Internal PA Selection */
    rpl_txpa_sel_setf(0x0);
    ASSERT_ERR(rpl_txpa_sel_getf() == 0x0);
    rpl_txpa_cfg_setf(0x03);
    ASSERT_ERR(rpl_txpa_cfg_getf() == 0x3);

#if RF_CLASS1
    /* External PA selection */
    rf_rpl_set_txcntl0();
#endif
    rf_rpl_set_txcntl1();
    
    /* Make sure IF_EN is set to 0 */
    rpl_txfilt_if_en_setf(0x0);
    ASSERT_ERR(rpl_txfilt_if_en_getf() == 0x0);
    rpl_txtank_sel_setf(0x07);
    ASSERT_ERR(rpl_txtank_sel_getf() == 0x07);

    /* Set DAC gains */
    // Except GFSK, needed to pass relative power
    rpl_rftx_daciq_gain_set(0x00777777);
    ASSERT_ERR(rpl_rftx_daciq_gain_get() == 0x00777777);

    /* **********************
     * RF Rx Chain Settings *
     * **********************/
    rpl_rxlna_cfg_setf(0x02);
    ASSERT_ERR(rpl_rxlna_cfg_getf() == 0x02);
    rpl_rxadc_cfg_setf(0x17);
    ASSERT_ERR(rpl_rxadc_cfg_getf() == 0x17);
    rpl_rxadc_clk_inv_en_setf(0x0);

    // Rx Chain optimal settings
    rpl_rxmix_trim_setf(0x3);
    ASSERT_ERR(rpl_rxmix_trim_getf() == 0x3);
    rpl_rxlna_trim_setf(0x3);
    ASSERT_ERR(rpl_rxlna_trim_getf() == 0x3);

    //AGC Settings
    rpl_rssi_preload_cnt_setf(0x1F);
    ASSERT_ERR(rpl_rssi_preload_cnt_getf() == 0x1F);
    rpl_rssi_on_dlyd_cnt_setf(0x8);
    ASSERT_ERR(rpl_rssi_on_dlyd_cnt_getf() == 0x8);
    rpl_agc_settle_time_setf(0x34);
    ASSERT_ERR(rpl_agc_settle_time_getf() == 0x34);

    /* RF Tx/Rx Common Settings */
    rpl_icp_cal_dsb_setf(0x1);
    ASSERT_ERR(rpl_icp_cal_dsb_getf() == 0x1);
    rpl_calib_mode_setf(0x00);
    ASSERT_ERR(rpl_calib_mode_getf() == 0x00);
    rpl_bb_bias_cfg_setf(0x02);
    ASSERT_ERR(rpl_bb_bias_cfg_getf() == 0x02);

    rpl_dap_clk_mode_setf(0x0);
    ASSERT_ERR(rpl_dap_clk_mode_getf() == 0x0);
    rpl_lodiv2_trim_setf(0x3);
    ASSERT_ERR(rpl_lodiv2_trim_getf() == 0x3);
    rpl_lobuf_trim_setf(0x3);
    ASSERT_ERR(rpl_lobuf_trim_getf() == 0x3);

    // Settings to pass TRMCA11
    rpl_filt_rc_val_setf(0x7);
    ASSERT_ERR(rpl_filt_rc_val_getf() == 0x7);

    /* Others Static Settings */
    rpl_pullup_cntl_setf(0x1);
    ASSERT_ERR(rpl_pullup_cntl_getf() == 0x1);
    rpl_rfswcntl_set(0x0);
    ASSERT_ERR(rpl_rfswcntl_get() == 0x0);

    rpl_rpllsdcntl2_set(0x80402C0F);
    ASSERT_ERR(rpl_rpllsdcntl2_get() == 0x80402C0F);

    #ifdef CFG_MBP
    rpl_rxmdm_bypass_en_setf(0x1);
    rpl_txmdm_bypass_en_setf(0x1);
    rpl_mux0_ctrl_setf(0xA2);
    #endif //CFG_MBP

    //calibration procedure
    rf_rpl_calib();

}

/**
 *****************************************************************************************
 * @brief Init RF sequence after reset.
 *****************************************************************************************
 */
static void rf_reset(void)
{
    // Calibration procedure
    rf_rpl_calib();
}

#ifdef CFG_BT
static void rf_txpwr_max(uint8_t link_id)
{
    struct cntl_struct *cs_ptr;
    cs_ptr = (struct cntl_struct *)(REG_BT_EM_CS_BASE_ADDR + REG_BT_EM_CS_ADDR_GET(link_id));
    cs_ptr->TXPWR = RPL_POWER_MAX;
}

/**
 *****************************************************************************************
 * @brief Increase the TX power by one step
 *
 * @param[in] link_id Link ID for which the TX power has to be increased
 *
 * @return true when maximum power is reached, false otherwise
 *****************************************************************************************
 */
static bool rf_txpwr_inc(uint8_t link_id)
{
    bool boMaxpow = true;
    struct cntl_struct *cs_ptr;

    cs_ptr = (struct cntl_struct *)(REG_BT_EM_CS_BASE_ADDR + REG_BT_EM_CS_ADDR_GET(link_id));
    if ((cs_ptr->TXPWR & RPL_POWER_MSK) <= RPL_POWER_MAX)
    {
        //Increase the TX power value

        if (!((cs_ptr->TXPWR & RPL_POWER_MSK) == RPL_POWER_MAX))
        {
            cs_ptr->TXPWR++;
            boMaxpow = false;
        }
    }
    return(boMaxpow);
}
/**
 *****************************************************************************************
 * @brief Increase the TX power by one step
 *
 * @param[in] link_id Link ID for which the TX power has to be increased
 *
 * @return true when maximum power is reached, false otherwise
 *****************************************************************************************
 */
static bool rf_txpwr_epc_inc(uint8_t link_id)
{
    bool boMaxpow = true;
    struct cntl_struct *cs_ptr;

    cs_ptr = (struct cntl_struct *)(REG_BT_EM_CS_BASE_ADDR + REG_BT_EM_CS_ADDR_GET(link_id));

    if ((cs_ptr->TXPWR & RPL_POWER_MSK) < RPL_POWER_MAX)
    {
        //Increase the TX power value
        cs_ptr->TXPWR++;
        if (!((cs_ptr->TXPWR & RPL_POWER_MSK) == RPL_POWER_MAX))
        {
            boMaxpow = false;
        }
    }
    return(boMaxpow);
}
/**
 *****************************************************************************************
 * @brief Decrease the TX power by one step
 *
 * @param[in] link_id Link ID for which the TX power has to be decreased
 *
 * @return true when minimum power is reached, false otherwise
 *****************************************************************************************
 */
static bool rf_txpwr_dec(uint8_t link_id)
{
    bool boMinpow = true;
    struct cntl_struct *cs_ptr;

    cs_ptr = (struct cntl_struct *)(REG_BT_EM_CS_BASE_ADDR + REG_BT_EM_CS_ADDR_GET(link_id));
    if ((cs_ptr->TXPWR & RPL_POWER_MSK) >= RPL_POWER_MIN)
    {
        // Decrease the TX power value
        if (!((cs_ptr->TXPWR & RPL_POWER_MSK) == RPL_POWER_MIN))
        {
            cs_ptr->TXPWR--;
            boMinpow = false;
        }
    }
    return(boMinpow);
}

/**
 *****************************************************************************************
 * @brief Decrease the TX power by one step
 *
 * @param[in] link_id Link ID for which the TX power has to be decreased
 *
 * @return true when minimum power is reached, false otherwise
 *****************************************************************************************
 */
static bool rf_txpwr_epc_dec(uint8_t link_id)
{
    bool boMinpow = true;
    struct cntl_struct *cs_ptr;

    cs_ptr = (struct cntl_struct *)(REG_BT_EM_CS_BASE_ADDR + REG_BT_EM_CS_ADDR_GET(link_id));

    if ((cs_ptr->TXPWR & RPL_POWER_MSK) > RPL_POWER_MIN)
    {
        // Decrease the TX power value
        cs_ptr->TXPWR--;
        if (!((cs_ptr->TXPWR & RPL_POWER_MSK) == RPL_POWER_MIN))
        {
            boMinpow = false;
        }
    }
    return(boMinpow);
}


/**
 ****************************************************************************************
 * @brief Execute the EPC request received from the peer device.
 *
 * @param[in] link_id     Link Identifier
 * @param[in] action      Increase, decrease or go to maximum
 *
 * @return Tx power status depending the modulation
 *
 ****************************************************************************************
 */
static uint8_t rf_txpwr_epc_req(uint8_t link_id, uint8_t action)
{
    bool minmax;
    uint8_t tx_power_set;

    switch(action)
    {
        case LCEPC_DEC:
            // No per-modulation power control supported, so simply decrease the TX power
            minmax = rf_txpwr_epc_dec(link_id);

            // Check if min level is reached
            if (minmax)
            {
                tx_power_set = LCEPC_PWRMIN;       //1MB
                tx_power_set |= LCEPC_PWRMIN << 2; //2MB
                tx_power_set |= LCEPC_PWRMIN << 4; //3MB
            }
            else
            {
                tx_power_set = LCEPC_PWRONESTEP;       //1MB
                tx_power_set |= LCEPC_PWRONESTEP << 2; //2MB
                tx_power_set |= LCEPC_PWRONESTEP << 4; //3MB
            }
            break;

        case LCEPC_INC:
            // No per-modulation power control supported, so simply increase the TX power
            minmax = rf_txpwr_epc_inc(link_id);

            // Check if maximum level is reached
            if (minmax)
            {
                tx_power_set = LCEPC_PWRMAX;       //1MB
                tx_power_set |= LCEPC_PWRMAX << 2; //2MB
                tx_power_set |= LCEPC_PWRMAX << 4; //3MB
            }
            else
            {
                tx_power_set = LCEPC_PWRONESTEP;       //1MB
                tx_power_set |= LCEPC_PWRONESTEP << 2; //2MB
                tx_power_set |= LCEPC_PWRONESTEP << 4; //3MB
            }
            break;

        case LCEPC_MAX:
            rf_txpwr_max(link_id);
            tx_power_set = LCEPC_PWRMAX;       //1MB
            tx_power_set |= LCEPC_PWRMAX << 2; //2MB
            tx_power_set |= LCEPC_PWRMAX << 4; //3MB
            break;

        default:
            tx_power_set = LCEPC_PWRNOTSUPPORTED;       //1MB
            tx_power_set |= LCEPC_PWRNOTSUPPORTED << 2; //2MB
            tx_power_set |= LCEPC_PWRNOTSUPPORTED << 4; //3MB
            break;
    }

    return(tx_power_set);
}


/**
 *****************************************************************************************
 * @brief Get the TX power as control structure TX power field from a value in dBm.
 *
 * @param[in] txpwr_dbm   TX power in dBm
 *
 * @return The index of the TX power
 *
 *****************************************************************************************
 */
static uint8_t rf_txpwr_cs_get (int8_t txpwr_dbm)
{
    uint8_t i;

    for (i = RPL_POWER_MIN; i <= RPL_POWER_MAX; i++)
    {
        // Loop until we find a power just higher or equal to the requested one
        if (RF_RPL_TX_PW_CONV_TBL[i] >= txpwr_dbm)
            break;
    }

    /* Then check if this is achievable compared to max TX power */
    if (i > RPL_POWER_MAX)
    {
        i = RPL_POWER_MAX;
    }
    return(i);
}
#endif
/**
 *****************************************************************************************
 * @brief Convert RSSI to dBm
 *
 * @param[in] rssi_reg RSSI read from the HW registers
 *
 * @return The converted RSSI
 *
 *****************************************************************************************
 */
static uint8_t rf_rssi_convert (uint8_t rssi_reg)
{
    uint8_t RssidBm = 0, GRx;
    uint16_t PowerModem;

    /* Get the RSSI value from the look up table and get its signed value
     * Get the 2-complements signed value on 8 bits */
    PowerModem = ((rssi_reg & 0xF8) >> 3)*2;

    GRx = RF_RPL_RX_GAIN_TBL[rssi_reg & 0x07];

    RssidBm = PowerModem  - GRx - 64;

    return(RssidBm);
}

#ifdef CFG_BLE
/**
 ****************************************************************************************
 * @brief ISR to be called in BLE ISR routine when RF Interrupt occurs.
 *****************************************************************************************
 */
static void RADIOCNTL_Handler(void)
{

}

/**
 *****************************************************************************************
 * @brief Enable/disable force AGC mechanism
 *
 * @param[in]  True: Enable / False: disable
 *****************************************************************************************
 */
static void rf_force_agc_enable(bool en)
{
    #if defined(CFG_BT)
        bt_forceagc_en_setf(en);
    #else
        #ifndef CFG_BLECORE_10
            ble_forceagc_en_setf(en);
        #endif //CFG_BLECORE_10
    #endif //CFG_BLE
}
#endif //CFG_BLE

/**
 *****************************************************************************************
 * @brief Get TX power in dBm from the index in the control structure
 *
 * @param[in] txpwr_idx  Index of the TX power in the control structure
 * @param[in] modulation Modulation: 1 or 2 or 3 MBPS
 *
 * @return The TX power in dBm
 *
 *****************************************************************************************
 */
static uint8_t rf_txpwr_dbm_get(uint8_t txpwr_idx, uint8_t modulation)
{
    // power table is the same for BR and EDR
    return(RF_RPL_TX_PW_CONV_TBL[txpwr_idx]);
}


/**
 *****************************************************************************************
 * @brief Sleep function for the RF.
 *****************************************************************************************
 */
static void rf_sleep(void)
{
    ble_deepslcntl_set(ble_deepslcntl_get() |
                      BLE_DEEP_SLEEP_ON_BIT |    // RW BLE Core sleep
                      BLE_RADIO_SLEEP_EN_BIT |   // Radio sleep
                      BLE_OSC_SLEEP_EN_BIT);     // Oscillator sleep
   // ble_deepslcntl_set(ble_deepslcntl_get() | BLE_DEEP_SLEEP_ON_BIT );     
   rf_in_sleep = true;
}

/*
 * RADIO FUNCTION INTERFACE
 ****************************************************************************************
 */

void rf_init_func(struct rwip_rf_api *api)
//void rf_init(struct rwip_rf_api *api)
{
    uint8_t idx = 0;
    #if defined(CFG_BT)
    uint8_t temp_freq_tbl[EM_BT_FREQ_TABLE_LEN];
    #elif defined(CFG_BLE)
    uint8_t temp_freq_tbl[EM_BLE_FREQ_TABLE_LEN];
    #endif //CFG_BLE

    // Initialize the RF driver API structure
    api->reg_rd = rf_rpl_reg_rd;
    api->reg_wr = rf_rpl_reg_wr;
    api->txpwr_dbm_get = rf_txpwr_dbm_get;

    api->txpwr_max = RPL_POWER_MAX;
    api->sleep = rf_sleep;
    api->reset = rf_reset;
    #ifdef CFG_BLE
    api->isr = RADIOCNTL_Handler;
    api->force_agc_enable = rf_force_agc_enable;
    #endif //CFG_BLE

    api->rssi_convert = rf_rssi_convert;

    #if defined(CFG_BT)
    api->txpwr_inc = rf_txpwr_inc;
    api->txpwr_dec = rf_txpwr_dec;
    api->txpwr_epc_req = rf_txpwr_epc_req;
    api->txpwr_cs_get = rf_txpwr_cs_get;
    api->rssi_convert = rf_rssi_convert;
    api->rssi_high_thr = (uint8_t)RPL_RSSI_20dB_THRHLD;
    api->rssi_low_thr = (uint8_t)RPL_RSSI_60dB_THRHLD;
    api->rssi_interf_thr = (uint8_t)RPL_RSSI_70dB_THRHLD;
    #ifdef CFG_BTCORE_30
    api->wakeup_delay = RPL_WK_UP_DELAY;
    #endif //CFG_BTCORE_30
    api->skew = RPL_RADIO_SKEW;
    #endif //CFG_BT

    #if defined(CFG_BLE)
    //RADIOPWRUP
    ble_rtrip_delay_setf(0xC);
    ble_rxpwrup_setf(0x42);
    ble_txpwrdn_setf(0x07);
    ble_txpwrup_setf(0x56);

//SUGGESTIONS OF JPL, SEE ISSUE 572 		
    BLE->BLE_RADIOPWRUPDN_REG = 0x0c4a0056;
    

    #if !defined(CFG_BT)
    // First half part of frequency table is for the even frequencies
    #ifdef CFG_BLECORE_10
    for( idx=0; idx < EM_BLE_FREQ_TABLE_LEN; idx++)
    {
        temp_freq_tbl[idx] = 0;
        switch (idx)
        {
            case 37:
                temp_freq_tbl[idx] = 0;
                break;
            case 38:
                temp_freq_tbl[idx] = 24;
                break;
            case 39:
                temp_freq_tbl[idx] = 78;
                break;
            default:
                if(idx < 11)
                    temp_freq_tbl[idx] = 2*idx + 2;
                else
                    temp_freq_tbl[idx] = 2*idx + 4;
                break;
        }
    }
    #else //CFG_BLECORE_10
    while(idx < EM_BLE_FREQ_TABLE_LEN)
    {
        temp_freq_tbl[idx] = 2*idx + RPL_FREQTAB_OFFSET;
        idx++;
    }
    #endif //CFG_BLECORE_10

//    em_ble_burst_wr(&temp_freq_tbl[0], EM_BLE_FT_OFFSET, EM_BLE_FREQ_TABLE_LEN);
    em_ble_burst_wr(&temp_freq_tbl[0], EM_FT_OFFSET, EM_BLE_FREQ_TABLE_LEN);
 //   em_ble_burst_wr(&temp_freq_tbl[0], jump_table_struct[offset_em_ft] , EM_BLE_FREQ_TABLE_LEN);
    

    /*
    RADIOCNTL0
    | 31-26 | 25-24 | 23 |22|21-14|13-12|11|10|09|08|07|06|05|04|03|02|01|00|
    |VGACNTL|LNACNTL|AGCF|DP|     |RXLOW|-----|BC|BD|SP|SPI F|SM|-----|SC|SG|
 rst|000000 | 0   0 | 0  | 0|     | 1  0|     | 0| 1| 0| 0 0 | 0|     | 1| 0|

    0x010E0080
    */
    
//@wik, in this RADIOCNTL0 register a lot of bitfields regarding spi are not used, we use our own spi.
    ble_dpcorr_en_setf(1);
    ble_forceagc_en_setf(1);
    ble_forceagc_length_setf(0x420);
  //@wik    ble_rxlowif_setf(0);
  //@wik  ble_bidirclk_setf(0);
  //@wik  ble_bidirdata_setf(0);
  //@wik  ble_spipol_setf(1);
  //@wik  ble_spifreq_setf(0); //6.5MHz
  //@wik  ble_spimode_setf(0);
  //ble_spigo_setf(0); //@wik, commented because spi is done in SW iso SPIGO

    /*
    RADIOCNTL1
    |   31-21   |20|19|18|17|16|15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
    |-----------| XRFSEL       |          SPI POINTER                          |
    |xxxxxxxxxxx| 0  0  0  0  0|  pointer to the SPI buffer                    |
    */
    //@wik, this RADIOCNTL1 doesn't exist in the 580	
    //@wik  ble_xrfsel_setf(0x02); 
    //set ptr just once		
    //@wik ble_spiptr_setf(EM_BLE_RF_SPI_OFFSET);
    //@wik, but is exist in the FPGA
//GZ    ble_xrfsel_setf(0x03); 
    ble_xrfsel_setf(0x02); 
    
    //set ptr just once	
    //ble_spiptr_setf(EM_BLE_RF_SPI_OFFSET); //wik, commented this line, because pointers are handled in spi functions
    #endif // !defined CFG_BT
    #endif // defined CFG_BLE



    //init modem in core
    rf_rpl_mdm_init();

    //power up sequence for radio
    rf_rpl_pw_up();

    // Ripple Modem and RF TC1 initialization sequence
    rf_rpl_init_seq();

//@WIK, IMPORTANT !!!!!!! additional or different settings for FPGA -> BB_CLK must be divided by 4.
    rpl_crm_cntl1_set(0x200);

    // Settings for proper reception
    #ifdef CFG_BLE
    ble_forcebleiq_setf(1);
    ble_dpcorr_en_setf(0x0);
    ASSERT_ERR(ble_dpcorr_en_getf() == 0x0);
    #endif // CFG_BLE

    #ifdef CFG_BT
    bt_dpcorr_en_setf(0x1);
    ASSERT_ERR(bt_dpcorr_en_getf() == 0x1);
    #endif // CFG_BT
}


void rf_reinit_func(void)
{
	rf_in_sleep = false;
	
	REG_SET_BIT(BLE, BLE_CNTL2_REG, SW_RPL_SPI);
}
#endif
///@} RF_RPL

