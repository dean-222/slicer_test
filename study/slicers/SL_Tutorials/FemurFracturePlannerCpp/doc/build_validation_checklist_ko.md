# 빌드 및 실행 검증 체크리스트

## 빌드

- [ ] Slicer 5.10.0 Release와 동일한 빌드 설정 사용
- [ ] Markups, Segmentations, Volumes, VolumeRendering target 탐색 성공
- [ ] Logic library 컴파일 성공
- [ ] Loadable module library 컴파일 성공
- [ ] 테스트 드라이버 컴파일 성공
- [ ] CTest Logic 기본 테스트 통과

## Scene 및 상태

- [ ] 모듈 최초 진입 시 기본 Segmentation 생성 또는 기존 노드 복원
- [ ] Input Volume, Segmentation, Guide 선택 저장
- [ ] Scene 저장 후 다시 열었을 때 UI 상태 복원
- [ ] Scene Clear 시 크래시 및 죽은 포인터 없음
- [ ] Node 삭제 후 골편 표 자동 갱신

## 기능

- [ ] 일반 에지 버튼이 threshold 없이 동작
- [ ] 마스킹 에지 버튼이 threshold를 사용
- [ ] 골편 이름 변경 후에도 표와 ICP에서 인식
- [ ] Combined ICP에서 Guide가 Source에 포함되지 않음
- [ ] Surface Snap에서 선택 골편이 이동
- [ ] 마커 노드 자동 생성 및 Place mode 진입
- [ ] 6점 자동 분할
- [ ] 비공선/개수 불일치 입력 거부
- [ ] Mask ROI 자동 생성 및 Mask ICP 실행

## Transform

- [ ] 부모 Transform이 없는 골편
- [ ] 선형 부모 Transform 1단계
- [ ] 선형 부모 Transform 2단계
- [ ] 비선형 Transform 입력 시 안전하게 실패
- [ ] Gizmo를 켜도 기존 부모 Transform 계층 유지
