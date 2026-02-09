/*
 * ============================================================
 *  SPI SD 카드 읽기/쓰기 예제
 * ============================================================
 *
 *  기능 설명:
 *  - SPI 통신으로 MicroSD 카드 모듈에 접근합니다
 *  - SD 카드에 텍스트 파일을 생성하고 데이터를 기록합니다
 *  - 기록한 파일을 다시 읽어서 시리얼 모니터에 출력합니다
 *  - SD 카드 정보 (용량, 파일 목록)를 출력합니다
 *  - 센서 데이터 로깅 예제를 포함합니다
 *
 *  배선 안내 (Wiring Guide):
 *  ┌──────────────────────────────────────────────────┐
 *  │                                                  │
 *  │  ESP32              MicroSD 카드 모듈            │
 *  │  ─────              ────────────────             │
 *  │                                                  │
 *  │  3.3V ──────────── VCC (3.3V)                    │
 *  │  GND  ──────────── GND                           │
 *  │  GPIO 23 (MOSI) ── MOSI (DI)                    │
 *  │  GPIO 19 (MISO) ── MISO (DO)                    │
 *  │  GPIO 18 (SCK)  ── SCK  (CLK)                   │
 *  │  GPIO 5  (CS)   ── CS   (CD)                    │
 *  │                                                  │
 *  │  ※ 위 핀 번호는 ESP32의 기본 VSPI 핀입니다      │
 *  │  ※ MicroSD 모듈의 핀 이름은 제조사마다 다름:     │
 *  │    MOSI=DI=SDI, MISO=DO=SDO, SCK=CLK=SCLK      │
 *  │                                                  │
 *  │  ※ SD 카드는 반드시 FAT16 또는 FAT32로          │
 *  │    포맷되어 있어야 합니다!                        │
 *  │                                                  │
 *  │  SPI 통신 구조:                                  │
 *  │                                                  │
 *  │  ESP32 ── MOSI ──────▶ SD (데이터 전송)          │
 *  │  ESP32 ◀── MISO ────── SD (데이터 수신)          │
 *  │  ESP32 ── SCK  ──────▶ SD (클럭 동기화)          │
 *  │  ESP32 ── CS   ──────▶ SD (칩 선택)              │
 *  │                                                  │
 *  └──────────────────────────────────────────────────┘
 *
 *  필요 부품:
 *  - ESP32 개발보드
 *  - MicroSD 카드 모듈 (SPI 방식)
 *  - MicroSD 카드 (FAT32 포맷, 32GB 이하 권장)
 *  - 브레드보드 및 점퍼 와이어
 *
 *  시리얼 모니터 설정: 115200 baud
 * ============================================================
 */

#include <SPI.h>   // SPI 통신 라이브러리
#include <SD.h>    // SD 카드 라이브러리
#include <FS.h>    // 파일 시스템 라이브러리

// ─────────────── SPI 핀 정의 ───────────────
// ESP32 기본 VSPI 핀 사용
const int CS_PIN   = 5;    // Chip Select (SS) 핀
// MOSI = GPIO 23 (고정, SPI 라이브러리 내부 설정)
// MISO = GPIO 19 (고정)
// SCK  = GPIO 18 (고정)

// ─────────────── 파일 경로 정의 ───────────────
const char* LOG_FILE = "/sensor_log.txt";    // 센서 데이터 로그 파일
const char* TEST_FILE = "/test_data.txt";    // 테스트 파일

// ─────────────── 초기 설정 ───────────────
void setup() {
    // 시리얼 통신 초기화
    Serial.begin(115200);
    delay(1000);  // 시리얼 안정화 대기

    Serial.println();
    Serial.println("=================================");
    Serial.println("  ESP32 SPI SD 카드 예제");
    Serial.println("=================================");
    Serial.println();

    // ──── SD 카드 초기화 ────
    Serial.println("[1단계] SD 카드 초기화 중...");

    // SD.begin(CS핀): SD 카드 초기화
    // CS 핀을 LOW로 만들어 SD 카드를 선택하고 SPI 통신 시작
    if (!SD.begin(CS_PIN)) {
        Serial.println("  *** SD 카드 초기화 실패! ***");
        Serial.println("  확인 사항:");
        Serial.println("    1. SD 카드가 삽입되어 있는지 확인");
        Serial.println("    2. SD 카드가 FAT32로 포맷되었는지 확인");
        Serial.println("    3. 배선(MOSI/MISO/SCK/CS)이 올바른지 확인");
        Serial.println("    4. SD 카드 모듈의 전원(3.3V/GND)이 연결되었는지 확인");
        Serial.println();
        Serial.println("  프로그램을 중단합니다.");
        while (1) {
            delay(1000);  // 무한 대기
        }
    }
    Serial.println("  SD 카드 초기화 성공!");

    // ──── SD 카드 정보 출력 ────
    Serial.println();
    Serial.println("[2단계] SD 카드 정보:");
    printCardInfo();

    // ──── 파일 쓰기 테스트 ────
    Serial.println();
    Serial.println("[3단계] 파일 쓰기 테스트...");
    writeTestFile();

    // ──── 파일 읽기 테스트 ────
    Serial.println();
    Serial.println("[4단계] 파일 읽기 테스트...");
    readTestFile();

    // ──── 파일에 데이터 추가 ────
    Serial.println();
    Serial.println("[5단계] 파일에 데이터 추가...");
    appendToFile();

    // ──── 추가 후 다시 읽기 ────
    Serial.println();
    Serial.println("[6단계] 추가 후 파일 읽기...");
    readTestFile();

    // ──── 루트 디렉토리 파일 목록 ────
    Serial.println();
    Serial.println("[7단계] SD 카드 파일 목록:");
    listDirectory("/", 0);

    Serial.println();
    Serial.println("=================================");
    Serial.println("  테스트 완료! 센서 로깅 시작...");
    Serial.println("=================================");
    Serial.println();
}

