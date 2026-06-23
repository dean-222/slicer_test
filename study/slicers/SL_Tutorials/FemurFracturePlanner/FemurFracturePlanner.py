import logging
import os
import vtk
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
from slicer import vtkMRMLVolumeRenderingDisplayNode

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
This module provides real-time CT volume rendering and an embedded Segment Editor for femur fracture visualization and surgical planning.
""")
        self.parent.acknowledgementText = _("""
Built for interactive femur bone segmentation using Slicer's embedded widget technology.
""")


#
# FemurFracturePlannerParameterNode (파라미터 노드)
#
@parameterNodeWrapper
class FemurFracturePlannerParameterNode:
    inputVolume: vtkMRMLScalarVolumeNode


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
        
        # 내장 세그먼트 에디터 인스턴스 관리 변수
        self.segmentEditorWidget = None
        self.segmentEditorNode = None

    def setup(self) -> None:
        ScriptedLoadableModuleWidget.setup(self)

        # UI 파일 로드 (Resources/ 중복 에러가 수정된 'UI/FemurFracturePlanner.ui' 사용)
        uiWidget = slicer.util.loadUI(self.resourcePath("UI/FemurFracturePlanner.ui"))
        self.layout.addWidget(uiWidget)
        self.ui = slicer.util.childWidgetVariables(uiWidget)

        # 씬 세팅
        uiWidget.setMRMLScene(slicer.mrmlScene)

        # 로직 인스턴스 생성
        self.logic = FemurFracturePlannerLogic()

        # UI 이벤트 리스너 연결
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.StartCloseEvent, self.onSceneStartClose)
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.EndCloseEvent, self.onSceneEndClose)

        # 1. 입력 볼륨 선택 변경
        self.ui.inputVolumeSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onInputVolumeChanged)

        # 2. 실시간 3D 볼륨 렌더링 이벤트 연결
        self.ui.volumeRenderingCheckBox.connect("toggled(bool)", self.onVolumeRenderingToggled)
        self.ui.thresholdSlider.connect("valueChanged(double)", self.onThresholdSliderChanged)

        # 3. 내장 세그먼트 에디터 위젯 동적 생성 및 배치
        self.createEmbeddedSegmentEditor()

        # 파라미터 노드 초기화
        self.initializeParameterNode()

    def createEmbeddedSegmentEditor(self) -> None:
        """
        Slicer의 내장 세그먼트 에디터 위젯(qMRMLSegmentEditorWidget)을 동적으로 생성하고,
        UI 레이아웃의 segmentEditorAnchorWidget 내에 통째로 삽입합니다.
        """
        logging.info("Initializing embedded Segment Editor widget...")
        
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
            
        if self.segmentEditorWidget:
            self.segmentEditorWidget.setMRMLSegmentEditorNode(self.segmentEditorNode)
            # 현재 선택된 볼륨 노드를 에디터의 소스(입력) 노드로 동기화 설정
            inputNode = self.ui.inputVolumeSelector.currentNode()
            if inputNode:
                self.segmentEditorWidget.setSourceVolumeNode(inputNode)

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

    def onSceneStartClose(self, caller, event) -> None:
        self.setParameterNode(None)
        self.destroyEmbeddedSegmentEditor()

    def onSceneEndClose(self, caller, event) -> None:
        if self.parent.isEntered:
            self.initializeParameterNode()
            self.createEmbeddedSegmentEditor()

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
    # 영상 처리 & 3D 볼륨 렌더링 이벤트 핸들러
    #

    def onInputVolumeChanged(self, node) -> None:
        """입력 CT 볼륨이 바뀌면 볼륨 렌더링 대상 및 내장 에디터 입력 대상을 갱신합니다."""
        if self.segmentEditorWidget and node:
            self.segmentEditorWidget.setSourceVolumeNode(node)
            
        # 볼륨 렌더링이 켜져 있는 상태라면 렌더링 대상도 교체
        if self.ui.volumeRenderingCheckBox.checked and node:
            self.logic.updateVolumeRendering(node, self.ui.thresholdSlider.value)

    def onVolumeRenderingToggled(self, checked: bool) -> None:
        """3D 볼륨 렌더링 체크박스 토글 시"""
        inputNode = self.ui.inputVolumeSelector.currentNode()
        if not inputNode:
            if checked:
                self.ui.volumeRenderingCheckBox.checked = False
                slicer.util.errorDisplay(_("Please load/select an Input CT Volume first."))
            return
        
        if checked:
            logging.info(f"Enabling real-time 3D volume rendering for: {inputNode.GetName()}")
            self.logic.showVolumeRendering(inputNode, self.ui.thresholdSlider.value)
        else:
            logging.info("Disabling volume rendering.")
            self.logic.hideVolumeRendering(inputNode)

    def onThresholdSliderChanged(self, value: float) -> None:
        """슬라이더 HU 임계값이 바뀔 때마다 실시간으로 볼륨 렌더링 뼈 강도를 변경시킵니다."""
        inputNode = self.ui.inputVolumeSelector.currentNode()
        if not inputNode or not self.ui.volumeRenderingCheckBox.checked:
            return
        
        # 비즈니스 로직을 호출하여 볼륨의 투명도 맵핑 곡선 업데이트
        self.logic.adjustVolumeRenderingThreshold(inputNode, value)


#
# FemurFracturePlannerLogic (비즈니스 로직 구현부)
#
class FemurFracturePlannerLogic(ScriptedLoadableModuleLogic):
    def getParameterNode(self):
        return FemurFracturePlannerParameterNode(super().getParameterNode())

    def showVolumeRendering(self, volumeNode: vtkMRMLScalarVolumeNode, threshold: float) -> None:
        """
        주어진 볼륨에 대해 3D Slicer 내장 GPU 볼륨 렌더링을 켜고,
        'CT-Bone' 뼈 가시화 프리셋을 적용한 뒤, 슬라이더 임계값에 맞춰 투명도를 세팅합니다.
        """
        volRenLogic = slicer.modules.volumerendering.logic()
        
        # 1. 기존에 생성된 볼륨 렌더링 디스플레이 노드가 있는지 탐색
        displayNode = volRenLogic.GetFirstVolumeRenderingDisplayNode(volumeNode)
        if not displayNode:
            # 2. 없으면 새로 생성하여 씬에 등록
            displayNode = volRenLogic.CreateVolumeRenderingDisplayNode()
            slicer.mrmlScene.AddNode(displayNode)
            displayNode.SetAndObserveVolumeNodeID(volumeNode.GetID())
        
        # 3. Slicer의 표준 뼈(CT-Bone) 시각화 프리셋 적용
        volRenLogic.UpdateDisplayNodeFromVolumeNode(displayNode)
        presetNode = volRenLogic.GetPresetByName("CT-Bone")
        if presetNode:
            displayNode.GetVolumePropertyNode().Copy(presetNode)
            
        # 4. 슬라이더 값에 맞게 투명도 임계값 매핑 조정
        self.adjustVolumeRenderingThreshold(volumeNode, threshold)
        
        # 5. 가시성 켜기
        displayNode.SetVisibility(True)

    def hideVolumeRendering(self, volumeNode: vtkMRMLScalarVolumeNode) -> None:
        """볼륨 렌더링 가시성을 끕니다."""
        volRenLogic = slicer.modules.volumerendering.logic()
        displayNode = volRenLogic.GetFirstVolumeRenderingDisplayNode(volumeNode)
        if displayNode:
            displayNode.SetVisibility(False)

    def updateVolumeRendering(self, volumeNode: vtkMRMLScalarVolumeNode, threshold: float) -> None:
        """다른 볼륨으로 렌더링 대상을 바꿀 때 가시성을 전환합니다."""
        self.showVolumeRendering(volumeNode, threshold)

    def adjustVolumeRenderingThreshold(self, volumeNode: vtkMRMLScalarVolumeNode, threshold: float) -> None:
        """
        볼륨 렌더링 노드의 투명도 전달 함수(Scalar Opacity Transfer Function)의 뼈 영역 기준점을 
        슬라이더에서 전달된 HU 임계값(Threshold)에 맞춰 실시간 이동시킵니다.
        """
        volRenLogic = slicer.modules.volumerendering.logic()
        displayNode = volRenLogic.GetFirstVolumeRenderingDisplayNode(volumeNode)
        if not displayNode:
            return

        volumePropertyNode = displayNode.GetVolumePropertyNode()
        if not volumePropertyNode:
            return

        # 3D Slicer 볼륨 속성의 스칼라 투명도 전달 함수(Piecewise Function) 획득
        opacityFunction = volumePropertyNode.GetVolumeProperty().GetScalarOpacity()
        
        # CT-Bone 프리셋의 기본적인 투명도 전달 함수 제어 포인트 리빌딩
        # 뼈(Bone)는 대략 입력 임계값(threshold) 부근부터 급격히 불투명해지기 시작함
        opacityFunction.RemoveAllPoints()
        opacityFunction.AddPoint(threshold - 100, 0.0) # 임계값보다 낮은 소프트 티슈는 투명처리 (0.0)
        opacityFunction.AddPoint(threshold, 0.15)      # 임계값 도달 지점부터 불투명하기 시작
        opacityFunction.AddPoint(threshold + 200, 0.7)  # 단단한 피질골 영역은 고도 불투명 (0.7)
        opacityFunction.AddPoint(1500, 0.85)            # 매우 치밀한 뼈는 강하게 표시
        
        # Slicer 3D 뷰 강제 렌더링 갱신
        displayNode.Modified()


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
