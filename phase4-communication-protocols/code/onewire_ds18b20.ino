/*
 * ============================================================
 *  1-Wire DS18B20 온도 센서 읽기 예제
 * ============================================================
 *
 *  기능 설명:
 *  - DS18B20 온도 센서에서 온도를 읽어옵니다
 *  - 1-Wire 프로토콜을 사용하여 통신합니다
 *  - 여러 센서를 자동 검색하고 주소를 출력합니다
 *  - 각 센서의 온도를 섭씨(°C)와 화씨(°F)로 표시합니다
 *  - 해상도 설정 및 변환 시간을 관리합니다
 *  - 최소/최대 온도를 기록합니다
 *
 *  필요 라이브러리 (Arduino IDE에서 설치):
 *  - OneWire (by Jim Studt, Paul Stoffregen)
 *  - DallasTemperature (by Miles Burton)
 *
 *  라이브러리 설치 방법:
 *  Arduino IDE → 스케치 → 라이브러리 포함하기 → 라이브러리 관리
 *  → "OneWire" 검색하여 설치
 *  → "DallasTemperature" 검색하여 설치
 *
 *  배선 안내 (Wiring Guide):
 *  ┌──────────────────────────────────────────────────┐
 *  │                                                  │
 *  │  [단일 센서 연결]                                │
 *  │                                                  │
 *  │  ESP32              DS18B20                      │
 *  │  ─────              ────────                     │
 *  │                                                  │
 *  │  3.3V ──────┬────── VDD (핀 3, 오른쪽)           │
 *  │             │                                    │
 *  │           [4.7kΩ]   ← 풀업 저항 (필수!)          │
 *  │             │                                    │
 *  │  GPIO 4 ────┼────── DQ  (핀 2, 가운데)           │
 *  │             │                                    │
 *  │  GND ───────┴────── GND (핀 1, 왼쪽)            │
 *  │                                                  │
 *  │                                                  │
 *  │  [DS18B20 핀 배치 (정면에서 볼 때)]              │
 *  │                                                  │
 *  │       ┌─────────┐                                │
 *  │       │  정면   │                                │
 *  │       │ (평면)  │                                │
 *  │       └─┬──┬──┬─┘                                │
 *  │         │  │  │                                  │
 *  │        GND DQ VDD                                │
 *  │        (1) (2) (3)                               │
 *  │                                                  │
 *  │  ※ 방향을 잘못 연결하면 센서가 뜨거워지며        │
 *  │    손상됩니다! 핀 배치를 반드시 확인하세요!       │
 *  │                                                  │
 *  │                                                  │
 *  │  [여러 센서 연결 (같은 DQ 라인)]                 │
 *  │                                                  │
 *  │  3.3V ──────┬──────────────────── VDD            │
 *  │             │                                    │
 *  │           [4.7kΩ]                                │
 *  │             │                                    │
 *  │  GPIO 4 ────┼────── DQ ──── DQ ──── DQ          │
 *  │             │       │       │       │            │
 *  │  GND ───────┴──── GND ─── GND ─── GND           │
 *  │                   센서1   센서2   센서3          │
 *  │                                                  │
 *  │  ※ 풀업 저항은 하나만 있으면 됩니다              │
 *  │  ※ 각 센서는 고유한 64비트 주소를 가지고 있어    │
 *  │    같은 데이터 라인에서도 구분 가능               │
 *  │                                                  │
 *  └──────────────────────────────────────────────────┘
 *
 *  필요 부품:
 *  - ESP32 개발보드
 *  - DS18B20 온도 센서 1개 이상
 *  - 4.7kΩ 저항 1개 (풀업 저항)
 *  - 브레드보드 및 점퍼 와이어
 *
 *  시리얼 모니터 설정: 115200 baud
 * ============================================================
 */

// ─────────────── 라이브러리 포함 ───────────────
#include <OneWire.h>            // 1-Wire 프로토콜 기본 라이브러리
#include <DallasTemperature.h>  // DS18B20 전용 상위 라이브러리

// ─────────────── 핀 정의 ───────────────
const int ONE_WIRE_BUS = 4;     // 1-Wire 데이터 핀 (GPIO 4)
                                // 이 핀에 4.7kΩ 풀업 저항을 연결!

