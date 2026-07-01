/*
File Name: qSlicerFemurFracturePlannerCppModulePlannerDialog.cxx
Version: v0-7.0.2
Date: 2026-07-01
Description: 대퇴골 골절 수술 계획을 위한 독립 팝업 다이얼로그 클래스 구현 (Segment Editor 및 씬 바인딩)

Version History:
- v0-7.0.2 (2026-07-01)
  - 내장 Segment Editor의 Add/Remove 버튼이 첫 씬 로드 시 비활성화되는 버그 수정을 위해 SegmentEditorNode에 SegmentationNode 및 SourceVolumeNode를 명시적으로 결합 (Z 증가)
- v0-7.0.1 (2026-07-01)
  - 다이얼로그 소멸 및 setMRMLScene(nullptr) 시 SegmentEditorNode의 씬 명시적 제거 및 Widget의 씬 연동을 끊어 VTK 메모리 누수 방지 (Z 증가)
- v0-7.0.0 (2026-07-01)
  - 가상 뼈 정복 연산 시 std::function 람다 콜백을 전달하여 reductionLogWidget에 실시간 진행 로그를 출력하도록 연동 (X 증가)
- v0-6.0.2 (2026-07-01)
  - updateFragmentTable 실행 중 노드 생성 이벤트에 의한 무한 재귀 재진입 방지 플래그 추가 및 Slicer 크래시 수정 (Z 증가)
- v0-6.0.1 (2026-07-01)
  - vtkSegmentation::AddEmptySegment 호출 시 const double[] 관련 컴파일 에러 수정 (Z 증가)
- v0-6.0.0 (2026-06-30)
  - 완전 이식 보완: 컴파일 헤더 정리, MRML 상태 저장, Scene 수명주기, 마커 생성/배치, 에지 기능 분리 (X 증가)
- v0-5.1.0 (2026-06-30)
  - 수동 타겟팅 매칭 슬롯을 Python 원본과 일치하게 수정: 마커 노드명(마커 접미사), 소스/타겟 구조(테이블+가이드 셀렉터 기반), 자동 생성+Place 모드, 6개 자동 분할, RemoveAllControlPoints 방식 클리어 (Y 증가)
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
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QVariantMap>
#include <QSignalBlocker>
#include <QByteArray>
#include <QCheckBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QColorDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QTime>
#include <QCoreApplication>
#include <QVBoxLayout>
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
#include <vtkMRMLMarkupsDisplayNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLScriptedModuleNode.h>
#include <vtkMRMLSegmentationNode.h>
#include <vtkMRMLSegmentationDisplayNode.h>
#include <vtkSegmentation.h>
#include <vtkSegment.h>
#include <vtkStringArray.h>
#include <vtkVector.h>

// STD includes
#include <algorithm>
#include <string>
#include <vector>

namespace
{
constexpr const char* PARAMETER_NODE_NAME = "FemurFracturePlannerCpp_Parameters";
constexpr const char* INPUT_VOLUME_REFERENCE = "InputVolume";
constexpr const char* SEGMENTATION_REFERENCE = "Segmentation";
constexpr const char* GUIDE_MODEL_REFERENCE = "GuideModel";
constexpr const char* FRAGMENT_ROLE_ATTRIBUTE = "FemurFracturePlannerCpp.Role";
constexpr const char* FRAGMENT_ROLE_VALUE = "Fragment";
constexpr const char* SOURCE_MARKERS_NAME = "Targeting_Source_Markers";
constexpr const char* TARGET_MARKERS_NAME = "Targeting_Target_Markers";

bool IsFragmentModel(vtkMRMLModelNode* modelNode)
{
  if (!modelNode)
  {
    return false;
  }
  const char* role = modelNode->GetAttribute(FRAGMENT_ROLE_ATTRIBUTE);
  if (role && std::string(role) == FRAGMENT_ROLE_VALUE)
  {
    return true;
  }
  const std::string name = modelNode->GetName() ? modelNode->GetName() : "";
  return name.rfind("Femur_Fragment", 0) == 0;
}

int FragmentSortIndex(vtkMRMLModelNode* modelNode)
{
  if (!modelNode)
  {
    return 1000000;
  }
  const char* value = modelNode->GetAttribute("FemurFracturePlannerCpp.FragmentIndex");
  if (value)
  {
    bool ok = false;
    const int index = QString::fromUtf8(value).toInt(&ok);
    if (ok)
    {
      return index;
    }
  }
  const std::string name = modelNode->GetName() ? modelNode->GetName() : "";
  const std::size_t separator = name.find_last_of('_');
  if (separator != std::string::npos)
  {
    try
    {
      return std::stoi(name.substr(separator + 1));
    }
    catch (...)
    {
    }
  }
  return 1000000;
}

std::vector<vtkMRMLModelNode*> CollectFragmentModels(vtkMRMLScene* scene)
{
  std::vector<vtkMRMLModelNode*> fragments;
  if (!scene)
  {
    return fragments;
  }
  std::vector<vtkMRMLNode*> modelNodes;
  scene->GetNodesByClass("vtkMRMLModelNode", modelNodes);
  for (vtkMRMLNode* node : modelNodes)
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);
    if (IsFragmentModel(modelNode))
    {
      fragments.push_back(modelNode);
    }
  }
  std::stable_sort(fragments.begin(), fragments.end(),
    [](vtkMRMLModelNode* left, vtkMRMLModelNode* right)
    {
      const int leftIndex = FragmentSortIndex(left);
      const int rightIndex = FragmentSortIndex(right);
      if (leftIndex != rightIndex)
      {
        return leftIndex < rightIndex;
      }
      const std::string leftName = left && left->GetName() ? left->GetName() : "";
      const std::string rightName = right && right->GetName() ? right->GetName() : "";
      return leftName < rightName;
    });
  return fragments;
}
}

//-----------------------------------------------------------------------------
class qSlicerFemurFracturePlannerCppModulePlannerDialogPrivate : public Ui::FemurFracturePlanner
{
public:
  qSlicerFemurFracturePlannerCppModulePlannerDialogPrivate();
  qMRMLSegmentEditorWidget* SegmentEditorWidget;
  vtkMRMLSegmentEditorNode* SegmentEditorNode;
  vtkSlicerFemurFracturePlannerCppLogic* Logic;
  vtkMRMLScriptedModuleNode* ParameterNode;

  vtkMRMLModelNode* ActiveModelNode;
  vtkMRMLLinearTransformNode* ActiveTransformNode;
  bool UpdatingTable;
};

//-----------------------------------------------------------------------------
qSlicerFemurFracturePlannerCppModulePlannerDialogPrivate::qSlicerFemurFracturePlannerCppModulePlannerDialogPrivate()
  : SegmentEditorWidget(nullptr)
  , SegmentEditorNode(nullptr)
  , Logic(nullptr)
  , ParameterNode(nullptr)
  , ActiveModelNode(nullptr)
  , ActiveTransformNode(nullptr)
  , UpdatingTable(false)
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
  if (d->maskedEdgeDetectionButton)
  {
    connect(d->maskedEdgeDetectionButton, SIGNAL(clicked()), this, SLOT(onMaskedEdgeDetectionClicked()));
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
  d->SegmentEditorNode = nullptr;
  d->ParameterNode = nullptr;

  // 다이얼로그와 관련된 모든 등록된 VTK 관찰자(Observer) 일괄 강제 해제
  this->qvtkDisconnectAll();
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);

  vtkMRMLScene* previousScene = this->mrmlScene();
  this->qvtkReconnect(previousScene, scene, vtkMRMLScene::StartCloseEvent,
    this, SLOT(onMRMLSceneStartClose()));
  this->qvtkReconnect(previousScene, scene, vtkMRMLScene::EndCloseEvent,
    this, SLOT(onMRMLSceneEndClose()));
  this->qvtkReconnect(previousScene, scene, vtkMRMLScene::NodeAddedEvent,
    this, SLOT(onMRMLSceneChanged()));
  this->qvtkReconnect(previousScene, scene, vtkMRMLScene::NodeRemovedEvent,
    this, SLOT(onMRMLSceneChanged()));
  this->qvtkReconnect(previousScene, scene, vtkMRMLScene::EndBatchProcessEvent,
    this, SLOT(onMRMLSceneChanged()));

  this->qMRMLWidget::setMRMLScene(scene);

  if (!scene)
  {
    if (d->SegmentEditorWidget)
    {
      d->SegmentEditorWidget->setMRMLSegmentEditorNode(nullptr);
      d->SegmentEditorWidget->setMRMLScene(nullptr);
    }
    if (previousScene && d->SegmentEditorNode)
    {
      previousScene->RemoveNode(d->SegmentEditorNode);
    }
    d->SegmentEditorNode = nullptr;
    d->ParameterNode = nullptr;
    if (d->inputVolumeSelector) d->inputVolumeSelector->setMRMLScene(nullptr);
    if (d->activeSegmentationSelector) d->activeSegmentationSelector->setMRMLScene(nullptr);
    if (d->guideModelSelector) d->guideModelSelector->setMRMLScene(nullptr);
    return;
  }

  // 모듈 선택 상태를 MRML Scene과 함께 저장하기 위한 상태 노드.
  d->ParameterNode = vtkMRMLScriptedModuleNode::SafeDownCast(
    scene->GetFirstNodeByName(PARAMETER_NODE_NAME));
  if (!d->ParameterNode)
  {
    d->ParameterNode = vtkMRMLScriptedModuleNode::SafeDownCast(
      scene->AddNewNodeByClass("vtkMRMLScriptedModuleNode"));
    if (d->ParameterNode)
    {
      d->ParameterNode->SetName(PARAMETER_NODE_NAME);
      d->ParameterNode->SetAttribute("ModuleName", "FemurFracturePlannerCpp");
      d->ParameterNode->SetHideFromEditors(true);
    }
  }

  d->SegmentEditorNode = vtkMRMLSegmentEditorNode::SafeDownCast(
    scene->GetFirstNodeByName("FemurFracturePlannerCpp_SegmentEditorNode"));
  if (!d->SegmentEditorNode)
  {
    // 이전 개발 버전의 노드 이름도 한 번 재사용하여 기존 Scene 호환성을 유지한다.
    d->SegmentEditorNode = vtkMRMLSegmentEditorNode::SafeDownCast(
      scene->GetFirstNodeByName("FemurFracturePlanner_SegmentEditorNode"));
  }
  if (!d->SegmentEditorNode)
  {
    d->SegmentEditorNode = vtkMRMLSegmentEditorNode::SafeDownCast(
      scene->AddNewNodeByClass("vtkMRMLSegmentEditorNode"));
    if (d->SegmentEditorNode)
    {
      d->SegmentEditorNode->SetName("FemurFracturePlannerCpp_SegmentEditorNode");
      d->SegmentEditorNode->SetHideFromEditors(true);
    }
  }

  if (d->SegmentEditorWidget)
  {
    d->SegmentEditorWidget->setMRMLScene(scene);
    d->SegmentEditorWidget->setMRMLSegmentEditorNode(d->SegmentEditorNode);
  }

  vtkMRMLSegmentationNode* defaultSegmentation = nullptr;
  if (d->ParameterNode)
  {
    defaultSegmentation = vtkMRMLSegmentationNode::SafeDownCast(
      d->ParameterNode->GetNodeReference(SEGMENTATION_REFERENCE));
  }
  if (!defaultSegmentation)
  {
    defaultSegmentation = vtkMRMLSegmentationNode::SafeDownCast(
      scene->GetFirstNodeByClass("vtkMRMLSegmentationNode"));
  }
  if (!defaultSegmentation)
  {
    defaultSegmentation = vtkMRMLSegmentationNode::SafeDownCast(
      scene->AddNewNodeByClass("vtkMRMLSegmentationNode"));
    if (defaultSegmentation)
    {
      defaultSegmentation->SetName("Femur_Segmentation");
      defaultSegmentation->CreateDefaultDisplayNodes();
    }
  }

  if (d->inputVolumeSelector) d->inputVolumeSelector->setMRMLScene(scene);
  if (d->activeSegmentationSelector) d->activeSegmentationSelector->setMRMLScene(scene);
  if (d->guideModelSelector) d->guideModelSelector->setMRMLScene(scene);

  if (defaultSegmentation && d->activeSegmentationSelector)
  {
    d->activeSegmentationSelector->setCurrentNode(defaultSegmentation);
  }

  this->restoreUiFromParameterNode();

  vtkMRMLSegmentationNode* activeSegmentation = d->activeSegmentationSelector
    ? vtkMRMLSegmentationNode::SafeDownCast(d->activeSegmentationSelector->currentNode()) : nullptr;
  vtkMRMLScalarVolumeNode* inputVolume = d->inputVolumeSelector
    ? vtkMRMLScalarVolumeNode::SafeDownCast(d->inputVolumeSelector->currentNode()) : nullptr;

  if (d->SegmentEditorWidget)
  {
    if (d->SegmentEditorNode)
    {
      d->SegmentEditorNode->SetAndObserveSegmentationNode(activeSegmentation);
      d->SegmentEditorNode->SetAndObserveSourceVolumeNode(inputVolume);
    }
    d->SegmentEditorWidget->setSegmentationNode(activeSegmentation);
    d->SegmentEditorWidget->setSourceVolumeNode(inputVolume);
  }
  if (activeSegmentation && inputVolume)
  {
    activeSegmentation->SetReferenceImageGeometryParameterFromVolumeNode(inputVolume);
    activeSegmentation->Modified();
  }

  this->updateFragmentTable();
  this->updateParameterNodeFromUi();
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
vtkMRMLModelNode* qSlicerFemurFracturePlannerCppModulePlannerDialog::selectedFragmentNode()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!this->mrmlScene() || !d->fragmentTableWidget)
  {
    return nullptr;
  }

  const int selectedRow = d->fragmentTableWidget->currentRow();
  QTableWidgetItem* nameItem = selectedRow >= 0
    ? d->fragmentTableWidget->item(selectedRow, 0) : nullptr;
  if (!nameItem)
  {
    return nullptr;
  }

  const QString nodeId = nameItem->data(Qt::UserRole).toString();
  return nodeId.isEmpty() ? nullptr : vtkMRMLModelNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(nodeId.toUtf8().constData()));
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::updateParameterNodeFromUi()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->ParameterNode)
  {
    return;
  }

  vtkMRMLNode* inputVolume = d->inputVolumeSelector ? d->inputVolumeSelector->currentNode() : nullptr;
  vtkMRMLNode* segmentation = d->activeSegmentationSelector ? d->activeSegmentationSelector->currentNode() : nullptr;
  vtkMRMLNode* guideModel = d->guideModelSelector ? d->guideModelSelector->currentNode() : nullptr;
  d->ParameterNode->SetNodeReferenceID(INPUT_VOLUME_REFERENCE, inputVolume ? inputVolume->GetID() : nullptr);
  d->ParameterNode->SetNodeReferenceID(SEGMENTATION_REFERENCE, segmentation ? segmentation->GetID() : nullptr);
  d->ParameterNode->SetNodeReferenceID(GUIDE_MODEL_REFERENCE, guideModel ? guideModel->GetID() : nullptr);

  if (d->thresholdSlider)
  {
    const QByteArray value = QByteArray::number(d->thresholdSlider->value(), 'g', 16);
    d->ParameterNode->SetAttribute("Threshold", value.constData());
  }
  if (d->rotationAngleSlider)
  {
    const QByteArray value = QByteArray::number(d->rotationAngleSlider->value(), 'g', 16);
    d->ParameterNode->SetAttribute("RotationAngle", value.constData());
  }

  const char* axis = d->rotationXRadio && d->rotationXRadio->isChecked() ? "X" :
    (d->rotationYRadio && d->rotationYRadio->isChecked() ? "Y" : "Z");
  d->ParameterNode->SetAttribute("RotationAxis", axis);
  d->ParameterNode->SetAttribute("VolumeVisible",
    d->volumeVisibilityButton && d->volumeVisibilityButton->isChecked() ? "1" : "0");
  d->ParameterNode->SetAttribute("VolumeRenderingVisible",
    d->volumeRenderingVisibilityButton && d->volumeRenderingVisibilityButton->isChecked() ? "1" : "0");
  d->ParameterNode->SetAttribute("GuideVisible",
    d->guideVisibilityButton && d->guideVisibilityButton->isChecked() ? "1" : "0");
  d->ParameterNode->Modified();
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::restoreUiFromParameterNode()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->ParameterNode)
  {
    return;
  }

  if (d->inputVolumeSelector)
  {
    QSignalBlocker blocker(d->inputVolumeSelector);
    d->inputVolumeSelector->setCurrentNode(
      d->ParameterNode->GetNodeReference(INPUT_VOLUME_REFERENCE));
  }
  if (d->activeSegmentationSelector)
  {
    vtkMRMLNode* savedSegmentation = d->ParameterNode->GetNodeReference(SEGMENTATION_REFERENCE);
    if (savedSegmentation)
    {
      QSignalBlocker blocker(d->activeSegmentationSelector);
      d->activeSegmentationSelector->setCurrentNode(savedSegmentation);
    }
  }
  if (d->guideModelSelector)
  {
    QSignalBlocker blocker(d->guideModelSelector);
    d->guideModelSelector->setCurrentNode(
      d->ParameterNode->GetNodeReference(GUIDE_MODEL_REFERENCE));
  }

  if (d->thresholdSlider)
  {
    if (const char* value = d->ParameterNode->GetAttribute("Threshold"))
    {
      QSignalBlocker blocker(d->thresholdSlider);
      d->thresholdSlider->setValue(QString::fromUtf8(value).toDouble());
    }
  }
  if (d->rotationAngleSlider)
  {
    if (const char* value = d->ParameterNode->GetAttribute("RotationAngle"))
    {
      QSignalBlocker blocker(d->rotationAngleSlider);
      d->rotationAngleSlider->setValue(QString::fromUtf8(value).toDouble());
    }
  }

  const QString axis = QString::fromUtf8(
    d->ParameterNode->GetAttribute("RotationAxis") ?
    d->ParameterNode->GetAttribute("RotationAxis") : "X");
  if (d->rotationXRadio && d->rotationYRadio && d->rotationZRadio)
  {
    QSignalBlocker blockX(d->rotationXRadio);
    QSignalBlocker blockY(d->rotationYRadio);
    QSignalBlocker blockZ(d->rotationZRadio);
    d->rotationXRadio->setChecked(axis == "X");
    d->rotationYRadio->setChecked(axis == "Y");
    d->rotationZRadio->setChecked(axis == "Z");
  }

  vtkMRMLScriptedModuleNode* parameterNode = d->ParameterNode;
  auto restoreChecked = [parameterNode](QPushButton* button, const char* attributeName)
  {
    if (!button || !parameterNode)
    {
      return;
    }
    const char* value = parameterNode->GetAttribute(attributeName);
    if (value)
    {
      QSignalBlocker blocker(button);
      button->setChecked(QString::fromUtf8(value) == "1");
    }
  };
  restoreChecked(d->volumeVisibilityButton, "VolumeVisible");
  restoreChecked(d->volumeRenderingVisibilityButton, "VolumeRenderingVisible");
  restoreChecked(d->guideVisibilityButton, "GuideVisible");
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onMRMLSceneStartClose()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (d->ActiveTransformNode)
  {
    this->qvtkDisconnect(d->ActiveTransformNode, vtkCommand::ModifiedEvent,
      this, SLOT(onActiveTransformNodeModified()));
  }
  d->ActiveTransformNode = nullptr;
  d->ActiveModelNode = nullptr;
  d->SegmentEditorNode = nullptr;
  d->ParameterNode = nullptr;

  if (d->SegmentEditorWidget)
  {
    d->SegmentEditorWidget->setSegmentationNode(nullptr);
    d->SegmentEditorWidget->setSourceVolumeNode(nullptr);
    d->SegmentEditorWidget->setMRMLSegmentEditorNode(nullptr);
  }
  if (d->fragmentTableWidget)
  {
    d->fragmentTableWidget->clearContents();
    d->fragmentTableWidget->setRowCount(0);
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onMRMLSceneEndClose()
{
  vtkMRMLScene* scene = this->mrmlScene();
  if (scene)
  {
    this->setMRMLScene(scene);
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onMRMLSceneChanged()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!this->mrmlScene() || this->mrmlScene()->IsBatchProcessing())
  {
    return;
  }
  if (d->ParameterNode && d->ParameterNode->GetScene() != this->mrmlScene())
  {
    d->ParameterNode = nullptr;
  }
  if (d->SegmentEditorNode && d->SegmentEditorNode->GetScene() != this->mrmlScene())
  {
    d->SegmentEditorNode = nullptr;
  }
  this->updateFragmentTable();
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onVolumeRenderingToggled(bool checked)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  this->updateParameterNodeFromUi();
  if (!d->Logic || !d->inputVolumeSelector || !d->thresholdSlider)
  {
    return;
  }
  vtkMRMLScalarVolumeNode* inputNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    d->inputVolumeSelector->currentNode());
  if (!inputNode)
  {
    if (checked && d->volumeRenderingVisibilityButton)
    {
      QSignalBlocker blocker(d->volumeRenderingVisibilityButton);
      d->volumeRenderingVisibilityButton->setChecked(false);
      this->updateParameterNodeFromUi();
    }
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
  this->updateParameterNodeFromUi();
  if (!d->Logic || !d->inputVolumeSelector || !d->volumeRenderingVisibilityButton)
  {
    return;
  }
  vtkMRMLScalarVolumeNode* inputNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    d->inputVolumeSelector->currentNode());
  if (inputNode && d->volumeRenderingVisibilityButton->isChecked())
  {
    d->Logic->AdjustVolumeRenderingThreshold(inputNode, value);
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onRotationAngleChanged(double angle)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  this->updateParameterNodeFromUi();
  if (!d->Logic || !d->inputVolumeSelector || !d->guideModelSelector)
  {
    return;
  }
  vtkMRMLScalarVolumeNode* inputNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    d->inputVolumeSelector->currentNode());
  vtkMRMLModelNode* guideNode = vtkMRMLModelNode::SafeDownCast(
    d->guideModelSelector->currentNode());

  const char* axis = "Z";
  if (d->rotationXRadio && d->rotationXRadio->isChecked())
  {
    axis = "X";
  }
  else if (d->rotationYRadio && d->rotationYRadio->isChecked())
  {
    axis = "Y";
  }

  if (inputNode || guideNode)
  {
    d->Logic->RotateVolume(inputNode, axis, angle, guideNode);
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onRotationAxisChanged()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  this->updateParameterNodeFromUi();
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
  if (!d->Logic || !d->inputVolumeSelector)
  {
    return;
  }
  vtkMRMLScalarVolumeNode* inputNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    d->inputVolumeSelector->currentNode());
  if (!inputNode)
  {
    QMessageBox::warning(this, "경고", "에지 검출을 수행할 입력 볼륨을 선택하세요.");
    return;
  }
  vtkMRMLScalarVolumeNode* edgeNode = d->Logic->DetectVolumeEdges(inputNode);
  if (edgeNode)
  {
    d->inputVolumeSelector->setCurrentNode(edgeNode);
    if (d->volumeVisibilityButton)
    {
      d->volumeVisibilityButton->setChecked(true);
    }
  }
  else
  {
    QMessageBox::critical(this, "오류", "일반 3D 에지 검출에 실패했습니다.");
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onMaskedEdgeDetectionClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->Logic || !d->inputVolumeSelector || !d->thresholdSlider)
  {
    return;
  }
  vtkMRMLScalarVolumeNode* inputNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    d->inputVolumeSelector->currentNode());
  if (!inputNode)
  {
    QMessageBox::warning(this, "경고", "마스킹 에지 검출을 수행할 입력 볼륨을 선택하세요.");
    return;
  }
  vtkMRMLScalarVolumeNode* edgeNode = d->Logic->DetectVolumeEdgesMasked(
    inputNode, d->thresholdSlider->value());
  if (edgeNode)
  {
    d->inputVolumeSelector->setCurrentNode(edgeNode);
    if (d->volumeVisibilityButton)
    {
      d->volumeVisibilityButton->setChecked(true);
    }
  }
  else
  {
    QMessageBox::critical(this, "오류", "임계값 마스킹 3D 에지 검출에 실패했습니다.");
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
  vtkMRMLScalarVolumeNode* inputNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    d->inputVolumeSelector->currentNode());
  if (!inputNode)
  {
    if (checked)
    {
      QSignalBlocker blocker(d->volumeVisibilityButton);
      d->volumeVisibilityButton->setChecked(false);
    }
    this->updateParameterNodeFromUi();
    return;
  }

  d->Logic->SetVolumeVisibility(inputNode, checked);
  d->volumeVisibilityButton->setText(checked ? "숨기기" : "보기");
  d->volumeVisibilityButton->setToolTip(
    checked ? "뷰어에서 볼륨을 숨깁니다." : "뷰어에서 볼륨을 표시합니다.");
  this->updateParameterNodeFromUi();
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onInputVolumeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  qDebug() << "onInputVolumeChanged called, Node:" << (node ? node->GetName() : "nullptr");

  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  vtkMRMLSegmentationNode* segNode = d->activeSegmentationSelector
    ? vtkMRMLSegmentationNode::SafeDownCast(d->activeSegmentationSelector->currentNode()) : nullptr;

  if (d->SegmentEditorWidget)
  {
    d->SegmentEditorWidget->setSegmentationNode(segNode);
    d->SegmentEditorWidget->setSourceVolumeNode(volumeNode);
  }
  if (segNode && volumeNode)
  {
    segNode->SetReferenceImageGeometryParameterFromVolumeNode(volumeNode);
    segNode->Modified();
  }

  if (d->rotationAngleSlider)
  {
    QSignalBlocker blocker(d->rotationAngleSlider);
    d->rotationAngleSlider->setValue(0.0);
  }

  if (d->volumeVisibilityButton)
  {
    if (!volumeNode)
    {
      QSignalBlocker blocker(d->volumeVisibilityButton);
      d->volumeVisibilityButton->setChecked(false);
      d->volumeVisibilityButton->setText("보기");
    }
    else if (d->volumeVisibilityButton->isChecked())
    {
      if (d->Logic)
      {
        d->Logic->SetVolumeVisibility(volumeNode, true);
      }
    }
    else
    {
      d->volumeVisibilityButton->setChecked(true);
    }
  }
  this->updateParameterNodeFromUi();
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
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);
    std::string nodeName = node->GetName() ? node->GetName() : "";
    if (IsFragmentModel(modelNode) || (nodeName.length() > 9 && nodeName.substr(nodeName.length() - 9) == "_Mirrored"))
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
  if (d->UpdatingTable)
  {
    return;
  }
  d->UpdatingTable = true;

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

  const QString selectedNodeId = d->ActiveModelNode && d->ActiveModelNode->GetID()
    ? QString::fromUtf8(d->ActiveModelNode->GetID()) : QString();
  std::vector<vtkMRMLModelNode*> fragmentNodes = CollectFragmentModels(this->mrmlScene());

  d->fragmentTableWidget->setRowCount(static_cast<int>(fragmentNodes.size()));

  int rowToRestore = -1;
  for (size_t row = 0; row < fragmentNodes.size(); ++row)
  {
    vtkMRMLModelNode* node = fragmentNodes[row];
    if (!node) continue;
    if (!selectedNodeId.isEmpty() && node->GetID()
        && selectedNodeId == QString::fromUtf8(node->GetID()))
    {
      rowToRestore = static_cast<int>(row);
    }

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
      vtkMRMLTransformNode* originalParent = transformNode;
      vtkMRMLLinearTransformNode* newTrans = vtkMRMLLinearTransformNode::SafeDownCast(
        this->mrmlScene()->AddNewNodeByClass("vtkMRMLLinearTransformNode")
      );
      if (newTrans)
      {
        const std::string tName = std::string(node->GetName() ? node->GetName() : "Fragment")
          + "_Transform";
        newTrans->SetName(tName.c_str());
        if (originalParent)
        {
          newTrans->SetAndObserveTransformNodeID(originalParent->GetID());
        }
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

  if (rowToRestore >= 0)
  {
    d->fragmentTableWidget->selectRow(rowToRestore);
  }
  d->fragmentTableWidget->blockSignals(false);
  d->UpdatingTable = false;
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
    this->updateParameterNodeFromUi();
    return;
  }

  if (d->SegmentEditorNode && !d->SegmentEditorWidget->mrmlSegmentEditorNode())
  {
    d->SegmentEditorWidget->setMRMLSegmentEditorNode(d->SegmentEditorNode);
  }

  vtkMRMLSegmentationNode* segNode = vtkMRMLSegmentationNode::SafeDownCast(node);
  vtkMRMLScalarVolumeNode* volumeNode = d->inputVolumeSelector
    ? vtkMRMLScalarVolumeNode::SafeDownCast(d->inputVolumeSelector->currentNode()) : nullptr;

  if (d->SegmentEditorNode)
  {
    d->SegmentEditorNode->SetAndObserveSegmentationNode(segNode);
    d->SegmentEditorNode->SetAndObserveSourceVolumeNode(volumeNode);
  }
  d->SegmentEditorWidget->setSegmentationNode(segNode);
  d->SegmentEditorWidget->setSourceVolumeNode(volumeNode);
  if (segNode && volumeNode)
  {
    segNode->SetReferenceImageGeometryParameterFromVolumeNode(volumeNode);
    segNode->Modified();
  }
  this->updateParameterNodeFromUi();
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
    const bool visible = guideNode && guideNode->GetDisplayNode()
      && guideNode->GetDisplayNode()->GetVisibility() != 0;
    QSignalBlocker blocker(d->guideVisibilityButton);
    d->guideVisibilityButton->setChecked(visible);
  }

  if (d->guideInteractionButton)
  {
    bool interaction = false;
    if (guideNode)
    {
      vtkMRMLTransformNode* transformNode = guideNode->GetParentTransformNode();
      vtkMRMLTransformDisplayNode* displayNode = transformNode
        ? vtkMRMLTransformDisplayNode::SafeDownCast(transformNode->GetDisplayNode()) : nullptr;
      interaction = displayNode && displayNode->GetEditorVisibility() != 0;
    }
    QSignalBlocker blocker(d->guideInteractionButton);
    d->guideInteractionButton->setChecked(interaction);
  }
  this->updateParameterNodeFromUi();
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onGuideVisibilityToggled(bool checked)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->guideModelSelector)
  {
    return;
  }

  vtkMRMLModelNode* guideNode = vtkMRMLModelNode::SafeDownCast(
    d->guideModelSelector->currentNode());
  if (!guideNode)
  {
    if (d->guideVisibilityButton)
    {
      QSignalBlocker blocker(d->guideVisibilityButton);
      d->guideVisibilityButton->setChecked(false);
    }
    this->updateParameterNodeFromUi();
    return;
  }

  guideNode->CreateDefaultDisplayNodes();
  if (guideNode->GetDisplayNode())
  {
    guideNode->GetDisplayNode()->SetVisibility(checked ? 1 : 0);
  }
  this->updateParameterNodeFromUi();
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onGuideInteractionToggled(bool checked)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->guideModelSelector || !this->mrmlScene())
  {
    return;
  }

  vtkMRMLModelNode* guideNode = vtkMRMLModelNode::SafeDownCast(
    d->guideModelSelector->currentNode());
  if (!guideNode)
  {
    if (d->guideInteractionButton)
    {
      QSignalBlocker blocker(d->guideInteractionButton);
      d->guideInteractionButton->setChecked(false);
    }
    return;
  }

  vtkMRMLTransformNode* transformNode = guideNode->GetParentTransformNode();
  if (checked && (!transformNode || !transformNode->IsA("vtkMRMLLinearTransformNode")))
  {
    vtkMRMLTransformNode* originalParent = transformNode;
    vtkMRMLLinearTransformNode* newTransform = vtkMRMLLinearTransformNode::SafeDownCast(
      this->mrmlScene()->AddNewNodeByClass("vtkMRMLLinearTransformNode"));
    if (newTransform)
    {
      const std::string name = std::string(guideNode->GetName() ? guideNode->GetName() : "Guide")
        + "_Transform";
      newTransform->SetName(name.c_str());
      if (originalParent)
      {
        newTransform->SetAndObserveTransformNodeID(originalParent->GetID());
      }
      guideNode->SetAndObserveTransformNodeID(newTransform->GetID());
      transformNode = newTransform;
    }
  }

  if (transformNode)
  {
    transformNode->CreateDefaultDisplayNodes();
    vtkMRMLTransformDisplayNode* displayNode = vtkMRMLTransformDisplayNode::SafeDownCast(
      transformNode->GetDisplayNode());
    if (displayNode)
    {
      displayNode->SetEditorVisibility(checked ? 1 : 0);
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
  if (!d->Logic || !d->guideModelSelector || !this->mrmlScene())
  {
    return;
  }

  vtkMRMLModelNode* guideNode = vtkMRMLModelNode::SafeDownCast(
    d->guideModelSelector->currentNode());
  if (!guideNode)
  {
    QMessageBox::warning(this, "경고", "자동 정합의 기준이 될 가이드 모델을 먼저 선택하세요.");
    return;
  }

  std::vector<vtkMRMLModelNode*> fragmentNodes = CollectFragmentModels(this->mrmlScene());
  fragmentNodes.erase(std::remove(fragmentNodes.begin(), fragmentNodes.end(), guideNode),
    fragmentNodes.end());
  if (fragmentNodes.empty())
  {
    QMessageBox::warning(this, "경고", "씬에서 정합할 골편 모델을 찾을 수 없습니다.");
    return;
  }

  if (d->reductionLogWidget)
  {
    d->reductionLogWidget->clear();
    this->addLogMessage("========================================");
    this->addLogMessage("ICP 자동 정복(Auto-Reduction) 연산 시작...");
    this->addLogMessage("========================================");
  }

  double meanDistance = 0.0;
  int iterations = 0;
  auto logCallback = [this](const std::string& msg) {
    this->addLogMessage(QString::fromStdString(msg));
  };

  const bool success = d->Logic->RunIcpRegistration(
    fragmentNodes, guideNode, meanDistance, iterations, logCallback);
  if (success)
  {
    if (d->reductionLogWidget)
    {
      this->addLogMessage("ICP 자동 정복이 성공적으로 완료되었습니다.");
    }
    QMessageBox::information(this, "정합 성공",
      QString("병합 ICP 자동 정합 완료!\n평균 정합 오차: %1 mm\n반복 횟수: %2 회")
        .arg(meanDistance, 0, 'f', 4).arg(iterations));
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
  if (!d->Logic || !this->mrmlScene())
  {
    return;
  }

  std::vector<vtkMRMLModelNode*> fragments = CollectFragmentModels(this->mrmlScene());
  if (fragments.size() < 2)
  {
    QMessageBox::warning(this, "경고", "골절면 스냅에는 서로 다른 골편 모델이 최소 2개 필요합니다.");
    return;
  }

  vtkMRMLModelNode* moving = this->selectedFragmentNode();
  if (!moving || !IsFragmentModel(moving))
  {
    moving = fragments.front();
  }
  vtkMRMLModelNode* fixed = nullptr;
  for (vtkMRMLModelNode* fragment : fragments)
  {
    if (fragment != moving)
    {
      fixed = fragment;
      break;
    }
  }
  if (!moving || !fixed)
  {
    QMessageBox::warning(this, "경고", "서로 다른 두 골편을 선택할 수 없습니다.");
    return;
  }

  if (d->reductionLogWidget)
  {
    d->reductionLogWidget->clear();
    this->addLogMessage("========================================");
    this->addLogMessage("골절면 정밀 스냅(Surface Snap) 연산 시작...");
    this->addLogMessage("========================================");
  }

  double meanDistance = 0.0;
  auto logCallback = [this](const std::string& msg) {
    this->addLogMessage(QString::fromStdString(msg));
  };

  const bool success = d->Logic->RunFractureSurfaceSnap(moving, fixed, meanDistance, logCallback);
  if (success)
  {
    if (d->reductionLogWidget)
    {
      this->addLogMessage("골절면 스냅 정합이 성공적으로 완료되었습니다.");
    }
    QMessageBox::information(this, "정합 성공",
      QString("골절면 스냅 정합 완료!\n정합 오차: %1 mm").arg(meanDistance, 0, 'f', 4));
    this->updateFragmentTable();
  }
  else
  {
    QMessageBox::warning(this, "정합 경고",
      QString("골절면 대응점 거리가 %1 mm 이내인 쌍이 없거나 대응점 수가 부족합니다.\n\n"
              "수동 기즈모로 두 골절면을 더 가깝게 배치한 뒤 다시 시도하세요.")
        .arg(vtkSlicerFemurFracturePlannerCppLogic::SNAP_DISTANCE_LIMIT));
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onRunLandmarkMatchClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->Logic || !this->mrmlScene() || !d->guideModelSelector)
  {
    return;
  }

  vtkMRMLModelNode* targetNode = vtkMRMLModelNode::SafeDownCast(
    d->guideModelSelector->currentNode());
  if (!targetNode)
  {
    QMessageBox::warning(this, "경고", "먼저 가이드 모델 또는 기준 골편을 선택하세요.");
    return;
  }
  vtkMRMLModelNode* sourceNode = this->selectedFragmentNode();
  if (!sourceNode)
  {
    QMessageBox::warning(this, "경고", "골편 표에서 움직일 대상 골편을 선택하세요.");
    return;
  }
  if (sourceNode == targetNode)
  {
    QMessageBox::warning(this, "경고", "이동 골편과 기준 모델은 서로 달라야 합니다.");
    return;
  }

  vtkMRMLMarkupsFiducialNode* sourceMarkers = vtkMRMLMarkupsFiducialNode::SafeDownCast(
    this->mrmlScene()->GetFirstNodeByName(SOURCE_MARKERS_NAME));
  vtkMRMLMarkupsFiducialNode* targetMarkers = vtkMRMLMarkupsFiducialNode::SafeDownCast(
    this->mrmlScene()->GetFirstNodeByName(TARGET_MARKERS_NAME));
  const bool markerNodesMissing = !sourceMarkers || !targetMarkers;

  if (!sourceMarkers)
  {
    sourceMarkers = vtkMRMLMarkupsFiducialNode::SafeDownCast(
      this->mrmlScene()->AddNewNodeByClass("vtkMRMLMarkupsFiducialNode", SOURCE_MARKERS_NAME));
    if (sourceMarkers)
    {
      sourceMarkers->CreateDefaultDisplayNodes();
      vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(
        sourceMarkers->GetDisplayNode());
      if (displayNode)
      {
        displayNode->SetSelectedColor(1.0, 1.0, 0.0);
      }
    }
  }
  if (!targetMarkers)
  {
    targetMarkers = vtkMRMLMarkupsFiducialNode::SafeDownCast(
      this->mrmlScene()->AddNewNodeByClass("vtkMRMLMarkupsFiducialNode", TARGET_MARKERS_NAME));
    if (targetMarkers)
    {
      targetMarkers->CreateDefaultDisplayNodes();
      vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(
        targetMarkers->GetDisplayNode());
      if (displayNode)
      {
        displayNode->SetSelectedColor(0.0, 1.0, 1.0);
      }
    }
  }
  if (!sourceMarkers || !targetMarkers)
  {
    QMessageBox::critical(this, "오류", "마커 노드를 생성하지 못했습니다.");
    return;
  }

  const bool startPlacement = markerNodesMissing ||
    (sourceMarkers->GetNumberOfControlPoints() == 0
     && targetMarkers->GetNumberOfControlPoints() == 0);
  if (startPlacement)
  {
    vtkMRMLSelectionNode* selectionNode = vtkMRMLSelectionNode::SafeDownCast(
      this->mrmlScene()->GetNodeByID("vtkMRMLSelectionNodeSingleton"));
    vtkMRMLInteractionNode* interactionNode = vtkMRMLInteractionNode::SafeDownCast(
      this->mrmlScene()->GetNodeByID("vtkMRMLInteractionNodeSingleton"));
    if (selectionNode)
    {
      selectionNode->SetReferenceActivePlaceNodeClassName("vtkMRMLMarkupsFiducialNode");
      selectionNode->SetActivePlaceNodeID(sourceMarkers->GetID());
    }
    if (interactionNode)
    {
      interactionNode->SetCurrentInteractionMode(vtkMRMLInteractionNode::Place);
    }
    QMessageBox::information(this, "마커 배치 안내",
      "마커 노드를 생성하고 점 찍기 모드를 켰습니다.\n\n"
      "1. 이동 골편 표면에 대응점 3개 이상을 찍습니다.\n"
      "2. 이어서 기준 모델의 같은 위치에 같은 순서로 점을 찍습니다.\n"
      "3. 모두 찍은 뒤 이 버튼을 다시 누르세요.\n\n"
      "한 노드에 점을 연속으로 찍어도 짝수 개이면 앞/뒤 절반으로 자동 분리합니다.");
    return;
  }

  const int sourceCount = sourceMarkers->GetNumberOfControlPoints();
  if (sourceCount >= 6 && targetMarkers->GetNumberOfControlPoints() == 0
      && sourceCount % 2 == 0)
  {
    const int halfCount = sourceCount / 2;
    for (int index = halfCount; index < sourceCount; ++index)
    {
      double position[3] = {0.0, 0.0, 0.0};
      sourceMarkers->GetNthControlPointPositionWorld(index, position);
      targetMarkers->AddControlPointWorld(vtkVector3d(position[0], position[1], position[2]));
    }
    for (int index = sourceCount - 1; index >= halfCount; --index)
    {
      sourceMarkers->RemoveNthControlPoint(index);
    }
  }

  const int finalSourceCount = sourceMarkers->GetNumberOfControlPoints();
  const int finalTargetCount = targetMarkers->GetNumberOfControlPoints();
  if (finalSourceCount < 3 || finalTargetCount < 3)
  {
    QMessageBox::warning(this, "경고", "Source와 Target에 각각 최소 3개의 대응점이 필요합니다.");
    return;
  }
  if (finalSourceCount != finalTargetCount)
  {
    QMessageBox::warning(this, "경고", "Source와 Target 마커의 점 개수가 일치하지 않습니다.");
    return;
  }

  const bool success = d->Logic->RunLandmarkRegistration(
    sourceNode, targetNode, sourceMarkers, targetMarkers);
  if (success)
  {
    QMessageBox::information(this, "성공", "마커 기반 랜드마크 정합을 완료했습니다.");
    this->updateFragmentTable();
  }
  else
  {
    QMessageBox::critical(this, "실패",
      "대응점이 일직선에 가깝거나 변환 행렬 계산에 실패했습니다. 점 위치와 순서를 확인하세요.");
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onRunMaskMatchClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->Logic || !d->activeSegmentationSelector || !this->mrmlScene()
      || !d->guideModelSelector)
  {
    return;
  }

  vtkMRMLModelNode* guideNode = vtkMRMLModelNode::SafeDownCast(
    d->guideModelSelector->currentNode());
  if (!guideNode)
  {
    QMessageBox::warning(this, "경고", "먼저 가이드 모델 또는 기준 골편을 선택하세요.");
    return;
  }

  vtkMRMLModelNode* fragmentNode = this->selectedFragmentNode();
  if (!fragmentNode)
  {
    QMessageBox::warning(this, "경고", "골편 표에서 움직일 대상 골편을 선택하세요.");
    return;
  }
  if (fragmentNode == guideNode)
  {
    QMessageBox::warning(this, "경고", "이동 골편과 기준 모델은 서로 달라야 합니다.");
    return;
  }

  vtkMRMLSegmentationNode* segNode = vtkMRMLSegmentationNode::SafeDownCast(
    d->activeSegmentationSelector->currentNode());
  if (!segNode || !segNode->GetSegmentation())
  {
    QMessageBox::warning(this, "경고", "마스크를 저장할 세그멘테이션 노드를 선택하세요.");
    return;
  }

  vtkSegmentation* segmentation = segNode->GetSegmentation();
  std::string sourceId = segmentation->GetSegmentIdBySegmentName("Targeting_Source_ROI");
  std::string targetId = segmentation->GetSegmentIdBySegmentName("Targeting_Target_ROI");
  if (sourceId.empty() || targetId.empty())
  {
    double yellowColor[3] = {1.0, 1.0, 0.0};
    double cyanColor[3] = {0.0, 1.0, 1.0};
    if (sourceId.empty())
    {
      segmentation->AddEmptySegment("Targeting_Source_ROI", "Targeting_Source_ROI", yellowColor);
    }
    if (targetId.empty())
    {
      segmentation->AddEmptySegment("Targeting_Target_ROI", "Targeting_Target_ROI", cyanColor);
    }
    QMessageBox::information(this, "안내",
      "Targeting_Source_ROI와 Targeting_Target_ROI 세그먼트를 생성했습니다.\n\n"
      "Segment Editor에서 이동 골편과 기준 모델의 대응 표면 영역을 각각 색칠한 뒤 다시 실행하세요.");
    return;
  }

  const bool success = d->Logic->RunMaskedIcpRegistration(
    fragmentNode, guideNode, segNode, sourceId, targetId);
  if (success)
  {
    QMessageBox::information(this, "성공", "색칠 영역 기반 마스킹 ICP 정합을 완료했습니다.");
    this->updateFragmentTable();
  }
  else
  {
    QMessageBox::critical(this, "실패",
      "마스크가 닫힌 표면을 만들지 못했거나 내부 표면점이 부족합니다. 색칠 영역을 확인하세요.");
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::onClearMarkersClicked()
{
  if (!this->mrmlScene()) return;

  // Python 원본 동일: 노드를 삭제하지 않고 포인트만 제거(RemoveAllControlPoints)
  vtkMRMLMarkupsFiducialNode* srcFid = vtkMRMLMarkupsFiducialNode::SafeDownCast(
    this->mrmlScene()->GetFirstNodeByName(SOURCE_MARKERS_NAME));
  vtkMRMLMarkupsFiducialNode* tgtFid = vtkMRMLMarkupsFiducialNode::SafeDownCast(
    this->mrmlScene()->GetFirstNodeByName(TARGET_MARKERS_NAME));

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
    QMessageBox::information(this, "알림", "마커(점)가 모두 지워졌습니다. 이제 다시 정확한 위치에 순서대로 점을 찍어주세요.");
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

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModulePlannerDialog::addLogMessage(const QString& message)
{
  Q_D(qSlicerFemurFracturePlannerCppModulePlannerDialog);
  if (!d->reductionLogWidget)
  {
    return;
  }
  QString timeStr = QTime::currentTime().toString("hh:mm:ss");
  d->reductionLogWidget->append(QString("[%1] %2").arg(timeStr).arg(message));
  d->reductionLogWidget->ensureCursorVisible();
  
  // UI 실시간 갱신 강제 수행
  QCoreApplication::processEvents();
}
