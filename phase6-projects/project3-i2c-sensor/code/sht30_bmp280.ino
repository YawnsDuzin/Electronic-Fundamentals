/*
 * ===================================================================
 *  프로젝트 3: I2C 센서 연결 (SHT30 + BMP280)
 * ===================================================================
 *
 *  설명: I2C 버스에 SHT30 온습도 센서와 BMP280 기압 센서를 동시에
 *        연결하여 환경 데이터(온도, 습도, 기압, 고도)를 수집합니다.
 *
 *  보드: ESP32 DevKit V1
 *  작성: Phase 6 - 실전 프로젝트
 *
 *  필요 라이브러리 (Arduino IDE 라이브러리 매니저에서 설치):
 *    - Adafruit SHT31 Library
 *    - Adafruit BMP280 Library
 *    - Adafruit Unified Sensor
 *    - Adafruit BusIO
 *
 *  회로 연결:
 *    - ESP32 GPIO21 (SDA) → SHT30 SDA, BMP280 SDA (병렬)
 *    - ESP32 GPIO22 (SCL) → SHT30 SCL, BMP280 SCL (병렬)
 *    - ESP32 3.3V         → SHT30 VCC, BMP280 VCC
 *    - ESP32 GND          → SHT30 GND, BMP280 GND
 *    - SHT30 ADDR → GND (주소: 0x44)
 *    - BMP280 SDO → GND (주소: 0x76)
 *    - (옵션) 4.7KΩ 풀업 저항: SDA→3.3V, SCL→3.3V
 *
 * ===================================================================
 */

// ===================================================================
//  라이브러리 포함
// ===================================================================

#include <Wire.h>                // I2C 통신 기본 라이브러리
#include <Adafruit_SHT31.h>      // SHT30/31 온습도 센서 라이브러리
#include <Adafruit_BMP280.h>     // BMP280 기압 센서 라이브러리

// ===================================================================
//  상수 정의
// ===================================================================

// I2C 핀 정의 (ESP32 기본값)
const int I2C_SDA_PIN = 21;     // SDA 핀 (데이터)
const int I2C_SCL_PIN = 22;     // SCL 핀 (클록)

// I2C 주소 정의
const uint8_t SHT30_ADDR  = 0x44;  // SHT30 I2C 주소 (ADDR→GND)
const uint8_t BMP280_ADDR = 0x76;  // BMP280 I2C 주소 (SDO→GND)

// 해수면 기압 (hPa) - 고도 계산에 사용
// ※ 정확한 고도를 위해 해당 지역의 현재 해수면 기압을 입력하세요
//    기상청 웹사이트에서 확인 가능
const float SEA_LEVEL_PRESSURE = 1013.25;  // 표준 대기압 (hPa)

// 측정 주기 (밀리초)
const unsigned long MEASURE_INTERVAL = 2000;  // 2초마다 측정
                                                // SHT30 최소 측정 간격: ~15ms
                                                // BMP280 최소 측정 간격: ~38ms

// 데이터 유효 범위 (이상값 검출용)
const float TEMP_MIN = -40.0;     // 온도 최소값 (°C)
const float TEMP_MAX = 85.0;      // 온도 최대값 (°C)
const float HUMI_MIN = 0.0;       // 습도 최소값 (%)
const float HUMI_MAX = 100.0;     // 습도 최대값 (%)
const float PRES_MIN = 300.0;     // 기압 최소값 (hPa)
const float PRES_MAX = 1100.0;    // 기압 최대값 (hPa)

// ===================================================================
//  센서 객체 생성
// ===================================================================

// Adafruit 라이브러리의 센서 객체
Adafruit_SHT31  sht30 = Adafruit_SHT31();    // SHT30 온습도 센서
Adafruit_BMP280 bmp280;                        // BMP280 기압 센서

// ===================================================================
//  전역 변수
// ===================================================================

// 센서 상태 플래그
bool sht30Ready  = false;    // SHT30 초기화 성공 여부
bool bmp280Ready = false;    // BMP280 초기화 성공 여부

