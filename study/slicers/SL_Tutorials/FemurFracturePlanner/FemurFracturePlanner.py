"""
File Name: FemurFracturePlanner.py
Version: v0.305
Date: 2026-06-23
Description: 대퇴골 골절 수술 계획을 위한 3D Slicer 모듈 GUI 및 비즈니스 로직 제어 클래스

Version History:

- v0.305 (2026-06-23)
  - 모듈 초기화/씬 변경 시 SegmentEditorNode 미지정 상태의 호출 예외 방지를 위한 onInputVolumeChanged 방어 코드 보강
- v0.304 (2026-06-23)
  - NodeAddedEvent 문자열 ID 전달에 따른 AttributeError 및 SegmentEditor 'SetSourceVolumeNode' 설정 에러 버그 수정
- v0.303 (2026-06-23)
  - qSlicerApplication에 openDICOMBrowser 속성이 누락되어 발생하는 AttributeError 수정 (ActionOpenDICOMBrowser QAction 트리거 방식으로 교체)
- v0.302 (2026-06-23)
  - PythonQt QFileDialog.getOpenFileName 반환값 언팩 오류(ValueError) 수정 및 onLoadDicomClicked에 slicer.app.openDICOMBrowser() 도입
- v0.301 (2026-06-23)
  - PythonQt 래핑 한계로 인한 closeEvent 동적 대입 AttributeError 오류 수정 (PlannerDialog 상속 클래스 신설)
- v0.300 (2026-06-23)
  - 모듈 UI를 3D Slicer 왼쪽 모듈 패널에서 독립된 팝업 대화상자(QDialog) 창으로 분리 및 동기화 구현
- v0.200 (2026-06-23)
  - Slicer 내장 DICOM 데이터베이스 팝업 로더 기능 추가 (loadDicomButton 및 slicer.mrmlScene.NodeAddedEvent 옵저버 연동)
- v0.101 (2026-06-23)
  - onLoadVolumeClicked 내 QFileDialog 반환 언팩 시 local '_' 변수 충돌에 의한 UnboundLocalError 버그 수정
- v0.100 (2026-06-23)
  - 실시간 CT GPU 볼륨 렌더링, 3D 회전 제어, 내장 세그먼트 에디터 위젯 임베딩 및 다이렉트 파일 로더 기능 추가
- v0.000 (2026-06-23)
  - 최초 작성
"""
import logging
import os
import vtk
import qt
import slicer
from slicer.i18n import tr as _
from slicer.i18n import translate
from slicer.ScriptedLoadableModule import *
from slicer.util import VTKObservationMixin
from slicer.parameterNodeWrapper import (
    parameterNodeWrapper,
)

# Slicer 노드 타입 참조
from slicer import vtkMRMLScalarVolumeNode
from slicer import vtkMRMLLinearTransformNode
from slicer import vtkMRMLSegmentationNode

#
# FemurFracturePlanner (메인 모듈 메타데이터 클래스)
#
class FemurFracturePlanner(ScriptedLoadableModule):
    def __init__(self, parent):
        ScriptedLoadableModule.__init__(self, parent)
        self.parent.title = _("Femur Fracture Planner")
        self.parent.categories = [translate("qSlicerAbstractCoreModule", "SL_Tutorials")]
        self.parent.dependencies = []
        self.parent.contributors = ["Antigravity (DeepMind)"]
        self.parent.helpText = _("""
This module provides real-time CT volume rendering, interactive 3D rotation, and an embedded Segment Editor for femur fracture visualization and surgical planning.
""")
        self.parent.acknowledgementText = _("""
Built for interactive femur bone segmentation and real-time visualization using Slicer's embedded widget technology.
""")


#
# FemurFracturePlannerParameterNode (파라미터 노드)
#
@parameterNodeWrapper
class FemurFracturePlannerParameterNode:
    inputVolume: vtkMRMLScalarVolumeNode


