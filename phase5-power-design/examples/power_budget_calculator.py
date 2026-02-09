#!/usr/bin/env python3
"""
ESP32 프로젝트 전력 예산 계산기
================================
이 스크립트는 ESP32 기반 프로젝트의 전력 소비를 분석하고
적절한 전원 공급 장치를 추천합니다.

주요 기능:
  - 부품별 전류 소비 계산
  - 전원 공급 장치 추천 (어댑터, 배터리)
  - 배터리 수명 예측
  - 리니어 레귤레이터 발열 계산
  - 전력 보고서 출력

사용법:
  python power_budget_calculator.py            # 대화형 모드
  python power_budget_calculator.py --example  # 예제 프로젝트 실행
"""

from dataclasses import dataclass, field
from typing import Optional
import sys
import math


# =============================================================================
# 데이터 구조 정의
# =============================================================================

@dataclass
class Component:
    """
    전자 부품 정보를 담는 데이터 클래스.

    Attributes:
        name: 부품 이름
        voltage: 동작 전압 (V)
        current_ma: 전류 소비 (mA)
        quantity: 사용 수량
        mode: 동작 모드 (예: 'Active WiFi', 'Deep Sleep')
        note: 추가 참고 사항
    """
    name: str
    voltage: float
    current_ma: float
    quantity: int = 1
    mode: str = ""
    note: str = ""

    @property
    def total_current_ma(self) -> float:
        """수량을 반영한 총 전류 소비 (mA)"""
        return self.current_ma * self.quantity

    @property
    def power_mw(self) -> float:
        """수량을 반영한 총 전력 소비 (mW)"""
        return self.voltage * self.total_current_ma


@dataclass
class PowerSupply:
    """
    전원 공급 장치 정보.

    Attributes:
        name: 전원 이름
        voltage: 출력 전압 (V)
        max_current_ma: 최대 출력 전류 (mA)
        price_range: 대략적인 가격대
    """
    name: str
    voltage: float
    max_current_ma: float
    price_range: str = ""


@dataclass
class Battery:
    """
    배터리 정보.

    Attributes:
        name: 배터리 이름/종류
        capacity_mah: 용량 (mAh)
        voltage: 공칭 전압 (V)
        note: 참고 사항
    """
    name: str
    capacity_mah: float
    voltage: float
    note: str = ""


# =============================================================================
# 사전 정의된 부품 라이브러리 (카탈로그)
# =============================================================================

# ESP32 동작 모드별 전류 소비 사전
# 출처: ESP32 데이터시트 (Espressif Systems)
ESP32_MODES = {
    "active_wifi": Component(
        name="ESP32",
        voltage=3.3,
        current_ma=240.0,
        mode="Active WiFi",
        note="WiFi 송수신 시 평균 소비 전류",
    ),
    "active_bt": Component(
        name="ESP32",
        voltage=3.3,
        current_ma=130.0,
        mode="Active Bluetooth",
        note="블루투스 송수신 시 평균 소비 전류",
    ),
    "light_sleep": Component(
        name="ESP32",
        voltage=3.3,
        current_ma=0.8,
        mode="Light Sleep",
        note="라이트 슬립 모드 (RTC 메모리 유지)",
    ),
    "deep_sleep": Component(
        name="ESP32",
        voltage=3.3,
        current_ma=0.01,
        mode="Deep Sleep",
        note="딥 슬립 모드 (RTC만 동작, 최소 소비)",
    ),
}