// 센서 데이터 저장 구조체
struct SensorData {
  // SHT30 데이터
  float sht30_temperature;    // 온도 (°C) - SHT30
  float sht30_humidity;       // 습도 (%) - SHT30

  // BMP280 데이터
  float bmp280_temperature;   // 온도 (°C) - BMP280
  float bmp280_pressure;      // 기압 (hPa) - BMP280
  float bmp280_altitude;      // 고도 (m) - BMP280

  // 유효성 플래그
  bool sht30_valid;           // SHT30 데이터 유효 여부
  bool bmp280_valid;          // BMP280 데이터 유효 여부
};

SensorData currentData;        // 현재 센서 데이터

// 타이밍 변수
unsigned long lastMeasureTime = 0;    // 마지막 측정 시각
unsigned long measureCount = 0;        // 총 측정 횟수

// ===================================================================
//  초기 설정 함수 (setup)
// ===================================================================

void setup() {
  // --- 시리얼 통신 초기화 ---
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  Serial.println();
  Serial.println("============================================");
  Serial.println("  프로젝트 3: I2C 센서 (SHT30 + BMP280)");
  Serial.println("============================================");
  Serial.println();

  // --- I2C 버스 초기화 ---
  // Wire.begin(SDA, SCL): I2C 핀 지정 (ESP32는 임의 핀 사용 가능)
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // I2C 클록 속도 설정 (100kHz = 표준 모드)
  // 400kHz (고속 모드)도 사용 가능하지만 안정성을 위해 100kHz 권장
  Wire.setClock(100000);

  Serial.print("  I2C 버스 초기화 (SDA=GPIO");
  Serial.print(I2C_SDA_PIN);
  Serial.print(", SCL=GPIO");
  Serial.print(I2C_SCL_PIN);
  Serial.println(")");
  Serial.println();

  // --- I2C 장치 스캔 ---
  scanI2CDevices();

  // --- SHT30 센서 초기화 ---
  initSHT30();

  // --- BMP280 센서 초기화 ---
  initBMP280();

  // --- 초기화 결과 요약 ---
  Serial.println();
  Serial.println("────────────────────────────────────────");
  Serial.println("  [초기화 결과]");
  Serial.print("  SHT30 (온습도):  ");
  Serial.println(sht30Ready ? "성공" : "실패!");
  Serial.print("  BMP280 (기압):   ");
  Serial.println(bmp280Ready ? "성공" : "실패!");
  Serial.println("────────────────────────────────────────");

  if (!sht30Ready && !bmp280Ready) {
    Serial.println();
    Serial.println("  *** 두 센서 모두 초기화 실패! ***");
    Serial.println("  배선을 확인하고 재시작하세요.");
    Serial.println("  I2C 스캔 결과를 참고하세요.");
  }

  Serial.println();
  Serial.print("  측정 주기: ");
  Serial.print(MEASURE_INTERVAL / 1000.0, 1);
  Serial.println("초");
  Serial.print("  해수면 기압: ");
  Serial.print(SEA_LEVEL_PRESSURE, 2);
  Serial.println(" hPa");
  Serial.println();
  Serial.println("============================================");
  Serial.println("  데이터 수집을 시작합니다...");
  Serial.println("============================================");
  Serial.println();
}

// ===================================================================
//  메인 루프 함수 (loop)
// ===================================================================

void loop() {
  unsigned long currentTime = millis();

  // 측정 주기마다 센서 데이터 읽기
  if (currentTime - lastMeasureTime >= MEASURE_INTERVAL) {
    lastMeasureTime = currentTime;
    measureCount++;

    // --- SHT30 데이터 읽기 ---
    if (sht30Ready) {
      readSHT30Data();
    }

    // --- BMP280 데이터 읽기 ---
    if (bmp280Ready) {
      readBMP280Data();
    }

    // --- 데이터 출력 ---
    printFormattedData();
  }
}

// ===================================================================
//  I2C 장치 스캔 함수
// ===================================================================

