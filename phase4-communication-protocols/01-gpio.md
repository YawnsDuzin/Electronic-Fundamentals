# 01. GPIO (General Purpose Input/Output) - 범용 입출력

## 개요

GPIO(General Purpose Input/Output)는 마이크로컨트롤러의 **가장 기본적인 인터페이스**입니다.
각 GPIO 핀은 **디지털 HIGH(3.3V)** 또는 **LOW(0V)** 두 가지 상태만 가질 수 있으며,
**출력**(LED 켜기) 또는 **입력**(버튼 읽기)으로 설정할 수 있습니다.

> **소프트웨어 비유**: GPIO는 Boolean 변수와 같습니다.
> `true`(HIGH, 3.3V) 또는 `false`(LOW, 0V) 두 가지 값만 존재합니다.

---

## 1. 디지털 HIGH와 LOW

```
전압 (V)
3.3V ─── ┌───┐     ┌───┐     ┌───┐
         │   │     │   │     │   │
         │   │     │   │     │   │        HIGH = 3.3V (논리 1)
         │   │     │   │     │   │
 0V ─────┘   └─────┘   └─────┘   └────   LOW  = 0V  (논리 0)

         ◀─▶
        한 주기
```

| 상태 | 전압 | 논리값 | Arduino 코드 |
|------|------|--------|-------------|
| HIGH | 3.3V | 1 | `digitalWrite(pin, HIGH)` |
| LOW | 0V | 0 | `digitalWrite(pin, LOW)` |

> **주의**: ESP32는 **3.3V 로직**입니다. Arduino Uno(5V)와 다릅니다!
> 5V 신호를 직접 연결하면 ESP32가 손상될 수 있습니다.

---

## 2. ESP32 GPIO 핀맵

### 사용 가능한 핀

```
        ┌──────────────────────┐
        │     ESP32 DevKit     │
        │                      │
   3V3 ─┤ 3V3            VIN  ├─ 5V
   GND ─┤ GND            GND  ├─ GND
 GPIO15─┤ D15            D13  ├─GPIO13
  GPIO2─┤ D2             D12  ├─GPIO12
  GPIO4─┤ D4             D14  ├─GPIO14
 GPIO16─┤ RX2            D27  ├─GPIO27
 GPIO17─┤ TX2            D26  ├─GPIO26
  GPIO5─┤ D5             D25  ├─GPIO25
 GPIO18─┤ D18            D33  ├─GPIO33
 GPIO19─┤ D19            D32  ├─GPIO32
 GPIO21─┤ D21(SDA)       D35  ├─GPIO35 (입력전용)
  GPIO3─┤ RX0            D34  ├─GPIO34 (입력전용)
  GPIO1─┤ TX0            D39  ├─GPIO39 (입력전용)
 GPIO22─┤ D22(SCL)       D36  ├─GPIO36 (입력전용)
 GPIO23─┤ D23            EN   ├─ EN
        │                      │
        │      [USB 포트]      │
        └──────────────────────┘
```

### 핀 분류표

| 분류 | GPIO 번호 | 비고 |
|------|----------|------|
| **자유롭게 사용 가능** | 4, 5, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33 | 입력/출력 모두 가능 |
| **입력 전용** | 34, 35, 36 (VP), 39 (VN) | 내부 풀업 없음, 출력 불가 |
| **부팅 시 주의** | 0, 2, 12, 15 | 부팅 모드에 영향 - 외부 풀업/풀다운 주의 |
| **내장 플래시 연결** | 6, 7, 8, 9, 10, 11 | **절대 사용하지 마세요!** |
| **시리얼 통신용** | 1 (TX0), 3 (RX0) | USB 시리얼과 공유 - 일반 용도 비추천 |

> **팁**: 초보자는 **GPIO 4, 5, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33** 핀만 사용하세요.

---

## 3. 디지털 출력: LED 제어

### 회로 연결

```
ESP32                        LED
GPIO 2 ──────┤220Ω├──────►|────── GND
                          (애노드) (캐소드)
```

### 기본 코드

