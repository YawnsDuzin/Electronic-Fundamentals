/*
 * ===================================================================
 *  프로젝트 1: LED + 버튼 제어
 * ===================================================================
 *
 *  설명: 택트 스위치(버튼)를 사용하여 LED를 제어하는 프로젝트
 *        소프트웨어 디바운싱, 인터럽트, 다중 LED 순차 제어를 학습합니다.
 *
 *  보드: ESP32 DevKit V1
 *  작성: Phase 6 - 실전 프로젝트
 *
 *  회로 연결:
 *    - LED1 (빨강): GPIO16 → LED → 220Ω → GND
 *    - LED2 (빨강): GPIO17 → LED → 220Ω → GND
 *    - LED3 (초록): GPIO18 → LED → 220Ω → GND
 *    - LED4 (노랑): GPIO19 → LED → 220Ω → GND
 *    - BTN1 (토글): GPIO13 → 버튼 → GND (내부 풀다운 또는 외부 10KΩ)
 *    - BTN2 (모드): GPIO14 → 버튼 → GND
 *
 * ===================================================================
 */

// ===================================================================
//  핀 정의 - 하드웨어 연결에 따라 수정하세요
// ===================================================================

// LED 핀 정의 (배열로 관리하여 순차 제어에 활용)
const int LED_PINS[] = {16, 17, 18, 19};  // LED1~LED4 GPIO 핀 번호
const int LED_COUNT = 4;                    // LED 총 개수

// 버튼 핀 정의
const int BTN_TOGGLE_PIN = 13;   // BTN1: LED 토글/순차 제어용
const int BTN_MODE_PIN   = 14;   // BTN2: 동작 모드 변경용

// 내장 LED (디버깅용)
const int BUILTIN_LED_PIN = 2;   // ESP32 내장 LED (GPIO2)

// ===================================================================
//  상수 정의
// ===================================================================

// 디바운싱 관련 상수
const unsigned long DEBOUNCE_DELAY = 50;   // 디바운싱 지연 시간 (밀리초)
                                            // 50ms 미만이면 채터링 발생 가능
                                            // 200ms 초과하면 반응이 느려짐

// LED 순차 제어 관련 상수
const unsigned long SEQUENCE_INTERVAL = 200;  // 순차 점등 간격 (밀리초)
const unsigned long BLINK_INTERVAL = 500;     // 점멸 간격 (밀리초)
const unsigned long PING_PONG_INTERVAL = 150; // 핑퐁 간격 (밀리초)

// 동작 모드 정의 (가독성을 위해 열거형 사용)
enum OperationMode {
  MODE_TOGGLE      = 0,   // 모드 0: 단일 LED 토글
  MODE_SEQUENCE    = 1,   // 모드 1: LED 순차 점등
  MODE_BLINK_ALL   = 2,   // 모드 2: LED 전체 점멸
  MODE_PING_PONG   = 3,   // 모드 3: LED 핑퐁 (왕복)
  MODE_COUNT       = 4    // 전체 모드 개수
};

// ===================================================================
//  전역 변수
// ===================================================================

// --- 버튼 1 (토글) 관련 변수 ---
bool btn1State         = LOW;       // 현재 버튼1 상태
bool btn1LastState     = LOW;       // 이전 버튼1 상태 (디바운싱용)
unsigned long btn1LastDebounceTime = 0;  // 마지막 디바운싱 시각

// --- 버튼 2 (모드) 관련 변수 ---
bool btn2State         = LOW;       // 현재 버튼2 상태
bool btn2LastState     = LOW;       // 이전 버튼2 상태
unsigned long btn2LastDebounceTime = 0;  // 마지막 디바운싱 시각

// --- LED 상태 변수 ---
bool ledToggleState    = false;     // LED 토글 상태 (true=켜짐, false=꺼짐)
int  currentLedIndex   = 0;         // 현재 순차 제어 중인 LED 인덱스
bool blinkState        = false;     // 점멸 상태
int  pingPongDirection = 1;         // 핑퐁 방향 (1=오른쪽, -1=왼쪽)

// --- 모드 및 타이밍 변수 ---
OperationMode currentMode = MODE_TOGGLE;  // 현재 동작 모드
unsigned long lastPatternTime = 0;         // 마지막 패턴 업데이트 시각

// --- 인터럽트 관련 변수 ---
// volatile: 인터럽트 서비스 루틴(ISR)에서 변경되는 변수는
//           반드시 volatile로 선언해야 합니다.
//           컴파일러가 최적화로 변수 읽기를 생략하는 것을 방지합니다.
volatile bool interruptFlag = false;               // 인터럽트 발생 플래그
volatile unsigned long lastInterruptTime = 0;      // 마지막 인터럽트 시각

