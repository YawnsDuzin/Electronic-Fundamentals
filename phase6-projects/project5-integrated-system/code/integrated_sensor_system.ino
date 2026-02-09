/*
 * ===================================================================
 *  프로젝트 5: 통합 센서 모니터링 시스템
 * ===================================================================
 *
 *  설명: SHT30 온습도 센서, MQ-2 가스 센서, 릴레이, WiFi 웹서버를
 *        하나의 시스템으로 통합한 환경 모니터링 프로젝트입니다.
 *        Phase 1~5에서 학습한 모든 지식을 종합합니다.
 *
 *  보드: ESP32 DevKit V1
 *  작성: Phase 6 - 실전 프로젝트 (최종)
 *
 *  주요 기능:
 *    1. SHT30 I2C 온습도 센서 읽기
 *    2. MQ-2 가스 센서 ADC 읽기 (이동 평균 필터 적용)
 *    3. 릴레이 자동/수동 제어 (임계값 초과 시 자동 동작)
 *    4. WiFi 웹서버 (센서 데이터 표시, 릴레이 원격 제어)
 *    5. JSON API 엔드포인트 (/api/data, /api/relay)
 *    6. 시리얼 모니터 디버깅 출력
 *    7. 내장 LED 상태 표시
 *
 *  필요 라이브러리:
 *    - Wire.h (ESP32 내장, I2C 통신용)
 *    - WiFi.h (ESP32 내장, WiFi 연결용)
 *    - WebServer.h (ESP32 내장, HTTP 웹서버용)
 *    ※ 외부 라이브러리 없이 Wire 라이브러리로 SHT30 직접 통신
 *
 *  회로 연결:
 *  ┌──────────────────────────────────────────────────────────┐
 *  │                                                          │
 *  │  [SHT30 온습도 센서] (I2C)                                │
 *  │    VCC ──── ESP32 3.3V                                   │
 *  │    GND ──── ESP32 GND                                    │
 *  │    SDA ──── ESP32 GPIO21                                 │
 *  │    SCL ──── ESP32 GPIO22                                 │
 *  │    ADDR ─── GND (I2C 주소: 0x44)                         │
 *  │                                                          │
 *  │  [MQ-2 가스 센서] (아날로그)                               │
 *  │    VCC ──── ESP32 VIN (5V)                               │
 *  │    GND ──── ESP32 GND                                    │
 *  │    AOUT ─── ESP32 GPIO34 (ADC 입력, 입력 전용 핀)         │
 *  │    DOUT ─── (미사용)                                      │
 *  │                                                          │
 *  │  [릴레이 모듈] (1채널)                                     │
 *  │    VCC ──── ESP32 VIN (5V)                               │
 *  │    GND ──── ESP32 GND                                    │
 *  │    IN  ──── ESP32 GPIO26                                 │
 *  │    COM ──── 외부 전원 (+)                                 │
 *  │    NO  ──── 부하(팬 등) (+)                               │
 *  │    부하(-) ── 외부 전원(-)                                 │
 *  │                                                          │
 *  │  [내장 LED]                                               │
 *  │    GPIO2 ── ESP32 온보드 LED (상태 표시)                   │
 *  │                                                          │
 *  │  [I2C 풀업 저항] (모듈에 내장된 경우 생략)                  │
 *  │    SDA ── 4.7KΩ ── 3.3V                                  │
 *  │    SCL ── 4.7KΩ ── 3.3V                                  │
 *  │                                                          │
 *  └──────────────────────────────────────────────────────────┘
 *
 * ===================================================================
 */

// ===================================================================
//  라이브러리 포함
// ===================================================================

#include <Wire.h>          // I2C 통신 (SHT30 센서용)
#include <WiFi.h>          // WiFi 연결 (ESP32 내장)
#include <WebServer.h>     // HTTP 웹서버 (ESP32 내장)

// ===================================================================
//  WiFi 설정 - 본인의 네트워크 정보로 수정하세요
// ===================================================================

// WiFi 모드 선택: true = AP 모드 (ESP32가 핫스팟), false = STA 모드 (공유기에 연결)
#define WIFI_USE_AP_MODE    false

// STA 모드 설정 (기존 공유기에 연결)
#define WIFI_STA_SSID       "YourWiFiSSID"       // 공유기 WiFi 이름
#define WIFI_STA_PASSWORD   "YourWiFiPassword"   // 공유기 WiFi 비밀번호

// AP 모드 설정 (ESP32가 핫스팟 생성)
#define WIFI_AP_SSID        "ESP32-Monitor"      // AP 이름 (핫스팟 이름)
#define WIFI_AP_PASSWORD    "12345678"           // AP 비밀번호 (최소 8자)

// WiFi 연결 시도 최대 횟수 (STA 모드)
#define WIFI_CONNECT_TIMEOUT  20   // 20회 시도 (약 10초)

// WiFi 재연결 주기 (밀리초)
#define WIFI_RECONNECT_INTERVAL  30000   // 30초마다 연결 상태 확인

