#include "esphome/core/log.h"
#include "pthc_float_output.h"
#include "hal/mcpwm_ll.h"
#include "soc/mcpwm_reg.h"

extern void mcpwm_module_enable(mcpwm_unit_t mcpwm_num);

namespace esphome {
namespace pthc_float_output {

static const char *TAG = "pthc_float_output.output";

static bool IRAM_ATTR zcd_isr_handler(mcpwm_unit_t mcpwm, mcpwm_capture_channel_id_t cap_sig, const cap_event_data_t *edata, void *user_data) {

    zcdIntData_t *data = (zcdIntData_t *)user_data;

    if (mcpwm == MCPWM_UNIT_1 && cap_sig == MCPWM_SELECT_CAP0){
        
        uint32_t cDiff = (edata->cap_value - data->lastValue)/80; 
        //Disable ZCD on start of new half wave, triac is on until end of half wave, which is sync signal of mcpwm
        if(edata->cap_edge == MCPWM_NEG_EDGE && data->zcd_state == zcd_calibration_state_t::IDLE && (edata->cap_value-data->tmpValue) > (data->zeroCrossSignalDuration>>1)) {           
            gpio_set_level(ZCD_EN_GPIO,1);
        }
        //Init Data for measure
        else if(data->zcd_state == zcd_calibration_state_t::START_MEASURE){
            data->zcd_state = zcd_calibration_state_t::FIRST_HALF_WAVE;
            data->tmpValue = 0;
            data->halfWaveDuration = 0;
            data->zeroCrossSignalDuration = 0;
            data->lastValue = 0;
        }
        
        else if(edata->cap_edge == MCPWM_POS_EDGE && cDiff > 5000) {   
            if(data->zcd_state == zcd_calibration_state_t::FIRST_HALF_WAVE){
                data->zcd_state = zcd_calibration_state_t::SECOND_HALF_WAVE;
                data->tmpValue = edata->cap_value;
            }
            else if (data->zcd_state == zcd_calibration_state_t::SECOND_HALF_WAVE){
                data->zeroCrossSignalDuration = (data->lastValue - data->tmpValue)/80;
                data->halfWaveDuration = (edata->cap_value - data->lastValue)/80;
                
                if( data->retryCount > 10 || (data->zeroCrossSignalDuration > 200 && data->zeroCrossSignalDuration < 1000 && data->halfWaveDuration > 5000 && data->halfWaveDuration < 12000) ){
                    data->zcd_state = zcd_calibration_state_t::FINISHED;
                }else{
                    data->zcd_state = zcd_calibration_state_t::START_MEASURE;   
                    data->retryCount++;
                }
            }
            else if (data->zcd_state == zcd_calibration_state_t::FINISHED){
                data->tmpValue = edata->cap_value;
                data->retryCount = 0;
            }
        }
        //Store last capture value
        data->lastValue = edata->cap_value;
    } 

    return 0;
}

void PTHCFloatOutput::setup(){
    ESP_LOGCONFIG(TAG, "Setup");
    
    if(with_triac){
        relCfg = relCfgWithTriac;
    }else{
        relCfg = relCfgWithoutTriac;
    }
    
    if(phase_is_ccw){
        phaseAngle = phaseAngleCCW;
    }else{
        phaseAngle = phaseAngleCW;
    }
    
    esp_err_t err = 0;
    
    //Output Pins
    gpio_config_t gpioCfg = {
        .pin_bit_mask = (1ULL<<26) | (1ULL<<22) | (1ULL<<16) | (1ULL<<25) | (1ULL<<33) | (1ULL<<27) | (1ULL<<32) | (1ULL<<17) | (1ULL<<2) | (1ULL<<19),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    err = gpio_config(&gpioCfg);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "GPIO init failed Output Pins");
    }
    
