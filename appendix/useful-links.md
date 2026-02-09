# 유용한 링크 및 참고 자료

> ESP32 전자공학 학습에 도움이 되는 리소스 모음

---

## 목차

1. [학습 리소스](#1-학습-리소스)
2. [시뮬레이터](#2-시뮬레이터)
3. [회로 설계 도구](#3-회로-설계-도구)
4. [ESP32 관련 자료](#4-esp32-관련-자료)
5. [부품 구매처](#5-부품-구매처)
6. [데이터시트 검색](#6-데이터시트-검색)
7. [커뮤니티 및 포럼](#7-커뮤니티-및-포럼)
8. [유용한 계산 도구](#8-유용한-계산-도구)

---

## 1. 학습 리소스

### 1.1 유튜브 채널

#### 한국어 채널

| 채널명 | 주요 내용 | 추천 대상 | 링크 |
|--------|-----------|-----------|------|
| **코딩전자** | 아두이노/ESP32 프로젝트, 전자공학 기초 | 입문자 | youtube.com/@codingelectronics |
| **에듀아이** | 아두이노 기초, 센서 활용 실습 | 입문자 | youtube.com/@eduino |
| **자작나무숲 (BIRCHWOOD)** | 전자공학 이론, DIY 프로젝트 | 입문~중급 | youtube.com/@birchwoodforest |
| **뚝딱뚝딱** | 아두이노/라즈베리파이 프로젝트 | 입문~중급 | 유튜브에서 검색 |
| **피지컬 컴퓨팅** | IoT, 센서, 마이크로컨트롤러 | 중급 | 유튜브에서 검색 |

#### 영어 채널 (한국어 자막 지원 다수)

| 채널명 | 주요 내용 | 추천 대상 | 링크 |
|--------|-----------|-----------|------|
| **Ben Eater** | 디지털 전자공학 심층 강의, 8비트 컴퓨터 제작 | 중급~고급 | youtube.com/@BenEater |
| **GreatScott!** | 전자공학 기초~중급, 실용적 프로젝트 | 입문~중급 | youtube.com/@greatscottlab |
| **EEVblog** | 전자공학 전반, 장비 리뷰, 계측기 설명 | 중급~고급 | youtube.com/@EEVblog |
| **Andreas Spiess** | ESP32/IoT 프로젝트 전문 | 중급 | youtube.com/@AndreasSpiess |
| **DroneBot Workshop** | 아두이노/ESP32 튜토리얼, 센서 활용 | 입문~중급 | youtube.com/@Dronebotworkshop |
| **ElectroBOOM** | 전자공학 원리 (재미있는 설명) | 입문~중급 | youtube.com/@ElectroBOOM |
| **The Engineering Mindset** | 전기/전자 기초 이론 애니메이션 | 입문자 | youtube.com/@EngineeringMindset |
| **RalphS Bacon** | ESP32 심화 프로젝트, 전력 관리 | 중급 | youtube.com/@RalphBacon |
| **Paul McWhorter** | Arduino/ESP32 체계적 강좌 시리즈 | 입문자 | youtube.com/@paaborern |
| **Afrotechmods** | 전자부품 기초 설명 (짧고 명확) | 입문자 | youtube.com/@Afrotechmods |

---

### 1.2 온라인 강좌

#### 무료 강좌

| 플랫폼 | 강좌명/내용 | 언어 | 링크 |
|--------|------------|------|------|
| **Khan Academy** | 전기 회로 기초 (Electrical Engineering) | 영어 (한국어 자막) | khanacademy.org |
| **All About Circuits** | 전자공학 교과서 수준의 무료 온라인 교재 | 영어 | allaboutcircuits.com |
| **Electronics Tutorials** | 부품별/회로별 체계적 설명 | 영어 | electronics-tutorials.ws |
| **SparkFun Learn** | 전자공학 기초 튜토리얼 시리즈 | 영어 | learn.sparkfun.com |
| **Adafruit Learn** | 부품/모듈별 상세 가이드 | 영어 | learn.adafruit.com |
| **ESP32 IO** | ESP32 전용 튜토리얼 (Arduino + MicroPython) | 영어 | esp32io.com |

#### 유료 강좌

| 플랫폼 | 강좌명/내용 | 가격대 | 언어 |
|--------|------------|--------|------|
| **인프런** | "아두이노 입문", "IoT 프로젝트" 등 검색 | 무료~5만원 | 한국어 |
| **Udemy** | "ESP32 for Beginners", "Electronics Fundamentals" | 15,000~30,000원 (세일 시) | 영어 |
| **Coursera** | "Introduction to Electronics" (Georgia Tech) | 무료 청강 가능 | 영어 |

---

### 1.3 참고 서적

#### 한국어 서적

| 서적명 | 저자/출판사 | 내용 | 추천 대상 |
|--------|------------|------|-----------|
| **만들면서 배우는 아두이노와 40개의 작품들** | 서민우 / 앤써북 | 아두이노 기초 + 실습 프로젝트 | 입문자 |
| **사물인터넷을 위한 ESP32 프로그래밍** | 허경용 / 제이펍 | ESP32 종합 가이드 | 입문~중급 |
| **전자공학 입문** (시리즈) | 다수 저자 | 전자공학 이론 기초 | 입문자 |
| **핵심이 보이는 전자회로** | 고윤석 / 한빛아카데미 | 전자회로 이론 | 중급 |

#### 영어 서적

| 서적명 | 저자 | 내용 | 추천 대상 |
|--------|------|------|-----------|
| **Make: Electronics (3판)** | Charles Platt | 실습 중심 전자공학 입문서의 정석 | 입문자 |
| **The Art of Electronics (3판)** | Horowitz & Hill | 전자공학 바이블 (깊이 있는 이론) | 중급~고급 |
| **Practical Electronics for Inventors (4판)** | Paul Scherz | 실용적 전자공학 참고서 | 입문~중급 |
| **Getting Started with ESP32** | Espressif (공식) | ESP32 공식 시작 가이드 | 입문자 |
| **Internet of Things with ESP32** | 다수 저자 | ESP32 IoT 프로젝트 | 중급 |
| **Programming with MicroPython** | Nicholas Tollervey | MicroPython 기반 임베디드 프로그래밍 | 입문~중급 |

> **입문자 필독**: "Make: Electronics"는 전자공학을 처음 배우는 사람에게 가장 추천하는 책입니다. 실습 중심으로 기초 개념을 자연스럽게 익힐 수 있습니다.

---

## 2. 시뮬레이터

실제 하드웨어 없이도 회로를 테스트하고 학습할 수 있는 도구들입니다.

### 2.1 Wokwi - ESP32 온라인 시뮬레이터

| 항목 | 내용 |
|------|------|
| **웹사이트** | [wokwi.com](https://wokwi.com) |
| **특징** | ESP32/아두이노 전용 온라인 시뮬레이터 |
| **지원 보드** | ESP32, ESP32-S2, ESP32-S3, ESP32-C3, Arduino Uno/Mega/Nano 등 |
| **지원 부품** | LED, 버튼, 저항, 서보, LCD, OLED, DHT22, NeoPixel 등 다수 |
| **지원 프레임워크** | Arduino, MicroPython, ESP-IDF, Rust |
| **가격** | 무료 (일부 고급 기능 유료) |
| **장점** | 브라우저에서 바로 실행, ESP32 지원 최고, 코드+회로 동시 시뮬레이션 |
| **단점** | 아날로그 시뮬레이션 제한적, 일부 라이브러리 미지원 |

> **강력 추천**: 이 커리큘럼과 가장 잘 맞는 시뮬레이터입니다. 하드웨어를 구매하기 전에 Wokwi에서 회로를 미리 테스트해보세요. VS Code 확장도 제공합니다.

#### Wokwi 활용 팁

- 프로젝트 공유 링크로 다른 사람과 회로/코드 공유 가능
- `diagram.json` 파일로 회로 구성 관리
- VS Code의 Wokwi 확장을 설치하면 로컬에서도 시뮬레이션 가능
- 커뮤니티 프로젝트에서 다양한 예제 참고 가능

---

### 2.2 Tinkercad Circuits

| 항목 | 내용 |
|------|------|
| **웹사이트** | [tinkercad.com/circuits](https://www.tinkercad.com/circuits) |
| **특징** | Autodesk 제공, 비주얼 회로 시뮬레이터 |
| **지원 보드** | Arduino Uno, Micro:bit (ESP32 미지원) |
| **가격** | 완전 무료 |
| **장점** | 3D 시각화, 직관적 인터페이스, 브레드보드 배선 시뮬레이션 |
| **단점** | ESP32 미지원, 부품 종류 제한적 |

> 브레드보드 배선 연습과 기초 회로 학습에 적합합니다. ESP32는 지원하지 않으므로 Arduino Uno 기반으로 기초 개념을 연습할 때 활용하세요.

---

### 2.3 Falstad Circuit Simulator

| 항목 | 내용 |
|------|------|
| **웹사이트** | [falstad.com/circuit](https://falstad.com/circuit/) |
| **특징** | 아날로그 회로 시뮬레이션 전문 |
| **가격** | 완전 무료, 오픈소스 |
| **장점** | 전류 흐름 애니메이션, 실시간 파형 표시, 가볍고 빠름 |
| **단점** | 마이크로컨트롤러 미지원, UI가 다소 오래됨 |

> 전류의 흐름을 시각적으로 보여주어 Phase 1(전기 기초)과 Phase 2(부품 이해) 학습에 매우 유용합니다. 옴의 법칙, 직병렬 회로, 커패시터/인덕터 동작을 직관적으로 이해할 수 있습니다.

---

### 2.4 기타 시뮬레이터

| 시뮬레이터 | 용도 | 가격 | 링크 |
|-----------|------|------|------|
| **LTspice** | 아날로그 회로 정밀 시뮬레이션 (SPICE) | 무료 | analog.com/ltspice |
| **CircuitJS** | Falstad의 오픈소스 버전 | 무료 | lushprojects.com/circuitjs |
| **EveryCircuit** | 모바일 회로 시뮬레이터 (iOS/Android) | 유료 (약 $15) | everycircuit.com |
| **Proteus** | MCU + 회로 통합 시뮬레이션 | 유료 (학생판 약 $100) | labcenter.com |

---

## 3. 회로 설계 도구

### 3.1 KiCad - 무료 PCB 설계

| 항목 | 내용 |
|------|------|
| **웹사이트** | [kicad.org](https://www.kicad.org) |
| **가격** | 완전 무료, 오픈소스 |
| **플랫폼** | Windows, macOS, Linux |
| **주요 기능** | 회로도 작성, PCB 레이아웃, 3D 뷰어, Gerber 파일 출력 |
| **장점** | 전문가 수준의 무료 도구, 활발한 커뮤니티, 지속 업데이트 |
| **단점** | 학습 곡선 있음, 초보자에게 다소 복잡 |
| **추천 대상** | PCB 제작까지 고려하는 학습자, 중급 이상 |

#### KiCad 학습 자료

- 공식 문서: docs.kicad.org (영어)
- 유튜브: "KiCad 7 tutorial" 또는 "KiCad 8 tutorial" 검색
- DigiKey KiCad 튜토리얼 시리즈 (유튜브, 영어)

---

### 3.2 Fritzing - 브레드보드 다이어그램

| 항목 | 내용 |
|------|------|
| **웹사이트** | [fritzing.org](https://fritzing.org) |
| **가격** | 약 $10 (기부형 유료), 소스코드로 무료 빌드 가능 |
| **플랫폼** | Windows, macOS, Linux |
| **주요 기능** | 브레드보드 뷰, 회로도 뷰, PCB 뷰, 부품 라이브러리 |
| **장점** | 직관적인 브레드보드 다이어그램, 문서화에 적합 |
| **단점** | 업데이트 느림, 부품 라이브러리 제한적, 전문 PCB 설계에는 부족 |
| **추천 대상** | 입문자, 브레드보드 배선도 문서화 |

> 브레드보드 위에 실제처럼 부품을 배치하고 배선할 수 있어, 회로 문서화와 학습에 매우 직관적입니다.

---

### 3.3 EasyEDA - 온라인 회로 설계

| 항목 | 내용 |
|------|------|
| **웹사이트** | [easyeda.com](https://easyeda.com) |
| **가격** | 무료 (Pro 버전 유료) |
| **플랫폼** | 웹 브라우저, 데스크톱 앱 |
| **주요 기능** | 회로도, PCB 설계, SPICE 시뮬레이션, JLCPCB 직접 주문 |
| **장점** | 설치 불필요, LCSC 부품 라이브러리 통합, PCB 주문 연동 |
| **단점** | 인터넷 필요, 오프라인 작업 제한 |
| **추천 대상** | PCB 제작 입문, 빠른 프로토타이핑 |

> EasyEDA에서 설계한 PCB를 JLCPCB에서 바로 주문할 수 있어 편리합니다. PCB 제작 비용은 5장 기준 약 $2~5 + 배송비입니다.

---

### 3.4 회로 설계 도구 비교 요약

| 도구 | 가격 | 브레드보드 뷰 | PCB 설계 | 시뮬레이션 | 난이도 | 추천 |
|------|------|:----------:|:-------:|:---------:|:-----:|------|
| **KiCad** | 무료 | X | 전문가급 | 기본 SPICE | 중~상 | PCB 제작 목표 시 |
| **Fritzing** | ~$10 | O (최고) | 기본 | X | 하 | 입문, 문서화 |
| **EasyEDA** | 무료 | X | 좋음 | SPICE | 중 | PCB 주문 연동 |

---

## 4. ESP32 관련 자료

### 4.1 Espressif 공식 자료

| 자료 | 내용 | 링크 |
|------|------|------|
| **ESP-IDF 공식 문서** | ESP32 공식 개발 프레임워크 문서 | [docs.espressif.com](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/) |
| **ESP32 기술 참조 매뉴얼** | 레지스터, 주변장치 상세 설명 | Espressif 문서 내 Technical Reference Manual |
| **ESP32 데이터시트** | 전기적 특성, 핀 배치 | Espressif 문서 내 Datasheet |
| **ESP-IDF GitHub** | ESP-IDF 소스코드 및 예제 | [github.com/espressif/esp-idf](https://github.com/espressif/esp-idf) |
| **Espressif FAQ** | 자주 묻는 질문 모음 | Espressif 공식 문서 내 FAQ 섹션 |

---

### 4.2 ESP32 Arduino Core

| 자료 | 내용 | 링크 |
|------|------|------|
| **Arduino Core GitHub** | ESP32 Arduino 코어 소스코드 | [github.com/espressif/arduino-esp32](https://github.com/espressif/arduino-esp32) |
| **Arduino Core 문서** | API 레퍼런스, 설치 가이드 | [docs.espressif.com/projects/arduino-esp32](https://docs.espressif.com/projects/arduino-esp32/en/latest/) |
| **Arduino 공식 문서** | Arduino 언어 레퍼런스 | [arduino.cc/reference](https://www.arduino.cc/reference/en/) |
| **Arduino 라이브러리 매니저** | 라이브러리 검색 및 설치 | Arduino IDE 내장 |

---

### 4.3 ESP32 튜토리얼 사이트

| 사이트 | 내용 | 언어 | 링크 |
|--------|------|------|------|
| **Random Nerd Tutorials** | ESP32/ESP8266 최대 튜토리얼 사이트, 프로젝트 수백 개 | 영어 | [randomnerdtutorials.com](https://randomnerdtutorials.com) |
| **Last Minute Engineers** | ESP32/아두이노 부품별 상세 튜토리얼 | 영어 | [lastminuteengineers.com](https://lastminuteengineers.com) |
| **Microcontrollers Lab** | ESP32 프로젝트, MicroPython 튜토리얼 | 영어 | [microcontrollerslab.com](https://microcontrollerslab.com) |
| **CircuitDigest** | ESP32 프로젝트, 회로도 포함 | 영어 | [circuitdigest.com](https://circuitdigest.com) |
| **ESP32 IO** | ESP32 전용 Arduino/MicroPython 튜토리얼 | 영어 | [esp32io.com](https://esp32io.com) |
| **Techtutorialsx** | ESP32 BLE, Wi-Fi, 센서 튜토리얼 | 영어 | [techtutorialsx.com](https://techtutorialsx.com) |

> **가장 추천**: Random Nerd Tutorials는 ESP32 관련 가장 방대한 튜토리얼을 보유하고 있습니다. 센서 연결, 프로토콜 사용, 프로젝트 예제를 찾을 때 가장 먼저 검색해보세요.

---

### 4.4 ESP32 핵심 스펙 요약 (빠른 참조)

| 항목 | ESP32 (클래식) | ESP32-S3 | ESP32-C3 |
|------|---------------|----------|----------|
| CPU | Xtensa 듀얼코어 240MHz | Xtensa 듀얼코어 240MHz | RISC-V 싱글코어 160MHz |
| RAM | 520KB SRAM | 512KB SRAM | 400KB SRAM |
| Flash | 외장 4~16MB | 외장 4~16MB | 외장 4MB |
| Wi-Fi | 802.11 b/g/n | 802.11 b/g/n | 802.11 b/g/n |
| Bluetooth | BT 4.2 + BLE | BLE 5.0 | BLE 5.0 |
| GPIO | 34개 | 45개 | 22개 |
| ADC | 18채널 (12비트) | 20채널 (12비트) | 6채널 (12비트) |
| DAC | 2채널 (8비트) | 없음 | 없음 |
| 동작 전압 | 3.3V | 3.3V | 3.3V |
| GPIO 허용 전압 | 3.3V (5V 비허용) | 3.3V | 3.3V |
| 딥슬립 전류 | ~10uA | ~7uA | ~5uA |

> **중요**: ESP32의 GPIO는 3.3V 로직입니다. 5V 신호를 직접 연결하면 칩이 손상될 수 있습니다. 레벨 시프터를 사용하거나 전압 분배기를 통해 연결하세요. (Phase 3에서 자세히 다룹니다.)

---

## 5. 부품 구매처

### 5.1 국내 전자부품 전문

| 쇼핑몰 | 특징 | 배송 | 링크 |
|--------|------|------|------|
| **디바이스마트** | 국내 최대, 부품 종류 다양, 기술 자료 | 당일~익일 | [devicemart.co.kr](https://www.devicemart.co.kr) |
| **엘레파츠** | 아두이노/ESP32 키트 특화, 교육용 | 당일~익일 | [eleparts.co.kr](https://www.eleparts.co.kr) |
| **아이씨뱅큐** | 전자부품 전문, 산업용 | 당일~익일 | [icbanq.com](https://www.icbanq.com) |
| **메카솔루션** | 로봇/아두이노 특화, 키트 | 당일~익일 | [mechasolution.com](https://www.mechasolution.com) |
| **아두이노스토리** | 아두이노 전문, 튜토리얼 제공 | 1~2일 | [arduinostory.com](https://www.arduinostory.com) |
| **로보다인시스템** | 로봇/제어 부품 전문 | 1~2일 | [robodyne.co.kr](https://www.robodyne.co.kr) |

### 5.2 종합 쇼핑몰

| 쇼핑몰 | 전자부품 장점 | 비고 |
|--------|-------------|------|
| **쿠팡** | 로켓배송, 빠른 배송, 키트 다양 | 전문 부품은 제한적 |
| **네이버 쇼핑** | 가격 비교 편리, 판매자 다양 | 검색 키워드가 중요 |
| **11번가** | 해외직구 포함, 가격 경쟁력 | 배송 기간 확인 필요 |

### 5.3 해외 구매처

| 쇼핑몰 | 특징 | 배송 기간 | 비고 |
|--------|------|-----------|------|
| **AliExpress** | 최저가, 부품 종류 최다 | 2~4주 | 품질 편차 있음, 리뷰 확인 필수 |
| **Banggood** | AliExpress 대안, 가끔 더 빠른 배송 | 2~3주 | 할인 행사 잦음 |
| **Amazon (일본)** | 품질 신뢰, 한국 직배송 | 3~7일 | 배송비 고려 |
| **DigiKey** | 정품 부품 전문, 데이터시트 연동 | 3~5일 (DHL) | 소량도 정품 보장, 가격 높음 |
| **Mouser** | DigiKey와 유사, 정품 전문 | 3~5일 (DHL) | 기술 자료 풍부 |
| **LCSC** | 중국 부품 유통, EasyEDA 연동 | 1~2주 | PCB 부품 대량 구매에 적합 |

### 5.4 구매 전략

```
급하게 필요한 경우:
  └─> 쿠팡 로켓배송 또는 디바이스마트 (당일~익일)

일반적인 학습용 구매:
  └─> 디바이스마트, 엘레파츠 (국내 전문몰, 1~2일)

대량/저렴하게 구매:
  └─> AliExpress (2~4주 소요, 가격 1/3~1/5)

정품/신뢰성 중요:
  └─> DigiKey, Mouser (해외 정품 유통)

PCB 제작 + 부품:
  └─> JLCPCB + LCSC (설계~부품까지 원스톱)
```

---

## 6. 데이터시트 검색

데이터시트는 부품의 전기적 특성, 핀 배치, 사용법을 담은 공식 문서입니다. 전자공학에서 데이터시트를 읽는 능력은 필수입니다.

### 6.1 데이터시트 검색 사이트

| 사이트 | 특징 | 링크 |
|--------|------|------|
| **Alldatasheet** | 가장 대중적, 방대한 DB | [alldatasheet.com](https://www.alldatasheet.com) |
| **Datasheet Catalog** | 오래된 부품도 검색 가능 | [datasheetcatalog.com](https://www.datasheetcatalog.com) |
| **DigiKey** | 판매 부품의 데이터시트 직접 연결 | [digikey.com](https://www.digikey.com) |
| **Mouser** | DigiKey와 유사, 기술 자료 풍부 | [mouser.com](https://www.mouser.com) |
| **Octopart** | 부품 검색 + 가격 비교 + 데이터시트 | [octopart.com](https://octopart.com) |
| **Google 검색** | "[부품명] datasheet pdf" 로 검색 | google.com |

### 6.2 데이터시트 읽기 팁

부품의 데이터시트에서 가장 먼저 확인해야 할 항목:

1. **Absolute Maximum Ratings** - 절대 최대 정격 (이 값을 초과하면 부품 파손)
2. **Electrical Characteristics** - 동작 조건에서의 전기적 특성
3. **Pin Configuration** - 핀 배치도
4. **Typical Application Circuit** - 권장 회로 구성

> 데이터시트 읽기에 대한 자세한 내용은 Phase 2의 [데이터시트 읽기](../phase2-components/08-datasheet-reading.md)를 참고하세요.

---

## 7. 커뮤니티 및 포럼

### 7.1 한국어 커뮤니티

| 커뮤니티 | 주요 주제 | 링크 |
|---------|-----------|------|
| **네이버 카페 "아두이노 스토리"** | 아두이노/ESP32 질문, 프로젝트 공유 | 네이버 카페 검색 |
| **네이버 카페 "라즈베리파이"** | 라즈베리파이, 임베디드 전반 | 네이버 카페 검색 |
| **클리앙 DIY 게시판** | DIY/전자공학 프로젝트 공유 | [clien.net](https://www.clien.net) |
| **뽐뿌 DIY/전자 게시판** | DIY 프로젝트, 장비 리뷰 | [ppomppu.co.kr](https://www.ppomppu.co.kr) |

### 7.2 영어 커뮤니티

| 커뮤니티 | 주요 주제 | 링크 |
|---------|-----------|------|
| **ESP32 공식 포럼** | ESP32 관련 공식 질문/답변 | [esp32.com](https://www.esp32.com) |
| **Arduino Forum** | Arduino/ESP32 질문, 프로젝트 공유 | [forum.arduino.cc](https://forum.arduino.cc) |
| **Reddit r/esp32** | ESP32 프로젝트, 질문, 뉴스 | [reddit.com/r/esp32](https://www.reddit.com/r/esp32) |
| **Reddit r/arduino** | Arduino 전반, 입문자 질문 | [reddit.com/r/arduino](https://www.reddit.com/r/arduino) |
| **Reddit r/electronics** | 전자공학 전반 | [reddit.com/r/electronics](https://www.reddit.com/r/electronics) |
| **Reddit r/AskElectronics** | 전자공학 질문 특화 | [reddit.com/r/AskElectronics](https://www.reddit.com/r/AskElectronics) |
| **Stack Overflow** | 프로그래밍 관련 질문 | [stackoverflow.com](https://stackoverflow.com) (태그: esp32) |
| **Electrical Engineering Stack Exchange** | 전자공학 이론/설계 질문 | [electronics.stackexchange.com](https://electronics.stackexchange.com) |
| **Hackaday** | 전자공학 프로젝트 소개, 뉴스 | [hackaday.com](https://hackaday.com) |
| **Instructables** | DIY 프로젝트 상세 가이드 | [instructables.com](https://www.instructables.com) |

### 7.3 Discord/채팅 커뮤니티

| 커뮤니티 | 특징 |
|---------|------|
| **Arduino Discord** | 실시간 질문/답변, 프로젝트 공유 |
| **ESP8266/ESP32 Discord** | ESP 칩 전용 커뮤니티 |
| **r/electronics Discord** | 전자공학 전반 토론 |

---

## 8. 유용한 계산 도구

### 8.1 온라인 계산기

| 도구 | 용도 | 링크 |
|------|------|------|
| **저항 컬러코드 계산기** | 저항값 확인 (색띠 해독) | "resistor color code calculator" 검색 |
| **LED 저항 계산기** | LED 전류 제한 저항 계산 | [ledcalculator.net](https://ledcalculator.net) |
| **옴의 법칙 계산기** | V=IR 관련 계산 | "ohms law calculator" 검색 |
| **전압 분배기 계산기** | 전압 분배 회로 설계 | "voltage divider calculator" 검색 |
| **RC 시정수 계산기** | 커패시터 충방전 시간 계산 | "rc time constant calculator" 검색 |
| **PCB 트레이스 폭 계산기** | PCB 배선 폭 결정 | "pcb trace width calculator" 검색 |
| **디커플링 캐패시터 계산기** | 바이패스 캐패시터 선정 | 제조사 앱노트 참고 |

### 8.2 모바일 앱

| 앱 | 플랫폼 | 용도 | 가격 |
|----|--------|------|------|
| **ElectroDroid** | Android/iOS | 전자공학 종합 참고 앱 (저항 계산, 핀아웃 등) | 무료/유료 |
| **Electronics Toolkit** | Android | 각종 전자공학 계산기 | 무료 |
| **Droid Tesla** | Android | 회로 시뮬레이터 | 무료 |
| **iCircuit** | iOS | 회로 시뮬레이터 | 유료 (약 $10) |

---

## 부록: 학습 로드맵 요약

이 커리큘럼과 위 리소스들을 활용한 추천 학습 순서입니다.

```
1단계: 기초 이론 다지기
   ├─ 이 커리큘럼의 Phase 1~2 학습
   ├─ Falstad 시뮬레이터로 회로 동작 시각화
   └─ 유튜브 (The Engineering Mindset, Afrotechmods)

2단계: 실습 시작
   ├─ 이 커리큘럼의 Phase 3~4 학습
   ├─ Wokwi 시뮬레이터로 ESP32 코드 테스트
   ├─ 실제 브레드보드에 회로 구성
   └─ Random Nerd Tutorials 예제 따라하기

3단계: 프로젝트 수행
   ├─ 이 커리큘럼의 Phase 5~6 학습
   ├─ 자신만의 프로젝트 기획 및 제작
   └─ 커뮤니티에서 프로젝트 공유 및 피드백

4단계: 심화 학습 (선택)
   ├─ KiCad로 PCB 설계
   ├─ ESP-IDF 프레임워크 학습
   ├─ The Art of Electronics 참고
   └─ 오실로스코프/로직 분석기 활용
```

---

> **이전 단계**: [추천 도구/장비 가이드](./tools-and-equipment.md)에서 필요한 장비를 확인하세요.
