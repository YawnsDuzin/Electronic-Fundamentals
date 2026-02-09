/*
 * ===================================================================
 *  프로젝트 4: 릴레이로 외부 장치 제어
 * ===================================================================
 *
 *  설명: ESP32로 릴레이 모듈을 제어하는 프로젝트
 *        시리얼 명령, 버튼, 타이머를 통한 다양한 제어 방식을 학습합니다.
 *        안전 타임아웃 기능으로 릴레이가 장시간 켜져 있는 것을 방지합니다.
 *
 *  보드: ESP32 DevKit V1
 *  작성: Phase 6 - 실전 프로젝트
 *
 *  주요 기능:
 *    1. 기본 ON/OFF 제어 (버튼 + 시리얼 명령)
 *    2. 타이머 기반 자동 제어 (millis 사용, delay 미사용)
 *    3. 시리얼 명령어 제어 (ON, OFF, STATUS, TIMER 등)
 *    4. 상태 LED 표시 (릴레이 상태 시각화)
 *    5. 안전 타임아웃 (일정 시간 후 자동 OFF)
 *
 *  회로 연결:
 *  ┌─────────────────────────────────────────────────────────┐
 *  │                                                         │
 *  │  [릴레이 모듈]                                           │
 *  │    VCC ──── ESP32 VIN (5V)                              │
 *  │    GND ──── ESP32 GND                                   │
 *  │    IN  ──── ESP32 GPIO26                                │
 *  │    COM ──── 외부 전원 (+) (DC 12V 등)                    │
 *  │    NO  ──── 부하 (+) (팬, LED 스트립 등)                  │
 *  │    부하 (-) ── 외부 전원 (-)                              │
 *  │                                                         │
 *  │  [상태 LED]                                              │
 *  │    ESP32 GPIO2 ── LED(+) ── 220Ω ── GND                │
 *  │    (ESP32 내장 LED를 사용하므로 외부 LED 없이도 동작)      │
 *  │                                                         │
 *  │  [제어 버튼]                                              │
 *  │    ESP32 GPIO27 ── 버튼 ── GND                           │
 *  │    (내부 풀업 저항 사용, 버튼 누르면 LOW)                   │
 *  │                                                         │
 *  │  ※ 릴레이 모듈에 트랜지스터와 플라이백 다이오드가           │
 *  │    내장되어 있으므로 별도 부품 없이 연결 가능합니다.        │
 *  │                                                         │
 *  │  ※ 릴레이 모듈이 Active LOW인 경우:                       │
 *  │    IN=LOW → 릴레이 ON, IN=HIGH → 릴레이 OFF              │
 *  │    코드의 RELAY_ACTIVE_LOW를 true로 설정하세요.            │
 *  │                                                         │
 *  └─────────────────────────────────────────────────────────┘
 *
 * ===================================================================
 */

// ===================================================================
//  핀 정의 - 하드웨어 연결에 따라 수정하세요
// ===================================================================

const int RELAY_PIN  = 26;    // 릴레이 제어 핀 (GPIO26)
const int LED_PIN    = 2;     // 상태 LED 핀 (GPIO2, ESP32 내장 LED)
const int BUTTON_PIN = 27;    // 제어 버튼 핀 (GPIO27)

// ===================================================================
//  설정 상수 - 필요에 따라 수정하세요
// ===================================================================

// 릴레이 동작 방식 설정
// true  = Active LOW (IN=LOW일 때 릴레이 ON) - 대부분의 릴레이 모듈
// false = Active HIGH (IN=HIGH일 때 릴레이 ON) - 일부 릴레이 모듈
const bool RELAY_ACTIVE_LOW = true;

// 안전 타임아웃 설정
// 릴레이가 연속으로 켜져 있을 수 있는 최대 시간 (밀리초)
// 이 시간이 지나면 자동으로 릴레이가 꺼집니다 (과열/화재 방지)
const unsigned long SAFETY_TIMEOUT_MS = 600000;   // 10분 (600,000ms)
                                                    // 0으로 설정하면 타임아웃 비활성화

// 버튼 디바운싱 시간 (밀리초)
const unsigned long DEBOUNCE_DELAY_MS = 50;

