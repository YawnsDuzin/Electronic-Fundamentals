# 06. 릴레이 (Relay)

## 목차
1. [릴레이란?](#1-릴레이란)
2. [릴레이의 원리](#2-릴레이의-원리)
3. [NO vs NC](#3-no-vs-nc)
4. [릴레이 모듈 사용법](#4-릴레이-모듈-사용법)
5. [ESP32로 릴레이 제어 시 주의점](#5-esp32로-릴레이-제어-시-주의점)
6. [고전압 장치 제어 시 안전 주의사항](#6-고전압-장치-제어-시-안전-주의사항)
7. [플라이백 다이오드 필요성](#7-플라이백-다이오드-필요성)
8. [실습: ESP32로 릴레이 제어하기](#8-실습-esp32로-릴레이-제어하기)
9. [핵심 정리](#핵심-정리)

---

## 1. 릴레이란?

릴레이(Relay)는 **전자석을 이용하여 물리적인 스위치를 제어하는 부품**입니다.
작은 전압(5V/12V)으로 큰 전압(AC 220V)의 장치를 ON/OFF 할 수 있습니다.

```
소프트웨어 비유:
릴레이는 "프록시(Proxy)" 또는 "미들웨어(Middleware)"와 같습니다.
- 저전력 시스템(ESP32)이 고전력 시스템(AC 220V)을 제어할 때
  중간에서 "다리" 역할을 합니다.
- 두 시스템이 전기적으로 완전히 분리(격리)됩니다.

// 프록시 패턴과 유사
relay.control(lowVoltageSignal) → highVoltageSwitching
```

### MOSFET과 릴레이의 차이

| 특성 | MOSFET | 릴레이 |
|------|:------:|:------:|
| 제어 대상 | DC 전원 | **DC + AC 모두** |
| 전기적 격리 | 없음 | **완전 격리** |
| 스위칭 속도 | 매우 빠름 (μs) | 느림 (~10ms) |
| 수명 | 사실상 무한 | 기계적 수명 한계 (10만~100만 회) |
| 소음 | 없음 | "딸깍" 소리 |
| PWM 가능 | 가능 | **불가** (기계식이라 느림) |
| AC 220V 제어 | 불가 | **가능** |
| 소비 전류 | 거의 없음 | 코일 전류 필요 (~70mA) |

> **언제 릴레이를 쓰나?**
> - AC 220V 장치 (조명, 선풍기, 에어컨 등)를 제어할 때
> - 전기적으로 완전히 격리가 필요할 때
> - DC라도 양방향 전류가 필요할 때

---

## 2. 릴레이의 원리

### 내부 구조

```
    릴레이 내부 구조:

    ┌─────────────────────────────────────────┐
    │                                         │
    │   코일 (전자석)        스위치 접점        │
    │                                         │
    │   ┌─────────┐         COM ○──┐          │
    │   │ ┌─────┐ │              ┌─┤          │
    │   │ │코일  │ │         스프링│ │          │
    │   │ │(전자│ │              │ │          │
    │   │ │ 석) │ │         ┌───┘ │          │
    │   │ └─────┘ │    NO ○─┘     ○ NC       │
    │   │    ↕    │                           │
    │   │  가동철편 │    (코일 OFF 상태)         │
    │   └─────────┘    COM과 NC가 연결됨       │
    │                                         │
    └─────────────────────────────────────────┘

    코일에 전류 흐르면:
    ┌─────────────────────────────────────────┐
    │                                         │
    │   ┌─────────┐         COM ○──┐          │
    │   │ ┌─────┐ │              │ ┐          │
    │   │ │코일  │ │         가동 │ │          │
    │   │ │(ON) │◄──── 자석이     │ │          │
    │   │ │     │ │    철편을     └─┤          │
    │   │ └─────┘ │    당김   NO ○─┘  ○ NC    │
    │   │         │                           │
    │   └─────────┘    (코일 ON 상태)          │
    │                   COM과 NO가 연결됨       │
    └─────────────────────────────────────────┘
```

### 동작 순서

```
1. 코일에 전류 인가 (5V 또는 12V)
2. 코일이 전자석이 됨
3. 전자석이 가동 접점(철편)을 끌어당김
4. 스위치 접점이 전환됨 ("딸깍!")
5. COM-NO 연결 (또는 COM-NC 분리)
```

---

## 3. NO vs NC

### NO (Normally Open) - 상시 열림

```
코일 OFF (평상시):          코일 ON (동작 시):

    COM ○     ○ NO           COM ○─────○ NO
         (열림, 끊어짐)           (닫힘, 연결됨)

    → 전류 안 흐름              → 전류 흐름!
```

### NC (Normally Closed) - 상시 닫힘

```
코일 OFF (평상시):          코일 ON (동작 시):

    COM ○─────○ NC           COM ○     ○ NC
         (닫힘, 연결됨)           (열림, 끊어짐)

    → 전류 흐름!               → 전류 안 흐름
```

### 릴레이 핀 구성 (5핀 릴레이)

```
    릴레이 바닥면:

    ┌──────────────────────┐
    │                      │
    │  ○ 코일+   ○ 코일-   │ ← 코일 (제어 측, 저전압)
    │                      │
    │                      │
    │  ○ COM              │ ← 접점 (부하 측, 고전압 가능)
    │  ○ NO               │
    │  ○ NC               │
    │                      │
    └──────────────────────┘

    코일 측: ESP32 + 트랜지스터로 제어 (5V/12V)
    접점 측: 부하(AC 220V 조명 등) 연결
```

### 어떤 접점을 사용할까?

```
NO (Normally Open) 사용 - 가장 일반적:
┌─────────────────────────────────────────┐
│ ESP32가 명령 → 릴레이 ON → 장치 켜짐     │
│ ESP32 꺼짐   → 릴레이 OFF → 장치 꺼짐    │
│                                         │
│ 예: 조명 제어, 모터 제어, 펌프 제어        │
│ 안전: ESP32가 고장나면 장치가 꺼짐 → 안전! │
└─────────────────────────────────────────┘

NC (Normally Closed) 사용 - 특수한 경우:
┌─────────────────────────────────────────┐
│ ESP32가 명령 → 릴레이 ON → 장치 꺼짐     │
│ ESP32 꺼짐   → 릴레이 OFF → 장치 켜짐    │
│                                         │
│ 예: 비상 정지 회로, 안전 장치              │
│ 안전: ESP32가 고장나면 장치가 켜짐 상태 유지│
│ (냉장고, 환기팬 등 항상 켜져야 하는 장치)   │
└─────────────────────────────────────────┘
```

---

## 4. 릴레이 모듈 사용법

### 릴레이 모듈이란?

릴레이를 직접 사용하려면 트랜지스터, 다이오드, LED 등을 별도로 연결해야 합니다.
**릴레이 모듈**은 이 모든 것이 하나의 PCB에 통합된 모듈입니다.

```
릴레이 모듈 구성:

    ┌───────────────────────────────────────────────┐
    │                                               │
    │  ┌──────┐    ┌──────────┐                     │
    │  │옵토   │    │ 릴레이    │    COM  NO  NC      │
    │  │커플러 │───▶│ 구동      │───○────○───○       │
    │  │      │    │ 트랜지스터 │                     │
    │  └──────┘    └──────────┘                     │
    │      ↑              ↑                         │
    │   광 격리       플라이백                        │
    │   (안전!)      다이오드 내장                    │
    │                                               │
    │  ○ VCC   ○ GND   ○ IN                         │
    │  (5V)           (제어 신호)                     │
    │                                               │
    │  ● LED (동작 표시등)                            │
    │                                               │
    └───────────────────────────────────────────────┘
```

### 릴레이 모듈의 장점

```
직접 구성 vs 모듈:

직접 구성 (복잡):
    ESP32 → [저항] → [트랜지스터] → [플라이백 다이오드] → [릴레이]
    + LED 표시등도 별도로...

모듈 사용 (간단):
    ESP32 → [릴레이 모듈 IN 핀]
    끝! (트랜지스터, 다이오드, LED, 옵토커플러 모두 내장)
```

### 릴레이 모듈 연결 방법

```
    ESP32                   릴레이 모듈
    ┌──────────┐           ┌─────────────────────────┐
    │          │           │                         │
    │  3.3V    ├─── or ───┤ VCC (5V 권장)           │
    │  (또는   │           │                         │
    │   5V)    │           │         ┌─────────┐     │
    │          │           │    COM ─┤ 릴레이    │    │
    │  GPIO 25 ├───────────┤ IN  NO ─┤ (접점)   │    │
    │          │           │    NC ─┤          │    │
    │          │           │         └─────────┘     │
    │  GND     ├───────────┤ GND                     │
    │          │           │                         │
    └──────────┘           └─────────────────────────┘
```

### Active LOW vs Active HIGH

릴레이 모듈에 따라 제어 신호의 논리가 다릅니다:

```
Active HIGH (HIGH = ON):
    IN = HIGH (3.3V) → 릴레이 ON  (LED 켜짐)
    IN = LOW  (0V)   → 릴레이 OFF (LED 꺼짐)

Active LOW (LOW = ON):  ← 옵토커플러 내장 모듈에 많음!
    IN = LOW  (0V)   → 릴레이 ON  (LED 켜짐)
    IN = HIGH (3.3V) → 릴레이 OFF (LED 꺼짐)

    ※ 모듈에 따라 다르므로 반드시 확인하세요!
    ※ Active LOW인 경우 코드에서 논리를 반전해야 합니다.
```

---

## 5. ESP32로 릴레이 제어 시 주의점

### 주의점 1: 전원 분리

```
┌──────────────────────────────────────────────────┐
│                                                  │
│  ESP32의 3.3V 핀으로 릴레이 코일을 직접 구동하지   │
│  마세요! 전류가 부족합니다.                         │
│                                                  │
│  릴레이 코일 전류: ~70mA (5V 릴레이 기준)          │
│  ESP32 3.3V 핀 출력: ~50mA (최대)                 │
│                                                  │
│  해결:                                            │
│  1. 릴레이 모듈의 VCC를 5V(USB)에 연결             │
│  2. IN 핀만 ESP32 GPIO에 연결                     │
│  3. 옵토커플러 내장 모듈이면 3.3V로도 IN 구동 가능  │
│                                                  │
└──────────────────────────────────────────────────┘
```

```
권장 연결:

    USB 5V ─────────── 릴레이 모듈 VCC
    ESP32 GPIO 25 ──── 릴레이 모듈 IN
    ESP32 GND ──────── 릴레이 모듈 GND

    ※ 릴레이 전원을 ESP32의 VIN/5V 핀에서 가져와도 됩니다.
    ※ 여러 릴레이를 사용하면 별도 5V 전원 권장!
```

### 주의점 2: ESP32 부팅 시 GPIO 상태

```
┌──────────────────────────────────────────────────┐
│                                                  │
│  ESP32 부팅(리셋) 중에 일부 GPIO가 HIGH가 될 수    │
│  있습니다! 이로 인해 릴레이가 의도치 않게            │
│  잠깐 켜질 수 있습니다.                             │
│                                                  │
│  해결:                                            │
│  1. Active LOW 릴레이 모듈 사용 (부팅 시 HIGH가     │
│     되어도 릴레이 OFF 상태 유지)                    │
│  2. 부팅 시 안전한 GPIO 선택:                       │
│     - GPIO 4, 16, 17, 18, 19, 21, 22, 23, 25,   │
│       26, 27, 32, 33 등이 비교적 안전               │
│     - GPIO 0, 2, 5, 12, 15는 부팅 시 특수 동작     │
│  3. 풀다운/풀업 저항으로 기본 상태 확정              │
│                                                  │
└──────────────────────────────────────────────────┘
```

### 주의점 3: 역기전력 (릴레이 모듈이 아닌 경우)

```
릴레이를 직접 사용할 때 (모듈이 아닌 경우):
반드시 플라이백 다이오드를 코일에 병렬로 달아야 합니다!

릴레이 모듈을 사용할 때:
이미 내장되어 있으므로 별도로 필요 없음!
```

### 주의점 4: 다채널 릴레이의 전류

```
4채널 릴레이 모듈의 전류 소비:

    릴레이 1개: ~70mA
    4채널 동시 ON: 70mA × 4 = 280mA

    ESP32의 USB 전원(500mA)에서 모두 가져오면:
    ESP32 자체: ~200mA (WiFi 사용 시)
    릴레이 4개: ~280mA
    합계: ~480mA → USB 한계에 근접!

    해결: 릴레이 전용 5V 전원을 별도로 공급!
```

---

## 6. 고전압 장치 제어 시 안전 주의사항

```
┌──────────────────────────────────────────────────────┐
│                                                      │
│         ⚠ AC 220V 안전 경고 ⚠                        │
│                                                      │
│  AC 220V(가정용 전원)는 감전 시 사망할 수 있습니다!     │
│                                                      │
│  다음 규칙을 반드시 지키세요:                           │
│                                                      │
│  1. AC 배선 작업은 반드시 전원을 차단한 상태에서!        │
│                                                      │
│  2. AC 부분은 절연 처리 (열수축 튜브, 절연 테이프)      │
│                                                      │
│  3. AC 배선은 DC 배선과 물리적으로 분리                 │
│                                                      │
│  4. 적절한 정격의 릴레이 사용                           │
│     (AC 250V 10A 이상 권장)                           │
│                                                      │
│  5. 반드시 케이스(인클로저)에 넣어 사용                  │
│     절대 노출된 상태로 사용하지 마세요!                  │
│                                                      │
│  6. 퓨즈를 함께 사용하세요                              │
│                                                      │
│  7. 잘 모르겠으면 전문가(전기 기사)에게 문의!            │
│                                                      │
│  8. 초보자는 AC 220V 대신 DC 12V/5V로 먼저 연습!       │
│                                                      │
└──────────────────────────────────────────────────────┘
```

### 릴레이 정격 확인

```
릴레이 표기 예: "SRD-05VDC-SL-C"

    SRD:     제조사/시리즈
    05VDC:   코일 전압 5V DC
    SL:      사이즈
    C:       CO(Change Over) = COM/NO/NC 모두 있음

릴레이 접점 정격 (보통 릴레이 위에 표기):
    10A 250VAC    → AC 250V에서 최대 10A
    10A 30VDC     → DC 30V에서 최대 10A

    ※ AC와 DC 정격이 다름에 주의!
    ※ 실제 사용 시 정격의 60~80%로 사용 권장
```

### AC 220V 장치 연결 예시 (조명)

```
    ┌──── 릴레이 접점 ────┐
    │                     │
    │  AC 220V ──── COM   │
    │                     │
    │  조명(부하) ── NO    │     릴레이 ON → 조명 켜짐
    │                     │     릴레이 OFF → 조명 꺼짐
    │  AC Neutral ────────┤
    │  (중성선)     직접   │
    │             연결     │
    └─────────────────────┘

    ※ 릴레이는 AC의 Live(활선)에 연결!
    ※ Neutral(중성선)은 부하에 직접 연결!

    상세도:

    AC Live ─── 릴레이 COM
                릴레이 NO ─── 조명 한쪽
    AC Neutral ─────────────── 조명 다른쪽

    릴레이 코일:
    5V ───── 코일+
    GND ──── 코일- (트랜지스터/모듈 경유)
```

---

## 7. 플라이백 다이오드 필요성

### 릴레이 직접 사용 시 (모듈 아닌 경우)

```
릴레이 코일 = 인덕터(유도성 부하)

릴레이 OFF 순간:
    코일의 자기장이 소멸하면서 역기전력(Back-EMF) 발생!
    수십~수백 V의 전압 스파이크!
    → 제어 트랜지스터/MOSFET 파손!
    → 최악의 경우 ESP32까지 손상!
```

### 플라이백 다이오드 연결

```
    5V
     │
     ├──────────────┐
     │              │
   ┌─┴──────┐    ──▶|── ← 1N4007 또는 1N5819
   │ 릴레이  │      │     (코일과 역방향 병렬)
   │ 코일    │      │
   └─┬──────┘      │
     │              │
     ├──────────────┘
     │
   [트랜지스터/MOSFET Collector/Drain]
     │
   GND

   다이오드 방향 주의:
   - 캐소드(밴드 쪽) → VCC(5V)
   - 애노드 → 트랜지스터 쪽
   - 정상 동작 시에는 역방향이라 전류 안 흐름
   - OFF 순간 역기전력이 다이오드를 통해 순환 → 흡수!
```

### 릴레이 모듈을 사용하면?

```
┌─────────────────────────────────────────┐
│                                         │
│  릴레이 모듈에는 플라이백 다이오드가       │
│  이미 내장되어 있습니다!                   │
│                                         │
│  따라서 릴레이 모듈을 사용하면             │
│  별도의 다이오드를 달 필요가 없습니다.      │
│                                         │
│  ※ 하지만 릴레이를 직접 사용할 때는       │
│    반드시 플라이백 다이오드를 달아야 합니다! │
│                                         │
└─────────────────────────────────────────┘
```

---

## 8. 실습: ESP32로 릴레이 제어하기

### 실습 1: 릴레이 기본 ON/OFF

#### 부품

- ESP32 개발보드
- 1채널 릴레이 모듈 (5V, Active HIGH)
- 12V LED 또는 소형 DC 팬 (안전한 저전압 부하)
- 12V DC 전원
- 점퍼 와이어

#### 회로

```
    ESP32                 릴레이 모듈          부하
    ┌──────────┐         ┌────────────┐      ┌────┐
    │          │         │            │      │    │
    │  5V(USB) ├─────────┤ VCC        │      │12V │
    │          │         │            │      │LED │
    │  GPIO 25 ├─────────┤ IN     COM ├──┐   │    │
    │          │         │            │  │   └─┬──┘
    │          │         │        NO  ├──┼─────┘
    │          │         │            │  │
    │  GND     ├─────────┤ GND    NC  │  │
    │          │         │            │  │
    └──────────┘         └────────────┘  │
                                         │
    12V 전원(+) ─────────────────────────┘
    12V 전원(-) ─── 부하 다른쪽 ─── GND
```

#### 코드

```cpp
const int RELAY_PIN = 25;

void setup() {
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);  // 초기: OFF
    Serial.begin(115200);
    Serial.println("릴레이 제어 시작!");
}

void loop() {
    Serial.println("릴레이 ON (장치 켜짐)");
    digitalWrite(RELAY_PIN, HIGH);
    delay(5000);  // 5초 ON

    Serial.println("릴레이 OFF (장치 꺼짐)");
    digitalWrite(RELAY_PIN, LOW);
    delay(3000);  // 3초 OFF
}
```

### 실습 2: Active LOW 릴레이 모듈

```cpp
// Active LOW 모듈: LOW = ON, HIGH = OFF
const int RELAY_PIN = 25;

// 가독성을 위한 매크로 (논리 반전)
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

void setup() {
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_OFF);  // 초기: OFF
    Serial.begin(115200);
}

void loop() {
    Serial.println("릴레이 ON");
    digitalWrite(RELAY_PIN, RELAY_ON);
    delay(5000);

    Serial.println("릴레이 OFF");
    digitalWrite(RELAY_PIN, RELAY_OFF);
    delay(3000);
}
```

### 실습 3: 웹 서버로 릴레이 제어 (IoT)

```cpp
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "YourWiFi";
const char* password = "YourPassword";

const int RELAY_PIN = 25;
bool relayState = false;

WebServer server(80);

void handleRoot() {
    String html = "<html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body{font-family:sans-serif;text-align:center;padding:50px;}";
    html += ".btn{padding:20px 40px;font-size:24px;margin:10px;border:none;";
    html += "border-radius:10px;cursor:pointer;}";
    html += ".on{background:#4CAF50;color:white;}";
    html += ".off{background:#f44336;color:white;}";
    html += "</style></head><body>";
    html += "<h1>ESP32 릴레이 제어</h1>";
    html += "<p>현재 상태: ";
    html += relayState ? "<b style='color:green'>ON</b>" : "<b style='color:red'>OFF</b>";
    html += "</p>";
    html += "<a href='/on'><button class='btn on'>켜기</button></a>";
    html += "<a href='/off'><button class='btn off'>끄기</button></a>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleOn() {
    relayState = true;
    digitalWrite(RELAY_PIN, HIGH);
    server.sendHeader("Location", "/");
    server.send(303);
}

void handleOff() {
    relayState = false;
    digitalWrite(RELAY_PIN, LOW);
    server.sendHeader("Location", "/");
    server.send(303);
}

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi 연결됨!");
    Serial.print("IP 주소: ");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.on("/on", handleOn);
    server.on("/off", handleOff);
    server.begin();
}

void loop() {
    server.handleClient();
}
```

### 실습 4: 타이머로 릴레이 제어

```cpp
const int RELAY_PIN = 25;
const int BUTTON_PIN = 4;

unsigned long relayOnTime = 0;
const unsigned long AUTO_OFF_DELAY = 30000;  // 30초 후 자동 OFF
bool relayState = false;

void setup() {
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(RELAY_PIN, LOW);
    Serial.begin(115200);
}

void loop() {
    // 버튼으로 릴레이 토글
    if (digitalRead(BUTTON_PIN) == LOW) {
        delay(50);  // 디바운싱
        if (digitalRead(BUTTON_PIN) == LOW) {
            relayState = !relayState;
            digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);

            if (relayState) {
                relayOnTime = millis();
                Serial.println("릴레이 ON (30초 후 자동 OFF)");
            } else {
                Serial.println("릴레이 OFF (수동)");
            }

            while (digitalRead(BUTTON_PIN) == LOW);  // 버튼 뗄 때까지 대기
        }
    }

    // 자동 OFF (안전 타이머)
    if (relayState && (millis() - relayOnTime > AUTO_OFF_DELAY)) {
        relayState = false;
        digitalWrite(RELAY_PIN, LOW);
        Serial.println("릴레이 OFF (자동, 30초 경과)");
    }
}
```

---

## 핵심 정리

```
┌─────────────────────────────────────────────────────────┐
│                    릴레이 핵심 정리                        │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  1. 릴레이 = 전자석으로 물리적 스위치를 제어하는 부품      │
│     - AC 220V 제어 가능 (MOSFET은 DC만 가능)             │
│     - 전기적으로 완전히 격리됨 (안전!)                     │
│                                                         │
│  2. NO(상시 열림) vs NC(상시 닫힘):                       │
│     - NO: 릴레이 ON → 연결됨 (가장 많이 사용)             │
│     - NC: 릴레이 ON → 끊어짐 (비상 회로 등)               │
│                                                         │
│  3. 릴레이 모듈 사용 권장!                                │
│     - 트랜지스터, 플라이백 다이오드, 옵토커플러 내장        │
│     - ESP32 GPIO 직접 연결 가능                           │
│                                                         │
│  4. 전원 주의:                                           │
│     - 릴레이 VCC는 5V 연결 (3.3V 부족!)                   │
│     - 여러 채널 사용 시 별도 전원 고려                     │
│                                                         │
│  5. Active LOW vs Active HIGH 확인!                      │
│     - 모듈에 따라 제어 논리가 다름                         │
│                                                         │
│  6. 플라이백 다이오드:                                    │
│     - 직접 구성 시: 반드시 필요!                           │
│     - 모듈 사용 시: 내장되어 있어 불필요                    │
│                                                         │
│  7. AC 220V 주의: 감전 위험! 안전 규칙 철저히 준수!        │
│     초보자는 DC 12V/5V로 먼저 연습!                       │
│                                                         │
│  8. ESP32 부팅 시 GPIO 상태 주의 (의도치 않은 ON 방지)     │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

---

> **다음 학습:** [07. 레귤레이터](07-regulator.md)에서 안정적인 전원 공급 방법을 배워봅시다!
