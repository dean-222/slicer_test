import logging
import os
from typing import Annotated

import vtk

import slicer
# 다국어 번역을 위한 번역 헬퍼 함수 가져오기
from slicer.i18n import tr as _
from slicer.i18n import translate
# Slicer의 파이썬 기반 스크립트 모듈 베이스 클래스 가져오기
from slicer.ScriptedLoadableModule import *
# MRML 이벤트(씬 변경 등) 관찰용 믹스인 클래스 가져오기
from slicer.util import VTKObservationMixin
# GUI와 파이썬 변수 바인딩을 위한 parameterNode 래퍼 클래스 가져오기
from slicer.parameterNodeWrapper import (
    parameterNodeWrapper,
    WithinRange,
)

# Slicer 볼륨(Scalar Volume) 노드 클래스 가져오기
from slicer import vtkMRMLScalarVolumeNode


#
# t_ApplyThreshold (모듈 메인 메타데이터 클래스)
#

class t_ApplyThreshold(ScriptedLoadableModule):
    """
    3D Slicer 모듈의 이름, 기여자, 도움말 등의 정보를 관리하는 메타데이터 클래스입니다.
    """

    def __init__(self, parent):
        ScriptedLoadableModule.__init__(self, parent)
        # 모듈 타이틀 설정 (모듈 선택창에 표시될 이름)
        self.parent.title = _("t_ApplyThreshold")  
        
        # 모듈이 위치할 카테고리 정의 (SL_Tutorials 폴더에 노출됨)
        self.parent.categories = [translate("qSlicerAbstractCoreModule", "SL_Tutorials")]
        # 모듈 실행을 위해 필요한 타 모듈 리스트(의존성)
        self.parent.dependencies = []  
        # 모듈 개발자 기여자 정보
        self.parent.contributors = ["John Doe (AnyWare Corp.)"]  
        
        # 모듈 도움말 정보 (HTML 형식으로 작성 가능, 번역 지원)
        self.parent.helpText = _("""
이 모듈은 익스텐션 내에 포함된 스크립트 기반 로드 모듈의 예시입니다.<br>
자세한 정보는 <a href="https://github.com/organization/projectname#t_ApplyThreshold">모듈 문서</a>를 참조하세요.
""")
        # 모듈 연구 과제 지원 및 감사 관련 정보
        self.parent.acknowledgementText = _("""
이 파일은 최초에 Jean-Christophe Fillion-Robin(Kitware Inc.), Andras Lasso(PerkLab), Steve Pieper(Isomics, Inc.) 등에 의해 개발되었으며, 일부 NIH grant 3P41RR013218-12S1의 지원을 받았습니다.
""")

        # Slicer의 애플리케이션 시작 프로세스가 완전히 완료되면 샘플 데이터를 등록하도록 이벤트 연결
        slicer.app.connect("startupCompleted()", registerSampleData)


#
# 샘플 데이터 세트를 Slicer Sample Data 모듈에 등록
#

def registerSampleData():
    """
    사용자가 모듈을 바로 테스트해볼 수 있도록 기본 테스트용 볼륨 데이터를 등록합니다.
    """

    import SampleData

    iconsPath = os.path.join(os.path.dirname(__file__), "Resources/Icons")

    # t_ApplyThreshold1 샘플 데이터 등록
    SampleData.SampleDataLogic.registerCustomSampleDataSource(
        # Sample Data 모듈 내에서 표시될 카테고리와 샘플 이름
        category="t_ApplyThreshold",
        sampleName="t_ApplyThreshold1",
        # 썸네일 경로 (Resources/Icons 폴더 아래 이미지 파일 매핑)
        thumbnailFileName=os.path.join(iconsPath, "t_ApplyThreshold1.png"),
        # 샘플 데이터 다운로드 링크
        uris="https://github.com/Slicer/SlicerTestingData/releases/download/SHA256/998cb522173839c78657f4bc0ea907cea09fd04e44601f17c82ea27927937b95",
        fileNames="t_ApplyThreshold1.nrrd",
        # 파일 무결성 검증을 위한 SHA-256 해시값
        checksums="SHA256:998cb522173839c78657f4bc0ea907cea09fd04e44601f17c82ea27927937b95",
        # 데이터 로드 시 Slicer 씬에 생성될 노드 이름
        nodeNames="t_ApplyThreshold1",
    )

    # t_ApplyThreshold2 샘플 데이터 등록
    SampleData.SampleDataLogic.registerCustomSampleDataSource(
        category="t_ApplyThreshold",
        sampleName="t_ApplyThreshold2",
        thumbnailFileName=os.path.join(iconsPath, "t_ApplyThreshold2.png"),
        uris="https://github.com/Slicer/SlicerTestingData/releases/download/SHA256/1a64f3f422eb3d1c9b093d1a18da354b13bcf307907c66317e2463ee530b7a97",
        fileNames="t_ApplyThreshold2.nrrd",
        checksums="SHA256:1a64f3f422eb3d1c9b093d1a18da354b13bcf307907c66317e2463ee530b7a97",
        nodeNames="t_ApplyThreshold2",
    )