// 시리얼 통신 속도
const unsigned long SERIAL_BAUD = 115200;

// 상태 출력 주기 (밀리초) - 주기적으로 현재 상태를 시리얼에 출력
const unsigned long STATUS_PRINT_INTERVAL_MS = 30000;   // 30초마다

// ===================================================================
//  릴레이 동작 모드 정의
// ===================================================================

enum RelayMode {
  MODE_MANUAL = 0,    // 수동 제어 모드 (버튼/시리얼 명령으로만 제어)
  MODE_TIMER  = 1,    // 타이머 모드 (설정된 간격으로 자동 ON/OFF 반복)
  MODE_COUNTDOWN = 2  // 카운트다운 모드 (지정 시간 후 자동 OFF)
};

// ===================================================================
//  전역 변수
// ===================================================================

// --- 릴레이 상태 변수 ---
bool relayState = false;               // 현재 릴레이 상태 (true=ON, false=OFF)
RelayMode currentMode = MODE_MANUAL;   // 현재 동작 모드
unsigned long relayOnStartTime = 0;    // 릴레이가 켜진 시각 (안전 타임아웃용)

// --- 타이머 모드 변수 ---
unsigned long timerInterval = 5000;    // 타이머 ON/OFF 간격 (밀리초, 기본 5초)
unsigned long lastTimerToggle = 0;     // 마지막 타이머 토글 시각

// --- 카운트다운 모드 변수 ---
unsigned long countdownDuration = 0;   // 카운트다운 시간 (밀리초)
unsigned long countdownStartTime = 0;  // 카운트다운 시작 시각

// --- 버튼 관련 변수 ---
bool buttonState = HIGH;              // 현재 버튼 상태 (풀업이므로 HIGH가 기본)
bool lastButtonState = HIGH;          // 이전 버튼 읽기값
unsigned long lastDebounceTime = 0;   // 마지막 디바운싱 시각

// --- 상태 출력 변수 ---
unsigned long lastStatusPrint = 0;     // 마지막 상태 출력 시각

// --- 통계 변수 ---
unsigned long totalOnTime = 0;         // 릴레이 총 ON 시간 (밀리초)
unsigned long toggleCount = 0;         // 릴레이 토글 횟수

// ===================================================================
//  초기 설정 함수 (setup)
// ===================================================================

void setup() {
  // --- 시리얼 통신 초기화 ---
  Serial.begin(SERIAL_BAUD);
  while (!Serial) { delay(10); }

  Serial.println();
  Serial.println("============================================");
  Serial.println("  프로젝트 4: 릴레이 제어 시스템");
  Serial.println("============================================");
  Serial.println();

  // --- 핀 모드 설정 ---

  // 릴레이 핀: 출력 모드
  pinMode(RELAY_PIN, OUTPUT);

  // 릴레이 초기 상태: OFF
  // Active LOW 모듈이면 HIGH가 OFF, Active HIGH이면 LOW가 OFF
  setRelayOutput(false);
  Serial.print("  릴레이 핀: GPIO");
  Serial.print(RELAY_PIN);
  Serial.print(" (Active ");
  Serial.print(RELAY_ACTIVE_LOW ? "LOW" : "HIGH");
  Serial.println(")");

  // 상태 LED 핀: 출력 모드
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);   // LED 초기 상태: 꺼짐
  Serial.print("  상태 LED: GPIO");
  Serial.println(LED_PIN);

  // 버튼 핀: 입력 모드 (내부 풀업 저항 사용)
  // 버튼을 누르면 LOW, 뗴면 HIGH
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.print("  버튼 핀:  GPIO");
  Serial.print(BUTTON_PIN);
  Serial.println(" (내부 풀업)");

  // --- 안전 타임아웃 설정 표시 ---
  Serial.println();
  if (SAFETY_TIMEOUT_MS > 0) {
    Serial.print("  안전 타임아웃: ");
    Serial.print(SAFETY_TIMEOUT_MS / 1000);
    Serial.println("초");
  } else {
    Serial.println("  안전 타임아웃: 비활성화 (주의!)");
  }

  // --- 사용법 안내 ---
  Serial.println();
  printHelp();

  Serial.println();
  Serial.println("============================================");
  Serial.println("  시스템 준비 완료! 명령을 입력하세요.");
  Serial.println("============================================");
  Serial.println();
}