// ===================================================================
//  핀 정의
// ===================================================================

#define PIN_SDA         21    // I2C 데이터 핀 (SHT30 SDA)
#define PIN_SCL         22    // I2C 클록 핀 (SHT30 SCL)
#define PIN_MQ2_ANALOG  34    // MQ-2 가스 센서 아날로그 출력 (ADC1, 입력 전용)
#define PIN_RELAY       26    // 릴레이 제어 핀
#define PIN_LED         2     // 내장 LED 핀 (상태 표시)

// ===================================================================
//  센서 및 제어 설정
// ===================================================================

// SHT30 I2C 주소 (ADDR 핀이 GND에 연결된 경우)
#define SHT30_ADDR      0x44

// MQ-2 센서 설정
#define MQ2_SAMPLES     10           // 이동 평균 필터 샘플 수
#define MQ2_PREHEAT_SEC 120          // MQ-2 예열 시간 (초) - 최소 2분

// 가스 농도 임계값 (ADC 원시값 기준, 0~4095)
// ※ 실제 환경에 맞게 보정이 필요합니다
#define GAS_THRESHOLD_ON    1500     // 이 값 이상이면 릴레이 ON (위험)
#define GAS_THRESHOLD_OFF   1200     // 이 값 이하로 내려가면 릴레이 OFF (안전)
                                      // ON/OFF 임계값 차이 = 히스테리시스 (떨림 방지)

// 온도 경고 임계값 (°C)
#define TEMP_WARNING_HIGH   35.0     // 이 온도 이상이면 경고
#define TEMP_WARNING_LOW    5.0      // 이 온도 이하이면 경고

// 릴레이 설정
#define RELAY_ACTIVE_LOW    true     // true = Active LOW 릴레이 모듈

// 측정 주기 (밀리초)
#define MEASURE_INTERVAL    2000     // 2초마다 센서 읽기

// 시리얼 출력 주기 (밀리초)
#define SERIAL_PRINT_INTERVAL  5000  // 5초마다 시리얼에 출력

// 시리얼 통신 속도
#define SERIAL_BAUD         115200

// ===================================================================
//  웹서버 객체 생성
// ===================================================================

WebServer server(80);   // 포트 80에서 HTTP 서버 동작

// ===================================================================
//  전역 변수
// ===================================================================

// --- 센서 데이터 ---
float   sht30Temperature = 0.0;   // SHT30 온도 (°C)
float   sht30Humidity    = 0.0;   // SHT30 습도 (%)
bool    sht30Valid       = false;  // SHT30 데이터 유효 여부
int     mq2RawValue      = 0;     // MQ-2 ADC 원시값 (이동 평균 적용 후)
bool    mq2Preheated     = false;  // MQ-2 예열 완료 여부

// MQ-2 이동 평균 필터용 배열
int     mq2Samples[MQ2_SAMPLES];   // 샘플 저장 배열
int     mq2SampleIndex = 0;        // 현재 샘플 인덱스
long    mq2SampleSum   = 0;        // 샘플 합계 (평균 계산용)
bool    mq2BufferFull  = false;    // 버퍼가 한 번 이상 가득 찼는지

// --- 릴레이 상태 ---
bool    relayState     = false;    // 현재 릴레이 상태 (true=ON)
bool    relayAutoMode  = true;     // 자동 제어 모드 (true=자동, false=수동)
bool    gasAlert       = false;    // 가스 경고 상태

// --- 타이밍 변수 ---
unsigned long lastMeasureTime    = 0;   // 마지막 센서 측정 시각
unsigned long lastSerialPrint    = 0;   // 마지막 시리얼 출력 시각
unsigned long lastWifiCheck      = 0;   // 마지막 WiFi 상태 확인 시각
unsigned long systemStartTime    = 0;   // 시스템 시작 시각
unsigned long measureCount       = 0;   // 총 측정 횟수

// ===================================================================
//  초기 설정 함수 (setup)
// ===================================================================

void setup() {
  // --- 시리얼 통신 초기화 ---
  Serial.begin(SERIAL_BAUD);
  while (!Serial) { delay(10); }

  systemStartTime = millis();

  Serial.println();
  Serial.println("╔══════════════════════════════════════════════╗");
  Serial.println("║   프로젝트 5: 통합 센서 모니터링 시스템       ║");
  Serial.println("╚══════════════════════════════════════════════╝");
  Serial.println();

  // --- 핀 설정 ---
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  setRelayOutput(false);            // 릴레이 초기 상태: OFF
  digitalWrite(PIN_LED, LOW);       // LED 초기 상태: 꺼짐

  Serial.println("  [핀 설정]");
  Serial.print("    릴레이: GPIO");
  Serial.println(PIN_RELAY);
  Serial.print("    LED:    GPIO");
  Serial.println(PIN_LED);
  Serial.print("    MQ-2:   GPIO");
  Serial.println(PIN_MQ2_ANALOG);
  Serial.println();

  // --- I2C 초기화 ---
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);            // I2C 클록: 100kHz (표준 모드)
  Serial.print("  [I2C] SDA=GPIO");
  Serial.print(PIN_SDA);
  Serial.print(", SCL=GPIO");
  Serial.println(PIN_SCL);

  // --- SHT30 초기화 ---
  initSHT30();

  // --- MQ-2 초기화 ---
  initMQ2();

  // --- WiFi 연결 ---
  initWiFi();

  // --- 웹서버 라우트 설정 ---
  setupWebRoutes();

  // --- 웹서버 시작 ---
  server.begin();
  Serial.println("  [웹서버] 포트 80에서 시작됨");

  Serial.println();
  Serial.println("══════════════════════════════════════════════");
  Serial.println("  시스템 초기화 완료! 동작을 시작합니다.");
  Serial.println("══════════════════════════════════════════════");
  Serial.println();
}

