# BAC_ROUTING Device 상태 분석

## 한 줄 결론

`BAC_ROUTING`의 **virtual device별 per-device 구조는 이미 구현되어 있다.**
다만 모든 Device Object property가 per-device인 것은 아니며, 현재는 일부 property만 `Devices[]`에 저장되고 나머지 여러 상태는 `device.c`의 static 전역 값을 공유한다.

따라서 이 문서에서 말하는 “추가 per-device 분리”는 **BAC_ROUTING을 새로 만든다는 뜻이 아니라**, 다음 항목들을 기존 per-device 구조 위에 virtual device별 상태로 더 확장해야 한다는 뜻이다. 소형 MICOM 메모리 제약 때문에 이런 추가 필드는 `BAC_ROUTING` 빌드에서만 포함되어야 한다.

- Backup/Restore 상태
- Reinitialize 관련 상태/password
- `System_Status`, `Model_Name`, `Serial_Number` 같은 Device Object static property

## 현재 이미 per-device로 구현된 것

`gw_device.c`는 managed device를 `Devices[MAX_NUM_DEVICES]` 배열로 관리한다.

```c
DEVICE_OBJECT_DATA Devices[MAX_NUM_DEVICES];
uint16_t iCurrent_Device_Idx = 0;
```

현재 device 선택은 `iCurrent_Device_Idx`로 이뤄지고, `Get_Routed_Device_Object(-1)`는 현재 선택된 `Devices[iCurrent_Device_Idx]`를 반환한다.

현재 `DEVICE_OBJECT_DATA`에는 아래 필드가 있다.

```c
typedef struct devObj_s {
    BACNET_ADDRESS bacDevAddr;
    COMMON_BAC_OBJECT bacObj;
    char Description[MAX_DEV_DESC_LEN];
    uint32_t Database_Revision;
} DEVICE_OBJECT_DATA;
```

그래서 아래 항목들은 이미 virtual device별로 분리되어 있다.

| 항목 | 저장 위치 | 상태 |
|---|---|---|
| Device address | `Devices[i].bacDevAddr` | per-device 구현됨 |
| Object instance | `Devices[i].bacObj.Object_Instance_Number` | per-device 구현됨 |
| Object name | `Devices[i].bacObj.Object_Name` | per-device 구현됨 |
| Description | `Devices[i].Description` | per-device 구현됨 |
| Database revision | `Devices[i].Database_Revision` | per-device 구현됨 |
| 일부 object list | 각 object의 `Object_Lists[Routed_Device_Object_Index()]` 패턴 | per-device 구현됨 |

## 현재 아직 공유되는 것

`Routed_Device_Read_Property_Local()`과 `Routed_Device_Write_Property_Local()`은 일부 Device property만 `Devices[]`에서 처리하고, 나머지는 기존 `Device_*_Property_Local()`로 넘긴다.

| 경로 | `Devices[]`에서 직접 처리 | 그 외 property |
|---|---|---|
| `Routed_Device_Read_Property_Local()` | `Object_Identifier`, `Object_Name`, `Description`, `Database_Revision` | `Device_Read_Property_Local()`로 fallthrough |
| `Routed_Device_Write_Property_Local()` | `Object_Identifier`, `Object_Name` | `Device_Write_Property_Local()`로 fallthrough |

이 fallthrough 경로에서 읽거나 쓰는 값 중 상당수는 `device.c`의 static 전역 상태다.

대표 예시는 다음과 같다.

| 상태 | 현재 저장 방식 | 의미 |
|---|---|---|
| `System_Status` | `device.c` static | 모든 virtual device가 같은 system status를 보게 될 수 있음 |
| `Vendor_Name`, `Vendor_Identifier` | `device.c` static | 모든 virtual device가 같은 vendor 정보를 보게 됨 |
| `Model_Name` | `device.c` static | 모든 virtual device가 같은 model name을 보게 됨 |
| `Application_Software_Version` | `device.c` static | 모든 virtual device가 같은 app version을 보게 됨 |
| `Firmware_Revision` | `device.c` static | 모든 virtual device가 같은 firmware revision을 보게 됨 |
| `Location` | `device.c` static | 모든 virtual device가 같은 location을 보게 됨 |
| `Serial_Number` | `device.c` static | 모든 virtual device가 같은 serial number를 보게 됨 |
| `Reinitialize_State` | `device.c` static | RD 상태가 device별로 독립되지 않음 |
| `Reinit_Password` | `device.c` static | password가 device별로 독립되지 않음 |
| Backup/Restore 상태 | `device.c` static | Backup/Restore 상태가 device별로 독립되지 않음 |

