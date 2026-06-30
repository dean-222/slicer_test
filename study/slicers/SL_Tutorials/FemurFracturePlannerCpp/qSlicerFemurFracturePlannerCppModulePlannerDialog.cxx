/*
File Name: qSlicerFemurFracturePlannerCppModulePlannerDialog.cxx
Version: v0-0.0.0
Date: 2026-06-30
Description: 대퇴골 골절 수술 계획을 위한 독립 팝업 다이얼로그 클래스 구현

Version History:
- v0-0.0.0 (2026-06-30)
  - 최초 작성
*/

/*
File Name: qSlicerFemurFracturePlannerCppModulePlannerDialog.cxx
Version: v0-5.1.0
Date: 2026-06-30
Description: 대퇴골 골절 수술 계획을 위한 독립 팝업 다이얼로그 클래스 구현 (Segment Editor 및 씬 바인딩)

Version History:
- v0-5.1.0 (2026-06-30)
  - 수동 타겟팅 매칭 슬롯을 파이쓬 원본과 일치하게 수정: 마커 노드명(마커 접미사), 소스/타겟 구조(테이블+가이드 셀렉터 기반), 자동 생성+Place 모드, 6개 자동 분할, RemoveAllControlPoints 방식 클리어 (Y 증가)
- v0-5.0.0 (2026-06-30)
  - 5단계: 골절면 정밀 스냅 및 수동/마스크 정합용 slots 구현 추가 (X 증가)
- v0-4.0.2 (2026-06-30)
  - ModelFile 및 Model 다중 데이터 로드 시도 및 디버깅 로그 기능 보강 (Z 증가)
- v0-4.0.1 (2026-06-30)
  - 4단계 가이드 데이터 파일 추가 시 콤보박스 연동 누락 오류 수정 (Z 증가)
- v0-4.0.0 (2026-06-30)
  - 4단계: 정상 측 가이드 미러링 및 ICP 자동 정복용 slots 구현 추가 (X 증가)
- v0-3.0.0 (2026-06-30)
  - 90도 고속 회전 슬롯 구현 및 초기화 시 세그멘테이션/소스 볼륨 에디터 노드 자동 바인딩 로직 추가 (X 증가)
- v0-2.1.0 (2026-06-30)
  - 세그멘테이션 노드 동기화 슬롯 구현 및 입력 볼륨 변경 시 슬라이더 초기화, 3D 회전 제어 실시간 리렌더링 버그 수정 (Y 증가)
- v0-2.0.0 (2026-06-30)
  - 3단계 골편 분리 및 골편 목록 QTableWidget 연동 slots 구현 (X 증가)
- v0-1.3.0 (2026-06-30)
  - 입력 볼륨 선택 변경 시 2D Slice 가시성 자동 보기 연결용 슬롯 구현 (Y 증가)
- v0-1.2.1 (2026-06-30)
  - 밝기 반전 및 에지 검출 연산 직후 가시성 보기 기능을 자동 연결 활성화하도록 슬롯 개선 (Z 증가)
- v0-1.2.0 (2026-06-30)
  - 슬라이스 뷰 내 배경 볼륨 가시성 제어 슬롯 추가 및 연동 (Y 증가)
- v0-1.1.1 (2026-06-30)
  - 소멸자 내 SegmentEditorNode를 씬에서 명시적 제거하여 VTK 메모리 누수 해결 (Z 증가)
- v0-1.1.0 (2026-06-30)
  - 2단계 볼륨 렌더링 및 3D 회전 제어용 slots 구현 추가 (Y 증가)
- v0-1.0.3 (2026-06-30)
  - 상속 대상을 qMRMLWidget으로 변경하여 setupUi 컴파일 에러 해결 (Z 증가)
- v0-1.0.2 (2026-06-30)
  - 널 포인터 런타임 크래시 방어용 레이아웃 및 셀렉터 검증 로직 추가 (Z 증가)
- v0-1.0.1 (2026-06-30)
  - MSVC 한글 인코딩 오류 대응 (Z 증가)
- v0-1.0.0 (2026-06-30)
  - setMRMLScene() 구현 및 qMRMLSegmentEditorWidget 임베딩 추가 (X 증가)
- v0-0.0.0 (2026-06-30)
  - 최초 작성
*/

#include "qSlicerFemurFracturePlannerCppModulePlannerDialog.h"
#include "ui_qSlicerFemurFracturePlannerCppModulePlannerDialog.h"
#include "vtkSlicerFemurFracturePlannerCppLogic.h"

// Slicer includes
#include "qMRMLSegmentEditorWidget.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLSegmentEditorNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLModelNode.h"
#include "vtkCollection.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QMainWindow>
#include <QAction>
#include <QTableWidget>
#include <QCheckBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QColorDialog>
#include <QMessageBox>
#include <qSlicerApplication.h>
#include <qSlicerMainWindow.h>
#include <qSlicerModuleSelectorToolBar.h>
#include <qSlicerCoreIOManager.h>

#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLTransformDisplayNode.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkCommand.h>
#include <vtkMRMLTransformableNode.h>
#include <vtkPolyData.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>

//-----------------------------------------------------------------------------
class qSlicerFemurFracturePlannerCppModulePlannerDialogPrivate : public Ui::FemurFracturePlanner
{
public:
  qSlicerFemurFracturePlannerCppModulePlannerDialogPrivate();
  qMRMLSegmentEditorWidget* SegmentEditorWidget;
  vtkMRMLSegmentEditorNode* SegmentEditorNode;
  vtkSlicerFemurFracturePlannerCppLogic* Logic;

  vtkMRMLModelNode* ActiveModelNode;
  vtkMRMLLinearTransformNode* ActiveTransformNode;
};

//-----------------------------------------------------------------------------
qSlicerFemurFracturePlannerCppModulePlannerDialogPrivate::qSlicerFemurFracturePlannerCppModulePlannerDialogPrivate()
  : SegmentEditorWidget(nullptr)
  , SegmentEditorNode(nullptr)
  , Logic(nullptr)
  , ActiveModelNode(nullptr)
  , ActiveTransformNode(nullptr)
{
}

