# 07. 1-Wire 프로토콜 - 단선 통신

## 개요

1-Wire는 Dallas Semiconductor(현 Maxim Integrated)에서 개발한 통신 프로토콜로,
**단 하나의 데이터 선**으로 여러 장치와 통신할 수 있습니다.
가장 대표적인 1-Wire 장치는 **DS18B20 온도 센서**입니다.

> **소프트웨어 비유**: 1-Wire는 **공유 버스 네트워크**와 비슷합니다.
> - 하나의 물리적 회선(데이터 선)에 여러 장치가 연결
> - 각 장치는 공장에서 부여된 **64비트 고유 주소**(MAC 주소와 유사)를 가짐
> - 마스터(ESP32)가 주소로 특정 장치를 선택하여 통신

---

## 1. 1-Wire 프로토콜 특징

### 기본 구조

```
         4.7kΩ 풀업 저항
  3.3V ────┤├──────┬─────────┬─────────┬────── DQ (데이터)
                   │         │         │
             ┌─────┴───┐ ┌───┴───┐ ┌───┴───┐
             │ DS18B20 │ │DS18B20│ │DS18B20│
             │  센서1  │ │ 센서2 │ │ 센서3 │
             └─────┬───┘ └───┬───┘ └───┬───┘
                   │         │         │
  GND  ────────────┴─────────┴─────────┴────── GND

  단 3개의 선만 필요:
  1. VCC (3.3V 또는 5V)
  2. DQ (데이터) - 모든 장치가 공유
  3. GND
```

### 1-Wire vs 다른 프로토콜

| 특성 | 1-Wire | I2C | SPI |
|------|--------|-----|-----|
| 데이터 선 수 | **1** | 2 | 4+ |
| 전선 총 수 | 3 (VCC, DQ, GND) | 4 (VCC, SDA, SCL, GND) | 6+ |
| 장치 주소 | 64비트 (공장 고유) | 7비트 (사용자 설정) | CS 핀 |
| 속도 | ~16 kbps | 100k~3.4M bps | ~80M bps |
| 최대 거리 | ~100m | ~1m | ~0.1m |
| 풀업 저항 | **4.7kΩ 필요** | 4.7kΩ 필요 | 불필요 |
| 주요 장치 | 온도 센서, iButton | 센서, 디스플레이 | SD카드, TFT |

### 통신 순서

```
    마스터(ESP32)                    슬레이브(DS18B20)
         │                                │
   1.    │──── 리셋 펄스 (480μs LOW) ────▶│
         │     "누구 있어?"                │
         │                                │
   2.    │◀──── 프레즌스 펄스 ─────────────│
         │     "네, 여기 있습니다!"         │
         │                                │
   3.    │──── ROM 명령 (주소 지정) ─────▶│
         │     "0x28FF1234... 장치!"       │
         │                                │
   4.    │──── 기능 명령 (온도 변환) ────▶│
         │     "온도 측정해줘!"             │
         │                                │
   5.    │     ~~~ 변환 대기 (750ms) ~~~   │
         │                                │
   6.    │──── 데이터 읽기 요청 ──────────▶│
         │                                │
   7.    │◀──── 온도 데이터 전송 ──────────│
         │     25.3°C                      │
```

---

## 2. 풀업 저항 (4.7kΩ)

1-Wire 데이터 라인은 **오픈 드레인** 방식으로 동작합니다.
풀업 저항이 없으면 HIGH 상태를 유지할 수 없습니다.

```
             4.7kΩ
  3.3V ──────┤├────── DQ (데이터 라인)
                │
          ┌─────┴─────┐
          │  DS18B20   │
          │            │
          │  내부 구조: │
          │  ┌───┐     │
          │  │FET│     │  ← LOW로 끌어내리기만 가능
          │  └─┬─┘     │
          │    │       │
          └────┴───────┘
               │
              GND

유휴 상태: 풀업에 의해 HIGH (3.3V)
통신 중:   장치가 FET으로 LOW (0V)로 끌어내림
```