즉, 현재 구조는 이렇게 이해하면 된다.

```text
BAC_ROUTING virtual device 구조       : 있음
Devices[] 기반 기본 Device 정보       : 일부 구현됨
Device Object static property 분리    : 일부 미구현
Backup/Restore per-device 상태        : 미구현
```

## 왜 당장 문제가 크게 보이지 않을 수 있나

현재 `Routed_Device_Service_Approval()`은 virtual device(`iCurrent_Device_Idx > 0`)에서 다음 서비스를 거부한다.

- `SERVICE_SUPPORTED_REINITIALIZE_DEVICE`
- `SERVICE_SUPPORTED_DEVICE_COMMUNICATION_CONTROL`

그래서 현재 정책에서는 virtual device가 직접 RD/DCC/Backup 흐름을 타지 못한다. 이 때문에 Backup/Restore static 상태 공유 문제가 기능적으로 크게 드러나지 않을 수 있다.

하지만 virtual device별 RD/DCC/Backup을 열면 이야기가 달라진다. 그때는 공유 static 상태가 곧바로 다음 문제로 이어질 수 있다.

- 한 virtual device의 Backup 상태가 다른 virtual device에도 보임
- 한 virtual device의 Reinitialize 상태가 전체에 공유됨
- 모든 virtual device가 같은 `Model_Name`, `Serial_Number`, `Location`을 반환함
- timeout countdown 같은 runtime 상태가 장치별로 독립되지 않음

## 정책 결정 결과

이번 계획에서는 정책을 다음처럼 둔다.

| 항목 | 결정 |
|---|---|
| `ReinitializeDevice` 범위 | virtual device에서도 RD 서비스는 허용하되, 실제 처리는 Backup/Restore state로 제한 |
| Password 범위 | `BAC_ROUTING`에서는 virtual device별 password 적용. 기본값은 기존과 동일한 `"filister"` 유지 |
| DCC 범위 | 이번 범위에서는 기존처럼 virtual device 거부 유지 |
| Gateway/system-level side effect | virtual device RD에서는 수행하지 않음. `COLDSTART`/`WARMSTART`/`ACTIVATE_CHANGES`는 Simple ACK하지 않고 unsupported error 응답 |

따라서 service gate를 열기 전에 `Reinitialize_State`, `Reinit_Password`, Backup/Restore 상태를 먼저 per-device로 옮겨야 한다.
DCC까지 virtual device별로 허용하려면 `System_Status` 같은 상태 property 분리가 추가로 필요하므로 후속 작업으로 둔다.

`COLDSTART`, `WARMSTART`, `ACTIVATE_CHANGES`는 유효한 RD state이지만 virtual device에서 실제 gateway/system-level 동작을 수행하지 않는다.
따라서 `Routed_Device_Service_Approval()`에서 `Reject`로 막기보다는, `Device_Reinitialize()`에서 아래 Error를 설정하고 false를 반환하는 방식이 자연스럽다.

```text
ERROR_CLASS_SERVICES
ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED
```

이 선택은 기존 RD 구현에서 Backup/Restore 기능이 빠진 경우에 같은 error code를 사용하고 있는 패턴과도 맞다.

## 추가 분리가 필요한 후보

### 1. Backup/Restore 관련 상태

Issue #1287의 직접 대상이다. virtual device별 Backup/Restore를 지원하려면 아래 상태를 `DEVICE_OBJECT_DATA` 내부의 중첩 구조체로 이동하는 것이 자연스럽다.

| 변수 | 이유 |
|---|---|
| `Backup_State` | 장치별 Backup/Restore 상태 독립성 필요 |
| `Backup_Failure_Timeout` | 장치별 timeout 설정값 필요 |
| `Backup_Failure_Timeout_Milliseconds` | 장치별 countdown runtime 상태 필요 |
| `Configuration_Files[]` | 장치별 파일 목록 필요 |
| `Last_Restore_Time` | 장치별 마지막 복원 시간 필요 |
| `Backup_Preparation_Time` | 장치별 backup 준비 시간 필요 |
| `Restore_Preparation_Time` | 장치별 restore 준비 시간 필요 |
| `Restore_Completion_Time` | 장치별 restore 완료 시간 필요 |