// ===================================================================
//  메인 루프 함수 (loop)
// ===================================================================

void loop() {
  unsigned long currentTime = millis();

  // 1. 웹 클라이언트 요청 처리 (매 루프마다)
  server.handleClient();

  // 2. 센서 데이터 읽기 (MEASURE_INTERVAL마다)
  if (currentTime - lastMeasureTime >= MEASURE_INTERVAL) {
    lastMeasureTime = currentTime;
    measureCount++;

    // SHT30 온습도 읽기
    readSHT30();

    // MQ-2 가스 센서 읽기 (이동 평균 필터 적용)
    readMQ2();

    // 자동 릴레이 제어 (자동 모드인 경우)
    if (relayAutoMode) {
      autoRelayControl();
    }

    // 내장 LED 상태 업데이트
    updateLED();
  }

  // 3. 시리얼 디버그 출력 (SERIAL_PRINT_INTERVAL마다)
  if (currentTime - lastSerialPrint >= SERIAL_PRINT_INTERVAL) {
    lastSerialPrint = currentTime;
    printSensorData();
  }

  // 4. WiFi 연결 상태 확인 및 재연결 (STA 모드일 때)
  if (!WIFI_USE_AP_MODE) {
    if (currentTime - lastWifiCheck >= WIFI_RECONNECT_INTERVAL) {
      lastWifiCheck = currentTime;
      checkWiFiConnection();
    }
  }
}

// ===================================================================
//  SHT30 센서 함수 (Wire 라이브러리 직접 통신)
// ===================================================================

/*
 * SHT30 센서를 초기화합니다.
 * I2C 통신이 가능한지 확인합니다.
 */
void initSHT30() {
  Serial.print("  [SHT30] 초기화 중... (주소: 0x");
  Serial.print(SHT30_ADDR, HEX);
  Serial.print(") → ");

  // SHT30에 소프트 리셋 명령 전송 (0x30A2)
  Wire.beginTransmission(SHT30_ADDR);
  Wire.write(0x30);
  Wire.write(0xA2);
  uint8_t error = Wire.endTransmission();

  if (error == 0) {
    Serial.println("성공!");
    sht30Valid = true;
    delay(15);   // 리셋 후 안정화 대기
  } else {
    Serial.println("실패!");
    Serial.println("    → SHT30 센서를 찾을 수 없습니다.");
    Serial.println("    → I2C 배선과 ADDR 핀을 확인하세요.");
    sht30Valid = false;
  }
}

/*
 * SHT30에서 온도와 습도를 읽습니다.
 * Wire 라이브러리를 사용하여 직접 I2C 통신합니다.
 *
 * SHT30 통신 프로토콜:
 *   1. 측정 명령 전송 (0x2400 = 고반복성 단일 측정)
 *   2. 약 15ms 대기 (측정 완료 시간)
 *   3. 6바이트 데이터 수신:
 *      [온도 MSB] [온도 LSB] [온도 CRC] [습도 MSB] [습도 LSB] [습도 CRC]
 *   4. CRC 검증 후 물리값으로 변환
 */
void readSHT30() {
  if (!sht30Valid) return;

  // 측정 명령 전송: 고반복성, 클록 스트레칭 활성화 (0x2C06)
  Wire.beginTransmission(SHT30_ADDR);
  Wire.write(0x2C);   // 명령 MSB
  Wire.write(0x06);   // 명령 LSB
  uint8_t error = Wire.endTransmission();

  if (error != 0) {
    sht30Valid = false;
    return;
  }

  // 측정 완료 대기 (클록 스트레칭 모드에서는 빠르지만 안전 마진 추가)
  delay(20);

  // 6바이트 데이터 요청
  uint8_t bytesReceived = Wire.requestFrom((uint8_t)SHT30_ADDR, (uint8_t)6);
  if (bytesReceived != 6) {
    sht30Valid = false;
    return;
  }

  // 데이터 수신
  uint8_t data[6];
  for (int i = 0; i < 6; i++) {
    data[i] = Wire.read();
  }

  // CRC 검증 (온도)
  if (!checkSHT30CRC(data[0], data[1], data[2])) {
    return;   // CRC 오류: 이전 값 유지
  }

  // CRC 검증 (습도)
  if (!checkSHT30CRC(data[3], data[4], data[5])) {
    return;   // CRC 오류: 이전 값 유지
  }

  // 온도 계산: -45 + 175 * (rawTemp / 65535)
  uint16_t rawTemp = ((uint16_t)data[0] << 8) | data[1];
  sht30Temperature = -45.0 + 175.0 * ((float)rawTemp / 65535.0);

  // 습도 계산: 100 * (rawHumi / 65535)
  uint16_t rawHumi = ((uint16_t)data[3] << 8) | data[4];
  sht30Humidity = 100.0 * ((float)rawHumi / 65535.0);

  // 범위 검증
  if (sht30Temperature < -40.0 || sht30Temperature > 125.0 ||
      sht30Humidity < 0.0 || sht30Humidity > 100.0) {
    sht30Valid = false;
    return;
  }

  sht30Valid = true;
}

