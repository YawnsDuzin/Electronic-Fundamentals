/*
 * ===================================================================
 *  프로젝트 2: MQ-2 가스센서 아날로그 읽기
 * ===================================================================
 *
 *  설명: MQ-2 가스센서의 아날로그 출력을 읽어 가스 농도를 측정하고,
 *        농도 수준에 따라 LED와 부저로 경고하는 시스템입니다.
 *        이동 평균 필터를 적용하여 노이즈를 제거합니다.
 *
 *  보드: ESP32 DevKit V1
 *  작성: Phase 6 - 실전 프로젝트
 *
 *  회로 연결:
 *    - MQ-2 VCC  → ESP32 VIN (5V)
 *    - MQ-2 GND  → ESP32 GND
 *    - MQ-2 AOUT → ESP32 GPIO36 (VP, ADC1_CH0)
 *    - LED 빨강  → GPIO19 → 220Ω → GND (위험)
 *    - LED 노랑  → GPIO18 → 220Ω → GND (주의)
 *    - LED 초록  → GPIO17 → 220Ω → GND (정상)
 *    - 부저      → GPIO16 → GND         (경고음)
 *
 *  주의: MQ-2 센서는 처음 사용 시 24~48시간 번인(예열)이 필요합니다.
 *        매 사용 시에도 최소 2~3분 예열 후 값이 안정됩니다.
 *
 * ===================================================================
 */

// ===================================================================
//  핀 정의
// ===================================================================

// MQ-2 센서 아날로그 출력 핀
// GPIO36(VP)은 ADC1 채널이므로 WiFi와 충돌 없이 사용 가능
const int MQ2_ANALOG_PIN = 36;

// 경고 LED 핀 정의
const int LED_RED_PIN    = 19;   // 빨간 LED: 위험 경고
const int LED_YELLOW_PIN = 18;   // 노란 LED: 주의 경고
const int LED_GREEN_PIN  = 17;   // 초록 LED: 정상 상태

// 부저 핀
const int BUZZER_PIN     = 16;   // 능동 부저 (HIGH로 울림)

// ===================================================================
//  상수 정의
// ===================================================================

// ADC 관련 상수
const float ADC_RESOLUTION = 4095.0;  // ESP32 ADC 12비트 최대값
const float ADC_VOLTAGE    = 3.3;     // ADC 기준 전압 (3.3V)

// 가스 농도 임계값 (ADC 원시값 기준)
// ※ 이 값은 환경에 따라 반드시 캘리브레이션이 필요합니다!
//    깨끗한 공기에서의 기본 값을 측정한 후 조정하세요.
const int THRESHOLD_NORMAL  = 1000;   // 정상 범위 상한 (0 ~ 1000)
const int THRESHOLD_WARNING = 2000;   // 주의 범위 상한 (1001 ~ 2000)
const int THRESHOLD_DANGER  = 3000;   // 경고 범위 상한 (2001 ~ 3000)
                                       // 3001 이상 = 위험

// 측정 주기 (밀리초)
const unsigned long MEASURE_INTERVAL  = 100;    // 센서 읽기 간격 (100ms)
const unsigned long DISPLAY_INTERVAL  = 1000;   // 시리얼 출력 간격 (1초)
const unsigned long PLOTTER_INTERVAL  = 100;    // 플로터 출력 간격 (100ms)

// 이동 평균 필터 설정
const int FILTER_WINDOW_SIZE = 10;    // 이동 평균 윈도우 크기
                                       // 값이 클수록 안정적이지만 응답 느림
                                       // 권장: 5~20

// 부저 관련 상수
const unsigned long BUZZER_INTERVAL = 300;      // 부저 간헐 울림 간격 (밀리초)

// 예열 시간 (밀리초)
const unsigned long WARMUP_TIME = 120000;       // 2분 (120초) 예열

// ===================================================================
//  경고 레벨 열거형
// ===================================================================

enum AlertLevel {
  ALERT_NORMAL  = 0,   // 정상: 초록 LED
  ALERT_CAUTION = 1,   // 주의: 노란 LED
  ALERT_WARNING = 2,   // 경고: 빨간 LED 점멸
  ALERT_DANGER  = 3    // 위험: 빨간 LED + 부저
};

