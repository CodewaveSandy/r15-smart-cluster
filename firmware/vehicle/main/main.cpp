#include "shared/protocol/GearPacket.h"
#include "driver/gpio.h"
#include "InputConfig.h"
#include "shared/protocol/Button.h"
#include "shared/protocol/DiscreteInputs.h"
#include "driver/uart_vfs.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <cstdio>

extern "C" void app_main() {
    gpio_config_t inputs{};
    const auto add=[&](wiring::Input input){if(input.pin>=0)inputs.pin_bit_mask|=1ULL<<input.pin;};
    add(wiring::tripButton);add(wiring::infoButton);
    add(wiring::left);add(wiring::right);add(wiring::highBeam);
    for(auto input:wiring::gears)add(input);
    inputs.mode=GPIO_MODE_INPUT;
    // Bench active-HIGH inputs: weak pull-downs keep unwired channels inactive.
    // Active-LOW overrides below use pull-ups instead. Neither detects wire faults.
    inputs.pull_up_en=GPIO_PULLUP_DISABLE;inputs.pull_down_en=GPIO_PULLDOWN_ENABLE;
    inputs.intr_type=GPIO_INTR_DISABLE;
    if(inputs.pin_bit_mask)ESP_ERROR_CHECK(gpio_config(&inputs));
    const auto bias=[](wiring::Input input) {
        if(input.pin>=0)ESP_ERROR_CHECK(gpio_set_pull_mode(gpio_num_t(input.pin),
            input.activeHigh?GPIO_PULLDOWN_ONLY:GPIO_PULLUP_ONLY));
    };
    bias(wiring::tripButton);bias(wiring::infoButton);
    bias(wiring::left);bias(wiring::right);bias(wiring::highBeam);
    for(auto input:wiring::gears)bias(input);
    setvbuf(stdout,nullptr,_IONBF,0);
    // Binary packet bytes must never receive stdout's default LF -> CRLF conversion.
    uart_vfs_dev_port_set_tx_line_endings(0,ESP_LINE_ENDINGS_LF);
    cluster::gearlink::Packet packet;packet.session=esp_random();
    cluster::inputs::Button tripButton,infoButton;
    for(;;) {
        const auto read=[](wiring::Input input)->cluster::Signal<bool> {
            if(input.pin<0)return {};
            return {bool(gpio_get_level(gpio_num_t(input.pin)))==input.activeHigh,cluster::Quality::Available};
        };
        packet.left=read(wiring::left);packet.right=read(wiring::right);packet.highBeam=read(wiring::highBeam);
        std::array<bool,7> asserted{};
        for(unsigned i=0;i<asserted.size();++i)asserted[i]=read(wiring::gears[i]).value;
        packet.gear=cluster::inputs::gear(asserted);
        packet.levels=std::uint8_t((wiring::left.pin>=0?gpio_get_level(gpio_num_t(wiring::left.pin)):0)|
            ((wiring::right.pin>=0?gpio_get_level(gpio_num_t(wiring::right.pin)):0)<<1));
        packet.uptimeMs=std::uint32_t(esp_timer_get_time()/1000);
        if(tripButton.update(read(wiring::tripButton).value,packet.uptimeMs))++packet.tripPresses;
        if(infoButton.update(read(wiring::infoButton).value,packet.uptimeMs))++packet.infoPresses;
        const auto bytes=cluster::gearlink::encode(packet);
        // 36 bytes per 20 ms: bounded output well below the UART FIFO capacity.
        fwrite(bytes.data(),1,bytes.size(),stdout);
        ++packet.sequence;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