// ─────────────── 메인 루프 ───────────────
void loop() {
    // 주기적으로 센서 데이터를 SD 카드에 기록하는 예제
    static unsigned long lastLogTime = 0;
    const unsigned long LOG_INTERVAL = 5000;  // 5초마다 기록

    if (millis() - lastLogTime >= LOG_INTERVAL) {
        lastLogTime = millis();

        // 가상 센서 데이터 생성 (실제로는 센서에서 읽어야 함)
        float temperature = 20.0 + random(0, 150) / 10.0;  // 20.0~35.0
        float humidity = 40.0 + random(0, 400) / 10.0;     // 40.0~80.0
        unsigned long timestamp = millis() / 1000;          // 초 단위 시간

        // 로그 데이터 생성
        char logEntry[100];
        snprintf(logEntry, sizeof(logEntry),
                 "%lu초, 온도: %.1f°C, 습도: %.1f%%",
                 timestamp, temperature, humidity);

        // SD 카드에 기록
        logToSD(LOG_FILE, logEntry);

        // 시리얼 모니터에도 출력
        Serial.printf("[로그] %s\n", logEntry);
    }
}

// ─────────────── SD 카드 정보 출력 함수 ───────────────
/**
 * SD 카드의 타입과 용량 정보를 출력하는 함수
 */
void printCardInfo() {
    // 카드 타입 확인
    uint8_t cardType = SD.cardType();
    Serial.print("  카드 타입: ");
    switch (cardType) {
        case CARD_NONE:  Serial.println("없음 (카드 미삽입)"); return;
        case CARD_MMC:   Serial.println("MMC");   break;
        case CARD_SD:    Serial.println("SDSC");   break;
        case CARD_SDHC:  Serial.println("SDHC");   break;
        default:         Serial.println("알 수 없음"); break;
    }

    // 카드 용량 출력
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);  // MB 단위
    uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
    uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);

    Serial.printf("  카드 크기: %llu MB\n", cardSize);
    Serial.printf("  전체 공간: %llu MB\n", totalBytes);
    Serial.printf("  사용 공간: %llu MB\n", usedBytes);
    Serial.printf("  남은 공간: %llu MB\n", totalBytes - usedBytes);
}

// ─────────────── 파일 쓰기 함수 ───────────────
/**
 * 테스트 파일을 생성하고 데이터를 기록하는 함수
 * FILE_WRITE 모드: 기존 파일이 있으면 덮어쓰기
 */
void writeTestFile() {
    // FILE_WRITE: 파일이 있으면 열고 끝에 추가, 없으면 새로 생성
    // 여기서는 먼저 삭제 후 새로 만들어 덮어쓰기 효과
    if (SD.exists(TEST_FILE)) {
        SD.remove(TEST_FILE);  // 기존 파일 삭제
    }

    File file = SD.open(TEST_FILE, FILE_WRITE);

    if (file) {
        // 파일에 데이터 쓰기
        file.println("=== ESP32 SD 카드 테스트 ===");
        file.println("이 파일은 ESP32에서 생성되었습니다.");
        file.println();
        file.println("테스트 데이터:");
        file.printf("  정수: %d\n", 12345);
        file.printf("  실수: %.2f\n", 3.14);
        file.printf("  문자열: %s\n", "안녕하세요!");
        file.println();
        file.println("=== 테스트 끝 ===");

        file.close();  // 파일 닫기 (반드시!)

        Serial.println("  파일 쓰기 성공: " + String(TEST_FILE));
    } else {
        Serial.println("  *** 파일 열기 실패! ***");
    }
}