// ===================================================================
//  메인 루프 함수 (loop)
// ===================================================================

void loop() {
  unsigned long currentTime = millis();

  // 1. 시리얼 명령 처리
  handleSerialCommand();

  // 2. 버튼 입력 처리 (디바운싱 적용)
  handleButton(currentTime);

  // 3. 타이머 모드 처리
  handleTimerMode(currentTime);

  // 4. 카운트다운 모드 처리
  handleCountdownMode(currentTime);

  // 5. 안전 타임아웃 검사
  checkSafetyTimeout(currentTime);

  // 6. 상태 LED 업데이트
  updateStatusLED();

  // 7. 주기적 상태 출력
  handlePeriodicStatus(currentTime);
}

// ===================================================================
//  릴레이 제어 함수
// ===================================================================

/*
 * 릴레이의 물리적 출력을 설정합니다.
 * Active LOW / Active HIGH 설정에 따라 출력 레벨이 자동으로 결정됩니다.
 *
 * @param on: true = 릴레이 ON, false = 릴레이 OFF
 */
void setRelayOutput(bool on) {
  if (RELAY_ACTIVE_LOW) {
    // Active LOW: ON일 때 LOW 출력, OFF일 때 HIGH 출력
    digitalWrite(RELAY_PIN, on ? LOW : HIGH);
  } else {
    // Active HIGH: ON일 때 HIGH 출력, OFF일 때 LOW 출력
    digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  }
}

/*
 * 릴레이를 켭니다.
 * 이미 켜져 있으면 아무 동작도 하지 않습니다.
 *
 * @param source: 명령 출처 문자열 (로그 출력용)
 */
void turnRelayOn(const char* source) {
  if (relayState) {
    // 이미 켜져 있는 경우
    Serial.print("  [");
    Serial.print(source);
    Serial.println("] 릴레이가 이미 ON 상태입니다.");
    return;
  }

  relayState = true;
  setRelayOutput(true);
  relayOnStartTime = millis();   // 안전 타임아웃 시작 시각 기록
  toggleCount++;

  Serial.print("  [");
  Serial.print(source);
  Serial.println("] 릴레이 ON");
}

/*
 * 릴레이를 끕니다.
 * 이미 꺼져 있으면 아무 동작도 하지 않습니다.
 *
 * @param source: 명령 출처 문자열 (로그 출력용)
 */
void turnRelayOff(const char* source) {
  if (!relayState) {
    Serial.print("  [");
    Serial.print(source);
    Serial.println("] 릴레이가 이미 OFF 상태입니다.");
    return;
  }

  // ON 시간 누적 (통계용)
  totalOnTime += (millis() - relayOnStartTime);

  relayState = false;
  setRelayOutput(false);
  toggleCount++;

  Serial.print("  [");
  Serial.print(source);
  Serial.println("] 릴레이 OFF");
}

/*
 * 릴레이 상태를 반전합니다 (토글).
 *
 * @param source: 명령 출처 문자열 (로그 출력용)
 */
void toggleRelay(const char* source) {
  if (relayState) {
    turnRelayOff(source);
  } else {
    turnRelayOn(source);
  }
}

// ===================================================================
//  시리얼 명령 처리 함수
// ===================================================================

/*
 * 시리얼로 수신된 명령어를 파싱하고 실행합니다.
 *
 * 지원 명령어:
 *   ON         - 릴레이 켜기
 *   OFF        - 릴레이 끄기
 *   TOGGLE     - 릴레이 상태 반전
 *   STATUS     - 현재 상태 출력
 *   TIMER n    - 타이머 모드 (n초 간격으로 ON/OFF 반복)
 *   TIMER OFF  - 타이머 모드 해제
 *   COUNTDOWN n - n초 후 자동 OFF (릴레이를 켜고 n초 뒤 자동 꺼짐)
 *   MANUAL     - 수동 제어 모드로 전환
 *   STATS      - 동작 통계 출력
 *   HELP       - 도움말 표시
 */
