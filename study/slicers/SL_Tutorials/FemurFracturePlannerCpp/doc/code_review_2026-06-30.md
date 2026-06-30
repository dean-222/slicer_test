# FemurFracturePlannerCpp 코드 리뷰 정리

Date: 2026-06-30

## 리뷰 범위

- `FemurFracturePlannerCpp/Logic`
- `qSlicerFemurFracturePlannerCppModulePlannerDialog.*`
- `qSlicerFemurFracturePlannerCppModuleWidget.*`
- `CMakeLists.txt`
- `Testing` 구성

이번 리뷰는 정적 코드 검토 기준으로 수행했다. Slicer 빌드 및 실제 모듈 실행 검증은 수행하지 않았다.

## 주요 발견 사항

### 1. 공개 헤더의 표준 라이브러리 include 누락

대상 파일:

- `Logic/vtkSlicerFemurFracturePlannerCppLogic.h`

내용:

- `RunIcpRegistration()` 선언에서 `std::vector`를 사용한다.
- `RunMaskedIcpRegistration()` 선언에서 `std::string`을 사용한다.
- 하지만 헤더에 `<vector>`와 `<string>`이 직접 포함되어 있지 않다.

영향:

- 현재는 다른 파일의 include 순서 때문에 우연히 컴파일될 수 있다.
- 다른 translation unit 또는 빌드 환경에서는 헤더 단독 include 시 컴파일 오류가 발생할 수 있다.

권장 수정:

```cpp
#include <string>
#include <vector>
```

를 `vtkSlicerFemurFracturePlannerCppLogic.h`에 명시적으로 추가한다.

### 2. Logic 구현 파일의 표준 헤더 include 누락

대상 파일:

- `Logic/vtkSlicerFemurFracturePlannerCppLogic.cxx`

내용:

- `std::sort`, `std::min`, `std::abs`, `std::sqrt`, `strcmp`, `std::to_string` 등을 사용한다.
- 현재 파일에는 `<string>`, `<vector>`만 명시되어 있다.

영향:

- 컴파일러나 플랫폼에 따라 transitive include에 의존하게 된다.
- MSVC/Clang/GCC 또는 Slicer 버전 차이에 따라 컴파일 실패 가능성이 있다.

권장 수정:

```cpp
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
```

처럼 사용 중인 표준 API에 맞는 헤더를 직접 포함한다.

### 3. PlannerDialog 구현 파일의 직접 include 누락

대상 파일:

- `qSlicerFemurFracturePlannerCppModulePlannerDialog.cxx`

내용:

- `QHeaderView`, `QVariantMap`, `std::find`, `std::vector`, `std::string`을 사용한다.
- `vtkMRMLSegmentationNode`, `vtkSegmentation`, `vtkSegment` 타입과 멤버 함수도 직접 사용한다.
- 하지만 해당 헤더들이 구현 파일에 명시적으로 포함되어 있지 않다.

영향:

- 현재는 UI 생성 헤더 또는 다른 Slicer 헤더의 간접 include에 의존한다.
- include 순서 변경, Qt/Slicer 버전 변경, 빌드 설정 변경 시 컴파일 오류가 발생할 수 있다.

권장 수정:

```cpp
#include <QHeaderView>
#include <QVariantMap>

#include <algorithm>
#include <string>
#include <vector>

#include <vtkMRMLSegmentationNode.h>
#include <vtkSegmentation.h>
#include <vtkSegment.h>
```

를 필요한 위치에 추가한다.

### 4. 마커 노드 이름 불일치

대상 파일:

- `qSlicerFemurFracturePlannerCppModulePlannerDialog.cxx`

확인 위치:

- `onRunLandmarkMatchClicked()`
- `onClearMarkersClicked()`

내용:

- 랜드마크 정합 실행 시 다음 노드를 조회한다.

```text
Targeting_Source_Fiducials
Targeting_Target_Fiducials
```

- 마커 초기화 시에는 다음 노드를 조회한다.

```text
Targeting_Source_Markers
Targeting_Target_Markers
```

영향:

- 사용자가 마커를 배치하거나 초기화해도, 정합 실행 슬롯과 초기화 슬롯이 서로 다른 노드를 대상으로 동작할 수 있다.
- 결과적으로 마커 정합 버튼이 항상 "마커 정보 노드가 존재하지 않음" 경고를 띄우거나, 초기화 버튼이 실제 정합에 쓰이는 마커를 지우지 못할 수 있다.

권장 수정:

- 마커 노드 이름을 하나의 규칙으로 통일한다.
- 예: 모든 코드에서 `Targeting_Source_Fiducials`, `Targeting_Target_Fiducials`를 사용한다.
- 노드 생성, 조회, 초기화, 안내 메시지의 이름을 함께 맞춘다.
- 가능하면 문자열 상수로 분리해 오타와 불일치를 방지한다.

### 5. 테스트가 사실상 비활성화되어 있음

대상 파일:

- `Testing/Cxx/CMakeLists.txt`
- `CMakeLists.txt`

내용:

- `Testing/Cxx/CMakeLists.txt`의 테스트 소스가 주석 처리되어 있다.
- 루트 `CMakeLists.txt`에서도 `WITH_GENERIC_TESTS`가 주석 처리되어 있다.

영향:

- include 누락, 슬롯 연결 문제, MRML 노드 이름 불일치 같은 회귀를 자동으로 잡기 어렵다.
- 모듈이 커질수록 수동 실행 전까지 문제가 발견되지 않을 가능성이 높다.

권장 수정:

- 최소한 모듈 로딩 또는 Logic 객체 생성 수준의 Cxx 테스트를 활성화한다.
- 이후 MRML scene 생성, segmentation/model node 생성, 주요 Logic 함수 호출 여부를 확인하는 작은 테스트를 추가한다.

## 권장 수정 순서

1. `vtkSlicerFemurFracturePlannerCppLogic.h`에 `<vector>`, `<string>` 추가
2. `vtkSlicerFemurFracturePlannerCppLogic.cxx`에 `<algorithm>`, `<cmath>`, `<cstring>` 추가
3. `qSlicerFemurFracturePlannerCppModulePlannerDialog.cxx`에 Qt/STL/VTK 직접 include 추가
4. 마커 노드 이름을 `Fiducials` 또는 `Markers` 중 하나로 통일
5. 최소 Cxx 테스트 또는 generic module test 활성화
6. Slicer 빌드 후 모듈 로딩, 볼륨 선택, 골편 분리, 마커 정합 버튼을 수동 검증

## 검증 메모

실제 검증 완료:

- `rg`와 `Get-Content`로 관련 파일의 사용 위치와 include 상태를 확인했다.

정적 검토 완료:

- C++ 헤더 self-contained 여부
- 구현 파일의 transitive include 의존 여부
- 마커 정합/초기화 노드 이름 일관성
- 테스트 구성 활성화 여부

직접 검증하지 못한 항목:

- Slicer Extension 전체 빌드
- 3D Slicer 내 모듈 로딩
- UI 버튼 클릭 동작
- MRML scene 기반 실제 골편 분리 및 정합 결과

## 남은 위험

- 기존 문서와 일부 소스 주석은 환경에 따라 한글 인코딩이 깨져 보일 수 있다.
- 실제 빌드 로그가 없으므로, include 추가 후에도 Slicer API 버전 차이로 인한 추가 컴파일 오류가 남아 있을 수 있다.
- 마커 노드 생성 흐름은 코드상 명확히 확인되지 않았으므로, 이름 통일과 함께 실제 생성 위치를 추가로 점검해야 한다.
