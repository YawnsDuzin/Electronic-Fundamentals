# 06. UART/Serial (Universal Asynchronous Receiver/Transmitter) - 범용 비동기 송수신기

## 개요

UART는 가장 **오래되고 단순한** 직렬 통신 방식 중 하나입니다.
두 장치 사이에 **TX(송신)과 RX(수신) 2개의 선**만으로 데이터를 주고받으며,
별도의 클럭 신호 없이 **약속된 속도(보레이트)**로 통신합니다.

> **소프트웨어 비유**: UART는 **WebSocket** 통신과 비슷합니다.
> - 점대점(Point-to-Point) 1:1 연결
> - 양쪽이 독립적으로 데이터를 보낼 수 있음 (전이중)
> - 연결 시 속도(보레이트)를 미리 합의해야 함 (핸드셰이크 없음)
> - Arduino의 `Serial.println()`이 바로 UART 통신!

---

## 1. UART 동작 원리

### TX/RX 교차 연결

```
    장치 A (ESP32)              장치 B (GPS 모듈)
    ┌──────────┐               ┌──────────┐
    │          │   TX ────────▶│ RX       │
    │          │               │          │
    │          │   RX ◀────────│ TX       │
    │          │               │          │
    │          │   GND ────────│ GND      │
    └──────────┘               └──────────┘

    ※ TX(송신)은 상대방의 RX(수신)에 연결!
    ※ GND는 반드시 공유해야 함!
```

> **주의**: TX↔TX, RX↔RX로 연결하면 통신이 되지 않습니다!
> 반드시 **교차 연결** (TX→RX, RX→TX)해야 합니다.

### UART 데이터 프레임 구조

```
유휴 ──┐   ┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐   ┌── 유휴
(HIGH) │   │  │  │  │  │  │  │  │  │  │  │   │  (HIGH)
       └───┘  │  │  │  │  │  │  │  │  │  └───┘
        ↑     └──┴──┴──┴──┴──┴──┴──┴──┘  ↑
      시작     D0 D1 D2 D3 D4 D5 D6 D7  정지
      비트      ◀──── 8비트 데이터 ────▶  비트

1 프레임 = 시작비트(1) + 데이터(8) + 정지비트(1) = 10비트
```

| 구성 요소 | 값 | 설명 |
|----------|-----|------|
| 유휴 상태 | HIGH | 통신이 없을 때 라인은 HIGH |
| 시작 비트 | LOW (1비트) | 데이터 전송 시작을 알림 |
| 데이터 비트 | 5~9비트 (보통 8비트) | 실제 전송할 데이터 |
| 패리티 비트 | 0 또는 1비트 (선택) | 오류 검출용 (잘 사용하지 않음) |
| 정지 비트 | HIGH (1~2비트) | 데이터 전송 완료를 알림 |

---

## 2. 보레이트 (Baud Rate)

보레이트는 **1초에 전송하는 비트 수**입니다.
양쪽 장치가 **같은 보레이트**를 사용해야 통신이 가능합니다.

### 일반적인 보레이트

| 보레이트 | 초당 바이트 | 용도 |
|---------|-----------|------|
| 9600 | ~960 B/s | GPS 모듈, 저속 센서 (기본 속도) |
| 19200 | ~1,920 B/s | 일부 장치 |
| 38400 | ~3,840 B/s | 일부 장치 |
| 57600 | ~5,760 B/s | 블루투스 모듈 |
| **115200** | **~11,520 B/s** | **ESP32 기본 (가장 많이 사용)** |
| 921600 | ~92,160 B/s | 고속 데이터 전송 |

> **보레이트 불일치 시 발생하는 현상**:
> ```
> 올바른 보레이트:  "Hello World!"
> 잘못된 보레이트:  "⸮⸮⸮⸮⸮⸮⸮⸮"  ← 알 수 없는 문자(깨진 글자)
> ```

### 보레이트와 실제 전송 속도 관계

```
115200 보레이트에서 실제 데이터 전송 속도:

115200 비트/초 ÷ 10 비트/프레임 = 11,520 바이트/초
(10비트 = 시작1 + 데이터8 + 정지1)

"Hello" (5글자) 전송 시간:
5 바이트 ÷ 11,520 바이트/초 ≈ 0.43 밀리초
```

---

## 3. ESP32 UART 포트

ESP32에는 **3개의 UART 포트**가 있습니다.

