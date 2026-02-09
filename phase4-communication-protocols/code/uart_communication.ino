/*
 * ============================================================
 *  UART2 외부 장치 시리얼 통신 예제
 * ============================================================
 *
 *  기능 설명:
 *  - ESP32의 UART2를 사용하여 외부 장치와 시리얼 통신합니다
 *  - UART0(USB 시리얼)으로 디버깅 메시지를 출력합니다
 *  - UART2로 수신된 데이터를 시리얼 모니터에 표시합니다
 *  - 시리얼 모니터에서 입력한 명령을 UART2로 전송합니다
 *  - 간단한 명령줄 인터페이스(CLI)를 제공합니다
 *  - GPS 모듈이나 다른 UART 장치와 통신 가능합니다
 *
 *  배선 안내 (Wiring Guide):
 *  ┌──────────────────────────────────────────────────┐
 *  │                                                  │
 *  │  ESP32              외부 UART 장치               │
 *  │  ─────              ──────────────               │
 *  │                                                  │
 *  │  GPIO 17 (TX2) ──── RX   ← ESP32 송신→장치 수신 │
 *  │  GPIO 16 (RX2) ──── TX   ← 장치 송신→ESP32 수신 │
 *  │  GND ───────────── GND   ← 반드시 공유!         │
 *  │                                                  │
 *  │  ※ TX와 RX를 반드시 교차 연결!                   │
 *  │    ESP32 TX → 장치 RX                            │
 *  │    ESP32 RX ← 장치 TX                            │
 *  │                                                  │
 *  │  ※ 전압 레벨 주의!                               │
 *  │    ESP32 = 3.3V 로직                             │
 *  │    5V 장치 연결 시 레벨 시프터 필요               │
 *  │                                                  │
 *  │  ┌────────┐          ┌────────────┐              │
 *  │  │ ESP32  │          │ 외부 장치  │              │
 *  │  │        │   TX ──▶│ RX         │              │
 *  │  │        │          │            │              │
 *  │  │        │   RX ◀──│ TX         │              │
 *  │  │        │          │            │              │
 *  │  │        │   GND ───│ GND        │              │
 *  │  └────────┘          └────────────┘              │
 *  │                                                  │
 *  │  호환 장치 예시:                                 │
 *  │  - GPS 모듈 (NEO-6M): 기본 9600 baud            │
 *  │  - 블루투스 모듈 (HC-05): 기본 9600 baud         │
 *  │  - CO2 센서 (MH-Z19B): 기본 9600 baud           │
 *  │  - 다른 ESP32/Arduino: 115200 baud               │
 *  │                                                  │
 *  └──────────────────────────────────────────────────┘
 *
 *  필요 부품:
 *  - ESP32 개발보드
 *  - UART 장치 1개 (GPS, 블루투스, 다른 마이크로컨트롤러 등)
 *  - 점퍼 와이어 3개 (TX, RX, GND)
 *
 *  시리얼 모니터 설정: 115200 baud, "Both NL & CR" (줄바꿈 모두)
 * ============================================================
 */

// ─────────────── UART 핀 정의 ───────────────
const int UART2_TX = 17;   // UART2 송신 핀 (ESP32 기본값)
const int UART2_RX = 16;   // UART2 수신 핀 (ESP32 기본값)

// ─────────────── UART 설정 ───────────────
const long USB_BAUD = 115200;    // UART0 (USB) 보레이트: 디버깅용
const long UART2_BAUD = 9600;    // UART2 보레이트: 외부 장치에 맞춤
                                  // GPS, 블루투스 등 대부분 9600 기본

// ─────────────── 수신 버퍼 ───────────────
String uart2Buffer = "";          // UART2에서 수신한 데이터 버퍼
const int MAX_BUFFER_SIZE = 256;  // 최대 버퍼 크기

// ─────────────── 통계 변수 ───────────────
unsigned long rxCount = 0;        // 수신 바이트 카운터
unsigned long txCount = 0;        // 송신 바이트 카운터
unsigned long startTime = 0;      // 시작 시각

