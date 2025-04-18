#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/output/float_output.h"
#include "driver/mcpwm.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include <cstdint>
#include <cstring>
#include <iostream>

namespace esphome {
namespace pthc_float_output {
    
#define BOOST_GPIO GPIO_NUM_22 
#define ZCD_EN_GPIO GPIO_NUM_2
#define ZCD_SIG_GPIO GPIO_NUM_35
    
#define SWR0 (1<<0)
#define SWR1 (1<<1)
#define SWR2 (1<<2)    
#define SWR3 (1<<3)   
#define SWR4 (1<<4)      
#define SWR5 (1<<5)    
#define SWR6 (1<<6)     
#define SWR7 (1<<7)    
    
//Spezial Handling Flags
#define DELAY_SW_OFF (1<<8)
#define STAGGERED_SWITCH_OFF (1<<9)
#define SWITCH_RELAY_LAST (1<<10)
#define ADD_TWO_CYLCE_MARKER_OFF (1<<11)
#define ADD_TWO_CYLCE_MARKER_ON (1<<12)
#define ADD_TWO_CYLCE_NR_SHIFT 13
#define ADD_TWO_CYLCE_TO(x) ((x<<ADD_TWO_CYLCE_NR_SHIFT))
    

    
#define PWR_STEPS 17

#define DISABLE_ZERO_CROSS    0

#define BOOST_ON_TIME 500
#define BOOST_ON_BEFORE_SWITCH 50    
    
#define forbiddenTransitionsSize 25
    
typedef struct{
    float pwr;
    uint16_t swRel;
    uint16_t L1RelState;
    uint16_t L2RelState;
    uint16_t L3RelState;
    uint16_t L1L2RelState;
    uint16_t L2L3RelState;
    uint16_t L3L1RelState;
} relCfg_t;    

typedef struct{
    uint32_t freq;
    uint32_t duty;
} boostCfg_t; 

typedef enum zcd_calibration_state {IDLE, START_MEASURE, FIRST_HALF_WAVE, SECOND_HALF_WAVE, FINISHED } zcd_calibration_state_t;

typedef struct{
    uint32_t halfWaveDuration;
    uint32_t zeroCrossSignalDuration;
    uint32_t tmpValue;
    uint32_t lastValue;
    uint8_t retryCount;
    zcd_calibration_state_t zcd_state;
} zcdIntData_t;

typedef struct{
    uint8_t from;
    uint8_t to;
    uint8_t stepBetween;
} transition_t; 


class PTHCFloatOutput : public output::FloatOutput, public Component {
    typedef enum pthc_state {SETUP, IDLE, WAIT_FOR_BOOST, LOAD_TIMER_AND_WAIT_FOR_NET_SYNC, WAIT_FOR_DISABLE_TIMER, WAIT_FOR_DISABLE_BOOST, WAIT_FOR_DELAYED_REQ } pthc_state_t;
public:
    void setup() override;
    void loop() override;
    void write_state(float state) override;
    void dump_config() override;

    void set_with_triac(bool with_triac) { this->with_triac = with_triac; }
    void set_phase_is_ccw(bool phase_is_ccw) { this->phase_is_ccw = phase_is_ccw; }
    void set_max_pwr(int max_pwr) { this->max_pwr = max_pwr; }
    void set_rel_on_delay_us(const std::array<int, 8>& delays) { this->rel_on_delay_us = delays; }
    void set_rel_off_delay_us(const std::array<int, 8>& delays) { this->rel_off_delay_us = delays; }
    
    
    uint32_t boost_freq = 0;
    uint32_t boost_duty = 0;
    uint8_t boost_override_settings = 0; 
    int8_t stateOverride = -1;

    static void getTimerAndGen(uint8_t pin,uint8_t *hw_num, mcpwm_timer_t *timer_num, mcpwm_generator_t *gen);

    zcdIntData_t zcdData = { .halfWaveDuration = 0, .zeroCrossSignalDuration = 0, .tmpValue = 0, .lastValue = 0, .retryCount = 0, .zcd_state = zcd_calibration_state_t::IDLE };
  
protected:
    bool with_triac = false;
    bool phase_is_ccw = false;
    int max_pwr = 3000;
  