/*
 * I2C 버스에 연결된 모든 장치의 주소를 스캔합니다.
 * 각 주소에 대해 통신을 시도하고, 응답하는 장치를 표시합니다.
 *
 * 동작 원리:
 *   1. 0x01 ~ 0x7F 범위의 모든 주소에 대해
 *   2. Wire.beginTransmission()으로 통신 시작
 *   3. Wire.endTransmission()의 반환값으로 응답 확인
 *      - 0: 성공 (장치가 응답함)
 *      - 2: 주소 NACK (해당 주소에 장치 없음)
 *      - 4: 기타 오류
 */
void scanI2CDevices() {
  Serial.println("  [I2C 장치 스캔 시작]");

  int deviceCount = 0;

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      // 장치 발견!
      Serial.print("    발견: 0x");
      if (addr < 16) Serial.print("0");  // 한 자리 주소 앞에 0 추가
      Serial.print(addr, HEX);

      // 알려진 장치 이름 표시
      Serial.print(" → ");
      identifyDevice(addr);
      Serial.println();

      deviceCount++;
    }
    else if (error == 4) {
      // 통신 오류
      Serial.print("    오류: 0x");
      if (addr < 16) Serial.print("0");
      Serial.print(addr, HEX);
      Serial.println(" (통신 오류 발생)");
    }
  }

  Serial.print("  [스캔 완료] ");
  Serial.print(deviceCount);
  Serial.println("개 장치 발견");
  Serial.println();
}

/*
 * I2C 주소로 알려진 장치 이름을 식별합니다.
 */
void identifyDevice(uint8_t addr) {
  switch (addr) {
    case 0x44: Serial.print("SHT30/31 (온습도, ADDR=GND)"); break;
    case 0x45: Serial.print("SHT30/31 (온습도, ADDR=VDD)"); break;
    case 0x76: Serial.print("BMP280/BME280 (기압, SDO=GND)"); break;
    case 0x77: Serial.print("BMP280/BME280 (기압, SDO=VDD)"); break;
    case 0x3C: Serial.print("SSD1306 OLED (128x64)"); break;
    case 0x3D: Serial.print("SSD1306 OLED (128x32)"); break;
    case 0x27: Serial.print("PCF8574 LCD I2C 어댑터"); break;
    case 0x48: Serial.print("ADS1115 ADC / TMP102 온도"); break;
    case 0x68: Serial.print("MPU6050 IMU / DS3231 RTC"); break;
    case 0x57: Serial.print("AT24C32 EEPROM"); break;
    default:   Serial.print("알 수 없는 장치"); break;
  }
}

// ===================================================================
//  SHT30 초기화
// ===================================================================

/*
 * SHT30 온습도 센서를 초기화합니다.
 * Adafruit_SHT31 라이브러리를 사용합니다.
 */
void initSHT30() {
  Serial.print("  [SHT30] 초기화 중... (주소: 0x");
  Serial.print(SHT30_ADDR, HEX);
  Serial.print(") → ");

  // SHT30 초기화 시도
  if (sht30.begin(SHT30_ADDR)) {
    sht30Ready = true;
    Serial.println("성공!");

    // SHT30 히터 상태 확인
    // 히터: 결로 방지용, 일반적으로 OFF 상태
    Serial.print("    히터 상태: ");
    Serial.println(sht30.isHeaterEnabled() ? "ON" : "OFF");

    // 히터가 켜져있으면 끄기 (일반 사용에서는 OFF 권장)
    if (sht30.isHeaterEnabled()) {
      sht30.heater(false);
      Serial.println("    → 히터를 OFF로 전환했습니다.");
    }
  }
  else {
    sht30Ready = false;
    Serial.println("실패!");
    Serial.println("    → SHT30 센서를 찾을 수 없습니다.");
    Serial.println("    → 배선 및 주소(ADDR 핀)를 확인하세요.");
  }
}

// ===================================================================
//  BMP280 초기화
// ===================================================================

