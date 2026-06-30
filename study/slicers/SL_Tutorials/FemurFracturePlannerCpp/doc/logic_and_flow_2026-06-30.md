# FemurFracturePlannerCpp 로직 및 동작 흐름 정리

Date: 2026-06-30

## 문서 목적

이 문서는 `FemurFracturePlannerCpp` 모듈의 주요 로직, UI 이벤트 흐름, MRML 노드 관계, 정합 처리 단계를 한눈에 볼 수 있도록 정리한 개발 참고 문서다.

대상 파일:

- `Logic/vtkSlicerFemurFracturePlannerCppLogic.h`
- `Logic/vtkSlicerFemurFracturePlannerCppLogic.cxx`
- `qSlicerFemurFracturePlannerCppModulePlannerDialog.h`
- `qSlicerFemurFracturePlannerCppModulePlannerDialog.cxx`
- `qSlicerFemurFracturePlannerCppModuleWidget.cxx`

실제 빌드나 Slicer 런타임 검증은 수행하지 않았고, 현재 코드 기준의 정적 구조를 정리했다.

## 파일별 주석

### `CMakeLists.txt`

파일 역할:

- `FemurFracturePlannerCpp` loadable module의 최상위 빌드 설정 파일이다.
- Logic, Widgets 하위 디렉터리를 먼저 추가한다.
- 모듈 소스, UI 파일, 리소스, target library 의존성을 `slicerMacroBuildLoadableModule()`에 전달한다.

주요 의존성:

- `vtkSlicerFemurFracturePlannerCppModuleLogic`
- `qSlicerFemurFracturePlannerCppModuleWidgets`
- `qSlicerSegmentationsModuleWidgets`
- `vtkSlicerSegmentationsModuleLogic`

주의할 점:

- Segmentations 관련 위젯과 로직을 사용하므로 CMake 의존성과 모듈 `dependencies()` 선언이 서로 맞아야 한다.
- 테스트는 `BUILD_TESTING`이 켜졌을 때만 `Testing` 하위 디렉터리를 추가한다.

### `Logic/CMakeLists.txt`

파일 역할:

- `vtkSlicerFemurFracturePlannerCppModuleLogic` 라이브러리 빌드 설정을 담당한다.
- Logic 구현 파일과 export directive를 Slicer 빌드 매크로에 전달한다.

주요 의존성:

- `vtkSlicerVolumesModuleLogic`
- `vtkSlicerVolumeRenderingModuleLogic`
- `vtkSlicerSegmentationsModuleLogic`
- `${ITK_LIBRARIES}`

주의할 점:

- Logic에서 `Volumes`, `VolumeRendering`, `Segmentations` logic을 직접 조회하므로 빌드 의존성이 누락되면 링크 또는 런타임 조회 문제가 생길 수 있다.

### `Logic/vtkSlicerFemurFracturePlannerCppLogic.h`

파일 역할:

- UI에서 호출하는 핵심 계산 API를 선언하는 공개 헤더다.
- 볼륨 표시, 볼륨 렌더링, 영상 처리, 골편 분리, 미러링, 정합 함수의 외부 인터페이스를 제공한다.
- 알고리즘 제어 상수를 한곳에 모아 둔다.

주요 API 그룹:

- 볼륨 제어: `SetVolumeVisibility`, `ShowVolumeRendering`, `HideVolumeRendering`, `AdjustVolumeRenderingThreshold`
- 영상 처리: `RotateVolume`, `InvertVolumeIntensity`, `DetectVolumeEdges`, `DetectVolumeEdgesMasked`
- 골편 처리: `SeparateBoneFragments`
- 가이드 모델 처리: `MirrorModel`
- 정합 처리: `RunIcpRegistration`, `RunFractureSurfaceSnap`, `RunLandmarkRegistration`, `RunMaskedIcpRegistration`

주의할 점:

