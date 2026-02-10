# 05. SPI (Serial Peripheral Interface) - 직렬 주변장치 인터페이스

## 개요

SPI는 **고속 전이중(Full-Duplex) 직렬 통신** 프로토콜입니다.
I2C보다 **더 빠르지만** 핀을 **더 많이** 사용합니다.
SD 카드, TFT 디스플레이, 플래시 메모리 등 대량 데이터 전송에 적합합니다.

> **소프트웨어 비유**: SPI는 **전이중 TCP 소켓**과 비슷합니다.
> - 동시에 송수신 가능 (전이중)
> - 각 클라이언트(슬레이브)마다 전용 연결(CS 핀)이 필요
> - I2C(HTTP)보다 빠르지만 연결(핀) 자원을 더 많이 소비

---

## 1. SPI 동작 원리

### 4개의 신호선

```
┌──────────┐                          ┌──────────┐
│          │   MOSI (Master Out,      │          │
│          │   Slave In)              │          │
│  ESP32   │─────────────────────────▶│  슬레이브 │
│ (마스터) │                          │  장치    │
│          │   MISO (Master In,       │          │
│          │   Slave Out)             │          │
│          │◀─────────────────────────│          │
│          │                          │          │
│          │   SCK (Serial Clock)     │          │
│          │─────────────────────────▶│          │
│          │                          │          │
│          │   CS (Chip Select)       │          │
│          │─────────────────────────▶│          │
│          │   LOW = 이 장치 선택     │          │
└──────────┘                          └──────────┘
```

| 신호 | 방향 | 설명 |
|------|------|------|
| **MOSI** | 마스터 → 슬레이브 | Master Out Slave In: 마스터가 보내는 데이터 |
| **MISO** | 슬레이브 → 마스터 | Master In Slave Out: 슬레이브가 보내는 데이터 |
| **SCK** | 마스터 → 슬레이브 | Serial Clock: 동기화 클럭 신호 |
| **CS/SS** | 마스터 → 슬레이브 | Chip Select: 통신할 장치 선택 (LOW = 활성) |

### 통신 흐름

```
CS   ────┐                              ┌────────
         └──────────────────────────────┘
         ↓ 장치 선택 (LOW)          해제 (HIGH)

SCK  ────┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌──┐  ┌────
         └──┘  └──┘  └──┘  └──┘  └──┘  └──┘  └──┘  └──┘

MOSI ──┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬──
       │ D7  │ D6  │ D5  │ D4  │ D3  │ D2  │ D1  │ D0  │
       └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
        마스터 → 슬레이브 데이터 (동시 전송!)

MISO ──┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬──
       │ D7  │ D6  │ D5  │ D4  │ D3  │ D2  │ D1  │ D0  │
       └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
        슬레이브 → 마스터 데이터 (동시 전송!)

※ MOSI와 MISO가 동시에 데이터를 전송 = 전이중(Full-Duplex)
```

---

## 2. I2C와 SPI 비교

| 특성 | I2C | SPI |
|------|-----|-----|
| **필요 핀 수** | 2 (SDA, SCL) | 4+ (MOSI, MISO, SCK, CS) |
| **속도** | 100kHz ~ 3.4MHz | 최대 80MHz |
| **통신 방식** | 반이중 (Half-Duplex) | **전이중 (Full-Duplex)** |
| **장치 선택** | 주소 기반 (소프트웨어) | CS 핀 (하드웨어) |
| **풀업 저항** | 필요 (4.7kΩ) | 불필요 |
| **장치 수** | 주소로 최대 127개 | CS 핀 수만큼 |
| **배선 복잡도** | 간단 | 복잡 |
| **적합한 용도** | 센서, 소형 디스플레이 | SD카드, TFT, 고속 통신 |

### 언제 어떤 것을 사용할까?

```
┌───────────────────────────────────────────────────────┐
│                  프로토콜 선택 가이드                   │
│                                                       │
│  데이터 전송량이 많은가?                               │
│      ├── 예 → SPI 사용 (SD카드, TFT 디스플레이)       │
│      └── 아니오                                       │
│              │                                        │
│              핀이 부족한가?                            │
│              ├── 예 → I2C 사용 (2핀으로 여러 장치)    │
│              └── 아니오                               │
│                      │                                │
│                      장치가 많은가?                    │
│                      ├── 예 → I2C 사용 (주소 기반)    │
│                      └── 아니오 → 어느 것이든 OK      │
└───────────────────────────────────────────────────────┘
```

---

## 3. ESP32 SPI 핀 설정

ESP32에는 **4개의 SPI 인터페이스**가 있습니다:

| SPI 버스 | MOSI | MISO | SCK | CS | 비고 |
|---------|------|------|-----|----|------|
| SPI0 | - | - | - | - | 내부 플래시 전용 (사용 불가) |
| SPI1 | - | - | - | - | 내부 플래시 전용 (사용 불가) |
| **HSPI** | **GPIO 13** | **GPIO 12** | **GPIO 14** | **GPIO 15** | 사용 가능 |
| **VSPI** | **GPIO 23** | **GPIO 19** | **GPIO 18** | **GPIO 5** | 사용 가능 (기본) |

