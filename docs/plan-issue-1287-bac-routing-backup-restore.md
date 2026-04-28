# Issue #1287 BAC_ROUTING Reinitialize + Backup/Restore 실행 계획

## 먼저 정정할 전제

`BAC_ROUTING`의 virtual device별 per-device 구조는 이미 있다.
`gw_device.c`는 `Devices[MAX_NUM_DEVICES]` 배열과 `iCurrent_Device_Idx`를 통해 virtual device별 기본 Device 정보를 관리한다.

이 계획에서 추가로 하려는 일은 **BAC_ROUTING 자체를 새로 구현하는 것**이 아니다.
목표는 아직 `device.c` static 전역 상태로 남아 있는 Reinitialize/Backup/Restore 관련 상태를 virtual device별로 분리하는 것이다.

```text
이미 구현됨  : virtual device 배열, object instance/name/description/database revision 등
이번 대상    : Reinitialize/Backup/Restore static 상태의 BAC_ROUTING 조건부 per-device화
후속 후보    : System_Status, Model_Name, Serial_Number 등
```

상세한 현재 상태 분석은 [analysis-device-static-variables.md](analysis-device-static-variables.md)에 둔다.

## 현재 상태 요약

### 이미 되어 있는 것

| 영역 | 상태 |
|---|---|
| virtual device 관리 | `Devices[MAX_NUM_DEVICES]` 기반으로 구현됨 |
| 현재 device 선택 | `iCurrent_Device_Idx`, `Set_Routed_Device_Object_Index()`로 구현됨 |
| 현재 device 포인터 조회 | `Get_Routed_Device_Object(-1)` 패턴 구현됨 |
| 기본 Device 식별 property 일부 | object instance/name/description/database revision은 per-device 처리됨 |
| 일부 object list | `Object_Lists[Routed_Device_Object_Index()]` 패턴으로 분리됨 |

### 아직 공유 static인 것

| 영역 | 현재 상태 | 문제 발생 조건 |
|---|---|---|
| Backup/Restore 상태 | `device.c` static | virtual device별 Backup/Restore를 열 때 문제 |
| Reinitialize 상태/password | `device.c` static | virtual device별 RD를 열 때 문제 |
| `System_Status` 등 Device 상태 property | `device.c` static | virtual device별 상태를 정확히 노출해야 할 때 문제 |
| `Model_Name`, `Serial_Number` 등 신원 property | `device.c` static | virtual device를 실제 개별 장치처럼 노출할 때 문제 |

### 현재 service gate

현재 `Routed_Device_Service_Approval()`은 virtual device에서 다음 서비스를 거부한다.

- `ReinitializeDevice`
- `DeviceCommunicationControl`

따라서 지금 당장은 Backup/Restore static 공유 문제가 크게 드러나지 않을 수 있다.
하지만 Issue #1287 목표처럼 virtual device별 Backup/Restore를 지원하려면, service gate를 열기 전에 Backup/Restore 상태를 먼저 per-device로 옮겨야 한다.

## 구현 원칙

1. 기존 BAC_ROUTING per-device 구조를 활용한다.
2. 새 parallel 구조를 만들지 말고 `DEVICE_OBJECT_DATA`를 확장한다.
3. **메모리 제약 때문에 새 필드는 반드시 `#if defined(BAC_ROUTING)` 안에 둔다.**
   - 소형 MICOM 빌드에서 `BAC_ROUTING`이 꺼져 있으면 `DEVICE_OBJECT_DATA` 크기가 늘어나면 안 된다.
   - `BAC_ROUTING` OFF 빌드는 기존 static 변수 기반 동작을 그대로 유지한다.
4. `BACNET_BACKUP_RESTORE`가 필요한 필드는 `BAC_ROUTING && BACNET_BACKUP_RESTORE` 조건으로 더 좁힌다.
5. `gw_device.c`에 과도한 `Routed_Device_*` getter/setter를 늘리지 않는다.
6. `device.c`에서는 `Get_Routed_Device_Object(-1)` 패턴으로 현재 device 상태에 접근한다.
7. virtual device에서는 RD 서비스 자체는 허용하되, 실제 처리는 Backup/Restore state로 제한한다.
   - `COLDSTART`/`WARMSTART`/`ACTIVATE_CHANGES`는 gateway/system-level 동작이므로 virtual device에서는 Simple ACK하지 않고 Error 응답한다.
   - Error는 기존 RD 구현 패턴과 맞춰 `ERROR_CLASS_SERVICES` / `ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED`를 사용한다.
8. service gate는 마지막에 연다. 상태 분리 전에 RD/DCC를 먼저 허용하지 않는다.
9. 테스트 계획은 별도 문서가 아니라 이 문서의 검증 섹션에 둔다.