//-----------------------------------------------------------------------------
qSlicerFemurFracturePlannerCppModulePlannerDialog::qSlicerFemurFracturePlannerCppModulePlannerDialog(QWidget* parent)
  : qMRMLWidget(parent)
  , d_ptr(new qSlicerFemurFracturePlannerCppModulePlannerDialogPrivate)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  d->setupUi(this);

  // qMRMLSegmentEditorWidget 동적 생성 및 배치
  d->SegmentEditorWidget = new qMRMLSegmentEditorWidget(this);
  
  // segmentEditorAnchorWidget 및 내부 Layout의 널 포인터 런타임 크래시 방지 방어 로직
  if (d->segmentEditorAnchorWidget)
  {
    QLayout* layout = d->segmentEditorAnchorWidget->layout();
    if (!layout)
    {
      layout = new QVBoxLayout(d->segmentEditorAnchorWidget);
      d->segmentEditorAnchorWidget->setLayout(layout);
    }
    layout->addWidget(d->SegmentEditorWidget);
  }

  // 세그먼테이션 선택기는 노출시키고, 소스 볼륨 선택기는 상단 입력 볼륨 selector와 중복되므로 숨김
  d->SegmentEditorWidget->setSegmentationNodeSelectorVisible(true);
  d->SegmentEditorWidget->setSourceVolumeNodeSelectorVisible(false);
  
  // 골절 수술 계획 시 필요한 도구들만 엄선하여 세팅
  QStringList effects;
  effects << "Paint" << "Draw" << "Erase" << "Level tracing" << "Threshold" << "Margin" << "Hollow" << "Scissors";
  d->SegmentEditorWidget->setEffectNameOrder(effects);

  // 회전 각도 슬라이더 범위 지정 (-180 ~ +180도) 및 기본 축 선택
  if (d->rotationAngleSlider)
  {
    d->rotationAngleSlider->setMinimum(-180.0);
    d->rotationAngleSlider->setMaximum(180.0);
  }
  if (d->rotationXRadio)
  {
    d->rotationXRadio->setChecked(true);
  }

  // 볼륨 보기/볼륨 렌더링 보기 버튼 checkable 강제 보장 및 커넥션 설정
  if (d->inputVolumeSelector)
  {
    connect(d->inputVolumeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onInputVolumeChanged(vtkMRMLNode*)));
  }
  if (d->activeSegmentationSelector)
  {
    connect(d->activeSegmentationSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onSegmentationNodeChanged(vtkMRMLNode*)));
  }
  if (d->volumeVisibilityButton)
  {
    d->volumeVisibilityButton->setCheckable(true);
    connect(d->volumeVisibilityButton, SIGNAL(toggled(bool)), this, SLOT(onVolumeVisibilityToggled(bool)));
  }
  if (d->volumeRenderingVisibilityButton)
  {
    d->volumeRenderingVisibilityButton->setCheckable(true);
    connect(d->volumeRenderingVisibilityButton, SIGNAL(toggled(bool)), this, SLOT(onVolumeRenderingToggled(bool)));
  }
  if (d->thresholdSlider)
  {
    connect(d->thresholdSlider, SIGNAL(valueChanged(double)), this, SLOT(onThresholdChanged(double)));
  }
  if (d->rotationAngleSlider)
  {
    connect(d->rotationAngleSlider, SIGNAL(valueChanged(double)), this, SLOT(onRotationAngleChanged(double)));
  }
  if (d->rotationXRadio)
  {
    connect(d->rotationXRadio, SIGNAL(clicked()), this, SLOT(onRotationAxisChanged()));
  }
  if (d->rotationYRadio)
  {
    connect(d->rotationYRadio, SIGNAL(clicked()), this, SLOT(onRotationAxisChanged()));
  }
  if (d->rotationZRadio)
  {
    connect(d->rotationZRadio, SIGNAL(clicked()), this, SLOT(onRotationAxisChanged()));
  }
  if (d->resetRotationButton)
  {
    connect(d->resetRotationButton, SIGNAL(clicked()), this, SLOT(onResetRotationClicked()));
  }
  if (d->rotate90Button)
  {
    connect(d->rotate90Button, SIGNAL(clicked()), this, SLOT(onRotate90Clicked()));
  }
  if (d->invertIntensityButton)
  {
    connect(d->invertIntensityButton, SIGNAL(clicked()), this, SLOT(onInvertIntensityClicked()));
  }
  if (d->edgeDetectionButton)
  {
    connect(d->edgeDetectionButton, SIGNAL(clicked()), this, SLOT(onEdgeDetectionClicked()));
  }
  if (d->loadVolumeButton)
  {
    connect(d->loadVolumeButton, SIGNAL(clicked()), this, SLOT(onLoadVolumeClicked()));
  }
  if (d->loadDicomButton)
  {
    connect(d->loadDicomButton, SIGNAL(clicked()), this, SLOT(onLoadDicomClicked()));
  }
  if (d->loadGuideButton)
  {
    connect(d->loadGuideButton, SIGNAL(clicked()), this, SLOT(onLoadGuideClicked()));
  }

  // 3단계 UI 시그널-슬롯 연결
  if (d->separateFragmentsButton)
  {
    connect(d->separateFragmentsButton, SIGNAL(clicked()), this, SLOT(onSeparateFragmentsClicked()));
  }
  if (d->clearModelsButton)
  {
    connect(d->clearModelsButton, SIGNAL(clicked()), this, SLOT(onClearModelsClicked()));
  }
  if (d->fragmentTableWidget)
  {
    connect(d->fragmentTableWidget, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(onFragmentTableItemChanged(QTableWidgetItem*)));
    connect(d->fragmentTableWidget, SIGNAL(itemSelectionChanged()), this, SLOT(onFragmentTableSelectionChanged()));
  }

  // 4단계 UI 시그널-슬롯 연결
  if (d->guideModelSelector)
  {
    connect(d->guideModelSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onGuideModelChanged(vtkMRMLNode*)));
  }
  if (d->guideVisibilityButton)
  {
    d->guideVisibilityButton->setCheckable(true);
    connect(d->guideVisibilityButton, SIGNAL(toggled(bool)), this, SLOT(onGuideVisibilityToggled(bool)));
  }
  if (d->guideInteractionButton)
  {
    d->guideInteractionButton->setCheckable(true);
    connect(d->guideInteractionButton, SIGNAL(toggled(bool)), this, SLOT(onGuideInteractionToggled(bool)));
  }
  if (d->mirrorGuideButton)
  {
    connect(d->mirrorGuideButton, SIGNAL(clicked()), this, SLOT(onMirrorGuideClicked()));
  }
  if (d->runIcpReductionButton)
  {
    connect(d->runIcpReductionButton, SIGNAL(clicked()), this, SLOT(onRunIcpReductionClicked()));
  }

  // 5단계 UI 시그널-슬롯 연결
  if (d->runSurfaceSnapButton)
  {
    connect(d->runSurfaceSnapButton, SIGNAL(clicked()), this, SLOT(onRunSurfaceSnapClicked()));
  }
  if (d->runLandmarkMatchButton)
  {
    connect(d->runLandmarkMatchButton, SIGNAL(clicked()), this, SLOT(onRunLandmarkMatchClicked()));
  }
  if (d->runMaskMatchButton)
  {
    connect(d->runMaskMatchButton, SIGNAL(clicked()), this, SLOT(onRunMaskMatchClicked()));
  }
  if (d->clearMarkersButton)
  {
    connect(d->clearMarkersButton, SIGNAL(clicked()), this, SLOT(onClearMarkersClicked()));
  }
  if (d->clearMasksButton)
  {
    connect(d->clearMasksButton, SIGNAL(clicked()), this, SLOT(onClearMasksClicked()));
  }
}