### 기본 VSPI 핀 (가장 많이 사용)

```
ESP32 핀          기능
─────────        ─────
GPIO 23    ───── MOSI (데이터 출력)
GPIO 19    ───── MISO (데이터 입력)
GPIO 18    ───── SCK  (클럭)
GPIO 5     ───── CS   (칩 선택)
```

### 사용자 정의 핀

```cpp
#include <SPI.h>

// 기본 VSPI 핀 사용
SPIClass vspi(VSPI);
vspi.begin();  // GPIO 18(SCK), 19(MISO), 23(MOSI), 5(CS)

// 사용자 정의 핀
SPIClass hspi(HSPI);
hspi.begin(14, 12, 13, 15);  // SCK, MISO, MOSI, CS
```

---

## 4. 여러 SPI 장치 연결

SPI에서 **여러 장치를 연결할 때는 각 장치마다 별도의 CS 핀**이 필요합니다.
MOSI, MISO, SCK는 모든 장치가 공유합니다.

```
                    ESP32
                   ┌──────┐
          GPIO 23 ─┤ MOSI ├──────────┬──────────── MOSI
          GPIO 19 ─┤ MISO ├──────────┼──────────── MISO
          GPIO 18 ─┤ SCK  ├──────────┼──────────── SCK
           GPIO 5 ─┤ CS0  ├────┐     │
           GPIO 4 ─┤ CS1  ├──┐ │     │
          GPIO 15 ─┤ CS2  ├┐ │ │     │
                   └──────┘│ │ │     │
                           │ │ │     │
                    ┌──────┤ │ │     │
                    │      │ │ └─────┼── CS ─ SD카드 모듈
                    │      │ │       │
                    │      │ └───────┼── CS ─ TFT 디스플레이
                    │      │         │
                    │      └─────────┼── CS ─ SPI 플래시
                    │                │
                    └────────────────┘
                   (MOSI/MISO/SCK 공유)

CS LOW  = 해당 장치 선택 (통신 활성)
CS HIGH = 해당 장치 해제 (통신 비활성)
```

```cpp
const int CS_SD = 5;
const int CS_TFT = 4;
const int CS_FLASH = 15;

void setup() {
    pinMode(CS_SD, OUTPUT);
    pinMode(CS_TFT, OUTPUT);
    pinMode(CS_FLASH, OUTPUT);

    // 모든 CS를 HIGH로 (비활성)
    digitalWrite(CS_SD, HIGH);
    digitalWrite(CS_TFT, HIGH);
    digitalWrite(CS_FLASH, HIGH);

    SPI.begin();
}

void readSD() {
    digitalWrite(CS_SD, LOW);    // SD카드 선택
    // ... SD카드 통신 ...
    digitalWrite(CS_SD, HIGH);   // SD카드 해제
}

void writeTFT() {
    digitalWrite(CS_TFT, LOW);   // TFT 선택
    // ... TFT 통신 ...
    digitalWrite(CS_TFT, HIGH);  // TFT 해제
}
```

---

## 5. 대표 SPI 장치

### 5.1 MicroSD 카드 모듈

```
ESP32                MicroSD 모듈
─────                ────────────
3.3V ────────────── VCC (3.3V)
GND  ────────────── GND
GPIO 23 (MOSI) ──── MOSI (DI)
GPIO 19 (MISO) ──── MISO (DO)
GPIO 18 (SCK)  ──── SCK (CLK)
GPIO 5  (CS)   ──── CS (CD)
```

```cpp
#include <SPI.h>
#include <SD.h>

const int CS_PIN = 5;

void setup() {
    Serial.begin(115200);

    if (!SD.begin(CS_PIN)) {
        Serial.println("SD 카드 초기화 실패!");
        return;
    }
    Serial.println("SD 카드 초기화 성공!");

    // 파일 쓰기
    File file = SD.open("/data.txt", FILE_WRITE);
    if (file) {
        file.println("온도: 25.3°C");
        file.println("습도: 60.5%");
        file.close();
        Serial.println("파일 쓰기 완료!");
    }

    // 파일 읽기
    file = SD.open("/data.txt");
    if (file) {
        while (file.available()) {
            Serial.write(file.read());
        }
        file.close();
    }
}
```

### 5.2 TFT 디스플레이 (ILI9341, ST7789 등)

```
ESP32                TFT 디스플레이
─────                ──────────────
3.3V ────────────── VCC
GND  ────────────── GND
GPIO 23 (MOSI) ──── SDA (MOSI)
GPIO 18 (SCK)  ──── SCL (SCK)
GPIO 5  (CS)   ──── CS
GPIO 4         ──── DC (Data/Command)
GPIO 2         ──── RST (Reset)
GPIO 15        ──── BL (Backlight)
```

```cpp
#include <SPI.h>
#include <TFT_eSPI.h>  // 또는 Adafruit_ILI9341

TFT_eSPI tft = TFT_eSPI();

void setup() {
    tft.init();
    tft.setRotation(1);          // 가로 방향
    tft.fillScreen(TFT_BLACK);   // 검은 배경
    tft.setTextColor(TFT_WHITE); // 흰 글씨
    tft.setTextSize(2);
    tft.drawString("Hello ESP32!", 10, 10);
}
```

