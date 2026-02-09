/*
 * ============================================================
 *  I2C 버스 스캐너 (I2C Bus Scanner)
 * ============================================================
 *
 *  기능 설명:
 *  - I2C 버스에 연결된 모든 장치의 주소를 검색합니다
 *  - 0x01 ~ 0x7F 범위의 주소를 순차적으로 스캔합니다
 *  - 발견된 장치의 주소와 가능한 장치 이름을 출력합니다
 *  - 주소 맵(Address Map)을 표 형태로 출력합니다
 *  - 5초마다 자동으로 재스캔합니다
 *
 *  배선 안내 (Wiring Guide):
 *  ┌──────────────────────────────────────────────────┐
 *  │                                                  │
 *  │  ESP32             I2C 장치 (예: OLED, 센서)     │
 *  │  ─────             ────────────────────────      │
 *  │                                                  │
 *  │  3.3V ──────────── VCC                           │
 *  │  GND  ──────────── GND                           │
 *  │  GPIO 21 (SDA) ─── SDA                           │
 *  │  GPIO 22 (SCL) ─── SCL                           │
 *  │                                                  │
 *  │  풀업 저항 (많은 모듈에 내장되어 있음):           │
 *  │                                                  │
 *  │    3.3V          3.3V                            │
 *  │     │              │                             │
 *  │   [4.7kΩ]        [4.7kΩ]                        │
 *  │     │              │                             │
 *  │    SDA            SCL                            │
 *  │                                                  │
 *  │  ※ 대부분의 브레이크아웃 보드에는 풀업 저항이    │
 *  │    이미 포함되어 있으므로 별도 저항이 불필요      │
 *  │                                                  │
 *  │  ※ 여러 I2C 장치를 같은 SDA/SCL에 병렬 연결     │
 *  │    가능 (각각 다른 주소를 가져야 함)              │
 *  │                                                  │
 *  └──────────────────────────────────────────────────┘
 *
 *  필요 부품:
 *  - ESP32 개발보드
 *  - I2C 장치 1개 이상 (OLED, 센서 등)
 *  - 브레드보드 및 점퍼 와이어
 *
 *  시리얼 모니터 설정: 115200 baud
 * ============================================================
 */

#include <Wire.h>  // I2C 통신을 위한 표준 라이브러리

// ─────────────── I2C 핀 정의 ───────────────
const int SDA_PIN = 21;   // I2C 데이터 라인 (ESP32 기본값)
const int SCL_PIN = 22;   // I2C 클럭 라인 (ESP32 기본값)

// ─────────────── 스캔 설정 ───────────────
const unsigned long SCAN_INTERVAL = 5000;  // 스캔 간격: 5초
unsigned long lastScanTime = 0;            // 마지막 스캔 시각
int scanCount = 0;                         // 스캔 횟수 카운터

// ─────────────── 초기 설정 ───────────────
void setup() {
    // 시리얼 통신 초기화
    Serial.begin(115200);

    // I2C 초기화 (SDA=GPIO21, SCL=GPIO22)
    Wire.begin(SDA_PIN, SCL_PIN);

    // I2C 클럭 속도 설정 (100kHz 표준 모드)
    Wire.setClock(100000);

    // 시작 메시지
    Serial.println();
    Serial.println("=================================");
    Serial.println("  ESP32 I2C 버스 스캐너");
    Serial.println("=================================");
    Serial.printf("SDA 핀: GPIO %d\n", SDA_PIN);
    Serial.printf("SCL 핀: GPIO %d\n", SCL_PIN);
    Serial.println("=================================");
    Serial.println();
    Serial.println("I2C 장치를 검색합니다...");
    Serial.println();

    // 첫 번째 스캔 즉시 실행
    scanI2C();
}

// ─────────────── 메인 루프 ───────────────
void loop() {
    // 일정 간격으로 재스캔
    unsigned long currentTime = millis();
    if (currentTime - lastScanTime >= SCAN_INTERVAL) {
        lastScanTime = currentTime;
        scanI2C();
    }
}