/*
 * BMP280 기압 센서를 초기화합니다.
 * Adafruit_BMP280 라이브러리를 사용합니다.
 *
 * 오버샘플링 설정:
 *   - SAMPLING_NONE: 비활성화
 *   - SAMPLING_X1:   1배 (가장 빠르지만 노이즈 많음)
 *   - SAMPLING_X2:   2배
 *   - SAMPLING_X4:   4배
 *   - SAMPLING_X8:   8배
 *   - SAMPLING_X16:  16배 (가장 정확하지만 느림)
 */
void initBMP280() {
  Serial.print("  [BMP280] 초기화 중... (주소: 0x");
  Serial.print(BMP280_ADDR, HEX);
  Serial.print(") → ");

  // BMP280 초기화 시도
  if (bmp280.begin(BMP280_ADDR)) {
    bmp280Ready = true;
    Serial.println("성공!");

    // BMP280 측정 설정
    // setSampling(모드, 온도_오버샘플링, 기압_오버샘플링, 필터, 대기시간)
    bmp280.setSampling(
      Adafruit_BMP280::MODE_NORMAL,      // 연속 측정 모드
      Adafruit_BMP280::SAMPLING_X2,      // 온도: 2배 오버샘플링
      Adafruit_BMP280::SAMPLING_X16,     // 기압: 16배 오버샘플링 (고정밀)
      Adafruit_BMP280::FILTER_X16,       // IIR 필터: 16배 (노이즈 감소)
      Adafruit_BMP280::STANDBY_MS_500    // 대기 시간: 500ms
    );

    Serial.println("    설정: 온도 x2, 기압 x16, 필터 x16");

    // 센서 칩 ID 확인 (BMP280 = 0x58, BME280 = 0x60)
    Serial.print("    칩 ID: 0x");
    Serial.println(bmp280.sensorID(), HEX);
  }
  else {
    bmp280Ready = false;
    Serial.println("실패!");
    Serial.println("    → BMP280 센서를 찾을 수 없습니다.");
    Serial.println("    → 배선 및 주소(SDO 핀)를 확인하세요.");
    Serial.println("    → BME280 모듈인 경우 다른 라이브러리가 필요합니다.");
  }
}

// ===================================================================
//  SHT30 데이터 읽기
// ===================================================================

/*
 * SHT30에서 온도와 습도 데이터를 읽습니다.
 * NaN 체크를 통해 데이터 유효성을 검증합니다.
 */
void readSHT30Data() {
  // 온도 읽기 (°C)
  float temp = sht30.readTemperature();

  // 습도 읽기 (%)
  float humi = sht30.readHumidity();

  // NaN 체크 (센서 통신 오류 시 NaN 반환)
  if (isnan(temp) || isnan(humi)) {
    currentData.sht30_valid = false;
    Serial.println("  [SHT30] 데이터 읽기 실패! (NaN)");
    return;
  }

  // 범위 검증 (이상값 필터링)
  if (temp < TEMP_MIN || temp > TEMP_MAX ||
      humi < HUMI_MIN || humi > HUMI_MAX) {
    currentData.sht30_valid = false;
    Serial.println("  [SHT30] 데이터 범위 초과!");
    return;
  }

  // 유효한 데이터 저장
  currentData.sht30_temperature = temp;
  currentData.sht30_humidity    = humi;
  currentData.sht30_valid       = true;
}

// ===================================================================
//  BMP280 데이터 읽기
// ===================================================================

/*
 * BMP280에서 온도, 기압, 고도 데이터를 읽습니다.
 *
 * 고도 계산:
 *   bmp280.readAltitude(해수면 기압)
 *   내부적으로 국제 표준 대기 공식을 사용하여 계산
 */