// ─────────────── 초기 설정 ───────────────
void setup() {
    // ──── UART0 초기화: USB 시리얼 (디버깅용) ────
    Serial.begin(USB_BAUD);
    delay(500);  // 시리얼 안정화 대기

    // ──── UART2 초기화: 외부 장치 통신용 ────
    // Serial2.begin(보레이트, 프로토콜, RX핀, TX핀)
    // SERIAL_8N1 = 8비트 데이터 + 패리티 없음 + 1 정지비트 (가장 일반적)
    Serial2.begin(UART2_BAUD, SERIAL_8N1, UART2_RX, UART2_TX);

    startTime = millis();

    // 시작 메시지 출력
    Serial.println();
    Serial.println("=============================================");
    Serial.println("  ESP32 UART2 시리얼 통신 예제");
    Serial.println("=============================================");
    Serial.println();
    Serial.println("UART0 (USB 시리얼 모니터):");
    Serial.printf("  보레이트: %ld bps\n", USB_BAUD);
    Serial.printf("  용도: 디버깅 및 명령 입력\n");
    Serial.println();
    Serial.println("UART2 (외부 장치 연결):");
    Serial.printf("  보레이트: %ld bps\n", UART2_BAUD);
    Serial.printf("  TX: GPIO %d → 장치의 RX에 연결\n", UART2_TX);
    Serial.printf("  RX: GPIO %d ← 장치의 TX에 연결\n", UART2_RX);
    Serial.printf("  프로토콜: 8N1 (8데이터+패리티없음+1정지)\n");
    Serial.println();
    Serial.println("─────────────────────────────────────────────");
    Serial.println("  사용 가능한 명령:");
    Serial.println("  help    - 도움말 표시");
    Serial.println("  status  - 통신 상태 확인");
    Serial.println("  clear   - 화면 정리");
    Serial.println("  echo    - 에코 테스트 (UART2로 전송 후 수신 대기)");
    Serial.println("  그 외   - 입력한 텍스트를 UART2로 전송");
    Serial.println("─────────────────────────────────────────────");
    Serial.println();
    Serial.println("대기 중... (외부 장치 데이터 수신 또는 명령 입력)");
    Serial.println();
}

// ─────────────── 메인 루프 ───────────────
void loop() {
    // ──── 1. UART2에서 데이터 수신 처리 ────
    handleUART2Receive();

    // ──── 2. USB 시리얼(UART0)에서 명령 입력 처리 ────
    handleUSBInput();
}

// ─────────────── UART2 수신 처리 함수 ───────────────
/**
 * UART2에서 수신된 데이터를 읽어서 처리하는 함수
 * 줄 단위(\n)로 데이터를 버퍼링한 후 출력합니다
 */
void handleUART2Receive() {
    while (Serial2.available()) {
        char c = Serial2.read();  // 1바이트 읽기
        rxCount++;                // 수신 카운터 증가

        if (c == '\n') {
            // 한 줄 완성 → 출력 및 처리
            uart2Buffer.trim();  // 앞뒤 공백/개행 제거

            if (uart2Buffer.length() > 0) {
                // 타임스탬프와 함께 수신 데이터 출력
                unsigned long elapsed = (millis() - startTime) / 1000;
                Serial.printf("[UART2 수신] [%lus] %s\n",
                              elapsed, uart2Buffer.c_str());

                // 수신 데이터 파싱 (필요에 따라 처리)
                parseReceivedData(uart2Buffer);
            }

            uart2Buffer = "";  // 버퍼 초기화

        } else if (c != '\r') {
            // 버퍼에 문자 추가 (\r은 무시)
            if (uart2Buffer.length() < MAX_BUFFER_SIZE) {
                uart2Buffer += c;
            }
        }
    }
}

// ─────────────── USB 시리얼 입력 처리 함수 ───────────────
/**
 * 시리얼 모니터에서 입력한 텍스트를 처리하는 함수
 * 내부 명령이면 실행하고, 그 외에는 UART2로 전송합니다
 */