// ===================================================================
//  인터럽트 서비스 루틴 (ISR)
// ===================================================================

/*
 * IRAM_ATTR: ESP32에서 ISR 함수는 반드시 이 속성을 가져야 합니다.
 *            ISR 코드를 IRAM(Internal RAM)에 배치하여
 *            플래시 메모리 접근 지연을 방지합니다.
 *
 * ISR 작성 규칙:
 *   1. 가능한 짧게 작성 (최소한의 작업만)
 *   2. Serial.print() 등 시간이 오래 걸리는 함수 호출 금지
 *   3. delay() 함수 사용 금지
 *   4. volatile 변수만 수정
 *   5. 복잡한 로직은 메인 루프에서 처리
 */
void IRAM_ATTR handleButtonInterrupt() {
  // ISR 내부에서의 디바운싱 처리
  // millis() 대신 더 정확한 방법이 필요하지만, 여기서는 간단하게 처리
  unsigned long currentTime = millis();

  // 마지막 인터럽트로부터 DEBOUNCE_DELAY 이상 경과한 경우에만 처리
  if (currentTime - lastInterruptTime > DEBOUNCE_DELAY) {
    interruptFlag = true;            // 플래그만 설정 (실제 처리는 loop()에서)
    lastInterruptTime = currentTime; // 시간 갱신
  }
}

// ===================================================================
//  초기 설정 함수 (setup)
// ===================================================================

void setup() {
  // --- 시리얼 통신 초기화 ---
  // 115200 보드레이트: ESP32의 기본 권장 속도
  Serial.begin(115200);

  // 시리얼 통신이 준비될 때까지 대기 (USB 시리얼인 경우)
  while (!Serial) {
    delay(10);
  }

  Serial.println();
  Serial.println("============================================");
  Serial.println("  프로젝트 1: LED + 버튼 제어");
  Serial.println("============================================");
  Serial.println();

  // --- LED 핀 설정 ---
  // 모든 LED 핀을 출력 모드로 설정하고 초기 상태를 꺼짐(LOW)으로
  for (int i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);      // 출력 모드 설정
    digitalWrite(LED_PINS[i], LOW);    // 초기 상태: 꺼짐
    Serial.print("  LED");
    Serial.print(i + 1);
    Serial.print(" → GPIO");
    Serial.print(LED_PINS[i]);
    Serial.println(" (출력 모드 설정 완료)");
  }

  // 내장 LED 설정
  pinMode(BUILTIN_LED_PIN, OUTPUT);
  digitalWrite(BUILTIN_LED_PIN, LOW);

  // --- 버튼 핀 설정 ---
  // INPUT_PULLDOWN: ESP32 내부 풀다운 저항 활용
  // 외부 풀다운 저항(10KΩ)을 사용하는 경우 INPUT으로 설정
  pinMode(BTN_TOGGLE_PIN, INPUT_PULLDOWN);  // 버튼1: 내부 풀다운
  pinMode(BTN_MODE_PIN, INPUT_PULLDOWN);    // 버튼2: 내부 풀다운

  Serial.print("  BTN1 (토글) → GPIO");
  Serial.println(BTN_TOGGLE_PIN);
  Serial.print("  BTN2 (모드) → GPIO");
  Serial.println(BTN_MODE_PIN);

  // --- 인터럽트 설정 ---
  // BTN1에 인터럽트 연결 (RISING: LOW→HIGH 전환 시 트리거)
  // 풀다운 저항 사용 시 버튼을 누르면 HIGH가 되므로 RISING 사용
  attachInterrupt(digitalPinToInterrupt(BTN_TOGGLE_PIN),
                  handleButtonInterrupt,
                  RISING);

  Serial.println();
  Serial.println("  인터럽트 설정 완료 (BTN1, RISING 트리거)");
  Serial.println();
  Serial.println("============================================");
  Serial.println("  사용법:");
  Serial.println("    BTN1: LED 제어 (모드에 따라 다름)");
  Serial.println("    BTN2: 동작 모드 변경");
  Serial.println("    모드 0: 단일 LED 토글");
  Serial.println("    모드 1: LED 순차 점등");
  Serial.println("    모드 2: LED 전체 점멸");
  Serial.println("    모드 3: LED 핑퐁 (왕복)");
  Serial.println("============================================");
  Serial.println();
}

// ===================================================================
//  메인 루프 함수 (loop)
// ===================================================================