    //Calibration Values for Relays
    //Omron
    //on delay 2,96ms - 4,2ms 
    //off delay 960us - 1ms

    //songle
    //On delay 1,66ms - 2.46ms
    //off delay 920us  
    std::array<int, 8> rel_on_delay_us{3580, 3580, 3580, 3580, 3580, 2060, 2060, 0};
    std::array<int, 8> rel_off_delay_us{980, 980, 980, 980, 980, 920, 920, 0};
  
private:
    
    uint32_t calculateTimerValue(uint8_t state, uint8_t phaseAngleIdx, uint8_t pin);
    void setupTimer(uint8_t pin, uint8_t state, uint32_t loadValue);

    
    pthc_state_t cState = SETUP;
    
    uint8_t cPwrLevel = 0;
    uint8_t pwrLevelReq = 0;
    uint16_t cRelCfg[6];
    uint16_t nRelCfg[6];
    uint8_t nRelState;
    uint8_t cRelState;
    
    int32_t halfWaveTimeUs;
    

    //Phase Angle Table CCW
    const uint8_t phaseAngleCCW[6] = { 0, 60, 120, 30, 90, 150 };
    //Phase Angle Table CW
    const uint8_t phaseAngleCW[6] = { 0, 120, 60, 150, 90, 30 };    
    uint8_t const *phaseAngle;
    
    int8_t delayedPwrLevelReq = -1;
    uint64_t delayedPwrLevelReqTimer = 0;
    
    const transition_t forbiddenTransitions[forbiddenTransitionsSize] = {
        { .from = 3, .to = 6, .stepBetween = 0 },       
        { .from = 3, .to = 8, .stepBetween = 0 },      
        { .from = 3, .to = 10, .stepBetween = 0 },  
        { .from = 3, .to = 13, .stepBetween = 0 },
        { .from = 3, .to = 15, .stepBetween = 0 },  
        
        { .from = 6, .to = 3, .stepBetween = 0 },       
        { .from = 8, .to = 3, .stepBetween = 0 },      
        { .from = 10, .to = 3, .stepBetween = 0 },  
        { .from = 13, .to = 3, .stepBetween = 0 },
        { .from = 15, .to = 3, .stepBetween = 0 },  
        
        { .from = 6, .to = 5, .stepBetween = 0 },       
        { .from = 8, .to = 5, .stepBetween = 0 },      
        { .from = 10, .to = 5, .stepBetween = 0 },  
        { .from = 13, .to = 5, .stepBetween = 0 },
        { .from = 15, .to = 5, .stepBetween = 0 },      
        
        { .from = 10, .to = 6, .stepBetween = 0 },   
        { .from = 10, .to = 8, .stepBetween = 0 }, 
        { .from = 10, .to = 13, .stepBetween = 0 },   
        { .from = 10, .to = 15, .stepBetween = 0 }, 
        
        { .from = 6, .to = 10, .stepBetween = 0 },   
        { .from = 8, .to = 10, .stepBetween = 0 }, 
        { .from = 13, .to = 10, .stepBetween = 0 },   
        { .from = 15, .to = 10, .stepBetween = 0 }, 
        
        { .from = 5, .to = 13, .stepBetween = 0 },  
        { .from = 5, .to = 6, .stepBetween = 0 },  
    }; 
        
    
    const boostCfg_t boostCfg[7] = {
        //1 Relay On
        {
            .freq = 10000,
            .duty = 40,
        },
        //2 Relay On
        {
            .freq = 25000,
            .duty = 90,
        },  
        //3 Relay On
        {
            .freq = 30000,
            .duty = 120,
        },      
        //4 Relay On
        {
            .freq = 120000,
            .duty = 170,
        },   
        //5 Relay On
        {
            .freq = 120000,
            .duty = 175,
        },         
        //6 Relay On
        {
            .freq = 120000,
            .duty = 175,
        },         
        //7 Relay On
        {
            .freq = 120000,
            .duty = 175,
        },         
    };
    