# 센서 및 모듈 카탈로그
COMPONENT_CATALOG = {
    "sht30": Component(
        name="SHT30 온습도 센서",
        voltage=3.3,
        current_ma=0.6,
        note="I2C 통신, 측정 시 최대 1.5mA (평균 0.6mA)",
    ),
    "bmp280": Component(
        name="BMP280 기압/온도 센서",
        voltage=3.3,
        current_ma=0.3,
        note="I2C 통신, 강제 모드에서 매우 저전력",
    ),
    "oled_ssd1306": Component(
        name="OLED SSD1306 디스플레이",
        voltage=3.3,
        current_ma=20.0,
        note="128x64 해상도, 실제 소비는 표시 내용에 따라 변동",
    ),
    "mq2": Component(
        name="MQ-2 가스 센서",
        voltage=5.0,
        current_ma=150.0,
        note="히터 포함, 예열 시간 필요 (약 20초), 5V 전원 필요",
    ),
    "relay": Component(
        name="릴레이 모듈",
        voltage=5.0,
        current_ma=70.0,
        note="코일 동작 시 전류, 대기 시 약 5mA",
    ),
    "led": Component(
        name="LED",
        voltage=3.3,
        current_ma=20.0,
        note="표준 LED (적색 기준), 저항 포함",
    ),
    "ds18b20": Component(
        name="DS18B20 온도 센서",
        voltage=3.3,
        current_ma=1.5,
        note="1-Wire 통신, 변환 시 최대 1.5mA",
    ),
}

# 일반적인 배터리 종류
COMMON_BATTERIES = [
    Battery("18650 리튬이온", 3000.0, 3.7, "충전 가능, ESP32 프로젝트에 가장 적합"),
    Battery("AA 알카라인 (x2 직렬)", 2500.0, 3.0, "2개 직렬 = 3V, 레귤레이터 필요할 수 있음"),
    Battery("LiPo 1000mAh", 1000.0, 3.7, "소형 웨어러블/IoT 프로젝트용"),
    Battery("LiPo 2000mAh", 2000.0, 3.7, "중형 IoT 프로젝트용"),
    Battery("CR2032 코인셀", 220.0, 3.0, "딥 슬립 위주 초저전력 프로젝트만 적합"),
]

# 전원 공급 장치 목록
POWER_SUPPLIES = [
    PowerSupply("USB 충전기 (소형)", 5.0, 500, "3,000~5,000원"),
    PowerSupply("USB 충전기 (일반)", 5.0, 1000, "5,000~8,000원"),
    PowerSupply("USB 충전기 (고출력)", 5.0, 2000, "8,000~15,000원"),
    PowerSupply("5V 2A 어댑터", 5.0, 2000, "5,000~10,000원"),
    PowerSupply("5V 3A 어댑터", 5.0, 3000, "8,000~15,000원"),
    PowerSupply("12V 1A 어댑터", 12.0, 1000, "5,000~10,000원"),
    PowerSupply("12V 2A 어댑터", 12.0, 2000, "8,000~15,000원"),
]

# 안전 여유율 (20~30% 권장, 기본값 25%)
SAFETY_MARGIN = 0.25


# =============================================================================
# 전력 계산 함수
# =============================================================================

def calculate_total_current(components: list[Component]) -> float:
    """
    모든 부품의 전류 소비 합계를 계산합니다.

    Args:
        components: 부품 목록

    Returns:
        총 전류 소비 (mA)
    """
    return sum(c.total_current_ma for c in components)


def calculate_total_power(components: list[Component]) -> float:
    """
    모든 부품의 전력 소비 합계를 계산합니다.

    Args:
        components: 부품 목록

    Returns:
        총 전력 소비 (mW)
    """
    return sum(c.power_mw for c in components)


def apply_safety_margin(value: float, margin: float = SAFETY_MARGIN) -> float:
    """
    안전 여유율을 적용한 값을 반환합니다.

    설계 시 항상 여유를 두는 것이 좋습니다.
    - 측정 오차, 돌입 전류, 온도 변화 등을 고려

    Args:
        value: 원래 값
        margin: 여유율 (기본값 0.25 = 25%)

    Returns:
        여유율이 적용된 값
    """
    return value * (1.0 + margin)


def recommend_power_supply(total_current_ma: float) -> list[PowerSupply]:
    """
    총 전류 소비량에 기반하여 적절한 전원 공급 장치를 추천합니다.

    안전 여유율(25%)을 적용한 후, 해당 전류를 감당할 수 있는
    전원 공급 장치를 필터링하여 반환합니다.

    Args:
        total_current_ma: 총 전류 소비 (mA)

    Returns:
        추천 전원 공급 장치 목록
    """
    required_ma = apply_safety_margin(total_current_ma)
    suitable = []
    for ps in POWER_SUPPLIES:
        if ps.max_current_ma >= required_ma:
            suitable.append(ps)
    return suitable