void readBMP280Data() {
  // 온도 읽기 (°C)
  float temp = bmp280.readTemperature();

  // 기압 읽기 (Pa → hPa 변환)
  float pres = bmp280.readPressure() / 100.0;  // Pa를 hPa로 변환

  // 고도 읽기 (m) - 해수면 기압 기준
  float alti = bmp280.readAltitude(SEA_LEVEL_PRESSURE);

  // 범위 검증
  if (temp < TEMP_MIN || temp > TEMP_MAX ||
      pres < PRES_MIN || pres > PRES_MAX) {
    currentData.bmp280_valid = false;
    Serial.println("  [BMP280] 데이터 범위 초과!");
    return;
  }

  // 유효한 데이터 저장
  currentData.bmp280_temperature = temp;
  currentData.bmp280_pressure    = pres;
  currentData.bmp280_altitude    = alti;
  currentData.bmp280_valid       = true;
}

// ===================================================================
//  데이터 출력 (포맷팅)
// ===================================================================

/*
 * 수집된 센서 데이터를 보기 좋은 테이블 형식으로 출력합니다.
 */
void printFormattedData() {
  Serial.println("╔══════════════════════════════════════════════════╗");
  Serial.print("║  측정 #");
  Serial.print(measureCount);
  Serial.print("  │  경과 시간: ");

  // 경과 시간 포맷 (HH:MM:SS)
  unsigned long elapsed = millis() / 1000;
  unsigned long hours   = elapsed / 3600;
  unsigned long minutes = (elapsed % 3600) / 60;
  unsigned long seconds = elapsed % 60;
  if (hours < 10) Serial.print("0");
  Serial.print(hours);
  Serial.print(":");
  if (minutes < 10) Serial.print("0");
  Serial.print(minutes);
  Serial.print(":");
  if (seconds < 10) Serial.print("0");
  Serial.print(seconds);
  Serial.println();

  Serial.println("╠══════════════════════════════════════════════════╣");

  // --- SHT30 데이터 ---
  Serial.println("║  [SHT30 온습도 센서]");
  if (currentData.sht30_valid) {
    Serial.print("║    온도: ");
    Serial.print(currentData.sht30_temperature, 2);
    Serial.println(" °C");

    Serial.print("║    습도: ");
    Serial.print(currentData.sht30_humidity, 2);
    Serial.println(" %RH");

    // 불쾌지수 계산 및 표시
    float discomfort = calculateDiscomfortIndex(
      currentData.sht30_temperature,
      currentData.sht30_humidity
    );
    Serial.print("║    불쾌지수: ");
    Serial.print(discomfort, 1);
    Serial.print(" (");
    Serial.print(getDiscomfortLevel(discomfort));
    Serial.println(")");
  }
  else {
    Serial.println("║    [데이터 없음 또는 오류]");
  }

  Serial.println("╠──────────────────────────────────────────────────╣");

  // --- BMP280 데이터 ---
  Serial.println("║  [BMP280 기압 센서]");
  if (currentData.bmp280_valid) {
    Serial.print("║    온도: ");
    Serial.print(currentData.bmp280_temperature, 2);
    Serial.println(" °C");

    Serial.print("║    기압: ");
    Serial.print(currentData.bmp280_pressure, 2);
    Serial.println(" hPa");

    Serial.print("║    고도: ");
    Serial.print(currentData.bmp280_altitude, 1);
    Serial.print(" m (해수면 기압: ");
    Serial.print(SEA_LEVEL_PRESSURE, 2);
    Serial.println(" hPa)");
  }
  else {
    Serial.println("║    [데이터 없음 또는 오류]");
  }

  // --- 두 센서 온도 비교 ---
  if (currentData.sht30_valid && currentData.bmp280_valid) {
    Serial.println("╠──────────────────────────────────────────────────╣");
    float tempDiff = currentData.sht30_temperature - currentData.bmp280_temperature;
    Serial.print("║  온도 차이 (SHT30 - BMP280): ");
    Serial.print(tempDiff, 2);
    Serial.print(" °C ");
    if (abs(tempDiff) > 2.0) {
      Serial.print("(차이가 큽니다! 센서 확인 필요)");
    }
    else {
      Serial.print("(정상 범위)");
    }
    Serial.println();
  }

  Serial.println("╚══════════════════════════════════════════════════╝");
  Serial.println();
}

// ===================================================================
//  불쾌지수 계산
// ===================================================================

