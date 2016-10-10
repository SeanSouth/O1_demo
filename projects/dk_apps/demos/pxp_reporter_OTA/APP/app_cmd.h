/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#ifndef APP_CMD
#define APP_CMD
 
#include "..\LIB\cJSON.h"
#include "sdk_defs.h"

    #define MAX_CMD 256
    #define TIME_FMT "%Y-%m-%d %H:%M:%S"
    #define SENSOR_DATA_LEN     (12)  
    
    
    
    typedef struct _data_buf
    {
        uint8       buf[MAX_CMD];
        uint16      length;
    }data_buf;    
    
    typedef cJSON * (*fun)(cJSON *);
    
    typedef struct _cmd_table
    {
        uint8       cmd;
        fun         funcptr;
    }cmd_table;
  
    
//    cJSON * cmd_set_time(cJSON *rxbuf);
//    cJSON * cmd_get_time(cJSON *rxbuf);
//    cJSON * cmd_get_state(cJSON *rxbuf);
//    cJSON * cmd_set_userid(cJSON *rxbuf);
//    cJSON * cmd_get_userid(cJSON *rxbuf);
//    cJSON * cmd_restore_factory(cJSON *rxbuf);
//    cJSON * cmd_get_position(cJSON *rxbuf);
//    cJSON * cmd_get_last_record(cJSON *rxbuf);
//    cJSON * cmd_get_all_record(cJSON *rxbuf);
//    cJSON * cmd_get_specified_record(cJSON *rxbuf);
//    cJSON * cmd_get_pos_model(cJSON *rxbuf);
//    cJSON * cmd_set_pos_model(cJSON *rxbuf);
//    cJSON * cmd_set_interactive(cJSON *rxbuf);
//    cJSON * cmd_set_pause(cJSON *rxbuf);
//    cJSON * cmd_clean_brush_data(cJSON *rxbuf);
//    cJSON * cmd_set_led1(cJSON *rxbuf);
//    cJSON * cmd_set_led2(cJSON *rxbuf);
//    cJSON * cmd_set_motor(cJSON *rxbuf);
//    cJSON * cmd_let_dev_reset(cJSON *rxbuf);
//    cJSON * cmd_get_uniqueid(cJSON *rxbuf);
//    cJSON * cmd_get_qrcode(cJSON *rxbuf);
//    cJSON * cmd_enter_bootloader(cJSON *rxbuf);
//    cJSON * cmd_binding_req(cJSON *rxbuf);
//    cJSON * cmd_complete_packet(cJSON *rxbuf);
//    cJSON * cmd_get_bat(cJSON *rxbuf);
//    cJSON * cmd_ble_disconnect(cJSON *rxbuf);
//    cJSON * cmd_check_switch(cJSON *rxbuf);
//    cJSON * cmd_set_dominant_hand(cJSON *rxbuf);
//    cJSON * cmd_set_current_apppos(cJSON *rxbuf);
//    cJSON * cmd_set_calibration(cJSON *rxbuf);
//
    void cmd_async_feedback_F1(uint8 res);
//    void cmd_async_feedback_F2(uint8 state);
//    void cmd_async_feedback_F3(void);
//    void cmd_async_feedback_F4(uint8 pos);
//    void cmd_async_feedback_F5(void);
//    void cmd_async_feedback_F6(uint8 percent);
//    void cmd_async_feedback_F7(char* debug);
    
    void cmd_parse(uint8 *in,uint16 inlen,uint8 *out,uint16 *outlen,uint8 *txcmd);
    
#endif
/* [] END OF FILE */