// ===================================================================
//  이동 평균 필터 클래스
// ===================================================================

/*
 * MovingAverageFilter: 이동 평균 필터를 구현한 클래스
 *
 * 원리:
 *   - 고정 크기의 원형 버퍼에 샘플을 순차적으로 저장
 *   - 새 샘플이 들어오면 가장 오래된 샘플을 덮어씀
 *   - 버퍼에 저장된 모든 샘플의 평균을 반환
 *
 * 장점:
 *   - 고주파 노이즈 제거에 효과적
 *   - 구현이 간단하고 메모리 사용량이 적음
 *   - 실시간 처리에 적합 (O(1) 시간복잡도)
 *
 * 단점:
 *   - 급격한 변화에 대한 응답이 느림 (윈도우 크기에 비례)
 *   - 위상 지연 발생 (윈도우 크기 / 2 만큼)
 */
class MovingAverageFilter {
private:
  int* buffer;           // 샘플 저장 버퍼 (동적 할당)
  int windowSize;        // 윈도우 크기
  int writeIndex;        // 다음 쓰기 위치 (원형 인덱스)
  long sum;              // 현재 합계 (매번 재계산 대신 누적)
  int sampleCount;       // 현재 저장된 샘플 수

public:
  // 생성자: 윈도우 크기 지정하여 버퍼 초기화
  MovingAverageFilter(int size) {
    windowSize = size;
    buffer = new int[size];      // 동적 메모리 할당
    writeIndex = 0;
    sum = 0;
    sampleCount = 0;

    // 버퍼 초기화 (0으로 채움)
    for (int i = 0; i < size; i++) {
      buffer[i] = 0;
    }
  }

  // 소멸자: 동적 할당 메모리 해제
  ~MovingAverageFilter() {
    delete[] buffer;
  }

  // 새 샘플 추가 및 필터링된 값 반환
  int addSample(int newSample) {
    // 버퍼가 가득 찬 경우: 가장 오래된 값을 합계에서 빼기
    if (sampleCount >= windowSize) {
      sum -= buffer[writeIndex];
    } else {
      sampleCount++;
    }

    // 새 샘플 저장 및 합계 업데이트
    buffer[writeIndex] = newSample;
    sum += newSample;

    // 원형 인덱스 업데이트
    writeIndex = (writeIndex + 1) % windowSize;

    // 평균 반환 (정수 나눗셈)
    return (int)(sum / sampleCount);
  }

  // 현재 필터링된 값 반환 (새 샘플 추가 없이)
  int getAverage() {
    if (sampleCount == 0) return 0;
    return (int)(sum / sampleCount);
  }

  // 필터 초기화
  void reset() {
    writeIndex = 0;
    sum = 0;
    sampleCount = 0;
    for (int i = 0; i < windowSize; i++) {
      buffer[i] = 0;
    }
  }
};

// ===================================================================
//  전역 변수
// ===================================================================

// 이동 평균 필터 인스턴스 생성
MovingAverageFilter gasFilter(FILTER_WINDOW_SIZE);

// 센서 데이터 변수
int rawAdcValue        = 0;        // ADC 원시값 (0~4095)
int filteredAdcValue   = 0;        // 필터링된 ADC 값
float voltage          = 0.0;      // 변환된 전압 (V)
AlertLevel currentAlert = ALERT_NORMAL;  // 현재 경고 레벨

// 타이밍 변수 (millis 기반, delay 미사용)
unsigned long lastMeasureTime  = 0;  // 마지막 측정 시각
unsigned long lastDisplayTime  = 0;  // 마지막 시리얼 출력 시각
unsigned long lastPlotterTime  = 0;  // 마지막 플로터 출력 시각
unsigned long lastBuzzerToggle = 0;  // 마지막 부저 토글 시각
unsigned long startupTime      = 0;  // 시스템 시작 시각

// 부저 상태
bool buzzerState       = false;     // 부저 현재 상태
bool warmupComplete    = false;     // 예열 완료 여부

// 통계 변수
int minValue = 4095;                // 최소값 추적
int maxValue = 0;                   // 최대값 추적