```
┌──────────────────────────────────────────────────────┐
│                    ESP32 UART 포트                     │
│                                                       │
│  ┌──────────────────────┐                            │
│  │   UART0              │                            │
│  │   TX = GPIO 1        │ ◀── USB 시리얼 연결       │
│  │   RX = GPIO 3        │     (디버깅/프로그래밍)    │
│  │                      │     Serial.println() 사용  │
│  └──────────────────────┘                            │
│                                                       │
│  ┌──────────────────────┐                            │
│  │   UART1              │                            │
│  │   TX = GPIO 10 (기본)│ ◀── 기본 핀이 플래시와    │
│  │   RX = GPIO 9  (기본)│     충돌! 핀 재할당 필요   │
│  │   ※ 핀 변경 필요!   │                            │
│  └──────────────────────┘                            │
│                                                       │
│  ┌──────────────────────┐                            │
│  │   UART2              │                            │
│  │   TX = GPIO 17       │ ◀── 외부 장치 연결에      │
│  │   RX = GPIO 16       │     가장 많이 사용         │
│  │   ※ 자유롭게 사용!  │                            │
│  └──────────────────────┘                            │
└──────────────────────────────────────────────────────┘
```

| UART | TX | RX | 용도 |
|------|----|----|------|
| **UART0** | GPIO 1 | GPIO 3 | USB 시리얼 (Serial) - 디버깅용 |
| **UART1** | 핀 재할당 필요 | 핀 재할당 필요 | 기본 핀이 플래시와 충돌 |
| **UART2** | GPIO 17 | GPIO 16 | 외부 장치 연결 (Serial2) |

### 각 UART 사용법

```cpp
void setup() {
    // UART0: USB 시리얼 (디버깅용)
    Serial.begin(115200);
    Serial.println("UART0: USB 시리얼 모니터");

    // UART2: 외부 장치 (GPS, 센서 등)
    Serial2.begin(9600, SERIAL_8N1, 16, 17);
    // 9600 보레이트, 8데이터비트, 패리티없음, 1정지비트
    // RX=GPIO 16, TX=GPIO 17

    // UART1: 핀 재할당하여 사용
    Serial1.begin(9600, SERIAL_8N1, 25, 26);
    // RX=GPIO 25, TX=GPIO 26 (다른 핀으로 변경)
}

void loop() {
    // UART2로 받은 데이터를 UART0(시리얼 모니터)에 출력
    if (Serial2.available()) {
        String data = Serial2.readStringUntil('\n');
        Serial.println("UART2 수신: " + data);
    }
}
```

---

## 4. Serial Monitor 디버깅

시리얼 모니터는 ESP32 개발에서 **가장 중요한 디버깅 도구**입니다.

### 기본 출력 함수

```cpp
void setup() {
    Serial.begin(115200);  // 보레이트 설정

    // 다양한 출력 방법
    Serial.println("줄바꿈 포함 출력");
    Serial.print("줄바꿈 없이 출력");
    Serial.printf("포맷 출력: 온도=%.1f°C, 습도=%d%%\n", 25.3, 60);

    // 숫자 출력 형식
    int value = 255;
    Serial.println(value);        // 10진수: 255
    Serial.println(value, HEX);   // 16진수: FF
    Serial.println(value, BIN);   // 2진수: 11111111
    Serial.println(value, OCT);   // 8진수: 377
}
```

### 시리얼 데이터 수신

```cpp
void loop() {
    // 시리얼 모니터에서 입력한 데이터 읽기
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();  // 앞뒤 공백/개행 제거

        Serial.print("받은 명령: ");
        Serial.println(input);

        if (input == "on") {
            digitalWrite(LED_PIN, HIGH);
            Serial.println("LED 켜짐!");
        } else if (input == "off") {
            digitalWrite(LED_PIN, LOW);
            Serial.println("LED 꺼짐!");
        }
    }
}
```

### 디버깅 팁

```cpp
// 1. 실행 시간 측정
unsigned long start = millis();
// ... 측정할 코드 ...
unsigned long elapsed = millis() - start;
Serial.printf("실행 시간: %lu ms\n", elapsed);

// 2. 변수 모니터링 (Serial Plotter 호환 형식)
Serial.printf("%d,%d,%d\n", sensorA, sensorB, sensorC);
// Arduino IDE의 Serial Plotter에서 그래프로 볼 수 있음!

// 3. 디버그 레벨 매크로
#define DEBUG 1
#if DEBUG
  #define DEBUG_PRINT(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)  // 릴리스 시 출력 없음
#endif
```

---

## 5. UART 장치 연결 예시

### 5.1 GPS 모듈 (NEO-6M)

```
ESP32                NEO-6M GPS
─────                ──────────
3.3V ────────────── VCC
GND  ────────────── GND
GPIO 16 (RX2) ───── TX     ← GPS의 TX → ESP32의 RX
GPIO 17 (TX2) ───── RX     ← ESP32의 TX → GPS의 RX
```

```cpp
void setup() {
    Serial.begin(115200);    // 디버깅용
    Serial2.begin(9600);     // GPS 기본 보레이트: 9600
}

void loop() {
    // GPS에서 NMEA 데이터 수신
    while (Serial2.available()) {
        char c = Serial2.read();
        Serial.print(c);  // 시리얼 모니터에 GPS 데이터 출력
    }
}

// GPS NMEA 출력 예시:
// $GPGGA,123456.00,3724.0000,N,12700.0000,E,1,08,0.9,545.4,M,...
// $GPRMC,123456.00,A,3724.0000,N,12700.0000,E,0.0,0.0,091224,,,A*XX
```

