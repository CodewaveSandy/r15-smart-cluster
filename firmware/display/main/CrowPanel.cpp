#include "CrowPanel.h"
#include <algorithm>
#include <cstring>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

namespace cluster::board {
namespace {
constexpr char tag[]="crowpanel-v3";
constexpr int width=800, height=480;
constexpr gpio_num_t backlightPin=GPIO_NUM_2;
esp_lcd_panel_handle_t panel=nullptr;
i2c_master_dev_handle_t expander=nullptr, touch=nullptr;
StaticSemaphore_t syncStorage;
SemaphoreHandle_t sync=nullptr;
unsigned flushes=0, longestFlush=0;
portMUX_TYPE frameMux=portMUX_INITIALIZER_UNLOCKED;
void* frameBuffers[2]{};

bool frameComplete(esp_lcd_panel_handle_t, const esp_lcd_rgb_panel_event_data_t*, void*) {
    BaseType_t wake=pdFALSE;
    xSemaphoreGiveFromISR(sync,&wake);
    return wake==pdTRUE;
}
void expanderWrite(uint8_t reg,uint8_t value) {
    const uint8_t bytes[]={reg,value};
    ESP_ERROR_CHECK(i2c_master_transmit(expander,bytes,sizeof bytes,25));
}
uint8_t expanderRead(uint8_t reg) {
    uint8_t value=0;
    ESP_ERROR_CHECK(i2c_master_transmit_receive(expander,&reg,1,&value,1,25));
    return value;
}
void expanderBit(uint8_t reg,unsigned bit,bool high) {
    auto value=expanderRead(reg);
    expanderWrite(reg, high?value|(1U<<bit):value&~(1U<<bit));
}
bool touchRead(uint16_t reg,uint8_t* data,size_t length) {
    const uint8_t address[]={uint8_t(reg>>8),uint8_t(reg)};
    return i2c_master_transmit_receive(touch,address,2,data,length,10)==ESP_OK;
}
void flush(lv_display_t* display,const lv_area_t*,uint8_t* pixels) {
    // DIRECT mode can flush several dirty rectangles into the same off-screen
    // framebuffer. Present only once all those rectangles have been drawn.
    if(!lv_display_flush_is_last(display)) { lv_display_flush_ready(display); return; }
    if(pixels!=frameBuffers[0] && pixels!=frameBuffers[1]) abort();
    const auto started=esp_timer_get_time();
    // app_main is pinned to CPU0 and allocates the RGB DMA interrupt there.
    // With interrupts disabled on that core, discard the old completion and
    // select the new driver-owned framebuffer atomically relative to EOF.
    // In pinned IDF 5.5.4 with bounce buffers this API path only selects an
    // existing buffer; it does no copy, allocation, cache sync or blocking I/O.
    // No on_color_trans_done callback is registered.
    portENTER_CRITICAL(&frameMux);
    xSemaphoreTake(sync,0);
    const auto result=esp_lcd_panel_draw_bitmap(panel,0,0,width,height,pixels);
    portEXIT_CRITICAL(&frameMux);
    ESP_ERROR_CHECK(result);
    // IDF's bounce EOF handler first finishes copying the old full frame,
    // then latches cur_fb_index for the next frame, then invokes this event.
    // It is now safe for LVGL DIRECT mode to synchronize/reuse the old buffer.
    if(xSemaphoreTake(sync,pdMS_TO_TICKS(100))!=pdTRUE) {
        ESP_LOGE(tag,"RGB frame-complete timeout; refusing to reuse scanout memory");
        backlight(false);
        abort();
    }
    ++flushes;
    longestFlush=std::max(longestFlush,unsigned(esp_timer_get_time()-started));
    lv_display_flush_ready(display);
}
uint32_t tick() { return uint32_t(esp_timer_get_time()/1000); }
}

void backlight(bool enabled) { ESP_ERROR_CHECK(gpio_set_level(backlightPin,enabled)); }
void initialize() {
    static_assert(CONFIG_ESP_MAIN_TASK_AFFINITY==0,"RGB EOF handoff requires app_main pinned to CPU0");
    if(xPortGetCoreID()!=0) abort();
    gpio_config_t light{};
    light.pin_bit_mask=1ULL<<backlightPin; light.mode=GPIO_MODE_OUTPUT;
    ESP_ERROR_CHECK(gpio_config(&light)); backlight(false);

    // Provenance: Elecrow example/V3.0/PlatformIO70 main.cpp, touch.h,
    // patches/LovyanGFX/esp32s3/Bus_RGB.cpp; user flashed this exact baseline.
    i2c_master_bus_config_t bus{};
    bus.i2c_port=I2C_NUM_0; bus.sda_io_num=GPIO_NUM_19; bus.scl_io_num=GPIO_NUM_20;
    bus.clk_source=I2C_CLK_SRC_DEFAULT; bus.glitch_ignore_cnt=7;
    bus.flags.enable_internal_pullup=true;
    i2c_master_bus_handle_t i2c=nullptr;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus,&i2c));
    i2c_device_config_t device{};
    device.dev_addr_length=I2C_ADDR_BIT_LEN_7; device.scl_speed_hz=400000;
    device.device_address=0x18;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c,&device,&expander));
    device.device_address=0x5d;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c,&device,&touch));

    sync=xSemaphoreCreateBinaryStatic(&syncStorage);
    configASSERT(sync);
    esp_lcd_rgb_panel_config_t cfg{};
    cfg.clk_src=LCD_CLK_SRC_PLL160M;
    cfg.timings.pclk_hz=24000000; cfg.timings.h_res=width; cfg.timings.v_res=height;
    cfg.timings.hsync_pulse_width=48; cfg.timings.hsync_back_porch=40; cfg.timings.hsync_front_porch=40;
    cfg.timings.vsync_pulse_width=31; cfg.timings.vsync_back_porch=13; cfg.timings.vsync_front_porch=1;
    cfg.timings.flags.pclk_active_neg=true;
    cfg.data_width=16; cfg.bits_per_pixel=16; cfg.num_fbs=2;
    cfg.bounce_buffer_size_px=width*10; cfg.dma_burst_size=64;
    cfg.hsync_gpio_num=39; cfg.vsync_gpio_num=40; cfg.de_gpio_num=41;
    cfg.pclk_gpio_num=0; cfg.disp_gpio_num=-1;
    const int pins[]={15,7,6,5,4,9,46,3,8,16,1,14,21,47,48,45};
    std::copy(pins,pins+16,cfg.data_gpio_nums);
    cfg.flags.fb_in_psram=true;
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&cfg,&panel));
    esp_lcd_rgb_panel_event_callbacks_t callbacks{};
    callbacks.on_frame_buf_complete=frameComplete;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel,&callbacks,nullptr));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));

    // V3.0 PCA9557 reset timing, preserving unrelated expander output bits.
    expanderWrite(3,0xff);
    expanderBit(3,0,false); expanderBit(3,1,false);
    expanderBit(1,0,false); expanderBit(1,1,false);
    vTaskDelay(pdMS_TO_TICKS(20));
    expanderBit(1,0,true); vTaskDelay(pdMS_TO_TICKS(100));
    expanderBit(3,1,true);

    void* first=nullptr; void* second=nullptr;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel,2,&first,&second));
    frameBuffers[0]=first; frameBuffers[1]=second;
    ESP_LOGI(tag,"RGB buffers %p %p; 24MHz, RGB565, 2x768000 bytes PSRAM",first,second);
    if(!first||!second) abort();
    std::memset(first,0,width*height*2); std::memset(second,0,width*height*2);
    lv_init(); lv_tick_set_cb(tick);
    auto* display=lv_display_create(width,height);
    if(!display) abort();
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display,flush);
    lv_display_set_buffers(display,first,second,width*height*2,LV_DISPLAY_RENDER_MODE_DIRECT);
}