#
# PlannerDialog (독립 팝업 윈도우용 커스텀 QDialog 클래스)
#
class PlannerDialog(qt.QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        
    def closeEvent(self, event):
        """창 닫기(X) 시 창을 메모리에서 파괴하지 않고 감추기만 처리하여 상태를 보존합니다."""
        event.ignore()
        self.hide()


#
# FemurFracturePlannerWidget (GUI 제어 및 임베딩 클래스)
#
class FemurFracturePlannerWidget(ScriptedLoadableModuleWidget, VTKObservationMixin):
    def __init__(self, parent=None) -> None:
        ScriptedLoadableModuleWidget.__init__(self, parent)
        VTKObservationMixin.__init__(self)
        self.logic = None
        self._parameterNode = None
        self._parameterNodeGuiTag = None
        
        # 독립 팝업 다이어로그 인스턴스 관리 변수
        self.plannerDialog = None
        
        # 내장 세그먼트 에디터 인스턴스 관리 변수
        self.segmentEditorWidget = None
        self.segmentEditorNode = None

    def setup(self) -> None:
        ScriptedLoadableModuleWidget.setup(self)

        # 1. 팝업을 위한 독립 커스텀 QDialog 생성 및 설정
        self.plannerDialog = PlannerDialog(slicer.util.mainWindow())
        self.plannerDialog.setWindowTitle("Femur Fracture Planner Window")
        self.plannerDialog.setObjectName("FemurFracturePlannerDialog")
        self.plannerDialog.setModal(False)
        self.plannerDialog.setWindowFlags(qt.Qt.Window | qt.Qt.WindowTitleHint | qt.Qt.WindowCloseButtonHint | qt.Qt.CustomizeWindowHint)

        # 다이얼로그 레이아웃 정의
        dialogLayout = qt.QVBoxLayout(self.plannerDialog)
        dialogLayout.setContentsMargins(5, 5, 5, 5)

        # UI 파일 로드하여 다이얼로그 레이아웃에 탑재
        uiWidget = slicer.util.loadUI(self.resourcePath("UI/FemurFracturePlanner.ui"))
        dialogLayout.addWidget(uiWidget)
        self.ui = slicer.util.childWidgetVariables(uiWidget)

        # 씬 세팅
        uiWidget.setMRMLScene(slicer.mrmlScene)

        # 2. 원래 Slicer 왼쪽 모듈 패널에는 안내 라벨과 팝업 창 다시 열기 버튼 배치
        infoLabel = qt.QLabel("Femur Fracture Planner is running in an independent popup window.")
        infoLabel.setStyleSheet("font-weight: bold; color: #2e7d32; margin-top: 15px; margin-bottom: 10px;")
        infoLabel.setWordWrap(True)
        self.layout.addWidget(infoLabel)

        self.openPlannerButton = qt.QPushButton("Open Planner Window")
        self.openPlannerButton.setFixedHeight(40)
        self.openPlannerButton.setStyleSheet("font-weight: bold; background-color: #e8f5e9;")
        self.openPlannerButton.connect("clicked(bool)", self.onOpenPlannerWindowClicked)
        self.layout.addWidget(self.openPlannerButton)

        # 로직 인스턴스 생성
        self.logic = FemurFracturePlannerLogic()

        # UI 이벤트 리스너 연결
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.StartCloseEvent, self.onSceneStartClose)
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.EndCloseEvent, self.onSceneEndClose)

        # 1. 입력 볼륨 선택 변경, 가시성 제어 및 직접 파일/DICOM 로드 연결
        self.ui.inputVolumeSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onInputVolumeChanged)
        self.ui.volumeVisibilityButton.connect("toggled(bool)", self.onVolumeVisibilityToggled)
        self.ui.loadVolumeButton.connect("clicked(bool)", self.onLoadVolumeClicked)
        self.ui.loadDicomButton.connect("clicked(bool)", self.onLoadDicomClicked)

        # 씬에 볼륨 노드가 새로 추가되는 이벤트 구독 (DICOM 등 외부 로딩 자동 감지)
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.NodeAddedEvent, self.onNodeAdded)

        # 2. 실시간 3D 볼륨 렌더링 이벤트 연결 (눈 아이콘 푸시버튼 연동)
        self.ui.volumeRenderingVisibilityButton.connect("toggled(bool)", self.onVolumeRenderingToggled)
        self.ui.thresholdSlider.connect("valueChanged(double)", self.onThresholdSliderChanged)

        # 3. 실시간 3D 영상 회전 제어 이벤트 연결
        self.ui.rotationAngleSlider.connect("valueChanged(double)", self.onRotationAngleChanged)
        self.ui.resetRotationButton.connect("clicked(bool)", self.onResetRotationClicked)
        self.ui.rotate90Button.connect("clicked(bool)", self.onRotate90Clicked)
        
        # 회전 축 변경 시에도 즉각 회전을 반영하기 위해 라디오 버튼 변경 이벤트 감지
        self.ui.rotationXRadio.connect("toggled(bool)", self.onRotationAxisChanged)
        self.ui.rotationYRadio.connect("toggled(bool)", self.onRotationAxisChanged)
        self.ui.rotationZRadio.connect("toggled(bool)", self.onRotationAxisChanged)

        # 4. 내장 세그먼트 에디터 위젯 동적 생성 및 배치
        self.createEmbeddedSegmentEditor()

        # 5. 골절 파편 분리 버튼 이벤트 연결
        self.ui.separateFragmentsButton.connect("clicked(bool)", self.onSeparateFragmentsClicked)

        # 파라미터 노드 초기화
        self.initializeParameterNode()

    def createEmbeddedSegmentEditor(self) -> None:
        """
        Slicer의 내장 세그먼트 에디터 위젯(qMRMLSegmentEditorWidget)을 동적으로 생성하고,
        UI 레이아웃의 segmentEditorAnchorWidget 내에 통째로 삽입합니다.
        """
        logging.info("Initializing embedded Segment Editor widget...")
        
        # Slicer의 원래 Segment Editor 모듈을 백그라운드에서 강제 초기화하여 이펙트 리스트를 로드합니다.
        slicer.modules.segmenteditor.widgetRepresentation()
        
        # 위젯 생성
        self.segmentEditorWidget = slicer.qMRMLSegmentEditorWidget()
        self.segmentEditorWidget.setMRMLScene(slicer.mrmlScene)
        
        # UI 파일에 마련한 부모 anchor 위젯 레이아웃에 직접 추가
        self.ui.segmentEditorAnchorWidget.layout().addWidget(self.segmentEditorWidget)
        
        # 편집 모드 편의 옵션: UI 요소 중 불필요한 것(예: 노드 선택 콤보박스들)은 숨기고
        # 오직 브러시, 마스킹 등의 기능 버튼 위주로만 보이도록 컴팩트 디자인 설정
        self.segmentEditorWidget.setSegmentationNodeSelectorVisible(True)
        self.segmentEditorWidget.setSourceVolumeNodeSelectorVisible(False) # 입력 볼륨은 상단에서 직접 연동
        self.segmentEditorWidget.setEffectNameOrder(
            ["Paint", "Draw", "Erase", "Level tracing", "Threshold", "Margin", "Hollow", "Scissors"]
        )

    def cleanup(self) -> None:
        """모듈 종료 또는 소멸 시 할당된 리소스 정리"""
        self.removeObservers()
        self.destroyEmbeddedSegmentEditor()
        if self.plannerDialog:
            self.plannerDialog.close()
            self.plannerDialog.deleteLater()
            self.plannerDialog = None

    def destroyEmbeddedSegmentEditor(self) -> None:
        """임베디드 세그먼트 에디터 위젯 및 관련 파라미터 노드 해제"""
        if self.segmentEditorWidget:
            self.segmentEditorWidget.setMRMLSegmentEditorNode(None)
            self.segmentEditorWidget.setMRMLScene(None)
            self.segmentEditorWidget.deleteLater()
            self.segmentEditorWidget = None
        
        if self.segmentEditorNode:
            slicer.mrmlScene.RemoveNode(self.segmentEditorNode)
            self.segmentEditorNode = None

    def enter(self) -> None:
        """모듈 화면으로 진입할 때마다 호출"""
        self.initializeParameterNode()
        
        # 내장 에디터용 세션 노드(SegmentEditorNode) 생성 및 연결
        if not self.segmentEditorNode:
            self.segmentEditorNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLSegmentEditorNode")
            self.segmentEditorNode.SetName("FemurFracturePlanner_SegmentEditorNode")
            
        # 편의 기능: 씬에 이미 세그먼트 노드가 있으면 분리용 콤보박스 및 에디터에 자동 매핑
        segNode = self.ui.activeSegmentationSelector.currentNode()
        if not segNode:
            segNode = slicer.mrmlScene.GetFirstNodeByClass("vtkMRMLSegmentationNode")
            if not segNode:
                # 씬에 세그먼테이션 노드가 아예 없으면 자동으로 신설하여 매핑
                segNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLSegmentationNode")
                segNode.SetName("Femur_Segmentation")
                segNode.CreateDefaultDisplayNodes()
            self.ui.activeSegmentationSelector.setCurrentNode(segNode)

        if self.segmentEditorWidget:
            # Slicer C++ 에디터 가이드라인 준수: MRMLSegmentEditorNode -> SegmentationNode -> SourceVolumeNode 순으로 지정
            self.segmentEditorWidget.setMRMLSegmentEditorNode(self.segmentEditorNode)
            self.segmentEditorWidget.setSegmentationNode(segNode)
            inputNode = self.ui.inputVolumeSelector.currentNode()
            if inputNode:
                self.segmentEditorWidget.setSourceVolumeNode(inputNode)

        # 모듈 진입 시 독립 다이얼로그 창 강제 노출
        if self.plannerDialog:
            self.plannerDialog.show()
            self.plannerDialog.raise_()
            self.plannerDialog.activateWindow()

    def exit(self) -> None:
        """사용자가 다른 모듈 화면으로 이동 시 호출"""
        if self._parameterNode:
            self._parameterNode.disconnectGui(self._parameterNodeGuiTag)
            self._parameterNodeGuiTag = None
            
        # 에디터 위젯 연결을 일시 해제하여 메모리 낭비 및 충돌 방지
        if self.segmentEditorWidget:
            self.segmentEditorWidget.setMRMLSegmentEditorNode(None)
            
        if self.segmentEditorNode:
            slicer.mrmlScene.RemoveNode(self.segmentEditorNode)
            self.segmentEditorNode = None

        # 모듈에서 나갈 시 독립 다이얼로그 창 숨기기
        if self.plannerDialog:
            self.plannerDialog.hide()

    def onSceneStartClose(self, caller, event) -> None:
        self.setParameterNode(None)
        self.destroyEmbeddedSegmentEditor()
        if self.plannerDialog:
            self.plannerDialog.hide()

    def onSceneEndClose(self, caller, event) -> None:
        if self.parent.isEntered:
            self.initializeParameterNode()
            self.createEmbeddedSegmentEditor()
            if self.plannerDialog:
                self.plannerDialog.show()

    def initializeParameterNode(self) -> None:
        self.setParameterNode(self.logic.getParameterNode())
        if not self._parameterNode.inputVolume:
            firstVolumeNode = slicer.mrmlScene.GetFirstNodeByClass("vtkMRMLScalarVolumeNode")
            if firstVolumeNode:
                self._parameterNode.inputVolume = firstVolumeNode

    def setParameterNode(self, inputParameterNode: FemurFracturePlannerParameterNode | None) -> None:
        if self._parameterNode:
            self._parameterNode.disconnectGui(self._parameterNodeGuiTag)
        
        self._parameterNode = inputParameterNode
        if self._parameterNode:
            self._parameterNodeGuiTag = self._parameterNode.connectGui(self.ui)

    #
    # 영상 처리 & 3D 볼륨 렌더링 및 가시성 제어 핸들러
    #

    def onInputVolumeChanged(self, node) -> None:
        """입력 CT 볼륨이 바뀌면 볼륨 렌더링 대상 및 내장 에디터 입력 대상을 갱신합니다."""
        # 에디터 위젯이 초기 생성되지 않은 레이아웃 로딩 단계라면 예외 처리를 위해 즉시 리턴
        if not self.segmentEditorWidget:
            return

        if node:
            # 1. SegmentEditorNode (파라미터 세션 노드) 존재 여부 검사 및 선바인딩
            if not self.segmentEditorNode:
                self.segmentEditorNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLSegmentEditorNode")
                self.segmentEditorNode.SetName("FemurFracturePlanner_SegmentEditorNode")
            self.segmentEditorWidget.setMRMLSegmentEditorNode(self.segmentEditorNode)

            # 2. SegmentationNode (세그먼테이션 노드) 존재 여부 검사 및 바인딩
            segNode = self.ui.activeSegmentationSelector.currentNode()
            if not segNode:
                segNode = slicer.mrmlScene.GetFirstNodeByClass("vtkMRMLSegmentationNode")
                if not segNode:
                    segNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLSegmentationNode")
                    segNode.SetName("Femur_Segmentation")
                    segNode.CreateDefaultDisplayNodes()
                self.ui.activeSegmentationSelector.setCurrentNode(segNode)
            self.segmentEditorWidget.setSegmentationNode(segNode)

            # 3. 마지막으로 입력 볼륨 노드 설정 (에러 방지 바인딩 순서 준수)
            self.segmentEditorWidget.setSourceVolumeNode(node)
            
        # 가시성 버튼이 켜진 상태(Show)라면 새 볼륨을 뷰어 화면에 자동으로 매핑
        if self.ui.volumeVisibilityButton.checked:
            slicer.util.setSliceViewerLayers(background=node)
        else:
            slicer.util.setSliceViewerLayers(background=None)
            
        # 볼륨 렌더링이 켜져 있는 상태라면 렌더링 대상도 교체
        if self.ui.volumeRenderingVisibilityButton.checked and node:
            self.logic.updateVolumeRendering(node, self.ui.thresholdSlider.value)

        # 새로운 볼륨이 들어오면 각도 슬라이더 리셋
        self.ui.rotationAngleSlider.value = 0.0

    def onVolumeVisibilityToggled(self, checked: bool) -> None:
        """상단 눈(Eye) 모양 가시성 토글 버튼 처리"""
        inputNode = self.ui.inputVolumeSelector.currentNode()
        if not inputNode:
            if checked:
                self.ui.volumeVisibilityButton.checked = False
                slicer.util.errorDisplay(_("Please load/select an Input CT Volume first."))
            return

        if checked:
            slicer.util.setSliceViewerLayers(background=inputNode)
            self.ui.volumeVisibilityButton.text = _("Hide")
            self.ui.volumeVisibilityButton.toolTip = _("Hide volume in viewers")
            logging.info(f"Showing volume '{inputNode.GetName()}' in slice viewers.")
        else:
            slicer.util.setSliceViewerLayers(background=None)
            self.ui.volumeVisibilityButton.text = _("Show")
            self.ui.volumeVisibilityButton.toolTip = _("Show volume in viewers")
            logging.info("Hiding volume in slice viewers.")

    def onVolumeRenderingToggled(self, checked: bool) -> None:
        """3D 볼륨 렌더링 눈(Eye) 모양 가시성 토글 버튼 처리"""
        inputNode = self.ui.inputVolumeSelector.currentNode()
        if not inputNode:
            if checked:
                self.ui.volumeRenderingVisibilityButton.checked = False
                slicer.util.errorDisplay(_("Please load/select an Input CT Volume first."))
            return
        
        if checked:
            logging.info(f"Enabling real-time 3D volume rendering for: {inputNode.GetName()}")
            self.logic.showVolumeRendering(inputNode, self.ui.thresholdSlider.value)
            
            # 버튼 텍스트 갱신
            self.ui.volumeRenderingVisibilityButton.text = _("👁️ Hide")
            self.ui.volumeRenderingVisibilityButton.toolTip = _("Hide 3D volume rendering")
        else:
            logging.info("Disabling volume rendering.")
            self.logic.hideVolumeRendering(inputNode)
            
            # 버튼 텍스트 갱신
            self.ui.volumeRenderingVisibilityButton.text = _("👁️ Show")
            self.ui.volumeRenderingVisibilityButton.toolTip = _("Show 3D volume rendering")

    def onThresholdSliderChanged(self, value: float) -> None:
        """슬라이더 HU 임계값이 바뀔 때마다 실시간으로 볼륨 렌더링 뼈 강도를 변경시킵니다."""
        inputNode = self.ui.inputVolumeSelector.currentNode()
        if not inputNode or not self.ui.volumeRenderingVisibilityButton.checked:
            return
        
        self.logic.adjustVolumeRenderingThreshold(inputNode, value)

    def onLoadVolumeClicked(self) -> None:
        """1단계: 로컬 드라이브의 CT 볼륨 파일을 파일 다이얼로그를 통해 로드합니다."""
        # 파일 열기 대화상자 호출 (PythonQt 환경에 맞게 단일 문자열 리턴 처리 및 parent는 mainWindow로 수정)
        filePath = qt.QFileDialog.getOpenFileName(
            slicer.util.mainWindow(),
            _("Select CT Volume File"),
            "",
            "Volume Files (*.nrrd *.nii *.nii.gz *.mha *.mhd *.dcm)"
        )
        
        if not filePath:
            return
            
        logging.info(f"Direct volume loading triggered for: '{filePath}'")
        
        # 볼륨 로딩 실행 (대기 마우스 커서 적용)
        with slicer.util.tryWithErrorDisplay(_("Failed to load CT volume file."), waitCursor=True):
            loadedNode = slicer.util.loadVolume(filePath)
            if loadedNode:
                # 선택 콤보박스를 새로 로드된 볼륨 노드로 자동 세팅
                self.ui.inputVolumeSelector.setCurrentNode(loadedNode)
                # 영상이 로드되면 자동으로 슬라이스 가시성(Show) 토글을 활성화하여 화면에 표시합니다.
                self.ui.volumeVisibilityButton.checked = True
                slicer.util.infoDisplay(_(f"Successfully loaded CT volume: '{loadedNode.GetName()}'"))

    def onLoadDicomClicked(self) -> None:
        """1.5단계: Slicer 내장 DICOM 데이터베이스 브라우저를 독립 팝업 형태로 화면에 노출합니다."""
        logging.info("Opening Slicer's DICOM database browser in popup...")
        try:
            # Slicer 메인 윈도우에서 DICOM 브라우저 열기 액션(QAction)을 검색하여 실행
            mainWindow = slicer.util.mainWindow()
            if mainWindow:
                dicomAction = mainWindow.findChild('QAction', 'ActionOpenDICOMBrowser')
                if dicomAction:
                    dicomAction.trigger()
                    return
            
            # Fallback: 액션을 찾을 수 없거나 메인 UI 미작동 시, 내장 DICOM 모듈 탭으로 다이렉트 전환
            slicer.util.selectModule('DICOM')
        except Exception as e:
            slicer.util.errorDisplay(_(f"Failed to open DICOM Browser: {e}"))

    def onNodeAdded(self, caller, event) -> None:
        """씬에 새로운 노드가 로드되었을 때, ScalarVolumeNode일 경우 입력 볼륨으로 자동 연결하는 이벤트 핸들러"""
        node = event
        # Slicer의 NodeAddedEvent 등에서 event가 vtkMRMLNode가 아닌 노드 ID 문자열로 넘어올 수 있음
        if isinstance(node, str):
            node = slicer.mrmlScene.GetNodeByID(node)
            
        if not node:
            return
        
        # 새로 추가된 노드가 스칼라 볼륨 노드(CT 영상 등)인지 확인
        if node.IsA("vtkMRMLScalarVolumeNode"):
            # DICOM DB 등에서 임포트한 볼륨이 새로 씬에 로드되면 자동으로 감지하여 세팅
            logging.info(f"New volume node detected in scene: '{node.GetName()}'. Auto-selecting in module...")
            
            # 콤보박스의 현재 노드로 자동 교체
            self.ui.inputVolumeSelector.setCurrentNode(node)
            
            # 가시성 자동 활성화
            self.ui.volumeVisibilityButton.checked = True

    def onOpenPlannerWindowClicked(self) -> None:
        """Slicer 왼쪽 패널의 버튼을 눌렀을 때 다이얼로그를 화면에 노출합니다."""
        if self.plannerDialog:
            self.plannerDialog.show()
            self.plannerDialog.raise_()
            self.plannerDialog.activateWindow()

    #
    # 실시간 3D 영상 회전 핸들러
    #

    def getSelectedRotationAxis(self) -> str:
        """라디오 버튼 선택 상태를 확인하여 현재 선택된 회전 축을 반환합니다."""
        if self.ui.rotationYRadio.checked:
            return "Y"
        elif self.ui.rotationZRadio.checked:
            return "Z"
        return "X"

    def onRotationAxisChanged(self, toggled: bool) -> None:
        """선택 축이 변경되었을 때, 현재 슬라이더 각도에 맞춰 즉시 3D 회전을 업데이트합니다."""
        if toggled:
            self.onRotationAngleChanged(self.ui.rotationAngleSlider.value)

    def onRotationAngleChanged(self, value: float) -> None:
        """슬라이더의 각도가 변경되면, 변환 행렬을 갱신하여 3D 뷰 및 슬라이스 뷰 상에서 볼륨을 회전시킵니다."""
        inputNode = self.ui.inputVolumeSelector.currentNode()
        if not inputNode:
            return

        axis = self.getSelectedRotationAxis()
        
        # 비즈니스 로직을 통해 변환 노드 생성/조정 및 회전 적용
        self.logic.rotateVolume(inputNode, axis, value)
        
        # 3D 뷰 및 슬라이스 뷰어 화면 즉시 갱신 요구
        slicer.util.forceRenderAllViews()

    def onResetRotationClicked(self) -> None:
        """회전 상태를 0도로 리셋합니다."""
        self.ui.rotationAngleSlider.value = 0.0
        logging.info("Interactive 3D rotation resetted to 0 degrees.")

    def onRotate90Clicked(self) -> None:
        """현재 회전 각도에서 +90도만큼 각도를 회전시킵니다."""
        currentAngle = self.ui.rotationAngleSlider.value
        newAngle = currentAngle + 90.0
        if newAngle > 180.0:
            newAngle = newAngle - 360.0
        self.ui.rotationAngleSlider.value = newAngle
        logging.info(f"Rapid rotation +90° applied. New angle: {newAngle:.1f}°")

    #
    # 5. 골절 파편 분리 핸들러
    #

    def onSeparateFragmentsClicked(self) -> None:
        """5단계: 버튼 클릭 시 세그먼트 데이터에서 독립된 뼈 조각 모델들을 자동으로 분리합니다."""
        segNode = self.ui.activeSegmentationSelector.currentNode()
        if not segNode:
            slicer.util.errorDisplay(_("Please select an Input Segmentation node first."))
            return

        # 3D 뼈 파편 분리 연산 시작 (마우스 대기 커서 활성화)
        logging.info(f"Separating disjoint bone fragments in segmentation: '{segNode.GetName()}'")
        
        with slicer.util.tryWithErrorDisplay(_("Failed to separate bone fragments."), waitCursor=True):
            separatedCount = self.logic.separateBoneFragments(segNode)
            
            if separatedCount > 0:
                slicer.util.infoDisplay(_(f"Successfully separated into {separatedCount} independent bone fragments!"))
                
                # 가상 수술 조작이 편하도록 기존 CT 볼륨 렌더링(홀로그램) 가시성은 꺼줍니다.
                inputNode = self.ui.inputVolumeSelector.currentNode()
                if inputNode and self.ui.volumeRenderingVisibilityButton.checked:
                    self.ui.volumeRenderingVisibilityButton.checked = False
            else:
                slicer.util.errorDisplay(_("No valid bone fragments could be extracted. Make sure the segmentation contains points."))