- 공개 헤더이므로 `std::vector`, `std::string`처럼 선언부에서 쓰는 타입의 표준 헤더를 직접 include해야 한다.
- UI나 Dialog 타입을 포함하지 않아 Logic과 UI 결합도를 낮게 유지하는 것이 좋다.

### `Logic/vtkSlicerFemurFracturePlannerCppLogic.cxx`

파일 역할:

- 실제 MRML 조작과 VTK 알고리즘 실행을 담당하는 구현 파일이다.
- Dialog는 사용자 이벤트를 받아 이 파일의 Logic API를 호출한다.

주요 처리:

- slice background volume 설정
- volume rendering display node 생성 및 opacity 조정
- volume/segmentation/model transform 동기화
- volume clone 후 intensity inversion 및 edge detection
- visible segmentation closed surface 기반 골편 모델 생성
- guide model mirror 생성
- ICP, landmark, ROI mask 기반 rigid registration 수행

주의할 점:

- scene 전체 노드를 순회하는 로직이 많아 모듈 외부 노드에 영향을 줄 수 있다.
- `Femur_Fragment`, `_Mirrored` 같은 이름 규칙에 의존하는 코드가 있으므로 이름 변경 시 관련 경로를 함께 수정해야 한다.
- 표준 라이브러리와 VTK 타입 include를 transitive include에 의존하지 않도록 정리하는 것이 좋다.

### `qSlicerFemurFracturePlannerCppModule.h`

파일 역할:

- Slicer loadable module 클래스의 공개 인터페이스를 선언한다.
- 모듈 메타데이터, widget representation 생성, logic 생성 진입점을 제공한다.

주요 책임:

- `helpText`, `acknowledgementText`, `contributors`, `icon`, `categories`, `dependencies`
- `createWidgetRepresentation()`
- `createLogic()`

주의할 점:

- `dependencies()`에서 선언한 모듈 이름은 실제 사용하는 Slicer 모듈 의존성과 일치해야 한다.

### `qSlicerFemurFracturePlannerCppModule.cxx`

파일 역할:

- Slicer module metadata와 lifecycle hook을 구현한다.
- `Segmentations` 모듈 의존성을 선언하고 Logic/Widget 객체를 생성한다.

주요 흐름:

- `createLogic()`에서 `vtkSlicerFemurFracturePlannerCppLogic::New()` 호출
- `createWidgetRepresentation()`에서 `qSlicerFemurFracturePlannerCppModuleWidget` 생성

주의할 점:

- 추가 Slicer 모듈 logic을 Logic에서 사용하게 되면 `dependencies()`와 CMake target library를 함께 점검해야 한다.

### `qSlicerFemurFracturePlannerCppModuleWidget.h`

파일 역할:

- 모듈 패널 위젯의 인터페이스를 선언한다.
- Planner Dialog를 열기 위한 슬롯과 MRML scene 전달 hook을 정의한다.

주요 책임:

- `setup()`
- `enter()`
- `exit()`
- `setMRMLScene(...)`
- `onOpenPlannerWindowClicked()`

주의할 점:

- 실제 기능 UI 대부분은 Planner Dialog에 있으므로 이 파일은 얇은 bridge 역할로 유지하는 것이 좋다.

### `qSlicerFemurFracturePlannerCppModuleWidget.cxx`

파일 역할:

- 메인 모듈 위젯에서 Planner Dialog를 생성하고 관리한다.
- 모듈 진입/이탈 시 Dialog 표시 상태를 제어한다.

주요 흐름:

1. `setup()`에서 UI를 만들고 `openPlannerButton` signal을 연결한다.
2. Planner Dialog를 생성한다.
3. Dialog에 Logic을 주입한다.
4. `enter()`에서 Dialog를 표시한다.
5. `exit()`에서 Dialog를 숨긴다.
6. `setMRMLScene()`에서 Dialog로 scene을 전달한다.

주의할 점:

- Dialog는 parent가 모듈 위젯이므로 소멸 시점과 MRML scene 해제 순서를 주의해야 한다.
- Logic 주입 시 `SafeDownCast(this->logic())` 결과가 null일 가능성도 방어하면 좋다.

### `qSlicerFemurFracturePlannerCppModulePlannerDialog.h`

파일 역할:

- Planner Dialog의 public method와 UI slot 목록을 선언한다.
- Dialog는 `qMRMLWidget`을 상속하고 `QVTK_OBJECT`를 통해 VTK observer 연결을 사용한다.

주요 슬롯 그룹:

- 입력/표시: `onInputVolumeChanged`, `onVolumeVisibilityToggled`, `onVolumeRenderingToggled`, `onThresholdChanged`
- 영상 처리: `onRotationAngleChanged`, `onRotate90Clicked`, `onInvertIntensityClicked`, `onEdgeDetectionClicked`
- 데이터 로드: `onLoadVolumeClicked`, `onLoadDicomClicked`, `onLoadGuideClicked`
- 골편 관리: `onSeparateFragmentsClicked`, `updateFragmentTable`, `onFragment...`
- 가이드/정합: `onMirrorGuideClicked`, `onRunIcpReductionClicked`, `onRunSurfaceSnapClicked`, `onRunLandmarkMatchClicked`, `onRunMaskMatchClicked`
- 초기화: `onClearMarkersClicked`, `onClearMasksClicked`

주의할 점:

- slot이 많기 때문에 UI 이름, signal 연결, Logic API 이름이 서로 어긋나지 않도록 유지해야 한다.
- public API는 `setLogic()`과 `setMRMLScene()` 중심으로 제한되어 있어 구조는 비교적 명확하다.

### `qSlicerFemurFracturePlannerCppModulePlannerDialog.cxx`

파일 역할:

- Planner Dialog UI 이벤트의 대부분을 처리하는 중심 구현 파일이다.
- 사용자 조작을 MRML node 변경과 Logic API 호출로 연결한다.

주요 처리:

- Segment Editor Widget 동적 생성
- selector와 MRML scene 연결
- volume/segmentation/guide model 선택 변경 처리
- 버튼 signal과 slot 연결
- 골편 테이블 생성 및 per-row control 구성
- transform node observer 연결 및 위치/회전 표시 갱신
- 가이드 모델 표시, 이동, 미러링 처리
- ICP, fracture snap, landmark, mask matching 실행

주의할 점:

- 파일 길이가 길고 역할이 많아 향후 유지보수를 위해 기능 그룹별 helper 분리를 검토할 수 있다.
- marker node 이름 불일치처럼 UI slot 간 계약이 어긋나기 쉬우므로 문자열 상수화가 필요하다.
- scene node 삭제와 transform 삭제는 사용자 데이터에 영향을 줄 수 있어 이름 규칙과 삭제 범위를 신중히 유지해야 한다.

### `Widgets/CMakeLists.txt`

파일 역할:

- 별도 Widgets 라이브러리 빌드를 설정한다.
- `qSlicerFemurFracturePlannerCppFooBarWidget` 관련 소스와 UI를 빌드에 포함한다.

주의할 점:

- 현재 주요 Planner Dialog 기능과 직접 연결되는 파일은 아니지만, module target에서 widget library를 링크한다.
- 사용하지 않는 샘플 위젯이라면 유지 목적을 명확히 하거나 제거 여부를 별도로 판단해야 한다.

### `Widgets/qSlicerFemurFracturePlannerCppFooBarWidget.*`

파일 역할:

- Slicer extension template에서 생성된 보조/샘플 위젯으로 보인다.
- 현재 핵심 fracture planning 흐름에서는 직접적인 역할이 크지 않다.

주의할 점:

- 실제 기능에서 사용하지 않는다면 빌드 대상 유지 필요성을 점검할 수 있다.
- 제거하려면 `Widgets/CMakeLists.txt`, 상위 `CMakeLists.txt`, export/include 관계를 함께 확인해야 한다.