```cpp
const int LED_PIN = 2;  // GPIO 2번 핀

void setup() {
    pinMode(LED_PIN, OUTPUT);    // 핀을 출력 모드로 설정
}

void loop() {
    digitalWrite(LED_PIN, HIGH); // LED 켜기 (3.3V 출력)
    delay(1000);                 // 1초 대기
    digitalWrite(LED_PIN, LOW);  // LED 끄기 (0V 출력)
    delay(1000);                 // 1초 대기
}
```

### 전류 제한 저항 계산

```
저항값 = (전원 전압 - LED 순방향 전압) / LED 전류
       = (3.3V - 2.0V) / 0.010A
       = 130Ω → 실제로는 220Ω 사용 (안전 마진)
```

---

## 4. 디지털 입력: 버튼 읽기

### 풀업 저항 방식 (눌렀을 때 LOW)

```
    3.3V
     │
    [10kΩ] ← 풀업 저항
     │
     ├──────── GPIO 4 (입력)
     │
    [버튼]
     │
    GND
```

```
버튼 놓음:  GPIO = HIGH (3.3V, 풀업 저항을 통해 3.3V 연결)
버튼 누름:  GPIO = LOW  (0V,   GND에 직접 연결)
```

### ESP32 내부 풀업 사용 (외부 저항 불필요)

```cpp
const int BUTTON_PIN = 4;

void setup() {
    // INPUT_PULLUP: 내부 풀업 저항 활성화
    // 외부 10kΩ 저항 없이도 동작!
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Serial.begin(115200);
}

void loop() {
    int buttonState = digitalRead(BUTTON_PIN);

    if (buttonState == LOW) {  // 풀업이므로 눌렀을 때 LOW
        Serial.println("버튼 눌림!");
    } else {
        Serial.println("버튼 놓음");
    }
    delay(100);
}
```

---

## 5. 디바운싱 (Debouncing)

### 바운싱 문제란?

물리적 버튼을 누르면 접점이 완전히 접촉되기까지 **수 밀리초 동안 신호가 떨린다(bouncing)**.
이로 인해 한 번 눌렀는데 여러 번 눌린 것으로 인식될 수 있습니다.

```
이상적인 신호:           실제 신호 (바운싱):

HIGH ────┐               HIGH ────┐ ┌┐ ┌┐
         │                        │ ││ ││
         │                        │ ││ ││
LOW      └────────       LOW      └─┘└─┘└────────
         ↑                        ↑
      버튼 누름                 버튼 누름
                              (바운싱 발생!)
```

### 하드웨어 디바운싱

```
                  ┌───────────┐
GPIO 4 ───────────┤           │
                  │  100nF    │  ← 커패시터가 급격한 변화를 흡수
GND ──────────────┤           │
                  └───────────┘
```

커패시터(100nF)를 버튼과 병렬로 연결하면 전압 변화가 부드러워져 바운싱이 제거됩니다.

### 소프트웨어 디바운싱

```cpp
const int BUTTON_PIN = 4;
const int LED_PIN = 2;
const unsigned long DEBOUNCE_DELAY = 50; // 50ms 디바운스 시간

bool ledState = false;
int lastButtonState = HIGH;
int currentButtonState;
unsigned long lastDebounceTime = 0;

void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    int reading = digitalRead(BUTTON_PIN);

    // 상태가 변했으면 타이머 리셋
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }

    // 50ms 이상 안정적으로 유지되면 진짜 변화로 판단
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
        if (reading != currentButtonState) {
            currentButtonState = reading;

            // 버튼이 눌렸을 때만 (HIGH→LOW 전환)
            if (currentButtonState == LOW) {
                ledState = !ledState;  // LED 토글
                digitalWrite(LED_PIN, ledState);
            }
        }
    }

    lastButtonState = reading;
}
```

---

## 6. 인터럽트 (Interrupt) 개념

### 폴링 vs 인터럽트

