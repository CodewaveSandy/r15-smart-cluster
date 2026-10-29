#pragma once
#include "embedded/application/Preferences.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

namespace cluster::board {
// One bounded storage worker; it never touches application/LVGL objects.
// NVS errors never trigger automatic erase of existing settings.
class PreferenceStore {
public:
    void initialize(Preferences& confirmed) {
        ready_=nvs_flash_init()==ESP_OK;
        if(!ready_) return;
        nvs_handle_t handle=0;
        if(nvs_open("display",NVS_READONLY,&handle)==ESP_OK) {
            PreferenceBytes bytes{};size_t size=bytes.size();
            if(nvs_get_blob(handle,"appearance",bytes.data(),&size)==ESP_OK)
                decodePreferences(bytes.data(),size,confirmed);
            nvs_close(handle);
        }
        requests_=xQueueCreateStatic(1,sizeof(PreferenceBytes),requestBytes_,&requestQueue_);
        results_=xQueueCreateStatic(1,sizeof(bool),resultBytes_,&resultQueue_);
        worker_=xTaskCreateStatic(run,"display-nvs",4096,this,1,stack_,&task_);
        ready_=requests_&&results_&&worker_;
    }
    bool request(const Preferences& value) {
        const auto bytes=encodePreferences(value);Preferences validated;
        if(!decodePreferences(bytes,validated))return false;
        return ready_&&xQueueSend(requests_,&bytes,0)==pdTRUE;
    }
    bool result(bool& success) {return results_&&xQueueReceive(results_,&success,0)==pdTRUE;}
private:
    static void run(void* context) {
        auto& self=*static_cast<PreferenceStore*>(context);
        PreferenceBytes bytes;
        for(;;) if(xQueueReceive(self.requests_,&bytes,portMAX_DELAY)==pdTRUE) {
            nvs_handle_t handle=0;
            bool saved=nvs_open("display",NVS_READWRITE,&handle)==ESP_OK;
            if(saved) {
                saved=nvs_set_blob(handle,"appearance",bytes.data(),bytes.size())==ESP_OK&&nvs_commit(handle)==ESP_OK;
                nvs_close(handle);
            }
            xQueueOverwrite(self.results_,&saved);
        }
    }
    bool ready_=false;
    StaticQueue_t requestQueue_{},resultQueue_{};
    uint8_t requestBytes_[sizeof(PreferenceBytes)]{},resultBytes_[sizeof(bool)]{};
    QueueHandle_t requests_=nullptr,results_=nullptr;
    StaticTask_t task_{};StackType_t stack_[4096]{};TaskHandle_t worker_=nullptr;
};
}