def calculate_battery_life(
    total_current_ma: float,
    battery: Battery,
    duty_cycle: float = 1.0,
) -> dict:
    """
    배터리 수명을 예측합니다.

    실제 배터리 수명은 다음 요인에 의해 달라집니다:
    - 방전 곡선 (리튬이온은 비교적 평탄, 알카라인은 점진적 하락)
    - 온도 (저온에서 용량 감소)
    - 자기 방전 (장기간 보관 시)
    - Duty cycle (간헐적 동작 패턴)

    Args:
        total_current_ma: 총 전류 소비 (mA)
        battery: 배터리 정보
        duty_cycle: 듀티 사이클 (0.0~1.0, 1.0 = 항상 활성)

    Returns:
        수명 정보 딕셔너리 (시간, 일, 실효 전류 등)
    """
    if total_current_ma <= 0:
        return {"hours": float("inf"), "days": float("inf"), "effective_current_ma": 0}

    # 듀티 사이클 적용한 실효 전류
    effective_current = total_current_ma * duty_cycle

    # 실제 사용 가능 용량 (약 80% 효율 적용 - 방전 곡선, 컷오프 전압 고려)
    usable_capacity = battery.capacity_mah * 0.8

    hours = usable_capacity / effective_current if effective_current > 0 else float("inf")
    days = hours / 24.0

    return {
        "battery_name": battery.name,
        "capacity_mah": battery.capacity_mah,
        "usable_capacity_mah": usable_capacity,
        "effective_current_ma": round(effective_current, 3),
        "hours": round(hours, 2),
        "days": round(days, 2),
    }


def calculate_heat_dissipation(
    vin: float,
    vout: float,
    current_ma: float,
) -> dict:
    """
    리니어 레귤레이터의 발열량을 계산합니다.

    리니어 레귤레이터는 입출력 전압 차이를 열로 소비합니다.
    공식: P_heat = (Vin - Vout) x I

    일반적인 기준:
    - 0.5W 이하: 방열판 불필요
    - 0.5~1.0W: 소형 방열판 권장
    - 1.0W 이상: 큰 방열판 필수 또는 DC-DC 컨버터 전환 권장

    Args:
        vin: 입력 전압 (V)
        vout: 출력 전압 (V)
        current_ma: 출력 전류 (mA)

    Returns:
        발열 정보 딕셔너리
    """
    voltage_drop = vin - vout
    current_a = current_ma / 1000.0
    power_w = voltage_drop * current_a
    power_mw = power_w * 1000.0

    # 효율 계산
    output_power_w = vout * current_a
    input_power_w = vin * current_a
    efficiency = (output_power_w / input_power_w * 100.0) if input_power_w > 0 else 0

    # 경고 수준 판단
    if power_w < 0.5:
        warning_level = "안전"
        recommendation = "방열판 없이 사용 가능합니다."
    elif power_w < 1.0:
        warning_level = "주의"
        recommendation = "소형 방열판을 부착하세요."
    elif power_w < 2.0:
        warning_level = "경고"
        recommendation = "큰 방열판이 필요합니다. DC-DC 컨버터 전환을 고려하세요."
    else:
        warning_level = "위험"
        recommendation = (
            "리니어 레귤레이터 사용이 부적합합니다! "
            "반드시 DC-DC 벅 컨버터를 사용하세요."
        )

    return {
        "vin": vin,
        "vout": vout,
        "voltage_drop": round(voltage_drop, 2),
        "current_ma": current_ma,
        "heat_dissipation_mw": round(power_mw, 1),
        "heat_dissipation_w": round(power_w, 3),
        "efficiency_percent": round(efficiency, 1),
        "warning_level": warning_level,
        "recommendation": recommendation,
    }


# =============================================================================
# 보고서 출력 함수
# =============================================================================

def print_separator(char: str = "=", length: int = 72) -> None:
    """구분선 출력"""
    print(char * length)


