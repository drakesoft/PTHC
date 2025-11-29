/*
 * PTHC - Power To Heat Controller
 * 
 * Copyright (C) 2023-2025 draketronic / Maximilian Niedernhuber
 * 
 * This file is part of the PTHC project.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 * Author: Maximilian Niedernhuber
 * Project: https://github.com/drakesoft/PTHC
 */

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/output/float_output.h"
#include "driver/mcpwm.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_idf_version.h"
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
#define ADD_DELAY_ON_NR_SHIFT 10
#define ADD_DELAY_ON_TO(x) (((x+1)<<ADD_DELAY_ON_NR_SHIFT))

#define ADD_DELAY_OFF_NR_SHIFT 14
#define ADD_DELAY_OFF_TO(x) (((x+1)<<ADD_DELAY_OFF_NR_SHIFT))

#define ADD2_DELAY_ON_NR_SHIFT 18
#define ADD2_DELAY_ON_TO(x) (((x+1)<<ADD2_DELAY_ON_NR_SHIFT))

#define ADD2_DELAY_OFF_NR_SHIFT 22
#define ADD2_DELAY_OFF_TO(x) (((x+1)<<ADD2_DELAY_OFF_NR_SHIFT))

#define ADD_TO_ALL 14




#define PWR_STEPS 17

#define DISABLE_ZERO_CROSS    0

#define BOOST_ON_TIME 500
#define BOOST_ON_BEFORE_SWITCH 50    

#define forbiddenTransitionsSize 25