### `Resources/UI/qSlicerFemurFracturePlannerCppModulePlannerDialog.ui`

파일 역할:

- Planner Dialog의 Qt Designer UI 정의 파일이다.
- 입력 볼륨, 영상 처리, 세그멘테이션, 골편 테이블, 가이드 모델, 정합 버튼 등의 화면 요소를 정의한다.

주의할 점:

- `.cxx`에서 참조하는 objectName과 `.ui`의 widget name이 반드시 일치해야 한다.
- 버튼/selector 이름 변경 시 slot 연결 코드도 함께 수정해야 한다.

### `Resources/UI/qSlicerFemurFracturePlannerCppModuleWidget.ui`

파일 역할:

- 메인 모듈 위젯 UI 정의 파일이다.
- Planner Dialog를 여는 버튼 등 최소 진입 UI를 제공한다.

주의할 점:

- `openPlannerButton` 이름이 `.cxx`의 signal 연결과 일치해야 한다.

### `Resources/qSlicerFemurFracturePlannerCppModule.qrc`

파일 역할:

- Slicer module icon 등 Qt resource 목록을 관리한다.

주의할 점:

- `qSlicerFemurFracturePlannerCppModule.cxx`의 icon path와 qrc 등록 path가 일치해야 한다.

### `Testing/CMakeLists.txt` 및 `Testing/Cxx/CMakeLists.txt`

파일 역할:

- 모듈 테스트 빌드 구성을 담당한다.

주의할 점:

- 현재 Cxx 테스트 소스가 주석 처리되어 있어 자동 검증 효과가 거의 없다.
- 최소 module load 또는 Logic 생성 테스트부터 활성화하는 것이 좋다.

## 전체 구조

### 모듈 진입부

파일:

- `qSlicerFemurFracturePlannerCppModule.cxx`
- `qSlicerFemurFracturePlannerCppModuleWidget.cxx`

역할:

- Slicer loadable module 등록
- 모듈 카테고리와 아이콘 제공
- `Segmentations` 모듈 의존성 선언
- `vtkSlicerFemurFracturePlannerCppLogic` 생성
- 메인 위젯에서 Planner Dialog 생성 및 표시

흐름:

1. Slicer가 모듈을 로드한다.
2. `createLogic()`이 `vtkSlicerFemurFracturePlannerCppLogic` 인스턴스를 만든다.
3. `createWidgetRepresentation()`이 모듈 위젯을 만든다.
4. 위젯 `setup()`에서 Planner Dialog를 생성하고 Logic을 주입한다.
5. 모듈에 진입하면 `enter()`에서 Planner Dialog를 표시한다.
6. 모듈에서 나가면 `exit()`에서 Planner Dialog를 숨긴다.

### Planner Dialog

파일:

- `qSlicerFemurFracturePlannerCppModulePlannerDialog.h`
- `qSlicerFemurFracturePlannerCppModulePlannerDialog.cxx`

역할:

- 사용자 조작을 받는 팝업 UI
- 입력 볼륨, 세그멘테이션, 가이드 모델 선택
- Segment Editor 위젯 생성 및 MRML scene 연결
- Logic API 호출
- 골편 모델 테이블 관리
- 변환 노드의 수동 이동/회전 상태 표시

주요 내부 상태:

- `SegmentEditorWidget`: Slicer Segment Editor UI
- `SegmentEditorNode`: Segment Editor 동작 상태 저장 MRML 노드
- `Logic`: 계산 로직 객체
- `ActiveModelNode`: 현재 선택된 골편 모델
- `ActiveTransformNode`: 현재 선택된 골편의 transform node

### Logic

파일:

- `Logic/vtkSlicerFemurFracturePlannerCppLogic.h`
- `Logic/vtkSlicerFemurFracturePlannerCppLogic.cxx`

역할:

- UI와 분리된 계산 및 MRML 조작 담당
- 볼륨 표시/렌더링 제어
- 볼륨 회전 및 영상 처리
- 세그멘테이션에서 골편 모델 분리
- 가이드 모델 미러링
- ICP, landmark, mask 기반 정합 수행

## Logic API 정리

### 볼륨 표시 및 렌더링

#### `SetVolumeVisibility(vtkMRMLScalarVolumeNode* volumeNode, bool visible)`

역할:

- Slice view의 background volume을 설정하거나 해제한다.
- 모든 `vtkMRMLSliceCompositeNode`를 순회한다.

주요 동작:

- `visible == true`이면 입력 volume ID를 background로 지정
- `visible == false`이면 background volume ID를 `nullptr`로 지정

주의:

- 전체 slice composite node를 대상으로 하므로 현재 모듈 외의 slice view 상태에도 영향을 줄 수 있다.

#### `ShowVolumeRendering(vtkMRMLScalarVolumeNode* volumeNode, double threshold)`

역할:

- Volume Rendering display node를 만들거나 가져와 CT-Bone preset 기반으로 표시한다.

주요 동작:

- `VolumeRendering` logic 조회
- display node 생성 또는 재사용
- `CT-Bone` preset 적용
- threshold 기반 opacity function 조정
- volume rendering visibility 활성화

#### `HideVolumeRendering(vtkMRMLScalarVolumeNode* volumeNode)`

역할:

- 입력 볼륨의 volume rendering display node를 scene에서 제거한다.

주의:

- 단순 visibility off가 아니라 display node 제거 방식이다.
- 이후 다시 켜면 display node와 property가 새로 만들어질 수 있다.

#### `AdjustVolumeRenderingThreshold(vtkMRMLScalarVolumeNode* volumeNode, double threshold)`

역할:

- volume property의 scalar opacity function을 threshold 기준으로 재설정한다.

관련 상수:

- `OPACITY_OFFSET_MIN = -100.0`
- `OPACITY_OFFSET_MID = 200.0`
- `OPACITY_MAX_HU = 1500.0`

### 볼륨 회전 및 영상 처리

#### `RotateVolume(vtkMRMLScalarVolumeNode* volumeNode, const char* axis, double angle, vtkMRMLModelNode* guideNode = nullptr)`

역할:

- 입력 볼륨에 linear transform을 적용해 X/Y/Z 축 회전을 제어한다.
- 세그멘테이션 및 관련 모델도 transform 계층에 연결해 함께 회전하도록 처리한다.

주요 동작:

1. 입력 볼륨의 parent transform을 확인한다.
2. 없으면 새 `vtkMRMLLinearTransformNode`를 생성한다.
3. scene 내 segmentation node들을 같은 transform에 연결한다.
4. `Femur_Fragment*`, `_Mirrored`, 선택된 guide node 모델을 transform 계층에 연결한다.
5. 축과 각도에 따라 rotation matrix를 만들고 transform node에 적용한다.
6. volume rendering display node가 있으면 `Modified()`를 호출한다.

주의:

- scene 내 segmentation node 전체를 대상으로 한다.
- 이름 규칙 기반으로 모델을 선택하므로 모델 이름 변경에 민감하다.

#### `InvertVolumeIntensity(vtkMRMLScalarVolumeNode* volumeNode)`

역할:

- 입력 볼륨을 clone한 뒤 intensity를 반전한 새 volume node를 반환한다.

주요 동작:

- `Volumes` logic으로 clone 생성
- unsigned scalar는 short로 cast
- short image 기준으로 min/max를 구하고 `max + min - value` 적용
- display node auto window/level 활성화

#### `DetectVolumeEdges(vtkMRMLScalarVolumeNode* volumeNode)`

역할:

- 입력 볼륨을 clone한 뒤 3D gradient magnitude 기반 edge volume을 만든다.

#### `DetectVolumeEdgesMasked(vtkMRMLScalarVolumeNode* volumeNode, double threshold)`

역할:

- threshold 이하 영역을 air value로 마스킹한 뒤 edge detection을 수행한다.

