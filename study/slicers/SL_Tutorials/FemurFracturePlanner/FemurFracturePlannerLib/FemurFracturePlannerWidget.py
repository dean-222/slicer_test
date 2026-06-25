"""
File Name: FemurFracturePlannerWidget.py
Version: v0-2.0.0
Date: 2026-06-24
Description: 대퇴골 골절 계획 모듈의 팝업 UI 생성, 위젯 생명주기 관리, 공통 이벤트 연결을 담당한다.

Version History:
- v0-2.0.0 (2026-06-24)
  - 뼈 마스킹 에지 검출 테스트용 버튼을 1.5 영상 처리 레이아웃에 동적으로 추가하고 바인딩하는 로직 구현 (X 증가, Y/Z 초기화)
- v0-1.1.0 (2026-06-24)
  - 팝업 창의 닫기 이벤트(closeEvent) 기각을 통한 메모리 유지 전략, 내장 Segment Editor의 Slicer 위젯 설정 가이드라인, 위젯 파괴 시 cleanup 소멸자 청소 과정 설명 보충 (Y 증가, Z 초기화)
- v0-1.0.2 (2026-06-24)
  - setup 및 enter 단계에서 세그멘테이션 노드와 소스 볼륨 노드가 연결될 때 SetReferenceImageGeometryParameterFromVolumeNode를 확실히 호출하도록 수정 (Z 증가)
- v0-1.0.1 (2026-06-24)
  - Threshold 적용 시 R/A/S 범위가 좁아지는 버그 수정: setup/enter에서 SetReferenceImageGeometryParameterFromVolumeNode 호출 추가
- v0-1.0.0 (2026-06-23)
  - 영상처리 버튼(영상 반전, 3D 엣지 검출) 클릭 이벤트 UI 바인딩 연동 (X 증가, Y/Z 초기화)
- v0-0.2.0 (2026-06-23)
  - 입력/렌더링/회전/가이드 모델 흐름 메서드를 별도 workflow mixin 파일로 분리하고 위젯 파일을 생명주기 중심으로 정리
- v0-0.1.0 (2026-06-23)
  - Fragment Manager 관련 메서드를 별도 mixin 파일로 분리하고 위젯 파일을 팝업/입력 흐름 중심으로 정리
- v0-0.0.0 (2026-06-23)
  - FemurFracturePlanner.py에서 PlannerDialog와 FemurFracturePlannerWidget 클래스를 분리하여 최초 작성
"""

import logging

import qt
import slicer
import vtk

from slicer.ScriptedLoadableModule import ScriptedLoadableModuleWidget
from slicer.util import VTKObservationMixin

from FemurFracturePlannerLib.FemurFracturePlannerFragmentManager import FemurFracturePlannerFragmentManagerMixin
from FemurFracturePlannerLib.FemurFracturePlannerLogic import FemurFracturePlannerLogic, FemurFracturePlannerParameterNode
from FemurFracturePlannerLib.FemurFracturePlannerWorkflow import FemurFracturePlannerWorkflowMixin

# 이 위젯 파일은 UI를 만들고 이벤트를 연결하는 "교통정리" 역할을 한다.
# 계산 자체는 Logic 파일에 맡기고, 입력/버튼 흐름은 Workflow와 FragmentManager mixin으로 나눈다.


class PlannerDialog(qt.QDialog):
    """닫기 버튼을 눌러도 삭제하지 않고 숨겨 두는 독립 계획 창이다."""

    def __init__(self, parent=None):
        super().__init__(parent)

    def closeEvent(self, event):
        """
        [팝업 창의 닫기 이벤트(closeEvent) 기각을 통한 메모리 유지 전략]
        1. 사용자가 독립 플래너 창 우측 상단의 'X' 버튼을 누르면, closeEvent가 발생한다.
        2. 기본 QDialog는 close 시 객체가 소멸(destroy)되거나 메모리가 반환될 수 있으나,
           여기서는 event.ignore()를 호출하여 소멸을 기각한다.
        3. 대신 self.hide()를 수행하여 창을 보이지 않게 숨김 처리만 한다.
        4. 이로써 사용자가 도중에 창을 닫더라도 기존에 선택한 세그멘테이션 데이터, 볼륨 상태,
           UI 콤보박스 선택값 등 모든 메모리 구조가 유지되고, 다시 열었을 때 작업을 그대로 이어갈 수 있다.
        """
        event.ignore()
        self.hide()