// ===================================================================
//  초기 설정 함수 (setup)
// ===================================================================

void setup() {
  // --- 시리얼 통신 초기화 ---
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  Serial.println();
  Serial.println("============================================");
  Serial.println("  프로젝트 2: MQ-2 가스센서 모니터링");
  Serial.println("============================================");

  // --- LED 핀 설정 ---
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // 초기 상태: 모두 꺼짐
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_YELLOW_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // --- ADC 설정 ---
  // ESP32 ADC 감쇠 설정 (입력 전압 범위 확장)
  // ADC_ATTEN_DB_0   : 0~1.1V
  // ADC_ATTEN_DB_2_5 : 0~1.5V
  // ADC_ATTEN_DB_6   : 0~2.2V
  // ADC_ATTEN_DB_11  : 0~3.3V (전체 범위 사용)
  analogSetAttenuation(ADC_11db);   // 0~3.3V 전체 범위
  analogSetWidth(12);                // 12비트 해상도 (0~4095)

  Serial.println();
  Serial.println("  [ADC 설정]");
  Serial.println("  - 해상도: 12비트 (0~4095)");
  Serial.println("  - 감쇠: 11dB (0~3.3V)");
  Serial.print("  - 센서 핀: GPIO");
  Serial.println(MQ2_ANALOG_PIN);
  Serial.println();

  // --- 예열 시작 ---
  startupTime = millis();
  Serial.println("  [센서 예열 시작]");
  Serial.print("  - 예열 시간: ");
  Serial.print(WARMUP_TIME / 1000);
  Serial.println("초");
  Serial.println("  - 예열 중 값이 불안정할 수 있습니다.");
  Serial.println();

  // 시작 시 LED 테스트 (모든 LED를 순서대로 점등)
  ledStartupTest();

  Serial.println("============================================");
  Serial.println("  시리얼 플로터 사용법:");
  Serial.println("  도구 > 시리얼 플로터 (Ctrl+Shift+L)");
  Serial.println("============================================");
  Serial.println();

  // 초록 LED 켜기 (시스템 동작 중 표시)
  digitalWrite(LED_GREEN_PIN, HIGH);
}

// ===================================================================
//  메인 루프 함수 (loop)
// ===================================================================

void loop() {
  unsigned long currentTime = millis();

  // --- 1단계: 센서 값 읽기 (100ms 간격) ---
  if (currentTime - lastMeasureTime >= MEASURE_INTERVAL) {
    lastMeasureTime = currentTime;
    readAndFilterSensor();
  }

  // --- 2단계: 예열 상태 확인 ---
  checkWarmupStatus(currentTime);

  // --- 3단계: 경고 레벨 판단 ---
  currentAlert = evaluateAlertLevel(filteredAdcValue);

  // --- 4단계: LED 및 부저 제어 ---
  updateAlertIndicators(currentTime);

  // --- 5단계: 시리얼 모니터 출력 (1초 간격) ---
  if (currentTime - lastDisplayTime >= DISPLAY_INTERVAL) {
    lastDisplayTime = currentTime;
    printSensorData();
  }

  // --- 6단계: 시리얼 플로터용 출력 (100ms 간격) ---
  if (currentTime - lastPlotterTime >= PLOTTER_INTERVAL) {
    lastPlotterTime = currentTime;
    printPlotterData();
  }
}

// ===================================================================
//  센서 읽기 및 필터링
// ===================================================================

/*
 * ADC에서 원시값을 읽고, 이동 평균 필터를 적용합니다.
 * 전압 변환도 동시에 수행합니다.
 */
void readAndFilterSensor() {
  // ADC 원시값 읽기 (12비트: 0~4095)
  rawAdcValue = analogRead(MQ2_ANALOG_PIN);

  // 이동 평균 필터 적용
  // 필터가 노이즈를 제거하여 안정적인 값을 반환합니다
  filteredAdcValue = gasFilter.addSample(rawAdcValue);

  // ADC 값 → 전압 변환
  // 공식: 전압 = ADC값 × (기준전압 / 최대 ADC값)
  voltage = filteredAdcValue * (ADC_VOLTAGE / ADC_RESOLUTION);

  // 통계 업데이트 (예열 완료 후에만)
  if (warmupComplete) {
    if (filteredAdcValue < minValue) minValue = filteredAdcValue;
    if (filteredAdcValue > maxValue) maxValue = filteredAdcValue;
  }
}

