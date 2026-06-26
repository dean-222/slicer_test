# FemurFracturePlanner 문서

이 폴더는 `FemurFracturePlanner` 모듈을 빠르게 파악하기 위한 내부 문서 모음입니다.

## 추천 읽기 순서

1. `01_전체구조.md`
2. `02_메인모듈과위젯.md`
3. `03_워크플로우.md`
4. `04_로직계층.md`
5. `05_골편관리.md`

## 현재 코드 파일 구성

- `FemurFracturePlanner.py`
  - Slicer 모듈 등록 진입점
  - 기본 테스트 클래스 포함
- `FemurFracturePlannerWidget.py`
  - 메인 위젯 컨트롤러
  - 팝업 창 생성 및 UI 생명주기 관리
- `FemurFracturePlannerWorkflow.py`
  - 입력 볼륨, 렌더링, 회전, 세그멘테이션, 가이드 모델 흐름 담당
- `FemurFracturePlannerLogic.py`
  - 실제 계산 로직 담당
- `FemurFracturePlannerFragmentManager.py`
  - 골편 목록, 선택, 색상, 이동, 삭제 관리
- `Resources/UI/FemurFracturePlanner.ui`
  - Qt Designer 기반 UI 레이아웃

## 이 문서의 목적

- 파일별 역할을 빠르게 이해하기
- 어떤 기능이 어느 파일에 있는지 찾기 쉽게 하기
- 이후 리팩터링이나 기능 추가 시 수정 위치를 판단하기 쉽게 하기