void loop() {
  // --- 인터럽트 플래그 처리 ---
  // ISR에서 설정한 플래그를 메인 루프에서 처리
  // (ISR 내에서 복잡한 로직을 실행하지 않기 위함)
  handleInterruptFlag();

  // --- 버튼 2 디바운싱 처리 (폴링 방식) ---
  // BTN2는 폴링 방식으로 디바운싱을 적용합니다.
  // 인터럽트 방식과 비교하여 학습할 수 있습니다.
  handleModeButton();

  // --- 현재 모드에 따른 LED 패턴 실행 ---
  executeCurrentMode();

  // --- 내장 LED로 현재 모드 표시 ---
  // 내장 LED가 빠르게 깜빡이는 횟수로 현재 모드를 표시
  updateBuiltinLED();
}

// ===================================================================
//  인터럽트 플래그 처리 함수
// ===================================================================

/*
 * 인터럽트에서 설정된 플래그를 확인하고 처리합니다.
 * ISR은 최대한 짧게 유지하고, 실제 로직은 여기서 처리합니다.
 */
void handleInterruptFlag() {
  if (interruptFlag) {
    interruptFlag = false;  // 플래그 초기화

    // 현재 모드가 토글 모드인 경우
    if (currentMode == MODE_TOGGLE) {
      // LED 토글 상태 반전
      ledToggleState = !ledToggleState;
      digitalWrite(LED_PINS[0], ledToggleState ? HIGH : LOW);

      // 시리얼 모니터에 상태 출력
      Serial.print("[인터럽트] LED1 ");
      Serial.println(ledToggleState ? "켜짐 (ON)" : "꺼짐 (OFF)");
    }
    // 다른 모드에서는 인터럽트를 시작/정지 토글로 사용
    else {
      static bool patternRunning = true;
      patternRunning = !patternRunning;

      if (!patternRunning) {
        // 모든 LED 끄기
        turnOffAllLeds();
        Serial.println("[인터럽트] 패턴 정지");
      } else {
        Serial.println("[인터럽트] 패턴 시작");
      }
    }
  }
}

// ===================================================================
//  모드 변경 버튼 처리 함수 (폴링 + 디바운싱)
// ===================================================================

/*
 * BTN2를 폴링 방식으로 읽고, 소프트웨어 디바운싱을 적용합니다.
 *
 * 디바운싱 알고리즘:
 *   1. 현재 버튼 상태를 읽음
 *   2. 이전 상태와 다르면 디바운싱 타이머 시작
 *   3. DEBOUNCE_DELAY 이상 경과한 후에도 같은 상태면 유효한 입력으로 인정
 *   4. 유효한 HIGH→LOW 전환(버튼 뗌) 시 모드 변경 실행
 */
void handleModeButton() {
  // 현재 버튼 상태 읽기
  bool reading = digitalRead(BTN_MODE_PIN);

  // 이전 읽기값과 다르면 디바운싱 타이머 재시작
  if (reading != btn2LastState) {
    btn2LastDebounceTime = millis();
  }

  // 디바운싱 시간이 경과했는지 확인
  if ((millis() - btn2LastDebounceTime) > DEBOUNCE_DELAY) {
    // 안정된 상태가 현재 상태와 다르면 상태 업데이트
    if (reading != btn2State) {
      btn2State = reading;

      // 버튼이 눌린 순간에만 동작 (RISING 엣지)
      // HIGH = 버튼 눌림 (풀다운 저항 사용 시)
      if (btn2State == HIGH) {
        // 모드 순환: 0 → 1 → 2 → 3 → 0 ...
        currentMode = (OperationMode)((currentMode + 1) % MODE_COUNT);

        // 모드 변경 시 LED 상태 초기화
        turnOffAllLeds();
        currentLedIndex = 0;
        pingPongDirection = 1;

        // 모드 변경 알림 출력
        Serial.print("[모드 변경] 현재 모드: ");
        printModeName(currentMode);
        Serial.println();
      }
    }
  }

  // 이전 상태 저장 (다음 루프에서 비교용)
  btn2LastState = reading;
}

// ===================================================================
//  모드별 LED 패턴 실행 함수
// ===================================================================

/*
 * 현재 선택된 모드에 따라 해당 LED 패턴을 실행합니다.
 * 모든 타이밍은 millis()를 기반으로 하여 non-blocking으로 동작합니다.
 * (delay()를 사용하지 않아 버튼 입력을 놓치지 않습니다)
 */