// ===================================================================
//  예열 상태 확인
// ===================================================================

/*
 * 센서 예열 완료 여부를 확인하고, 진행률을 표시합니다.
 */
void checkWarmupStatus(unsigned long currentTime) {
  if (!warmupComplete) {
    unsigned long elapsed = currentTime - startupTime;

    if (elapsed >= WARMUP_TIME) {
      warmupComplete = true;
      Serial.println();
      Serial.println("  *** 센서 예열 완료! 정상 측정을 시작합니다. ***");
      Serial.println();

      // 필터 초기화 (예열 중 불안정한 데이터 제거)
      gasFilter.reset();
      minValue = 4095;
      maxValue = 0;
    }
    else {
      // 예열 진행률 표시 (10초마다)
      static unsigned long lastWarmupPrint = 0;
      if (currentTime - lastWarmupPrint >= 10000) {
        lastWarmupPrint = currentTime;
        int progress = (elapsed * 100) / WARMUP_TIME;
        Serial.print("  [예열 중] ");
        Serial.print(progress);
        Serial.print("% (");
        Serial.print(elapsed / 1000);
        Serial.print("/");
        Serial.print(WARMUP_TIME / 1000);
        Serial.println("초)");
      }
    }
  }
}

// ===================================================================
//  경고 레벨 판단
// ===================================================================

/*
 * 필터링된 ADC 값을 기반으로 경고 레벨을 판단합니다.
 *
 * @param adcValue: 필터링된 ADC 값
 * @return: 해당하는 AlertLevel 열거값
 */
AlertLevel evaluateAlertLevel(int adcValue) {
  if (adcValue <= THRESHOLD_NORMAL) {
    return ALERT_NORMAL;
  }
  else if (adcValue <= THRESHOLD_WARNING) {
    return ALERT_CAUTION;
  }
  else if (adcValue <= THRESHOLD_DANGER) {
    return ALERT_WARNING;
  }
  else {
    return ALERT_DANGER;
  }
}

// ===================================================================
//  경고 표시기 업데이트 (LED + 부저)
// ===================================================================

/*
 * 현재 경고 레벨에 따라 LED와 부저를 제어합니다.
 *
 * 정상 (NORMAL):  초록 ON,  노랑 OFF, 빨강 OFF, 부저 OFF
 * 주의 (CAUTION): 초록 OFF, 노랑 ON,  빨강 OFF, 부저 OFF
 * 경고 (WARNING): 초록 OFF, 노랑 OFF, 빨강 점멸, 부저 간헐적
 * 위험 (DANGER):  초록 OFF, 노랑 OFF, 빨강 ON,  부저 연속
 */
