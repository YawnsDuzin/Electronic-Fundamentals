/*
 * ============================================================
 *  ADC 아날로그 값 읽기 + 전압 변환 예제
 * ============================================================
 *
 *  기능 설명:
 *  - ADC 핀에서 아날로그 값 (0~4095)을 읽습니다
 *  - 읽은 값을 실제 전압 (0~3.3V)으로 변환합니다
 *  - 다중 샘플 평균으로 노이즈를 줄입니다
 *  - 시리얼 모니터에 ADC 값, 전압, 백분율을 출력합니다
 *  - 시리얼 플로터(Serial Plotter)용 데이터도 출력합니다
 *
 *  배선 안내 (Wiring Guide):
 *  ┌──────────────────────────────────────────────────┐
 *  │                                                  │
 *  │  ESP32           가변저항 (10kΩ)                 │
 *  │  ─────           ──────────────                  │
 *  │                                                  │
 *  │  3.3V ─────────── 한쪽 끝                        │
 *  │                                                  │
 *  │  GPIO 34 ──────── 가운데 핀 (와이퍼)             │
 *  │  (ADC1_CH6)                                      │
 *  │                                                  │
 *  │  GND ──────────── 다른쪽 끝                      │
 *  │                                                  │
 *  │  ※ GPIO 34는 ADC1에 속하므로 WiFi와 무관하게     │
 *  │    안전하게 사용 가능                             │
 *  │  ※ GPIO 34는 입력 전용 핀 (출력 불가)            │
 *  │                                                  │
 *  │  가변저항 연결도:                                 │
 *  │                                                  │
 *  │    3.3V                                          │
 *  │     │                                            │
 *  │     ┤ [가변저항 10kΩ]                            │
 *  │     │     ↕ 회전                                 │
 *  │     ├──── GPIO 34                                │
 *  │     │                                            │
 *  │     ┤                                            │
 *  │     │                                            │
 *  │    GND                                           │
 *  │                                                  │
 *  └──────────────────────────────────────────────────┘
 *
 *  필요 부품:
 *  - ESP32 개발보드
 *  - 가변저항(포텐셔미터) 10kΩ 1개
 *  - 브레드보드 및 점퍼 와이어
 *
 *  시리얼 모니터 설정: 115200 baud
 * ============================================================
 */

// ─────────────── 핀 정의 ───────────────
const int ADC_PIN = 34;     // 아날로그 입력 핀 (GPIO 34 = ADC1_CH6)
                            // ADC1 핀이므로 WiFi 사용 시에도 정상 동작

// ─────────────── ADC 설정 ───────────────
const float ADC_MAX = 4095.0;    // ESP32 ADC 최대값 (12비트 해상도)
const float VREF = 3.3;          // 기준 전압 (3.3V)
const int SAMPLE_COUNT = 64;     // 평균을 낼 샘플 수 (노이즈 감소용)

// ─────────────── 타이밍 설정 ───────────────
const unsigned long READ_INTERVAL = 500;  // 읽기 간격 (밀리초)
unsigned long lastReadTime = 0;           // 마지막 읽기 시각

// ─────────────── 초기 설정 ───────────────
void setup() {
    // 시리얼 통신 초기화
    Serial.begin(115200);

    // ADC 해상도 설정 (ESP32 기본값: 12비트)
    // analogReadResolution(12);  // 0~4095 (기본값이므로 생략 가능)

    // ADC 감쇠(Attenuation) 설정
    // ADC_11db: 0~3.3V 범위 (기본값)
    // ADC_0db: 0~1.1V, ADC_2_5db: 0~1.5V, ADC_6db: 0~2.2V
    analogSetAttenuation(ADC_11db);

    // 시작 메시지
    Serial.println("=================================");
    Serial.println("  ESP32 ADC 아날로그 읽기 예제");
    Serial.println("=================================");
    Serial.println();
    Serial.println("ADC 핀: GPIO 34 (ADC1_CH6)");
    Serial.println("해상도: 12비트 (0~4095)");
    Serial.println("입력 범위: 0~3.3V");
    Serial.printf("샘플 평균: %d회\n", SAMPLE_COUNT);
    Serial.println();
    Serial.println("가변저항을 돌려보세요!");
    Serial.println("---------------------------------");
}