def print_power_report(
    components: list[Component],
    project_name: str = "ESP32 프로젝트",
    vin: float = 5.0,
    vout: float = 3.3,
) -> None:
    """
    전력 예산 종합 보고서를 출력합니다.

    Args:
        components: 프로젝트에 사용되는 부품 목록
        project_name: 프로젝트 이름
        vin: 전원 입력 전압 (V)
        vout: 레귤레이터 출력 전압 (V)
    """
    total_current = calculate_total_current(components)
    total_power = calculate_total_power(components)
    margin_current = apply_safety_margin(total_current)

    print()
    print_separator("=")
    print(f"  전력 예산 보고서: {project_name}")
    print_separator("=")

    # ----- 부품별 전류 소비 표 -----
    print()
    print("[ 부품별 전류 소비 ]")
    print_separator("-")
    header = f"{'부품 이름':<28} {'모드':<16} {'전압':>5} {'전류(mA)':>9} {'수량':>4} {'합계(mA)':>9}"
    print(header)
    print_separator("-")

    for c in components:
        mode_str = c.mode if c.mode else "-"
        print(
            f"{c.name:<28} {mode_str:<16} {c.voltage:>5.1f} "
            f"{c.current_ma:>9.2f} {c.quantity:>4} {c.total_current_ma:>9.2f}"
        )

    print_separator("-")
    print(f"{'총 전류 소비':<46} {total_current:>9.2f} mA")
    print(f"{'총 전력 소비':<46} {total_power:>9.1f} mW")
    print(f"{'안전 여유율 ({:.0f}%) 적용 후'.format(SAFETY_MARGIN * 100):<46} {margin_current:>9.2f} mA")
    print()

    # ----- 전원 공급 장치 추천 -----
    print("[ 전원 공급 장치 추천 ]")
    print_separator("-")
    recommendations = recommend_power_supply(total_current)
    if recommendations:
        print(f"  필요 전류 (여유 포함): {margin_current:.1f} mA")
        print()
        for ps in recommendations:
            utilization = (margin_current / ps.max_current_ma) * 100
            print(
                f"  - {ps.name:<24} "
                f"({ps.voltage}V / {ps.max_current_ma}mA) "
                f"사용률: {utilization:.0f}%  "
                f"가격: {ps.price_range}"
            )
    else:
        print("  [!] 적합한 전원 공급 장치를 찾을 수 없습니다.")
        print(f"      필요 전류: {margin_current:.1f} mA - 더 높은 용량의 전원이 필요합니다.")
    print()

    # ----- 배터리 수명 예측 -----
    print("[ 배터리 수명 예측 (항상 활성 모드) ]")
    print_separator("-")
    print(f"  {'배터리 종류':<28} {'용량':>8} {'예상 수명(시간)':>14} {'예상 수명(일)':>13}")
    print(f"  {'-'*28} {'-'*8} {'-'*14} {'-'*13}")

    for bat in COMMON_BATTERIES:
        result = calculate_battery_life(total_current, bat)
        hours_str = f"{result['hours']:.1f}"
        days_str = f"{result['days']:.1f}"
        print(
            f"  {bat.name:<28} {bat.capacity_mah:>7.0f} "
            f"{hours_str:>14} {days_str:>13}"
        )

    print()
    print("  * 실제 수명은 방전 효율(80%), 듀티 사이클, 온도에 따라 달라집니다.")
    print("  * 딥 슬립 모드를 활용하면 배터리 수명을 크게 늘릴 수 있습니다.")
    print()

    # ----- 리니어 레귤레이터 발열 분석 -----
    print("[ 리니어 레귤레이터 발열 분석 ]")
    print_separator("-")
    heat = calculate_heat_dissipation(vin, vout, total_current)

    print(f"  입력 전압:      {heat['vin']:.1f} V")
    print(f"  출력 전압:      {heat['vout']:.1f} V")
    print(f"  전압 강하:      {heat['voltage_drop']:.1f} V")
    print(f"  출력 전류:      {heat['current_ma']:.1f} mA")
    print(f"  발열량:         {heat['heat_dissipation_mw']:.1f} mW ({heat['heat_dissipation_w']:.3f} W)")
    print(f"  효율:           {heat['efficiency_percent']:.1f}%")
    print(f"  상태:           [{heat['warning_level']}] {heat['recommendation']}")

    # 12V 입력의 경우도 표시 (일반적인 시나리오)
    if vin != 12.0:
        heat_12v = calculate_heat_dissipation(12.0, vout, total_current)
        print()
        print(f"  참고) 12V 어댑터 사용 시:")
        print(f"    발열량: {heat_12v['heat_dissipation_mw']:.1f} mW ({heat_12v['heat_dissipation_w']:.3f} W)")
        print(f"    효율: {heat_12v['efficiency_percent']:.1f}%")
        print(f"    상태: [{heat_12v['warning_level']}] {heat_12v['recommendation']}")

    print()

    # ----- 설계 팁 -----
    print("[ 설계 팁 ]")
    print_separator("-")
    _print_design_tips(components, total_current, heat)

    print_separator("=")
    print()