권장 구조:

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
```

### 2. Reinitialize 관련 상태

| 변수 | 조건 | 이유 |
|---|---|---|
| `Reinitialize_State` | virtual device별 RD 허용 시 필요 | 장치별 reinitialize 상태가 독립이어야 함 |
| `Reinit_Password` | virtual device별 RD 허용 시 필요 | 장치별 password 정책이 자연스러움. 저장 방식은 기존과 같은 `const char *` 유지 |

이 두 필드는 소형 MICOM의 non-routing 빌드 메모리를 늘리지 않도록 `#if defined(BAC_ROUTING)` 안에만 추가한다.

### 3. Device Object 신원/상태 property

아래 항목은 BACnet client가 각 virtual device를 별도 장치로 인식할 때 중요하다.

| 변수 / Property | 권장 분류 | 이유 |
|---|---|---|
| `System_Status` / `PROP_SYSTEM_STATUS` | per-device 필수 | Backup/Restore, DCC 상태가 장치별로 달라질 수 있음 |
| `Model_Name` / `PROP_MODEL_NAME` | per-device 필요 | Gateway 뒤 장비 모델이 다를 수 있음 |
| `Application_Software_Version` / `PROP_APPLICATION_SOFTWARE_VERSION` | per-device 필요 | 장치별 SW 버전이 다를 수 있음 |
| `Firmware_Revision` / `PROP_FIRMWARE_REVISION` | per-device 권장 | 장치별 FW 버전이 다를 수 있음 |
| `Location` / `PROP_LOCATION` | per-device 필요 | 물리적 설치 위치가 다름 |
| `Serial_Number` / `PROP_SERIAL_NUMBER` | per-device 필수 | BACnet 식별 조건에 사용됨 |
| `Vendor_Name`, `Vendor_Identifier` | 정책 의존 | Gateway 뒤 장비를 어떤 vendor로 노출할지에 따라 결정 |
| `Device_UUID` | per-device 권장 | virtual device별 고유 식별자가 필요할 수 있음 |

`Object_Table`은 현재처럼 공용 유지가 자연스럽다. Gateway가 지원 object 목록을 통합 제공하는 구조이기 때문이다.

## Device_Timer 주의사항

현재 `BAC_ROUTING` 코드에서는 `Device_Backup_Failure_Timeout_Countdown(milliseconds)`가 per-device object loop 밖에서 한 번 호출된다.
현재처럼 Backup/Restore 상태가 Gateway static 상태라면 이 구조가 오히려 안전하다. loop 안에서 static countdown을 호출하면 managed device 수만큼 timeout이 빠르게 감소할 수 있기 때문이다.

Backup 상태를 `DEVICE_OBJECT_DATA.Backup`으로 옮긴 뒤에는 순서가 바뀐다.

1. `Device_Backup_Failure_Timeout_Countdown()`이 현재 device의 Backup 상태를 사용하도록 바꾼다.
2. getter/setter는 `Get_Routed_Device_Object(-1)`로 현재 `iCurrent_Device_Idx`의 `DEVICE_OBJECT_DATA`를 얻는다.
3. 그 다음에 countdown 호출을 per-device loop 안으로 옮긴다.

## 구현 전 체크리스트

- [ ] 현재 per-device 구현 범위와 static 공유 범위를 구분해서 PR 설명에 명시
- [ ] virtual device에서 RD 서비스 허용
- [ ] virtual device의 Backup/Restore RD state는 per-device Backup 상태로 처리
- [ ] virtual device의 `COLDSTART` / `WARMSTART` / `ACTIVATE_CHANGES`는 `ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED` Error 응답
- [ ] `Reinitialize_State` / `Reinit_Password`는 `BAC_ROUTING` 조건부 필드로 추가
- [ ] Backup/Restore 상태를 `BAC_ROUTING && BACNET_BACKUP_RESTORE` 조건부 중첩 구조체로 추가
- [ ] routed read/write에서 Backup property를 fallthrough 전에 처리
- [ ] service gate 변경은 관련 상태 이동 이후 적용
- [ ] `BAC_ROUTING` OFF 빌드에서는 기존 static 변수 기반 동작 유지
- [ ] timeout countdown 회귀 시나리오 검증