void executeCurrentMode() {
  switch (currentMode) {
    case MODE_TOGGLE:
      // 토글 모드: 인터럽트에서 직접 처리하므로 여기서는 아무것도 하지 않음
      break;

    case MODE_SEQUENCE:
      executeSequencePattern();
      break;

    case MODE_BLINK_ALL:
      executeBlinkAllPattern();
      break;

    case MODE_PING_PONG:
      executePingPongPattern();
      break;

    default:
      break;
  }
}

// ===================================================================
//  패턴 1: 순차 점등 (LED가 하나씩 순서대로 켜짐)
// ===================================================================

/*
 * LED1 → LED2 → LED3 → LED4 → LED1 → ... 순서로 하나씩 점등
 *
 * 동작 원리:
 *   - SEQUENCE_INTERVAL마다 현재 LED를 끄고 다음 LED를 켬
 *   - 마지막 LED 다음에는 첫 번째 LED로 돌아감 (순환)
 *   - millis() 기반으로 non-blocking 동작
 */
void executeSequencePattern() {
  unsigned long currentTime = millis();

  // 지정된 간격이 경과했는지 확인
  if (currentTime - lastPatternTime >= SEQUENCE_INTERVAL) {
    lastPatternTime = currentTime;  // 시간 갱신

    // 현재 LED 끄기
    digitalWrite(LED_PINS[currentLedIndex], LOW);

    // 다음 LED로 이동 (순환)
    currentLedIndex = (currentLedIndex + 1) % LED_COUNT;

    // 다음 LED 켜기
    digitalWrite(LED_PINS[currentLedIndex], HIGH);

    // 디버그 출력 (선택사항 - 주석 해제하여 사용)
    // Serial.print("[순차] LED");
    // Serial.print(currentLedIndex + 1);
    // Serial.println(" 켜짐");
  }
}

// ===================================================================
//  패턴 2: 전체 점멸 (모든 LED가 동시에 깜빡임)
// ===================================================================

/*
 * 모든 LED가 동시에 켜졌다 꺼졌다를 반복합니다.
 *
 * 동작 원리:
 *   - BLINK_INTERVAL마다 blinkState를 반전
 *   - blinkState에 따라 모든 LED를 동시에 ON/OFF
 */
void executeBlinkAllPattern() {
  unsigned long currentTime = millis();

  if (currentTime - lastPatternTime >= BLINK_INTERVAL) {
    lastPatternTime = currentTime;

    // 점멸 상태 반전
    blinkState = !blinkState;

    // 모든 LED에 동일한 상태 적용
    for (int i = 0; i < LED_COUNT; i++) {
      digitalWrite(LED_PINS[i], blinkState ? HIGH : LOW);
    }
  }
}

// ===================================================================
//  패턴 3: 핑퐁 (LED가 왕복하며 이동)
// ===================================================================

/*
 * LED가 왼쪽에서 오른쪽으로, 다시 오른쪽에서 왼쪽으로 왕복합니다.
 *
 * 동작 원리:
 *   - 순차 점등과 유사하지만 끝에 도달하면 방향이 반전됨
 *   - pingPongDirection: +1이면 오른쪽, -1이면 왼쪽
 *
 * 패턴 예시 (LED 4개):
 *   → LED1, LED2, LED3, LED4, LED3, LED2, LED1, LED2, ...
 *   (양 끝에서 반사되는 핑퐁 공처럼 움직임)
 */
void executePingPongPattern() {
  unsigned long currentTime = millis();

  if (currentTime - lastPatternTime >= PING_PONG_INTERVAL) {
    lastPatternTime = currentTime;

    // 현재 LED 끄기
    digitalWrite(LED_PINS[currentLedIndex], LOW);

    // 다음 위치 계산
    currentLedIndex += pingPongDirection;

    // 끝에 도달하면 방향 반전
    if (currentLedIndex >= LED_COUNT - 1) {
      pingPongDirection = -1;  // 왼쪽으로 방향 전환
      currentLedIndex = LED_COUNT - 1;
    }
    else if (currentLedIndex <= 0) {
      pingPongDirection = 1;   // 오른쪽으로 방향 전환
      currentLedIndex = 0;
    }

    // 다음 LED 켜기
    digitalWrite(LED_PINS[currentLedIndex], HIGH);
  }
}

// ===================================================================
//  유틸리티 함수
// ===================================================================

/*
 * 모든 LED를 끄는 함수
 * 모드 전환 시 또는 정지 시 사용됩니다.
 */
void turnOffAllLeds() {
  for (int i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
}

/*
 * 모든 LED를 켜는 함수
 */
void turnOnAllLeds() {
  for (int i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PINS[i], HIGH);
  }
}