// ─────────────── 파일 읽기 함수 ───────────────
/**
 * 테스트 파일의 내용을 읽어서 시리얼 모니터에 출력하는 함수
 */
void readTestFile() {
    File file = SD.open(TEST_FILE);

    if (file) {
        Serial.println("  --- 파일 내용 시작 ---");

        // 파일에서 한 줄씩 읽기
        while (file.available()) {
            String line = file.readStringUntil('\n');
            Serial.println("  | " + line);
        }

        Serial.println("  --- 파일 내용 끝 ---");
        Serial.printf("  파일 크기: %d 바이트\n", file.size());

        file.close();  // 파일 닫기
    } else {
        Serial.println("  *** 파일을 찾을 수 없습니다! ***");
    }
}

// ─────────────── 파일 추가 쓰기 함수 ───────────────
/**
 * 기존 파일 끝에 데이터를 추가하는 함수
 * FILE_APPEND 모드: 기존 내용을 유지하고 끝에 추가
 */
void appendToFile() {
    File file = SD.open(TEST_FILE, FILE_APPEND);

    if (file) {
        file.println();
        file.println("--- 추가된 데이터 ---");
        file.printf("기록 시각: %lu ms (부팅 후 경과)\n", millis());
        file.println("이 줄은 나중에 추가되었습니다.");

        file.close();
        Serial.println("  데이터 추가 성공!");
    } else {
        Serial.println("  *** 파일 추가 열기 실패! ***");
    }
}

// ─────────────── 센서 데이터 로깅 함수 ───────────────
/**
 * 센서 데이터를 SD 카드의 로그 파일에 기록하는 함수
 *
 * @param path    로그 파일 경로 (예: "/sensor_log.txt")
 * @param message 기록할 메시지
 */
void logToSD(const char* path, const char* message) {
    File file = SD.open(path, FILE_APPEND);

    if (file) {
        file.println(message);
        file.close();
    } else {
        Serial.println("  *** 로그 파일 열기 실패! ***");
    }
}

// ─────────────── 디렉토리 목록 출력 함수 ───────────────
/**
 * 지정된 디렉토리의 파일 목록을 재귀적으로 출력하는 함수
 *
 * @param dirname  디렉토리 경로
 * @param levels   재귀 깊이 (하위 디렉토리 탐색 레벨)
 */
void listDirectory(const char* dirname, uint8_t levels) {
    File root = SD.open(dirname);

    if (!root) {
        Serial.println("  *** 디렉토리 열기 실패! ***");
        return;
    }

    if (!root.isDirectory()) {
        Serial.println("  *** 디렉토리가 아닙니다! ***");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        // 들여쓰기
        for (uint8_t i = 0; i < levels; i++) {
            Serial.print("    ");
        }

        if (file.isDirectory()) {
            // 디렉토리인 경우
            Serial.printf("  [폴더] %s/\n", file.name());
            if (levels > 0) {
                listDirectory(file.name(), levels - 1);  // 재귀 탐색
            }
        } else {
            // 파일인 경우
            Serial.printf("  [파일] %-20s  (%d 바이트)\n",
                          file.name(), file.size());
        }

        file = root.openNextFile();
    }

    root.close();
}

/*
 * ============================================================
 *  SPI 통신 원리 (SD 카드)
 * ============================================================
 *
 *  SD 카드는 SPI 모드 0 (CPOL=0, CPHA=0)으로 통신합니다.
 *
 *  통신 흐름:
 *  1. CS 핀을 LOW로 → SD 카드 선택
 *  2. MOSI로 명령 전송
 *  3. MISO로 응답 수신
 *  4. CS 핀을 HIGH로 → SD 카드 해제
 *
 *  SD 라이브러리가 이 과정을 추상화하므로
 *  사용자는 File.read(), File.write()만 호출하면 됩니다.
 *
 * ============================================================
 *  주의사항
 * ============================================================
 *
 *  1. 파일 닫기: file.close()를 반드시 호출하세요!
 *     닫지 않으면 데이터가 기록되지 않을 수 있습니다.
 *
 *  2. 파일 이름: 8.3 형식 제한이 있을 수 있습니다.
 *     (예: "data.txt" OK, "very_long_filename.txt" 확인 필요)
 *
 *  3. 전원 차단: 쓰기 중 전원이 꺼지면 파일이 손상될 수 있습니다.
 *     중요한 데이터는 쓰기 후 바로 close()하세요.
 *
 *  4. SD 카드 수명: 플래시 메모리는 쓰기 횟수 제한이 있습니다.
 *     너무 자주 쓰기하면 카드 수명이 단축됩니다.
 *     (1초마다 쓰기보다는 1분마다 쓰기를 권장)
 *
 * ============================================================
 */