    const gpio_num_t relPortNr[8] = { GPIO_NUM_16, GPIO_NUM_33, GPIO_NUM_32, GPIO_NUM_17, GPIO_NUM_26, GPIO_NUM_25, GPIO_NUM_27, GPIO_NUM_19 };
    
    relCfg_t const *relCfg = relCfgWithoutTriac;
    
    const relCfg_t relCfgWithTriac[PWR_STEPS] = {
        //Pwr level 0
        { .pwr = 0,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = 0,
          .L2L3RelState = 0,
          .L3L1RelState = 0,
        },
        //Pwr level 1 -> N  - - - L2
        { .pwr = 0.0367,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = SWR6,
          .L3RelState = 0,
          .L1L2RelState = 0,
          .L2L3RelState = 0,
          .L3L1RelState = 0,
          
        },     
        //Pwr level 2 -> N - - L3
        { .pwr = 0.055,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = SWR6|SWR3|STAGGERED_SWITCH_OFF,
          .L1L2RelState = 0,
          .L2L3RelState = 0,
          .L3L1RelState = 0,
        },     
        //Pwr level 3 -> N = - L2
        { .pwr = 0.0734,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = SWR5,
          .L3RelState = 0,
          .L1L2RelState = 0,
          .L2L3RelState = 0,
          .L3L1RelState = 0,
        },    
        
        //Pwr level 4 -> N - L1
        { .pwr = 0.1111,
          .swRel = 0,
          .L1RelState = SWR6|SWR1,
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = 0,
          .L2L3RelState = 0,
          .L3L1RelState = 0,
        },      
        
        //Pwr level 5 -> N - L3 + N - - L2
        //Achtung Sicherstellen das SWR5 vor SWR3 schaltet
        { .pwr = 0.1666666,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = SWR5,
          .L3RelState = SWR3|SWITCH_RELAY_LAST,
          .L1L2RelState = 0,
          .L2L3RelState = 0,
          .L3L1RelState = 0,
        },          
        
        //Pwr level 6 -> L2 - SP - L1 + N - SP
        //Achtung Sicherstellen das SWR1 vor SWR5 schaltet
        { .pwr = 0.1833333,
          .swRel = 0,
          .L1RelState = SWR5|SWITCH_RELAY_LAST, //Zerocross on SP is equal to L1 Phase  
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = SWR1|DELAY_SW_OFF|ADD_TWO_CYLCE_MARKER_OFF|ADD_TWO_CYLCE_TO(1),
          .L2L3RelState = 0,
          .L3L1RelState = 0,
        },          
        
        //Pwr level 7 -> N - L2 + N - L3
        //Achtung Sicherstellen das SWR5/0 vor SWR3 schaltet
        { .pwr = 0.2204,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = SWR5|SWR0,
          .L3RelState = SWR3|SWITCH_RELAY_LAST,
          .L1L2RelState = 0,
          .L2L3RelState = 0,
          .L3L1RelState = 0,
        },    
        
        //Pwr level 8 -> L2 - - L1 + N - L3
        //Achtung erst SWR1 dann SWR3/SWR5
        { .pwr = 0.277733333,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = SWR3|SWR5|SWITCH_RELAY_LAST,
          .L1L2RelState = SWR1|DELAY_SW_OFF,
          .L2L3RelState = 0,
          .L3L1RelState = 0,
        },            
        //Pwr level 9 -> L2 - L3
        { .pwr = 0.333333333,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = 0,
          .L2L3RelState = SWR4,
          .L3L1RelState = 0,
        },       
        //Pwr level 10 -> L2 - L3 + N - - L1
        { .pwr = 0.3884,
          .swRel = 0,
          .L1RelState = SWR2|SWR5|DELAY_SW_OFF|ADD_TWO_CYLCE_MARKER_OFF|ADD_TWO_CYLCE_TO(2),
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = 0,
          .L2L3RelState = SWR4,
          .L3L1RelState = 0,
        },    
        //Pwr level 11 -> L2 - L3 + N - L1
        { .pwr = 0.4444,
          .swRel = 0,
          .L1RelState = SWR1|SWR6|SWITCH_RELAY_LAST,
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = 0,
          .L2L3RelState = SWR4,
          .L3L1RelState = 0,
        },     
        //Pwr level 12 -> L1 - L3 + L2 - - L1
        { .pwr = 0.5,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = SWR1,
          .L2L3RelState = 0,
          .L3L1RelState = SWR2|SWR3,
        },   
        //Pwr level 13 -> L2 - L3 + N - L1 + N - L3
        { .pwr = 0.5533333,
          .swRel = 0,
          .L1RelState = SWR1|SWR6|DELAY_SW_OFF,
          .L2RelState = 0,
          .L3RelState = SWR3|SWR5|SWITCH_RELAY_LAST|ADD_TWO_CYLCE_MARKER_ON|ADD_TWO_CYLCE_TO(5),
          .L1L2RelState = 0,
          .L2L3RelState = SWR4|DELAY_SW_OFF|ADD_TWO_CYLCE_MARKER_ON|ADD_TWO_CYLCE_MARKER_OFF|ADD_TWO_CYLCE_TO(4),
          .L3L1RelState = 0,
        },  
        //Pwr level 14 -> L2 - L3 + L1 - L2
        { .pwr = 0.666666666,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = SWR1|SWR0|DELAY_SW_OFF|SWITCH_RELAY_LAST,
          .L2L3RelState = SWR4,
          .L3L1RelState = 0,
        },        
        //Pwr level 15 -> L2 - L3 + L1 - L2 + N - L3
        { .pwr = 0.77773333,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = SWR5|SWR3,
          .L1L2RelState = SWR1|SWR0|DELAY_SW_OFF|SWITCH_RELAY_LAST,
          .L2L3RelState = SWR4,
          .L3L1RelState = 0,
        },           
        
        //Pwr level 16 -> L2 - L3 + L1 - L2 + L1 - L3
        { .pwr = 1.0,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = SWR1|SWR0|DELAY_SW_OFF|SWITCH_RELAY_LAST,
          .L2L3RelState = SWR4,
          .L3L1RelState = SWR2|SWR3,
        },   
    };

    
    const relCfg_t relCfgWithoutTriac[PWR_STEPS] = {
        //Pwr level 0
        { .pwr = 0,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = 0,
          .L2L3RelState = 0,
          .L3L1RelState = 0,
        },
        //Pwr level 1 -> N  - - - L2
        { .pwr = 0.0367,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = SWR6,
          .L3RelState = 0,
          .L1L2RelState = 0,
          .L2L3RelState = 0,
          .L3L1RelState = 0,
          
        },     
        //Pwr level 2 -> N - - L3
        { .pwr = 0.055,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = SWR6|SWR3|STAGGERED_SWITCH_OFF,
          .L1L2RelState = 0,
          .L2L3RelState = 0,
          .L3L1RelState = 0,
        },     
        //Pwr level 3 -> N = - L2
        { .pwr = 0.0734,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = SWR5,
          .L3RelState = 0,
          .L1L2RelState = 0,
          .L2L3RelState = 0,
          .L3L1RelState = 0,
        },    
               
        //Pwr level 4 -> L2 - - - L3
        //Better without Triac
        { .pwr = 0.1111,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = 0,
          .L2L3RelState = SWR3,
          .L3L1RelState = 0,
        },              
        
        
        //Pwr level 5 -> N - L3 + N - - L2
        //Achtung Sicherstellen das SWR5 vor SWR3 schaltet
        { .pwr = 0.1666666,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = SWR5,
          .L3RelState = SWR3|SWITCH_RELAY_LAST,
          .L1L2RelState = 0,
          .L2L3RelState = 0,
          .L3L1RelState = 0,
        },          
        
        //Pwr level 6 -> L2 - SP - L1 + N - SP
        //Achtung Sicherstellen das SWR1 vor SWR5 schaltet
        { .pwr = 0.1833333,
          .swRel = 0,
          .L1RelState = SWR5|SWITCH_RELAY_LAST, //Zerocross on SP is equal to L1 Phase  
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = SWR1|DELAY_SW_OFF|ADD_TWO_CYLCE_MARKER_OFF|ADD_TWO_CYLCE_TO(1),
          .L2L3RelState = 0,
          .L3L1RelState = 0,
        },          
        
        //Pwr level 7 -> L2 - = L1
        { .pwr = 0.2204,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = SWR1|SWR2,
          .L2L3RelState = 0,
          .L3L1RelState = 0,
        },
        
        //Pwr level 8 -> L2 - - L1 + N - L3
        //Achtung erst SWR1 dann SWR3/SWR5
        { .pwr = 0.277733333,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = SWR3|SWR5|SWITCH_RELAY_LAST,
          .L1L2RelState = SWR1|DELAY_SW_OFF,
          .L2L3RelState = 0,
          .L3L1RelState = 0,
        },            
        //Pwr level 9 -> L2 - L3
        { .pwr = 0.333333333,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = 0,
          .L2L3RelState = SWR4,
          .L3L1RelState = 0,
        },       
        //Pwr level 10 -> L2 - L3 + N - - L1
        { .pwr = 0.3884,
          .swRel = 0,
          .L1RelState = SWR2|SWR5|DELAY_SW_OFF|ADD_TWO_CYLCE_MARKER_OFF|ADD_TWO_CYLCE_TO(2),
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = 0,
          .L2L3RelState = SWR4,
          .L3L1RelState = 0,
        },    
        //Pwr level 11 -> L2 - L3 + N - L1
        { .pwr = 0.4444,
          .swRel = 0,
          .L1RelState = SWR1|SWR6|SWITCH_RELAY_LAST,
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = 0,
          .L2L3RelState = SWR4,
          .L3L1RelState = 0,
        },     
        //Pwr level 12 -> L1 - L3 + L2 - - L1
        { .pwr = 0.5,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = SWR1,
          .L2L3RelState = 0,
          .L3L1RelState = SWR2|SWR3,
        },   
        //Pwr level 13 -> L2 - L3 + N - L1 + N - L3
        { .pwr = 0.5533333,
          .swRel = 0,
          .L1RelState = SWR1|SWR6|DELAY_SW_OFF,
          .L2RelState = 0,
          .L3RelState = SWR3|SWR5|SWITCH_RELAY_LAST|ADD_TWO_CYLCE_MARKER_ON|ADD_TWO_CYLCE_TO(5),
          .L1L2RelState = 0,
          .L2L3RelState = SWR4|DELAY_SW_OFF|ADD_TWO_CYLCE_MARKER_ON|ADD_TWO_CYLCE_MARKER_OFF|ADD_TWO_CYLCE_TO(4),
          .L3L1RelState = 0,
        },      
        //Pwr level 14 -> L2 - L3 + L1 - L2
        { .pwr = 0.666666666,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = SWR1|SWR0|DELAY_SW_OFF|SWITCH_RELAY_LAST,
          .L2L3RelState = SWR4,
          .L3L1RelState = 0,
        },    
        //Pwr level 15 -> L2 - L3 + L1 - L2 + N - L3
        { .pwr = 0.77773333,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = SWR5|SWR3,
          .L1L2RelState = SWR1|SWR0|DELAY_SW_OFF|SWITCH_RELAY_LAST,
          .L2L3RelState = SWR4,
          .L3L1RelState = 0,
        },           
        
        //Pwr level 16 -> L2 - L3 + L1 - L2 + L1 - L3
        { .pwr = 1.0,
          .swRel = 0,
          .L1RelState = 0,
          .L2RelState = 0,
          .L3RelState = 0,
          .L1L2RelState = SWR1|SWR0|DELAY_SW_OFF|SWITCH_RELAY_LAST,
          .L2L3RelState = SWR4,
          .L3L1RelState = SWR2|SWR3,
        },     
    };    

};


} //namespace empty_float_output
} //namespace esphome