    err = gpio_set_level(ZCD_EN_GPIO,0); 
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "Error init gpio 2");
    }    
    
    err = gpio_set_level(BOOST_GPIO,0); 
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "Error init gpio 22");
    }    
    
    //Input Pins
    gpioCfg.pin_bit_mask = (1ULL<<35);
    gpioCfg.pull_up_en = GPIO_PULLUP_ENABLE;
    gpioCfg.mode = GPIO_MODE_INPUT,
    err = gpio_config(&gpioCfg);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "GPIO init failed input Pins");
    }
    
    //PWM for Boost converter
    const ledc_timer_config_t ledcCfg = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_3,
        .freq_hz = 312500,
        .clk_cfg = LEDC_USE_APB_CLK,
    };
    
    
    err = ledc_timer_config(&ledcCfg);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "Ledc init failed");
    }
    
    const ledc_channel_config_t ledcChannelCfg = {
        .gpio_num = 22,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_7,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_3,
        .duty = 0,
        .hpoint = 0xff,
    };
    
    err = ledc_channel_config(&ledcChannelCfg);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "Ledc init failed Pin 22");
    }
    
    err = ledc_fade_func_install(0);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "Ledc fade install failed");
    }

#if DISABLE_ZERO_CROSS == 0

    err =  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0A, relPortNr[0]);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM GPIO init failed Pin %i", relPortNr[0]);
    }
    
    err =  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0B, relPortNr[1]);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM GPIO init failed Pin %i",relPortNr[1]);
    }
    
    err =  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1A, relPortNr[2]);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM GPIO init failed Pin %i", relPortNr[2]);
    }    

    err =  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1B, relPortNr[3]);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM GPIO init failed Pin %i", relPortNr[3]);
    }   
    
    err =  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM2A, relPortNr[4]);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM GPIO init failed Pin %i", relPortNr[4]);
    }
    
    err =  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM2B, relPortNr[5]);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM GPIO init failed Pin %i", relPortNr[5]);
    }    
    
    err =  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, relPortNr[6]);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM GPIO init failed Pin %i", relPortNr[6]);
    }

    err =  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM2A, relPortNr[7]);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM GPIO init failed Pin %i", relPortNr[7]);
    }
    
    
    err =  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM_CAP_0, 35);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM CAP0 GPIO init failed Pin 35");
    }        
    
    mcpwm_capture_config_t capCfg = {
        .cap_edge = MCPWM_BOTH_EDGE,
        .cap_prescale = 1,
        .capture_cb = zcd_isr_handler,
        .user_data = &zcdData,
    };
    
    err =  mcpwm_capture_enable_channel(MCPWM_UNIT_1, MCPWM_SELECT_CAP0, &capCfg);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM1 Capture enable error");
    }
     
    //just for enable modules
    capCfg.capture_cb = 0;
    capCfg.cap_edge = (mcpwm_capture_on_edge_t)0;
    err =  mcpwm_capture_enable_channel(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, &capCfg);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM0 Capture enable error");
    }
    //mcpwm_capture_disable_channel(MCPWM_UNIT_0, MCPWM_SELECT_CAP0);
    
    /*capCfg.cap_edge = MCPWM_BOTH_EDGE;
    err =  mcpwm_capture_enable_channel(MCPWM_UNIT_1, MCPWM_SELECT_CAP1, &capCfg);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM Capture enable error");
    }*/
    
    err =  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM_SYNC_0, 35);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM0 GPIO init failed Pin 35");
    }

    err =  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_SYNC_0, 35);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM1 GPIO init failed Pin 35");
    }
    
    mcpwm_sync_config_t syncCfg = {
        .sync_sig = MCPWM_SELECT_GPIO_SYNC0,
        .timer_val = 0,
        .count_direction = MCPWM_TIMER_DIRECTION_UP,
    };
    
    err = mcpwm_sync_configure(MCPWM_UNIT_0, MCPWM_TIMER_0, &syncCfg);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM0 Sync0 enable error");
    }
     
    err = mcpwm_sync_configure(MCPWM_UNIT_1, MCPWM_TIMER_0, &syncCfg);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM1 Sync0 enable error");
    }
    err = mcpwm_sync_configure(MCPWM_UNIT_1, MCPWM_TIMER_1, &syncCfg);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM1 Sync1 enable error");
    }    
    err = mcpwm_sync_configure(MCPWM_UNIT_1, MCPWM_TIMER_2, &syncCfg);
    if(err != ESP_OK){
        ESP_LOGCONFIG(TAG, "MCPWM1 Sync2 enable error");
    }        
    

    

        