---

## 6. SPI 모드 (CPOL, CPHA)

SPI에는 **4가지 동작 모드**가 있으며, 클럭의 극성(CPOL)과 위상(CPHA)으로 결정됩니다.

### 클럭 극성(CPOL)과 위상(CPHA)

```
CPOL = 0: 클럭 기본 상태가 LOW
CPOL = 1: 클럭 기본 상태가 HIGH

CPHA = 0: 클럭의 첫 번째 엣지에서 데이터 샘플링
CPHA = 1: 클럭의 두 번째 엣지에서 데이터 샘플링
```

### 4가지 SPI 모드

```
[모드 0: CPOL=0, CPHA=0]  (가장 일반적)
SCK  ──┐  ┌──┐  ┌──┐  ┌──
       └──┘  └──┘  └──┘
DATA ──X─────X─────X─────X──
       ↑     ↑     ↑     ↑
    상승엣지에서 샘플링

[모드 1: CPOL=0, CPHA=1]
SCK  ──┐  ┌──┐  ┌──┐  ┌──
       └──┘  └──┘  └──┘
DATA ────X─────X─────X─────X
         ↑     ↑     ↑     ↑
      하강엣지에서 샘플링

[모드 2: CPOL=1, CPHA=0]
SCK  ──┘  └──┘  └──┘  └──┘
       ┌──┐  ┌──┐  ┌──┐
DATA ──X─────X─────X─────X──
       ↑     ↑     ↑     ↑
    하강엣지에서 샘플링

[모드 3: CPOL=1, CPHA=1]
SCK  ──┘  └──┘  └──┘  └──┘
       ┌──┐  ┌──┐  ┌──┐
DATA ────X─────X─────X─────X
         ↑     ↑     ↑     ↑
      상승엣지에서 샘플링
```

| 모드 | CPOL | CPHA | 대표 장치 |
|------|------|------|----------|
| **모드 0** | 0 | 0 | **대부분의 장치** (SD카드, 대부분의 센서) |
| 모드 1 | 0 | 1 | 일부 ADC, DAC |
| 모드 2 | 1 | 0 | 거의 사용하지 않음 |
| 모드 3 | 1 | 1 | MAX31855 (열전대), 일부 가속도계 |

> **팁**: 대부분의 SPI 장치는 **모드 0**을 사용합니다.
> 장치의 데이터시트를 확인하여 올바른 모드를 설정하세요.

### 모드 설정 코드

```cpp
#include <SPI.h>

// SPI 설정 객체 생성
SPISettings settings(1000000, MSBFIRST, SPI_MODE0);
// 1MHz 클럭, MSB 먼저, 모드 0

void sendData(byte data) {
    SPI.beginTransaction(settings);
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(data);  // 1바이트 송수신
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
}
```

---

## 7. SPI 트랜잭션 API

```cpp
#include <SPI.h>

const int CS_PIN = 5;

void setup() {
    SPI.begin();
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH);
}

// 단일 바이트 전송
byte readByte(byte address) {
    byte result;
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_PIN, LOW);

    SPI.transfer(address | 0x80);  // 읽기 명령 (MSB=1)
    result = SPI.transfer(0x00);   // 더미 바이트 전송하면서 응답 수신

    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
    return result;
}

// 여러 바이트 전송
void readBytes(byte address, byte* buffer, int length) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_PIN, LOW);

    SPI.transfer(address | 0x80);
    for (int i = 0; i < length; i++) {
        buffer[i] = SPI.transfer(0x00);
    }

    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
}
```

---

## 핵심 정리

| 개념 | 설명 |
|------|------|
| **SPI** | 고속 전이중 직렬 통신 (최대 80MHz) |
| **MOSI** | Master Out Slave In - 마스터 → 슬레이브 데이터 |
| **MISO** | Master In Slave Out - 슬레이브 → 마스터 데이터 |
| **SCK** | Serial Clock - 동기화 클럭 |
| **CS/SS** | Chip Select - 장치 선택 (LOW=활성) |
| **전이중** | MOSI와 MISO로 동시에 송수신 가능 |
| **VSPI** | ESP32 기본 SPI (GPIO 23, 19, 18, 5) |
| **HSPI** | ESP32 두 번째 SPI (GPIO 13, 12, 14, 15) |
| **SPI 모드** | CPOL/CPHA 조합으로 4가지 모드 (모드 0이 가장 일반적) |

---

## 실습 과제

1. SD 카드 모듈에 텍스트 파일 생성 및 읽기
2. SD 카드에 센서 데이터를 주기적으로 기록하기 (데이터 로거)
3. TFT 디스플레이에 도형 그리기 (원, 사각형, 선)
4. (도전) SD 카드의 이미지 파일을 TFT에 표시하기

---

## 관련 코드 예제

- [spi_sd_card.ino](./code/spi_sd_card.ino) - SD 카드 읽기/쓰기 전체 코드