/*
 * SHT30 CRC-8 검증 함수
 * 다항식: x^8 + x^5 + x^4 + 1 (0x31)
 * 초기값: 0xFF
 *
 * @param msb: 데이터 상위 바이트
 * @param lsb: 데이터 하위 바이트
 * @param crc: 수신된 CRC 값
 * @return: CRC 일치 시 true
 */
bool checkSHT30CRC(uint8_t msb, uint8_t lsb, uint8_t crc) {
  uint8_t calcCRC = 0xFF;   // CRC 초기값
  uint8_t data[2] = {msb, lsb};

  for (int i = 0; i < 2; i++) {
    calcCRC ^= data[i];
    for (int bit = 0; bit < 8; bit++) {
      if (calcCRC & 0x80) {
        calcCRC = (calcCRC << 1) ^ 0x31;   // 다항식 적용
      } else {
        calcCRC = calcCRC << 1;
      }
    }
  }

  return (calcCRC == crc);
}

// ===================================================================
//  MQ-2 가스 센서 함수
// ===================================================================

/*
 * MQ-2 가스 센서를 초기화합니다.
 * 이동 평균 필터 버퍼를 0으로 초기화합니다.
 */
void initMQ2() {
  Serial.print("  [MQ-2] 초기화 중... (GPIO");
  Serial.print(PIN_MQ2_ANALOG);
  Serial.println(")");

  // ADC 감쇠 설정 (ESP32 ADC는 기본적으로 0~1.1V 범위)
  // 11dB 감쇠 → 약 0~3.3V 범위로 확장 (실제로는 ~3.1V)
  analogSetAttenuation(ADC_11db);

  // 이동 평균 버퍼 초기화
  for (int i = 0; i < MQ2_SAMPLES; i++) {
    mq2Samples[i] = 0;
  }
  mq2SampleSum = 0;
  mq2SampleIndex = 0;
  mq2BufferFull = false;

  // 예열 시간 안내
  Serial.print("    예열 시간: ");
  Serial.print(MQ2_PREHEAT_SEC);
  Serial.println("초 (예열 중에는 가스 값이 부정확합니다)");

  mq2Preheated = false;
}

/*
 * MQ-2 센서의 아날로그 값을 읽고 이동 평균 필터를 적용합니다.
 *
 * 이동 평균 필터(Moving Average Filter):
 *   - 최근 N개의 샘플 평균값을 사용
 *   - 순간적인 노이즈를 효과적으로 제거
 *   - N이 클수록 안정적이지만 응답이 느려짐
 *
 * 동작 원리:
 *   1. 새 샘플값에서 가장 오래된 샘플값의 차이를 합계에 반영
 *   2. 새 샘플을 배열에 저장
 *   3. 합계를 샘플 수로 나눠 평균 계산
 *   → O(1) 시간 복잡도로 효율적 (매번 전체 합산 불필요)
 */
void readMQ2() {
  // ADC 원시값 읽기 (12비트: 0~4095)
  int rawValue = analogRead(PIN_MQ2_ANALOG);

  // 이동 평균 필터 적용
  // 가장 오래된 샘플을 합계에서 빼고, 새 샘플을 합계에 더함
  mq2SampleSum -= mq2Samples[mq2SampleIndex];
  mq2Samples[mq2SampleIndex] = rawValue;
  mq2SampleSum += rawValue;

  // 인덱스를 순환시킴 (0, 1, 2, ..., N-1, 0, 1, ...)
  mq2SampleIndex = (mq2SampleIndex + 1) % MQ2_SAMPLES;

  // 버퍼가 한 바퀴 이상 돌았는지 확인
  if (mq2SampleIndex == 0) {
    mq2BufferFull = true;
  }

  // 이동 평균 계산
  int divisor = mq2BufferFull ? MQ2_SAMPLES : (mq2SampleIndex > 0 ? mq2SampleIndex : 1);
  mq2RawValue = (int)(mq2SampleSum / divisor);

  // 예열 상태 확인
  unsigned long elapsed = (millis() - systemStartTime) / 1000;
  if (!mq2Preheated && elapsed >= MQ2_PREHEAT_SEC) {
    mq2Preheated = true;
    Serial.println("  [MQ-2] 예열 완료! 가스 센서 측정값이 유효합니다.");
  }
}