def _print_design_tips(
    components: list[Component],
    total_current: float,
    heat_info: dict,
) -> None:
    """프로젝트 상황에 맞는 설계 팁을 출력합니다."""
    tips = []

    # MQ-2 가스 센서 사용 시 주의
    has_mq2 = any("MQ-2" in c.name for c in components)
    if has_mq2:
        tips.append(
            "MQ-2 가스 센서는 히터로 인해 전류 소비가 높습니다. "
            "배터리 구동 시 간헐적으로 히터를 켜는 방식을 고려하세요."
        )

    # 높은 전류 소비 시
    if total_current > 500:
        tips.append(
            "총 전류가 500mA를 초과합니다. "
            "USB 전원으로는 부족할 수 있으니 별도 어댑터를 사용하세요."
        )

    # 발열 경고 시
    if heat_info["heat_dissipation_w"] > 0.5:
        tips.append(
            "리니어 레귤레이터 발열이 큽니다. "
            "AMS1117 대신 DC-DC 벅 컨버터(MP1584, LM2596 등)를 사용하면 "
            "효율이 85~95%로 개선됩니다."
        )

    # 배터리 구동 팁
    tips.append(
        "배터리 수명을 늘리려면 ESP32의 딥 슬립 모드를 활용하세요. "
        "딥 슬립 시 전류가 0.01mA로 줄어듭니다."
    )

    # OLED 사용 시
    has_oled = any("OLED" in c.name or "SSD1306" in c.name for c in components)
    if has_oled:
        tips.append(
            "OLED 디스플레이는 표시 픽셀 수에 비례하여 전류가 증가합니다. "
            "화면 밝기를 줄이거나 일정 시간 후 꺼두면 전력을 절약할 수 있습니다."
        )

    # 릴레이 사용 시
    has_relay = any("릴레이" in c.name for c in components)
    if has_relay:
        tips.append(
            "릴레이 코일의 역기전력 보호를 위해 플라이백 다이오드를 반드시 설치하세요. "
            "대부분의 릴레이 모듈에는 이미 포함되어 있습니다."
        )

    for i, tip in enumerate(tips, 1):
        print(f"  {i}. {tip}")


# =============================================================================
# 대화형 모드
# =============================================================================