void updateAlertIndicators(unsigned long currentTime) {
  switch (currentAlert) {
    case ALERT_NORMAL:
      // 정상: 초록 LED만 켜기
      digitalWrite(LED_GREEN_PIN, HIGH);
      digitalWrite(LED_YELLOW_PIN, LOW);
      digitalWrite(LED_RED_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      buzzerState = false;
      break;

    case ALERT_CAUTION:
      // 주의: 노란 LED만 켜기
      digitalWrite(LED_GREEN_PIN, LOW);
      digitalWrite(LED_YELLOW_PIN, HIGH);
      digitalWrite(LED_RED_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      buzzerState = false;
      break;

    case ALERT_WARNING:
      // 경고: 빨간 LED 점멸 + 부저 간헐적 울림
      digitalWrite(LED_GREEN_PIN, LOW);
      digitalWrite(LED_YELLOW_PIN, LOW);

      // 빨간 LED와 부저를 BUZZER_INTERVAL 간격으로 토글
      if (currentTime - lastBuzzerToggle >= BUZZER_INTERVAL) {
        lastBuzzerToggle = currentTime;
        buzzerState = !buzzerState;
        digitalWrite(LED_RED_PIN, buzzerState ? HIGH : LOW);
        digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
      }
      break;

    case ALERT_DANGER:
      // 위험: 빨간 LED 상시 켜짐 + 부저 연속
      digitalWrite(LED_GREEN_PIN, LOW);
      digitalWrite(LED_YELLOW_PIN, LOW);
      digitalWrite(LED_RED_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
      buzzerState = true;
      break;
  }
}

// ===================================================================
//  시리얼 모니터 출력 (사람이 읽기 쉬운 형식)
// ===================================================================

/*
 * 센서 데이터를 시리얼 모니터에 보기 좋게 출력합니다.
 * 1초마다 호출됩니다.
 */
void printSensorData() {
  Serial.println("────────────────────────────────────────");

  // 원시값과 필터링된 값 출력
  Serial.print("  ADC 원시값: ");
  Serial.print(rawAdcValue);
  Serial.print("  │  필터값: ");
  Serial.print(filteredAdcValue);
  Serial.print("  │  전압: ");
  Serial.print(voltage, 3);     // 소수점 3자리
  Serial.println("V");

  // 경고 레벨 출력
  Serial.print("  경고 레벨: ");
  switch (currentAlert) {
    case ALERT_NORMAL:  Serial.print("[정상]"); break;
    case ALERT_CAUTION: Serial.print("[주의]"); break;
    case ALERT_WARNING: Serial.print("[경고]"); break;
    case ALERT_DANGER:  Serial.print("[위험!!!]"); break;
  }

  // 예열 상태 표시
  if (!warmupComplete) {
    Serial.print("  (예열 중 - 값이 불안정할 수 있음)");
  }
  Serial.println();

  // 통계 정보 출력 (예열 완료 후)
  if (warmupComplete) {
    Serial.print("  최소: ");
    Serial.print(minValue);
    Serial.print("  │  최대: ");
    Serial.print(maxValue);
    Serial.print("  │  범위: ");
    Serial.println(maxValue - minValue);
  }

  // 간이 바 그래프 출력
  printBarGraph(filteredAdcValue);
}

// ===================================================================
//  간이 바 그래프 출력
// ===================================================================

/*
 * ADC 값을 시각적인 바 그래프로 표시합니다.
 *
 * 출력 예시:
 *   [████████░░░░░░░░░░░░░░░░░░░░░░] 1024/4095
 *
 * @param value: 표시할 ADC 값 (0~4095)
 */
void printBarGraph(int value) {
  const int BAR_LENGTH = 30;  // 바 그래프 길이 (문자 수)

  // 값을 바 길이에 매핑
  int filledLength = map(value, 0, 4095, 0, BAR_LENGTH);

  Serial.print("  [");
  for (int i = 0; i < BAR_LENGTH; i++) {
    if (i < filledLength) {
      Serial.print("#");    // 채워진 부분
    } else {
      Serial.print(".");    // 빈 부분
    }
  }
  Serial.print("] ");
  Serial.print(value);
  Serial.println("/4095");
}

// ===================================================================
//  시리얼 플로터용 출력
// ===================================================================

/*
 * Arduino IDE의 시리얼 플로터에서 그래프를 표시하기 위한 출력 형식입니다.
 *
 * 형식: 값1,값2,값3,...
 * 각 값은 별도의 그래프 라인으로 표시됩니다.
 *
 * 출력되는 값:
 *   1. 원시값 (Raw) - 노이즈 포함
 *   2. 필터값 (Filtered) - 이동 평균 적용
 *   3. 정상 임계값 라인
 *   4. 주의 임계값 라인
 *   5. 위험 임계값 라인
 *
 * 사용법: Arduino IDE > 도구 > 시리얼 플로터 (Ctrl+Shift+L)
 * 주의: 시리얼 모니터와 플로터를 동시에 사용할 수 없습니다.
 *       플로터 사용 시 printSensorData()의 호출을 주석 처리하세요.
 */
void printPlotterData() {
  // 아래 주석을 해제하고 printSensorData() 호출을 주석 처리하면
  // 시리얼 플로터에서 깔끔한 그래프를 볼 수 있습니다.

  /*
  // 첫 번째 줄: 레이블 (한 번만 출력)
  // Serial.println("Raw,Filtered,Normal,Warning,Danger");

  Serial.print(rawAdcValue);         // 원시 ADC 값
  Serial.print(",");
  Serial.print(filteredAdcValue);    // 필터링된 값
  Serial.print(",");
  Serial.print(THRESHOLD_NORMAL);    // 정상 임계값 라인
  Serial.print(",");
  Serial.print(THRESHOLD_WARNING);   // 주의 임계값 라인
  Serial.print(",");
  Serial.println(THRESHOLD_DANGER);  // 위험 임계값 라인
  */
}

// ===================================================================
//  LED 시작 테스트
// ===================================================================

/*
 * 시스템 시작 시 모든 LED를 순서대로 점등하여 정상 동작을 확인합니다.
 * 사용자가 LED 연결 상태를 시각적으로 확인할 수 있습니다.
 */
void ledStartupTest() {
  Serial.println("  [LED 테스트 시작]");

  // 초록 → 노랑 → 빨강 순서로 점등
  int testPins[] = {LED_GREEN_PIN, LED_YELLOW_PIN, LED_RED_PIN};
  const char* testNames[] = {"초록", "노랑", "빨강"};

  for (int i = 0; i < 3; i++) {
    digitalWrite(testPins[i], HIGH);
    Serial.print("    ");
    Serial.print(testNames[i]);
    Serial.println(" LED ON");
    delay(300);
    digitalWrite(testPins[i], LOW);
  }

  // 부저 테스트 (짧게)
  digitalWrite(BUZZER_PIN, HIGH);
  Serial.println("    부저 테스트");
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("  [LED 테스트 완료]");
  Serial.println();
}

// ===================================================================
//  추가 학습: 지수 이동 평균 필터 (EMA)
// ===================================================================

/*
 * 지수 이동 평균(Exponential Moving Average) 필터
 *
 * 단순 이동 평균(SMA)과 비교:
 *   - SMA: 모든 샘플에 동일한 가중치
 *   - EMA: 최근 샘플에 더 높은 가중치 → 변화에 빠른 응답
 *
 * 공식: EMA = alpha × 현재값 + (1 - alpha) × 이전 EMA값
 *   alpha (0~1): 값이 클수록 현재값 반영 비율 높음
 *     - alpha = 0.1: 매우 평활 (느린 응답)
 *     - alpha = 0.5: 중간
 *     - alpha = 0.9: 거의 필터링 없음 (빠른 응답)
 *
 * 사용 예시:
 *   static float emaValue = 0;
 *   float alpha = 0.2;
 *   emaValue = alpha * rawAdcValue + (1 - alpha) * emaValue;
 */

// ===================================================================
//  추가 학습: 가스 농도 계산 (ppm 변환)
// ===================================================================

/*
 * MQ-2 센서의 정확한 ppm 변환을 위해서는 다음이 필요합니다:
 *
 * 1. 깨끗한 공기에서의 기준 저항값 (Ro) 측정
 * 2. 센서 저항값 (Rs) 계산:
 *    Rs = ((Vc × RL) / Vout) - RL
 *    여기서: Vc = 센서 전원 전압 (보통 5V)
 *           RL = 부하 저항 (모듈에 내장, 보통 10KΩ)
 *           Vout = 센서 아날로그 출력 전압
 *
 * 3. Rs/Ro 비율 계산
 *
 * 4. 데이터시트의 감도 특성 곡선에서 ppm 값 도출
 *    (로그 스케일의 비선형 관계)
 *
 * 이 프로젝트에서는 간단히 ADC 값을 임계값과 비교하는
 * 방식을 사용합니다. 정확한 ppm 측정이 필요하면
 * 데이터시트를 참조하여 캘리브레이션을 수행하세요.
 *
 * 참고 공식 (LPG 가스 기준):
 *   ppm = 10 ^ ((log10(Rs/Ro) - b) / m)
 *   여기서 m, b는 데이터시트의 감도 곡선에서 추출한 상수
 */

// ===================================================================
//  끝 (End of File)
// ===================================================================