### TinyGPS++ 라이브러리 활용

```cpp
#include <TinyGPS++.h>

TinyGPSPlus gps;

void loop() {
    while (Serial2.available()) {
        gps.encode(Serial2.read());
    }

    if (gps.location.isUpdated()) {
        Serial.printf("위도: %.6f\n", gps.location.lat());
        Serial.printf("경도: %.6f\n", gps.location.lng());
        Serial.printf("고도: %.1f m\n", gps.altitude.meters());
        Serial.printf("속도: %.1f km/h\n", gps.speed.kmph());
        Serial.printf("위성 수: %d\n", gps.satellites.value());
    }
}
```

### 5.2 가스 센서 모듈 (UART 방식)

일부 가스 센서는 UART로 농도 데이터를 전송합니다.

```
ESP32                가스 센서 (MH-Z19B CO2)
─────                ──────────────────────
5V (VIN) ──────────── VIN
GND  ────────────────  GND
GPIO 16 (RX2) ─────── TX
GPIO 17 (TX2) ─────── RX
```

```cpp
void setup() {
    Serial.begin(115200);
    Serial2.begin(9600);   // MH-Z19B 보레이트: 9600
}

// CO2 농도 요청 명령
byte cmd[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

void readCO2() {
    Serial2.write(cmd, 9);  // 명령 전송
    delay(100);

    if (Serial2.available() >= 9) {
        byte response[9];
        Serial2.readBytes(response, 9);

        if (response[0] == 0xFF && response[1] == 0x86) {
            int co2 = response[2] * 256 + response[3];
            Serial.printf("CO2 농도: %d ppm\n", co2);
        }
    }
}
```

---

## 6. RS485 소개 (장거리 통신)

일반 UART(TTL)는 짧은 거리(~15m)에서만 안정적입니다.
**RS485**는 UART 신호를 차동 신호로 변환하여 **최대 1200m** 거리에서 통신이 가능합니다.

```
[일반 UART (TTL)]
ESP32 TX ──────── 짧은 거리 (<15m) ──────── 장치 RX
         0V / 3.3V 단일 신호


[RS485 (차동 신호)]
ESP32 TX ─── MAX485 ─── 긴 거리 (<1200m) ─── MAX485 ─── 장치 RX
              변환기    A(+) / B(-) 차동쌍     변환기

      ┌─────────┐    꼬인 페어 케이블     ┌─────────┐
      │ MAX485  │    ┌─────────────────┐  │ MAX485  │
TX ───┤ DI   A ─┼────┤ A ─────────── A ├──┼─ A   RO├──── RX
      │      B ─┼────┤ B ─────────── B ├──┼─ B     │
RX ───┤ RO  DE ─┼──┐ └─────────────────┘  │  DE  DI├──── TX
      │   RE  ─┼──┤                       │  RE    │
      └─────────┘  │                       └─────────┘
                   GPIO (송수신 전환)
```

### RS485 장점

| 특성 | TTL UART | RS485 |
|------|---------|-------|
| 최대 거리 | ~15m | **~1200m** |
| 노이즈 내성 | 약함 | **매우 강함** (차동 신호) |
| 장치 수 | 1:1 | **1:32** (멀티드롭) |
| 케이블 | 일반 | 꼬인 쌍선 (트위스트 페어) |
| 용도 | 보드 내부 | **산업용, 건물 자동화** |

---

## 핵심 정리

| 개념 | 설명 |
|------|------|
| **UART** | 클럭 없이 약속된 속도로 통신하는 비동기 직렬 통신 |
| **TX/RX** | 송신(TX)과 수신(RX) - 반드시 교차 연결 |
| **보레이트** | 통신 속도 (bps), 양쪽이 같아야 함 (ESP32 기본: 115200) |
| **UART0** | USB 시리얼 (Serial) - 디버깅, GPIO 1/3 |
| **UART1** | 핀 재할당 필요 (Serial1) - 기본 핀 충돌 |
| **UART2** | 외부 장치용 (Serial2) - GPIO 16(RX)/17(TX) |
| **8N1** | 8비트 데이터, 패리티 없음, 1 정지비트 (가장 일반적) |
| **RS485** | 장거리 (최대 1200m) 산업용 UART 변환 |

---

## 실습 과제

1. 시리얼 모니터에서 입력한 문자로 LED 제어하기 ("on"/"off")
2. GPS 모듈 연결하여 NMEA 데이터 출력하기
3. 두 개의 ESP32를 UART로 연결하여 메시지 주고받기
4. UART 수신 데이터를 파싱하여 센서 값 추출하기
5. (도전) 시리얼 모니터로 간단한 명령줄 인터페이스(CLI) 만들기

---

## 관련 코드 예제

- [uart_communication.ino](./code/uart_communication.ino) - UART2 외부 장치 통신 전체 코드