// ─────────────── I2C 스캔 함수 ───────────────
/**
 * I2C 버스의 모든 주소(0x01~0x7F)를 스캔하여
 * 응답하는 장치의 주소를 출력하는 함수
 */
void scanI2C() {
    scanCount++;
    int deviceCount = 0;     // 발견된 장치 수
    int errorCount = 0;      // 오류 발생 수

    Serial.printf("─── 스캔 #%d 시작 ───\n", scanCount);

    // 0x01 ~ 0x7F 범위의 모든 I2C 주소를 순차적으로 확인
    // 0x00은 일반 호출 주소이므로 건너뜀
    for (byte address = 1; address < 127; address++) {
        // I2C 전송 시작 (해당 주소로)
        Wire.beginTransmission(address);
        // 전송 종료하고 응답 확인
        byte error = Wire.endTransmission();

        // error 코드 해석:
        // 0 = 성공 (장치가 응답함, ACK 수신)
        // 1 = 데이터 버퍼 오버플로
        // 2 = 주소 전송 시 NACK 수신 (장치 없음)
        // 3 = 데이터 전송 시 NACK 수신
        // 4 = 기타 오류

        if (error == 0) {
            // ──── 장치 발견! ────
            deviceCount++;
            Serial.printf("  [발견] 주소: 0x%02X (%3d)", address, address);

            // 주소로 장치 이름 추측
            String deviceName = guessDeviceName(address);
            if (deviceName.length() > 0) {
                Serial.print("  →  ");
                Serial.print(deviceName);
            }
            Serial.println();

        } else if (error == 4) {
            // ──── 알 수 없는 오류 ────
            errorCount++;
            Serial.printf("  [오류] 주소 0x%02X에서 알 수 없는 오류 발생\n", address);
        }
        // error == 2는 정상 (장치 없음), 무시
    }

    // ──── 스캔 결과 요약 ────
    Serial.println();
    if (deviceCount == 0) {
        Serial.println("  ※ I2C 장치를 찾지 못했습니다!");
        Serial.println("  확인 사항:");
        Serial.println("    1. 배선이 올바른지 확인 (SDA↔SDA, SCL↔SCL)");
        Serial.println("    2. VCC와 GND가 연결되었는지 확인");
        Serial.println("    3. 풀업 저항(4.7kΩ)이 있는지 확인");
        Serial.println("    4. 장치 전원이 켜져 있는지 확인");
    } else {
        Serial.printf("  총 %d개의 I2C 장치를 발견했습니다.\n", deviceCount);
    }

    if (errorCount > 0) {
        Serial.printf("  ※ %d개의 오류가 발생했습니다.\n", errorCount);
    }

    // ──── 주소 맵 출력 ────
    printAddressMap();

    Serial.printf("\n─── %d초 후 재스캔 ───\n\n",
                  (int)(SCAN_INTERVAL / 1000));
}

// ─────────────── 장치 이름 추측 함수 ───────────────
/**
 * I2C 주소로 장치 이름을 추측하는 함수
 * 자주 사용되는 I2C 장치의 주소를 기반으로 합니다
 *
 * @param address  I2C 장치 주소 (7비트)
 * @return         추측된 장치 이름 문자열
 */
