# X-BLE Spam Pro - 增強版 BLE 廣播工具

基於 Flipper Zero 的多協議 BLE 廣播工具，支援觸發 Apple、Android (Google Fast Pair)、Samsung、Microsoft、小米/OPPO/Vivo (MDFE) 等設備的配對通知。

## 功能特性

### 支援的協議

| 協議 | 觸發設備 | 備註 |
|------|----------|------|
| **Apple Continuity** | iPhone, iPad, Mac, Apple Watch, AirPods | 支援 AirDrop、 proximity pair、notification 等 |
| **Google Fast Pair** | Pixel、三星、OPPO、小米、Sony、JBL 等 200+ 設備 | Model ID 洪水 |
| **Samsung EasySetup** | Galaxy Watch, Galaxy Buds, Galaxy 手機 | 需要 Samsung 生態 |
| **Microsoft SwiftPair** | Windows 10/11 電腦 | 藍牙通知 |
| **MDFE (互傳聯盟)** | 小米、OPPO、Vivo、Realme、一加 | 跨品牌文件傳輸 |
| **NameFlood** | 所有 BLE 設備 | 名稱洪水，幾乎所有設備 |

### 速度模式

- **Normal (~30ms)**: 標準間隔，穩定
- **Fast (~10ms)**: 較快觸發
- **Ultra (~3ms)**: 需要 ESP32 輔助

### 自動輪播模式

- 按順序自動切換不同協議和設備
- 可配置切換延遲 (預設 3 秒)
- 每個設備發送 N 個 packet 後切換

### ESP32 增強 (可選)

- 連接 ESP32 作為第二射頻
- 雙通道同時廣播，覆蓋範圍翻倍
- 需要 Ghost ESP 固件

## 安裝

### 前提條件

- Flipper Zero 設備
- qFlipper 或 SD 卡讀卡器
- (可選) 安裝了 Ghost ESP 固件的 ESP32-C3 或 ESP32-S3

### 安裝步驟

1. 下載最新的 `.fap` 文件
2. 將 `.fap` 文件複製到 Flipper Zero SD 卡的 `/apps/Bluetooth/` 目錄
3. 在 Flipper Zero 上進入 `Apps` -> `Bluetooth` -> `X-BLE Spam Pro`

### 從源碼構建

```bash
# 安裝 Flipper SDK (需要 ufbt)
git clone https://github.com/flipperdevices/flipperzero-firmware.git
cd flipperzero-firmware
./fbt launch_app APPSID=ble_spam_pro

# 或者使用 uFBT
pip install ufbt
ufbt launch APPID=ble_spam_pro
```

## 使用方法

### 基本操作

1. 選擇協議 (按 `Up`/`Down`)
2. 選擇速度模式 (按 `Left`/`Right`)
3. 按 `OK` 開始/停止攻擊
4. 按 `Back` 返回

### 協議說明

#### Apple Continuity
觸發 Apple 設備的配對請求和通知:
- AirDrop 彈窗
- Apple Watch 配對
- AirPods 靠近通知
- iOS 17 崩潰攻擊 (Continuity Custom)

#### Google Fast Pair
支援 200+ Fast Pair 設備，包括:
- Google Pixel Buds
- Samsung Galaxy Buds
- Sony WH-1000XM5, WF-1000XM5
- OPPO Enco Air3 Pro
- 小米 Buds 4 Pro
- JBL, Bose, Beats 等

#### Samsung EasySetup
- Galaxy Watch 系列
- Galaxy Buds 系列
- Galaxy S20-S24 系列

#### Microsoft SwiftPair
- Windows 10/11 藍牙設備通知

#### MDFE (互傳聯盟)
- 小米/Redmi/POCO 全系列
- OPPO/Realme/OnePlus 全系列
- Vivo/iQOO 全系列

#### NameFlood
- 發送各種設備名稱觸發所有 BLE 掃描設備

### 設置選項

- **Random MAC**: 隨機化 MAC 地址
- **Auto-Cycle**: 自動輪播所有設備
- **Cycle Delay**: 輪播切換延遲

## 文件結構

```
ble_spam_enhanced/
├── ble_spam.h              # 主頭文件
├── ble_spam_i.h            # 內部頭文件
├── main.c                  # 主程序
├── application.fam         # Flipper 應用清單
├── protocols/
│   ├── protocol_continuity.c   # Apple Continuity
│   ├── protocol_fastpair.c     # Google Fast Pair
│   ├── protocol_easysetup.c    # Samsung EasySetup
│   ├── protocol_swiftpair.c    # Microsoft SwiftPair
│   ├── protocol_mdfe.c         # MDFE (互傳聯盟)
│   └── protocol_nameflood.c    # NameFlood
└── README.md
```

## 技術細節

### BLE 廣播格式

#### Apple Continuity
```
[Len] [0xFF] [0x4C 0x00] [Type] [Data...]
```

#### Google Fast Pair
```
[0x02] [0x01] [0x06]          # Flags
[0x03] [0x03] [0x2C 0xFE]     # Fast Pair Service UUID
[0x05] [0x16] [0x2C 0xFE] [ModelID...]
```

#### MDFE
```
[Len] [0x09] [DeviceName...]  # 設備名稱
[Len] [0xFF] [0xFF 0xFF] [ProtocolVer] [DeviceType] [VendorID]
```

### 已知 Model ID

完整的 Model ID 列表請參考 `protocol_fastpair.c`

## 已知問題

1. **Vivo 設備**: 尚未找到公開的 Fast Pair Model ID，部分功能可能無法觸發
2. **MDFE 協議**: 由於是專有協議且未完全逆向，實現基於公開資料和合理猜測
3. **速度限制**: Flipper Zero 的 BLE 廣告間隔受限於硬體

## 免责声明

本工具僅用於教育目的和測試自己的設備。請遵守當地法律法規，不要對他人的設備進行攻擊。

## 參考來源

- [Google Fast Pair 規範](https://developers.google.com/nearby/fast-pair/specifications/introduction)
- [Apple Continuity 逆向工程](https://github.com/furiousMAC/continuity)
- [Microsoft Swift Pair 規範](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/bluetooth-swift-pair)
- [Flipper Zero BLE API](https://developer.flipper.net/flipperzero/doxygen/furi__hal__bt_8h.html)

## License

GPL-3.0

## 貢獻

歡迎提交 Issue 和 Pull Request！

## 更新日誌

### v2.0.0
- 新增 MDFE (互傳聯盟) 協議
- 新增 NameFlood 協議
- 擴展 Fast Pair 設備列表 (200+)
- 新增自動輪播模式
- 新增速度模式選項
- 重構代碼結構