def interactive_mode() -> None:
    """
    사용자와 대화하며 부품을 선택하고 전력 보고서를 생성합니다.
    """
    print()
    print_separator("*")
    print("  ESP32 전력 예산 계산기 - 대화형 모드")
    print_separator("*")
    print()

    # 프로젝트 이름 입력
    project_name = input("프로젝트 이름을 입력하세요 (Enter = 기본값): ").strip()
    if not project_name:
        project_name = "내 ESP32 프로젝트"

    selected_components: list[Component] = []

    # ----- ESP32 모드 선택 -----
    print()
    print("1단계: ESP32 동작 모드를 선택하세요")
    print_separator("-", 40)
    mode_keys = list(ESP32_MODES.keys())
    for i, key in enumerate(mode_keys, 1):
        mode = ESP32_MODES[key]
        print(f"  {i}. {mode.mode:<20} ({mode.current_ma} mA) - {mode.note}")

    while True:
        try:
            choice = input("\n모드 번호를 입력하세요 [1]: ").strip()
            if not choice:
                choice_idx = 0
            else:
                choice_idx = int(choice) - 1
            if 0 <= choice_idx < len(mode_keys):
                esp_mode = ESP32_MODES[mode_keys[choice_idx]]
                # dataclass를 복사하여 독립적인 인스턴스 생성
                selected_components.append(Component(
                    name=esp_mode.name,
                    voltage=esp_mode.voltage,
                    current_ma=esp_mode.current_ma,
                    quantity=1,
                    mode=esp_mode.mode,
                    note=esp_mode.note,
                ))
                print(f"  -> '{esp_mode.mode}' 모드 선택됨 ({esp_mode.current_ma} mA)")
                break
            else:
                print("  [!] 올바른 번호를 입력하세요.")
        except ValueError:
            print("  [!] 숫자를 입력하세요.")

    # ----- 추가 부품 선택 -----
    print()
    print("2단계: 추가 부품을 선택하세요 (완료하려면 'q' 입력)")
    print_separator("-", 40)
    catalog_keys = list(COMPONENT_CATALOG.keys())
    for i, key in enumerate(catalog_keys, 1):
        comp = COMPONENT_CATALOG[key]
        print(f"  {i}. {comp.name:<28} ({comp.current_ma:>6.1f} mA, {comp.voltage}V)")

    while True:
        print()
        choice = input("부품 번호를 입력하세요 (q=완료): ").strip().lower()
        if choice == "q" or choice == "":
            if choice == "":
                # 빈 입력 시 한 번 더 확인
                confirm = input("부품 추가를 완료하시겠습니까? (y/n) [y]: ").strip().lower()
                if confirm not in ("", "y", "yes"):
                    continue
            break

        try:
            idx = int(choice) - 1
            if 0 <= idx < len(catalog_keys):
                comp = COMPONENT_CATALOG[catalog_keys[idx]]
                qty_str = input(f"  '{comp.name}'의 수량을 입력하세요 [1]: ").strip()
                qty = int(qty_str) if qty_str else 1
                if qty < 1:
                    print("  [!] 수량은 1 이상이어야 합니다.")
                    continue

                selected_components.append(Component(
                    name=comp.name,
                    voltage=comp.voltage,
                    current_ma=comp.current_ma,
                    quantity=qty,
                    mode=comp.mode,
                    note=comp.note,
                ))
                print(f"  -> '{comp.name}' x{qty} 추가됨 (합계: {comp.current_ma * qty:.1f} mA)")
            else:
                print("  [!] 올바른 번호를 입력하세요.")
        except ValueError:
            print("  [!] 숫자를 입력하세요.")

    # ----- 전원 입력 전압 설정 -----
    print()
    vin_str = input("전원 입력 전압을 입력하세요 (V) [5.0]: ").strip()
    vin = float(vin_str) if vin_str else 5.0

    # ----- 보고서 출력 -----
    if not selected_components:
        print("\n  [!] 선택된 부품이 없습니다. 종료합니다.")
        return

    print_power_report(selected_components, project_name, vin=vin, vout=3.3)


# =============================================================================
# 예제 프로젝트: ESP32 환경 모니터링 시스템
# =============================================================================