/*
 * 불쾌지수(Temperature-Humidity Index) 계산
 *
 * 공식: THI = 0.81 × T + 0.01 × H × (0.99 × T - 14.3) + 46.3
 *
 * 여기서:
 *   T = 기온 (°C)
 *   H = 상대습도 (%)
 *
 * 불쾌지수 범위:
 *   68 미만: 쾌적
 *   68~74: 불쾌감을 느끼기 시작
 *   75~79: 절반 이상이 불쾌감
 *   80 이상: 거의 모두 불쾌감
 *
 * @param temperature: 온도 (°C)
 * @param humidity: 상대습도 (%)
 * @return: 불쾌지수 값
 */
float calculateDiscomfortIndex(float temperature, float humidity) {
  return 0.81 * temperature + 0.01 * humidity * (0.99 * temperature - 14.3) + 46.3;
}

/*
 * 불쾌지수 레벨을 문자열로 반환
 */
const char* getDiscomfortLevel(float index) {
  if (index < 68)      return "쾌적";
  else if (index < 75) return "약간 불쾌";
  else if (index < 80) return "불쾌";
  else                 return "매우 불쾌";
}

// ===================================================================
//  이슬점 계산 (추가 학습)
// ===================================================================

/*
 * 이슬점(Dew Point) 계산
 *
 * Magnus 공식을 사용합니다:
 *   gamma = ln(RH/100) + (b × T) / (c + T)
 *   Td = (c × gamma) / (b - gamma)
 *
 * 여기서:
 *   T  = 기온 (°C)
 *   RH = 상대습도 (%)
 *   b  = 17.625
 *   c  = 243.04
 *   Td = 이슬점 온도 (°C)
 *
 * 이슬점: 공기 중 수증기가 응결을 시작하는 온도
 *         이슬점이 현재 온도에 가까울수록 습하게 느껴짐
 *
 * @param temperature: 온도 (°C)
 * @param humidity: 상대습도 (%)
 * @return: 이슬점 온도 (°C)
 */
float calculateDewPoint(float temperature, float humidity) {
  const float b = 17.625;
  const float c = 243.04;

  float gamma = log(humidity / 100.0) + (b * temperature) / (c + temperature);
  float dewPoint = (c * gamma) / (b - gamma);

  return dewPoint;
}

// ===================================================================
//  CSV 형식 출력 (데이터 로깅용)
// ===================================================================

/*
 * 센서 데이터를 CSV 형식으로 출력합니다.
 * 엑셀이나 Python 등으로 데이터를 분석할 때 유용합니다.
 *
 * 형식: timestamp,sht_temp,sht_humi,bmp_temp,bmp_pres,bmp_alti
 *
 * 사용법:
 *   1. 시리얼 모니터의 출력을 텍스트 파일로 저장
 *   2. .csv 확장자로 변경
 *   3. 엑셀이나 Python pandas로 열기
 */
void printCSVData() {
  // 첫 번째 줄 헤더 (한 번만 출력)
  static bool headerPrinted = false;
  if (!headerPrinted) {
    Serial.println("timestamp_ms,sht30_temp_c,sht30_humi_pct,bmp280_temp_c,bmp280_pres_hpa,bmp280_alti_m");
    headerPrinted = true;
  }

  // 데이터 행 출력
  Serial.print(millis());
  Serial.print(",");

  if (currentData.sht30_valid) {
    Serial.print(currentData.sht30_temperature, 2);
    Serial.print(",");
    Serial.print(currentData.sht30_humidity, 2);
  } else {
    Serial.print("NaN,NaN");
  }
  Serial.print(",");

  if (currentData.bmp280_valid) {
    Serial.print(currentData.bmp280_temperature, 2);
    Serial.print(",");
    Serial.print(currentData.bmp280_pressure, 2);
    Serial.print(",");
    Serial.print(currentData.bmp280_altitude, 1);
  } else {
    Serial.print("NaN,NaN,NaN");
  }
  Serial.println();
}

// ===================================================================
//  끝 (End of File)
// ===================================================================