/*
 * 모드 이름을 시리얼 모니터에 출력하는 함수
 */
void printModeName(OperationMode mode) {
  switch (mode) {
    case MODE_TOGGLE:
      Serial.print("모드 0 - 단일 LED 토글");
      break;
    case MODE_SEQUENCE:
      Serial.print("모드 1 - LED 순차 점등");
      break;
    case MODE_BLINK_ALL:
      Serial.print("모드 2 - LED 전체 점멸");
      break;
    case MODE_PING_PONG:
      Serial.print("모드 3 - LED 핑퐁 (왕복)");
      break;
    default:
      Serial.print("알 수 없는 모드");
      break;
  }
}

/*
 * 내장 LED로 현재 모드를 표시하는 함수
 * 모드 번호만큼 짧게 깜빡임 (3초마다 반복)
 */
void updateBuiltinLED() {
  static unsigned long lastBlinkTime = 0;
  static int blinkCount = 0;
  static bool builtinLedState = false;
  static unsigned long cycleStartTime = 0;

  unsigned long currentTime = millis();

  // 3초마다 깜빡임 사이클 재시작
  if (currentTime - cycleStartTime >= 3000) {
    cycleStartTime = currentTime;
    blinkCount = 0;
    builtinLedState = false;
    digitalWrite(BUILTIN_LED_PIN, LOW);
  }

  // 현재 모드 번호 + 1 만큼 깜빡임
  if (blinkCount <= (int)currentMode * 2 + 1) {
    if (currentTime - lastBlinkTime >= 100) {
      lastBlinkTime = currentTime;
      builtinLedState = !builtinLedState;
      digitalWrite(BUILTIN_LED_PIN, builtinLedState ? HIGH : LOW);
      blinkCount++;
    }
  }
}

// ===================================================================
//  추가 학습: 폴링 방식 디바운싱 (참고용)
// ===================================================================

/*
 * 아래 함수는 인터럽트 대신 폴링 방식으로 버튼1을 처리하는 예제입니다.
 * 인터럽트 방식과 비교 학습할 수 있습니다.
 *
 * 인터럽트 방식 vs 폴링 방식:
 *
 *   [인터럽트 방식]
 *   장점: CPU가 다른 작업 중에도 즉시 응답 가능
 *   단점: ISR 내 코드 제약, 디바운싱 처리 복잡
 *
 *   [폴링 방식]
 *   장점: 구현이 간단, 디바운싱 처리 용이
 *   단점: loop() 주기에 따라 응답 지연 가능
 *
 * 이 프로젝트에서는 BTN1은 인터럽트, BTN2는 폴링으로 구현하여
 * 두 방식을 비교할 수 있습니다.
 */
void handleToggleButtonPolling() {
  // 현재 버튼 상태 읽기
  bool reading = digitalRead(BTN_TOGGLE_PIN);

  // 상태 변화 감지 시 디바운싱 타이머 시작
  if (reading != btn1LastState) {
    btn1LastDebounceTime = millis();
  }

  // 디바운싱 시간 경과 확인
  if ((millis() - btn1LastDebounceTime) > DEBOUNCE_DELAY) {
    // 안정된 새로운 상태 감지
    if (reading != btn1State) {
      btn1State = reading;

      // 버튼 눌림 감지 (RISING 엣지)
      if (btn1State == HIGH) {
        ledToggleState = !ledToggleState;
        digitalWrite(LED_PINS[0], ledToggleState ? HIGH : LOW);

        Serial.print("[폴링] LED1 ");
        Serial.println(ledToggleState ? "켜짐 (ON)" : "꺼짐 (OFF)");
      }
    }
  }

  btn1LastState = reading;
}

// ===================================================================
//  추가 학습: 하드웨어 디바운싱 참고
// ===================================================================

/*
 * 하드웨어 디바운싱 회로 (참고 정보):
 *
 * 방법 1: RC 필터
 *   버튼 → ─┬─── 저항(10KΩ) ─── GPIO
 *            │
 *          커패시터(100nF)
 *            │
 *           GND
 *
 *   시정수 τ = R × C = 10KΩ × 100nF = 1ms
 *   완전 충전 시간 ≈ 5τ = 5ms
 *
 * 방법 2: 슈미트 트리거 IC (74HC14)
 *   버튼의 불안정한 신호를 깨끗한 디지털 신호로 변환
 *
 * 이 프로젝트에서는 소프트웨어 디바운싱을 사용합니다.
 * 하드웨어 디바운싱은 참고용으로만 기재합니다.
 */

// ===================================================================
//  끝 (End of File)
// ===================================================================