// ===================================================================
//  릴레이 제어 함수
// ===================================================================

/*
 * 릴레이의 물리적 출력을 설정합니다.
 *
 * @param on: true = 릴레이 ON, false = 릴레이 OFF
 */
void setRelayOutput(bool on) {
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(PIN_RELAY, on ? LOW : HIGH);
  } else {
    digitalWrite(PIN_RELAY, on ? HIGH : LOW);
  }
  relayState = on;
}

/*
 * 가스 농도에 따라 릴레이를 자동으로 제어합니다.
 *
 * 히스테리시스(이력 현상) 적용:
 *   - ON 임계값(1500)과 OFF 임계값(1200)을 다르게 설정
 *   - 이렇게 하면 임계값 근처에서 릴레이가 빠르게 ON/OFF 반복하는
 *     '채터링' 현상을 방지할 수 있습니다.
 *
 *   가스 농도 →   0 ─── 1200 ─── 1500 ─── 4095
 *                     OFF구간   이력구간   ON구간
 *
 *   릴레이 OFF 상태에서: 1500 이상 → ON
 *   릴레이 ON 상태에서:  1200 이하 → OFF
 *   → 1200~1500 사이에서는 현재 상태 유지
 */
void autoRelayControl() {
  // MQ-2가 아직 예열 중이면 자동 제어하지 않음
  if (!mq2Preheated) return;

  if (!relayState && mq2RawValue >= GAS_THRESHOLD_ON) {
    // 릴레이가 꺼져 있는데 가스 농도가 ON 임계값 이상 → 릴레이 켜기
    setRelayOutput(true);
    gasAlert = true;
    Serial.println("  [자동 제어] 가스 농도 높음! 릴레이 ON (환기 시작)");
  }
  else if (relayState && mq2RawValue <= GAS_THRESHOLD_OFF) {
    // 릴레이가 켜져 있는데 가스 농도가 OFF 임계값 이하 → 릴레이 끄기
    setRelayOutput(false);
    gasAlert = false;
    Serial.println("  [자동 제어] 가스 농도 정상. 릴레이 OFF (환기 중지)");
  }
}

// ===================================================================
//  LED 상태 표시 함수
// ===================================================================

/*
 * 시스템 상태에 따라 내장 LED를 업데이트합니다.
 *
 * 동작 패턴:
 *   - 가스 경고: LED 빠르게 깜빡임 (200ms 간격)
 *   - 릴레이 ON: LED 계속 켜짐
 *   - MQ-2 예열 중: LED 느리게 깜빡임 (1초 간격)
 *   - 정상 대기: LED 꺼짐 (3초마다 한 번 짧게 깜빡임)
 */
void updateLED() {
  if (gasAlert) {
    // 가스 경고: 빠르게 깜빡임
    bool blink = ((millis() / 200) % 2) == 0;
    digitalWrite(PIN_LED, blink ? HIGH : LOW);
  }
  else if (relayState) {
    // 릴레이 ON: LED 계속 켜짐
    digitalWrite(PIN_LED, HIGH);
  }
  else if (!mq2Preheated) {
    // 예열 중: 느리게 깜빡임
    bool blink = ((millis() / 1000) % 2) == 0;
    digitalWrite(PIN_LED, blink ? HIGH : LOW);
  }
  else {
    // 정상 대기: 3초마다 한 번 짧게 깜빡임 (하트비트)
    unsigned long phase = millis() % 3000;
    digitalWrite(PIN_LED, (phase < 100) ? HIGH : LOW);
  }
}

// ===================================================================
//  WiFi 초기화 및 관리 함수
// ===================================================================

/*
 * WiFi를 초기화합니다. AP 모드 또는 STA 모드를 설정에 따라 선택합니다.
 */
void initWiFi() {
  Serial.println();
  Serial.println("  [WiFi] 초기화 중...");

  if (WIFI_USE_AP_MODE) {
    // --- AP 모드: ESP32가 핫스팟을 생성 ---
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);

    IPAddress apIP = WiFi.softAPIP();
    Serial.println("  [WiFi] AP 모드 시작!");
    Serial.print("    SSID: ");
    Serial.println(WIFI_AP_SSID);
    Serial.print("    비밀번호: ");
    Serial.println(WIFI_AP_PASSWORD);
    Serial.print("    IP 주소: ");
    Serial.println(apIP);
    Serial.println("    → 이 IP 주소를 웹 브라우저에 입력하세요.");
  }
  else {
    // --- STA 모드: 기존 공유기에 연결 ---
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);

    Serial.print("    SSID: ");
    Serial.println(WIFI_STA_SSID);
    Serial.print("    연결 중");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_TIMEOUT) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("  [WiFi] 연결 성공!");
      Serial.print("    IP 주소: ");
      Serial.println(WiFi.localIP());
      Serial.print("    신호 세기: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
      Serial.println("    → 이 IP 주소를 웹 브라우저에 입력하세요.");
    } else {
      Serial.println("  [WiFi] 연결 실패!");
      Serial.println("    → SSID와 비밀번호를 확인하세요.");
      Serial.println("    → AP 모드로 자동 전환합니다.");

      // STA 연결 실패 시 AP 모드로 폴백
      WiFi.mode(WIFI_AP);
      WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
      Serial.print("    AP IP: ");
      Serial.println(WiFi.softAPIP());
    }
  }
}