// ─────────────── 센서 설정 ───────────────
const int SENSOR_RESOLUTION = 12;    // 센서 해상도 (9~12비트)
                                      // 9비트:  0.5°C,   93.75ms 변환
                                      // 10비트: 0.25°C,  187.5ms 변환
                                      // 11비트: 0.125°C, 375ms 변환
                                      // 12비트: 0.0625°C, 750ms 변환 (가장 정밀)

const unsigned long READ_INTERVAL = 2000;  // 읽기 간격 (밀리초)
                                            // 12비트 변환에 750ms 필요하므로
                                            // 최소 1000ms 이상 권장

// ─────────────── 1-Wire 및 센서 객체 생성 ───────────────
OneWire oneWire(ONE_WIRE_BUS);            // 1-Wire 버스 객체
DallasTemperature sensors(&oneWire);       // 온도 센서 관리 객체

// ─────────────── 센서 정보 저장 ───────────────
const int MAX_SENSORS = 10;                // 최대 센서 수
DeviceAddress sensorAddresses[MAX_SENSORS]; // 센서 주소 배열
int sensorCount = 0;                        // 발견된 센서 수

// ─────────────── 온도 기록 ───────────────
float minTemp = 999.0;       // 최저 온도 기록
float maxTemp = -999.0;      // 최고 온도 기록
unsigned long readCount = 0;  // 읽기 횟수

// ─────────────── 타이밍 ───────────────
unsigned long lastReadTime = 0;  // 마지막 읽기 시각

// ─────────────── 초기 설정 ───────────────
void setup() {
    // 시리얼 통신 초기화
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("=============================================");
    Serial.println("  DS18B20 온도 센서 읽기 예제");
    Serial.println("  (1-Wire 프로토콜)");
    Serial.println("=============================================");
    Serial.println();

    // ──── 1-Wire 센서 라이브러리 초기화 ────
    sensors.begin();

    // ──── 연결된 센서 수 확인 ────
    sensorCount = sensors.getDeviceCount();
    Serial.printf("1-Wire 데이터 핀: GPIO %d\n", ONE_WIRE_BUS);
    Serial.printf("발견된 DS18B20 센서: %d개\n", sensorCount);
    Serial.println();

    if (sensorCount == 0) {
        Serial.println("*** 센서를 찾지 못했습니다! ***");
        Serial.println("확인 사항:");
        Serial.println("  1. 배선이 올바른지 확인 (GND/DQ/VDD 순서)");
        Serial.println("  2. 4.7kΩ 풀업 저항이 DQ와 3.3V 사이에 있는지 확인");
        Serial.println("  3. 전원(VCC, GND)이 제대로 연결되었는지 확인");
        Serial.println("  4. DS18B20의 핀 방향이 올바른지 확인");
        Serial.println();
        Serial.println("5초 후 재시도합니다...");
        delay(5000);
        ESP.restart();  // ESP32 재부팅
    }

    // ──── 기생 전원 모드 확인 ────
    if (sensors.isParasitePowerMode()) {
        Serial.println("*** 기생 전원(Parasite Power) 모드 감지 ***");
        Serial.println("  VDD 핀이 연결되지 않은 것 같습니다.");
        Serial.println("  안정적인 동작을 위해 VDD에 3.3V를 연결하세요.");
        Serial.println();
    } else {
        Serial.println("전원 모드: 일반 (외부 전원)");
    }

    // ──── 각 센서 정보 출력 및 설정 ────
    Serial.println();
    Serial.println("─── 센서 목록 ───");

    for (int i = 0; i < sensorCount && i < MAX_SENSORS; i++) {
        // 센서 주소 가져오기
        if (sensors.getAddress(sensorAddresses[i], i)) {
            Serial.printf("센서 #%d: ", i + 1);
            printAddress(sensorAddresses[i]);

            // 해상도 설정
            sensors.setResolution(sensorAddresses[i], SENSOR_RESOLUTION);

            // 설정된 해상도 확인
            int resolution = sensors.getResolution(sensorAddresses[i]);
            Serial.printf("  해상도: %d비트 (%.4f°C 단위)\n",
                          resolution,
                          getResolutionStep(resolution));
        } else {
            Serial.printf("센서 #%d: 주소 읽기 실패!\n", i + 1);
        }
    }

    Serial.println();
    Serial.println("─── 온도 측정 시작 ───");
    Serial.printf("읽기 간격: %lu ms\n", READ_INTERVAL);
    Serial.println();

    // 비동기 변환 모드 설정 (선택사항)
    // true = requestTemperatures()가 변환 완료까지 대기 (blocking)
    // false = 즉시 반환, 나중에 값 읽기 (non-blocking)
    sensors.setWaitForConversion(true);
}