관련 상수:

- `MASK_AIR_VALUE = -1000`

## 골편 분리 흐름

### `SeparateBoneFragments(vtkMRMLSegmentationNode* segmentationNode, vtkMRMLScalarVolumeNode* referenceVolumeNode = nullptr)`

역할:

- 세그멘테이션의 visible segment closed surface를 합친 뒤 connectivity filter로 독립 골편 모델을 추출한다.

처리 단계:

1. segmentation과 display node 유효성 확인
2. visible segment ID 수집
3. closed surface representation 생성
4. visible segment들의 polydata를 append
5. `vtkPolyDataConnectivityFilter`로 연결 영역 분리
6. 영역 크기 기준 내림차순 정렬
7. 상위 2개 영역 중 `MIN_FRAGMENT_POINTS`보다 큰 영역만 모델로 생성
8. `Femur_Fragment_1`, `Femur_Fragment_2` 이름으로 model node 생성
9. 원본 segmentation transform이 있으면 골편 모델에 연결
10. model display node 생성 후 색상 지정

관련 상수:

- `MIN_FRAGMENT_POINTS = 200`

출력:

- 생성된 골편 모델 개수

주의:

- 현재 최대 2개 골편만 생성한다.
- 골편 이름이 이후 UI 테이블, transform 정리, 정합 대상 검색에 사용된다.

## 가이드 모델 및 정합 흐름

### `MirrorModel(vtkMRMLModelNode* modelNode)`

역할:

- 가이드 모델을 좌우 반전한 mirrored model node를 생성한다.

처리 단계:

1. 입력 모델의 polydata 확인
2. mirror transform 적용
3. normal 방향 보정을 위해 reverse sense 처리
4. 새 model node 생성
5. 이름에 `_Mirrored` suffix 추가
6. display node 생성 및 반투명 파란색 계열로 표시

### `RunIcpRegistration(...)`

역할:

- 여러 골편 모델을 하나의 source polydata로 합친 뒤 guide model에 대해 ICP rigid registration을 수행한다.

입력:

- `fragmentModelNodes`: 이동 대상 골편 모델 목록
- `guideModelNode`: 기준 가이드 모델
- `meanDistance`: 평균 정합 오차 출력
- `iterations`: ICP 반복 횟수 출력

처리 단계:

1. guide model을 world coordinate polydata로 변환
2. 모든 fragment model을 world coordinate polydata로 변환
3. fragment polydata들을 append해 하나의 source polydata로 구성
4. `vtkIterativeClosestPointTransform` 실행
5. ICP matrix를 각 fragment transform node에 누적 적용

관련 상수:

- `ICP_MAX_ITERATIONS = 100`
- `ICP_MAX_LANDMARKS = 200`
- `ICP_CONVERGENCE_DISTANCE = 0.01`

주의:

- `StartByMatchingCentroidsOff()`로 설정되어 있어 사용자가 맞춘 초기 배치를 보존하는 방향이다.
- 각 fragment의 parent transform 계층이 있으면 world/local matrix 변환을 고려해 적용한다.

### `RunFractureSurfaceSnap(...)`

역할:

- `Femur_Fragment_1`과 `Femur_Fragment_2`의 골절면이 서로 맞닿도록 landmark 기반 rigid transform을 계산한다.

처리 단계 요약:

1. 두 골편 polydata를 world coordinate로 변환
2. target 골편에 KD-tree locator 생성
3. source 골편을 down-sampling
4. 두 골편의 point normal 계산
5. Z 위치와 normal 방향을 이용해 골절면 후보 영역 필터링
6. 가까운 대응점을 수집
7. 대응점 개수가 `SNAP_MIN_POINTS` 이상이면 landmark transform 계산
8. source 골편 transform에 누적 적용
9. 평균 거리 계산 후 반환

관련 상수:

- `SNAP_DISTANCE_LIMIT = 25.0`
- `SNAP_MIN_POINTS = 5`