/*
 * STA 모드에서 WiFi 연결 상태를 확인하고 끊어졌으면 재연결합니다.
 */
void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("  [WiFi] 연결 끊김! 재연결 시도...");
    WiFi.disconnect();
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
      delay(500);
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("  [WiFi] 재연결 성공! IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("  [WiFi] 재연결 실패. 다음 주기에 재시도합니다.");
    }
  }
}

// ===================================================================
//  웹서버 라우트 설정
// ===================================================================

/*
 * 웹서버의 URL 라우트(경로)를 설정합니다.
 * 각 URL에 대한 핸들러 함수를 등록합니다.
 */
void setupWebRoutes() {
  // 메인 페이지 (HTML)
  server.on("/", HTTP_GET, handleRoot);

  // JSON API: 센서 데이터 조회
  server.on("/api/data", HTTP_GET, handleApiData);

  // JSON API: 릴레이 제어
  server.on("/api/relay", HTTP_GET, handleApiRelay);

  // 404 처리
  server.onNotFound(handleNotFound);

  Serial.println("  [웹서버] 라우트 설정 완료");
  Serial.println("    /           → 메인 페이지 (HTML)");
  Serial.println("    /api/data   → 센서 데이터 (JSON)");
  Serial.println("    /api/relay  → 릴레이 제어 (JSON)");
}

// ===================================================================
//  웹 핸들러: 메인 페이지 (HTML)
// ===================================================================

/*
 * 메인 웹페이지를 생성하여 응답합니다.
 * HTML, CSS, JavaScript를 모두 내장하여 하나의 페이지로 제공합니다.
 * JavaScript의 fetch()를 사용하여 3초마다 데이터를 자동 갱신합니다.
 */