// ─────────────── 메인 루프 ───────────────
void loop() {
    unsigned long currentTime = millis();

    // 일정 간격으로 온도 읽기
    if (currentTime - lastReadTime >= READ_INTERVAL) {
        lastReadTime = currentTime;
        readCount++;

        // ──── 모든 센서에 온도 변환 요청 ────
        // 이 명령은 버스의 모든 DS18B20에 동시에 변환을 요청합니다
        unsigned long convStart = millis();
        sensors.requestTemperatures();
        unsigned long convTime = millis() - convStart;

        // ──── 각 센서의 온도 읽기 ────
        Serial.printf("─── 측정 #%lu (변환 시간: %lu ms) ───\n",
                      readCount, convTime);

        for (int i = 0; i < sensorCount && i < MAX_SENSORS; i++) {
            // 특정 센서의 온도 읽기 (주소 지정 방식)
            float tempC = sensors.getTempC(sensorAddresses[i]);

            // 센서 연결 해제 확인
            // DEVICE_DISCONNECTED_C = -127°C
            if (tempC == DEVICE_DISCONNECTED_C) {
                Serial.printf("  센서 #%d: *** 연결 끊김! ***\n", i + 1);
                continue;
            }

            // 섭씨 → 화씨 변환
            float tempF = DallasTemperature::toFahrenheit(tempC);

            // 최소/최대 온도 갱신
            if (tempC < minTemp) minTemp = tempC;
            if (tempC > maxTemp) maxTemp = tempC;

            // 온도 출력
            Serial.printf("  센서 #%d: %.2f°C (%.2f°F)", i + 1, tempC, tempF);

            // 온도 범위에 따른 상태 표시
            if (tempC < 0) {
                Serial.print("  [영하]");
            } else if (tempC < 10) {
                Serial.print("  [추움]");
            } else if (tempC < 25) {
                Serial.print("  [쾌적]");
            } else if (tempC < 35) {
                Serial.print("  [따뜻]");
            } else {
                Serial.print("  [고온]");
            }

            // 간단한 막대 그래프 (0~50°C 범위)
            Serial.print("  ");
            printTempBar(tempC);
            Serial.println();
        }

        // ──── 통계 출력 (센서가 1개 이상일 때) ────
        if (readCount % 5 == 0) {  // 5회마다 통계 표시
            Serial.println();
            Serial.printf("  [통계] 최저: %.2f°C | 최고: %.2f°C | "
                          "측정 횟수: %lu\n",
                          minTemp, maxTemp, readCount);
        }

        Serial.println();
    }
}

// ─────────────── 센서 주소 출력 함수 ───────────────
/**
 * DS18B20의 64비트 ROM 주소를 16진수로 출력하는 함수
 *
 * 주소 구조 (8바이트):
 * [0]: 패밀리 코드 (DS18B20 = 0x28)
 * [1~6]: 고유 일련번호
 * [7]: CRC 체크섬
 *
 * @param address  8바이트 장치 주소 배열
 */
void printAddress(DeviceAddress address) {
    Serial.print("주소: ");
    for (int i = 0; i < 8; i++) {
        // 1자리 16진수일 때 앞에 0 추가 (예: 0x0F → "0F")
        if (address[i] < 0x10) Serial.print("0");
        Serial.print(address[i], HEX);
        if (i < 7) Serial.print("-");
    }
    Serial.println();

    // 패밀리 코드로 센서 종류 확인
    Serial.print("  종류: ");
    switch (address[0]) {
        case 0x10: Serial.println("DS18S20 (구형)"); break;
        case 0x22: Serial.println("DS1822"); break;
        case 0x28: Serial.println("DS18B20"); break;
        case 0x3B: Serial.println("DS1825"); break;
        default:   Serial.println("알 수 없는 장치"); break;
    }
}

// ─────────────── 해상도별 단계 크기 반환 함수 ───────────────
/**
 * 해상도(비트)에 따른 온도 단계 크기를 반환하는 함수
 *
 * @param resolution  해상도 (9~12비트)
 * @return            온도 단계 크기 (°C)
 */