// ─────────────── 메인 루프 ───────────────
void loop() {
    // 일정 간격으로 ADC 읽기 (논블로킹 방식)
    unsigned long currentTime = millis();
    if (currentTime - lastReadTime >= READ_INTERVAL) {
        lastReadTime = currentTime;

        // ─── 방법 1: 단순 읽기 ───
        int rawValue = analogRead(ADC_PIN);

        // ─── 방법 2: 다중 샘플 평균 (노이즈 감소) ───
        int averagedValue = readADC_Averaged(ADC_PIN, SAMPLE_COUNT);

        // ─── 전압 변환 ───
        // 공식: 전압 = ADC값 × (기준전압 / ADC최대값)
        float rawVoltage = rawValue * (VREF / ADC_MAX);
        float avgVoltage = averagedValue * (VREF / ADC_MAX);

        // ─── 백분율 계산 ───
        float percentage = (averagedValue / ADC_MAX) * 100.0;

        // ─── 시리얼 모니터 출력 ───
        Serial.println("---------------------------------");
        Serial.printf("  원시 ADC 값:    %4d  (전압: %.3fV)\n", rawValue, rawVoltage);
        Serial.printf("  평균 ADC 값:    %4d  (전압: %.3fV)\n", averagedValue, avgVoltage);
        Serial.printf("  백분율:         %.1f%%\n", percentage);

        // 전압 레벨을 막대 그래프로 시각화
        printBar(averagedValue, ADC_MAX);

        // ─── 시리얼 플로터용 출력 (선택) ───
        // Arduino IDE의 시리얼 플로터에서 그래프로 확인 가능
        // 아래 줄의 주석을 해제하면 플로터 형식으로 출력
        // Serial.printf("Raw:%d,Averaged:%d\n", rawValue, averagedValue);
    }
}

// ─────────────── 다중 샘플 평균 함수 ───────────────
/**
 * 여러 번 ADC를 읽어서 평균값을 반환하는 함수
 * 노이즈와 ADC 변동을 줄여줍니다
 *
 * @param pin    ADC 핀 번호
 * @param samples 샘플 수 (많을수록 안정적이지만 느림)
 * @return       평균 ADC 값 (0~4095)
 */
int readADC_Averaged(int pin, int samples) {
    long sum = 0;  // 합계 (오버플로 방지를 위해 long 타입)

    for (int i = 0; i < samples; i++) {
        sum += analogRead(pin);      // ADC 값 누적
        delayMicroseconds(100);      // 샘플 사이에 짧은 대기
                                     // (ADC 내부 커패시터 충전 시간)
    }

    return (int)(sum / samples);     // 평균값 반환
}

// ─────────────── 막대 그래프 출력 함수 ───────────────
/**
 * ADC 값을 시각적 막대 그래프로 출력하는 함수
 *
 * @param value  현재 ADC 값
 * @param maxVal 최대 ADC 값 (4095)
 */
void printBar(int value, float maxVal) {
    const int BAR_LENGTH = 30;  // 막대 최대 길이 (문자 수)
    int filled = (int)((value / maxVal) * BAR_LENGTH);

    Serial.print("  [");
    for (int i = 0; i < BAR_LENGTH; i++) {
        if (i < filled) {
            Serial.print("#");  // 채워진 부분
        } else {
            Serial.print("-");  // 빈 부분
        }
    }
    Serial.println("]");
}

/*
 * ============================================================
 *  전압 변환 공식 설명
 * ============================================================
 *
 *  ESP32 ADC: 12비트 → 0~4095 범위
 *  입력 전압: 0~3.3V (ADC_11db 감쇠 기준)
 *
 *  변환 공식:
 *  전압(V) = ADC 읽은 값 × (3.3V / 4095)
 *
 *  예시:
 *    ADC = 0    → 0 × (3.3/4095)    = 0.000V
 *    ADC = 1024 → 1024 × (3.3/4095) = 0.825V
 *    ADC = 2048 → 2048 × (3.3/4095) = 1.650V
 *    ADC = 3072 → 3072 × (3.3/4095) = 2.475V
 *    ADC = 4095 → 4095 × (3.3/4095) = 3.300V
 *
 * ============================================================
 *  ESP32 ADC 비선형성 주의사항
 * ============================================================
 *
 *  ESP32의 ADC는 0V 근처와 3.3V 근처에서 비선형적입니다.
 *  정밀한 측정이 필요하면 다음 방법을 사용하세요:
 *
 *  1. 다중 샘플 평균 (이 코드에서 사용)
 *  2. esp_adc_cal 라이브러리 (ESP-IDF 보정 기능)
 *  3. 외부 ADC 모듈 사용 (ADS1115 등, I2C 방식)
 *
 * ============================================================
 *  센서 연결 응용
 * ============================================================
 *
 *  가변저항 대신 다양한 아날로그 센서를 연결할 수 있습니다:
 *
 *  - 조도 센서 (CdS): 빛의 밝기 측정
 *  - 가스 센서 (MQ-2): 가스 농도 측정 (5V→3.3V 전압 분배 필요)
 *  - 토양 수분 센서: 토양 수분 측정
 *  - 사운드 센서: 소리 크기 측정
 *  - NTC 서미스터: 온도 측정 (저항값으로 온도 계산)
 *
 *  ※ 5V 출력 센서는 전압 분배기로 3.3V 이하로 낮춰야 합니다!
 *     5V × (2kΩ / (1kΩ + 2kΩ)) = 3.33V (안전)
 *
 * ============================================================
 */
