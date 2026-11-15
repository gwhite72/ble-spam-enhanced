#include "ble_spam.h"
#include "ble_spam_i.h"
#include <furi.h>

// =============================================================================
// 自動輪播模式實現
// =============================================================================
// 按順序自動切換不同協議和設備，實現最大化的設備觸發覆蓋
// =============================================================================

// =============================================================================
// 輪播狀態
// =============================================================================
typedef struct {
    bool active;
    uint8_t current_protocol;
    uint8_t current_device_in_protocol;
    uint16_t packets_sent;
    uint16_t cycle_delay_s;
    uint8_t count_per_device;
    uint32_t cycle_count;
} AutoCycleState;

static AutoCycleState g_auto_cycle = {
    .active = false,
    .current_protocol = 0,
    .current_device_in_protocol = 0,
    .packets_sent = 0,
    .cycle_delay_s = 3,
    .count_per_device = 5,
    .cycle_count = 0,
};

// =============================================================================
// 獲取每個協議的設備數量
// =============================================================================
static const uint8_t g_protocol_device_counts[] = {
    19,  // Continuity: 19 種設備
    80,  // FastPair: ~80 種設備
    22,  // Easysetup: 22 種設備
    17,  // SwiftPair: 17 種設備
    25,  // MDFE: 25 種設備
    100, // NameFlood: 100+ 種設備
};

// =============================================================================
// 協議名稱
// =============================================================================
static const char* g_protocol_names[] = {
    "Apple Continuity",
    "Google Fast Pair",
    "Samsung EasySetup",
    "Microsoft SwiftPair",
    "MDFE (互傳聯盟)",
    "NameFlood",
};

// =============================================================================
// 初始化自動輪播
// =============================================================================
void auto_cycle_init(uint16_t delay_s, uint8_t count_per_device) {
    g_auto_cycle.active = true;
    g_auto_cycle.current_protocol = 0;
    g_auto_cycle.current_device_in_protocol = 0;
    g_auto_cycle.packets_sent = 0;
    g_auto_cycle.cycle_delay_s = delay_s;
    g_auto_cycle.count_per_device = count_per_device;
    g_auto_cycle.cycle_count = 0;
    
    FURI_LOG_I("AutoCycle", "Auto-cycle mode initialized: %ds delay, %d packets/device", 
               delay_s, count_per_device);
}

// =============================================================================
// 停止自動輪播
// =============================================================================
void auto_cycle_stop(void) {
    g_auto_cycle.active = false;
    FURI_LOG_I("AutoCycle", "Auto-cycle mode stopped after %lu cycles", g_auto_cycle.cycle_count);
}

// =============================================================================
// 執行自動輪播的一步
// =============================================================================
void auto_cycle_step(void) {
    if(!g_auto_cycle.active) return;
    
    // 檢查是否需要切換到下一個設備
    if(g_auto_cycle.packets_sent >= g_auto_cycle.count_per_device) {
        // 切換到下一個設備
        g_auto_cycle.current_device_in_protocol++;
        g_auto_cycle.packets_sent = 0;
        
        // 檢查是否需要切換到下一個協議
        uint8_t current_count = g_protocol_device_counts[g_auto_cycle.current_protocol];
        if(g_auto_cycle.current_device_in_protocol >= current_count) {
            // 切換到下一個協議
            g_auto_cycle.current_protocol++;
            g_auto_cycle.current_device_in_protocol = 0;
            
            // 檢查是否完成一輪
            if(g_auto_cycle.current_protocol >= Protocol_COUNT) {
                g_auto_cycle.current_protocol = 0;
                g_auto_cycle.cycle_count++;
                FURI_LOG_I("AutoCycle", "Cycle %lu completed", g_auto_cycle.cycle_count);
            }
        }
        
        FURI_LOG_D("AutoCycle", "Protocol: %s, Device: %d", 
                   g_protocol_names[g_auto_cycle.current_protocol],
                   g_auto_cycle.current_device_in_protocol);
    }
    
    g_auto_cycle.packets_sent++;
}

// =============================================================================
// 獲取當前輪播狀態
// =============================================================================
const char* auto_cycle_get_current_protocol_name(void) {
    return g_protocol_names[g_auto_cycle.current_protocol];
}

uint8_t auto_cycle_get_current_device_index(void) {
    return g_auto_cycle.current_device_in_protocol;
}

uint32_t auto_cycle_get_cycle_count(void) {
    return g_auto_cycle.cycle_count;
}

bool auto_cycle_is_active(void) {
    return g_auto_cycle.active;
}

// =============================================================================
// 設置輪播參數
// =============================================================================
void auto_cycle_set_delay(uint16_t delay_s) {
    g_auto_cycle.cycle_delay_s = delay_s;
}

void auto_cycle_set_count_per_device(uint8_t count) {
    g_auto_cycle.count_per_device = count;
}