#if 1
    
    for(uint8_t x=0;x<2;x++){
        ESP_LOGCONFIG(TAG, "Setup MCPWM%i",x);
        mcpwm_ll_group_set_clock_prescale(MCPWM_LL_GET_HW(x), 159);
        uint8_t maxTimer = 3;
        if(x == 0){
            maxTimer = 1;
        }
        //Timer 1 digit = 1us
        for(uint8_t i=0;i<maxTimer;i++){
            mcpwm_ll_timer_set_clock_prescale(MCPWM_LL_GET_HW(x), i, 8);
            mcpwm_ll_timer_set_peak(MCPWM_LL_GET_HW(x),i,0xFFFF,0); //Timer peak 512ms
            mcpwm_ll_timer_set_count_mode(MCPWM_LL_GET_HW(x), i, MCPWM_TIMER_COUNT_MODE_UP);
            mcpwm_ll_operator_enable_update_action_on_sync(MCPWM_LL_GET_HW(x), i, 1);
            //mcpwm_ll_operator_update_action_at_once(MCPWM_LL_GET_HW(x), i);
            for(uint8_t u=0;u<2;u++){
    //PWM DEBUG 50% duty @ 50hz
    #if 0
                mcpwm_ll_generator_set_action_on_timer_event(MCPWM_LL_GET_HW(x), i, u, MCPWM_TIMER_DIRECTION_DOWN, MCPWM_TIMER_EVENT_PEAK, MCPWM_GEN_ACTION_LOW);
                mcpwm_ll_generator_set_action_on_timer_event(MCPWM_LL_GET_HW(x), i, u, MCPWM_TIMER_DIRECTION_DOWN, MCPWM_TIMER_EVENT_ZERO, MCPWM_GEN_ACTION_LOW);
                mcpwm_ll_generator_set_action_on_timer_event(MCPWM_LL_GET_HW(x), i, u, MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_ZERO, MCPWM_GEN_ACTION_LOW);
                mcpwm_ll_generator_set_action_on_timer_event(MCPWM_LL_GET_HW(x), i, u, MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_PEAK, MCPWM_GEN_ACTION_LOW);
                mcpwm_ll_generator_set_action_on_compare_event(MCPWM_LL_GET_HW(x), i, u, MCPWM_TIMER_DIRECTION_DOWN, u, MCPWM_GEN_ACTION_HIGH);
                mcpwm_ll_generator_set_action_on_compare_event(MCPWM_LL_GET_HW(x), i, u, MCPWM_TIMER_DIRECTION_UP, u, MCPWM_GEN_ACTION_HIGH);
                mcpwm_ll_operator_set_compare_value(MCPWM_LL_GET_HW(x), i, u, 0x7FFF);
    #else
                mcpwm_ll_generator_set_action_on_timer_event(MCPWM_LL_GET_HW(x), i, u, MCPWM_TIMER_DIRECTION_DOWN, MCPWM_TIMER_EVENT_PEAK, MCPWM_GEN_ACTION_KEEP);
                mcpwm_ll_generator_set_action_on_timer_event(MCPWM_LL_GET_HW(x), i, u, MCPWM_TIMER_DIRECTION_DOWN, MCPWM_TIMER_EVENT_ZERO, MCPWM_GEN_ACTION_KEEP);
                mcpwm_ll_generator_set_action_on_timer_event(MCPWM_LL_GET_HW(x), i, u, MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_ZERO, MCPWM_GEN_ACTION_KEEP);
                mcpwm_ll_generator_set_action_on_timer_event(MCPWM_LL_GET_HW(x), i, u, MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_PEAK, MCPWM_GEN_ACTION_KEEP);
                mcpwm_ll_generator_set_action_on_compare_event(MCPWM_LL_GET_HW(x), i, u, MCPWM_TIMER_DIRECTION_DOWN, u, MCPWM_GEN_ACTION_KEEP);
                mcpwm_ll_generator_set_action_on_compare_event(MCPWM_LL_GET_HW(x), i, u, MCPWM_TIMER_DIRECTION_UP, u, MCPWM_GEN_ACTION_LOW);
                mcpwm_ll_operator_set_compare_value(MCPWM_LL_GET_HW(x), i, u, 0x1);
                mcpwm_ll_operator_update_compare_at_once(MCPWM_LL_GET_HW(x), i, u);            
    #endif
            }
            mcpwm_ll_timer_set_execute_command(MCPWM_LL_GET_HW(x), i, MCPWM_TIMER_START_NO_STOP);
        }
    }

    //Setup Timer for Triac
    mcpwm_ll_timer_set_clock_prescale(MCPWM_LL_GET_HW(0), 1, 1);
    mcpwm_ll_timer_set_clock_prescale(MCPWM_LL_GET_HW(0), 2, 1);

