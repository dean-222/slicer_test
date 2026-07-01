/*
File Name: qSlicerFemurFracturePlannerCppModulePlannerDialog.h
Version: v0-7.0.0
Date: 2026-07-01
Description: 대퇴골 골절 수술 계획을 위한 독립 팝업 다이얼로그 클래스 정의 (Slicer 씬 연동 추가)

Version History:
- v0-7.0.0 (2026-07-01)
  - 가상 뼈 정복 연산 실시간 로그 출력을 위한 addLogMessage 헬퍼 함수 추가 (X 증가)
- v0-6.0.0 (2026-06-30)
  - 완전 이식 보완: MRML 상태 저장/복원, Scene 생명주기, 마커 워크플로우, 일반/마스킹 에지 분리 (X 증가)
- v0-5.0.0 (2026-06-30)
  - 5단계: 골절면 정밀 스냅 및 수동/마스크 정합용 slots 추가 (X 증가)
- v0-4.0.2 (2026-06-30)
  - ModelFile 및 Model 다중 데이터 로드 시도 및 디버깅 로그 기능 보강 (Z 증가)
- v0-4.0.1 (2026-06-30)
  - 4단계 가이드 데이터 파일 추가 시 콤보박스 연동 누락 오류 수정 (Z 증가)
- v0-4.0.0 (2026-06-30)
  - 4단계: 정상 측 가이드 미러링 및 ICP 자동 정복용 slots 추가 (X 증가)
- v0-3.0.0 (2026-06-30)
  - 90도 고속 회전 슬롯 및 초기화 시 세그멘테이션/소스 볼륨 에디터 노드 자동 바인딩 로직 추가 (X 증가)
- v0-2.1.0 (2026-06-30)
  - 세그멘테이션 노드 동기화 슬롯 추가 및 입력 볼륨 변경 시 슬라이더 초기화, 3D 회전 제어 실시간 리렌더링 버그 수정 (Y 증가)
- v0-2.0.0 (2026-06-30)
  - 3단계 골편 분리 및 골편 목록 QTableWidget 연동 slots 추가 (X 증가)
- v0-1.3.0 (2026-06-30)
  - 입력 볼륨 선택 변경 시 2D Slice 가시성 자동 보기 연결용 슬롯 추가 (Y 증가)
- v0-1.2.1 (2026-06-30)
  - 밝기 반전 및 에지 검출 연산 직후 가시성 보기 기능을 자동 연결 활성화하도록 슬롯 개선 (Z 증가)
- v0-1.2.0 (2026-06-30)
  - 슬라이스 뷰 상의 볼륨 가시성 제어용 slot 추가 (Y 증가)
- v0-1.1.1 (2026-06-30)
  - 소멸자 내 SegmentEditorNode를 씬에서 명시적 제거하여 VTK 메모리 누수 해결 (Z 증가)
- v0-1.1.0 (2026-06-30)
  - 2단계 볼륨 렌더링 및 3D 회전 제어용 slots 선언 추가 (Y 증가)
- v0-1.0.3 (2026-06-30)
  - 상속 대상을 qMRMLWidget으로 변경하여 setupUi 컴파일 에러 해결 (Z 증가)
- v0-1.0.2 (2026-06-30)
  - 널 포인터 런타임 크래시 방어용 레이아웃 및 셀렉터 검증 로직 추가 (Z 증가)
- v0-1.0.1 (2026-06-30)
  - MSVC 한글 인코딩 오류 대응 (Z 증가)
- v0-1.0.0 (2026-06-30)
  - setMRMLScene() 선언 추가 (X 증가)
- v0-0.0.0 (2026-06-30)
  - 최초 작성
*/

#ifndef __qSlicerFemurFracturePlannerCppModulePlannerDialog_h
#define __qSlicerFemurFracturePlannerCppModulePlannerDialog_h

#include "qMRMLWidget.h"
#include <QScopedPointer>
#include <ctkVTKObject.h>

class qSlicerFemurFracturePlannerCppModulePlannerDialogPrivate;
class vtkMRMLScene;
class vtkMRMLNode;
class vtkSlicerFemurFracturePlannerCppLogic;
class vtkMRMLModelNode;
class QTableWidgetItem;
class QPushButton;
class QCloseEvent;

class qSlicerFemurFracturePlannerCppModulePlannerDialog : public qMRMLWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  explicit qSlicerFemurFracturePlannerCppModulePlannerDialog(QWidget* parent = nullptr);
  virtual ~qSlicerFemurFracturePlannerCppModulePlannerDialog();

  void setLogic(vtkSlicerFemurFracturePlannerCppLogic* logic);
  void setMRMLScene(vtkMRMLScene* scene);

protected:
  void closeEvent(QCloseEvent* event) override;

private slots:
  void onInputVolumeChanged(vtkMRMLNode* node);
  void onSegmentationNodeChanged(vtkMRMLNode* node);
  void onVolumeVisibilityToggled(bool checked);
  void onVolumeRenderingToggled(bool checked);
  void onThresholdChanged(double value);
  void onRotationAngleChanged(double angle);
  void onRotationAxisChanged();
  void onResetRotationClicked();
  void onRotate90Clicked();
  void onInvertIntensityClicked();
  void onEdgeDetectionClicked();
  void onMaskedEdgeDetectionClicked();
  void onLoadVolumeClicked();
  void onLoadDicomClicked();
  void onLoadGuideClicked();

  // 3단계 슬롯
  void onSeparateFragmentsClicked();
  void onClearModelsClicked();
  void updateFragmentTable();
  void onFragmentTableItemChanged(QTableWidgetItem* item);
  void onFragmentTableSelectionChanged();
  void onFragmentVisibilityToggled(vtkMRMLNode* node, bool checked);
  void onFragmentInteractionToggled(vtkMRMLNode* node, bool checked);
  void onFragmentColorClicked(vtkMRMLNode* node, QPushButton* button);
  void onFragmentDeleteClicked(vtkMRMLNode* node);
  void onActiveTransformNodeModified();

  // 4단계 슬롯
  void onGuideModelChanged(vtkMRMLNode* node);
  void onGuideVisibilityToggled(bool checked);
  void onGuideInteractionToggled(bool checked);
  void onMirrorGuideClicked();
  void onRunIcpReductionClicked();

  // 5단계 슬롯
  void onRunSurfaceSnapClicked();
  void onRunLandmarkMatchClicked();
  void onRunMaskMatchClicked();
  void onClearMarkersClicked();
  void onClearMasksClicked();

  // MRML Scene 생명주기
  void onMRMLSceneStartClose();
  void onMRMLSceneEndClose();
  void onMRMLSceneChanged();

private:
  vtkMRMLModelNode* selectedFragmentNode();
  void updateParameterNodeFromUi();
  void restoreUiFromParameterNode();
  void addLogMessage(const QString& message);

  QScopedPointer<qSlicerFemurFracturePlannerCppModulePlannerDialogPrivate> d_ptr;

  Q_DECLARE_PRIVATE(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  Q_DISABLE_COPY(qSlicerFemurFracturePlannerCppModulePlannerDialog);
};

#endif