float getResolutionStep(int resolution) {
    switch (resolution) {
        case 9:  return 0.5;      // 9비트: 0.5°C 단위
        case 10: return 0.25;     // 10비트: 0.25°C 단위
        case 11: return 0.125;    // 11비트: 0.125°C 단위
        case 12: return 0.0625;   // 12비트: 0.0625°C 단위
        default: return 0.0625;
    }
}

// ─────────────── 온도 막대 그래프 함수 ───────────────
/**
 * 온도를 시각적 막대 그래프로 출력하는 함수
 * 0~50°C 범위를 20칸으로 표시합니다
 *
 * @param tempC  섭씨 온도
 */
void printTempBar(float tempC) {
    const int BAR_LENGTH = 20;
    float constrained = constrain(tempC, 0.0, 50.0);  // 0~50°C 범위로 제한
    int filled = (int)((constrained / 50.0) * BAR_LENGTH);

    Serial.print("[");
    for (int i = 0; i < BAR_LENGTH; i++) {
        if (i < filled) {
            Serial.print("=");  // 채워진 부분
        } else {
            Serial.print(" ");  // 빈 부분
        }
    }
    Serial.print("]");
}

/*
 * ============================================================
 *  DS18B20 주요 명령어 요약
 * ============================================================
 *
 *  sensors.begin()
 *    → 센서 라이브러리 초기화, 버스 스캔
 *
 *  sensors.getDeviceCount()
 *    → 연결된 센서 수 반환
 *
 *  sensors.getAddress(address, index)
 *    → index번째 센서의 주소를 address에 저장
 *
 *  sensors.setResolution(address, bits)
 *    → 특정 센서의 해상도 설정 (9~12비트)
 *
 *  sensors.requestTemperatures()
 *    → 모든 센서에 온도 변환 요청
 *
 *  sensors.getTempC(address)
 *    → 특정 센서의 온도를 섭씨로 반환
 *
 *  sensors.getTempCByIndex(index)
 *    → index번째 센서의 온도를 섭씨로 반환
 *
 *  sensors.isParasitePowerMode()
 *    → 기생 전원 모드 여부 확인
 *
 * ============================================================
 *  문제 해결 가이드
 * ============================================================
 *
 *  1. "센서를 찾지 못했습니다" 오류:
 *     - GND, DQ, VDD 핀 순서를 다시 확인하세요
 *     - 4.7kΩ 풀업 저항이 DQ와 3.3V 사이에 있는지 확인
 *     - 다른 GPIO 핀으로 변경해 보세요
 *
 *  2. 온도가 -127°C로 표시:
 *     - 센서 연결이 끊어졌거나 통신 오류
 *     - 배선 접촉 불량 확인
 *     - 풀업 저항 값이 올바른지 확인
 *
 *  3. 온도가 85°C로 표시:
 *     - 센서 초기값 (변환이 아직 완료되지 않음)
 *     - requestTemperatures() 후 충분히 기다렸는지 확인
 *     - setWaitForConversion(true) 설정 확인
 *
 *  4. 여러 센서 중 일부만 읽히지 않을 때:
 *     - 배선 길이가 너무 긴 경우 풀업 저항을 2.2kΩ으로 교체
 *     - 분기 배선보다 직렬 배선이 안정적
 *     - 전원 전류가 부족할 수 있음 → 외부 전원 사용
 *
 * ============================================================
 *  비동기 읽기 (Non-blocking) 예제
 * ============================================================
 *
 *  // 변환 완료를 기다리지 않는 방식
 *  // loop()에서 다른 작업을 하면서 온도를 읽을 수 있음
 *
 *  sensors.setWaitForConversion(false);  // 비동기 모드
 *
 *  void loop() {
 *      static unsigned long requestTime = 0;
 *      static bool requested = false;
 *
 *      if (!requested) {
 *          sensors.requestTemperatures();  // 즉시 반환
 *          requestTime = millis();
 *          requested = true;
 *      }
 *
 *      // 750ms 후 결과 읽기 (12비트 해상도)
 *      if (requested && (millis() - requestTime >= 750)) {
 *          float temp = sensors.getTempCByIndex(0);
 *          Serial.printf("온도: %.2f°C\n", temp);
 *          requested = false;
 *      }
 *
 *      // 여기서 다른 작업 수행 가능!
 *      doOtherStuff();
 *  }
 *
 * ============================================================
 */