void handleUSBInput() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();  // 앞뒤 공백/개행 제거

        if (input.length() == 0) return;  // 빈 입력 무시

        // ──── 내부 명령 처리 ────
        if (input.equalsIgnoreCase("help")) {
            printHelp();

        } else if (input.equalsIgnoreCase("status")) {
            printStatus();

        } else if (input.equalsIgnoreCase("clear")) {
            // 시리얼 모니터 화면 정리 (ANSI 이스케이프 코드)
            Serial.print("\033[2J\033[H");
            Serial.println("화면이 정리되었습니다.\n");

        } else if (input.equalsIgnoreCase("echo")) {
            // 에코 테스트: "ECHO_TEST"를 보내고 응답 대기
            String testMsg = "ECHO_TEST";
            Serial.printf("[에코 테스트] UART2로 '%s' 전송 중...\n",
                          testMsg.c_str());
            sendToUART2(testMsg);
            Serial.println("  응답을 기다리는 중...");

        } else {
            // ──── 일반 텍스트 → UART2로 전송 ────
            Serial.printf("[UART2 전송] → %s\n", input.c_str());
            sendToUART2(input);
        }
    }
}

// ─────────────── UART2로 데이터 전송 함수 ───────────────
/**
 * UART2를 통해 외부 장치로 문자열을 전송하는 함수
 *
 * @param message  전송할 문자열
 */
void sendToUART2(String message) {
    Serial2.println(message);       // 줄바꿈(\n) 포함하여 전송
    txCount += message.length() + 1; // 전송 카운터 갱신 (+1은 줄바꿈)
}

// ─────────────── 수신 데이터 파싱 함수 ───────────────
/**
 * UART2에서 수신된 데이터를 분석하는 함수
 * GPS NMEA 데이터 등을 파싱할 수 있습니다
 *
 * @param data  수신된 문자열
 */
void parseReceivedData(String data) {
    // ──── GPS NMEA 문장 파싱 예시 ────
    // GPS 모듈은 NMEA 형식의 문자열을 전송합니다:
    // $GPGGA,123456.00,3724.0000,N,12700.0000,E,1,08,...
    // $GPRMC,123456.00,A,3724.0000,N,12700.0000,E,...

    if (data.startsWith("$GPGGA") || data.startsWith("$GNGGA")) {
        Serial.println("  → GPS 위치 정보 (GGA) 감지");
        // 실제 파싱은 TinyGPS++ 라이브러리 사용을 권장

    } else if (data.startsWith("$GPRMC") || data.startsWith("$GNRMC")) {
        Serial.println("  → GPS 이동 정보 (RMC) 감지");

    } else if (data.startsWith("$")) {
        Serial.println("  → NMEA 데이터 감지");
    }

    // ──── 간단한 키-값 파싱 예시 ────
    // "TEMP:25.3" 형식의 데이터 파싱
    int separatorIndex = data.indexOf(':');
    if (separatorIndex > 0) {
        String key = data.substring(0, separatorIndex);
        String value = data.substring(separatorIndex + 1);

        key.trim();
        value.trim();

        if (key.equalsIgnoreCase("TEMP")) {
            float temp = value.toFloat();
            Serial.printf("  → 온도 데이터 파싱: %.1f°C\n", temp);
        } else if (key.equalsIgnoreCase("HUM")) {
            float hum = value.toFloat();
            Serial.printf("  → 습도 데이터 파싱: %.1f%%\n", hum);
        }
    }
}

// ─────────────── 도움말 출력 함수 ───────────────
void printHelp() {
    Serial.println();
    Serial.println("=============================================");
    Serial.println("  UART 통신 예제 - 도움말");
    Serial.println("=============================================");
    Serial.println();
    Serial.println("  명령어 목록:");
    Serial.println("  ─────────────────────────────────────────");
    Serial.println("  help    : 이 도움말을 표시합니다");
    Serial.println("  status  : 통신 통계 및 상태를 표시합니다");
    Serial.println("  clear   : 시리얼 모니터 화면을 정리합니다");
    Serial.println("  echo    : UART2로 에코 테스트를 수행합니다");
    Serial.println("  [텍스트] : 입력한 텍스트를 UART2로 전송합니다");
    Serial.println();
    Serial.println("  수신된 데이터는 자동으로 화면에 표시됩니다.");
    Serial.println("  GPS 모듈(NMEA) 데이터는 자동으로 인식됩니다.");
    Serial.println("  'KEY:VALUE' 형식의 데이터는 자동 파싱됩니다.");
    Serial.println("=============================================");
    Serial.println();
}