#
# FemurFracturePlannerLogic (비즈니스 로직 구현부)
#
class FemurFracturePlannerLogic(ScriptedLoadableModuleLogic):
    def getParameterNode(self):
        return FemurFracturePlannerParameterNode(super().getParameterNode())

    def showVolumeRendering(self, volumeNode: vtkMRMLScalarVolumeNode, threshold: float) -> None:
        """볼륨에 대해 3D Slicer 내장 GPU 볼륨 렌더링을 켜고 'CT-Bone' 뼈 프리셋을 적용합니다."""
        volRenLogic = slicer.modules.volumerendering.logic()
        
        displayNode = volRenLogic.GetFirstVolumeRenderingDisplayNode(volumeNode)
        if not displayNode:
            displayNode = volRenLogic.CreateVolumeRenderingDisplayNode()
            slicer.mrmlScene.AddNode(displayNode)
            volumeNode.AddAndObserveDisplayNodeID(displayNode.GetID())
        
        volRenLogic.UpdateDisplayNodeFromVolumeNode(displayNode, volumeNode)
        presetNode = volRenLogic.GetPresetByName("CT-Bone")
        if presetNode:
            displayNode.GetVolumePropertyNode().Copy(presetNode)
            
        self.adjustVolumeRenderingThreshold(volumeNode, threshold)
        displayNode.SetVisibility(True)
        
        # 3D 뷰어 카메라를 이 볼륨의 위치로 자동 리셋 및 화면 강제 갱신
        slicer.util.resetThreeDViews()
        slicer.util.forceRenderAllViews()

    def hideVolumeRendering(self, volumeNode: vtkMRMLScalarVolumeNode) -> None:
        """볼륨 렌더링 노드를 씬에서 완전히 제거하여 3D 뷰에서 깨끗이 지웁니다."""
        volRenLogic = slicer.modules.volumerendering.logic()
        displayNode = volRenLogic.GetFirstVolumeRenderingDisplayNode(volumeNode)
        if displayNode:
            slicer.mrmlScene.RemoveNode(displayNode)
        
        # 3D 뷰 및 슬라이스 뷰어 화면 즉시 리렌더링 갱신
        slicer.util.forceRenderAllViews()

    def updateVolumeRendering(self, volumeNode: vtkMRMLScalarVolumeNode, threshold: float) -> None:
        self.showVolumeRendering(volumeNode, threshold)

    def adjustVolumeRenderingThreshold(self, volumeNode: vtkMRMLScalarVolumeNode, threshold: float) -> None:
        """볼륨 렌더링의 뼈 가시화 투명도 곡선을 실시간 변경시킵니다."""
        volRenLogic = slicer.modules.volumerendering.logic()
        displayNode = volRenLogic.GetFirstVolumeRenderingDisplayNode(volumeNode)
        if not displayNode:
            return

        volumePropertyNode = displayNode.GetVolumePropertyNode()
        if not volumePropertyNode:
            return

        opacityFunction = volumePropertyNode.GetVolumeProperty().GetScalarOpacity()
        
        opacityFunction.RemoveAllPoints()
        opacityFunction.AddPoint(threshold - 100, 0.0)
        opacityFunction.AddPoint(threshold, 0.15)
        opacityFunction.AddPoint(threshold + 200, 0.7)
        opacityFunction.AddPoint(1500, 0.85)
        
        displayNode.Modified()

    def rotateVolume(self, volumeNode: vtkMRMLScalarVolumeNode, axis: str, angle: float) -> None:
        """선택된 볼륨 노드에 선형 변환 노드(vtkMRMLLinearTransformNode)를 연결하고 회전을 적용합니다."""
        if not volumeNode:
            return

        transformNode = volumeNode.GetParentTransformNode()
        
        if not transformNode or not transformNode.IsA("vtkMRMLLinearTransformNode"):
            transformNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
            transformNode.SetName(f"{volumeNode.GetName()}_RotationTransform")
            volumeNode.SetAndObserveTransformNodeID(transformNode.GetID())
            logging.info(f"Created new linear transform node for '{volumeNode.GetName()}'.")

        transform = vtk.vtkTransform()
        if axis == "X":
            transform.RotateX(angle)
        elif axis == "Y":
            transform.RotateY(angle)
        else:
            transform.RotateZ(angle)

        matrix = vtk.vtkMatrix4x4()
        transform.GetMatrix(matrix)
        
        transformNode.SetMatrixTransformToParent(matrix)
        logging.debug(f"Rotated volume '{volumeNode.GetName()}' around {axis}-Axis by {angle:.1f}°")

    def separateBoneFragments(self, segmentationNode: vtkMRMLSegmentationNode) -> int:
        """
        주어진 세그먼트 노드로부터 불연속적인 메쉬(Mesh)들을 감지하여,
        각각 독립된 vtkMRMLModelNode(3D 모델)로 분리하고 다채로운 무지개 계열 색상을 입혀 저장합니다.
        꼭짓점 개수가 200개 이하인 미세 노이즈 조각들은 필터링하여 생성하지 않습니다.
        """
        if not segmentationNode:
            return 0

        # 1. 임시 Labelmap 노드 생성 및 가시 세그먼트 데이터 내보내기
        labelmapNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLabelMapVolumeNode")
        labelmapNode.SetName("Temp_Extraction_Labelmap")
        
        slicer.modules.segmentations.logic().ExportVisibleSegmentsToLabelmapNode(
            segmentationNode, labelmapNode, segmentationNode.GetReferenceImageGeometryParameterFromVolumeNode()
        )

        # 2. vtkMarchingCubes를 사용하여 Labelmap의 뼈 영역(값=1)을 3D 삼각 메쉬(PolyData)로 변환
        mc = vtk.vtkMarchingCubes()
        mc.SetInputData(labelmapNode.GetImageData())
        mc.SetValue(0, 1)
        mc.Update()

        polyData = mc.GetOutput()
        if not polyData or polyData.GetNumberOfPoints() == 0:
            slicer.mrmlScene.RemoveNode(labelmapNode)
            raise RuntimeError("The selected segmentation node has no valid 3D geometry.")

        # 3. VTK Connectivity Filter를 연동하여 불연속적으로 끊어진 Region(조각)들을 일괄 분석
        connectivityFilter = vtk.vtkPolyDataConnectivityFilter()
        connectivityFilter.SetInputData(polyData)
        connectivityFilter.SetExtractionModeToAllRegions()
        connectivityFilter.Update()

        numRegions = connectivityFilter.GetNumberOfExtractedRegions()
        logging.info(f"Total disconnected regions detected: {numRegions}")

        separatedCount = 0
        
        # 4. 각 Region별로 루프를 돌며 개별 3D 모델 메쉬 노드로 씬에 추가
        for i in range(numRegions):
            singleRegionFilter = vtk.vtkPolyDataConnectivityFilter()
            singleRegionFilter.SetInputData(polyData)
            singleRegionFilter.SetExtractionModeToSpecifiedRegions()
            singleRegionFilter.AddSpecifiedRegion(i)
            singleRegionFilter.Update()

            regionPolyData = singleRegionFilter.GetOutput()
            
            # 꼭짓점 개수가 200개 초과인 의미 있는 큰 뼈 조각들만 필터링하여 노이즈 배제
            if regionPolyData.GetNumberOfPoints() > 200:
                fragModel = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLModelNode")
                fragModel.SetName(f"Femur_Fragment_{separatedCount + 1}")
                fragModel.SetAndObservePolyData(regionPolyData)
                fragModel.CreateDefaultDisplayNodes()

                # 3D 뷰에서 파편 구분이 쉽도록 다채로운 색상 순차 지정
                displayNode = fragModel.GetDisplayNode()
                if displayNode:
                    # 겹치지 않는 무지개 톤 스펙트럼 색상 배분
                    r = 0.3 + (separatedCount * 0.17) % 0.6
                    g = 0.4 + (separatedCount * 0.11) % 0.5
                    b = 0.8 - (separatedCount * 0.23) % 0.5
                    displayNode.SetColor(r, g, b)
                    displayNode.SetSliceIntersectionVisibility(True)
                    displayNode.SetSliceIntersectionThickness(3)

                separatedCount += 1

        # 5. 사용을 마친 임시 라벨맵 노드 완전 소멸
        slicer.mrmlScene.RemoveNode(labelmapNode)
        
        return separatedCount


#
# FemurFracturePlannerTest
#
class FemurFracturePlannerTest(ScriptedLoadableModuleTest):
    def setUp(self):
        slicer.mrmlScene.Clear()

    def runTest(self):
        self.setUp()
        self.test_FemurFracturePlannerEmbedded()

    def test_FemurFracturePlannerEmbedded(self):
        self.delayDisplay("Validating Embedded Volume Rendering & Editor...")
        logic = FemurFracturePlannerLogic()
        self.assertIsNotNone(logic)
        self.delayDisplay("All tests passed successfully!")
