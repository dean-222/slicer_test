# FemurFracturePlannerCpp 완전 이식 보완 패치

- 패치 기준일: 2026-06-30
- 패치 기준: Scripted Module의 기능을 C++ Loadable Module로 완전 이식
- UI/Workflow를 Python에 남기는 혼합 구조는 이 패치의 목표가 아님
- 주요 수정 버전: PlannerDialog `v0-6.0.0`, Logic `v0-4.0.0`

## 이번 패치에서 실제 반영한 내용

### 1. 컴파일 안정성

- `PlannerDialog.h`의 include guard 불일치를 수정했다.
- Logic 공개 헤더에 `<string>`, `<vector>`를 직접 포함했다.
- Qt, STL, VTK/MRML 타입의 직접 include를 추가해 간접 include 의존을 줄였다.
- Markups MRML 라이브러리를 CMake 링크 대상과 모듈 런타임 의존성에 추가했다.
- `Segmentations`, `Volumes`, `VolumeRendering`, `Markups` 모듈 의존성을 명시했다.

### 2. 영상 처리 기능 분리

- 기존에는 일반 에지 버튼도 마스킹 에지 함수를 호출했다.
- `일반 에지 검출`과 `마스킹 에지 검출` 버튼 및 슬롯을 분리했다.
- 마스킹 에지는 signed short로 변환한 뒤 threshold와 gradient를 적용한다.
- 출력 영상은 VTK pipeline 임시 출력에 의존하지 않도록 `DeepCopy`한다.
- 밝기 반전은 특정 `VTK_SHORT` 포인터 형식에 고정하지 않고 `vtkDataArray`로 처리한다.

### 3. 골편 식별 방식 개선

- 골편 생성 시 다음 MRML 속성을 기록한다.
  - `FemurFracturePlannerCpp.Role=Fragment`
  - `FemurFracturePlannerCpp.FragmentIndex=<번호>`
- 골편 표, ICP, Surface Snap, 전체 삭제 기능은 속성을 우선 사용한다.
- 이전 Scene 호환성을 위해 `Femur_Fragment*` 이름 접두어도 fallback으로 유지한다.
- 사용자가 골편 이름을 변경해도 기능 연결이 유지된다.

### 4. Transform 계층과 World delta

- ICP, Surface Snap, Landmark, Mask ICP에 반복되던 World delta 적용 코드를 공통 헬퍼로 통합했다.
- 부모가 있는 선형 Transform 계층에서는 다음 순서로 변환한다.
  1. 현재 World 행렬 계산
  2. `deltaWorld × currentWorld`
  3. 부모 World 역행렬로 Local 행렬 환산
  4. Local Transform에 적용
- 기즈모용 Transform을 만들 때 기존 부모 Transform을 잃지 않도록 계층 아래에 삽입한다.
- 비선형 Transform 계층에서 행렬 누적을 강제로 수행하지 않고 실패하도록 방어한다.

### 5. Landmark 워크플로우 복구

- 실행과 초기화에서 서로 달랐던 마커 이름을 다음으로 통일했다.
  - `Targeting_Source_Markers`
  - `Targeting_Target_Markers`
- 표에서 선택한 골편을 이동 Source로 사용한다.
- 가이드 선택기의 모델을 Target으로 사용한다.
- 마커 노드가 없거나 비어 있으면 자동 생성하고 Place mode를 켠다.
- Source 노드에 6개 이상의 짝수 점을 연속 배치한 경우 앞/뒤 절반으로 자동 분리한다.
- 다음 입력 검증을 추가했다.
  - Source와 Target이 서로 다른 노드인지
  - 각 마커가 최소 3점인지
  - 점 개수가 동일한지
  - 대응점이 거의 일직선이 아닌지
  - 결과 행렬 값이 유한한지

### 6. Mask ICP 워크플로우

- 표의 선택 셀에 의존하지 않고 현재 행의 골편 ID를 사용한다.
- Source와 Target이 같은 노드인 경우 실행을 막는다.
- ROI 세그먼트가 없으면 자동 생성한다.
- 마스크와 추출 표면의 최소 점 수를 확인한다.
- 임시 VTK 출력은 `DeepCopy`해 소유권을 명확히 한다.

### 7. MRML 상태 저장과 Scene 생명주기

