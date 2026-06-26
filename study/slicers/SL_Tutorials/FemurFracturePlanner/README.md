# Femur Fracture Planner (대퇴골 골절 계획 플래너)

3D Slicer 환경에서 환자의 대퇴골 CT 볼륨 데이터를 기반으로 골편을 정밀하게 분리하고, 가이드 모델 및 최근접 점 탐색을 기반으로 정밀한 가상 뼈 정복(Bone Reduction) 계획을 수립하는 플래너 모듈입니다.

* **최신 릴리스 버전**: `v1-0.0.0` (정식 릴리스)
* **릴리스 일자**: 2026-06-26

---

## 주요 기능

### 1. 영상 처리 및 볼륨 렌더링
- CT 볼륨 강도 반전 (`vtkImageCast` 기반 Signed short 반전 처리로 언더플로우 방지)
- 마스크 기반 3D 에지 검출 (`detectVolumeEdgesMasked` 필터를 활용한 연부조직 노이즈 제거)
- CT-Bone 프리셋을 활용한 실시간 불투명도(Threshold) 조정

### 2. 고속 골편 분리
- 세그멘테이션(Segmentation)으로부터 서로 연결되지 않은 독립된 뼈 조각들을 연결 필터(`vtkConnectivityFilter`)를 통해 분리
- 고립 정점 제거(`vtkCleanPolyData`) 전처리를 수행하여 실제 조각 크기와 완전히 일치하는 가시성 및 Bounds 제어
- 뼈 조각 생성 시 시각적 구분이 쉬운 고대비(High-contrast) 색상 자동 매칭

### 3. 병합 ICP 자동 정복 (Combined ICP Registration) `★v1.0.0 핵심`
- **문제 해결**: 개별 뼈가 따로 정합되며 맞닿은 단면이 어긋나던 한계를 해결하기 위해 도입
- **연산 흐름**: 정합 대상인 모든 골편의 월드 좌표 메쉬를 단일 메쉬로 병합(`vtkAppendPolyData`)한 후, 가이드 표면과의 단 일회성 ICP를 수행하여 **공통 아핀 변환 행렬** 유도
- **결과**: 골편 간 맞물림(스냅) 상태를 100% 보존하면서, 가이드 뼈의 해부학적 축과 길이 방향 정렬에 맞춰 전체 뼈 뭉치를 정밀 도킹

### 4. 고정밀 골절면 스냅 (Fracture Surface Snap) `★v1.0.0 핵심`
- **단면 무게중심 기준 초기 오프셋 보정 (Pre-alignment)**: 두 뼈가 수평/수직으로 크게 어긋나 있어도 가상으로 단면 중심을 맞춰 KD-Tree를 돌려 올바른 맞대응 단면 점들만 1:1 매칭 수립
- **정밀한 노이즈 필터링**:
  - **법선 내적 임계값 (-0.6 이하)**: 정확히 정반대로 마주 보는 진짜 단면 정점들만 선별
  - **장축 정렬 방향성 임계값 (0.4 이상)**: 사선 방향의 옆구리(Cortex) 표면 유입을 원천 차단
- **원샷 최소제곱 강체 정합**: `vtkLandmarkTransform`을 활용해 단차 및 회전 뒤틀림 없는 매끄러운 밀착 결합 실현

### 5. 실시간 진행 피드백 로그창
- UI 패널 하단에 QTextEdit 기반의 로그창을 탑재하여 정합 델타 좌표, 수집된 랜드마크 포인트 수, 정합 최종 평균 오차(mm) 등을 실시간 출력 피드백

---

## 프로젝트 디렉토리 구조

```text
FemurFracturePlanner/
│
├── FemurFracturePlanner.py           # 모듈 등록 진입점 및 자가 테스트 클래스
├── CMakeLists.txt                    # Slicer 빌드 설정 파일
├── weekly_report.md                  # 작업 경과 리포트
├── README.md                         # [본 파일] 메인 설명 문서
│
├── FemurFracturePlannerLib/          # 모듈 비즈니스 로직 및 컴포넌트 폴더
│   ├── __init__.py
│   ├── FemurFracturePlannerLogic.py  # 핵심 연산 및 ICP/스냅/영상처리 백엔드 로직
│   ├── FemurFracturePlannerWidget.py # 팝업 UI 생성 및 위젯 생명주기 제어
│   ├── FemurFracturePlannerWorkflow.py # 버튼 시그널 바인딩 및 사용자 작업 흐름 관리
│   └── FemurFracturePlannerFragmentManager.py # 골편 목록 테이블, 조작, 삭제 컨트롤
│
├── Resources/
│   └── UI/
│       └── FemurFracturePlanner.ui   # Qt Designer 기반 UI 레이아웃
│
└── doc/                              # 파일별 상세 아키텍처 기술 문서 폴더
    ├── README.md                     # 문서 읽기 가이드
    ├── 01_전체구조.md
    ├── 02_메인모듈과위젯.md
    ├── 03_워크플로우.md
    ├── 04_로직계층.md
    ├── 05_골편관리.md
    ├── 06_엣지검출로직.md
    └── 07_골절면스냅로직.md          # 골절면 스냅 수학적/물리적 상세 알고리즘 가이드
```

---

## 상세 기술 문서 안내
모듈 개발 및 리팩터링을 위해 코드를 읽거나 수정할 경우, `doc/` 디렉토리 아래의 마크다운 상세 문서를 참고하시기 바랍니다.
특히 골절면 스냅 알고리즘의 세부 수학적 처리는 [07_골절면스냅로직.md](file:///C:/airs/study/slicers/SL_Tutorials/FemurFracturePlanner/doc/07_골절면스냅로직.md)에서 자세히 다루고 있습니다.