주의:

- Z축 기준 상하 관계와 normal 방향 조건에 의존한다.
- 골편이 충분히 가까이 배치되어 있지 않으면 대응점 수집에 실패할 수 있다.

### `RunLandmarkRegistration(...)`

역할:

- 사용자가 찍은 source/target fiducial marker를 이용해 rigid landmark registration을 수행한다.

처리 단계:

1. source/target markups fiducial node 확인
2. 두 마커 노드의 공통 control point 개수 계산
3. control point가 3개 미만이면 실패
4. world coordinate point pair 수집
5. `vtkLandmarkTransform`으로 rigid transform 계산
6. source model transform에 적용

주의:

- Dialog 코드에서 조회하는 marker node 이름이 clear 동작과 일치하지 않는다.
- 마커 생성 위치와 이름 규칙을 함께 점검해야 한다.

### `RunMaskedIcpRegistration(...)`

역할:

- 세그멘테이션에 칠해진 ROI segment를 mask로 사용해 source/target model의 부분 표면만 추출하고 ICP를 수행한다.

입력:

- `sourceNode`: 이동 대상 모델
- `targetNode`: 기준 모델
- `segNode`: ROI segment가 들어 있는 segmentation node
- `sourceSegId`: source ROI segment ID
- `targetSegId`: target ROI segment ID

처리 단계:

1. segmentation node와 segment representation 확인
2. source/target ROI closed surface 획득
3. model polydata를 world coordinate로 변환
4. `vtkSelectEnclosedPoints`와 `vtkThresholdPoints`로 ROI 내부 표면 추출
5. 추출된 source/target 부분 표면에 ICP 수행
6. source model transform에 결과 적용

주의:

- ROI segment가 비어 있거나 closed surface 생성이 실패하면 정합 실패
- Dialog에서는 `Targeting_Source_ROI`, `Targeting_Target_ROI` segment 이름을 사용한다.

## Dialog 이벤트 흐름

### Scene 연결

핵심 함수:

- `setMRMLScene(vtkMRMLScene* scene)`

역할:

- Dialog와 내부 selector, Segment Editor Widget에 MRML scene을 전달한다.
- 기본 segmentation node를 찾거나 생성한다.
- Segment Editor Node를 찾거나 생성한다.
- 현재 선택된 volume/segmentation을 Segment Editor에 동기화한다.

### 입력 볼륨 변경

핵심 함수:

- `onInputVolumeChanged(vtkMRMLNode* node)`

흐름:

1. 선택된 volume node 확인
2. 현재 segmentation node 확인
3. Segment Editor의 source volume 설정
4. segmentation reference geometry를 volume 기준으로 설정
5. 회전 slider 값을 0으로 초기화
6. volume visibility 버튼 상태에 따라 slice view 표시 갱신

### 세그멘테이션 변경

핵심 함수:

- `onSegmentationNodeChanged(vtkMRMLNode* node)`

흐름:

1. 선택된 segmentation node 확인
2. Segment Editor Widget에 segmentation node 설정
3. 현재 input volume이 있으면 reference geometry 동기화

### 골편 테이블 관리

핵심 함수:

- `updateFragmentTable()`
- `onFragmentTableSelectionChanged()`
- `onFragmentInteractionToggled(...)`
- `onFragmentVisibilityToggled(...)`
- `onFragmentColorClicked(...)`
- `onFragmentDeleteClicked(...)`

역할:

- scene 내 `Femur_Fragment*` model node를 검색해 테이블에 표시한다.
- 각 골편의 표시 여부, transform editor visibility, 색상, 삭제 동작을 제공한다.
- 선택된 골편의 transform node를 관찰하고 위치/회전 표시값을 갱신한다.

### 가이드 모델 관리

핵심 함수:

- `onLoadGuideClicked()`
- `onGuideModelChanged(...)`
- `onGuideVisibilityToggled(...)`
- `onGuideInteractionToggled(...)`
- `onMirrorGuideClicked()`