- 숨김 `vtkMRMLScriptedModuleNode`를 만들어 다음 값을 Scene에 저장한다.
  - Input Volume
  - Segmentation
  - Guide Model
  - Threshold
  - 회전 각도/축
  - 볼륨/렌더링/가이드 표시 상태
- `StartClose`, `EndClose`, `NodeAdded`, `NodeRemoved`, `EndBatchProcess`를 관찰한다.
- Scene close 시 Segment Editor, 선택 모델, Transform observer 참조를 해제한다.
- Scene을 다시 열었을 때 저장된 selector와 UI 상태를 복원한다.

### 8. 정합 입력과 파라미터

- Combined ICP는 가이드 모델이 골편 목록에 포함되지 않도록 제외한다.
- Scripted 기준에 맞춰 ICP 최대 반복 수와 landmark 상한을 각각 300, 1000으로 맞췄다.
- 골절면 스냅은 전체 triangle mesh에서 normal을 계산한 뒤 샘플링한다.
- Surface Snap에서 선택 골편을 이동 대상으로 사용할 수 있게 했다.

### 9. 최소 C++ 회귀 테스트

- Logic과 MRML Scene 연결 테스트를 추가했다.
- 빈 입력과 null 입력이 주요 정합 API에서 성공으로 처리되지 않는지 확인한다.
- 테스트는 Slicer 빌드 트리에서 `BUILD_TESTING=ON`으로 실행해야 한다.

## 정적 검증 결과

다음 검사를 수행했다.

- UI XML 파싱
- PlannerDialog 헤더 선언과 구현 함수 대응
- 중복 함수 구현 검사
- C++ 중괄호 및 함수 중첩 구조 검사
- 이전 `Targeting_*_Fiducials` 문자열 잔존 여부 검사
- 고정 `Femur_Fragment_1`, `Femur_Fragment_2` 의존 제거 확인
- CMake의 Markups 링크 및 런타임 dependency 확인

## 실제 Slicer 빌드 전 반드시 확인할 사항

이 환경에는 Slicer 5.10 C++ 빌드 트리가 없으므로 실제 컴파일과 실행은 수행하지 못했다. 다음 검증은 Windows의 기존 Slicer 빌드 환경에서 해야 한다.

1. Slicer 5.10 Release 설정과 동일한 MSVC/Qt 빌드로 CMake configure
2. `FemurFracturePlannerCpp` target 전체 빌드
3. `BUILD_TESTING=ON` 상태에서 Logic test 실행
4. 모듈 로드 후 Scene Clear 및 MRML Scene 재로드
5. 일반 에지와 마스킹 에지 버튼 각각 실행
6. 골편 이름을 바꾼 뒤 표/ICP/삭제 기능 확인
7. 부모 Transform이 2단계인 골편에서 ICP 결과 확인
8. 마커 6개 자동 분할 및 Source/Target 개별 마커 방식 확인
9. 열린 마스크와 닫힌 마스크 각각 실패/성공 조건 확인

## 아직 남은 구조적 보완

이번 패치는 즉시 수정 가능한 오류와 완전 이식의 필수 연결을 우선 반영했다. 아래 항목은 별도의 기능 변경 단계로 남겼다.

- Surface Snap의 World Z축 가정을 해부학적 대퇴골 장축 또는 PCA 축으로 교체
- 원점 X축 고정 미러링을 사용자 지정 평면 미러링으로 일반화
- `Logic.cxx`를 Combined ICP, Surface Snap, Mask Registration 등의 전용 클래스로 분리
- `vtkMRMLScriptedModuleNode`를 typed custom MRML parameter node로 교체
- ICP MeanDistance 외 대칭 표면 거리, RMS, gap, step-off 평가 추가
- `RotateVolume()`이 Scene의 모든 Segmentation을 연결하는 동작을 활성 Segmentation 중심으로 축소
- Undo/Redo 또는 정합 결과 미리보기 후 승인 방식 추가
- 실제 CT/골편 데이터 기반 Python-C++ 수치 회귀 테스트 추가

## 롤백

원본과 수정본의 차이는 패키지 루트의 `FemurFracturePlannerCpp_full_migration.patch`에서 확인할 수 있다. 문제가 있을 경우 원본 ZIP을 다시 사용하거나 patch의 해당 변경만 선택적으로 되돌릴 수 있다.