### 저항값 선택 가이드

| 조건 | 권장 저항 | 이유 |
|------|----------|------|
| **일반적인 사용** | **4.7kΩ** | **표준 권장 값** |
| 긴 케이블 (>10m) | 2.2kΩ~3.3kΩ | 신호 강도 보강 |
| 짧은 케이블 (<1m) | 4.7kΩ~10kΩ | 기본 값으로 충분 |
| 기생 전원 모드 | 2.2kΩ | 전류 공급 능력 필요 |
| 많은 장치 (>10개) | 2.2kΩ | 여러 장치의 부하 감당 |

---

## 3. DS18B20 온도 센서

### DS18B20 사양

| 특성 | 값 |
|------|-----|
| 측정 범위 | -55°C ~ +125°C |
| 정확도 | ±0.5°C (-10°C ~ +85°C 범위) |
| 해상도 | 9~12비트 (설정 가능) |
| 변환 시간 | 93.75ms (9비트) ~ 750ms (12비트) |
| 전원 전압 | 3.0V ~ 5.5V |
| 통신 | 1-Wire |
| 고유 주소 | 64비트 (공장에서 부여) |

### DS18B20 핀 배치

```
  DS18B20 (TO-92 패키지)

     ┌─────────┐
     │  정면   │
     │ (평면)  │
     └─┬──┬──┬─┘
       │  │  │
       1  2  3

  핀 1: GND
  핀 2: DQ (데이터)
  핀 3: VDD (3.3V~5V)

  ※ 방향 주의! 잘못 연결하면 발열 및 손상!
```

### 회로 연결 (일반 전원 모드)

```
  3.3V ───────────────────┬─────── VDD (핀 3)
                          │
                        [4.7kΩ]
                          │
  GPIO 4 ─────────────────┼─────── DQ  (핀 2)
                          │
  GND ────────────────────┴─────── GND (핀 1)
```

### 여러 센서 연결 (같은 데이터 라인)

```
  3.3V ──────────────────┬────── VDD
                         │
                       [4.7kΩ]
                         │
  GPIO 4 ────────────────┼────── DQ
                    ┌────┤────┐
              ┌─────┴──┐ │ ┌──┴─────┐
              │DS18B20 │ │ │DS18B20 │
              │ 센서1  │ │ │ 센서2  │
              │주소:   │ │ │주소:   │
              │28-FF-  │ │ │28-AA-  │
              │12-34...│ │ │56-78...│
              └─────┬──┘ │ └──┬─────┘
                    │    │    │
  GND ──────────────┴────┴────┴────── GND

  ※ 각 DS18B20은 공장에서 부여된 64비트 고유 주소를 가짐
  ※ 동일한 DQ 라인에 이론적으로 수십 개 연결 가능
```

### 기본 코드 (단일 센서)

```cpp
#include <OneWire.h>
#include <DallasTemperature.h>

const int ONE_WIRE_BUS = 4;  // GPIO 4

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
    Serial.begin(115200);
    sensors.begin();

    Serial.print("발견된 센서 수: ");
    Serial.println(sensors.getDeviceCount());
}

void loop() {
    sensors.requestTemperatures();  // 온도 변환 요청

    float tempC = sensors.getTempCByIndex(0);  // 첫 번째 센서

    if (tempC != DEVICE_DISCONNECTED_C) {
        Serial.print("온도: ");
        Serial.print(tempC, 1);
        Serial.println("°C");
    } else {
        Serial.println("센서 읽기 오류!");
    }

    delay(1000);
}
```

### 여러 센서 코드 (주소 지정)