void handleSerialCommand() {
  // 시리얼 데이터가 수신되었는지 확인
  if (!Serial.available()) return;

  // 줄바꿈(\n)까지 문자열 읽기
  String command = Serial.readStringUntil('\n');

  // 앞뒤 공백 제거
  command.trim();

  // 빈 문자열 무시
  if (command.length() == 0) return;

  // 대문자로 변환 (대소문자 구분 없이 처리)
  command.toUpperCase();

  // 수신된 명령 에코 출력
  Serial.print("\n  > 수신 명령: \"");
  Serial.print(command);
  Serial.println("\"");

  // --- 명령어 분기 처리 ---

  if (command == "ON") {
    // 수동 모드로 전환 후 릴레이 ON
    currentMode = MODE_MANUAL;
    turnRelayOn("시리얼");

  } else if (command == "OFF") {
    // 수동 모드로 전환 후 릴레이 OFF
    currentMode = MODE_MANUAL;
    turnRelayOff("시리얼");

  } else if (command == "TOGGLE") {
    // 수동 모드로 전환 후 토글
    currentMode = MODE_MANUAL;
    toggleRelay("시리얼");

  } else if (command == "STATUS") {
    // 현재 상태 출력
    printStatus();

  } else if (command.startsWith("TIMER")) {
    // 타이머 명령 처리
    handleTimerCommand(command);

  } else if (command.startsWith("COUNTDOWN")) {
    // 카운트다운 명령 처리
    handleCountdownCommand(command);

  } else if (command == "MANUAL") {
    // 수동 모드로 전환
    currentMode = MODE_MANUAL;
    Serial.println("  → 수동 제어 모드로 전환되었습니다.");

  } else if (command == "STATS") {
    // 통계 출력
    printStats();

  } else if (command == "HELP") {
    // 도움말 출력
    printHelp();

  } else {
    // 알 수 없는 명령
    Serial.print("  → 알 수 없는 명령: \"");
    Serial.print(command);
    Serial.println("\"");
    Serial.println("  → HELP를 입력하면 사용 가능한 명령을 확인할 수 있습니다.");
  }

  Serial.println();
}

/*
 * TIMER 명령을 파싱하고 처리합니다.
 *
 * 형식:
 *   "TIMER n"   → n초 간격으로 타이머 모드 시작
 *   "TIMER OFF" → 타이머 모드 해제
 *   "TIMER"     → 현재 타이머 간격으로 타이머 모드 시작/해제 토글
 */
void handleTimerCommand(String command) {
  // "TIMER" 뒤의 파라미터 추출
  String param = "";
  if (command.length() > 5) {
    param = command.substring(6);   // "TIMER " 이후 문자열
    param.trim();
  }

  if (param == "OFF" || param == "STOP") {
    // 타이머 모드 해제
    currentMode = MODE_MANUAL;
    Serial.println("  → 타이머 모드 해제. 수동 제어로 전환.");

  } else if (param.length() > 0) {
    // 숫자 파라미터가 있는 경우: 간격 설정 후 타이머 시작
    int seconds = param.toInt();

    if (seconds <= 0) {
      Serial.println("  → 오류: 타이머 간격은 1초 이상이어야 합니다.");
      return;
    }

    if (seconds > 3600) {
      Serial.println("  → 오류: 타이머 간격은 최대 3600초(1시간)입니다.");
      return;
    }

    timerInterval = (unsigned long)seconds * 1000;
    currentMode = MODE_TIMER;
    lastTimerToggle = millis();

    Serial.print("  → 타이머 모드 시작: ");
    Serial.print(seconds);
    Serial.println("초 간격으로 ON/OFF 반복");

  } else {
    // 파라미터 없이 "TIMER"만 입력한 경우: 토글
    if (currentMode == MODE_TIMER) {
      currentMode = MODE_MANUAL;
      Serial.println("  → 타이머 모드 해제.");
    } else {
      currentMode = MODE_TIMER;
      lastTimerToggle = millis();
      Serial.print("  → 타이머 모드 시작: ");
      Serial.print(timerInterval / 1000);
      Serial.println("초 간격");
    }
  }
}

/*
 * COUNTDOWN 명령을 파싱하고 처리합니다.
 *
 * 형식:
 *   "COUNTDOWN n" → 릴레이를 켜고 n초 후 자동 OFF
 */
