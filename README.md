# ESP32 & 라즈베리파이를 위한 전자공학 기초

> 소프트웨어 개발자를 위한 실습 중심 전자공학 커리큘럼

## 커리큘럼 개요

| Phase | 주제 | 예상 기간 | 핵심 목표 |
|-------|------|-----------|-----------|
| [Phase 1](./phase1-electrical-basics/) | 전기/전자 기초 이론 | 1~2주 | 회로를 "읽을 수" 있게 되기 |
| [Phase 2](./phase2-components/) | 핵심 부품 이해 | 1~2주 | 자주 쓰는 부품의 역할과 사용법 |
| [Phase 3](./phase3-circuit-design/) | 회로 설계 기초 | 1~2주 | 이론을 실제 연결로 옮기기 |
| [Phase 4](./phase4-communication-protocols/) | 디지털 통신 프로토콜 | 2~3주 | 센서/모듈 연결의 핵심 |
| [Phase 5](./phase5-power-design/) | 전원 설계 | 1주 | 안정적인 전원 공급 설계 |
| [Phase 6](./phase6-projects/) | 실전 프로젝트 | 지속 | 이론의 체화 |

## 사전 준비물

| 장비 | 용도 | 예산 |
|------|------|------|
| 멀티미터 | 전압/저항/통전 측정 (필수!) | 1~3만원 |
| 브레드보드 + 점퍼선 | 시제품 회로 구성 | 5천원 |
| 기본 부품 키트 | 저항/LED/커패시터/트랜지스터 세트 | 1~2만원 |
| ESP32 개발보드 | 실습용 마이크로컨트롤러 | 1만원 내외 |
| USB 케이블 (Type-C 또는 Micro-USB) | ESP32 연결용 | 보유 |

자세한 장비 가이드: [추천 도구/장비](./appendix/tools-and-equipment.md)

## 학습 방법

1. **각 Phase의 README.md**를 먼저 읽고 전체 흐름을 파악하세요
2. **번호 순서대로** 각 문서를 학습하세요
3. **코드 예제**는 직접 ESP32에 업로드하여 실행해보세요
4. **회로도**는 실제 브레드보드에 구성해보세요
5. 이해가 안 되는 부분은 **Phase 6 실전 프로젝트**에서 다시 만나게 됩니다

## 개발 환경 설정

### Arduino IDE (권장 - 입문자)
1. [Arduino IDE](https://www.arduino.cc/en/software) 설치
2. `파일 > 환경설정 > 추가 보드 매니저 URL`에 추가:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. `도구 > 보드 > 보드 매니저`에서 "ESP32" 검색 후 설치
4. 보드 선택: `ESP32 Dev Module`

### PlatformIO (권장 - 경험자)
1. VS Code 설치
2. PlatformIO 확장 설치
3. 새 프로젝트 생성 시 Board: `Espressif ESP32 Dev Module` 선택

## 디렉토리 구조

```
Electronic-Fundamentals/
├── README.md                          ← 현재 파일
├── phase1-electrical-basics/          ← 전기/전자 기초 이론
│   ├── 01-ohms-law.md
│   ├── 02-power.md
│   ├── 03-series-parallel.md
│   ├── 04-dc-ac.md
│   └── 05-ground.md
├── phase2-components/                 ← 핵심 부품 이해
│   ├── 01-resistor.md
│   ├── 02-led.md
│   ├── 03-capacitor.md
│   ├── 04-diode.md
│   ├── 05-transistor-mosfet.md
│   ├── 06-relay.md
│   ├── 07-regulator.md
│   └── 08-datasheet-reading.md
├── phase3-circuit-design/             ← 회로 설계 기초
│   ├── 01-breadboard.md
│   ├── 02-schematic-reading.md
│   ├── 03-pullup-pulldown.md
│   ├── 04-voltage-divider.md
│   ├── 05-decoupling-capacitor.md
│   └── 06-multimeter.md
├── phase4-communication-protocols/    ← 디지털 통신 프로토콜
│   ├── 01-gpio.md
│   ├── 02-adc.md
│   ├── 03-pwm.md
│   ├── 04-i2c.md
│   ├── 05-spi.md
│   ├── 06-uart.md
│   ├── 07-onewire.md
│   └── code/
├── phase5-power-design/               ← 전원 설계
│   ├── 01-power-calculation.md
│   ├── 02-voltage-levels.md
│   ├── 03-regulator-vs-dcdc.md
│   ├── 04-protection.md
│   └── examples/
├── phase6-projects/                   ← 실전 프로젝트
│   ├── project1-led-button/
│   ├── project2-analog-sensor/
│   ├── project3-i2c-sensor/
│   ├── project4-relay-control/
│   └── project5-integrated-system/
└── appendix/                          ← 부록
    ├── tools-and-equipment.md
    └── useful-links.md
```