## Phase 1. `DEVICE_OBJECT_DATA` 조건부 확장

파일: `src/bacnet/basic/object/device.h`

소형 MICOM 메모리 제약 때문에 새 per-device 필드는 `BAC_ROUTING` 빌드에서만 추가한다.
`BAC_ROUTING`이 꺼진 빌드는 기존 `DEVICE_OBJECT_DATA` 크기와 static 변수 기반 동작을 유지해야 한다.

추가할 상태:

- Reinitialize 상태/password: `BAC_ROUTING`일 때만 추가
- Backup/Restore 상태: `BAC_ROUTING && BACNET_BACKUP_RESTORE`일 때만 추가

```c
#if defined(BAC_ROUTING) && defined(BACNET_BACKUP_RESTORE)
typedef struct bacnet_backup_restore_data_s {
    BACNET_BACKUP_STATE Backup_State;
    uint16_t Backup_Failure_Timeout;
    uint32_t Backup_Failure_Timeout_Milliseconds;
    uint32_t Configuration_Files[BACNET_BACKUP_FILE_COUNT];
    BACNET_TIMESTAMP Last_Restore_Time;
    uint16_t Backup_Preparation_Time;
    uint16_t Restore_Preparation_Time;
    uint16_t Restore_Completion_Time;
} BACNET_BACKUP_RESTORE_DATA;
#endif

typedef struct devObj_s {
    BACNET_ADDRESS bacDevAddr;
    COMMON_BAC_OBJECT bacObj;
    char Description[MAX_DEV_DESC_LEN];
    uint32_t Database_Revision;

#if defined(BAC_ROUTING)
    BACNET_REINITIALIZED_STATE Reinitialize_State;
    const char *Reinit_Password;
#if defined(BACNET_BACKUP_RESTORE)
    BACNET_BACKUP_RESTORE_DATA Backup;
#endif
#endif
} DEVICE_OBJECT_DATA;
```

주의할 점:

- `Reinitialize_State`와 `Reinit_Password`는 routing 모드에서 RD 요청을 device별로 판단하기 위해 이번 범위에 포함한다.
- Backup/Restore 상태는 `BACNET_BACKUP_RESTORE`가 켜졌을 때만 포함한다.
- `System_Status`, `Model_Name`, `Serial_Number` 같은 일반 Device property는 후속 작업으로 분리해도 된다.
- 다만 DCC까지 virtual device별로 열 계획이면 `System_Status` 분리는 같은 작업 범위에 포함하는 것이 안전하다.

## Phase 2. Reinitialize/Backup 기본값 초기화

파일: `src/bacnet/basic/object/gateway/gw_device.c`

virtual device가 추가되거나 초기화될 때 `pDev->Reinitialize_*`와 `pDev->Backup` 기본값을 설정한다.

Reinitialize 기본값:

| 필드 | 기본값 | 조건 |
|---|---|---|
| `Reinitialize_State` | `BACNET_REINIT_IDLE` | `BAC_ROUTING` |
| `Reinit_Password` | 기존 기본값과 동일한 `"filister"` | `BAC_ROUTING` |

`Reinit_Password`는 기존 코드처럼 문자열을 복사하지 않는 `const char *`로 유지한다.
소형 MICOM 메모리 부담을 피하기 위해 device별 고정 길이 password buffer는 추가하지 않는다.

Backup 기본값:

| 필드 | 기본값 |
|---|---|
| `Backup_State` | `BACKUP_STATE_IDLE` |
| `Backup_Failure_Timeout` | `60 * 60` |
| `Backup_Failure_Timeout_Milliseconds` | `0` |
| `Configuration_Files[]` | `0` |
| `Last_Restore_Time` | timestamp 초기값 |
| preparation/completion time | `0` |

## Phase 3. `device.c`의 Reinitialize/Backup 접근을 wrapper 기반으로 변경

파일: `src/bacnet/basic/object/device.c`, `src/bacnet/basic/object/device.h`

현재 Reinitialize와 Backup/Restore 함수들은 static 변수를 직접 읽거나 쓴다. 이를 현재 device 기준 접근으로 바꾼다.

Reinitialize 대상:

- `Device_Reinitialized_State()`
- `Device_Reinitialize_State_Set()`
- `Device_Reinitialize_Password_Set()`
- `Device_Reinitialize()`의 password 검사와 state 저장

Backup 대상:

- `Device_Backup_And_Restore_State()`
- `Device_Backup_And_Restore_State_Set()`
- `Device_Backup_Failure_Timeout()`
- `Device_Backup_Failure_Timeout_Set()`
- `Device_Backup_Failure_Timeout_Restart()`
- `Device_Backup_Failure_Timeout_Countdown()`
- `Device_Reinitialize()` 내부 Backup 상태 전이
- `Device_Start_Backup()`, `Device_End_Restore()` 등 Backup/Restore 상태 변경 경로

routing 모드의 접근 규칙:

```text
현재 device index
  -> Get_Routed_Device_Object(-1)
  -> pDev->Reinitialize_State / pDev->Reinit_Password
  -> pDev->Backup.<field>  // BACNET_BACKUP_RESTORE일 때만
```

non-routing 모드에서는 기존 static 변수를 유지한다. 이 경로에서 `DEVICE_OBJECT_DATA`에 추가된 routing 전용 필드를 참조하면 안 된다.

## Phase 4. routed read/write에서 Backup property 처리

파일: `src/bacnet/basic/object/gateway/gw_device.c`

현재 routed read/write는 일부 property만 `Devices[]`에서 처리하고, 나머지는 `Device_Read_Property_Local()` / `Device_Write_Property_Local()`로 넘긴다.
Backup property는 fallthrough 전에 `pDev->Backup`에서 처리해야 한다.

Read 대상:

- `PROP_BACKUP_AND_RESTORE_STATE`
- `PROP_BACKUP_FAILURE_TIMEOUT`
- `PROP_BACKUP_PREPARATION_TIME`
- `PROP_RESTORE_PREPARATION_TIME`
- `PROP_RESTORE_COMPLETION_TIME`
- `PROP_CONFIGURATION_FILES`

Write 대상:

- `PROP_BACKUP_FAILURE_TIMEOUT`
- `PROP_BACKUP_PREPARATION_TIME`
- `PROP_RESTORE_PREPARATION_TIME`
- `PROP_CONFIGURATION_FILES`

`PROP_CONFIGURATION_FILES`는 array encode/write helper가 필요하다.

## Phase 5. `Device_Timer` 조정

파일: `src/bacnet/basic/object/device.c`

현재는 Backup countdown이 routing loop 밖에서 한 번 호출된다.
Backup 상태가 static인 현재 구조에서는 이게 맞다.

Backup 상태를 per-device로 옮긴 뒤에는 다음 순서로 바꾼다.

1. countdown 함수가 현재 device의 `pDev->Backup`을 사용하도록 변경
2. `Set_Routed_Device_Object_Index(dev_id)`로 device를 바꾸는 loop 안에서 countdown 호출
3. managed device 수가 N개일 때 timeout이 N배 빠르게 줄지 않는지 테스트

## Phase 6. service gate 변경

파일: `src/bacnet/basic/object/gateway/gw_device.c`

Reinitialize/Backup 상태 분리 후 `Routed_Device_Service_Approval()`에서 virtual device의 RD 서비스를 허용한다.
여기서는 “서비스 자체를 받을 수 있는가”만 판단하고, RD state별 지원 여부는 `Device_Reinitialize()`에서 처리한다.

| 서비스 | 처리 |
|---|---|
| `SERVICE_SUPPORTED_REINITIALIZE_DEVICE` | gateway와 virtual device 모두 허용 |
| `SERVICE_SUPPORTED_DEVICE_COMMUNICATION_CONTROL` | 이번 범위에서는 기존처럼 virtual device 거부 유지 |

RD state별 정책:

| RD state | Gateway device | Virtual device |
|---|---|---|
| `COLDSTART` | 기존 side effect 수행 | Error: `OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED` |
| `WARMSTART` | 기존 side effect 수행 | Error: `OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED` |
| `ACTIVATE_CHANGES` | 기존 side effect 수행 | Error: `OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED` |
| Backup/Restore 계열 | 기존 처리 | 해당 virtual device의 `Backup` 상태 사용 |

`COLDSTART`, `WARMSTART`, `ACTIVATE_CHANGES`는 virtual device에서 실제 동작을 수행하지 않으므로 Simple ACK하면 client가 성공으로 오해할 수 있다.
따라서 `Routed_Device_Service_Approval()`에서 `Reject`로 막기보다, `Device_Reinitialize()`에서 아래 Error를 설정하고 false를 반환한다.

```c
rd_data->error_class = ERROR_CLASS_SERVICES;
rd_data->error_code = ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
return false;
```

DCC까지 virtual device별로 허용하려면 `System_Status` per-device화가 필요하므로 후속 작업으로 둔다.

## Phase 7. 검증

별도 테스트 계획 문서는 만들지 않는다. 필요한 검증은 이 섹션에 유지한다.

### 빌드 검증