void handleCountdownCommand(String command) {
  // "COUNTDOWN" 뒤의 파라미터 추출
  String param = "";
  if (command.length() > 9) {
    param = command.substring(10);  // "COUNTDOWN " 이후 문자열
    param.trim();
  }

  if (param.length() == 0) {
    Serial.println("  → 사용법: COUNTDOWN n (n = 초 단위 시간)");
    Serial.println("  → 예: COUNTDOWN 30 (30초 후 자동 OFF)");
    return;
  }

  int seconds = param.toInt();

  if (seconds <= 0) {
    Serial.println("  → 오류: 카운트다운 시간은 1초 이상이어야 합니다.");
    return;
  }

  if (seconds > 7200) {
    Serial.println("  → 오류: 카운트다운 시간은 최대 7200초(2시간)입니다.");
    return;
  }

  // 카운트다운 모드 시작
  countdownDuration = (unsigned long)seconds * 1000;
  countdownStartTime = millis();
  currentMode = MODE_COUNTDOWN;

  // 릴레이 켜기
  if (!relayState) {
    turnRelayOn("카운트다운");
  }

  Serial.print("  → 카운트다운 시작: ");
  Serial.print(seconds);
  Serial.println("초 후 자동 OFF");
}

// ===================================================================
//  버튼 입력 처리 함수
// ===================================================================

/*
 * 버튼 입력을 읽고 소프트웨어 디바운싱을 적용합니다.
 * 버튼을 누르면 릴레이 상태가 토글됩니다.
 *
 * 풀업 저항을 사용하므로:
 *   - 버튼 안 누름: HIGH (기본 상태)
 *   - 버튼 누름: LOW
 *   - FALLING 엣지(HIGH→LOW)에서 동작 실행
 *
 * @param currentTime: 현재 millis() 값
 */