#endif
    
#endif
    ESP_LOGCONFIG(TAG, "MCPWM Setup Cmpl.");
    
    cState = IDLE;
 
}


uint32_t PTHCFloatOutput::calculateTimerValue(uint8_t state, uint8_t phaseAngleIdx, uint8_t pin){
    int32_t value;
    
    value = (halfWaveTimeUs*phaseAngle[phaseAngleIdx])/180;
    
    if(state)
        value -= rel_on_delay_us[pin];
    else
        value -= rel_off_delay_us[pin];
    
    //Add half of zeroCrossSignalDuration because real zero cross is in center of the Signal
    value += (zcdData.zeroCrossSignalDuration>>1);
    
    const int32_t limit = halfWaveTimeUs*2;
    
    while(value < limit){
        value += halfWaveTimeUs;        
    }
    
    //Use value nearest to two halfwaves
    if(std::abs(value-limit) > std::abs(value-limit- halfWaveTimeUs)){
        value -= halfWaveTimeUs;   
    }
    
    ESP_LOGI(TAG, "TimerVal: %i [us], phaseAngleIdx: %i State: %i ",value, phaseAngleIdx, state);
    
    return (uint32_t) value;
}

void PTHCFloatOutput::getTimerAndGen(uint8_t pin,uint8_t *hw_num, mcpwm_timer_t *timer_num, mcpwm_generator_t *gen){
        *hw_num = 1;
        switch(pin){
        case 0:
            *timer_num = MCPWM_TIMER_0;
            *gen = MCPWM_GEN_A;
            break;
        case 1:
            *timer_num = MCPWM_TIMER_0;
            *gen = MCPWM_GEN_B;
            break;        
        case 2:
            *timer_num = MCPWM_TIMER_1;
            *gen = MCPWM_GEN_A;
            break;   
        case 3:
            *timer_num = MCPWM_TIMER_1;
            *gen = MCPWM_GEN_B;
            break;              
        case 4:
            *timer_num = MCPWM_TIMER_2;
            *gen = MCPWM_GEN_A;
            break;   
        case 5:
            *timer_num = MCPWM_TIMER_2;
            *gen = MCPWM_GEN_B;
            break; 
        case 6:
            *hw_num = 0;
            *timer_num = MCPWM_TIMER_0;
            *gen = MCPWM_GEN_A;
            break;         
        default:
            return;
            break;        
    };
    
}