#
# t_ApplyThresholdParameterNode
#


@parameterNodeWrapper
class t_ApplyThresholdParameterNode:
    """
    모듈에 필요한 파라미터들을 정의하는 데이터 모델 클래스입니다.
    
    inputVolume - 임계값을 적용할 입력 볼륨 노드.
    imageThreshold - 임계값 수치 (범위: -100 ~ 500, 기본값: 100).
    invertThreshold - 참일 경우 임계값을 반전시켜 적용.
    thresholdedVolume - 임계값 연산 결과가 담길 출력 볼륨 노드.
    invertedVolume - 임계값 반전 연산 결과가 담길 출력 볼륨 노드.
    """

    inputVolume: vtkMRMLScalarVolumeNode
    imageThreshold: Annotated[float, WithinRange(-100, 500)] = 100
    invertThreshold: bool = False
    thresholdedVolume: vtkMRMLScalarVolumeNode
    invertedVolume: vtkMRMLScalarVolumeNode


#
# t_ApplyThresholdWidget (GUI 및 상호작용 제어 클래스)
#

class t_ApplyThresholdWidget(ScriptedLoadableModuleWidget, VTKObservationMixin):
    """
    Slicer 모듈의 UI 표시 및 사용자 이벤트 처리를 담당하는 위젯 클래스입니다.
    """

    def __init__(self, parent=None) -> None:
        """사용자가 모듈을 처음 열어 위젯이 초기화될 때 호출됩니다."""
        ScriptedLoadableModuleWidget.__init__(self, parent)
        VTKObservationMixin.__init__(self)  # MRML 파라미터 노드 관찰(Observation)을 위해 필요
        self.logic = None
        self._parameterNode = None
        self._parameterNodeGuiTag = None

    def setup(self) -> None:
        """위젯의 UI 요소를 생성하고 이벤트 연결 등의 초기 설정을 수행합니다."""
        ScriptedLoadableModuleWidget.setup(self)

        # Qt Designer로 생성된 .ui 파일을 로드하여 레이아웃에 추가합니다.
        uiWidget = slicer.util.loadUI(self.resourcePath("UI/t_ApplyThreshold.ui"))
        self.layout.addWidget(uiWidget)
        self.ui = slicer.util.childWidgetVariables(uiWidget)

        # MRML 씬을 UI 위젯에 세팅합니다.
        uiWidget.setMRMLScene(slicer.mrmlScene)

        # 실제 연산 알고리즘을 담당할 로직 객체 생성
        self.logic = t_ApplyThresholdLogic()

        # 연결(Connections) 설정
        
        # Slicer 씬이 닫히기 시작하고 완료될 때의 이벤트를 처리하기 위한 옵저버 추가
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.StartCloseEvent, self.onSceneStartClose)
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.EndCloseEvent, self.onSceneEndClose)

        # 실행 버튼 클릭 시 onApplyButton 함수와 연결
        self.ui.applyButton.connect("clicked(bool)", self.onApplyButton)

        # 파라미터 노드가 존재하는지 확인하고 초기화 (모듈 리로드 시에도 상태 유지 목적)
        self.initializeParameterNode()

    def cleanup(self) -> None:
        """애플리케이션이 종료되거나 모듈 위젯이 소멸할 때 옵저버들을 정리합니다."""
        self.removeObservers()

    def enter(self) -> None:
        """사용자가 모듈 화면으로 진입할 때마다 호출됩니다."""
        # 파라미터 노드를 확인 및 관찰 활성화
        self.initializeParameterNode()

    def exit(self) -> None:
        """사용자가 다른 모듈 화면으로 이동(종료)할 때 호출됩니다."""
        # GUI 위젯들과 파라미터 노드 간의 동기화 연결을 일시 해제하여 백그라운드 리소스 낭비 방지
        if self._parameterNode:
            self._parameterNode.disconnectGui(self._parameterNodeGuiTag)
            self._parameterNodeGuiTag = None
            self.removeObserver(self._parameterNode, vtk.vtkCommand.ModifiedEvent, self._checkCanApply)

    def onSceneStartClose(self, caller, event) -> None:
        """Slicer 씬이 닫히기 직전에 호출됩니다. 기존 파라미터 노드를 끊습니다."""
        self.setParameterNode(None)

    def onSceneEndClose(self, caller, event) -> None:
        """Slicer 씬이 완전히 닫힌 직후 호출됩니다. 모듈이 켜져있다면 파라미터 노드를 재생성합니다."""
        if self.parent.isEntered:
            self.initializeParameterNode()

    def initializeParameterNode(self) -> None:
        """씬 내에 파라미터 노드가 존재하는지 확인하고, 없다면 생성 및 초기 노드 세팅을 진행합니다."""
        self.setParameterNode(self.logic.getParameterNode())

        # 사용자의 편의를 돕기 위해 씬 내에 첫 번째로 존재하는 Scalar Volume 노드가 있다면 입력 볼륨으로 자동 선택
        if not self._parameterNode.inputVolume:
            firstVolumeNode = slicer.mrmlScene.GetFirstNodeByClass("vtkMRMLScalarVolumeNode")
            if firstVolumeNode:
                self._parameterNode.inputVolume = firstVolumeNode

    def setParameterNode(self, inputParameterNode: t_ApplyThresholdParameterNode | None) -> None:
        """
        파라미터 노드를 설정하고 관찰(Observe)합니다.
        파라미터의 값이 바뀌면 GUI의 상태도 실시간으로 반영되어야 합니다.
        """
        if self._parameterNode:
            # 이전 파라미터 노드와 GUI 간의 연결 및 이벤트 제거
            self._parameterNode.disconnectGui(self._parameterNodeGuiTag)
            self.removeObserver(self._parameterNode, vtk.vtkCommand.ModifiedEvent, self._checkCanApply)
        
        self._parameterNode = inputParameterNode
        if self._parameterNode:
            # 파라미터 노드와 UI 요소를 바인딩합니다. (.ui 파일 내 SlicerParameterName 속성 기준)
            self._parameterNodeGuiTag = self._parameterNode.connectGui(self.ui)
            # 파라미터 노드가 변경되었을 때(ModifiedEvent) Apply 버튼 활성화 여부 재검사
            self.addObserver(self._parameterNode, vtk.vtkCommand.ModifiedEvent, self._checkCanApply)
            self._checkCanApply()

    def _checkCanApply(self, caller=None, event=None) -> None:
        """필수 입력 데이터와 출력 노드가 올바르게 선택되었는지 검사하여 실행 버튼을 활성화/비활성화합니다."""
        if self._parameterNode and self._parameterNode.inputVolume and self._parameterNode.thresholdedVolume:
            self.ui.applyButton.toolTip = _("Compute output volume")
            self.ui.applyButton.enabled = True
        else:
            self.ui.applyButton.toolTip = _("Select input and output volume nodes")
            self.ui.applyButton.enabled = False

    def onApplyButton(self) -> None:
        """사용자가 'Apply' 버튼을 클릭하면 연산을 시작합니다."""
        # 예외 상황 발생 시 에러 대화상자를 사용자에게 띄워줍니다.
        with slicer.util.tryWithErrorDisplay(_("Failed to compute results."), waitCursor=True):
            # 기본 임계값 연산 실행
            self.logic.process(self.ui.inputSelector.currentNode(), self.ui.outputSelector.currentNode(),
                               self.ui.imageThresholdSliderWidget.value, self.ui.invertOutputCheckBox.checked)

            # 반전 출력용 노드가 선택되어 있는 경우, 반전된 결과를 추가 연산하여 저장
            if self.ui.invertedOutputSelector.currentNode():
                self.logic.process(self.ui.inputSelector.currentNode(), self.ui.invertedOutputSelector.currentNode(),
                                   self.ui.imageThresholdSliderWidget.value, not self.ui.invertOutputCheckBox.checked, showResult=False)