```bash
cmake -S test -B /tmp/bacnet-test-root -DBAC_ROUTING=ON
cmake --build /tmp/bacnet-test-root --target test_device
/tmp/bacnet-test-root/bacnet/basic/object/device/test_device
```

Backup/Restore 테스트는 `BACNET_BACKUP_RESTORE` 정의가 필요하다. 현재 `test/` CMake가 이 옵션을 직접 노출하지 않으면, 해당 테스트 target의 compile definition에 `BACNET_BACKUP_RESTORE`를 명시적으로 추가한다.

non-routing 회귀:

```bash
cmake -S test -B /tmp/bacnet-test-root-no-routing
cmake --build /tmp/bacnet-test-root-no-routing --target test_device
/tmp/bacnet-test-root-no-routing/bacnet/basic/object/device/test_device
```

### 추가할 테스트 시나리오

- Backup property read 6종
- Backup property write 4종
- `STARTBACKUP` / `STARTRESTORE` / `ABORTRESTORE` 상태 전이
- virtual device별 Backup 상태 독립성
- timeout countdown이 managed device 수만큼 빠르게 감소하지 않는지
- virtual device RD 서비스 허용 결과
- virtual device `COLDSTART` / `WARMSTART` / `ACTIVATE_CHANGES`가 Simple ACK하지 않고 `OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED` Error 응답하는지
- 실제 구현 때 virtual device unsupported RD state 테스트를 반드시 추가
  - 대상: `COLDSTART`, `WARMSTART`, `ACTIVATE_CHANGES`
  - 기대값: `Device_Reinitialize()` false, `ERROR_CLASS_SERVICES`, `ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED`
- virtual device `COLDSTART` / `WARMSTART` / `ACTIVATE_CHANGES`가 gateway/system-level side effect를 실행하지 않는지
- DCC는 이번 범위에서 여전히 virtual device 거부인지

## 완료 체크리스트

### 현재 구조 이해

- [ ] PR 설명에 “BAC_ROUTING per-device 구조는 이미 있음”을 명시
- [ ] 이번 변경 범위가 “Reinitialize/Backup/Restore static 상태의 BAC_ROUTING 조건부 per-device화”임을 명시

### 데이터 구조

- [ ] 새 필드가 `#if defined(BAC_ROUTING)` 안에만 추가됨
- [ ] `BAC_ROUTING` OFF 빌드에서 `DEVICE_OBJECT_DATA` 크기/동작이 기존과 동일함
- [ ] `Reinitialize_State` / `Reinit_Password`가 `BAC_ROUTING` 조건부 필드로 추가됨
- [ ] `BACNET_BACKUP_RESTORE_DATA`가 `BAC_ROUTING && BACNET_BACKUP_RESTORE` 조건으로 정의됨
- [ ] `DEVICE_OBJECT_DATA.Backup`이 `BAC_ROUTING && BACNET_BACKUP_RESTORE` 조건으로 추가됨
- [ ] Reinitialize/Backup 기본값 초기화

### 접근 경로

- [ ] Reinitialize getter/setter/password 경로가 routing 모드에서 `Get_Routed_Device_Object(-1)` 사용
- [ ] Backup getter/setter가 routing 모드에서 `Get_Routed_Device_Object(-1)` 사용
- [ ] service handler가 직접 static 대신 wrapper 사용
- [ ] timer/countdown 경로가 per-device 상태 사용

### routed property

- [ ] routed read에 Backup property case 추가
- [ ] routed write에 Backup property case 추가
- [ ] `Configuration_Files` array helper 추가

### 정책

- [ ] virtual device에서 RD 서비스 허용
- [ ] virtual device의 Backup/Restore RD state는 per-device Backup 상태로 처리
- [ ] virtual device의 `COLDSTART` / `WARMSTART` / `ACTIVATE_CHANGES`는 `ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED` Error 응답
- [ ] `Reinit_Password`는 `BAC_ROUTING`에서 per-device로 적용
- [ ] DCC service gate는 이번 범위에서 기존처럼 유지

### 검증

- [ ] 기존 `test_device` 통과
- [ ] `BAC_ROUTING=ON` 조합 통과
- [ ] `BAC_ROUTING=OFF` 조합에서 기존 동작 통과
- [ ] Backup read/write 테스트 통과
- [ ] Reinitialize 상태 전이 테스트 통과
- [ ] virtual device RD 서비스 허용 테스트 통과
- [ ] virtual device unsupported RD state Error 응답 테스트 추가 및 통과
  - [ ] `COLDSTART`
  - [ ] `WARMSTART`
  - [ ] `ACTIVATE_CHANGES`
- [ ] virtual device system-level side effect 차단 테스트 통과
- [ ] timeout countdown 회귀 테스트 통과
