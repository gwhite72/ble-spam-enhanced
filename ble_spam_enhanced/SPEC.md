# 增強版 ble_spam - 技術規格

## 目標
- 支援 Flipper Zero 原生藍牙 + 選配 ESP32 雙射頻
- 觸發 Apple / Google Fast Pair / Samsung / Microsoft SwiftPair / MDFE (小米/OPPO/Vivo) / Vivo 獨立

## 協議矩陣

| 協議 | UUID/格式 | 觸發方式 |
|------|-----------|----------|
| Apple Continuity | 0xFF (Company 0x004C) | 各種 continuity type |
| Google Fast Pair | Service 0xFE2C | 24-bit model ID |
| Samsung EasySetup | Service UUID ? | Buds/Watch model |
| Microsoft SwiftPair | Service 0x1812 | 藍牙通知 |
| MDFE (互傳聯盟) | 待研究 | 小米/OPPO 共用 |
| Vivo 獨立 | 待研究 | 需要逆向 |

## Auto-Cycle (輪播模式)
- [ALL] 模式按順序輪流發送各協議
- 每個設備類型廣播 N 個 packet 後切換
- 可配置間隔 (預設 3-5 秒)

## 速度優化
- 標準模式: ~30ms/packet (BLE 廣告間隔限制)
- 快速模式: <10ms/acket (需要短廣告間隔)
- 雙射頻模式: ESP32 + 內置藍牙同時發送

## 已知 Model ID (Fast Pair)
### 手機/平板:
- 0x00000C: Google Gphones Transfer
- 0x0577B1: Galaxy S23 Ultra
- 0x05A9BC: Galaxy S20+
- 0x06AE20: Galaxy S21 5G
- 0x8D16EA: Galaxy M14 5G
- 0x8E1996: Galaxy A24 5g
- 0x915CFA: Galaxy A14
- 0xA8CAAD: Galaxy F04
- 0x9D7D42: Galaxy S20
- 0x9D7EA: Galaxy S20 5G
- 0x99F098: Galaxy S22 Ultra

### 耳機/音頻:
- 0x06C197: OPPO Enco Air3 Pro ✅
- 0xDD4EC0: OPPO Enco Air3 Pro (另一ID)
- 0xDEEA86: Xiaomi Buds 4 Pro ✅
- 0xE6E8B8: POCO Pods ✅
- 0xC8C641: Redmi Buds 4 Lite ✅
- 0xD90617: Redmi Buds 4 Active ✅
- 0x21A04E: Oraimo FreePods Pro ✅
- 0x6B8C65: Oraimo FreePods 4 ✅
- 0x8C6B6A: realme Buds Air 3S ✅
- 0x8CD10F: realme Buds Air Pro ✅
- 0xE6E37E: realme Buds Air 5 Pro ✅

### Vivo (缺少):
- 需要新增 - 尚未找到公開的 model ID

## MDFE 協議參考
- 基於 BLE廣播發現 + Wi-Fi Direct 傳輸
- 設備發現使用標準 BLE 廣播
- Service UUID: 待確認（社區尚未完全逆向）
- 可參考: https://github.com/nicokosi/Android-SDK/blob/master /...

## Flipper BLE API
- furi_hal_bt_extra_beacon_set_config()
- furi_hal_bt_extra_beacon_start()
- furi_hal_bt_extra_beacon_stop()
- furi_hal_bt_tx() / furi_hal_bt_start_scan()