class FemurFracturePlannerWidget(
    ScriptedLoadableModuleWidget,
    VTKObservationMixin,
    FemurFracturePlannerWorkflowMixin,
    FemurFracturePlannerFragmentManagerMixin,
):
    """
    모듈의 메인 위젯 컨트롤러이다.

    이 클래스는 직접 계산을 많이 하기보다는,
    1. 팝업 창과 UI 위젯을 만들고,
    2. Slicer MRML 이벤트를 연결하고,
    3. Workflow/FragmentManager/Logic 계층으로 호출을 분배하는
    조정자 역할을 맡는다.
    """

    def __init__(self, parent=None) -> None:
        ScriptedLoadableModuleWidget.__init__(self, parent)
        VTKObservationMixin.__init__(self)

        # 로직과 파라미터 노드는 모듈 진입 후 setup/enter 과정에서 연결된다.
        self.logic = None
        self._parameterNode = None
        self._parameterNodeGuiTag = None

        # 팝업 창과 내장 Segment Editor 관련 객체를 별도로 유지한다.
        self.plannerDialog = None
        self.segmentEditorWidget = None
        self.segmentEditorNode = None

        # Threshold 효과가 활성화되면 창 높이를 일시적으로 확장한다.
        self.isThresholdActive = False

        # Fragment Manager에서 선택한 골편의 transform 관찰 상태를 추적한다.
        self._activeTransformNode = None
        self._activeTransformObserverTag = None
        self._activeModelNode = None

    def setup(self) -> None:
        """
        팝업 UI를 생성하고 모든 UI/Scene 이벤트를 연결한다.

        이 메서드는 모듈 패널이 처음 구성될 때 한 번 호출되며,
        사용자에게 보이는 독립 팝업 창과 내부 Segment Editor를 준비하는 핵심 진입점이다.
        """
        ScriptedLoadableModuleWidget.setup(self)

        # 실제 작업은 모듈 패널 내부가 아니라 독립 팝업 창에서 진행한다.
        # Slicer 모듈 패널은 공간이 좁기 때문에 복잡한 계획 UI를 별도 QDialog로 띄운다.
        self.plannerDialog = PlannerDialog(slicer.util.mainWindow())
        self.plannerDialog.setWindowTitle("Femur Fracture Planner 창")
        self.plannerDialog.setObjectName("FemurFracturePlannerDialog")
        self.plannerDialog.setModal(False)
        self.plannerDialog.setWindowFlags(
            qt.Qt.Window | qt.Qt.WindowTitleHint | qt.Qt.WindowCloseButtonHint | qt.Qt.CustomizeWindowHint
        )
        self.plannerDialog.resize(850, 830)

        dialogLayout = qt.QVBoxLayout(self.plannerDialog)
        # 여백을 작게 유지해서 Segment Editor처럼 큰 위젯이 창 안에 최대한 넓게 보이도록 한다.
        dialogLayout.setContentsMargins(5, 5, 5, 5)
        dialogLayout.setSizeConstraint(qt.QLayout.SetNoConstraint)

        # Qt Designer로 만든 .ui 파일을 읽어서 팝업 안에 배치한다.
        uiWidget = slicer.util.loadUI(self.resourcePath("UI/FemurFracturePlanner.ui"))
        uiWidget.setSizePolicy(qt.QSizePolicy.Expanding, qt.QSizePolicy.Expanding)
        uiWidget.setMaximumSize(16777215, 16777215)
        dialogLayout.addWidget(uiWidget)

        self.ui = slicer.util.childWidgetVariables(uiWidget)
        # .ui 안의 MRML selector 위젯들이 현재 Slicer 씬을 바라보도록 연결한다.
        # 이 연결이 없으면 노드 선택 콤보박스에 CT 볼륨이나 모델 노드가 나타나지 않는다.
        uiWidget.setMRMLScene(slicer.mrmlScene)

        # 모듈 패널에는 팝업 실행 버튼만 간단히 남겨 사용자 진입점을 제공한다.
        infoLabel = qt.QLabel("Femur Fracture Planner가 독립 팝업 창에서 실행 중입니다.")
        infoLabel.setStyleSheet("font-weight: bold; color: #2e7d32; margin-top: 15px; margin-bottom: 10px;")
        infoLabel.setWordWrap(True)
        self.layout.addWidget(infoLabel)

        self.openPlannerButton = qt.QPushButton("계획 창 열기")
        self.openPlannerButton.setFixedHeight(40)
        self.openPlannerButton.setStyleSheet("font-weight: bold; background-color: #e8f5e9;")
        self.openPlannerButton.connect("clicked(bool)", self.onOpenPlannerWindowClicked)
        self.layout.addWidget(self.openPlannerButton)

        self.logic = FemurFracturePlannerLogic()

        # Scene 단위 이벤트는 모듈 재진입, 노드 추가/삭제, Scene close 대응에 사용한다.
        # addObserver로 연결한 이벤트들은 cleanup/removeObservers에서 한 번에 해제된다.
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.StartCloseEvent, self.onSceneStartClose)
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.EndCloseEvent, self.onSceneEndClose)
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.NodeAddedEvent, self.onNodeAdded)
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.NodeRemovedEvent, self.onNodeRemoved)

        # 입력 볼륨, 렌더링, 회전과 관련된 UI 이벤트를 workflow mixin 메서드에 연결한다.
        # Qt의 connect는 "사용자가 UI를 조작함"과 "실행할 Python 메서드"를 이어 주는 배선이다.
        self.ui.inputVolumeSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onInputVolumeChanged)
        self.ui.volumeVisibilityButton.connect("toggled(bool)", self.onVolumeVisibilityToggled)
        self.ui.loadVolumeButton.connect("clicked(bool)", self.onLoadVolumeClicked)
        self.ui.loadDicomButton.connect("clicked(bool)", self.onLoadDicomClicked)

        # 1.5. 영상처리 필터 이벤트 연결
        self.ui.invertIntensityButton.connect("clicked(bool)", self.onInvertIntensityClicked)
        self.ui.edgeDetectionButton.connect("clicked(bool)", self.onEdgeDetectionClicked)

        # 뼈 마스킹 에지 검출 테스트용 버튼 동적 배치 및 바인딩
        if hasattr(self.ui, "imageProcessingCollapsibleButton") and hasattr(self.ui, "edgeDetectionButton"):
            self.ui.maskedEdgeDetectionButton = qt.QPushButton("마스킹 에지 검출 (Test)")
            self.ui.maskedEdgeDetectionButton.setToolTip("뼈 임계값 기준으로 먼저 마스킹을 수행한 뒤 3D 에지를 검출합니다.")
            self.ui.maskedEdgeDetectionButton.setStyleSheet(
                "font-weight: bold; background-color: #e3f2fd; color: #1565c0;"
            )
            self.ui.maskedEdgeDetectionButton.connect("clicked(bool)", self.onMaskedEdgeDetectionClicked)

            ipLayout = self.ui.imageProcessingCollapsibleButton.layout()
            if ipLayout:
                # 에지 검출 버튼이 배치된 가로 레이아웃(index 0) 바로 아래에 신규 버튼 삽입
                ipLayout.insertWidget(1, self.ui.maskedEdgeDetectionButton)

        self.ui.volumeRenderingVisibilityButton.connect("toggled(bool)", self.onVolumeRenderingToggled)
        self.ui.thresholdSlider.connect("valueChanged(double)", self.onThresholdSliderChanged)
        self.ui.rotationAngleSlider.connect("valueChanged(double)", self.onRotationAngleChanged)
        self.ui.resetRotationButton.connect("clicked(bool)", self.onResetRotationClicked)
        self.ui.rotate90Button.connect("clicked(bool)", self.onRotate90Clicked)
        self.ui.rotationXRadio.connect("toggled(bool)", self.onRotationAxisChanged)
        self.ui.rotationYRadio.connect("toggled(bool)", self.onRotationAxisChanged)
        self.ui.rotationZRadio.connect("toggled(bool)", self.onRotationAxisChanged)

        # 내장 Segment Editor는 UI 파일 안의 빈 자리(anchor widget)에 동적으로 꽂아 넣는다.
        # Slicer 기본 편집 기능을 그대로 재사용하므로 직접 페인트/임계값 도구를 구현하지 않아도 된다.
        self.createEmbeddedSegmentEditor()

        # Segment Editor가 사용할 MRML 노드를 보장한다.
        # SegmentEditorNode는 "현재 활성 효과, 선택 세그먼트, 도구 설정" 같은 편집 상태를 저장한다.
        if not self.segmentEditorNode:
            self.segmentEditorNode = slicer.mrmlScene.GetFirstNodeByName("FemurFracturePlanner_SegmentEditorNode")
            if not self.segmentEditorNode:
                self.segmentEditorNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLSegmentEditorNode")
                self.segmentEditorNode.SetName("FemurFracturePlanner_SegmentEditorNode")

        if self.segmentEditorWidget and self.segmentEditorNode:
            self.segmentEditorWidget.setMRMLSegmentEditorNode(self.segmentEditorNode)
            segNode = self.ui.activeSegmentationSelector.currentNode()
            if segNode:
                self.segmentEditorWidget.setSegmentationNode(segNode)

            inputNode = self.ui.inputVolumeSelector.currentNode()
            if inputNode:
                self.segmentEditorWidget.setSourceVolumeNode(inputNode)
                if segNode:
                    segNode.SetReferenceImageGeometryParameterFromVolumeNode(inputNode)

        # Threshold 효과가 열리고 닫힐 때 창 높이를 감시하기 위한 타이머이다.
        # Segment Editor 효과 UI는 사용자가 효과를 바꿀 때 내부에서 동적으로 바뀌므로,
        # 짧은 주기로 확인해서 Threshold 옵션이 잘리지 않도록 창 높이를 조절한다.
        self.resizeMonitorTimer = qt.QTimer()
        self.resizeMonitorTimer.setInterval(200)
        self.resizeMonitorTimer.connect("timeout()", self.monitorActiveEffectAndResize)

        # 골편 분리, 가이드 모델, Fragment Manager 관련 UI 이벤트를 연결한다.
        # 이 영역의 핸들러들은 대부분 WorkflowMixin 또는 FragmentManagerMixin에 정의되어 있다.
        self.ui.separateFragmentsButton.connect("clicked(bool)", self.onSeparateFragmentsClicked)
        self.ui.fragmentSeparationCollapsibleButton.setVisible(True)
        self.ui.activeSegmentationSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onSegmentationNodeChanged)
        self.segmentEditorWidget.connect("segmentationNodeChanged(vtkMRMLSegmentationNode*)", self.onEditorSegmentationNodeChanged)
        self.ui.mirrorGuideButton.connect("clicked(bool)", self.onMirrorGuideClicked)
        self.ui.runIcpReductionButton.connect("clicked(bool)", self.onRunIcpReductionClicked)
        self.ui.runSurfaceSnapButton.connect("clicked(bool)", self.onRunSurfaceSnapClicked)
        self.ui.loadGuideButton.connect("clicked(bool)", self.onLoadGuideClicked)
        self.ui.guideVisibilityButton.connect("toggled(bool)", self.onGuideVisibilityToggled)
        self.ui.guideModelSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onGuideModelChanged)
        self.ui.refreshManagerButton.connect("clicked(bool)", self.updateFragmentTable)
        self.ui.clearModelsButton.connect("clicked(bool)", self.onClearModelsClicked)
        self.ui.fragmentTableWidget.connect("itemChanged(QTableWidgetItem*)", self.onFragmentTableItemChanged)
        self.ui.fragmentTableWidget.connect("itemSelectionChanged()", self.onFragmentTableSelectionChanged)

        # 처음 열릴 때 이미 Threshold 효과가 활성화되어 있으면 창 높이를 먼저 맞춘다.
        self.isThresholdActive = False
        if self.segmentEditorWidget:
            try:
                # activeEffect()는 상황에 따라 None을 반환할 수 있으므로 안전하게 확인한다.
                effect = self.segmentEditorWidget.activeEffect()
                effectName = effect.name if effect else ""
            except Exception:
                effectName = ""

            if effectName == "Threshold":
                self.isThresholdActive = True
                self.plannerDialog.setMinimumHeight(960)
                self.plannerDialog.resize(850, 960)
            else:
                self.plannerDialog.setMinimumHeight(830)
                self.plannerDialog.resize(850, 830)

    def createEmbeddedSegmentEditor(self) -> None:
        """
        Slicer의 Segment Editor 위젯을 모듈 팝업 안에 내장한다.

        [내장 Segment Editor의 Slicer 위젯 설정 가이드라인]
        1. Slicer 기본 모듈에서 qMRMLSegmentEditorWidget 인스턴스를 동적으로 생성하고,
           UI 템플릿의 `segmentEditorAnchorWidget` 레이아웃에 직접 삽입(embed)한다.
        2. 중복 제어를 막기 위해 source volume 선택 콤보박스는 False로 숨기고,
           상단의 inputVolumeSelector와 바인딩되도록 통제한다.
        3. 세그멘테이션 선택기는 노출(True)시켜 사용자가 여러 세그멘테이션 노드를 전환할 수 있도록 한다.
        4. 대퇴골 골절 수술 계획 시 필요한 도구들만 엄선하여 세팅(`setEffectNameOrder`)한다:
           - Paint, Draw, Erase (영역 칠하기 및 지우기)
           - Level tracing (자동 경계면 감지 페인팅)
           - Threshold (하운스필드 단위 밀도 기반 자동 뼈 영역 분류)
           - Margin, Hollow, Scissors (형상 가공 보조 도구)
        """
        logging.info("Initializing embedded Segment Editor widget...")

        # Slicer 기본 Segment Editor 모듈 표현을 먼저 준비해 두어 관련 내부 클래스가 로드되도록 한다.
        slicer.modules.segmenteditor.widgetRepresentation()
        self.segmentEditorWidget = slicer.qMRMLSegmentEditorWidget()
        self.segmentEditorWidget.setMRMLScene(slicer.mrmlScene)
        self.ui.segmentEditorAnchorWidget.layout().addWidget(self.segmentEditorWidget)

        # 세그멘테이션 선택은 사용자가 볼 수 있게 남기고,
        # source volume 선택은 상단 입력 볼륨 selector와 중복되므로 숨긴다.
        self.segmentEditorWidget.setSegmentationNodeSelectorVisible(True)
        self.segmentEditorWidget.setSourceVolumeNodeSelectorVisible(False)
        self.segmentEditorWidget.setEffectNameOrder(
            ["Paint", "Draw", "Erase", "Level tracing", "Threshold", "Margin", "Hollow", "Scissors"]
        )

    def cleanup(self) -> None:
        """
        모듈이 내려갈 때 타이머, observer, 임베디드 위젯을 정리한다.

        [위젯 파괴 시 cleanup 소멸자 청소 과정]
        1. Slicer는 모듈의 리로딩(Reload)이 잦으며, 씬이 바뀔 때 기존 GUI 객체가 참조를 보존하고 있으면 메모리 누수와
           'C++ object already deleted' 런타임 크래시가 유발된다.
        2. 따라서 cleanup 단계에서 다음 소멸 파이프라인을 정확한 순서로 수행해야 한다:
           - 활성 골편 변환 옵저버(RemoveObserver) 제거
           - 리사이즈 감시 타이머 정지(stop) 및 None 대입
           - GUI와 Parameter Node 간 바인딩 해제(disconnectGui)
           - 씬(Scene)에서 생성한 VTK Observer 해제(removeObservers)
           - 임베디드 Segment Editor의 씬 바인딩 해제 및 메모리 반환(deleteLater)
           - 독립 팝업 다이얼로그 닫기(close), 메모리 파괴 지정(deleteLater), 참조 Null화
        """
        self.removeActiveTransformObserver()
        if hasattr(self, "resizeMonitorTimer") and self.resizeMonitorTimer:
            self.resizeMonitorTimer.stop()
            self.resizeMonitorTimer = None

        # UI와 MRML 노드 사이의 양방향 바인딩을 먼저 끊은 뒤 위젯/observer를 정리한다.
        # 순서를 지키면 이미 삭제된 Qt 위젯으로 이벤트가 들어오는 상황을 줄일 수 있다.
        self.setParameterNode(None)
        self.removeObservers()
        self.destroyEmbeddedSegmentEditor()
        if self.plannerDialog:
            self.plannerDialog.close()
            self.plannerDialog.deleteLater()
            self.plannerDialog = None

    def destroyEmbeddedSegmentEditor(self) -> None:
        """내장 Segment Editor와 관련 MRML 노드를 안전하게 제거한다."""
        if hasattr(self, "_editorNodeObserverTag") and self._editorNodeObserverTag is not None:
            if self.segmentEditorNode:
                self.segmentEditorNode.RemoveObserver(self._editorNodeObserverTag)
            self._editorNodeObserverTag = None

        if self.segmentEditorWidget:
            # Slicer MRML scene과의 연결을 끊어야 reload 후에도 이전 위젯이 씬 이벤트를 받지 않는다.
            self.segmentEditorWidget.setMRMLSegmentEditorNode(None)
            self.segmentEditorWidget.setMRMLScene(None)
            self.segmentEditorWidget.deleteLater()
            self.segmentEditorWidget = None

        if self.segmentEditorNode:
            slicer.mrmlScene.RemoveNode(self.segmentEditorNode)
            self.segmentEditorNode = None

    def enter(self) -> None:
        """
        사용자가 모듈에 진입할 때 현재 Scene 상태를 UI에 다시 연결한다.

        reload 이후에도 Segment Editor와 파라미터 노드가 정상 동작하도록
        필요한 MRML 노드와 UI 선택 상태를 다시 보정한다.
        """
        self.initializeParameterNode()

        if not self.segmentEditorNode:
            self.segmentEditorNode = slicer.mrmlScene.GetFirstNodeByName("FemurFracturePlanner_SegmentEditorNode")
            if not self.segmentEditorNode:
                self.segmentEditorNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLSegmentEditorNode")
                self.segmentEditorNode.SetName("FemurFracturePlanner_SegmentEditorNode")

        if not hasattr(self, "_editorNodeObserverTag") or self._editorNodeObserverTag is None:
            # Segment Editor의 활성 효과가 바뀌면 ModifiedEvent가 발생한다.
            # 이 이벤트를 이용해 Threshold 효과가 켜졌는지 감시한다.
            self._editorNodeObserverTag = self.segmentEditorNode.AddObserver(vtk.vtkCommand.ModifiedEvent, self.onEditorNodeModified)

        segNode = self.ui.activeSegmentationSelector.currentNode()
        if not segNode:
            # 사용자가 아직 segmentation을 만들지 않았으면 기본 노드를 하나 만들어 둔다.
            # 그래야 Segment Editor가 바로 편집 가능한 상태로 시작된다.
            segNode = slicer.mrmlScene.GetFirstNodeByClass("vtkMRMLSegmentationNode")
            if not segNode:
                segNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLSegmentationNode")
                segNode.SetName("Femur_Segmentation")
                segNode.CreateDefaultDisplayNodes()
            self.ui.activeSegmentationSelector.setCurrentNode(segNode)

        if self.segmentEditorWidget:
            self.segmentEditorWidget.setSegmentationNode(segNode)
            inputNode = self.ui.inputVolumeSelector.currentNode()
            if inputNode:
                self.segmentEditorWidget.setSourceVolumeNode(inputNode)

        self.updateFragmentTable()

        if self.plannerDialog:
            self.plannerDialog.show()
            self.plannerDialog.raise_()
            self.plannerDialog.activateWindow()

        if self.segmentEditorWidget and self.segmentEditorNode:
            self.segmentEditorWidget.setMRMLSegmentEditorNode(self.segmentEditorNode)
            if segNode:
                self.segmentEditorWidget.setSegmentationNode(segNode)
            inputNode = self.ui.inputVolumeSelector.currentNode()
            if inputNode:
                self.segmentEditorWidget.setSourceVolumeNode(inputNode)
                if segNode:
                    segNode.SetReferenceImageGeometryParameterFromVolumeNode(inputNode)

        if hasattr(self, "resizeMonitorTimer") and self.resizeMonitorTimer:
            # enter 시점부터만 타이머를 돌려 불필요한 백그라운드 감시를 줄인다.
            self.resizeMonitorTimer.start()

    def exit(self) -> None:
        """모듈을 벗어날 때 observer와 임시 연결 상태를 해제한다."""
        self.removeActiveTransformObserver()
        if hasattr(self, "resizeMonitorTimer") and self.resizeMonitorTimer:
            self.resizeMonitorTimer.stop()

        if self._parameterNode:
            # connectGui로 묶었던 UI-parameter node 연결을 해제한다.
            self._parameterNode.disconnectGui(self._parameterNodeGuiTag)
            self._parameterNodeGuiTag = None

        if hasattr(self, "_editorNodeObserverTag") and self._editorNodeObserverTag is not None:
            if self.segmentEditorNode:
                self.segmentEditorNode.RemoveObserver(self._editorNodeObserverTag)
            self._editorNodeObserverTag = None

        if self.segmentEditorWidget:
            self.segmentEditorWidget.setMRMLSegmentEditorNode(None)

        if self.segmentEditorNode:
            slicer.mrmlScene.RemoveNode(self.segmentEditorNode)
            self.segmentEditorNode = None
            self.plannerDialog.hide()

    def onSceneStartClose(self, caller, event) -> None:
        """Scene close 시작 시 파라미터와 내장 Editor 연결을 먼저 정리한다."""
        self.setParameterNode(None)
        self.destroyEmbeddedSegmentEditor()
        if self.plannerDialog:
            self.plannerDialog.hide()

    def onSceneEndClose(self, caller, event) -> None:
        """Scene close 후 모듈이 여전히 열려 있으면 필요한 UI/노드를 다시 준비한다."""
        if self.parent.isEntered:
            self.initializeParameterNode()
            self.createEmbeddedSegmentEditor()
            if self.plannerDialog:
                self.plannerDialog.show()

    def initializeParameterNode(self) -> None:
        """
        모듈 전용 parameter node를 연결하고 기본 입력 볼륨을 보정한다.

        사용자가 아직 아무 입력도 선택하지 않았다면 Scene의 첫 번째 scalar volume을 기본값으로 사용한다.
        """
        self.setParameterNode(self.logic.getParameterNode())
        if not self._parameterNode.inputVolume:
            firstVolumeNode = slicer.mrmlScene.GetFirstNodeByClass("vtkMRMLScalarVolumeNode")
            if firstVolumeNode:
                self._parameterNode.inputVolume = firstVolumeNode

    def setParameterNode(self, inputParameterNode: FemurFracturePlannerParameterNode | None) -> None:
        """기존 GUI-parameter 연결을 해제하고 새 parameter node를 다시 연결한다."""
        if self._parameterNode:
            self._parameterNode.disconnectGui(self._parameterNodeGuiTag)

        self._parameterNode = inputParameterNode
        if self._parameterNode:
            self._parameterNodeGuiTag = self._parameterNode.connectGui(self.ui)

    def onEditorNodeModified(self, caller, event) -> None:
        """Segment Editor 상태 변경 시 Threshold 패널 높이 감시 로직을 다시 실행한다."""
        self.monitorActiveEffectAndResize()
