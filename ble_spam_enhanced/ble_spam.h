#ifndef BLE_SPAM_H
#define BLE_SPAM_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// X-BLE Spam Pro - 多協議 BLE 廣播攻擊工具 v2.1
// =============================================================================
// 支持協議:
//   - Apple Continuity (AirDrop, AirPods, Apple Watch)
//   - Google Fast Pair (200+ 設備: Pixel, Samsung, OPPO, Xiaomi, Sony, JBL...)
//   - Samsung EasySetup (Galaxy Watch, Galaxy Buds)
//   - Microsoft SwiftPair (Windows 通知)
//   - MDFE / 互傳聯盟 (小米/OPPO/Vivo 跨品牌傳輸)
//   - NameFlood (設備名稱洪水觸發所有 BLE 設備)
//
// 新增功能:
//   - 擴展到 200+ 設備品牌型號
//   - 自動輪播模式，可配置切換延遲和每設備發包數
//   - 速度優化: Normal / Fast / Ultra 三檔
//   - TX 穩定性增強，自動檢測並恢復
//   - 隨機 MAC 地址選項
// =============================================================================

#define APP_NAME "X-BLE Spam Pro"
#define APP_VERSION "2.1"

// =============================================================================
// 協議定義
// =============================================================================
typedef enum {
    Protocol_AppleContinuity = 0,
    Protocol_GoogleFastPair,
    Protocol_SamsungEasysetup,
    Protocol_MicrosoftSwiftPair,
    Protocol_MDFE,
    Protocol_NameFlood,
    Protocol_COUNT
} ProtocolType;

// =============================================================================
// 攻擊模式
// =============================================================================
typedef enum {
    AttackMode_Single = 0,  // 單一設備
    AttackMode_Random,      // 隨機設備
    AttackMode_Cycle        // 輪播所有設備
} AttackMode;

// =============================================================================
// 速度模式
// =============================================================================
typedef enum {
    SpeedMode_Normal = 0,
    SpeedMode_Fast,
    SpeedMode_Ultra
} SpeedMode;

// =============================================================================
// Apple Continuity 類型
// =============================================================================
typedef enum {
    Continuity_AirDrop = 0x05,
    Continuity_ProximityPair = 0x07,
    Continuity_AirplayTarget = 0x09,
    Continuity_Handoff = 0x0C,
    Continuity_TetheringSource = 0x0E,
    Continuity_NearbyAction = 0x0F,
    Continuity_NearbyInfo = 0x10,
    Continuity_CustomCrash = 0xFF
} ContinuityType;

// =============================================================================
// Fast Pair Model 定義
// =============================================================================
typedef struct {
    uint32_t model_id;
    const char* name;
    const char* brand;
} FastPairModel;

// =============================================================================
// 協議驅動結構
// =============================================================================
typedef struct ProtocolDriver {
    const char* name;
    const char* short_name;
    bool is_active;
    uint8_t (*build_packet)(uint8_t* _size, uint8_t** _packet, struct AttackConfig* config);
    const char* (*get_device_name)(uint32_t index);
    uint32_t (*get_device_count)(void);
    void (*init)(void);
    void (*deinit)(void);
} ProtocolDriver;

// =============================================================================
// 特定協議配置
// =============================================================================

typedef struct {
    uint32_t model_id;
} FastPairCfg;

typedef struct {
    ContinuityType type;
    uint16_t model;
    uint8_t color;
    uint8_t prefix;
} ContinuityCfg;

typedef struct {
    uint8_t type;
    uint32_t model;
} EasysetupCfg;

typedef struct {
    char name[32];
} SwiftpairCfg;

typedef struct {
    bool enabled;
    uint8_t device_type;
    uint8_t protocol_ver;
    uint16_t vendor_id;
} MDFECfg;

typedef struct {
    char name[32];
} NamefloodCfg;

// =============================================================================
// 攻擊配置 - 包含所有協議的聯合配置
// =============================================================================
typedef struct AttackConfig {
    AttackMode mode;
    SpeedMode speed;
    bool random_mac;
    uint16_t interval_ms;
    uint8_t count_per_device;
    uint8_t cycle_delay_s;
    uint8_t target_device_idx;  // 自動輪播時的目標設備索引
    
    union {
        FastPairCfg fastpair;
        ContinuityCfg continuity;
        EasysetupCfg easysetup;
        SwiftpairCfg swiftpair;
        MDFECfg mdfe;
        NamefloodCfg nameflood;
    } config;
} AttackConfig;

#endif // BLE_SPAM_H