//-----------------------------------------------------------------------------
qSlicerFemurFracturePlannerCppModulePlannerDialog::~qSlicerFemurFracturePlannerCppModulePlannerDialog()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);

  this->setMRMLScene(nullptr);

  if (d->ActiveTransformNode)
  {
    this->qvtkDisconnect(d->ActiveTransformNode, vtkCommand::ModifiedEvent, this, SLOT(onActiveTransformNodeModified()));
  }
  d->ActiveTransformNode = nullptr;
  d->ActiveModelNode = nullptr;

  if (d->SegmentEditorWidget)
  {
    d->SegmentEditorWidget->setMRMLSegmentEditorNode(nullptr);
    d->SegmentEditorWidget->setMRMLScene(nullptr);
    delete d->SegmentEditorWidget;
    d->SegmentEditorWidget = nullptr;
  }
  if (d->SegmentEditorNode && this->mrmlScene())
  {
    this->mrmlScene()->RemoveNode(d->SegmentEditorNode);
  }
  d->SegmentEditorNode = nullptr;

  // 다이얼로그와 관련된 모든 등록된 VTK 관찰자(Observer) 일괄 강제 해제
  this->qvtkDisconnectAll();
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);

  this->qMRMLWidget::setMRMLScene(scene);

  if (!scene)
  {
    d->SegmentEditorNode = nullptr;
    if (d->inputVolumeSelector) d->inputVolumeSelector->setMRMLScene(nullptr);
    if (d->activeSegmentationSelector) d->activeSegmentationSelector->setMRMLScene(nullptr);
    if (d->guideModelSelector) d->guideModelSelector->setMRMLScene(nullptr);
    if (d->SegmentEditorWidget) d->SegmentEditorWidget->setMRMLScene(nullptr);
    return;
  }

  // 1. [핵심] 자식 위젯들에 씬을 뿌려 시그널이 트리거되기 전에, 에디터 전용 노드를 미리 확보하여 위젯에 등록 완료
  if (!d->SegmentEditorNode)
  {
    d->SegmentEditorNode = vtkMRMLSegmentEditorNode::SafeDownCast(
      scene->GetFirstNodeByName("FemurFracturePlanner_SegmentEditorNode")
    );
    if (!d->SegmentEditorNode)
    {
      d->SegmentEditorNode = vtkMRMLSegmentEditorNode::SafeDownCast(
        scene->AddNewNodeByClass("vtkMRMLSegmentEditorNode")
      );
      if (d->SegmentEditorNode)
      {
        d->SegmentEditorNode->SetName("FemurFracturePlanner_SegmentEditorNode");
      }
    }
  }

  if (d->SegmentEditorWidget)
  {
    d->SegmentEditorWidget->setMRMLScene(scene);
    if (d->SegmentEditorNode)
    {
      d->SegmentEditorWidget->setMRMLSegmentEditorNode(d->SegmentEditorNode);
    }
  }

  // 2. 기본 세그멘테이션 노드가 없으면 미리 자동 신설하여 준비
  vtkMRMLSegmentationNode* segNode = vtkMRMLSegmentationNode::SafeDownCast(
    scene->GetFirstNodeByClass("vtkMRMLSegmentationNode")
  );
  if (!segNode)
  {
    segNode = vtkMRMLSegmentationNode::SafeDownCast(
      scene->AddNewNodeByClass("vtkMRMLSegmentationNode")
    );
    if (segNode)
    {
      segNode->SetName("Femur_Segmentation");
      segNode->CreateDefaultDisplayNodes();
    }
  }

  // 3. 자식 콤보박스들에 씬 전달 (이 시점에 currentNodeChanged 시그널이 방출되더라도 안전하게 위젯이 준비됨)
  if (d->inputVolumeSelector)
  {
    d->inputVolumeSelector->setMRMLScene(scene);
  }
  if (d->activeSegmentationSelector)
  {
    d->activeSegmentationSelector->setMRMLScene(scene);
    if (segNode)
    {
      d->activeSegmentationSelector->setCurrentNode(segNode);
    }
  }
  if (d->guideModelSelector)
  {
    d->guideModelSelector->setMRMLScene(scene);
  }

  // 4. 최종 동기화 완료 (시그널이 돌지 않았을 수 있는 기본 선택 상태 복원)
  vtkMRMLSegmentationNode* initSeg = d->activeSegmentationSelector ?
    vtkMRMLSegmentationNode::SafeDownCast(d->activeSegmentationSelector->currentNode()) : nullptr;
  vtkMRMLScalarVolumeNode* initVol = d->inputVolumeSelector ?
    vtkMRMLScalarVolumeNode::SafeDownCast(d->inputVolumeSelector->currentNode()) : nullptr;

  if (d->SegmentEditorWidget)
  {
    if (initSeg)
    {
      d->SegmentEditorWidget->setSegmentationNode(initSeg);
    }
    if (initVol)
    {
      d->SegmentEditorWidget->setSourceVolumeNode(initVol);
    }

    if (initSeg && initVol)
    {
      initSeg->SetReferenceImageGeometryParameterFromVolumeNode(initVol);
      initSeg->Modified();
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::closeEvent(QCloseEvent* event)
{
  event->ignore();
  this->hide();
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::setLogic(vtkSlicerFemurFracturePlannerCppLogic* logic)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  d->Logic = logic;
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onVolumeRenderingToggled(bool checked)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->Logic || !d->inputVolumeSelector || !d->thresholdSlider)
  {
    return;
  }
  vtkMRMLScalarVolumeNode* inputNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->inputVolumeSelector->currentNode());
  if (!inputNode)
  {
    return;
  }
  if (checked)
  {
    d->Logic->ShowVolumeRendering(inputNode, d->thresholdSlider->value());
  }
  else
  {
    d->Logic->HideVolumeRendering(inputNode);
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onThresholdChanged(double value)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->Logic || !d->inputVolumeSelector || !d->volumeRenderingVisibilityButton)
  {
    return;
  }
  vtkMRMLScalarVolumeNode* inputNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->inputVolumeSelector->currentNode());
  if (inputNode && d->volumeRenderingVisibilityButton->isChecked())
  {
    d->Logic->AdjustVolumeRenderingThreshold(inputNode, value);
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onRotationAngleChanged(double angle)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->Logic || !d->inputVolumeSelector || !d->guideModelSelector)
  {
    return;
  }
  vtkMRMLScalarVolumeNode* inputNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->inputVolumeSelector->currentNode());
  vtkMRMLModelNode* guideNode = vtkMRMLModelNode::SafeDownCast(d->guideModelSelector->currentNode());

  const char* axis = "Z";
  if (d->rotationXRadio && d->rotationXRadio->isChecked())
  {
    axis = "X";
  }
  else if (d->rotationYRadio && d->rotationYRadio->isChecked())
  {
    axis = "Y";
  }

  d->Logic->RotateVolume(inputNode, axis, angle, guideNode);
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onRotationAxisChanged()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (d->rotationAngleSlider)
  {
    this->onRotationAngleChanged(d->rotationAngleSlider->value());
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onResetRotationClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (d->rotationAngleSlider)
  {
    d->rotationAngleSlider->setValue(0.0);
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onInvertIntensityClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->Logic || !d->inputVolumeSelector)
  {
    return;
  }
  vtkMRMLScalarVolumeNode* inputNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->inputVolumeSelector->currentNode());
  if (!inputNode)
  {
    return;
  }
  vtkMRMLScalarVolumeNode* invertedNode = d->Logic->InvertVolumeIntensity(inputNode);
  if (invertedNode)
  {
    d->inputVolumeSelector->setCurrentNode(invertedNode);
    if (d->volumeVisibilityButton)
    {
      d->volumeVisibilityButton->setChecked(true);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onEdgeDetectionClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->Logic || !d->inputVolumeSelector || !d->thresholdSlider)
  {
    return;
  }
  vtkMRMLScalarVolumeNode* inputNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->inputVolumeSelector->currentNode());
  if (!inputNode)
  {
    return;
  }
  vtkMRMLScalarVolumeNode* edgeNode = d->Logic->DetectVolumeEdgesMasked(inputNode, d->thresholdSlider->value());
  if (edgeNode)
  {
    d->inputVolumeSelector->setCurrentNode(edgeNode);
    if (d->volumeVisibilityButton)
    {
      d->volumeVisibilityButton->setChecked(true);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onLoadVolumeClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  QString filePath = QFileDialog::getOpenFileName(
    this,
    "CT 볼륨 파일 선택",
    "",
    "Volume Files (*.nrrd *.nhdr *.mha *.mhd *.nii *.nii.gz *.hdr *.img *.img.gz)"
  );
  if (filePath.isEmpty())
  {
    return;
  }

  qSlicerApplication* app = qSlicerApplication::application();
  if (app && app->coreIOManager())
  {
    vtkNew<vtkCollection> loadedNodes;
    bool success = app->coreIOManager()->loadNodes(
      "VolumeArchetypeStorageNode",
      QVariantMap{{"fileName", filePath}},
      loadedNodes.GetPointer()
    );
    if (success && loadedNodes->GetNumberOfItems() > 0)
    {
      vtkMRMLScalarVolumeNode* loadedNode = vtkMRMLScalarVolumeNode::SafeDownCast(loadedNodes->GetItemAsObject(0));
      if (loadedNode && d->inputVolumeSelector)
      {
        d->inputVolumeSelector->setCurrentNode(loadedNode);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onLoadDicomClicked()
{
  qSlicerApplication* app = qSlicerApplication::application();
  if (!app)
  {
    return;
  }
  
  qSlicerMainWindow* mainWindow = qobject_cast<qSlicerMainWindow*>(app->mainWindow());
  if (mainWindow)
  {
    QAction* dicomAction = mainWindow->findChild<QAction*>("ActionOpenDICOMBrowser");
    if (dicomAction)
    {
      dicomAction->trigger();
      return;
    }
    
    qSlicerModuleSelectorToolBar* moduleSelector = mainWindow->moduleSelector();
    if (moduleSelector)
    {
      moduleSelector->selectModule("DICOM");
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onLoadGuideClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  QString filePath = QFileDialog::getOpenFileName(
    this,
    "가이드 모델 파일 선택",
    "",
    "Model Files (*.vtk *.vtp *.stl *.ply *.obj)"
  );
  if (filePath.isEmpty())
  {
    return;
  }
  qDebug() << "onLoadGuideClicked: Attempting to load guide model from" << filePath;

  qSlicerApplication* app = qSlicerApplication::application();
  if (app && app->coreIOManager())
  {
    vtkNew<vtkCollection> loadedNodes;
    // 1차 시도: "ModelFile" 타입으로 로드 시도
    bool success = app->coreIOManager()->loadNodes(
      "ModelFile",
      QVariantMap{{"fileName", filePath}},
      loadedNodes.GetPointer()
    );

    // 2차 시도: 만약 실패했다면 "Model" 타입으로 로드 시도
    if (!success || loadedNodes->GetNumberOfItems() == 0)
    {
      qDebug() << "onLoadGuideClicked: ModelFile type load failed, trying Model type...";
      loadedNodes->InitTraversal();
      success = app->coreIOManager()->loadNodes(
        "Model",
        QVariantMap{{"fileName", filePath}},
        loadedNodes.GetPointer()
      );
    }

    qDebug() << "onLoadGuideClicked: Load success =" << success << ", loaded items count =" << loadedNodes->GetNumberOfItems();

    if (success && loadedNodes->GetNumberOfItems() > 0)
    {
      vtkMRMLModelNode* loadedNode = vtkMRMLModelNode::SafeDownCast(loadedNodes->GetItemAsObject(0));
      if (loadedNode)
      {
        qDebug() << "onLoadGuideClicked: Successfully loaded node:" << loadedNode->GetName();
        if (d->guideModelSelector)
        {
          d->guideModelSelector->setCurrentNode(loadedNode);
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onVolumeVisibilityToggled(bool checked)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->Logic || !d->inputVolumeSelector || !d->volumeVisibilityButton)
  {
    return;
  }
  vtkMRMLScalarVolumeNode* inputNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->inputVolumeSelector->currentNode());
  if (!inputNode)
  {
    if (checked)
    {
      d->volumeVisibilityButton->setChecked(false);
    }
    return;
  }

  d->Logic->SetVolumeVisibility(inputNode, checked);

  if (checked)
  {
    d->volumeVisibilityButton->setText("숨기기");
    d->volumeVisibilityButton->setToolTip("뷰어에서 볼륨을 숨깁니다.");
  }
  else
  {
    d->volumeVisibilityButton->setText("보기");
    d->volumeVisibilityButton->setToolTip("뷰어에서 볼륨을 표시합니다.");
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onInputVolumeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  qDebug() << "onInputVolumeChanged called, Node:" << (node ? node->GetName() : "nullptr");
  if (!node || !d->volumeVisibilityButton)
  {
    return;
  }

  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);

  vtkMRMLSegmentationNode* segNode = nullptr;
  if (d->activeSegmentationSelector)
  {
    segNode = vtkMRMLSegmentationNode::SafeDownCast(
      d->activeSegmentationSelector->currentNode()
    );
  }

  if (d->SegmentEditorWidget)
  {
    if (segNode)
    {
      d->SegmentEditorWidget->setSegmentationNode(segNode);
    }
    d->SegmentEditorWidget->setSourceVolumeNode(volumeNode);
  }

  if (segNode && volumeNode)
  {
    segNode->SetReferenceImageGeometryParameterFromVolumeNode(volumeNode);
    segNode->Modified();
  }

  if (d->rotationAngleSlider)
  {
    d->rotationAngleSlider->blockSignals(true);
    d->rotationAngleSlider->setValue(0.0);
    d->rotationAngleSlider->blockSignals(false);
  }

  // 볼륨 변경 시 "보기" 기능이 이미 활성화되어 있으면 즉각 새 볼륨을 slice viewer에 설정하고,
  // 비활성화 상태라면 setChecked(true)를 트리거하여 자동으로 보이게 함
  if (d->volumeVisibilityButton->isChecked())
  {
    if (d->Logic && volumeNode)
    {
      d->Logic->SetVolumeVisibility(volumeNode, true);
    }
  }
  else
  {
    d->volumeVisibilityButton->setChecked(true);
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onSeparateFragmentsClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->Logic || !this->mrmlScene())
  {
    return;
  }

  vtkMRMLNode* segNodeObj = d->SegmentEditorWidget ? d->SegmentEditorWidget->segmentationNode() : nullptr;
  vtkMRMLSegmentationNode* segNode = vtkMRMLSegmentationNode::SafeDownCast(segNodeObj);
  if (!segNode)
  {
    QMessageBox::warning(this, "경고", "먼저 입력 세그멘테이션 노드를 선택하세요.");
    return;
  }

  vtkMRMLScalarVolumeNode* inputVolume = vtkMRMLScalarVolumeNode::SafeDownCast(
    d->inputVolumeSelector ? d->inputVolumeSelector->currentNode() : nullptr
  );

  int separatedCount = d->Logic->SeparateBoneFragments(segNode, inputVolume);
  if (separatedCount > 0)
  {
    QMessageBox::information(this, "성공", QString("%1개의 독립 골편으로 분리했습니다.").arg(separatedCount));

    if (d->volumeRenderingVisibilityButton && d->volumeRenderingVisibilityButton->isChecked())
    {
      d->volumeRenderingVisibilityButton->setChecked(false);
    }

    this->updateFragmentTable();
  }
  else
  {
    QMessageBox::warning(this, "오류", "유효한 골편을 추출하지 못했습니다. 세그멘테이션에 실제 영역이 있는지 확인하세요.");
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onClearModelsClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!this->mrmlScene())
  {
    return;
  }

  std::vector<vtkMRMLNode*> nodesToDelete;
  std::vector<vtkMRMLNode*> transformsToDelete;

  vtkMRMLNode* guideNode = d->guideModelSelector ? d->guideModelSelector->currentNode() : nullptr;
  if (guideNode)
  {
    nodesToDelete.push_back(guideNode);
    vtkMRMLTransformableNode* transformableGuide = vtkMRMLTransformableNode::SafeDownCast(guideNode);
    vtkMRMLTransformNode* transformNode = transformableGuide ? transformableGuide->GetParentTransformNode() : nullptr;
    if (transformNode && transformNode->IsA("vtkMRMLLinearTransformNode"))
    {
      transformsToDelete.push_back(transformNode);
    }

    std::string guideName = guideNode->GetName() ? guideNode->GetName() : "";
    if (guideName.length() > 9 && guideName.substr(guideName.length() - 9) == "_Mirrored")
    {
      std::string originalName = guideName.substr(0, guideName.length() - 9);
      vtkMRMLNode* originalNode = this->mrmlScene()->GetFirstNodeByName(originalName.c_str());
      if (originalNode && originalNode->IsA("vtkMRMLModelNode"))
      {
        if (std::find(nodesToDelete.begin(), nodesToDelete.end(), originalNode) == nodesToDelete.end())
        {
          nodesToDelete.push_back(originalNode);
          vtkMRMLTransformableNode* transformableOrig = vtkMRMLTransformableNode::SafeDownCast(originalNode);
          vtkMRMLTransformNode* origTransform = transformableOrig ? transformableOrig->GetParentTransformNode() : nullptr;
          if (origTransform && origTransform->IsA("vtkMRMLLinearTransformNode"))
          {
            transformsToDelete.push_back(origTransform);
          }
        }
      }
    }
  }

  std::vector<vtkMRMLNode*> allModelNodes;
  this->mrmlScene()->GetNodesByClass("vtkMRMLModelNode", allModelNodes);
  for (vtkMRMLNode* node : allModelNodes)
  {
    std::string nodeName = node->GetName() ? node->GetName() : "";
    if (nodeName.rfind("Femur_Fragment", 0) == 0 || (nodeName.length() > 9 && nodeName.substr(nodeName.length() - 9) == "_Mirrored"))
    {
      if (std::find(nodesToDelete.begin(), nodesToDelete.end(), node) == nodesToDelete.end())
      {
        nodesToDelete.push_back(node);
      }

      vtkMRMLTransformableNode* transformableNode = vtkMRMLTransformableNode::SafeDownCast(node);
      vtkMRMLTransformNode* transformNode = transformableNode ? transformableNode->GetParentTransformNode() : nullptr;
      if (transformNode && transformNode->IsA("vtkMRMLLinearTransformNode"))
      {
        std::string tName = transformNode->GetName() ? transformNode->GetName() : "";
        if (tName.rfind("Femur_Fragment", 0) == 0 && 
            (tName.find("_Transform") != std::string::npos || tName.find("_RotationTransform") != std::string::npos))
        {
          if (std::find(transformsToDelete.begin(), transformsToDelete.end(), transformNode) == transformsToDelete.end())
          {
            transformsToDelete.push_back(transformNode);
          }
        }
      }
    }
  }

  std::vector<vtkMRMLNode*> allTransformNodes;
  this->mrmlScene()->GetNodesByClass("vtkMRMLLinearTransformNode", allTransformNodes);
  for (vtkMRMLNode* tNode : allTransformNodes)
  {
    std::string tName = tNode->GetName() ? tNode->GetName() : "";
    if (tName.rfind("Femur_Fragment", 0) == 0 && 
        (tName.find("_Transform") != std::string::npos || tName.find("_RotationTransform") != std::string::npos))
    {
      if (std::find(transformsToDelete.begin(), transformsToDelete.end(), tNode) == transformsToDelete.end())
      {
        transformsToDelete.push_back(tNode);
      }
    }
  }

  if (nodesToDelete.empty() && transformsToDelete.empty())
  {
    QMessageBox::information(this, "정보", "삭제할 골편, 미러링 모델 또는 변환 노드가 없습니다.");
    return;
  }

  QMessageBox::StandardButton reply = QMessageBox::question(
    this,
    "전체 삭제 확인",
    QString("장면에서 %1개의 골편/가이드 모델 및 %2개의 연관 변환 노드를 정말 삭제하시겠습니까?\n(가이드 모델 선택도 초기화됩니다.)")
      .arg(nodesToDelete.size())
      .arg(transformsToDelete.size()),
    QMessageBox::Yes | QMessageBox::No
  );

  if (reply != QMessageBox::Yes)
  {
    return;
  }

  if (d->ActiveTransformNode)
  {
    this->qvtkDisconnect(d->ActiveTransformNode, vtkCommand::ModifiedEvent, this, SLOT(onActiveTransformNodeModified()));
  }
  d->ActiveTransformNode = nullptr;
  d->ActiveModelNode = nullptr;

  for (vtkMRMLNode* node : nodesToDelete)
  {
    this->mrmlScene()->RemoveNode(node);
  }
  for (vtkMRMLNode* tNode : transformsToDelete)
  {
    this->mrmlScene()->RemoveNode(tNode);
  }

  if (d->guideModelSelector)
  {
    d->guideModelSelector->setCurrentNode(nullptr);
  }

  this->updateFragmentTable();
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::updateFragmentTable()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->fragmentTableWidget || !this->mrmlScene())
  {
    return;
  }

  d->fragmentTableWidget->blockSignals(true);
  d->fragmentTableWidget->clear();

  d->fragmentTableWidget->setColumnCount(5);
  d->fragmentTableWidget->setHorizontalHeaderLabels(QStringList() << "이름" << "표시" << "이동" << "색상" << "삭제");
  
  QHeaderView* header = d->fragmentTableWidget->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(4, QHeaderView::ResizeToContents);

  std::vector<vtkMRMLNode*> fragmentNodes;
  std::vector<vtkMRMLNode*> allModelNodes;
  this->mrmlScene()->GetNodesByClass("vtkMRMLModelNode", allModelNodes);
  for (vtkMRMLNode* node : allModelNodes)
  {
    std::string name = node->GetName() ? node->GetName() : "";
    if (name.rfind("Femur_Fragment", 0) == 0)
    {
      fragmentNodes.push_back(node);
    }
  }

  d->fragmentTableWidget->setRowCount(static_cast<int>(fragmentNodes.size()));

  for (size_t row = 0; row < fragmentNodes.size(); ++row)
  {
    vtkMRMLModelNode* node = vtkMRMLModelNode::SafeDownCast(fragmentNodes[row]);
    if (!node) continue;

    QTableWidgetItem* nameItem = new QTableWidgetItem(node->GetName());
    nameItem->setData(Qt::UserRole, QString(node->GetID()));
    d->fragmentTableWidget->setItem(static_cast<int>(row), 0, nameItem);

    QCheckBox* showCheckBox = new QCheckBox();
    vtkMRMLModelDisplayNode* dispNode = vtkMRMLModelDisplayNode::SafeDownCast(node->GetDisplayNode());
    showCheckBox->setChecked(dispNode ? dispNode->GetVisibility() : false);
    
    connect(showCheckBox, &QCheckBox::toggled, this, [this, node](bool checked) {
      this->onFragmentVisibilityToggled(node, checked);
    });

    QWidget* showContainer = new QWidget();
    QHBoxLayout* showLayout = new QHBoxLayout(showContainer);
    showLayout->addWidget(showCheckBox);
    showLayout->setAlignment(Qt::AlignCenter);
    showLayout->setContentsMargins(0, 0, 0, 0);
    d->fragmentTableWidget->setCellWidget(static_cast<int>(row), 1, showContainer);

    QCheckBox* moveCheckBox = new QCheckBox();
    vtkMRMLTransformNode* transformNode = node->GetParentTransformNode();
    if (!transformNode || !transformNode->IsA("vtkMRMLLinearTransformNode"))
    {
      vtkMRMLLinearTransformNode* newTrans = vtkMRMLLinearTransformNode::SafeDownCast(
        this->mrmlScene()->AddNewNodeByClass("vtkMRMLLinearTransformNode")
      );
      if (newTrans)
      {
        std::string tName = std::string(node->GetName()) + "_Transform";
        newTrans->SetName(tName.c_str());
        node->SetAndObserveTransformNodeID(newTrans->GetID());
        transformNode = newTrans;
      }
    }

    if (transformNode)
    {
      transformNode->CreateDefaultDisplayNodes();
    }
    vtkMRMLTransformDisplayNode* transDispNode = transformNode ? 
      vtkMRMLTransformDisplayNode::SafeDownCast(transformNode->GetDisplayNode()) : nullptr;
    
    moveCheckBox->setChecked(transDispNode ? transDispNode->GetEditorVisibility() : false);
    
    connect(moveCheckBox, &QCheckBox::toggled, this, [this, node](bool checked) {
      this->onFragmentInteractionToggled(node, checked);
    });

    QWidget* moveContainer = new QWidget();
    QHBoxLayout* moveLayout = new QHBoxLayout(moveContainer);
    moveLayout->addWidget(moveCheckBox);
    moveLayout->setAlignment(Qt::AlignCenter);
    moveLayout->setContentsMargins(0, 0, 0, 0);
    d->fragmentTableWidget->setCellWidget(static_cast<int>(row), 2, moveContainer);

    QPushButton* colorButton = new QPushButton();
    colorButton->setFixedWidth(40);
    colorButton->setFixedHeight(20);
    
    double* rgb = dispNode ? dispNode->GetColor() : nullptr;
    if (rgb)
    {
      int r = static_cast<int>(rgb[0] * 255);
      int g = static_cast<int>(rgb[1] * 255);
      int b = static_cast<int>(rgb[2] * 255);
      colorButton->setStyleSheet(
        QString("background-color: rgb(%1, %2, %3); border: 1px solid gray; border-radius: 3px;").arg(r).arg(g).arg(b)
      );
    }

    connect(colorButton, &QPushButton::clicked, this, [this, node, colorButton]() {
      this->onFragmentColorClicked(node, colorButton);
    });

    QWidget* colorContainer = new QWidget();
    QHBoxLayout* colorLayout = new QHBoxLayout(colorContainer);
    colorLayout->addWidget(colorButton);
    colorLayout->setAlignment(Qt::AlignCenter);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    d->fragmentTableWidget->setCellWidget(static_cast<int>(row), 3, colorContainer);

    QPushButton* deleteButton = new QPushButton("X");
    deleteButton->setFixedWidth(30);
    deleteButton->setFixedHeight(22);

    connect(deleteButton, &QPushButton::clicked, this, [this, node]() {
      this->onFragmentDeleteClicked(node);
    });

    QWidget* deleteContainer = new QWidget();
    QHBoxLayout* deleteLayout = new QHBoxLayout(deleteContainer);
    deleteLayout->addWidget(deleteButton);
    deleteLayout->setAlignment(Qt::AlignCenter);
    deleteLayout->setContentsMargins(0, 0, 0, 0);
    d->fragmentTableWidget->setCellWidget(static_cast<int>(row), 4, deleteContainer);
  }

  d->fragmentTableWidget->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onFragmentTableItemChanged(QTableWidgetItem* item)
{
  if (!item || item->column() != 0 || !this->mrmlScene())
  {
    return;
  }

  QString nodeId = item->data(Qt::UserRole).toString();
  if (nodeId.isEmpty())
  {
    return;
  }

  vtkMRMLNode* node = this->mrmlScene()->GetNodeByID(nodeId.toUtf8().constData());
  if (node && QString(node->GetName()) != item->text())
  {
    node->SetName(item->text().toUtf8().constData());
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onFragmentVisibilityToggled(vtkMRMLNode* node, bool checked)
{
  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);
  if (!modelNode) return;
  
  vtkMRMLModelDisplayNode* dispNode = vtkMRMLModelDisplayNode::SafeDownCast(modelNode->GetDisplayNode());
  if (dispNode)
  {
    dispNode->SetVisibility(checked);
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onFragmentColorClicked(vtkMRMLNode* node, QPushButton* button)
{
  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);
  if (!modelNode || !button) return;

  vtkMRMLModelDisplayNode* dispNode = vtkMRMLModelDisplayNode::SafeDownCast(modelNode->GetDisplayNode());
  if (!dispNode) return;

  double* rgb = dispNode->GetColor();
  QColor initialColor = QColor::fromRgbF(rgb[0], rgb[1], rgb[2]);

  QColor color = QColorDialog::getColor(initialColor, this, "골편 색상 선택");
  if (color.isValid())
  {
    dispNode->SetColor(color.redF(), color.greenF(), color.blueF());
    button->setStyleSheet(
      QString("background-color: rgb(%1, %2, %3); border: 1px solid gray; border-radius: 3px;")
        .arg(color.red()).arg(color.green()).arg(color.blue())
    );
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onFragmentDeleteClicked(vtkMRMLNode* node)
{
  if (!node || !this->mrmlScene()) return;

  vtkMRMLTransformableNode* transformableNode = vtkMRMLTransformableNode::SafeDownCast(node);
  vtkMRMLTransformNode* transformNode = transformableNode ? transformableNode->GetParentTransformNode() : nullptr;
  
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (node == d->ActiveModelNode)
  {
    if (d->ActiveTransformNode)
    {
      this->qvtkDisconnect(d->ActiveTransformNode, vtkCommand::ModifiedEvent, this, SLOT(onActiveTransformNodeModified()));
    }
    d->ActiveTransformNode = nullptr;
    d->ActiveModelNode = nullptr;
  }

  this->mrmlScene()->RemoveNode(node);

  if (transformNode && transformNode->IsA("vtkMRMLLinearTransformNode"))
  {
    std::string tName = transformNode->GetName() ? transformNode->GetName() : "";
    if (tName.rfind("Femur_Fragment", 0) == 0 && 
        (tName.find("_Transform") != std::string::npos || tName.find("_RotationTransform") != std::string::npos))
    {
      this->mrmlScene()->RemoveNode(transformNode);
    }
  }

  this->updateFragmentTable();
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onFragmentInteractionToggled(vtkMRMLNode* node, bool checked)
{
  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);
  if (!modelNode || !this->mrmlScene()) return;

  vtkMRMLTransformNode* transformNode = modelNode->GetParentTransformNode();
  vtkMRMLTransformNode* originalParentTransform = nullptr;

  if (transformNode && transformNode->GetName())
  {
    std::string tName = transformNode->GetName();
    if (tName.find("_Transform") == std::string::npos)
    {
      originalParentTransform = transformNode;
      transformNode = nullptr;
    }
  }

  if (!transformNode || !transformNode->IsA("vtkMRMLLinearTransformNode"))
  {
    vtkMRMLLinearTransformNode* newTrans = vtkMRMLLinearTransformNode::SafeDownCast(
      this->mrmlScene()->AddNewNodeByClass("vtkMRMLLinearTransformNode")
    );
    if (newTrans)
    {
      std::string tName = std::string(modelNode->GetName()) + "_Transform";
      newTrans->SetName(tName.c_str());
      if (originalParentTransform)
      {
        newTrans->SetAndObserveTransformNodeID(originalParentTransform->GetID());
      }
      modelNode->SetAndObserveTransformNodeID(newTrans->GetID());
      transformNode = newTrans;
    }
  }

  if (transformNode)
  {
    transformNode->CreateDefaultDisplayNodes();
    vtkMRMLTransformDisplayNode* transDispNode = 
      vtkMRMLTransformDisplayNode::SafeDownCast(transformNode->GetDisplayNode());
    if (transDispNode)
    {
      transDispNode->SetEditorVisibility(checked);
    }
  }

  this->onFragmentTableSelectionChanged();
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onFragmentTableSelectionChanged()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->fragmentTableWidget || !this->mrmlScene())
  {
    return;
  }

  if (d->ActiveTransformNode)
  {
    this->qvtkDisconnect(d->ActiveTransformNode, vtkCommand::ModifiedEvent, this, SLOT(onActiveTransformNodeModified()));
  }
  d->ActiveTransformNode = nullptr;
  d->ActiveModelNode = nullptr;

  if (d->selectedNameValue) d->selectedNameValue->setText("없음 (표에서 행을 선택하세요)");
  if (d->positionValue) d->positionValue->setText("N/A");
  if (d->rotationValue) d->rotationValue->setText("N/A");

  QList<QTableWidgetSelectionRange> selectedRanges = d->fragmentTableWidget->selectedRanges();
  if (selectedRanges.isEmpty())
  {
    return;
  }

  int row = selectedRanges[0].topRow();
  QTableWidgetItem* nameItem = d->fragmentTableWidget->item(row, 0);
  if (!nameItem)
  {
    return;
  }

  QString nodeId = nameItem->data(Qt::UserRole).toString();
  if (nodeId.isEmpty())
  {
    return;
  }

  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(nodeId.toUtf8().constData())
  );
  if (!modelNode)
  {
    return;
  }

  d->ActiveModelNode = modelNode;
  if (d->selectedNameValue)
  {
    d->selectedNameValue->setText(modelNode->GetName());
  }

  vtkMRMLTransformableNode* transformableNode = vtkMRMLTransformableNode::SafeDownCast(modelNode);
  vtkMRMLTransformNode* transformNode = transformableNode ? transformableNode->GetParentTransformNode() : nullptr;
  if (!transformNode || !transformNode->IsA("vtkMRMLLinearTransformNode"))
  {
    vtkPolyData* poly = modelNode->GetPolyData();
    if (poly)
    {
      double bounds[6] = {0.0};
      poly->GetBounds(bounds);
      double cx = (bounds[0] + bounds[1]) / 2.0;
      double cy = (bounds[2] + bounds[3]) / 2.0;
      double cz = (bounds[4] + bounds[5]) / 2.0;
      if (d->positionValue)
      {
        d->positionValue->setText(QString("X: %1, Y: %2, Z: %3").arg(cx, 0, 'f', 2).arg(cy, 0, 'f', 2).arg(cz, 0, 'f', 2));
      }
      if (d->rotationValue)
      {
        d->rotationValue->setText("R: 0.00, P: 0.00, Y: 0.00");
      }
    }
    return;
  }

  d->ActiveTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(transformNode);
  if (d->ActiveTransformNode)
  {
    this->qvtkConnect(d->ActiveTransformNode, vtkCommand::ModifiedEvent, this, SLOT(onActiveTransformNodeModified()));
    this->onActiveTransformNodeModified();
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onActiveTransformNodeModified()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->ActiveTransformNode)
  {
    return;
  }

  vtkNew<vtkMatrix4x4> worldMatrix;
  d->ActiveTransformNode->GetMatrixTransformToWorld(worldMatrix);

  vtkNew<vtkTransform> transform;
  transform->SetMatrix(worldMatrix);
  
  double orientation[3] = {0.0};
  transform->GetOrientation(orientation);

  double position[3] = {0.0};

  if (d->ActiveModelNode && d->ActiveModelNode->GetPolyData())
  {
    vtkPolyData* poly = d->ActiveModelNode->GetPolyData();
    double bounds[6] = {0.0};
    poly->GetBounds(bounds);
    double cx = (bounds[0] + bounds[1]) / 2.0;
    double cy = (bounds[2] + bounds[3]) / 2.0;
    double cz = (bounds[4] + bounds[5]) / 2.0;

    double point_local[4] = {cx, cy, cz, 1.0};
    double point_world[4] = {0.0, 0.0, 0.0, 1.0};
    worldMatrix->MultiplyPoint(point_local, point_world);
    
    position[0] = point_world[0];
    position[1] = point_world[1];
    position[2] = point_world[2];
  }
  else
  {
    transform->GetPosition(position);
  }

  if (d->positionValue)
  {
    d->positionValue->setText(QString("X: %1, Y: %2, Z: %3").arg(position[0], 0, 'f', 2).arg(position[1], 0, 'f', 2).arg(position[2], 0, 'f', 2));
  }
  if (d->rotationValue)
  {
    d->rotationValue->setText(QString("R: %1, P: %2, Y: %3").arg(orientation[0], 0, 'f', 2).arg(orientation[1], 0, 'f', 2).arg(orientation[2], 0, 'f', 2));
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onSegmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  qDebug() << "onSegmentationNodeChanged called, Node:" << (node ? node->GetName() : "nullptr");
  if (!d->SegmentEditorWidget)
  {
    return;
  }

  if (d->SegmentEditorNode && !d->SegmentEditorWidget->mrmlSegmentEditorNode())
  {
    d->SegmentEditorWidget->setMRMLSegmentEditorNode(d->SegmentEditorNode);
  }

  vtkMRMLSegmentationNode* segNode = vtkMRMLSegmentationNode::SafeDownCast(node);

  vtkMRMLScalarVolumeNode* volumeNode = nullptr;
  if (d->inputVolumeSelector)
  {
    volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
      d->inputVolumeSelector->currentNode()
    );
  }

  if (segNode)
  {
    d->SegmentEditorWidget->setSegmentationNode(segNode);
  }
  if (volumeNode)
  {
    d->SegmentEditorWidget->setSourceVolumeNode(volumeNode);
  }

  if (segNode && volumeNode)
  {
    segNode->SetReferenceImageGeometryParameterFromVolumeNode(volumeNode);
    segNode->Modified();
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onRotate90Clicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->rotationAngleSlider)
  {
    return;
  }
  double currentAngle = d->rotationAngleSlider->value();
  double newAngle = currentAngle + 90.0;
  if (newAngle > 180.0)
  {
    newAngle -= 360.0;
  }
  d->rotationAngleSlider->setValue(newAngle);
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onGuideModelChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  vtkMRMLModelNode* guideNode = vtkMRMLModelNode::SafeDownCast(node);

  if (d->guideVisibilityButton)
  {
    bool visible = false;
    if (guideNode && guideNode->GetDisplayNode())
    {
      visible = guideNode->GetDisplayNode()->GetVisibility() != 0;
    }
    d->guideVisibilityButton->blockSignals(true);
    d->guideVisibilityButton->setChecked(visible);
    d->guideVisibilityButton->blockSignals(false);
  }

  if (d->guideInteractionButton)
  {
    bool interaction = false;
    if (guideNode)
    {
      vtkMRMLTransformNode* tNode = guideNode->GetParentTransformNode();
      if (tNode && tNode->IsA("vtkMRMLLinearTransformNode"))
      {
        interaction = tNode->GetDisplayNode() && tNode->GetDisplayNode()->GetVisibility() != 0;
      }
    }
    d->guideInteractionButton->blockSignals(true);
    d->guideInteractionButton->setChecked(interaction);
    d->guideInteractionButton->blockSignals(false);
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onGuideVisibilityToggled(bool checked)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->guideModelSelector) return;

  vtkMRMLModelNode* guideNode = vtkMRMLModelNode::SafeDownCast(d->guideModelSelector->currentNode());
  if (!guideNode)
  {
    if (d->guideVisibilityButton)
    {
      d->guideVisibilityButton->blockSignals(true);
      d->guideVisibilityButton->setChecked(false);
      d->guideVisibilityButton->blockSignals(false);
    }
    return;
  }

  guideNode->CreateDefaultDisplayNodes();
  if (guideNode->GetDisplayNode())
  {
    guideNode->GetDisplayNode()->SetVisibility(checked ? 1 : 0);
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onGuideInteractionToggled(bool checked)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->guideModelSelector || !this->mrmlScene()) return;

  vtkMRMLModelNode* guideNode = vtkMRMLModelNode::SafeDownCast(d->guideModelSelector->currentNode());
  if (!guideNode)
  {
    if (d->guideInteractionButton)
    {
      d->guideInteractionButton->blockSignals(true);
      d->guideInteractionButton->setChecked(false);
      d->guideInteractionButton->blockSignals(false);
    }
    return;
  }

  vtkMRMLTransformNode* transformNode = guideNode->GetParentTransformNode();
  if (checked)
  {
    if (!transformNode || !transformNode->IsA("vtkMRMLLinearTransformNode"))
    {
      vtkMRMLLinearTransformNode* newTrans = vtkMRMLLinearTransformNode::SafeDownCast(
        this->mrmlScene()->AddNewNodeByClass("vtkMRMLLinearTransformNode")
      );
      if (newTrans)
      {
        std::string tName = std::string(guideNode->GetName() ? guideNode->GetName() : "Guide") + "_Transform";
        newTrans->SetName(tName.c_str());
        guideNode->SetAndObserveTransformNodeID(newTrans->GetID());
        transformNode = newTrans;
      }
    }
    if (transformNode)
    {
      transformNode->CreateDefaultDisplayNodes();
      vtkMRMLTransformDisplayNode* disp = vtkMRMLTransformDisplayNode::SafeDownCast(transformNode->GetDisplayNode());
      if (disp)
      {
        disp->SetEditorVisibility(1);
      }
    }
  }
  else
  {
    if (transformNode)
    {
      vtkMRMLTransformDisplayNode* disp = vtkMRMLTransformDisplayNode::SafeDownCast(transformNode->GetDisplayNode());
      if (disp)
      {
        disp->SetEditorVisibility(0);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onMirrorGuideClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->Logic || !d->guideModelSelector) return;

  vtkMRMLModelNode* guideNode = vtkMRMLModelNode::SafeDownCast(d->guideModelSelector->currentNode());
  if (!guideNode)
  {
    QMessageBox::warning(this, "경고", "미러링할 원본 가이드 모델을 먼저 선택하세요.");
    return;
  }

  vtkMRMLModelNode* mirrored = d->Logic->MirrorModel(guideNode);
  if (mirrored)
  {
    d->guideModelSelector->setCurrentNode(mirrored);
    if (d->guideVisibilityButton)
    {
      d->guideVisibilityButton->setChecked(true);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onRunIcpReductionClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->Logic || !d->guideModelSelector || !this->mrmlScene()) return;

  vtkMRMLModelNode* guideNode = vtkMRMLModelNode::SafeDownCast(d->guideModelSelector->currentNode());
  if (!guideNode)
  {
    QMessageBox::warning(this, "경고", "자동 정합의 기준이 될 가이드 모델을 먼저 선택하세요.");
    return;
  }

  std::vector<vtkMRMLModelNode*> fragmentNodes;
  std::vector<vtkMRMLNode*> modelNodes;
  this->mrmlScene()->GetNodesByClass("vtkMRMLModelNode", modelNodes);
  for (vtkMRMLNode* node : modelNodes)
  {
    vtkMRMLModelNode* mNode = vtkMRMLModelNode::SafeDownCast(node);
    if (mNode && mNode->GetName())
    {
      std::string name = mNode->GetName();
      if (name.rfind("Femur_Fragment", 0) == 0)
      {
        fragmentNodes.push_back(mNode);
      }
    }
  }

  if (fragmentNodes.empty())
  {
    QMessageBox::warning(this, "경고", "씬에서 골편 모델(Femur_Fragment_*)을 찾을 수 없습니다.");
    return;
  }

  double meanDistance = 0.0;
  int iterations = 0;
  bool success = d->Logic->RunIcpRegistration(fragmentNodes, guideNode, meanDistance, iterations);

  if (success)
  {
    QString msg = QString("병합 ICP 자동 정합 완료!\n평균 정합 오차: %1 mm\n반복 횟수: %2 회")
      .arg(meanDistance, 0, 'f', 4)
      .arg(iterations);
    QMessageBox::information(this, "정합 성공", msg);

    this->updateFragmentTable();
  }
  else
  {
    QMessageBox::critical(this, "정합 실패", "ICP 정합 연산 수행 도중 오류가 발생했습니다.");
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onRunSurfaceSnapClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->Logic || !this->mrmlScene()) return;

  vtkMRMLModelNode* frag1 = vtkMRMLModelNode::SafeDownCast(this->mrmlScene()->GetFirstNodeByName("Femur_Fragment_1"));
  vtkMRMLModelNode* frag2 = vtkMRMLModelNode::SafeDownCast(this->mrmlScene()->GetFirstNodeByName("Femur_Fragment_2"));

  if (!frag1 || !frag2)
  {
    QMessageBox::warning(this, "경고", "골절면 스냅을 실행하려면 씬 내에 Femur_Fragment_1 및 Femur_Fragment_2 모델이 모두 존재해야 합니다.");
    return;
  }

  double meanDistance = 0.0;
  bool success = d->Logic->RunFractureSurfaceSnap(frag1, frag2, meanDistance);

  if (success)
  {
    QString msg = QString("골절면 스냅 정합 완료!\n정합 오차: %1 mm").arg(meanDistance, 0, 'f', 4);
    QMessageBox::information(this, "정합 성공", msg);
    this->updateFragmentTable();
  }
  else
  {
    QMessageBox::warning(this, "정합 경고", QString("골절면 대응점 거리가 %1mm 이내인 쌍이 없거나 수집된 정밀 점의 개수가 부족합니다.\n\n수동 조작 기즈모를 이용해 두 골편 단면을 조금 더 근접시킨 후 다시 시도해 주세요.").arg(vtkSlicerFemurFracturePlannerCppLogic::SNAP_DISTANCE_LIMIT));
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onRunLandmarkMatchClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->Logic || !this->mrmlScene()) return;

  vtkMRMLModelNode* frag1 = vtkMRMLModelNode::SafeDownCast(this->mrmlScene()->GetFirstNodeByName("Femur_Fragment_1"));
  vtkMRMLModelNode* frag2 = vtkMRMLModelNode::SafeDownCast(this->mrmlScene()->GetFirstNodeByName("Femur_Fragment_2"));

  if (!frag1 || !frag2)
  {
    QMessageBox::warning(this, "경고", "마커 매칭을 실행하려면 Femur_Fragment_1 및 Femur_Fragment_2 모델이 모두 존재해야 합니다.");
    return;
  }

  vtkMRMLNode* srcFid = this->mrmlScene()->GetFirstNodeByName("Targeting_Source_Fiducials");
  vtkMRMLNode* tgtFid = this->mrmlScene()->GetFirstNodeByName("Targeting_Target_Fiducials");

  if (!srcFid || !tgtFid)
  {
    QMessageBox::warning(this, "경고", "마커 정보 노드(Targeting_Source_Fiducials / Targeting_Target_Fiducials)가 존재하지 않습니다.");
    return;
  }

  bool success = d->Logic->RunLandmarkRegistration(frag1, frag2, srcFid, tgtFid);
  if (success)
  {
    QMessageBox::information(this, "성공", "마커 기반 랜드마크 정합을 성공적으로 완료했습니다.");
    this->updateFragmentTable();
  }
  else
  {
    QMessageBox::critical(this, "실패", "정합에 필요한 제어점 개수가 3점 미만이거나 변환 도중 오류가 발생했습니다.");
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onRunMaskMatchClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->Logic || !d->activeSegmentationSelector || !this->mrmlScene()) return;

  // 타겟: 가이드 모델 셀렉터에서 선택된 모델 (=고정 기준)
  vtkMRMLModelNode* guideNode = vtkMRMLModelNode::SafeDownCast(d->guideModelSelector->currentNode());
  if (!guideNode)
  {
    QMessageBox::warning(this, "경고", "먼저 타겟 뉴를 위 '가이드 모델(또는 기준 골편)' 창에서 선택하세요.");
    return;
  }

  // 소스: 테이블에서 선택된 고편 (=이동 대상)
  QList<QTableWidgetItem*> selectedItems = d->fragmentTableWidget->selectedItems();
  if (selectedItems.isEmpty())
  {
    QMessageBox::warning(this, "경고", "표에서 움직일 대상 고편을 1개 선택하세요.");
    return;
  }
  QString fragmentNodeId = selectedItems.first()->data(Qt::UserRole).toString();
  vtkMRMLModelNode* fragmentNode = vtkMRMLModelNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(fragmentNodeId.toStdString().c_str()));
  if (!fragmentNode)
  {
    QMessageBox::warning(this, "경고", "선택된 고편 노드를 창에서 찾을 수 없습니다.");
    return;
  }

  vtkMRMLSegmentationNode* segNode = vtkMRMLSegmentationNode::SafeDownCast(d->activeSegmentationSelector->currentNode());
  if (!segNode)
  {
    QMessageBox::warning(this, "경고", "먼저 입력 세그멘테이션 노드를 5번 영역에서 지정하세요 (4번 에디터와 연동됨).");
    return;
  }

  vtkSegmentation* segmentation = segNode->GetSegmentation();
  std::string sourceId = segmentation ? segmentation->GetSegmentIdBySegmentName("Targeting_Source_ROI") : "";
  std::string targetId = segmentation ? segmentation->GetSegmentIdBySegmentName("Targeting_Target_ROI") : "";

  // 세그먼트가 없으면 자동 생성 후 안내 후 종료
  if (sourceId.empty() || targetId.empty())
  {
    if (segmentation)
    {
      double yellowColor[3] = {1.0, 1.0, 0.0};
      double cyanColor[3]   = {0.0, 1.0, 1.0};
      if (sourceId.empty())
      {
        segmentation->AddEmptySegment("Targeting_Source_ROI", "Targeting_Source_ROI", yellowColor);
      }
      if (targetId.empty())
      {
        segmentation->AddEmptySegment("Targeting_Target_ROI", "Targeting_Target_ROI", cyanColor);
      }
    }
    QMessageBox::information(this, "안내",
      "세그멘테이션 라벨이 추가되었습니다.\n\n"
      "4번 Segment Editor 탭에서 Paint 브러시를 이용해\n"
      "움직일 골편 단면과 타겟 뉴(가이드 또는 기준 골편) 단면을 각각 색칠한 뒤\n"
      "이 버튼을 다시 눌러주세요.");
    return;
  }

  bool success = d->Logic->RunMaskedIcpRegistration(fragmentNode, guideNode, segNode, sourceId, targetId);
  if (success)
  {
    QMessageBox::information(this, "성공", "색칠 영역 기반 마스킹 ICP 정합을 완료했습니다.");
    this->updateFragmentTable();
  }
  else
  {
    QMessageBox::critical(this, "실패", "마스킹 영역 추출에 실패했거나 정합 지점이 없습니다. 색칠을 했는지 확인하세요.");
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onClearMarkersClicked()
{
  if (!this->mrmlScene()) return;

  // 파이쓬 원본 동일: 노드를 삭제하지 않고 포인트만 제거(RemoveAllControlPoints)
  vtkMRMLMarkupsFiducialNode* srcFid = vtkMRMLMarkupsFiducialNode::SafeDownCast(
    this->mrmlScene()->GetFirstNodeByName("Targeting_Source_Markers"));
  vtkMRMLMarkupsFiducialNode* tgtFid = vtkMRMLMarkupsFiducialNode::SafeDownCast(
    this->mrmlScene()->GetFirstNodeByName("Targeting_Target_Markers"));

  bool cleared = false;
  if (srcFid)
  {
    srcFid->RemoveAllControlPoints();
    cleared = true;
  }
  if (tgtFid)
  {
    tgtFid->RemoveAllControlPoints();
    cleared = true;
  }

  if (cleared)
  {
    QMessageBox::information(this, "알림", "마커(점)가 모두 지워졌습니다. 이제 다시 정확한 위치에 순서대로 점을 직어주세요.");
  }
  else
  {
    QMessageBox::information(this, "알림", "지울 마커가 없습니다.");
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onClearMasksClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->activeSegmentationSelector || !this->mrmlScene()) return;

  vtkMRMLSegmentationNode* segNode = vtkMRMLSegmentationNode::SafeDownCast(d->activeSegmentationSelector->currentNode());
  if (!segNode || !segNode->GetSegmentation())
  {
    QMessageBox::warning(this, "경고", "세그먼트 소거를 수행할 세그멘테이션 노드가 없습니다.");
    return;
  }

  vtkSegmentation* segmentation = segNode->GetSegmentation();
  vtkNew<vtkStringArray> segmentIDs;
  segmentation->GetSegmentIDs(segmentIDs);
  
  int cleared = 0;
  for (vtkIdType i = 0; i < segmentIDs->GetNumberOfValues(); ++i)
  {
    std::string id = segmentIDs->GetValue(i);
    vtkSegment* seg = segmentation->GetSegment(id);
    if (seg && seg->GetName())
    {
      std::string name = seg->GetName();
      if (name == "Targeting_Source_ROI" || name == "Targeting_Target_ROI")
      {
        segmentation->RemoveSegment(id);
        cleared++;
      }
    }
  }

  if (cleared > 0)
  {
    QMessageBox::information(this, "알림", "ROI 마스킹 세그먼트 데이터가 성공적으로 클리어되었습니다.");
  }
  else
  {
    QMessageBox::information(this, "알림", "지울 마스킹 세그먼트(Targeting_Source_ROI, Targeting_Target_ROI)가 존재하지 않습니다.");
  }
}