```
[폴링 방식 - 비효율적]
┌──────────────────────────────────────┐
│  loop() {                            │
│      버튼 확인 ← 계속 반복 확인      │
│      다른 작업                       │
│      버튼 확인 ← 또 확인             │
│      다른 작업                       │
│      버튼 확인 ← 또또 확인           │
│      ...                             │
│  }                                   │
└──────────────────────────────────────┘

[인터럽트 방식 - 효율적]
┌──────────────────────────────────────┐
│  loop() {                            │
│      다른 작업 수행                  │
│      다른 작업 수행                  │
│      다른 작업 수행                  │
│  }                                   │
│           ↑                          │
│     버튼이 눌리면                    │
│     자동으로 ISR 함수 호출!          │
│  ISR() { LED 토글 }                  │
└──────────────────────────────────────┘
```

> **소프트웨어 비유**: 폴링 = `while(true) { checkEmail(); }`,
> 인터럽트 = 이메일 알림 설정 (새 메일이 오면 자동 알림)

### 인터럽트 트리거 모드

| 모드 | 설명 | Arduino 상수 |
|------|------|-------------|
| RISING | LOW → HIGH로 변할 때 | `RISING` |
| FALLING | HIGH → LOW로 변할 때 | `FALLING` |
| CHANGE | 상태가 변할 때마다 | `CHANGE` |
| LOW | LOW 상태일 때 계속 | `LOW` |
| HIGH | HIGH 상태일 때 계속 | `HIGH` |

### 인터럽트 코드 예제

```cpp
const int BUTTON_PIN = 4;
const int LED_PIN = 2;

// volatile: 인터럽트에서 변경되는 변수는 반드시 volatile 선언
volatile bool ledState = false;
volatile unsigned long lastInterruptTime = 0;

// ISR (Interrupt Service Routine) - 인터럽트 발생 시 실행되는 함수
void IRAM_ATTR handleButtonPress() {
    unsigned long currentTime = millis();
    // 디바운싱: 200ms 이내 재호출 무시
    if (currentTime - lastInterruptTime > 200) {
        ledState = !ledState;
        lastInterruptTime = currentTime;
    }
}

void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);

    // 인터럽트 등록: FALLING = 버튼 누를 때 (HIGH→LOW)
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN),
                    handleButtonPress, FALLING);
}

void loop() {
    digitalWrite(LED_PIN, ledState);
    // loop()에서는 다른 작업을 자유롭게 수행 가능
    delay(10);
}
```

### ISR 작성 시 주의사항

1. **IRAM_ATTR**: ESP32에서 ISR 함수에 반드시 이 속성을 붙여야 합니다 (RAM에서 실행).
2. **volatile**: ISR에서 수정하는 변수는 반드시 `volatile`로 선언합니다.
3. **ISR은 짧게**: ISR 내부에서 `delay()`, `Serial.println()` 등 시간이 걸리는 함수를 사용하지 마세요.
4. **플래그 패턴**: ISR에서는 플래그만 설정하고, 실제 처리는 `loop()`에서 합니다.

---

## 핵심 정리

| 개념 | 설명 |
|------|------|
| **GPIO** | 범용 입출력 핀, HIGH(3.3V) 또는 LOW(0V) |
| **pinMode()** | 핀 모드 설정: `OUTPUT`, `INPUT`, `INPUT_PULLUP` |
| **digitalWrite()** | 출력 핀에 HIGH 또는 LOW 쓰기 |
| **digitalRead()** | 입력 핀에서 HIGH 또는 LOW 읽기 |
| **풀업/풀다운 저항** | 입력 핀의 기본 상태를 결정하는 저항 |
| **디바운싱** | 기계적 버튼의 신호 떨림 제거 (하드웨어/소프트웨어) |
| **인터럽트** | 이벤트 발생 시 자동으로 함수를 호출하는 메커니즘 |
| **ISR** | 인터럽트 서비스 루틴 - 짧고 빠르게 작성해야 함 |

---

## 실습 과제

1. LED를 1초 간격으로 깜빡이기 (Blink)
2. 버튼을 누를 때마다 LED 토글하기 (디바운싱 적용)
3. 2개의 버튼으로 2개의 LED를 각각 제어하기
4. 인터럽트를 사용하여 버튼 누름 횟수를 시리얼 모니터에 출력하기

---

## 관련 코드 예제

- [gpio_led_button.ino](./code/gpio_led_button.ino) - LED 토글 + 디바운싱 전체 코드