```cpp
#include <OneWire.h>
#include <DallasTemperature.h>

const int ONE_WIRE_BUS = 4;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// 센서 주소 저장 (I2C 스캐너처럼 먼저 스캔해야 함)
DeviceAddress sensor1, sensor2;

void setup() {
    Serial.begin(115200);
    sensors.begin();

    // 연결된 센서 주소 검색
    if (sensors.getAddress(sensor1, 0)) {
        Serial.print("센서 1 주소: ");
        printAddress(sensor1);
    }
    if (sensors.getAddress(sensor2, 1)) {
        Serial.print("센서 2 주소: ");
        printAddress(sensor2);
    }

    // 해상도 설정 (9~12비트)
    sensors.setResolution(sensor1, 12);  // 12비트 (가장 정밀)
    sensors.setResolution(sensor2, 12);
}

void loop() {
    sensors.requestTemperatures();

    Serial.print("센서 1: ");
    Serial.print(sensors.getTempC(sensor1), 2);
    Serial.print("°C  |  센서 2: ");
    Serial.print(sensors.getTempC(sensor2), 2);
    Serial.println("°C");

    delay(1000);
}

// 주소를 16진수로 출력하는 함수
void printAddress(DeviceAddress address) {
    for (int i = 0; i < 8; i++) {
        if (address[i] < 16) Serial.print("0");
        Serial.print(address[i], HEX);
        if (i < 7) Serial.print("-");
    }
    Serial.println();
}
```

---

## 4. 기생 전원 (Parasite Power) 모드

DS18B20은 **VDD 핀 없이 데이터 라인(DQ)에서 전원을 공급받을 수** 있습니다.
이를 **기생 전원 모드**라고 합니다.

### 일반 전원 vs 기생 전원

```
[일반 전원 모드 - 3선]              [기생 전원 모드 - 2선]

  3.3V ──── VDD                     3.3V
             │                        │
           [4.7kΩ]                  [4.7kΩ] (더 낮은 값 권장: 2.2kΩ)
             │                        │
  GPIO ───── DQ                     GPIO ─── DQ ─── VDD
             │                               │
  GND ────── GND                    GND ──── GND

  전선 3개: VCC, DQ, GND            전선 2개: DQ, GND
  안정적인 전원 공급                 전원 선 절약, 긴 배선에 유리
                                    온도 변환 중 전류 부족 가능성
```

### 기생 전원 동작 원리

```
┌────────────────────────────────────────┐
│          DS18B20 내부 구조             │
│                                        │
│  DQ ─────┬───────────── 통신           │
│          │                             │
│         [다이오드]                      │
│          │                             │
│       ┌──┴──┐                          │
│       │내부 │ ← DQ가 HIGH일 때         │
│       │커패 │    충전됨                 │
│       │시터 │                          │
│       └──┬──┘                          │
│          │                             │
│  GND ────┘                             │
│                                        │
│  VDD ──── GND에 연결 (기생 모드)       │
└────────────────────────────────────────┘

DQ가 HIGH → 내부 커패시터 충전 → 이 에너지로 동작
DQ가 LOW → 통신 중 → 저장된 에너지 사용
```

### 기생 전원 모드 코드

```cpp
#include <OneWire.h>
#include <DallasTemperature.h>

const int ONE_WIRE_BUS = 4;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
    Serial.begin(115200);
    sensors.begin();

    // 기생 전원 모드 확인
    if (sensors.isParasitePowerMode()) {
        Serial.println("기생 전원 모드 감지!");
        Serial.println("변환 중 강한 풀업이 필요합니다.");
    } else {
        Serial.println("일반 전원 모드");
    }

    // 기생 전원 모드에서는 비동기 변환 비활성화 권장
    sensors.setWaitForConversion(true);
}

void loop() {
    sensors.requestTemperatures();
    // 기생 전원 모드에서는 변환 완료까지 대기가 중요

    float temp = sensors.getTempCByIndex(0);
    Serial.printf("온도: %.2f°C\n", temp);

    delay(1000);
}
```

### 기생 전원 모드 주의사항

| 항목 | 일반 전원 | 기생 전원 |
|------|----------|----------|
| 전선 수 | 3개 | 2개 |
| 안정성 | 높음 | 온도 변환 중 불안정 가능 |
| 풀업 저항 | 4.7kΩ | **2.2kΩ 권장** |
| 동시 측정 | 여러 센서 동시 가능 | 전류 부족 가능성 |
| 최대 온도 | 125°C | **100°C 이하 권장** |
| 케이블 길이 | ~100m | ~50m (전류 제한) |

