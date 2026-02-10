/*
 * ============================================================
 *  GPIO LED + 버튼 제어 예제 (소프트웨어 디바운싱 포함)
 * ============================================================
 *
 *  기능 설명:
 *  - 버튼을 누를 때마다 LED가 토글됩니다 (켜짐 ↔ 꺼짐)
 *  - 소프트웨어 디바운싱으로 버튼의 바운싱(채터링) 문제를 해결합니다
 *  - 시리얼 모니터에 LED 상태와 버튼 누름 횟수를 출력합니다
 *
 *  배선 안내 (Wiring Guide):
 *  ┌──────────────────────────────────────────────────┐
 *  │                                                  │
 *  │  ESP32          LED            버튼              │
 *  │  ─────          ───            ────              │
 *  │  GPIO 2 ──┤220Ω├──►|── GND                      │
 *  │                                                  │
 *  │  GPIO 4 ──────────────── 버튼 한쪽               │
 *  │                          버튼 다른쪽 ── GND      │
 *  │                                                  │
 *  │  ※ GPIO 4는 INPUT_PULLUP 모드로 설정             │
 *  │    (내부 풀업 저항 사용, 외부 저항 불필요)         │
 *  │                                                  │
 *  │  ※ 버튼을 안 누르면 HIGH(3.3V)                   │
 *  │    버튼을 누르면 LOW(0V)                          │
 *  │                                                  │
 *  └──────────────────────────────────────────────────┘
 *
 *  필요 부품:
 *  - ESP32 개발보드
 *  - LED 1개
 *  - 220Ω 저항 1개
 *  - 택트 스위치 (버튼) 1개
 *  - 브레드보드 및 점퍼 와이어
 *
 *  시리얼 모니터 설정: 115200 baud
 * ============================================================
 */

// ─────────────── 핀 정의 ───────────────
const int LED_PIN = 2;       // LED가 연결된 GPIO 핀 번호
const int BUTTON_PIN = 4;    // 버튼이 연결된 GPIO 핀 번호

// ─────────────── 디바운싱 설정 ───────────────
const unsigned long DEBOUNCE_DELAY = 50;  // 디바운스 지연 시간 (밀리초)
// 50ms 동안 상태가 안정적으로 유지되면 진짜 버튼 입력으로 판단

// ─────────────── 상태 변수 ───────────────
bool ledState = false;                 // 현재 LED 상태 (false = 꺼짐, true = 켜짐)
int lastButtonState = HIGH;            // 이전 루프에서의 버튼 상태
int currentButtonState = HIGH;         // 디바운싱 후 확정된 버튼 상태
unsigned long lastDebounceTime = 0;    // 마지막으로 상태가 변한 시각 (밀리초)
unsigned int pressCount = 0;           // 버튼 누름 횟수 카운터

// ─────────────── 초기 설정 ───────────────
void setup() {
    // 시리얼 통신 초기화 (디버깅용, 115200 보레이트)
    Serial.begin(115200);

    // LED 핀을 출력 모드로 설정
    pinMode(LED_PIN, OUTPUT);

    // 버튼 핀을 내부 풀업 저항이 있는 입력 모드로 설정
    // INPUT_PULLUP: 버튼을 안 누르면 HIGH(3.3V), 누르면 LOW(0V)
    // 외부에 10kΩ 풀업 저항을 연결할 필요 없음!
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // LED 초기 상태: 꺼짐
    digitalWrite(LED_PIN, LOW);

    // 시작 메시지 출력
    Serial.println("=================================");
    Serial.println("  GPIO LED + 버튼 제어 예제");
    Serial.println("  소프트웨어 디바운싱 적용");
    Serial.println("=================================");
    Serial.println("버튼을 누르면 LED가 토글됩니다.");
    Serial.println();
}

// ─────────────── 메인 루프 ───────────────
void loop() {
    // 1단계: 버튼의 현재 상태를 읽기
    int reading = digitalRead(BUTTON_PIN);

    // 2단계: 상태가 변했으면 디바운스 타이머 리셋
    // 바운싱이 발생하면 이 부분에서 타이머가 계속 리셋됨
    if (reading != lastButtonState) {
        lastDebounceTime = millis();  // 변화가 감지된 시각 기록
    }

    // 3단계: 디바운스 시간이 경과했는지 확인
    // DEBOUNCE_DELAY(50ms) 동안 상태가 변하지 않았으면 안정적인 입력으로 판단
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {

        // 4단계: 확정된 상태가 이전과 다른지 확인
        if (reading != currentButtonState) {
            currentButtonState = reading;  // 새로운 상태로 업데이트

            // 5단계: 버튼이 눌렸을 때만 동작 (HIGH → LOW 전환)
            // INPUT_PULLUP이므로 누르면 LOW가 됨
            if (currentButtonState == LOW) {
                // LED 상태 토글 (켜짐 ↔ 꺼짐)
                ledState = !ledState;
                digitalWrite(LED_PIN, ledState);

                // 버튼 누름 횟수 증가
                pressCount++;

                // 시리얼 모니터에 상태 출력
                Serial.print("[");
                Serial.print(pressCount);
                Serial.print("회] 버튼 눌림! → LED ");
                Serial.println(ledState ? "켜짐 (ON)" : "꺼짐 (OFF)");
            }
        }
    }

    // 6단계: 이전 상태 저장 (다음 루프에서 비교하기 위해)
    lastButtonState = reading;

    // ※ delay()를 사용하지 않음!
    // delay()를 사용하면 버튼 입력을 놓칠 수 있음
    // millis() 기반 디바운싱은 논블로킹 방식으로 동작
}

/*
 * ============================================================
 *  추가 설명: 디바운싱이 왜 필요한가?
 * ============================================================
 *
 *  기계식 버튼을 누르면 접점이 완전히 닫히기까지
 *  수 밀리초 동안 ON/OFF가 반복됩니다 (바운싱/채터링).
 *
 *  디바운싱 없이:
 *    버튼 1회 누름 → 마이크로컨트롤러가 3~5회로 인식!
 *
 *  디바운싱 적용 후:
 *    버튼 1회 누름 → 정확히 1회로 인식
 *
 *  동작 원리:
 *    1. 상태 변화 감지 → 타이머 시작
 *    2. 50ms 동안 같은 상태 유지 → 진짜 입력으로 확정
 *    3. 50ms 이내에 또 변하면 → 바운싱으로 간주, 타이머 리셋
 *
 * ============================================================
 *  인터럽트 방식 디바운싱 (참고용)
 * ============================================================
 *
 *  volatile bool buttonPressed = false;
 *  volatile unsigned long lastISRTime = 0;
 *
 *  void IRAM_ATTR buttonISR() {
 *      unsigned long now = millis();
 *      if (now - lastISRTime > 200) {  // 200ms 디바운스
 *          buttonPressed = true;
 *          lastISRTime = now;
 *      }
 *  }
 *
 *  void setup() {
 *      attachInterrupt(digitalPinToInterrupt(BUTTON_PIN),
 *                      buttonISR, FALLING);
 *  }
 *
 *  void loop() {
 *      if (buttonPressed) {
 *          buttonPressed = false;
 *          ledState = !ledState;
 *          digitalWrite(LED_PIN, ledState);
 *      }
 *  }
 *
 * ============================================================
 */