역할:

- 외부 모델 파일 로드
- guide model selector 동기화
- guide model 표시/수동 이동 제어
- mirrored guide model 생성

### 정합 실행 버튼

핵심 함수:

- `onRunIcpReductionClicked()`
- `onRunSurfaceSnapClicked()`
- `onRunLandmarkMatchClicked()`
- `onRunMaskMatchClicked()`

역할:

- UI에서 필요한 모델, 마커, 세그먼트를 확인한다.
- Logic의 정합 API를 호출한다.
- 성공/실패 메시지를 표시한다.
- 성공 시 fragment table을 갱신한다.

## 주요 MRML 노드와 이름 규칙

### Volume

- 입력 CT volume: 사용자가 selector 또는 파일 로드로 선택
- 파생 volume:
  - `{원본}_Inverted`
  - `{원본}_Edge`
  - `{원본}_EdgeMasked`

### Segmentation

- 기본 segmentation 이름:
  - `Femur_Segmentation`
- Segment Editor Node:
  - `FemurFracturePlanner_SegmentEditorNode`
- Mask ROI segment:
  - `Targeting_Source_ROI`
  - `Targeting_Target_ROI`

### Model

- 골편 모델:
  - `Femur_Fragment_1`
  - `Femur_Fragment_2`
- 미러링 모델:
  - `{원본 모델명}_Mirrored`

### Transform

- volume rotation transform:
  - `{volumeName}_RotationTransform`
- 골편 transform:
  - `{fragmentName}_Transform`
- guide transform:
  - `{guideName}_Transform`

## 현재 코드 기준 점검 포인트

### include 정리 필요

`Logic`과 `PlannerDialog` 구현은 일부 표준/Qt/VTK 타입을 transitive include에 의존한다. 빌드 안정성을 위해 직접 include를 추가하는 것이 좋다.

### 마커 노드 이름 통일 필요

현재 landmark registration은 `Targeting_Source_Fiducials`, `Targeting_Target_Fiducials`를 찾고, clear markers는 `Targeting_Source_Markers`, `Targeting_Target_Markers`를 찾는다.

정리 방향:

- 노드 이름을 하나로 통일
- 생성, 조회, 초기화, 안내 메시지까지 같은 이름 사용
- 문자열 상수화 권장

### 이름 기반 로직 의존성

다음 기능들은 모델 이름 prefix/suffix에 의존한다.

- 골편 테이블 갱신
- 골편 삭제
- transform 삭제
- 회전 동기화
- 정합 대상 검색

따라서 `Femur_Fragment` 및 `_Mirrored` 이름 규칙을 바꾸면 관련 코드 경로를 함께 점검해야 한다.

### scene 전체 대상 조작 주의

일부 함수는 scene 내 모든 segmentation/model/slice composite node를 순회한다. 모듈 외부에서 만든 노드까지 영향을 받을 수 있으므로, 향후에는 module-owned node 식별 규칙을 강화하는 것이 좋다.

## 권장 개발 순서

1. 누락 include 보강
2. marker/fiducial 노드 이름 통일
3. 마커 생성 흐름 명확화
4. Logic 함수별 최소 단위 테스트 추가
5. Slicer 내 수동 시나리오 검증

수동 검증 시나리오:

1. 모듈 로드
2. CT volume 로드
3. segmentation 선택 또는 자동 생성 확인
4. volume visibility 및 volume rendering 토글
5. threshold 변경
6. segmentation 생성 후 골편 분리
7. 골편 테이블 표시/색상/삭제 확인
8. guide model 로드 및 미러링
9. ICP 정합 실행
10. marker 기반 정합 실행
11. ROI mask 기반 정합 실행

## 관련 문서

- `doc/code_review_2026-06-30.md`: 코드 리뷰 발견 사항
- `doc/troubleshooting.md`: 기존 컴파일 및 런타임 문제 정리