void PTHCFloatOutput::setupTimer(uint8_t pin, uint8_t state, uint32_t loadValue){
    mcpwm_timer_t timer_num;
    mcpwm_generator_t gen;
    uint8_t hw_num;
    esp_err_t err = 0;
    
    PTHCFloatOutput::getTimerAndGen(pin,&hw_num, &timer_num,&gen);
    
        
    ESP_LOGCONFIG(TAG, "MCPWM %i PWM Setup Timer: %i Gen: %i Load: %i State: %i Pin: %i",hw_num, timer_num, gen, loadValue, state, pin);
    if(loadValue > 0x7FFFF){
        ESP_LOGE(TAG, "Error Timerload value to high: %i", loadValue);
        return;
    }
    
    //div by 8 because 8us per clk
    mcpwm_ll_operator_set_compare_value(MCPWM_LL_GET_HW(hw_num), timer_num, gen, loadValue>>3);
    if(state){
        mcpwm_ll_generator_set_action_on_compare_event(MCPWM_LL_GET_HW(hw_num),timer_num,gen,MCPWM_TIMER_DIRECTION_UP,gen, MCPWM_GEN_ACTION_HIGH);
    }
    else{
        mcpwm_ll_generator_set_action_on_compare_event(MCPWM_LL_GET_HW(hw_num),timer_num,gen,MCPWM_TIMER_DIRECTION_UP,gen, MCPWM_GEN_ACTION_LOW);
    }
    
    static uint32_t maxTimerValue = 0;
    
    if(loadValue > maxTimerValue){
        maxTimerValue = loadValue;
        ESP_LOGI(TAG, "New max Loadvalue: %i", maxTimerValue);
    }

    return ;

}


