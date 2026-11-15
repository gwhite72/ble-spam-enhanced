#ifndef BLE_SPAM_INTERNAL_H
#define BLE_SPAM_INTERNAL_H

#include "ble_spam.h"

// =============================================================================
// 內部頭文件 - 定義協議驅動結構和內部函數
// =============================================================================

// =============================================================================
// 攻擊配置 - 特定協議的聯合結構
// =============================================================================

// Fast Pair 配置
typedef struct {
    uint32_t model_id;
} FastPairCfg;

// Continuity 配置
typedef struct {
    ContinuityType type;
    uint16_t model;
    uint8_t color;
    uint8_t prefix;
} ContinuityCfg;

// Easysetup 配置
typedef struct {
    uint8_t type;      // 0=Buds, 1=Watch
    uint32_t model;
} EasysetupCfg;

// SwiftPair 配置
typedef struct {
    char name[25];
} SwiftpairCfg;

// MDFE 配置
typedef struct {
    bool enabled;
    uint8_t device_type;
    uint8_t protocol_ver;
    uint16_t vendor_id;
} MDFECfg;

// NameFlood 配置
typedef struct {
    char name[20];
} NamefloodCfg;

// =============================================================================
// 協議驅動 - 內部結構
// =============================================================================
typedef struct {
    const char* name;
    const char* short_name;
    bool is_active;
    uint8_t (*build_packet)(uint8_t* _size, uint8_t** _packet, AttackConfig* config);
    const char* (*get_device_name)(uint32_t index);
    uint32_t (*get_device_count)(void);
    void (*init)(void);
    void (*deinit)(void);
} ProtocolDriver;

// =============================================================================
// 攻擊模式枚舉
// =============================================================================
typedef enum {
    ConfigExtraStart = 100,
} ConfigExtra;

// =============================================================================
// 內部函數聲明
// =============================================================================

// 協議初始化
void protocol_init_all(void);
void protocol_deinit_all(void);

// 獲取隨機設備索引
uint32_t protocol_get_random_device_index(const ProtocolDriver* driver);

// 設置攻擊配置
void attack_config_set_protocol(AttackConfig* config, ProtocolType type);
void attack_config_set_mode(AttackConfig* config, AttackMode mode);
void attack_config_set_random_mac(AttackConfig* config, bool enable);

// 自動循環模式
void auto_cycle_init(uint16_t delay_s, uint8_t count_per_device);
void auto_cycle_step(void);
void auto_cycle_stop(void);

// ESP32 增強
bool esp32_check_connection(void);
void esp32_send_packet(const uint8_t* packet, uint8_t size);
bool esp32_set_mode(uint8_t mode);

// 工具函數
void random填充_buf(uint8_t* buf, uint16_t len);
uint16_t crc16_calc(const uint8_t* data, uint16_t len);

// =============================================================================
// 調試功能
// =============================================================================
#ifdef DEBUG
#define PROTOCOL_DEBUG 1
#else
#define PROTOCOL_DEBUG 0
#endif

#if PROTOCOL_DEBUG
#define PROTOCOL_LOG(fmt, ...) \
    do { FURI_LOG_D("BLE_SPAM", fmt, ##__VA_ARGS__); } while(0)
#else
#define PROTOCOL_LOG(fmt, ...) do {} while(0)
#endif

#endif // BLE_SPAM_INTERNAL_H