#
# t_ApplyThresholdLogic (비즈니스 로직 클래스)
#

class t_ApplyThresholdLogic(ScriptedLoadableModuleLogic):
    """
    모듈의 실제 수치 연산 및 백엔드 처리를 구현하는 로직 클래스입니다.
    이 클래스는 UI 없이 백그라운드 배치 작업이나 다른 스크립트에서도 직접 임포트하여 활용할 수 있습니다.
    """

    def __init__(self) -> None:
        """로직 클래스가 생성될 때 멤버 변수를 초기화합니다."""
        ScriptedLoadableModuleLogic.__init__(self)

    def getParameterNode(self):
        """공통 파라미터 노드 인스턴스를 가져옵니다."""
        return t_ApplyThresholdParameterNode(super().getParameterNode())

    def process(self,
                inputVolume: vtkMRMLScalarVolumeNode,
                outputVolume: vtkMRMLScalarVolumeNode,
                imageThreshold: float,
                invert: bool = False,
                showResult: bool = True) -> None:
        """
        임계값 연산 알고리즘을 수행합니다. Slicer 내장 CLI 모듈을 이용해 계산합니다.
        
        :param inputVolume: 임계값을 적용할 입력 Scalar Volume 노드
        :param outputVolume: 연산 결과를 담아 저장할 출력 Scalar Volume 노드
        :param imageThreshold: 필터링할 기준 임계값
        :param invert: 참(True)이면 임계값보다 높은 값을 0으로 설정, 거짓(False)이면 임계값보다 낮은 값을 0으로 설정
        :param showResult: 참(True)이면 출력 볼륨을 슬라이스 뷰어(화면)에 표시함
        """

        if not inputVolume or not outputVolume:
            raise ValueError("입력 혹은 출력 볼륨 노드가 유효하지 않습니다.")

        import time

        startTime = time.time()
        logging.info("임계값 연산 시작")

        # 3D Slicer의 내장 CLI 모듈인 "Threshold Scalar Volume"을 실행하기 위한 파라미터 구성
        cliParams = {
            "InputVolume": inputVolume.GetID(),
            "OutputVolume": outputVolume.GetID(),
            "ThresholdValue": imageThreshold,
            "ThresholdType": "Above" if invert else "Below",
        }
        # CLI 모듈 동기 실행 (wait_for_completion=True)
        cliNode = slicer.cli.run(slicer.modules.thresholdscalarvolume, None, cliParams, wait_for_completion=True, update_display=showResult)
        
        # 사용이 완료된 CLI 노드는 씬(Scene)이 복잡해지는 것을 방지하기 위해 씬에서 즉시 제거
        slicer.mrmlScene.RemoveNode(cliNode)

        stopTime = time.time()
        logging.info(f"연산 완료. 소요 시간: {stopTime-startTime:.2f}초")