typedef struct{
    float pwr;
    uint32_t swRel;
    uint32_t L1RelState;
    uint32_t L2RelState;
    uint32_t L3RelState;
    uint32_t L1L2RelState;
    uint32_t L2L3RelState;
    uint32_t L3L1RelState;
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
    
    int get_max_pwr(void) { return this->max_pwr; }
    int get_current_switch_state(void) { return this->cPwrLevel; }
    float get_current_pwr(void) { return max_pwr*relCfg[this->cPwrLevel].pwr; }
        
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
    std::array<int, 8> rel_on_delay_us;
    std::array<int, 8> rel_off_delay_us;
  
private:
    
    uint32_t calculateTimerValue(uint8_t state, uint8_t phaseAngleIdx, uint8_t pin);
    void setupTimer(uint8_t pin, uint8_t state, uint32_t loadValue);

    
    pthc_state_t cState = SETUP;
    
    uint8_t cPwrLevel = 0;
    uint8_t pwrLevelReq = 0;
    uint32_t cRelCfg[6];
    uint32_t nRelCfg[6];
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
        { .from = 10, .to = 13, .stepBetween = 9 },   
        { .from = 10, .to = 15, .stepBetween = 9 }, 
        
        { .from = 6, .to = 10, .stepBetween = 0 },   
        { .from = 8, .to = 10, .stepBetween = 0 }, 
        { .from = 13, .to = 10, .stepBetween = 9 },   
        { .from = 15, .to = 10, .stepBetween = 9 }, 
        
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
        .L3RelState = SWR6|SWR3|ADD_DELAY_OFF_TO(6),
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
        .L3RelState = SWR3|ADD_DELAY_ON_TO(ADD_TO_ALL),
        .L1L2RelState = 0,
        .L2L3RelState = 0,
        .L3L1RelState = 0,
      },          
      
      //Pwr level 6 -> L2 - SP - L1 + N - SP
      //Achtung Sicherstellen das SWR1 vor SWR5 schaltet
      { .pwr = 0.1833333,
        .swRel = 0,
        .L1RelState = SWR5|ADD_DELAY_ON_TO(ADD_TO_ALL), //Zerocross on SP is equal to L1 Phase  
        .L2RelState = 0,
        .L3RelState = 0,
        .L1L2RelState = SWR1|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_OFF_TO(1),
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
        .L3RelState = SWR3|SWR5|ADD_DELAY_ON_TO(ADD_TO_ALL)|ADD2_DELAY_ON_TO(5),
        .L1L2RelState = SWR1|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_OFF_TO(ADD_TO_ALL),
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
        .L1RelState = SWR2|SWR5|ADD_DELAY_ON_TO(ADD_TO_ALL)|ADD2_DELAY_ON_TO(ADD_TO_ALL),
        .L2RelState = 0,
        .L3RelState = 0,
        .L1L2RelState = 0,
        .L2L3RelState = SWR4|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_OFF_TO(ADD_TO_ALL),
        .L3L1RelState = 0,
      },    
      //Pwr level 11 -> L2 - L3 + N - L1
      { .pwr = 0.4444,
        .swRel = 0,
        .L1RelState = SWR1|SWR6|ADD_DELAY_ON_TO(ADD_TO_ALL),
        .L2RelState = 0,
        .L3RelState = 0,
        .L1L2RelState = 0,
        .L2L3RelState = SWR4|ADD_DELAY_OFF_TO(ADD_TO_ALL),
        .L3L1RelState = 0,
      },     
      //Pwr level 12 -> L1 - L3 + L2 - - L1
      { .pwr = 0.5,
        .swRel = 0,
        .L1RelState = 0,
        .L2RelState = 0,
        .L3RelState = 0,
        .L1L2RelState = SWR1|ADD_DELAY_ON_TO(ADD_TO_ALL),
        .L2L3RelState = 0,
        .L3L1RelState = SWR2|SWR3|ADD_DELAY_OFF_TO(ADD_TO_ALL),
      },   
      //Pwr level 13 -> L2 - L3 + N - L1 + N - L3
      { .pwr = 0.5533333,
        .swRel = 0,
        .L1RelState = SWR1|SWR6|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD_DELAY_ON_TO(ADD_TO_ALL),
        .L2RelState = 0,
        .L3RelState = SWR3|SWR5|ADD_DELAY_ON_TO(ADD_TO_ALL)|ADD2_DELAY_ON_TO(ADD_TO_ALL),
        .L1L2RelState = 0,
        .L2L3RelState = SWR4|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_OFF_TO(ADD_TO_ALL),
        .L3L1RelState = 0,
      },      
      //Pwr level 14 -> L2 - L3 + L1 - L2
      { .pwr = 0.666666666,
        .swRel = 0,
        .L1RelState = 0,
        .L2RelState = 0,
        .L3RelState = 0,
        .L1L2RelState = SWR1|SWR0|ADD_DELAY_ON_TO(ADD_TO_ALL)|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_ON_TO(ADD_TO_ALL)|ADD2_DELAY_OFF_TO(ADD_TO_ALL),
        .L2L3RelState = SWR4,
        .L3L1RelState = 0,
      },    
      //Pwr level 15 -> L2 - L3 + L1 - L2 + N - L3
      { .pwr = 0.77773333,
        .swRel = 0,
        .L1RelState = 0,
        .L2RelState = 0,
        .L3RelState = SWR5|SWR3|ADD_DELAY_ON_TO(ADD_TO_ALL)|ADD2_DELAY_ON_TO(ADD_TO_ALL),
        .L1L2RelState = SWR1|SWR0|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_ON_TO(ADD_TO_ALL),
        .L2L3RelState = SWR4|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_OFF_TO(ADD_TO_ALL),
        .L3L1RelState = 0,
      },           
      
      //Pwr level 16 -> L2 - L3 + L1 - L2 + L1 - L3
      { .pwr = 1.0,
        .swRel = 0,
        .L1RelState = 0,
        .L2RelState = 0,
        .L3RelState = 0,
        .L1L2RelState = SWR1|SWR0|ADD_DELAY_ON_TO(ADD_TO_ALL)|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_ON_TO(ADD_TO_ALL)|ADD2_DELAY_OFF_TO(ADD_TO_ALL),
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
        .L3RelState = SWR6|SWR3|ADD_DELAY_OFF_TO(6)|ADD2_DELAY_OFF_TO(6),
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
        .L2RelState = SWR5|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_OFF_TO(ADD_TO_ALL),
        .L3RelState = SWR3|ADD_DELAY_ON_TO(ADD_TO_ALL)|ADD2_DELAY_ON_TO(ADD_TO_ALL),
        .L1L2RelState = 0,
        .L2L3RelState = 0,
        .L3L1RelState = 0,
      },          
      
      //Pwr level 6 -> L2 - SP - L1 + N - SP
      //Achtung Sicherstellen das SWR1 vor SWR5 schaltet
      { .pwr = 0.1833333,
        .swRel = 0,
        .L1RelState = SWR5|ADD_DELAY_ON_TO(ADD_TO_ALL)|ADD2_DELAY_ON_TO(ADD_TO_ALL), //Zerocross on SP is equal to L1 Phase  
        .L2RelState = 0,
        .L3RelState = 0,
        .L1L2RelState = SWR1|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_OFF_TO(ADD_TO_ALL),
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
        .L3RelState = SWR3|SWR5|ADD_DELAY_ON_TO(ADD_TO_ALL)|ADD_DELAY_OFF_TO(3)|ADD2_DELAY_ON_TO(5),
        .L1L2RelState = SWR1|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_OFF_TO(ADD_TO_ALL),
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
        .L1RelState = SWR2|SWR5|ADD_DELAY_ON_TO(ADD_TO_ALL)|ADD2_DELAY_ON_TO(ADD_TO_ALL),
        .L2RelState = 0,
        .L3RelState = 0,
        .L1L2RelState = 0,
        .L2L3RelState = SWR4|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_OFF_TO(ADD_TO_ALL),
        .L3L1RelState = 0,
      },    
      //Pwr level 11 -> L2 - L3 + N - L1
      { .pwr = 0.4444,
        .swRel = 0,
        .L1RelState = SWR1|SWR6|ADD_DELAY_ON_TO(ADD_TO_ALL),
        .L2RelState = 0,
        .L3RelState = 0,
        .L1L2RelState = 0,
        .L2L3RelState = SWR4|ADD_DELAY_OFF_TO(ADD_TO_ALL),
        .L3L1RelState = 0,
      },     
      //Pwr level 12 -> L1 - L3 + L2 - - L1
      { .pwr = 0.5,
        .swRel = 0,
        .L1RelState = 0,
        .L2RelState = 0,
        .L3RelState = 0,
        .L1L2RelState = SWR1|ADD_DELAY_ON_TO(ADD_TO_ALL),
        .L2L3RelState = 0,
        .L3L1RelState = SWR2|SWR3|ADD_DELAY_OFF_TO(ADD_TO_ALL),
      },   
      //Pwr level 13 -> L2 - L3 + N - L1 + N - L3
      { .pwr = 0.5533333,
        .swRel = 0,
        .L1RelState = SWR1|SWR6|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD_DELAY_ON_TO(ADD_TO_ALL),
        .L2RelState = 0,
        .L3RelState = SWR3|SWR5|ADD_DELAY_ON_TO(ADD_TO_ALL)|ADD2_DELAY_ON_TO(ADD_TO_ALL),
        .L1L2RelState = 0,
        .L2L3RelState = SWR4|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_OFF_TO(ADD_TO_ALL),
        .L3L1RelState = 0,
      },      
      //Pwr level 14 -> L2 - L3 + L1 - L2
      { .pwr = 0.666666666,
        .swRel = 0,
        .L1RelState = 0,
        .L2RelState = 0,
        .L3RelState = 0,
        .L1L2RelState = SWR1|SWR0|ADD_DELAY_ON_TO(ADD_TO_ALL)|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_ON_TO(ADD_TO_ALL)|ADD2_DELAY_OFF_TO(ADD_TO_ALL),
        .L2L3RelState = SWR4,
        .L3L1RelState = 0,
      },    
      //Pwr level 15 -> L2 - L3 + L1 - L2 + N - L3
      { .pwr = 0.77773333,
        .swRel = 0,
        .L1RelState = 0,
        .L2RelState = 0,
        .L3RelState = SWR5|SWR3|ADD_DELAY_ON_TO(ADD_TO_ALL)|ADD2_DELAY_ON_TO(ADD_TO_ALL),
        .L1L2RelState = SWR1|SWR0|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD_DELAY_ON_TO(ADD_TO_ALL),
        .L2L3RelState = SWR4|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_OFF_TO(ADD_TO_ALL),
        .L3L1RelState = 0,
      },           
      
      //Pwr level 16 -> L2 - L3 + L1 - L2 + L1 - L3
      { .pwr = 1.0,
        .swRel = 0,
        .L1RelState = 0,
        .L2RelState = 0,
        .L3RelState = 0,
        .L1L2RelState = SWR1|SWR0|ADD_DELAY_ON_TO(ADD_TO_ALL)|ADD_DELAY_OFF_TO(ADD_TO_ALL)|ADD2_DELAY_ON_TO(ADD_TO_ALL)|ADD2_DELAY_OFF_TO(ADD_TO_ALL),
        .L2L3RelState = SWR4,
        .L3L1RelState = SWR2|SWR3,
      },     
  };    

};


} //namespace pthc_float_output
} //namespace esphome