void handleRoot() {
  // HTML 문자열 구성
  // F() 매크로: 문자열을 플래시 메모리에 저장하여 RAM 절약
  String html = F(
    "<!DOCTYPE html>"
    "<html lang='ko'>"
    "<head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>ESP32 환경 모니터링</title>"
    "<style>"
    "body{font-family:'Segoe UI',sans-serif;background:#1a1a2e;color:#eee;margin:0;padding:20px;}"
    ".container{max-width:600px;margin:0 auto;}"
    "h1{text-align:center;color:#e94560;margin-bottom:5px;}"
    ".subtitle{text-align:center;color:#888;margin-bottom:20px;font-size:14px;}"
    ".card{background:#16213e;border-radius:12px;padding:20px;margin:12px 0;"
    "box-shadow:0 4px 15px rgba(0,0,0,0.3);}"
    ".card h2{margin:0 0 15px;font-size:16px;color:#0f3460;background:#e94560;"
    "display:inline-block;padding:4px 12px;border-radius:6px;color:#fff;}"
    ".row{display:flex;justify-content:space-between;padding:8px 0;"
    "border-bottom:1px solid #0f3460;}"
    ".row:last-child{border-bottom:none;}"
    ".label{color:#aaa;}"
    ".value{font-weight:bold;font-size:18px;}"
    ".alert{color:#e94560;animation:blink 0.5s infinite;}"
    "@keyframes blink{50%{opacity:0.5;}}"
    ".normal{color:#4ecca3;}"
    ".btn-group{display:flex;gap:10px;margin-top:10px;flex-wrap:wrap;}"
    ".btn{flex:1;padding:12px;border:none;border-radius:8px;font-size:14px;"
    "cursor:pointer;font-weight:bold;min-width:80px;}"
    ".btn-on{background:#4ecca3;color:#1a1a2e;}"
    ".btn-off{background:#e94560;color:#fff;}"
    ".btn-auto{background:#0f3460;color:#fff;border:2px solid #4ecca3;}"
    ".btn-auto.active{background:#4ecca3;color:#1a1a2e;}"
    ".status-bar{text-align:center;color:#666;font-size:12px;margin-top:15px;}"
    "</style>"
    "</head>"
    "<body>"
    "<div class='container'>"
    "<h1>ESP32 환경 모니터링</h1>"
    "<p class='subtitle'>통합 센서 모니터링 시스템</p>"
    "<div class='card'>"
    "<h2>온습도 (SHT30)</h2>"
    "<div class='row'><span class='label'>온도</span>"
    "<span class='value' id='temp'>--</span></div>"
    "<div class='row'><span class='label'>습도</span>"
    "<span class='value' id='humi'>--</span></div>"
    "</div>"
    "<div class='card'>"
    "<h2>가스 센서 (MQ-2)</h2>"
    "<div class='row'><span class='label'>가스 농도 (ADC)</span>"
    "<span class='value' id='gas'>--</span></div>"
    "<div class='row'><span class='label'>상태</span>"
    "<span class='value' id='gasStatus'>--</span></div>"
    "</div>"
    "<div class='card'>"
    "<h2>릴레이 제어</h2>"
    "<div class='row'><span class='label'>릴레이 상태</span>"
    "<span class='value' id='relay'>--</span></div>"
    "<div class='row'><span class='label'>제어 모드</span>"
    "<span class='value' id='mode'>--</span></div>"
    "<div class='btn-group'>"
    "<button class='btn btn-on' onclick='relayCmd(\"on\")'>ON</button>"
    "<button class='btn btn-off' onclick='relayCmd(\"off\")'>OFF</button>"
    "<button class='btn btn-auto' id='btnAuto' onclick='relayCmd(\"auto\")'>자동</button>"
    "</div>"
    "</div>"
    "<div class='status-bar'>"
    "<span id='uptime'>가동 시간: --</span> | "
    "<span id='rssi'>WiFi: --</span> | "
    "<span id='lastUpdate'>업데이트: --</span>"
    "</div>"
    "</div>"
    "<script>"
    "function fetchData(){"
    "fetch('/api/data').then(r=>r.json()).then(d=>{"
    "document.getElementById('temp').textContent=d.temperature.toFixed(1)+' °C';"
    "document.getElementById('humi').textContent=d.humidity.toFixed(1)+' %';"
    "document.getElementById('gas').textContent=d.gas_raw;"
    "let gs=document.getElementById('gasStatus');"
    "if(d.gas_alert){gs.textContent='위험!';gs.className='value alert';}"
    "else{gs.textContent='정상';gs.className='value normal';}"
    "let rl=document.getElementById('relay');"
    "rl.textContent=d.relay_state?'ON':'OFF';"
    "rl.className=d.relay_state?'value alert':'value normal';"
    "document.getElementById('mode').textContent=d.relay_mode==='auto'?'자동':'수동';"
    "let ab=document.getElementById('btnAuto');"
    "ab.className=d.relay_mode==='auto'?'btn btn-auto active':'btn btn-auto';"
    "let s=d.uptime_sec;let h=Math.floor(s/3600);"
    "let m=Math.floor((s%3600)/60);let sc=s%60;"
    "document.getElementById('uptime').textContent='가동: '"
    "+String(h).padStart(2,'0')+':'+String(m).padStart(2,'0')+':'+String(sc).padStart(2,'0');"
    "document.getElementById('rssi').textContent='WiFi: '+d.wifi_rssi+'dBm';"
    "document.getElementById('lastUpdate').textContent='방금 업데이트';"
    "}).catch(e=>{document.getElementById('lastUpdate').textContent='연결 오류';});}"
    "function relayCmd(cmd){"
    "fetch('/api/relay?state='+cmd).then(r=>r.json()).then(d=>{"
    "fetchData();}).catch(e=>{alert('통신 오류');});}"
    "fetchData();setInterval(fetchData,3000);"
    "</script>"
    "</body></html>"
  );

  server.send(200, "text/html", html);
}

// ===================================================================
//  웹 핸들러: JSON API - 센서 데이터
// ===================================================================

/*
 * 센서 데이터를 JSON 형식으로 응답합니다.
 * JavaScript fetch()나 외부 프로그램에서 이 API를 호출하여
 * 센서 데이터를 가져올 수 있습니다.
 */