bool releasedTouch(int& x,int& y) {
    static int64_t sampled=0;
    static bool pressed=false;
    static unsigned failures=0;
    const auto now=esp_timer_get_time();
    if(now-sampled<30000) return false;
    sampled=now;
    uint8_t status=0;
    if(!touchRead(0x814e,&status,1)) {
        pressed=false;
        if(++failures==1 || failures%100==0) ESP_LOGW(tag,"GT911 read failed (%u)",failures);
        return false;
    }
    failures=0;
    if(!(status&0x80)) return false;
    const unsigned count=status&15;
    bool tapped=false;
    if(count==1) {
        uint8_t point[7]{};
        if(touchRead(0x814f,point,sizeof point)) {
            const int tx=point[1]|(point[2]<<8), ty=point[3]|(point[4]<<8);
            if(tx<width && ty<height) { x=tx; y=ty; pressed=true; }
            else pressed=false;
        } else pressed=false;
    } else { tapped=pressed&&count==0; pressed=false; }
    const uint8_t clear[]={0x81,0x4e,0};
    if(i2c_master_transmit(touch,clear,sizeof clear,10)!=ESP_OK) { pressed=false; return false; }
    return tapped;
}
unsigned flushCount() { return flushes; }
unsigned maxFlushUs() { return longestFlush; }
}