void PTHCFloatOutput::loop() {
    
    esp_err_t err = 0;
    
    //increase millis to 64bit
    uint32_t cmillis = millis();
    static uint32_t last_millis_val = 0;
    static uint32_t overrun_counter_ts = 0;
    //Check for overrun an increase high bytes
    if(cmillis < last_millis_val){
        overrun_counter_ts++;
    }        
    last_millis_val = cmillis;
    uint64_t ts = ((uint64_t)overrun_counter_ts<<32) | cmillis;
    
    static uint64_t boostOffTimer = 0;
    static uint64_t relSwitchTimer = 0;
    static uint64_t startZCDTimer = 0;
    static uint64_t timerStopCounter = 0;
    static uint64_t delayedPwrLevelReqTimer = 0;
    
    static int8_t delayedPwrLevelReq = -1;
    
   
    //check for delyed pwr request
    if(this->cState == WAIT_FOR_DELAYED_REQ && delayedPwrLevelReqTimer < ts){
        this->cState = IDLE;
        this->pwrLevelReq = delayedPwrLevelReq;
        delayedPwrLevelReq = -1;
    }
    
    if(this->cState == IDLE){
        if(this->stateOverride >= 0 && this->stateOverride < PWR_STEPS){
            this->pwrLevelReq = this->stateOverride;
        }
        
        //on request for pwrLevelChange
        if( this->cPwrLevel != this->pwrLevelReq){
            
            //Check for forbidden Transitions            
            for(uint8_t i=0;i<forbiddenTransitionsSize;i++){
                if(forbiddenTransitions[i].from == this->cPwrLevel && forbiddenTransitions[i].to == this->pwrLevelReq){
                    ESP_LOGI(TAG, "Forbidden transition detected: %i -> %i Switch to %i", forbiddenTransitions[i].from, forbiddenTransitions[i].to, forbiddenTransitions[i].stepBetween );
                    delayedPwrLevelReq = this->pwrLevelReq;
                    this->pwrLevelReq = forbiddenTransitions[i].stepBetween;
                    break;
                }
            }          
            
            
            float setPwr = max_pwr*relCfg[this->pwrLevelReq].pwr;
            
            ESP_LOGI(TAG, "nPwr: %f", setPwr);
            
            //Check if Relays have to switched on
            std::memcpy(cRelCfg,&relCfg[this->cPwrLevel].L1RelState,sizeof(cRelCfg));
            std::memcpy(nRelCfg,&relCfg[this->pwrLevelReq].L1RelState,sizeof(nRelCfg));
            //cRelCfg = relCfg[this->cPwrLevel];
            //nRelCfg = relCfg[this->pwrLevelReq];
            
            uint8_t relNextStateOnCount = 0;
            uint8_t relChangeStateToOnCount = 0;
            
            nRelState = relCfg[this->pwrLevelReq].swRel | relCfg[this->pwrLevelReq].L1RelState | relCfg[this->pwrLevelReq].L2RelState | relCfg[this->pwrLevelReq].L3RelState | relCfg[this->pwrLevelReq].L1L2RelState | relCfg[this->pwrLevelReq].L2L3RelState | relCfg[this->pwrLevelReq].L3L1RelState;
            cRelState = relCfg[this->cPwrLevel].swRel | relCfg[this->cPwrLevel].L1RelState | relCfg[this->cPwrLevel].L2RelState | relCfg[this->cPwrLevel].L3RelState | relCfg[this->cPwrLevel].L1L2RelState | relCfg[this->cPwrLevel].L2L3RelState | relCfg[this->cPwrLevel].L3L1RelState;
            
            ESP_LOGI(TAG, "nRelCfg: %i", nRelState);
            
            for(uint8_t i=0; i < 8; i++){
                if(nRelState & (1<<i)){
                    relNextStateOnCount++;
                    if(!(cRelState & (1<<i))){
                        relChangeStateToOnCount++;                    
                    }
                }
            }
            
            ESP_LOGI(TAG, "%i Relays change state to On. %i Relays on", relChangeStateToOnCount, relNextStateOnCount);
            //If relays need to be turned on ... start Boost
            if(relChangeStateToOnCount && boostOffTimer < ts){
                if(boost_override_settings){
                    err = ledc_set_freq(LEDC_HIGH_SPEED_MODE,LEDC_TIMER_3, boost_freq);
                    if(err != ESP_OK){
                        ESP_LOGE(TAG, "Error updating PWM freq.");
                    }       
                    
                    err = ledc_set_duty(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_7, boost_duty);
                    if(err != ESP_OK){
                        ESP_LOGE(TAG, "Error updating PWM set duty");
                    }        
                    
                    //err = ledc_update_duty(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_7);
                    err = ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_7, boostCfg[relNextStateOnCount].duty,10);
                    if(err != ESP_OK){
                        ESP_LOGE(TAG, "Error updating PWM update duty");
                    }   
                } else {
                    err = ledc_set_freq(LEDC_HIGH_SPEED_MODE,LEDC_TIMER_3, boostCfg[relNextStateOnCount].freq);
                    if(err != ESP_OK){
                        ESP_LOGE(TAG, "Error updating PWM freq.");
                    }       
                    
                    //err = ledc_set_duty(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_7, boostCfg[relNextStateOnCount].duty);
                    err = ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_7, boostCfg[relNextStateOnCount].duty,10);
                    
                    if(err != ESP_OK){
                        ESP_LOGE(TAG, "Error updating PWM set duty");
                    }        
                    
                    err = ledc_update_duty(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_7);
                    if(err != ESP_OK){
                        ESP_LOGE(TAG, "Error updating PWM update duty");
                    }      
                }           
                boostOffTimer = ts + BOOST_ON_TIME;
                relSwitchTimer = ts + BOOST_ON_BEFORE_SWITCH;
                
                this->cState = WAIT_FOR_BOOST;
            }else{
               this->cState = LOAD_TIMER_AND_WAIT_FOR_NET_SYNC;
            }                
            //Enable ZCD for sync with mains freq.
            zcdData.zcd_state = zcd_calibration_state_t::START_MEASURE;
            gpio_set_level(ZCD_EN_GPIO,0);
            ESP_LOGI(TAG, "Enable ZCD");     
        }
    }

    if(this->cState == WAIT_FOR_BOOST && relSwitchTimer < ts){    
        this->cState = LOAD_TIMER_AND_WAIT_FOR_NET_SYNC;        
    }
    
    if(this->cState == LOAD_TIMER_AND_WAIT_FOR_NET_SYNC && zcdData.zcd_state == zcd_calibration_state_t::FINISHED){
        
        ESP_LOGI(TAG, "ZCD Halfwave: %i ZeroCrossSig: %i",zcdData.halfWaveDuration,zcdData.zeroCrossSignalDuration);
        
        halfWaveTimeUs = zcdData.halfWaveDuration + zcdData.zeroCrossSignalDuration;
        
        #if DISABLE_ZERO_CROSS
        
        for(uint8_t i=0; i<8; i++){
            if(nRelState&(1<<i))
                err = gpio_set_level(relPortNr[i],1); 
            else
                err = gpio_set_level(relPortNr[i],0); 
            
            if(err != ESP_OK){
                ESP_LOGE(TAG, "Error updating gpio");
            }    
        }
        cState = WAIT_FOR_DISABLE_BOOST;
        
        #else
        
        //Switch Relays without phase
        for(uint8_t i=0; i<8; i++){
            if(relCfg[this->pwrLevelReq].swRel&(1<<i)){
                err = gpio_set_level(relPortNr[i],1); 
                ESP_LOGI(TAG, "Direct switch on relay: %i", i);
            }
            else if(relCfg[this->cPwrLevel].swRel & (1<<i)){
                err = gpio_set_level(relPortNr[i],0); 
                ESP_LOGI(TAG, "Direct switch off relay: %i", i);
            }
            
            if(err != ESP_OK){
                ESP_LOGE(TAG, "Error updating gpio");
            }    
        }
 

        uint8_t sw_off_count = 0;
        uint32_t maxSwitchOffTime = 0;
        for(uint8_t i=0; i < 8; i++){
            const uint8_t sv = (1<<i);  
            for(uint32_t u=0;u<6;u++){
                //Check for Switch Off
                if((cRelState & sv) && (!(nRelState & sv)) && (cRelCfg[u] & sv) ){
                    uint32_t timerValue = calculateTimerValue(0, u, i);
                    //delay switch off
                    if(cRelCfg[u] & DELAY_SW_OFF){
                        timerValue += (halfWaveTimeUs*4);
                        ESP_LOGI(TAG, "Delayed switch off"); 
                    }
                    if(cRelCfg[u] & STAGGERED_SWITCH_OFF){
                        timerValue += (halfWaveTimeUs*(sw_off_count*2+4));
                        ESP_LOGI(TAG, "Staggered switch off"); 
                    }
                    
                    if(cRelCfg[u] & ADD_TWO_CYLCE_MARKER_OFF && i == ((cRelCfg[u]>>ADD_TWO_CYLCE_NR_SHIFT)&0x7)){
                        timerValue += (halfWaveTimeUs*6);
                        ESP_LOGI(TAG, "Add two cycle"); 
                    }
                    
                    if(timerValue > (maxSwitchOffTime + rel_off_delay_us[i])){
                        maxSwitchOffTime = (timerValue + rel_off_delay_us[i]);
                    }
                    
                    timerValue -= 500; //Switch a bit bevore zero cross
                    
                    setupTimer(i,0,timerValue);
                    sw_off_count++;
                }
            }
        }
        
        //Delay on until all off states switched
        uint32_t onDelayAdder = 0;        
        if(sw_off_count){            
            onDelayAdder = halfWaveTimeUs*((maxSwitchOffTime / halfWaveTimeUs)+6);
            ESP_LOGI(TAG, "Set onDelayAdder to: %i", onDelayAdder); 
        }
        
        uint8_t phaseAngleCount = 0;
        for(uint32_t u=0;u<6;u++){
            bool swFlg = 0;
            for(uint8_t i=0; i < 8; i++){
                const uint8_t sv = (1<<i);  
                //Check for Switch On                
                if((!(cRelState & sv)) && (nRelCfg[u] & sv)){
                    uint32_t timerValue = calculateTimerValue(1, u, i);
                    
                    if(nRelCfg[u] & SWITCH_RELAY_LAST){
                        timerValue += (halfWaveTimeUs*4);
                        ESP_LOGI(TAG, "Delay Switch On"); 
                    }  
                    
                    if(nRelCfg[u] & ADD_TWO_CYLCE_MARKER_ON && i == ((nRelCfg[u]>>ADD_TWO_CYLCE_NR_SHIFT)&0x7)){
                        timerValue += (halfWaveTimeUs*4);
                        ESP_LOGI(TAG, "Add two cycle"); 
                    }                    
                    
                    /*if(phaseAngleCount){
                        timerValue += (halfWaveTimeUs*4*phaseAngleCount);
                        ESP_LOGI(TAG, "Add %i cycle for different Phaseangle", phaseAngleCount*2);
                    }*/
                    
                    timerValue += onDelayAdder;
                    
                    setupTimer(i,1,timerValue);
                    swFlg = 1;

                }
            }
            if(swFlg){
                phaseAngleCount++;
            }
        }        
        
        timerStopCounter = ts + 100;
               
        cState = WAIT_FOR_DISABLE_TIMER;
        
        zcdData.zcd_state = zcd_calibration_state_t::IDLE;
        
        #endif
        
        //Reload Timer to compensate delays in loop
        boostOffTimer = ts + (BOOST_ON_TIME - BOOST_ON_BEFORE_SWITCH);        
    }
    
    if(this->cState == WAIT_FOR_DISABLE_TIMER && timerStopCounter < ts){        
        timerStopCounter = 0;   
        this->cState = WAIT_FOR_DISABLE_BOOST;        
    }
    
    if(cState == WAIT_FOR_DISABLE_BOOST && boostOffTimer < ts){         
        //err = ledc_set_duty(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_7, 0);
        err = ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_7, 0,10);
        if(err != ESP_OK){
            ESP_LOGE(TAG, "Error updating PWM set duty");
        }        
        
        err = ledc_update_duty(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_7);
        if(err != ESP_OK){
            ESP_LOGE(TAG, "Error updating PWM update duty");
        }   
        
        this->cPwrLevel = this->pwrLevelReq;
        
        if(delayedPwrLevelReq < 0){
            this->cState = IDLE;
        }else{
            delayedPwrLevelReqTimer = ts + 1000; 
            this->cState = WAIT_FOR_DELAYED_REQ;
        }
         
        
        ESP_LOGI(TAG, "Disable Boost, Switch finished");
    }

}


void PTHCFloatOutput::write_state(float state){
    uint8_t i;
    static float lastValue = 0.0f;
    
    if(state == lastValue)
        return;
    
    lastValue = state;
    
    ESP_LOGCONFIG(TAG, "Change State to %f", state);
    for(i=PWR_STEPS-1;i;i--){
        if(relCfg[i].pwr <= state){
            break;            
        }        
    }
        
    if(this->cState != IDLE){
        ESP_LOGE(TAG, "Last Change State not complete");
    }
    
    if(i < PWR_STEPS){
        ESP_LOGI(TAG, "Change State to %i", i);
        this->pwrLevelReq = i;
    }
    else{
        ESP_LOGE(TAG, "wrong state: %i", i);
    }
}

void PTHCFloatOutput::dump_config() {
    //ESP_LOGCONFIG(TAG, "Empty custom float output");
}

} //namespace pthc_float_output
} //namespace esphome