// ─────────────── 통신 상태 출력 함수 ───────────────
void printStatus() {
    unsigned long elapsed = (millis() - startTime) / 1000;
    unsigned long minutes = elapsed / 60;
    unsigned long seconds = elapsed % 60;

    Serial.println();
    Serial.println("─────── 통신 상태 ───────");
    Serial.printf("  경과 시간: %lu분 %lu초\n", minutes, seconds);
    Serial.printf("  UART2 보레이트: %ld bps\n", UART2_BAUD);
    Serial.printf("  총 수신: %lu 바이트\n", rxCount);
    Serial.printf("  총 송신: %lu 바이트\n", txCount);
    Serial.printf("  UART2 수신 버퍼 대기: %d 바이트\n", Serial2.available());
    Serial.println("─────────────────────────");
    Serial.println();
}

/*
 * ============================================================
 *  UART 통신 원리 설명
 * ============================================================
 *
 *  UART = Universal Asynchronous Receiver/Transmitter
 *         범용 비동기 송수신기
 *
 *  "비동기"란?
 *  - I2C, SPI와 달리 별도의 클럭(SCK/SCL) 신호가 없습니다.
 *  - 양쪽이 미리 약속한 속도(보레이트)로 통신합니다.
 *  - 보레이트가 다르면 데이터가 깨져서 읽을 수 없습니다!
 *
 *  데이터 프레임 (8N1):
 *  ┌─────┬──┬──┬──┬──┬──┬──┬──┬──┬─────┐
 *  │시작 │D0│D1│D2│D3│D4│D5│D6│D7│정지 │
 *  │비트 │  │  │  │  │  │  │  │  │비트 │
 *  └─────┴──┴──┴──┴──┴──┴──┴──┴──┴─────┘
 *   LOW    ◀── 8비트 데이터 ──▶    HIGH
 *
 *  8N1 = 8 데이터비트 + No 패리티 + 1 정지비트
 *
 * ============================================================
 *  ESP32 UART 포트
 * ============================================================
 *
 *  UART0: Serial  (GPIO 1=TX, GPIO 3=RX)
 *    → USB를 통한 PC 통신 (시리얼 모니터)
 *    → 프로그래밍에도 사용됨
 *
 *  UART1: Serial1 (기본 핀이 플래시 메모리와 충돌!)
 *    → 반드시 다른 핀으로 재할당해서 사용:
 *      Serial1.begin(9600, SERIAL_8N1, 25, 26);
 *
 *  UART2: Serial2 (GPIO 16=RX, GPIO 17=TX)
 *    → 외부 장치 연결에 가장 많이 사용
 *    → 이 예제에서 사용하는 포트
 *
 * ============================================================
 *  문제 해결 가이드
 * ============================================================
 *
 *  1. 데이터가 깨져서 보일 때 (알 수 없는 문자):
 *     → 보레이트가 양쪽 장치에서 같은지 확인!
 *     → 9600, 115200 등 일반적인 값을 시도해 보세요.
 *
 *  2. 데이터가 전혀 안 올 때:
 *     → TX↔RX 교차 연결 확인 (가장 흔한 실수!)
 *     → GND 공유 확인
 *     → 장치 전원 확인
 *
 *  3. 일부 문자만 누락될 때:
 *     → 수신 버퍼 오버플로 가능성
 *     → loop()에서 Serial2.available() 빠르게 확인
 *     → Serial2.setRxBufferSize(1024)로 버퍼 확장
 *
 * ============================================================
 */