String guessDeviceName(byte address) {
    switch (address) {
        // OLED 디스플레이
        case 0x3C: return "SSD1306 OLED (128x64)";
        case 0x3D: return "SSD1306 OLED (대체 주소)";

        // 온습도 센서
        case 0x44: return "SHT30/SHT31 온습도 센서";
        case 0x45: return "SHT30/SHT31 (대체 주소)";
        case 0x40: return "HDC1080/SHT20 온습도 센서";

        // 기압/온도 센서
        case 0x76: return "BMP280/BME280 기압 센서";
        case 0x77: return "BMP280/BME280 (대체 주소) 또는 BMP180";

        // 가속도/자이로 센서
        case 0x68: return "MPU6050 가속도/자이로 또는 DS3231 RTC";
        case 0x69: return "MPU6050 (대체 주소)";

        // GPIO 확장기
        case 0x20: case 0x21: case 0x22: case 0x23:
        case 0x24: case 0x25: case 0x26: case 0x27:
            return "PCF8574 GPIO 확장기";

        // ADC
        case 0x48: return "ADS1115/ADS1015 16비트 ADC";
        case 0x49: case 0x4A: case 0x4B:
            return "ADS1115/ADS1015 ADC (대체 주소)";

        // EEPROM
        case 0x50: case 0x51: case 0x52: case 0x53:
        case 0x54: case 0x55: case 0x56: case 0x57:
            return "AT24C32/AT24C64 EEPROM";

        // 전류/전력 센서
        case 0x41: return "INA219 전류/전력 센서";

        // 조도 센서
        case 0x23: return "BH1750 조도 센서";
        case 0x5C: return "BH1750 (대체 주소)";

        // LCD
        case 0x27: return "PCF8574 (LCD I2C 백팩) 또는 GPIO 확장기";
        case 0x3F: return "PCF8574A (LCD I2C 백팩)";

        default: return "";  // 알 수 없는 장치
    }
}

// ─────────────── 주소 맵 출력 함수 ───────────────
/**
 * I2C 주소를 8x16 표 형태로 출력하는 함수
 * '##'은 장치가 있는 주소, '--'은 비어있는 주소를 나타냅니다
 */
void printAddressMap() {
    Serial.println();
    Serial.println("  I2C 주소 맵:");
    Serial.println("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");

    for (byte row = 0; row < 8; row++) {
        Serial.printf("  %X0", row);

        for (byte col = 0; col < 16; col++) {
            byte address = row * 16 + col;

            if (address < 1 || address > 126) {
                // 예약된 주소 범위
                Serial.print("   ");
            } else {
                // 장치 존재 여부 확인
                Wire.beginTransmission(address);
                byte error = Wire.endTransmission();

                if (error == 0) {
                    Serial.print(" ##");  // 장치 발견
                } else {
                    Serial.print(" --");  // 장치 없음
                }
            }
        }
        Serial.println();
    }
    Serial.println("  (## = 장치 발견, -- = 없음)");
}

/*
 * ============================================================
 *  I2C 스캐너 동작 원리
 * ============================================================
 *
 *  I2C에서 마스터(ESP32)가 주소를 보내면:
 *  - 해당 주소의 슬레이브가 있으면 → ACK(응답) 전송
 *  - 해당 주소의 슬레이브가 없으면 → NACK(무응답)
 *
 *  스캐너는 0x01~0x7E까지 모든 주소로 전송을 시도하여
 *  ACK를 받는 주소를 찾아냅니다.
 *
 *  Wire.beginTransmission(addr);  // 주소 전송 시작
 *  byte error = Wire.endTransmission();  // 전송 완료 및 결과 확인
 *
 *  error == 0: ACK 수신 (장치 존재!)
 *  error == 2: NACK 수신 (장치 없음)
 *
 * ============================================================
 *  문제 해결 가이드
 * ============================================================
 *
 *  1. "장치를 찾지 못했습니다" 메시지가 나올 때:
 *     - SDA(데이터)와 SCL(클럭) 배선 확인
 *     - VCC(3.3V)와 GND 연결 확인
 *     - 풀업 저항 확인 (모듈에 내장 안 된 경우 4.7kΩ 필요)
 *     - 장치가 3.3V 호환인지 확인 (5V 전용 장치는 레벨 시프터 필요)
 *
 *  2. 주소가 데이터시트와 다를 때:
 *     - 일부 데이터시트는 8비트 주소(R/W 비트 포함)를 사용
 *     - 예: 0x78(8비트) = 0x3C(7비트)
 *     - Arduino/ESP32 Wire 라이브러리는 7비트 주소 사용
 *
 *  3. 같은 주소의 장치 2개를 사용하고 싶을 때:
 *     - 장치의 ADDR 핀으로 주소 변경 (장치마다 다름)
 *     - I2C 멀티플렉서(TCA9548A) 사용
 *     - 두 번째 I2C 버스 사용 (Wire1)
 *
 * ============================================================
 */