#
# t_ApplyThresholdTest (단위 테스트 클래스)
#

class t_ApplyThresholdTest(ScriptedLoadableModuleTest):
    """
    모듈의 기능을 검증하기 위한 테스트 케이스 정의 클래스입니다.
    """

    def setUp(self):
        """테스트 수행 전 상태를 초기화합니다. 통상적으로 기존 MRML 씬을 클리어합니다."""
        slicer.mrmlScene.Clear()

    def runTest(self):
        """전체 테스트를 실행합니다."""
        self.setUp()
        self.test_t_ApplyThreshold1()

    def test_t_ApplyThreshold1(self):
        """
        기본 알고리즘이 올바르게 작동하는지 확인하는 단위 테스트입니다.
        원격 샘플 데이터를 다운로드하고, 임계값을 적용하여 출력 데이터의 범위가 기대 조건에 일치하는지 비교합니다.
        """

        self.delayDisplay("테스트 시작")

        # 입력 데이터 가져오기/생성
        import SampleData

        registerSampleData()
        # 등록된 custom datasource를 통해 샘플 파일 다운로드 및 노드 생성
        inputVolume = SampleData.downloadSample("t_ApplyThreshold1")
        self.delayDisplay("테스트용 샘플 데이터 로드 완료")

        # 입력 볼륨의 스칼라 범위 가져오기
        inputScalarRange = inputVolume.GetImageData().GetScalarRange()
        # 원래 데이터의 최솟값은 0, 최댓값은 695인지 확인
        self.assertEqual(inputScalarRange[0], 0)
        self.assertEqual(inputScalarRange[1], 695)

        # 출력을 받을 새 Scalar Volume 노드 생성
        outputVolume = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLScalarVolumeNode")
        threshold = 100

        # 모듈 로직 인스턴스 생성
        logic = t_ApplyThresholdLogic()

        # 테스트 1: 임계값 반전 적용 (임계값 100보다 큰 값은 0 처리 -> 최댓값이 100 이하가 되어야 함)
        logic.process(inputVolume, outputVolume, threshold, True)
        outputScalarRange = outputVolume.GetImageData().GetScalarRange()
        self.assertEqual(outputScalarRange[0], inputScalarRange[0])
        self.assertEqual(outputScalarRange[1], threshold)

        # 테스트 2: 일반 임계값 적용 (임계값 100보다 작은 값은 0 처리 -> 원본 최댓값인 695가 유지되어야 함)
        logic.process(inputVolume, outputVolume, threshold, False)
        outputScalarRange = outputVolume.GetImageData().GetScalarRange()
        self.assertEqual(outputScalarRange[0], inputScalarRange[0])
        self.assertEqual(outputScalarRange[1], inputScalarRange[1])

        self.delayDisplay("테스트 통과!")