void handleButton(unsigned long currentTime) {
  // 현재 버튼 상태 읽기
  bool reading = digitalRead(BUTTON_PIN);

  // 상태가 변했으면 디바운싱 타이머 재시작
  if (reading != lastButtonState) {
    lastDebounceTime = currentTime;
  }

  // 디바운싱 시간이 경과한 경우에만 상태 업데이트
  if ((currentTime - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    // 안정된 새로운 상태 감지
    if (reading != buttonState) {
      buttonState = reading;

      // 버튼이 눌린 순간에만 동작 (FALLING: HIGH → LOW)
      if (buttonState == LOW) {
        // 수동 모드로 전환하고 릴레이 토글
        currentMode = MODE_MANUAL;
        toggleRelay("버튼");
      }
    }
  }

  // 다음 루프에서 비교할 이전 읽기값 저장
  lastButtonState = reading;
}

// ===================================================================
//  타이머 모드 처리 함수
// ===================================================================

/*
 * 타이머 모드에서 설정된 간격마다 릴레이를 토글합니다.
 * millis() 기반으로 동작하여 다른 기능을 블로킹하지 않습니다.
 *
 * @param currentTime: 현재 millis() 값
 */
void handleTimerMode(unsigned long currentTime) {
  // 타이머 모드가 아니면 무시
  if (currentMode != MODE_TIMER) return;

  // 설정된 간격이 경과했는지 확인
  if (currentTime - lastTimerToggle >= timerInterval) {
    lastTimerToggle = currentTime;

    // 릴레이 상태 토글
    toggleRelay("타이머");

    // 남은 정보 출력
    Serial.print("  [타이머] 다음 토글까지: ");
    Serial.print(timerInterval / 1000);
    Serial.println("초");
  }
}

// ===================================================================
//  카운트다운 모드 처리 함수
// ===================================================================

/*
 * 카운트다운 모드에서 남은 시간을 추적하고,
 * 시간이 만료되면 릴레이를 자동으로 끕니다.
 *
 * @param currentTime: 현재 millis() 값
 */
void handleCountdownMode(unsigned long currentTime) {
  // 카운트다운 모드가 아니면 무시
  if (currentMode != MODE_COUNTDOWN) return;

  // 경과 시간 계산
  unsigned long elapsed = currentTime - countdownStartTime;

  // 카운트다운 만료 확인
  if (elapsed >= countdownDuration) {
    // 릴레이 끄기
    turnRelayOff("카운트다운 만료");
    currentMode = MODE_MANUAL;

    Serial.println("  [카운트다운] 시간 만료! 릴레이가 꺼졌습니다.");
    Serial.println("  [카운트다운] 수동 제어 모드로 전환.");
    return;
  }

  // 남은 시간 출력 (10초마다)
  static unsigned long lastCountdownPrint = 0;
  if (currentTime - lastCountdownPrint >= 10000) {
    lastCountdownPrint = currentTime;

    unsigned long remaining = (countdownDuration - elapsed) / 1000;
    Serial.print("  [카운트다운] 남은 시간: ");
    Serial.print(remaining);
    Serial.println("초");
  }
}

// ===================================================================
//  안전 타임아웃 검사 함수
// ===================================================================

/*
 * 릴레이가 연속으로 켜져 있는 시간을 확인하고,
 * 안전 타임아웃 시간을 초과하면 강제로 릴레이를 끕니다.
 *
 * 이 기능은 릴레이로 연결된 장치(히터, 펌프 등)가
 * 소프트웨어 오류나 사용자 실수로 장시간 동작하는 것을 방지합니다.
 *
 * @param currentTime: 현재 millis() 값
 */
void checkSafetyTimeout(unsigned long currentTime) {
  // 타임아웃 비활성화 상태이면 무시
  if (SAFETY_TIMEOUT_MS == 0) return;

  // 릴레이가 꺼져 있으면 무시
  if (!relayState) return;

  // 릴레이가 켜진 후 경과 시간 확인
  unsigned long onDuration = currentTime - relayOnStartTime;

  if (onDuration >= SAFETY_TIMEOUT_MS) {
    // 안전 타임아웃 발동!
    turnRelayOff("안전 타임아웃");
    currentMode = MODE_MANUAL;

    Serial.println();
    Serial.println("  ╔═══════════════════════════════════════╗");
    Serial.println("  ║  안전 타임아웃이 발동되었습니다!       ║");
    Serial.print("  ║  릴레이가 ");
    Serial.print(SAFETY_TIMEOUT_MS / 1000);
    Serial.println("초 이상 연속 ON 상태였습니다. ║");
    Serial.println("  ║  과열/화재 방지를 위해 자동 OFF됨     ║");
    Serial.println("  ║  수동으로 다시 ON 할 수 있습니다.      ║");
    Serial.println("  ╚═══════════════════════════════════════╝");
    Serial.println();
  }

  // 타임아웃 5분 전 경고 (타임아웃이 5분 이상인 경우)
  if (SAFETY_TIMEOUT_MS >= 300000) {
    unsigned long warningTime = SAFETY_TIMEOUT_MS - 300000;   // 5분 전
    static bool warningPrinted = false;

    if (onDuration >= warningTime && !warningPrinted) {
      warningPrinted = true;
      Serial.println();
      Serial.println("  [경고] 안전 타임아웃까지 5분 남았습니다!");
      Serial.print("  [경고] ");
      Serial.print((SAFETY_TIMEOUT_MS - onDuration) / 1000);
      Serial.println("초 후 릴레이가 자동으로 꺼집니다.");
      Serial.println();
    }

    // 릴레이가 꺼지면 경고 플래그 리셋
    if (!relayState) {
      warningPrinted = false;
    }
  }
}

// ===================================================================
//  상태 LED 업데이트 함수
// ===================================================================

/*
 * 릴레이 상태에 따라 LED를 업데이트합니다.
 *
 * 동작 방식:
 *   - 수동 모드 + 릴레이 ON:  LED 계속 켜짐
 *   - 수동 모드 + 릴레이 OFF: LED 계속 꺼짐
 *   - 타이머 모드:             LED 빠르게 깜빡임 (릴레이 ON일 때)
 *   - 카운트다운 모드:          LED 느리게 깜빡임 (릴레이 ON일 때)
 */
void updateStatusLED() {
  switch (currentMode) {
    case MODE_MANUAL:
      // 수동 모드: 릴레이 상태와 LED 동기화
      digitalWrite(LED_PIN, relayState ? HIGH : LOW);
      break;

    case MODE_TIMER: {
      // 타이머 모드: 릴레이 ON이면 빠르게 깜빡임 (250ms 간격)
      if (relayState) {
        bool blink = ((millis() / 250) % 2) == 0;
        digitalWrite(LED_PIN, blink ? HIGH : LOW);
      } else {
        digitalWrite(LED_PIN, LOW);
      }
      break;
    }

    case MODE_COUNTDOWN: {
      // 카운트다운 모드: 릴레이 ON이면 느리게 깜빡임 (1초 간격)
      if (relayState) {
        bool blink = ((millis() / 1000) % 2) == 0;
        digitalWrite(LED_PIN, blink ? HIGH : LOW);
      } else {
        digitalWrite(LED_PIN, LOW);
      }
      break;
    }
  }
}

// ===================================================================
//  주기적 상태 출력 함수
// ===================================================================

/*
 * 설정된 주기마다 현재 시스템 상태를 시리얼에 출력합니다.
 * 릴레이가 켜져 있을 때만 출력합니다.
 *
 * @param currentTime: 현재 millis() 값
 */
void handlePeriodicStatus(unsigned long currentTime) {
  // 릴레이가 꺼져 있으면 주기적 출력 불필요
  if (!relayState) return;

  if (currentTime - lastStatusPrint >= STATUS_PRINT_INTERVAL_MS) {
    lastStatusPrint = currentTime;

    Serial.println("  ── 주기적 상태 보고 ──");
    Serial.print("  릴레이: ON (");

    // 릴레이 켜진 시간 출력
    unsigned long onDuration = (currentTime - relayOnStartTime) / 1000;
    printTimeFormatted(onDuration);

    Serial.print(" 경과");

    // 안전 타임아웃까지 남은 시간
    if (SAFETY_TIMEOUT_MS > 0) {
      unsigned long remaining = (SAFETY_TIMEOUT_MS / 1000) - onDuration;
      Serial.print(", 타임아웃까지 ");
      printTimeFormatted(remaining);
    }

    Serial.println(")");
    Serial.print("  모드: ");
    printModeName();
    Serial.println();
    Serial.println("  ────────────────────");
  }
}

// ===================================================================
//  상태 출력 함수
// ===================================================================

/*
 * 현재 시스템의 전체 상태를 상세하게 출력합니다.
 * STATUS 명령에 의해 호출됩니다.
 */
void printStatus() {
  unsigned long currentTime = millis();

  Serial.println();
  Serial.println("  ┌──────────────────────────────────────┐");
  Serial.println("  │         현재 시스템 상태              │");
  Serial.println("  ├──────────────────────────────────────┤");

  // 릴레이 상태
  Serial.print("  │  릴레이:    ");
  Serial.print(relayState ? "ON" : "OFF");
  if (relayState) {
    unsigned long onSec = (currentTime - relayOnStartTime) / 1000;
    Serial.print(" (");
    printTimeFormatted(onSec);
    Serial.print(" 경과)");
  }
  Serial.println();

  // 동작 모드
  Serial.print("  │  동작 모드: ");
  printModeName();
  Serial.println();

  // 모드별 추가 정보
  if (currentMode == MODE_TIMER) {
    Serial.print("  │  타이머 간격: ");
    Serial.print(timerInterval / 1000);
    Serial.println("초");
  }

  if (currentMode == MODE_COUNTDOWN && relayState) {
    unsigned long elapsed = currentTime - countdownStartTime;
    unsigned long remaining = (countdownDuration - elapsed) / 1000;
    Serial.print("  │  카운트다운 남은 시간: ");
    Serial.print(remaining);
    Serial.println("초");
  }

  // 안전 타임아웃
  if (SAFETY_TIMEOUT_MS > 0 && relayState) {
    unsigned long onDuration = currentTime - relayOnStartTime;
    unsigned long remaining = (SAFETY_TIMEOUT_MS - onDuration) / 1000;
    Serial.print("  │  안전 타임아웃까지: ");
    printTimeFormatted(remaining);
    Serial.println();
  }

  // 핀 정보
  Serial.println("  ├──────────────────────────────────────┤");
  Serial.print("  │  릴레이 핀: GPIO");
  Serial.println(RELAY_PIN);
  Serial.print("  │  LED 핀:    GPIO");
  Serial.println(LED_PIN);
  Serial.print("  │  버튼 핀:   GPIO");
  Serial.println(BUTTON_PIN);

  Serial.println("  └──────────────────────────────────────┘");
}

/*
 * 동작 통계를 출력합니다.
 * STATS 명령에 의해 호출됩니다.
 */
void printStats() {
  unsigned long currentTime = millis();
  unsigned long uptime = currentTime / 1000;

  // 릴레이가 현재 켜져있으면 누적 ON 시간에 현재 세션 추가
  unsigned long currentOnTime = totalOnTime;
  if (relayState) {
    currentOnTime += (currentTime - relayOnStartTime);
  }

  Serial.println();
  Serial.println("  ┌──────────────────────────────────────┐");
  Serial.println("  │         동작 통계                     │");
  Serial.println("  ├──────────────────────────────────────┤");

  Serial.print("  │  시스템 가동 시간: ");
  printTimeFormatted(uptime);
  Serial.println();

  Serial.print("  │  릴레이 총 ON 시간: ");
  printTimeFormatted(currentOnTime / 1000);
  Serial.println();

  Serial.print("  │  릴레이 토글 횟수:  ");
  Serial.print(toggleCount);
  Serial.println("회");

  // ON 비율 계산
  if (currentTime > 0) {
    float onPercent = (float)currentOnTime / (float)currentTime * 100.0;
    Serial.print("  │  ON 비율:           ");
    Serial.print(onPercent, 1);
    Serial.println("%");
  }

  Serial.println("  └──────────────────────────────────────┘");
}

// ===================================================================
//  도움말 출력 함수
// ===================================================================

/*
 * 사용 가능한 모든 시리얼 명령과 설명을 출력합니다.
 */
void printHelp() {
  Serial.println("  ┌──────────────────────────────────────────────┐");
  Serial.println("  │          사용 가능한 명령어                   │");
  Serial.println("  ├──────────────────────────────────────────────┤");
  Serial.println("  │  ON           릴레이 켜기                    │");
  Serial.println("  │  OFF          릴레이 끄기                    │");
  Serial.println("  │  TOGGLE       릴레이 상태 반전               │");
  Serial.println("  │  STATUS       현재 상태 표시                 │");
  Serial.println("  │  TIMER n      n초 간격 자동 ON/OFF           │");
  Serial.println("  │  TIMER OFF    타이머 모드 해제               │");
  Serial.println("  │  COUNTDOWN n  n초 후 자동 OFF                │");
  Serial.println("  │  MANUAL       수동 제어 모드 전환            │");
  Serial.println("  │  STATS        동작 통계 표시                 │");
  Serial.println("  │  HELP         이 도움말 표시                 │");
  Serial.println("  ├──────────────────────────────────────────────┤");
  Serial.println("  │  버튼: GPIO27을 누르면 릴레이 토글           │");
  Serial.println("  └──────────────────────────────────────────────┘");
}

// ===================================================================
//  유틸리티 함수
// ===================================================================

/*
 * 현재 동작 모드 이름을 시리얼에 출력합니다.
 */
void printModeName() {
  switch (currentMode) {
    case MODE_MANUAL:
      Serial.print("수동 제어");
      break;
    case MODE_TIMER:
      Serial.print("타이머 (");
      Serial.print(timerInterval / 1000);
      Serial.print("초 간격)");
      break;
    case MODE_COUNTDOWN:
      Serial.print("카운트다운");
      break;
    default:
      Serial.print("알 수 없음");
      break;
  }
}

/*
 * 초 단위 시간을 사람이 읽기 쉬운 형식으로 출력합니다.
 * 예: 3661초 → "1시간 1분 1초"
 *
 * @param totalSeconds: 출력할 시간 (초 단위)
 */
void printTimeFormatted(unsigned long totalSeconds) {
  unsigned long hours   = totalSeconds / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;

  if (hours > 0) {
    Serial.print(hours);
    Serial.print("시간 ");
  }
  if (hours > 0 || minutes > 0) {
    Serial.print(minutes);
    Serial.print("분 ");
  }
  Serial.print(seconds);
  Serial.print("초");
}

// ===================================================================
//  끝 (End of File)
// ===================================================================
