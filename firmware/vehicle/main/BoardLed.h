#pragma once
#include "InputConfig.h"
#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_log.h"
// Board facts must be supplied from the verified schematic/pinout. Memory SKU
// N8R8 is not an LED profile. 0 unknown, 1 hardwired power, 2 GPIO, 3 WS2812.
#ifndef R15_VEHICLE_LED_KIND
#define R15_VEHICLE_LED_KIND 0
#endif
#ifndef R15_VEHICLE_LED_GPIO
#define R15_VEHICLE_LED_GPIO -1
#endif
#ifndef R15_VEHICLE_LED_ACTIVE_HIGH
#define R15_VEHICLE_LED_ACTIVE_HIGH 1
#endif
namespace cluster::board {
constexpr bool ledPinFree(int pin){
    if(pin<0)return false;
    if(pin==wiring::displayTx||pin==wiring::displayRx||pin==19||pin==20||pin==43||pin==44||(pin>=26&&pin<=37)||pin==0||pin==3||pin==45||pin==46)return false;
    const wiring::Input inputs[]={wiring::tripButton,wiring::infoButton,wiring::left,wiring::right,wiring::highBeam,wiring::gears[0],wiring::gears[1],wiring::gears[2],wiring::gears[3],wiring::gears[4],wiring::gears[5],wiring::gears[6]};
    for(const auto input:inputs)if(input.pin==pin)return false;
    return GPIO_IS_VALID_OUTPUT_GPIO(pin);
}
static_assert(R15_VEHICLE_LED_KIND>=0&&R15_VEHICLE_LED_KIND<=3,"Unsupported LED profile");
static_assert(R15_VEHICLE_LED_ACTIVE_HIGH==0||R15_VEHICLE_LED_ACTIVE_HIGH==1,"Invalid GPIO LED active level");
static_assert(R15_VEHICLE_LED_KIND<2||ledPinFree(R15_VEHICLE_LED_GPIO),"LED profile conflicts with reserved/owned/unsupported GPIO");
inline bool disableBoardLed(){
    constexpr int kind=R15_VEHICLE_LED_KIND,pin=R15_VEHICLE_LED_GPIO;
    if(kind==0){ESP_LOGW("board-led","LED mapping unverified: no GPIO driven");return false;}
    if(kind==1){ESP_LOGW("board-led","Hardwired power LED cannot be disabled in firmware");return false;}
    if(!ledPinFree(pin))return false;
    if(kind==2){
        if(gpio_reset_pin(gpio_num_t(pin))!=ESP_OK)return false;
        if(gpio_set_level(gpio_num_t(pin),R15_VEHICLE_LED_ACTIVE_HIGH?0:1)!=ESP_OK)return false;
        const bool ok=gpio_set_direction(gpio_num_t(pin),GPIO_MODE_OUTPUT)==ESP_OK;
        if(ok)ESP_LOGI("board-led","GPIO LED off");
        return ok;
    }
    // Zero is off regardless of GRB/RGB byte order. Hardware RMT emits the
    // verified WS2812 profile (10 MHz: 0.3 us high/0.9 us low, 600 us reset).
    rmt_channel_handle_t channel=nullptr;rmt_encoder_handle_t encoder=nullptr;
    rmt_tx_channel_config_t config{};config.gpio_num=gpio_num_t(pin);config.clk_src=RMT_CLK_SRC_DEFAULT;
    config.resolution_hz=10000000;config.mem_block_symbols=48;config.trans_queue_depth=1;
    bool ok=rmt_new_tx_channel(&config,&channel)==ESP_OK;
    rmt_copy_encoder_config_t copy{};if(ok)ok=rmt_new_copy_encoder(&copy,&encoder)==ESP_OK;
    if(ok)ok=rmt_enable(channel)==ESP_OK;
    rmt_symbol_word_t symbols[25]{};
    for(unsigned i=0;i<24;++i){symbols[i].level0=1;symbols[i].duration0=3;symbols[i].level1=0;symbols[i].duration1=9;}
    symbols[24].level0=symbols[24].level1=0;symbols[24].duration0=symbols[24].duration1=3000;
    rmt_transmit_config_t tx{};
    if(ok)ok=rmt_transmit(channel,encoder,symbols,sizeof symbols,&tx)==ESP_OK&&rmt_tx_wait_all_done(channel,100)==ESP_OK;
    if(channel){rmt_disable(channel);rmt_del_channel(channel);}if(encoder)rmt_del_encoder(encoder);
    const bool held=gpio_reset_pin(gpio_num_t(pin))==ESP_OK&&gpio_set_level(gpio_num_t(pin),0)==ESP_OK&&gpio_set_direction(gpio_num_t(pin),GPIO_MODE_OUTPUT)==ESP_OK;
    if(ok&&held)ESP_LOGI("board-led","Addressable LED latched off");else ESP_LOGW("board-led","LED-off driver failed");
    return ok&&held;
}
}