def run_example() -> None:
    """
    예제 프로젝트로 전력 보고서를 생성합니다.

    프로젝트 구성:
      - ESP32 (WiFi 활성 모드)
      - SHT30 온습도 센서 x1
      - OLED SSD1306 디스플레이 x1
      - 릴레이 모듈 x2 (환기팬, 가습기 제어)

    이 구성은 실내 환경을 모니터링하고 WiFi로 데이터를 전송하며,
    설정 조건에 따라 릴레이로 장치를 제어하는 시스템입니다.
    """
    print()
    print("=" * 72)
    print("  예제 프로젝트: 실내 환경 모니터링 시스템")
    print("  구성: ESP32(WiFi) + SHT30 + OLED + 릴레이 x2")
    print("=" * 72)

    # 부품 목록 구성
    project_components = [
        Component(
            name="ESP32",
            voltage=3.3,
            current_ma=240.0,
            quantity=1,
            mode="Active WiFi",
            note="WiFi로 데이터 전송",
        ),
        Component(
            name="SHT30 온습도 센서",
            voltage=3.3,
            current_ma=0.6,
            quantity=1,
            note="온도/습도 측정",
        ),
        Component(
            name="OLED SSD1306 디스플레이",
            voltage=3.3,
            current_ma=20.0,
            quantity=1,
            note="현재 상태 표시",
        ),
        Component(
            name="릴레이 모듈",
            voltage=5.0,
            current_ma=70.0,
            quantity=2,
            note="환기팬 + 가습기 제어",
        ),
    ]

    # 보고서 출력
    print_power_report(
        project_components,
        project_name="실내 환경 모니터링 시스템",
        vin=5.0,
        vout=3.3,
    )

    # ----- 추가: 듀티 사이클 적용 시 배터리 수명 비교 -----
    print()
    print("[ 보너스: 듀티 사이클에 따른 배터리 수명 비교 (18650 3000mAh) ]")
    print_separator("-")
    print()

    total_active = calculate_total_current(project_components)
    bat_18650 = COMMON_BATTERIES[0]  # 18650

    # 딥 슬립 모드에서의 전류 (ESP32만 딥 슬립, 나머지는 꺼짐으로 가정)
    deep_sleep_current = 0.01  # ESP32 딥 슬립

    duty_cycles = [1.0, 0.5, 0.1, 0.01]
    labels = [
        "항상 활성 (100%)",
        "50% 활성 / 50% 딥슬립",
        "10% 활성 / 90% 딥슬립",
        "1% 활성 / 99% 딥슬립 (예: 1분 중 0.6초 활성)",
    ]

    print(f"  {'듀티 사이클':<48} {'평균 전류':>10} {'수명(시간)':>10} {'수명(일)':>9}")
    print(f"  {'-'*48} {'-'*10} {'-'*10} {'-'*9}")

    for dc, label in zip(duty_cycles, labels):
        # 가중 평균 전류 계산: 활성 시 전류 * 비율 + 딥 슬립 전류 * (1 - 비율)
        avg_current = total_active * dc + deep_sleep_current * (1 - dc)
        result = calculate_battery_life(avg_current, bat_18650, duty_cycle=1.0)
        print(
            f"  {label:<48} "
            f"{avg_current:>9.2f}  "
            f"{result['hours']:>9.1f}  "
            f"{result['days']:>8.1f}"
        )

    print()
    print("  * 듀티 사이클 최적화는 배터리 프로젝트의 핵심 설계 요소입니다.")
    print("  * 1% 듀티 사이클이면 약 60초 주기에서 0.6초만 활성 상태입니다.")
    print()


# =============================================================================
# 메인 진입점
# =============================================================================

def main() -> None:
    """메인 함수: 명령줄 인수에 따라 실행 모드를 결정합니다."""
    if len(sys.argv) > 1 and sys.argv[1] == "--example":
        # 예제 프로젝트 실행
        run_example()
    elif len(sys.argv) > 1 and sys.argv[1] in ("--help", "-h"):
        print(__doc__)
    else:
        # 대화형 모드
        print()
        print("사용 방법:")
        print("  --example  : 예제 프로젝트(환경 모니터링)의 전력 보고서 출력")
        print("  --help     : 도움말 표시")
        print("  (인수 없음) : 대화형 모드 실행")
        print()

        try:
            interactive_mode()
        except KeyboardInterrupt:
            print("\n\n  프로그램을 종료합니다.")
        except EOFError:
            # 비대화형 환경에서 실행 시 예제 모드로 전환
            print("\n  대화형 입력을 사용할 수 없습니다. 예제 모드로 실행합니다.\n")
            run_example()


if __name__ == "__main__":
    main()