> **권장사항**: 가능하면 **일반 전원 모드**를 사용하세요.
> 기생 전원은 배선을 줄일 수 있지만 안정성이 떨어질 수 있습니다.

---

## 5. 해상도 설정

| 해상도 | 단계 | 변환 시간 | 정밀도 |
|--------|------|----------|--------|
| 9비트 | 0.5°C | 93.75ms | 낮음 |
| 10비트 | 0.25°C | 187.5ms | 중간 |
| 11비트 | 0.125°C | 375ms | 높음 |
| **12비트** | **0.0625°C** | **750ms** | **최대 (기본값)** |

```cpp
// 해상도 설정
sensors.setResolution(12);  // 전체 센서
// 또는
sensors.setResolution(sensorAddress, 12);  // 특정 센서
```

---

## 6. 1-Wire 장치 검색 (스캐너)

```cpp
#include <OneWire.h>

const int ONE_WIRE_BUS = 4;
OneWire oneWire(ONE_WIRE_BUS);

void setup() {
    Serial.begin(115200);
    Serial.println("1-Wire 장치 스캐너");
    Serial.println("==================");
}

void loop() {
    byte addr[8];
    int count = 0;

    oneWire.reset_search();

    while (oneWire.search(addr)) {
        count++;
        Serial.printf("장치 %d 발견! 주소: ", count);

        for (int i = 0; i < 8; i++) {
            if (addr[i] < 16) Serial.print("0");
            Serial.print(addr[i], HEX);
            if (i < 7) Serial.print("-");
        }

        // 패밀리 코드로 장치 종류 확인
        Serial.print("  (");
        switch (addr[0]) {
            case 0x10: Serial.print("DS18S20"); break;
            case 0x22: Serial.print("DS1822");  break;
            case 0x28: Serial.print("DS18B20"); break;
            case 0x3B: Serial.print("DS1825");  break;
            default:   Serial.print("알 수 없음");
        }
        Serial.println(")");

        // CRC 검증
        if (OneWire::crc8(addr, 7) != addr[7]) {
            Serial.println("  ※ CRC 오류! 배선을 확인하세요.");
        }
    }

    if (count == 0) {
        Serial.println("장치를 찾지 못했습니다.");
        Serial.println("  - 배선을 확인하세요.");
        Serial.println("  - 4.7kΩ 풀업 저항을 확인하세요.");
    }

    Serial.println("---");
    delay(5000);
}
```

---

## 핵심 정리

| 개념 | 설명 |
|------|------|
| **1-Wire** | 단일 데이터 선으로 여러 장치와 통신하는 프로토콜 |
| **DQ** | 데이터 라인 (풀업 저항 4.7kΩ 필수) |
| **64비트 주소** | 각 장치의 공장 부여 고유 주소 (ROM 코드) |
| **DS18B20** | 대표적인 1-Wire 온도 센서 (-55°C~+125°C) |
| **해상도** | 9~12비트 설정 가능 (12비트 기본, 750ms 변환) |
| **기생 전원** | VDD 없이 DQ에서 전원 공급받는 모드 (2선으로 동작) |
| **OneWire 라이브러리** | 1-Wire 프로토콜 기본 통신 라이브러리 |
| **DallasTemperature** | DS18B20 전용 상위 라이브러리 |

---

## 실습 과제

1. DS18B20 1개를 연결하여 온도를 시리얼 모니터에 출력하기
2. 1-Wire 스캐너로 센서 주소 확인하기
3. DS18B20 2개를 같은 데이터 라인에 연결하여 동시 측정
4. 해상도를 9비트/12비트로 변경하며 변환 시간 비교하기
5. (도전) 온도 데이터를 OLED에 표시하기 (I2C + 1-Wire 결합)

---

## 관련 코드 예제

- [onewire_ds18b20.ino](./code/onewire_ds18b20.ino) - DS18B20 온도 센서 읽기 전체 코드