void handleApiData() {
  // 시스템 가동 시간 (초)
  unsigned long uptimeSec = (millis() - systemStartTime) / 1000;

  // WiFi 신호 세기
  int rssi = WIFI_USE_AP_MODE ? 0 : WiFi.RSSI();

  // JSON 문자열 구성
  String json = "{";
  json += "\"temperature\":" + String(sht30Temperature, 2) + ",";
  json += "\"humidity\":" + String(sht30Humidity, 2) + ",";
  json += "\"gas_raw\":" + String(mq2RawValue) + ",";
  json += "\"gas_alert\":" + String(gasAlert ? "true" : "false") + ",";
  json += "\"gas_preheated\":" + String(mq2Preheated ? "true" : "false") + ",";
  json += "\"relay_state\":" + String(relayState ? "true" : "false") + ",";
  json += "\"relay_mode\":\"" + String(relayAutoMode ? "auto" : "manual") + "\",";
  json += "\"sht30_valid\":" + String(sht30Valid ? "true" : "false") + ",";
  json += "\"uptime_sec\":" + String(uptimeSec) + ",";
  json += "\"wifi_rssi\":" + String(rssi) + ",";
  json += "\"measure_count\":" + String(measureCount);
  json += "}";

  // CORS 헤더 추가 (외부에서 API 호출 시 필요)
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

// ===================================================================
//  웹 핸들러: JSON API - 릴레이 제어
// ===================================================================

/*
 * HTTP 요청으로 릴레이를 제어합니다.
 *
 * 사용법:
 *   /api/relay?state=on    → 릴레이 ON (수동 모드로 전환)
 *   /api/relay?state=off   → 릴레이 OFF (수동 모드로 전환)
 *   /api/relay?state=auto  → 자동 모드로 전환
 */
void handleApiRelay() {
  String json;

  if (server.hasArg("state")) {
    String stateArg = server.arg("state");
    stateArg.toLowerCase();

    if (stateArg == "on") {
      relayAutoMode = false;   // 수동 모드로 전환
      setRelayOutput(true);
      json = "{\"success\":true,\"relay_state\":true,\"message\":\"릴레이 ON (수동 모드)\"}";
      Serial.println("  [웹] 릴레이 ON (수동 모드)");
    }
    else if (stateArg == "off") {
      relayAutoMode = false;   // 수동 모드로 전환
      setRelayOutput(false);
      gasAlert = false;
      json = "{\"success\":true,\"relay_state\":false,\"message\":\"릴레이 OFF (수동 모드)\"}";
      Serial.println("  [웹] 릴레이 OFF (수동 모드)");
    }
    else if (stateArg == "auto") {
      relayAutoMode = true;    // 자동 모드로 전환
      json = "{\"success\":true,\"relay_state\":" + String(relayState ? "true" : "false");
      json += ",\"message\":\"자동 모드로 전환\"}";
      Serial.println("  [웹] 자동 제어 모드로 전환");
    }
    else {
      json = "{\"success\":false,\"message\":\"잘못된 명령. on/off/auto 중 선택하세요.\"}";
    }
  } else {
    // 파라미터 없이 호출 시 현재 상태만 반환
    json = "{\"relay_state\":" + String(relayState ? "true" : "false");
    json += ",\"relay_mode\":\"" + String(relayAutoMode ? "auto" : "manual") + "\"}";
  }

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

// ===================================================================
//  웹 핸들러: 404 Not Found
// ===================================================================

void handleNotFound() {
  String json = "{\"error\":\"not_found\",\"message\":\"요청한 페이지를 찾을 수 없습니다.\"}";
  server.send(404, "application/json", json);
}

// ===================================================================
//  시리얼 출력 함수
// ===================================================================

/*
 * 센서 데이터를 시리얼 모니터에 보기 좋은 테이블 형식으로 출력합니다.
 */
void printSensorData() {
  unsigned long uptimeSec = (millis() - systemStartTime) / 1000;

  Serial.println("┌──────────────────────────────────────────┐");

  // 측정 횟수 및 가동 시간
  Serial.print("│ #");
  Serial.print(measureCount);
  Serial.print("  가동 시간: ");
  unsigned long h = uptimeSec / 3600;
  unsigned long m = (uptimeSec % 3600) / 60;
  unsigned long s = uptimeSec % 60;
  if (h < 10) Serial.print("0");
  Serial.print(h); Serial.print(":");
  if (m < 10) Serial.print("0");
  Serial.print(m); Serial.print(":");
  if (s < 10) Serial.print("0");
  Serial.print(s);
  Serial.println();

  Serial.println("├──────────────────────────────────────────┤");

  // SHT30 데이터
  Serial.print("│ [SHT30] 온도: ");
  if (sht30Valid) {
    Serial.print(sht30Temperature, 1);
    Serial.print(" °C  습도: ");
    Serial.print(sht30Humidity, 1);
    Serial.println(" %");
  } else {
    Serial.println("센서 오류");
  }

  // MQ-2 데이터
  Serial.print("│ [MQ-2]  가스: ");
  Serial.print(mq2RawValue);
  Serial.print(" / 4095");
  if (!mq2Preheated) {
    Serial.print("  (예열 중...)");
  } else if (gasAlert) {
    Serial.print("  [위험!]");
  } else {
    Serial.print("  [정상]");
  }
  Serial.println();

  // 릴레이 상태
  Serial.print("│ [릴레이] ");
  Serial.print(relayState ? "ON" : "OFF");
  Serial.print("  모드: ");
  Serial.println(relayAutoMode ? "자동" : "수동");

  // WiFi 상태
  Serial.print("│ [WiFi]  ");
  if (WIFI_USE_AP_MODE) {
    Serial.print("AP 모드  IP: ");
    Serial.print(WiFi.softAPIP());
    Serial.print("  연결: ");
    Serial.print(WiFi.softAPgetStationNum());
    Serial.println("대");
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("연결됨  IP: ");
      Serial.print(WiFi.localIP());
      Serial.print("  RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println("dBm");
    } else {
      Serial.println("연결 끊김");
    }
  }

  Serial.println("└──────────────────────────────────────────┘");
}

// ===================================================================
//  끝 (End of File)
// ===================================================================
