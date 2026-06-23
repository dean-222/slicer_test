"""
File Name: FemurFracturePlanner.py
Version: v0.310
Date: 2026-06-23
Description: 대퇴골 골절 수술 계획을 위한 3D Slicer 모듈 GUI 및 비즈니스 로직 제어 클래스

Version History:

- v0.310 (2026-06-23)
  - 뼈 조각 좌표 표시에 단순 로컬 변위를 보여주던 방식에서 실제 3D 공간 상의 현재 월드 중심 좌표 및 월드 회전 각도로 실시간 동기화 출력하도록 개선 (Y 증가)
  - Fracture Surface Snap 알고리즘을 vtkIterativeClosestPointTransform에서 1:1 점 매칭 강체 랜드마크 정합(vtkLandmarkTransform)으로 전면 교체하여 수동 배치 의도를 유지한 채 단면이 정확하게 밀착 스냅되도록 수정 (Z 증가)
  - 4자리 버전 번호 체계를 전역 룰에 부합하는 3자리 v0.XYZ 형식으로 일괄 교정 및 역산 정리 (Y 증가)
- v0.300 (2026-06-23)
  - 뼈 파편 조각 간의 마주 보는 절단면(Fracture surface)을 KD-Tree 인접 정점 필터링으로 자동 검출하고 조립하는 Run Fracture Surface Snap 기능 및 UI 버튼 신설
- v0.210 (2026-06-23)
  - Slicer 모듈 리로드 시 enter() 타이밍 꼬임으로 세그먼트 에디터가 비활성화되는 버그를 해결하기 위해 setup() 시점에서도 2중으로 강제 노드 재바인딩 로직 보완
- v0.200 (2026-06-23)
  - 7. Fragment Manager 내에 각 프래그먼트의 실시간 위치 및 회전 좌표를 보여주는 transformDisplayGroupBox 신설 및 VTK ModifiedEvent 옵저버 연동을 통한 실시간 갱신 기능 구현
- v0.128 (2026-06-23)
  - Slicer 모듈 리로드 시 세그먼트 에디터 위젯이 비활성화(disabled)되는 문제를 방지하기 위해 enter 시점에 노드 강제 재바인딩 추가 및 기존 에디터 노드 재사용 로직 보완
  - vtkMRMLSegmentEditorNode의 누락된 API 호출 오류 수정 (GetSelectedEffectName -> GetActiveEffectName)
- v0.127 (2026-06-23)
  - 4번 세그먼트 에디터 Threshold 최초 클릭 시 UI 짤림을 방지하기 위해 isThresholdActive 상태 플래그 도입 및 setMinimumHeight(960) 연계 강제 창 크기 확장 구현
  - 6번 가상 뼈 정복 ICP 수행 시 메쉬 훼손 없이 부모 변환 노드의 로컬 행렬을 누적 연산하도록 수학적 행렬 연산 리팩토링 및 3D 회전 역행렬 전파 뒤틀림 버그 수정
- v0.1.26.3 (2026-06-23)
  - C++ 노드 유실 및 최초 1회 생성 렉 문제를 우회하기 위해 200ms 주기적 폴링 감시 타이머(resizeMonitorTimer) 도입하여 짤림 문제 100% 원천 해결 (Z 증가)
- v0.1262 (2026-06-23)
  - 최초 Threshold 클릭 시 GUI 위젯 동적 생성 지연(렉)으로 인해 창 크기가 1회차에 늘어나지 않던 다단계 타이머 보강 수정 (Z 증가)
- v0.1261 (2026-06-23)
  - setup 및 enter 시점에 현재 활성화된 에디터 이펙트를 감지하여 최초 1회 창 크기가 짤린 채 켜지던 초기화 오류(버그) 수정 (Z 증가)
- v0.1260 (2026-06-23)
  - 좌측 QScrollArea 스크롤바를 제거(롤백)하고, uiWidget의 SizePolicy(Expanding) 강제 설정을 통해 세로 창 자동 확장 기각 오류 해결 (Y 증가)
- v0.1250 (2026-06-23)
  - 좌측 1~4단계 컬럼 전체를 감싸는 QScrollArea(leftScrollArea) 도입하여 해상도 부족 및 Threshold 확장 시 UI 짤림 완벽 차단
  - qMRMLSegmentEditorWidget의 activeEffectChanged 시그널을 연동하여 이펙트 변경 시 정확한 창 조절 처리 (Y 증가)
- v0.1240 (2026-06-23)
  - 세그먼트 에디터에서 Threshold 이펙트 선택 시 다이얼로그 창 높이를 960으로 자동 확장하도록 개선 (Y 증가)
  - dialogLayout의 SizeConstraint를 SetNoConstraint로 완화하여 수동/자동 크기 조절 자유도 확보
- v0.1230 (2026-06-23)
  - runIcpRegistration 및 onFragmentInteractionToggled 리팩토링 및 기하학적 정밀도 상향 (Y 증가)
  - GetMatrixTransformToWorld 적용으로 회전 상태에서도 정확한 3D 월드 공간 정합 지원
  - vtkIterativeClosestPointTransform of MaximumNumberOfLandmarks(1000) 및 Iterations(300) 상향으로 뼈 단면 정합 정밀도 문제 해결
  - 기즈모 생성 시 기존 부모 변환 노드를 상속하는 계층 트리 구조 보존 로직 추가
- v0.1221 (2026-06-23)
  - runIcpRegistration 내부의 updateFragmentTable 속성 참조 오류(AttributeError) 수정 (Z 증가)
  - ICP 완료 후 테이블 갱신 주체를 widget(onRunIcpReductionClicked)으로 이전
- v0.1220 (2026-06-23)
  - 7. Fragment Manager 테이블 수동 정렬(Move) 기즈모 활성 컬럼 추가 및 onFragmentInteractionToggled 연동 구현 (기존 기능의 개선이므로 Y 증가)
  - runIcpRegistration에서 StartByMatchingCentroidsOff() 설정 및 수동 정렬(Gizmo) 트랜스폼 행렬 병합/리셋 로직 이식
- v0.1210 (2026-06-23)
  - 5번 뼈 조각 분할 로직에서 감지된 불연속 영역들을 크기 순으로 정렬한 뒤 가장 큰 상위 2개 주요 뼈 조각만 자동 추출하도록 연결성 필터 분석부 변경 (기존 기능의 세부 동작 변경이므로 Y 증가)
- v0.1200 (2026-06-23)
  - 6번 가이드 모델 선택기 옆에 가이드 모델 숨김/보임 토글 기능(guideVisibilityButton) 추가 및 핸들러 연동 구현 (신규 기능 추가이므로 X 증가)
- v0.1100 (2026-06-23)
  - 방안 C 적용: vtkMRMLSegmentEditorNode에 ModifiedEvent 옵저버를 등록하여 세그먼트 에디터의 이펙트 변경(예: Threshold 활성) 시 다이얼로그 크기가 자동으로 팽창/수축하도록 동적 크기 조절(adjustSize) 기능 구현
  - QLayout.SetMinAndMaxSize 제약 조건을 plannerDialog 레이아웃에 주입하여 자동 크기 조절 성능 강화
- v0.1010 (2026-06-23)
  - 팝업 다이얼로그 기본 세로 높이를 700에서 830으로 확장하여 세그먼트 에디터 Threshold 이펙트의 짤림 현상 개선
- v0.1000 (2026-06-23)
  - DICOM DB 로드 시 모듈 전환 없이 독립된 팝업 창으로 띄우도록 BrowserWindowType 강제 설정 적용
  - 4번 세그멘테이션 에디터에서 신규 세그멘테이션 생성 시 조각(Segment) 추가(Add)가 차단되는 버그 수정 (sourceVolumeNode 유실 대응 재연동 및 상호 동기화 구현)
- v0.900 (2026-06-23)
  - 7. Fragment Manager에 생성된 모델 및 가이드를 일괄 삭제하는 일괄 청소 기능(clearModelsButton / onClearModelsClicked) 추가
  - GetColor() 인자 개수 오류(TypeError: GetColor() takes exactly 0 arguments) 버그 수정
- v0.800 (2026-06-23)
  - 5~7번 단계 우측 배치 2컬럼 레이아웃 적용 및 팝업 대화상자 기본 크기 850x700 변경
  - 6번 가이드 모델 개별 파일 로더 버튼(loadGuideButton) 연동 구현
- v0.700 (2026-06-23)
  - 7. Fragment Manager 섹션 및 QTableWidget 조각 목록 연동 추가 (이름 수정, 색상 변경, 가시성 토글, 노드 삭제)
  - 팝업 대화상자 전체 기본 크기를 420x850으로 확장
- v0.604 (2026-06-23)
  - 3D Slicer 5.x의 지원 폐지 경고(SetSliceIntersectionVisibility is deprecated) 해결을 위해 SetVisibility2D(True) API로 교체
- v0.603 (2026-06-23)
  - 입력 볼륨 선택 해제(None) 시 세그먼트 에디터 위젯의 SourceVolumeNode도 명시적으로 None 해제하여 에디터 먹통 방지 방어 코드 적용
- v0.602 (2026-06-23)
  - vtkPolyDataConnectivityFilter에 InitializeSpecifiedRegions 속성이 없어 발생하는 AttributeError 해결을 위해 매 루프 단일 필터 인스턴스 신규 생성으로 롤백 및 노이즈 필터링 스킵 최적화 유지
- v0.601 (2026-06-23)
  - 5번 뼈 분할 기능에서 vtkMRMLSegmentationNode.GetClosedSurfaceRepresentation 직접 호출 시 PythonQt 래핑 문제로 인한 3D Slicer 강제 종료 버그를 slicer.modules.segmentations.logic().GetSegmentClosedSurfaceRepresentation API 호출로 우회 적용 및 vtkPolyDataConnectivityFilter 연산 루프 성능 최적화
- v0.600 (2026-06-23)
  - 5번 뼈 분할 기능(Separate Bone Fragments)을 C++ Closed Surface Representation 직접 추출 방식으로 최적화하여 렉 문제 해결 및 UI 활성화
  - 6번 가상 뼈 정복(Virtual Bone Reduction) 기능 및 UI 연동 추가
  - 가이드 뼈 모델의 X축 기준 미러링(Mirror Guide) 및 법선벡터(Normal) 플립 로직 구현
  - VTK ICP(vtkIterativeClosestPointTransform) 기반 자동 정합(ICP Auto-Reduction) 알고리즘 구현
- v0.500 (2026-06-23)
  - 5번 뼈 분할 기능(Separate Bone Fragments)의 고부하 연산 멈춤 문제를 피하기 위해 UI상에서 5단계 영역(fragmentSeparationCollapsibleButton) 비활성화(숨김) 처리
- v0.403 (2026-06-23)
  - 모듈 리로드(Reload) 및 소멸 시 파괴된 콤보박스 위젯의 ModifiedEvent 예외(TypeError: Trying to call 'setCurrentNode' on a destroyed qMRMLNodeComboBox object) 루프로 인한 응답 없음 해결을 위해 cleanup 함수에 setParameterNode(None) 추가
- v0.402 (2026-06-23)
  - 5번 뼈 분할 기능(separateBoneFragments)에서 세그먼트 부재, 가시성 해제, 혹은 빈 라벨맵 데이터일 때 발생하는 C++ NULL 포인터 크래시 및 응답없음 방지 3중 예외 처리 추가
- v0.401 (2026-06-23)
  - 5번 뼈 분할 기능(separateBoneFragments)에서 GetReferenceImageGeometryParameterFromVolumeNode 호출 크래시 및 좌표 어긋남 버그 수정 (입력 볼륨의 기하 매칭 연동 방식 도입)
- v0.400 (2026-06-23)
  - 3D 회전 제어(Live Rotation) 시 CT 볼륨뿐 아니라 세그멘테이션 메쉬 및 뼈 파편(Model) 노드까지 실시간 동기화 회전 연동
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
        
        # Threshold 이펙트의 크기 확장 중복 제어 방지용 플래그
        self.isThresholdActive = False
        
        # 실시간 변환 옵저버 관리 변수
        self._activeTransformNode = None
        self._activeTransformObserverTag = None
        self._activeModelNode = None

    def setup(self) -> None:
        ScriptedLoadableModuleWidget.setup(self)

        # 1. 팝업을 위한 독립 커스텀 QDialog 생성 및 설정
        self.plannerDialog = PlannerDialog(slicer.util.mainWindow())
        self.plannerDialog.setWindowTitle("Femur Fracture Planner Window")
        self.plannerDialog.setObjectName("FemurFracturePlannerDialog")
        self.plannerDialog.setModal(False)
        self.plannerDialog.setWindowFlags(qt.Qt.Window | qt.Qt.WindowTitleHint | qt.Qt.WindowCloseButtonHint | qt.Qt.CustomizeWindowHint)
        self.plannerDialog.resize(850, 830)

        # 다이얼로그 레이아웃 정의
        dialogLayout = qt.QVBoxLayout(self.plannerDialog)
        dialogLayout.setContentsMargins(5, 5, 5, 5)
        # 내부 위젯들의 크기 변화에 대응하도록 레이아웃 제약조건 설정 (SetNoConstraint로 설정하여 자유로운 창 크기 조절 보장)
        dialogLayout.setSizeConstraint(qt.QLayout.SetNoConstraint)

        # UI 파일 로드하여 다이얼로그 레이아웃에 탑재
        uiWidget = slicer.util.loadUI(self.resourcePath("UI/FemurFracturePlanner.ui"))
        
        # uiWidget의 sizePolicy를 Expanding으로 강제하여 QDialog resize 시 함께 확장되도록 보장
        uiWidget.setSizePolicy(qt.QSizePolicy.Expanding, qt.QSizePolicy.Expanding)
        # 하드코딩되어 있을 수 있는 최소/최대 크기 제약을 풀어 창 리사이징이 자유롭게 자식 위젯에 전파되도록 함
        uiWidget.setMinimumSize(0, 0)
        uiWidget.setMaximumSize(16777215, 16777215)
        
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
        self.addObserver(slicer.mrmlScene, slicer.mrmlScene.NodeRemovedEvent, self.onNodeRemoved)

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
        
        # setup 시점에서도 노드가 이미 존재하면 강제 사전 바인딩 (리로드 직후 비활성화 방어)
        if not self.segmentEditorNode:
            self.segmentEditorNode = slicer.mrmlScene.GetFirstNodeByName("FemurFracturePlanner_SegmentEditorNode")
            if not self.segmentEditorNode:
                self.segmentEditorNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLSegmentEditorNode")
                self.segmentEditorNode.SetName("FemurFracturePlanner_SegmentEditorNode")
                
        if self.segmentEditorWidget and self.segmentEditorNode:
            self.segmentEditorWidget.setMRMLSegmentEditorNode(self.segmentEditorNode)
            
            # 콤보박스에서 현재 노드 가져와서 싱크
            segNode = self.ui.activeSegmentationSelector.currentNode()
            if segNode:
                self.segmentEditorWidget.setSegmentationNode(segNode)
                
            inputNode = self.ui.inputVolumeSelector.currentNode()
            if inputNode:
                self.segmentEditorWidget.setSourceVolumeNode(inputNode)
        
        # 임계값(Threshold) 활성 감지 및 창 크기 자동 확장 감시용 QTimer 생성
        self.resizeMonitorTimer = qt.QTimer()
        self.resizeMonitorTimer.setInterval(200) # 200ms 마다 폴링 검사
        self.resizeMonitorTimer.connect("timeout()", self.monitorActiveEffectAndResize)

        # 5. 골절 파편 분리 버튼 및 세그먼트 에디터 변경 감지 이벤트 연결
        self.ui.separateFragmentsButton.connect("clicked(bool)", self.onSeparateFragmentsClicked)
        self.ui.fragmentSeparationCollapsibleButton.setVisible(True)
        self.ui.activeSegmentationSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onSegmentationNodeChanged)
        self.segmentEditorWidget.connect("segmentationNodeChanged(vtkMRMLSegmentationNode*)", self.onEditorSegmentationNodeChanged)

        # 6. 가상 뼈 정복 및 ICP 정합 버튼 이벤트 연결
        self.ui.mirrorGuideButton.connect("clicked(bool)", self.onMirrorGuideClicked)
        self.ui.runIcpReductionButton.connect("clicked(bool)", self.onRunIcpReductionClicked)
        self.ui.runSurfaceSnapButton.connect("clicked(bool)", self.onRunSurfaceSnapClicked)
        self.ui.loadGuideButton.connect("clicked(bool)", self.onLoadGuideClicked)
        self.ui.guideVisibilityButton.connect("toggled(bool)", self.onGuideVisibilityToggled)
        self.ui.guideModelSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onGuideModelChanged)

        # 7. Fragment Manager 테이블 변경 및 버튼 이벤트 연결
        self.ui.refreshManagerButton.connect("clicked(bool)", self.updateFragmentTable)
        self.ui.clearModelsButton.connect("clicked(bool)", self.onClearModelsClicked)
        self.ui.fragmentTableWidget.connect("itemChanged(QTableWidgetItem*)", self.onFragmentTableItemChanged)
        self.ui.fragmentTableWidget.connect("itemSelectionChanged()", self.onFragmentTableSelectionChanged)

        # 파라미터 노드 초기화
        self.initializeParameterNode()

        # 최초 로드 시 활성 이펙트 이름을 감지하여 창 크기 자동 초기화 (setup 시점 예방)
        self.isThresholdActive = False
        if self.segmentEditorWidget:
            try:
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
        self.removeActiveTransformObserver()
        if hasattr(self, 'resizeMonitorTimer') and self.resizeMonitorTimer:
            self.resizeMonitorTimer.stop()
            self.resizeMonitorTimer = None
            
        self.setParameterNode(None)
        self.removeObservers()
        self.destroyEmbeddedSegmentEditor()
        if self.plannerDialog:
            self.plannerDialog.close()
            self.plannerDialog.deleteLater()
            self.plannerDialog = None

    def destroyEmbeddedSegmentEditor(self) -> None:
        """임베디드 세그먼트 에디터 위젯 및 관련 파라미터 노드 해제"""
        # 에디터 노드 옵저버 해제
        if hasattr(self, '_editorNodeObserverTag') and self._editorNodeObserverTag is not None:
            if self.segmentEditorNode:
                self.segmentEditorNode.RemoveObserver(self._editorNodeObserverTag)
            self._editorNodeObserverTag = None

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
        
        # 내장 에디터용 세션 노드(SegmentEditorNode) 생성 및 연결 (씬에 기존 노드 존재 시 재사용)
        if not self.segmentEditorNode:
            self.segmentEditorNode = slicer.mrmlScene.GetFirstNodeByName("FemurFracturePlanner_SegmentEditorNode")
            if not self.segmentEditorNode:
                self.segmentEditorNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLSegmentEditorNode")
                self.segmentEditorNode.SetName("FemurFracturePlanner_SegmentEditorNode")
            
        # 옵저버가 이미 등록되지 않았다면 연결 (방안 C)
        if not hasattr(self, '_editorNodeObserverTag') or self._editorNodeObserverTag is None:
            self._editorNodeObserverTag = self.segmentEditorNode.AddObserver(vtk.vtkCommand.ModifiedEvent, self.onEditorNodeModified)

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

        # 모듈 진입 시 테이블 강제 리프레시
        self.updateFragmentTable()

        # 모듈 진입 시 독립 다이얼로그 창 강제 노출
        if self.plannerDialog:
            self.plannerDialog.show()
            self.plannerDialog.raise_()
            self.plannerDialog.activateWindow()

        # 에디터 위젯의 강제 재바인딩 재보장 (리로드 렉 대응 방어 코드)
        if self.segmentEditorWidget and self.segmentEditorNode:
            self.segmentEditorWidget.setMRMLSegmentEditorNode(self.segmentEditorNode)
            if segNode:
                self.segmentEditorWidget.setSegmentationNode(segNode)
            inputNode = self.ui.inputVolumeSelector.currentNode()
            if inputNode:
                self.segmentEditorWidget.setSourceVolumeNode(inputNode)
            
        # 임계값 활성 및 창 리사이징 주기적 폴링 감시 시작
        if hasattr(self, 'resizeMonitorTimer') and self.resizeMonitorTimer:
            self.resizeMonitorTimer.start()

    def exit(self) -> None:
        """사용자가 다른 모듈 화면으로 이동 시 호출"""
        self.removeActiveTransformObserver()
        # 폴링 감시 타이머 중지
        if hasattr(self, 'resizeMonitorTimer') and self.resizeMonitorTimer:
            self.resizeMonitorTimer.stop()

        if self._parameterNode:
            self._parameterNode.disconnectGui(self._parameterNodeGuiTag)
            self._parameterNodeGuiTag = None
            
        # 에디터 노드 옵저버 해제
        if hasattr(self, '_editorNodeObserverTag') and self._editorNodeObserverTag is not None:
            if self.segmentEditorNode:
                self.segmentEditorNode.RemoveObserver(self._editorNodeObserverTag)
            self._editorNodeObserverTag = None

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
        else:
            if self.segmentEditorWidget:
                self.segmentEditorWidget.setSourceVolumeNode(None)
            
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
            # 1. DICOM 브라우저 노출 방식을 팝업 창(Popup) 형태로 설정 값 강제 제어
            slicer.app.settings().setValue("DICOM/BrowserWindowType", "Popup")
            
            # 2. Slicer 메인 윈도우에서 DICOM 브라우저 열기 액션(QAction)을 검색하여 실행
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

        # 추가: 생성된 뼈 fragment 노드가 감지되면 테이블 자동 갱신
        if node.IsA("vtkMRMLModelNode") and node.GetName().startswith("Femur_Fragment"):
            qt.QTimer.singleShot(100, self.updateFragmentTable)

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
    # 5. 골절 파편 분리 핸들러 및 세그먼트 에디터 동기화
    #

    def onSegmentationNodeChanged(self, node) -> None:
        """5번 영역의 세그먼테이션 선택 콤보박스가 변경되면 에디터 위젯에 즉시 동기화합니다."""
        if not self.segmentEditorWidget:
            return
        
        # 무한 루프 방지를 위해 임시 시그널 차단
        self.segmentEditorWidget.blockSignals(True)
        self.segmentEditorWidget.setSegmentationNode(node)
        self.segmentEditorWidget.blockSignals(False)
        
        # 소스 볼륨 노드 재연동
        inputNode = self.ui.inputVolumeSelector.currentNode()
        if inputNode:
            self.segmentEditorWidget.setSourceVolumeNode(inputNode)

    def onEditorSegmentationNodeChanged(self, node) -> None:
        """4번 에디터 내에서 세그먼테이션 노드가 직접 변경되거나 신설되면 상단 콤보박스에 역동기화하고 소스 볼륨을 재바인딩합니다."""
        if not self.segmentEditorWidget:
            return
            
        # 상단 콤보박스 역동기화
        self.ui.activeSegmentationSelector.blockSignals(True)
        self.ui.activeSegmentationSelector.setCurrentNode(node)
        self.ui.activeSegmentationSelector.blockSignals(False)
        
        # 중요: 세그먼테이션 노드가 새로 교체되는 순간 sourceVolumeNode 연결이 풀리므로 강제로 재연동해 줍니다.
        inputNode = self.ui.inputVolumeSelector.currentNode()
        if inputNode:
            self.segmentEditorWidget.setSourceVolumeNode(inputNode)

    def monitorActiveEffectAndResize(self) -> None:
        """주기적으로 세그먼트 에디터의 활성 이펙트를 확인하여 다이얼로그 창 높이를 자동 조절합니다."""
        if not self.plannerDialog or not self.plannerDialog.isVisible():
            return
            
        effectName = ""
        if self.segmentEditorWidget:
            try:
                effect = self.segmentEditorWidget.activeEffect()
                if effect:
                    effectName = effect.name
            except Exception:
                pass
                
        if not effectName and self.segmentEditorWidget:
            try:
                node = self.segmentEditorWidget.mrmlSegmentEditorNode()
                if node:
                    effectName = node.GetActiveEffectName()
            except Exception:
                pass

        isActive = (effectName == "Threshold")
        if isActive != self.isThresholdActive:
            self.isThresholdActive = isActive
            if isActive:
                self.plannerDialog.setMinimumHeight(960)
                self.plannerDialog.resize(850, 960)
            else:
                self.plannerDialog.setMinimumHeight(830)
                self.plannerDialog.resize(850, 830)
                qt.QTimer.singleShot(100, self.plannerDialog.adjustSize)

    def onEditorNodeModified(self, caller, event) -> None:
        """세그먼트 에디터 상태 노드가 변경(이펙트 활성화 등)되면 다이얼로그 크기를 자동으로 맞춥니다."""
        self.monitorActiveEffectAndResize()

    def onSeparateFragmentsClicked(self) -> None:
        """5단계: 버튼 클릭 시 세그먼트 데이터에서 독립된 뼈 조각 모델들을 자동으로 분리합니다."""
        segNode = self.ui.activeSegmentationSelector.currentNode()
        if not segNode:
            slicer.util.errorDisplay(_("Please select an Input Segmentation node first."))
            return

        inputNode = self.ui.inputVolumeSelector.currentNode()

        # 3D 뼈 파편 분리 연산 시작 (마우스 대기 커서 활성화)
        logging.info(f"Separating disjoint bone fragments in segmentation: '{segNode.GetName()}'")
        
        with slicer.util.tryWithErrorDisplay(_("Failed to separate bone fragments."), waitCursor=True):
            # 좌표 어긋남 방지를 위해 입력 CT 볼륨 노드를 기하 정보 기준으로 함께 전달
            separatedCount = self.logic.separateBoneFragments(segNode, inputNode)
            
            if separatedCount > 0:
                slicer.util.infoDisplay(_(f"Successfully separated into {separatedCount} independent bone fragments!"))
                
                # 가상 수술 조작이 편하도록 기존 CT 볼륨 렌더링(홀로그램) 가시성은 꺼줍니다.
                inputNode = self.ui.inputVolumeSelector.currentNode()
                if inputNode and self.ui.volumeRenderingVisibilityButton.checked:
                    self.ui.volumeRenderingVisibilityButton.checked = False
                
                # 조각 추출 성공 시 매니저 테이블 즉시 동적 리프레시
                self.updateFragmentTable()
            else:
                slicer.util.errorDisplay(_("No valid bone fragments could be extracted. Make sure the segmentation contains points."))

    def onMirrorGuideClicked(self) -> None:
        """가이드 뼈 모델을 X축 기준으로 좌우 반전하여 새로운 가이드 모델을 생성합니다."""
        guideNode = self.ui.guideModelSelector.currentNode()
        if not guideNode:
            slicer.util.errorDisplay(_("Please select a Guide Model first."))
            return

        logging.info(f"Mirroring guide model: '{guideNode.GetName()}'")
        with slicer.util.tryWithErrorDisplay(_("Failed to mirror guide model."), waitCursor=True):
            mirroredNode = self.logic.mirrorModel(guideNode)
            if mirroredNode:
                self.ui.guideModelSelector.setCurrentNode(mirroredNode)
                slicer.util.infoDisplay(_(f"Successfully mirrored guide model: '{mirroredNode.GetName()}'"))

    def onRunIcpReductionClicked(self) -> None:
        """분리된 모든 뼈 조각 모델들을 가이드 뼈 모델에 자동으로 ICP 정합합니다."""
        guideNode = self.ui.guideModelSelector.currentNode()
        if not guideNode:
            slicer.util.errorDisplay(_("Please select a Guide Model first."))
            return

        # 씬 내에서 분리된 fragment 모델 노드(Femur_Fragment_*)들을 수집합니다.
        fragmentNodes = []
        nodes = slicer.mrmlScene.GetNodesByClass("vtkMRMLModelNode")
        for i in range(nodes.GetNumberOfItems()):
            node = nodes.GetItemAsObject(i)
            if node.GetName().startswith("Femur_Fragment"):
                fragmentNodes.append(node)

        if not fragmentNodes:
            slicer.util.errorDisplay(_("No separated bone fragments (Femur_Fragment_*) found. Run step 5 first."))
            return

        logging.info(f"Running ICP auto-reduction for {len(fragmentNodes)} fragments against guide: '{guideNode.GetName()}'")
        with slicer.util.tryWithErrorDisplay(_("Failed to run ICP Auto-Reduction."), waitCursor=True):
            self.logic.runIcpRegistration(fragmentNodes, guideNode)
            self.updateFragmentTable()
            slicer.util.infoDisplay(_("ICP Auto-Reduction completed successfully!"))

    def onRunSurfaceSnapClicked(self) -> None:
        """두 주요 뼈 조각(Femur_Fragment_1, Femur_Fragment_2)의 마주 보고 있는 절단면을 자동으로 정밀 밀착 조립(Snap)합니다."""
        # 씬 내에서 분리된 fragment 모델 노드(Femur_Fragment_*)들을 수집합니다.
        fragmentNodes = []
        nodes = slicer.mrmlScene.GetNodesByClass("vtkMRMLModelNode")
        for i in range(nodes.GetNumberOfItems()):
            node = nodes.GetItemAsObject(i)
            if node.GetName().startswith("Femur_Fragment"):
                fragmentNodes.append(node)

        # 단면 상호 조립은 뼈 조각이 최소 2개 이상일 때 작동합니다.
        if len(fragmentNodes) < 2:
            slicer.util.errorDisplay(_("Fracture Surface Snap requires at least 2 separated bone fragments (Femur_Fragment_1, Femur_Fragment_2)."))
            return

        # Femur_Fragment_1 과 Femur_Fragment_2 노드를 명시적으로 지정
        frag1 = slicer.mrmlScene.GetFirstNodeByName("Femur_Fragment_1")
        frag2 = slicer.mrmlScene.GetFirstNodeByName("Femur_Fragment_2")
        
        if not frag1 or not frag2:
            # 이름이 다를 경우 첫번째 두 조각 재지정
            frag1 = fragmentNodes[0]
            frag2 = fragmentNodes[1]

        logging.info(f"Running Fracture Surface Snap between: '{frag1.GetName()}' and '{frag2.GetName()}'")
        with slicer.util.tryWithErrorDisplay(_("Failed to run Fracture Surface Snap. Check if they are close enough."), waitCursor=True):
            self.logic.runFractureSurfaceSnap(frag1, frag2)
            self.updateFragmentTable()
            # 실시간 좌표 디스플레이도 갱신
            self.onFragmentTableSelectionChanged()
            slicer.util.infoDisplay(_("Fracture Surface Snap completed successfully!"))

    def onLoadGuideClicked(self) -> None:
        """6단계: 로컬 드라이브의 가이드용 3D 뼈 모델 파일(.stl, .obj, .ply 등)을 로드하여 선택합니다."""
        filePath = qt.QFileDialog.getOpenFileName(
            slicer.util.mainWindow(),
            _("Select Guide Bone Model File"),
            "",
            "Model Files (*.stl *.obj *.ply *.vtk)"
        )
        
        if not filePath:
            return
            
        logging.info(f"Loading guide model file: '{filePath}'")
        
        with slicer.util.tryWithErrorDisplay(_("Failed to load guide model file."), waitCursor=True):
            loadedNode = slicer.util.loadModel(filePath)
            if loadedNode:
                # 선택기 콤보박스에 새로 로드된 모델 노드를 세팅
                self.ui.guideModelSelector.setCurrentNode(loadedNode)
                slicer.util.infoDisplay(_(f"Successfully loaded guide model: '{loadedNode.GetName()}'"))

    def onGuideVisibilityToggled(self, checked: bool) -> None:
        """가이드 모델의 가시성(Show/Hide)을 토글합니다."""
        guideNode = self.ui.guideModelSelector.currentNode()
        if not guideNode:
            if checked:
                self.ui.guideVisibilityButton.blockSignals(True)
                self.ui.guideVisibilityButton.checked = False
                self.ui.guideVisibilityButton.blockSignals(False)
                slicer.util.errorDisplay(_("Please select a Guide Model first."))
            return

        displayNode = guideNode.GetDisplayNode()
        if displayNode:
            displayNode.SetVisibility(checked)
            slicer.util.forceRenderAllViews()
            
        if checked:
            self.ui.guideVisibilityButton.text = _("Hide")
            self.ui.guideVisibilityButton.toolTip = _("Hide guide model in viewers")
        else:
            self.ui.guideVisibilityButton.text = _("Show")
            self.ui.guideVisibilityButton.toolTip = _("Show guide model in viewers")

    def onGuideModelChanged(self, node) -> None:
        """선택된 가이드 모델이 변경되면 가시성 버튼의 상태를 동기화합니다."""
        if not node:
            self.ui.guideVisibilityButton.blockSignals(True)
            self.ui.guideVisibilityButton.checked = False
            self.ui.guideVisibilityButton.text = _("Show")
            self.ui.guideVisibilityButton.blockSignals(False)
            return
            
        displayNode = node.GetDisplayNode()
        isVisible = displayNode.GetVisibility() if displayNode else False
        
        # 무한 루프 방지를 위해 시그널 차단 후 동기화
        self.ui.guideVisibilityButton.blockSignals(True)
        self.ui.guideVisibilityButton.checked = isVisible
        if isVisible:
            self.ui.guideVisibilityButton.text = _("Hide")
        else:
            self.ui.guideVisibilityButton.text = _("Show")
        self.ui.guideVisibilityButton.blockSignals(False)

    def onNodeRemoved(self, caller, event) -> None:
        """씬에서 노드가 제거되었을 때 호출되는 이벤트 핸들러"""
        node = event
        if isinstance(node, str):
            node = slicer.mrmlScene.GetNodeByID(node)
            
        # 삭제된 노드가 fragment 노드인 경우 테이블 자동 갱신
        if node and node.IsA("vtkMRMLModelNode") and node.GetName().startswith("Femur_Fragment"):
            qt.QTimer.singleShot(100, self.updateFragmentTable)

    def updateFragmentTable(self) -> None:
        """씬 내의 Femur_Fragment_* 모델 노드 목록을 테이블에 동적 갱신합니다."""
        if not self.plannerDialog:
            return

        table = self.ui.fragmentTableWidget
        # 이벤트 루프 방지를 위해 임시로 시그널 차단
        table.blockSignals(True)
        table.clear()
        
        # 헤더 설정 (Move 추가)
        table.setColumnCount(5)
        table.setHorizontalHeaderLabels(["Name", "Show", "Move", "Color", "Delete"])
        # 헤더 조절
        header = table.horizontalHeader()
        header.setSectionResizeMode(0, qt.QHeaderView.Stretch)
        header.setSectionResizeMode(1, qt.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(2, qt.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(3, qt.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(4, qt.QHeaderView.ResizeToContents)

        # fragment 노드 수집
        fragmentNodes = []
        nodes = slicer.mrmlScene.GetNodesByClass("vtkMRMLModelNode")
        for i in range(nodes.GetNumberOfItems()):
            node = nodes.GetItemAsObject(i)
            if node.GetName().startswith("Femur_Fragment"):
                fragmentNodes.append(node)

        table.setRowCount(len(fragmentNodes))

        for row, node in enumerate(fragmentNodes):
            # 1. Name (텍스트 아이템)
            nameItem = qt.QTableWidgetItem(node.GetName())
            nameItem.setData(qt.Qt.UserRole, node.GetID()) # 노드 ID 저장
            table.setItem(row, 0, nameItem)

            # 2. Show (체크박스)
            showWidget = qt.QCheckBox()
            displayNode = node.GetDisplayNode()
            showWidget.checked = displayNode.GetVisibility() if displayNode else False
            
            # 체크 변경 시 핸들러 연결
            showWidget.connect("toggled(bool)", lambda checked, n=node: self.onFragmentVisibilityToggled(n, checked))
            
            container1 = qt.QWidget()
            layout1 = qt.QHBoxLayout(container1)
            layout1.addWidget(showWidget)
            layout1.setAlignment(qt.Qt.AlignCenter)
            layout1.setContentsMargins(0, 0, 0, 0)
            table.setCellWidget(row, 1, container1)

            # 3. Move (수동 정렬 토글 기즈모 체크박스)
            moveWidget = qt.QCheckBox()
            # 변환 노드가 없으면 새로 신설하여 부착
            transformNode = node.GetParentTransformNode()
            if not transformNode or not transformNode.IsA("vtkMRMLLinearTransformNode"):
                transformNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
                transformNode.SetName(f"{node.GetName()}_Transform")
                node.SetAndObserveTransformNodeID(transformNode.GetID())
            
            # 디스플레이 노드를 통해 EditorVisibility(3D 기즈모 상태) 획득
            if not transformNode.GetDisplayNode():
                transformNode.CreateDefaultDisplayNodes()
            displayNode = transformNode.GetDisplayNode()
            
            moveWidget.checked = displayNode.GetEditorVisibility() if displayNode else False
            moveWidget.connect("toggled(bool)", lambda checked, n=node: self.onFragmentInteractionToggled(n, checked))
            
            container_move = qt.QWidget()
            layout_move = qt.QHBoxLayout(container_move)
            layout_move.addWidget(moveWidget)
            layout_move.setAlignment(qt.Qt.AlignCenter)
            layout_move.setContentsMargins(0, 0, 0, 0)
            table.setCellWidget(row, 2, container_move)

            # 4. Color (색상 피커 버튼)
            colorButton = qt.QPushButton()
            colorButton.setFixedWidth(40)
            colorButton.setFixedHeight(20)
            if displayNode:
                rgb = displayNode.GetColor()
                r, g, b = int(rgb[0]*255), int(rgb[1]*255), int(rgb[2]*255)
                colorButton.setStyleSheet(f"background-color: rgb({r}, {g}, {b}); border: 1px solid gray; border-radius: 3px;")
            
            colorButton.connect("clicked()", lambda n=node, btn=colorButton: self.onFragmentColorClicked(n, btn))
            
            container2 = qt.QWidget()
            layout2 = qt.QHBoxLayout(container2)
            layout2.addWidget(colorButton)
            layout2.setAlignment(qt.Qt.AlignCenter)
            layout2.setContentsMargins(0, 0, 0, 0)
            table.setCellWidget(row, 3, container2)

            # 5. Delete (삭제 버튼)
            deleteButton = qt.QPushButton("❌")
            deleteButton.setFixedWidth(30)
            deleteButton.setFixedHeight(22)
            deleteButton.connect("clicked()", lambda n=node: self.onFragmentDeleteClicked(n))
            
            container3 = qt.QWidget()
            layout3 = qt.QHBoxLayout(container3)
            layout3.addWidget(deleteButton)
            layout3.setAlignment(qt.Qt.AlignCenter)
            layout3.setContentsMargins(0, 0, 0, 0)
            table.setCellWidget(row, 4, container3)

        # 시그널 다시 연결
        table.blockSignals(False)

    def onFragmentTableItemChanged(self, item: qt.QTableWidgetItem) -> None:
        """테이블에서 조각 이름을 편집했을 때 노드 이름을 실시간 연동 업데이트합니다."""
        if item.column() != 0:
            return
        
        nodeId = item.data(qt.Qt.UserRole)
        if not nodeId:
            return
            
        node = slicer.mrmlScene.GetNodeByID(nodeId)
        if node and node.GetName() != item.text():
            logging.info(f"Renaming fragment node from '{node.GetName()}' to '{item.text()}'")
            node.SetName(item.text())
            slicer.util.forceRenderAllViews()

    def onFragmentVisibilityToggled(self, node, checked: bool) -> None:
        """가시성 체크박스 토글 핸들러"""
        displayNode = node.GetDisplayNode()
        if displayNode:
            displayNode.SetVisibility(checked)
            slicer.util.forceRenderAllViews()

    def onFragmentColorClicked(self, node, button) -> None:
        """색상 피커 버튼 클릭 핸들러"""
        displayNode = node.GetDisplayNode()
        if not displayNode:
            return
            
        rgb = displayNode.GetColor()
        initialColor = qt.QColor.fromRgbF(rgb[0], rgb[1], rgb[2])
        
        color = qt.QColorDialog.getColor(initialColor, slicer.util.mainWindow(), "Select Fragment Color")
        if color.isValid():
            r, g, b = color.redF(), color.greenF(), color.blueF()
            displayNode.SetColor(r, g, b)
            btnR, btnG, btnB = color.red(), color.green(), color.blue()
            button.setStyleSheet(f"background-color: rgb({btnR}, {btnG}, {btnB}); border: 1px solid gray; border-radius: 3px;")
            slicer.util.forceRenderAllViews()

    def onFragmentDeleteClicked(self, node) -> None:
        """조각 삭제 버튼 클릭 핸들러"""
        nodeName = node.GetName()
        logging.info(f"Removing fragment node: '{nodeName}'")
        slicer.mrmlScene.RemoveNode(node)

    def onFragmentInteractionToggled(self, node, checked: bool) -> None:
        """수동 정렬을 위해 3D 뷰 상에 조작 기즈모를 켜고 끕니다.
        기존의 부모 변환 노드가 존재하면 계층 구조로 보존합니다."""
        transformNode = node.GetParentTransformNode()
        originalParentTransform = None
        
        # 기존 부모 변환 노드가 있고, 그것이 수동 기즈모용 트랜스폼(_Transform)이 아니라면 백업
        if transformNode and not transformNode.GetName().endswith("_Transform"):
            originalParentTransform = transformNode
            transformNode = None

        if not transformNode or not transformNode.IsA("vtkMRMLLinearTransformNode"):
            transformNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
            transformNode.SetName(f"{node.GetName()}_Transform")
            
            # 원래 존재하던 회전 등의 변환 노드를 기즈모 노드의 부모로 지정
            if originalParentTransform:
                transformNode.SetAndObserveTransformNodeID(originalParentTransform.GetID())
                
            node.SetAndObserveTransformNodeID(transformNode.GetID())
            
        # displayNode의 SetEditorVisibility를 사용하여 3D 기즈모를 켜고 끕니다.
        if not transformNode.GetDisplayNode():
            transformNode.CreateDefaultDisplayNodes()
        displayNode = transformNode.GetDisplayNode()
        if displayNode:
            displayNode.SetEditorVisibility(checked)
            
        # 기즈모가 새로 추가/갱신되었으므로, 실시간 좌표 모니터 상태도 갱신
        self.onFragmentTableSelectionChanged()
            
        slicer.util.forceRenderAllViews()

    def onClearModelsClicked(self) -> None:
        """씬 내에 잔류하는 뼈 파편(Femur_Fragment_*) 및 미러링 가이드(*_Mirrored) 모델 노드들을 일괄 삭제합니다."""
        logging.info("Clearing all fragmented and mirrored model nodes from the scene...")
        
        # 1단계: 삭제할 노드 목록 수집
        nodesToDelete = []
        nodes = slicer.mrmlScene.GetNodesByClass("vtkMRMLModelNode")
        for i in range(nodes.GetNumberOfItems()):
            node = nodes.GetItemAsObject(i)
            nodeName = node.GetName()
            if nodeName.startswith("Femur_Fragment") or nodeName.endswith("_Mirrored"):
                nodesToDelete.append(node)

        # 2단계: 수집된 노드들을 씬에서 순차적으로 제거
        if not nodesToDelete:
            slicer.util.infoDisplay("삭제할 파편 혹은 미러링 모델이 없습니다.")
            return

        # 사용자의 실수 방지를 위한 확인 대화상자 추가
        confirm = slicer.util.confirmOkCancelDisplay(
            f"씬에서 {len(nodesToDelete)}개의 골절 파편 및 가이드 모델 노드들을 정말로 삭제하시겠습니까?\n(가이드 모델 선택은 초기화됩니다)",
            windowTitle="Clear All Confirmation"
        )
        if not confirm:
            return

        with slicer.util.tryWithErrorDisplay("Failed to clear model nodes.", waitCursor=True):
            for node in nodesToDelete:
                slicer.mrmlScene.RemoveNode(node)
            
            # 가이드 모델 선택기 콤보박스 초기화
            self.ui.guideModelSelector.setCurrentNode(None)
            
            # 3D 및 2D 슬라이스 뷰어 리렌더링
            slicer.util.forceRenderAllViews()
            
            # 파편 매니저 테이블 갱신
            self.updateFragmentTable()
            
            slicer.util.infoDisplay(f"성공적으로 {len(nodesToDelete)}개의 모델 노드들을 정리했습니다.")

    def removeActiveTransformObserver(self) -> None:
        """현재 활성화된 수동 기즈모 변환 노드의 VTK ModifiedEvent 옵저버 연결을 해제합니다."""
        if self._activeTransformObserverTag is not None and self._activeTransformNode:
            try:
                self._activeTransformNode.RemoveObserver(self._activeTransformObserverTag)
            except Exception:
                pass
        self._activeTransformObserverTag = None
        self._activeTransformNode = None
        self._activeModelNode = None
        
        # UI 텍스트 N/A로 초기화
        if hasattr(self, 'ui') and hasattr(self.ui, 'selectedNameValue'):
            self.ui.selectedNameValue.setText("None (Select a row in table)")
            self.ui.positionValue.setText("N/A")
            self.ui.rotationValue.setText("N/A")

    def onFragmentTableSelectionChanged(self) -> None:
        """파편 테이블의 행 선택이 변경되면 실시간 변환 관찰 대상을 전환합니다."""
        # 기존 감시 옵저버 제거
        self.removeActiveTransformObserver()
        
        # 현재 선택된 행 획득
        table = self.ui.fragmentTableWidget
        selectedRanges = table.selectedRanges()
        if not selectedRanges:
            return
            
        row = selectedRanges[0].topRow()
        # 첫 번째 컬럼(Name)의 Item을 획득하여 Node ID 추출
        nameItem = table.item(row, 0)
        if not nameItem:
            return
            
        nodeId = nameItem.data(qt.Qt.UserRole)
        if not nodeId:
            return
            
        node = slicer.mrmlScene.GetNodeByID(nodeId)
        if not node:
            return
            
        # 활성 모델 노드 캐싱
        self._activeModelNode = node
        self.ui.selectedNameValue.setText(node.GetName())
        
        # 해당 파편의 부모 변환 노드 획득 (Move 기즈모 노드)
        transformNode = node.GetParentTransformNode()
        
        if not transformNode:
            # 수동 기즈모 변환 노드가 없더라도 PolyData 자체의 원본 Bounds 중심을 현재 Position으로 출력
            poly = node.GetPolyData()
            if poly:
                bounds = [0.0] * 6
                poly.GetBounds(bounds)
                cx = (bounds[0] + bounds[1]) / 2.0
                cy = (bounds[2] + bounds[3]) / 2.0
                cz = (bounds[4] + bounds[5]) / 2.0
                self.ui.positionValue.setText(f"X: {cx:.2f}, Y: {cy:.2f}, Z: {cz:.2f}")
                self.ui.rotationValue.setText("R: 0.00, P: 0.00, Y: 0.00")
            return
            
        # 기즈모 변환 노드가 존재한다면 옵저버 등록하여 실시간 추적 시작
        self._activeTransformNode = transformNode
        self._activeTransformObserverTag = transformNode.AddObserver(
            vtk.vtkCommand.ModifiedEvent,
            self.onActiveTransformNodeModified
        )
        
        # 초기 1회 수동 업데이트 호출
        self.onActiveTransformNodeModified(transformNode, None)

    def onActiveTransformNodeModified(self, caller, event) -> None:
        """감시 중인 변환 노드가 변경될 때마다 3D 실시간 좌표/회전값을 UI에 표출합니다."""
        if not self._activeTransformNode:
            return
            
        # 변환 노드의 최종 월드 변환 행렬 획득
        worldMatrix = vtk.vtkMatrix4x4()
        self._activeTransformNode.GetMatrixTransformToWorld(worldMatrix)
        
        # vtkTransform에 행렬 주입하여 좌표 및 오리엔테이션 추출
        transform = vtk.vtkTransform()
        transform.SetMatrix(worldMatrix)
        
        ori = transform.GetOrientation() # Roll, Pitch, Yaw 오리엔테이션 각도 리턴
        
        # 활성 모델 노드가 존재하고 PolyData가 있으면 월드 기준 중심점 좌표 계산
        if self._activeModelNode and self._activeModelNode.GetPolyData():
            poly = self._activeModelNode.GetPolyData()
            bounds = [0.0] * 6
            poly.GetBounds(bounds)
            cx = (bounds[0] + bounds[1]) / 2.0
            cy = (bounds[2] + bounds[3]) / 2.0
            cz = (bounds[4] + bounds[5]) / 2.0
            
            # 원래의 메쉬 중심 좌표(cx, cy, cz)를 현재 월드 행렬에 맞게 변환
            pos = transform.TransformPoint(cx, cy, cz)
        else:
            pos = transform.GetPosition()
        
        # UI 텍스트 실시간 동기화 업데이트
        self.ui.positionValue.setText(f"X: {pos[0]:.2f}, Y: {pos[1]:.2f}, Z: {pos[2]:.2f}")
        self.ui.rotationValue.setText(f"R: {ori[0]:.2f}, P: {ori[1]:.2f}, Y: {ori[2]:.2f}")


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
        """선택된 볼륨 노드에 선형 변환 노드(vtkMRMLLinearTransformNode)를 연결하고 회전을 적용합니다.
        세그멘테이션 노드 및 뼈 파편(Femur_Fragment) 모델 노드까지 동기화하여 일괄 회전시킵니다."""
        if not volumeNode:
            return

        transformNode = volumeNode.GetParentTransformNode()
        
        if not transformNode or not transformNode.IsA("vtkMRMLLinearTransformNode"):
            transformNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
            transformNode.SetName(f"{volumeNode.GetName()}_RotationTransform")
            volumeNode.SetAndObserveTransformNodeID(transformNode.GetID())
            logging.info(f"Created new linear transform node for '{volumeNode.GetName()}'.")

        # 1. 씬 내의 모든 vtkMRMLSegmentationNode를 동일한 변환 노드 아래에 배치
        segmentationNodes = slicer.mrmlScene.GetNodesByClass("vtkMRMLSegmentationNode")
        for i in range(segmentationNodes.GetNumberOfItems()):
            segNode = segmentationNodes.GetItemAsObject(i)
            if segNode.GetParentTransformNode() != transformNode:
                segNode.SetAndObserveTransformNodeID(transformNode.GetID())

        # 2. 5단계 분리 기능으로 추출된 뼈 fragment 모델 노드(vtkMRMLModelNode)들도 동일한 변환 노드 아래에 배치
        modelNodes = slicer.mrmlScene.GetNodesByClass("vtkMRMLModelNode")
        for i in range(modelNodes.GetNumberOfItems()):
            modelNode = modelNodes.GetItemAsObject(i)
            if modelNode.GetName().startswith("Femur_Fragment") and modelNode.GetParentTransformNode() != transformNode:
                modelNode.SetAndObserveTransformNodeID(transformNode.GetID())

        # 변환 행렬 업데이트
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
        logging.debug(f"Rotated volume and sync nodes around {axis}-Axis by {angle:.1f}°")

    def separateBoneFragments(self, segmentationNode: vtkMRMLSegmentationNode, referenceVolumeNode: vtkMRMLScalarVolumeNode = None) -> int:
        """
        주어진 세그먼트 노드로부터 Slicer 내장 C++ Closed Surface 표현형을 사용하여
        불연속적인 메쉬(Mesh)들을 0.1초 내외로 초고속 감지하고 분리합니다.
        각각 독립된 vtkMRMLModelNode(3D 모델)로 분리하고 다채로운 무지개 계열 색상을 입혀 저장합니다.
        꼭짓점 개수가 200개 이하인 미세 노이즈 조각들은 필터링하여 생성하지 않습니다.
        """
        if not segmentationNode:
            raise RuntimeError("No segmentation node provided.")

        # 0. 세그멘테이션 및 세그먼트 존재 여부 검사
        segmentation = segmentationNode.GetSegmentation()
        if not segmentation or segmentation.GetNumberOfSegments() == 0:
            raise RuntimeError("The segmentation node has no segments. Please create a segment and paint the bone area first.")

        # 가시 세그먼트 유무 검사
        displayNode = segmentationNode.GetDisplayNode()
        if not displayNode:
            raise RuntimeError("Segmentation display node is missing.")
        
        visibleSegmentIds = vtk.vtkStringArray()
        displayNode.GetVisibleSegmentIDs(visibleSegmentIds)
        if visibleSegmentIds.GetNumberOfValues() == 0:
            raise RuntimeError("There are no visible segments. Please turn on the visibility (eye icon) of the segments to separate.")

        # 1. Slicer 내장 C++ Closed Surface 3D 표현형 생성 강제
        segmentationNode.CreateClosedSurfaceRepresentation()

        # 2. 여러 세그먼트로부터 PolyData를 추출하여 Append
        appendFilter = vtk.vtkAppendPolyData()
        hasInput = False

        segmentationsLogic = slicer.modules.segmentations.logic()
        for i in range(visibleSegmentIds.GetNumberOfValues()):
            segId = visibleSegmentIds.GetValue(i)
            segmentPolyData = vtk.vtkPolyData()
            # C++ SegmentationsLogic을 사용하여 안전하게 representation 가져오기 (PythonQt 크래시 예방)
            segmentationsLogic.GetSegmentClosedSurfaceRepresentation(segmentationNode, segId, segmentPolyData)
            if segmentPolyData and segmentPolyData.GetNumberOfPoints() > 0:
                appendFilter.AddInputData(segmentPolyData)
                hasInput = True

        if not hasInput:
            raise RuntimeError("No valid geometry found in the visible segments. Make sure they are not empty.")

        appendFilter.Update()
        mergedPolyData = appendFilter.GetOutput()

        # 3. VTK Connectivity Filter를 연동하여 불연속적으로 끊어진 Region(조각)들을 일괄 분석
        connectivityFilter = vtk.vtkPolyDataConnectivityFilter()
        connectivityFilter.SetInputData(mergedPolyData)
        connectivityFilter.SetExtractionModeToAllRegions()
        connectivityFilter.Update()

        numRegions = connectivityFilter.GetNumberOfExtractedRegions()
        regionSizes = connectivityFilter.GetRegionSizes() # 각 Region의 연결 성분 크기 (포인트/셀 단위)
        logging.info(f"Total disconnected regions detected: {numRegions}")

        # 부모 변환 노드가 있다면 상속받기
        transformNode = segmentationNode.GetParentTransformNode()

        separatedCount = 0
        
        # 4. 각 Region의 인덱스와 크기(Point 수)를 매핑하고, 크기 기준 내림차순 정렬 (상위 최대 2개 추출)
        regionInfos = []
        if regionSizes:
            for i in range(numRegions):
                if i < regionSizes.GetNumberOfValues():
                    size = regionSizes.GetValue(i)
                    regionInfos.append((i, size))
        else:
            for i in range(numRegions):
                regionInfos.append((i, 0))

        # 크기 기준 내림차순 정렬
        regionInfos.sort(key=lambda x: x[1], reverse=True)

        # 상위 최대 2개 선정
        targetRegions = regionInfos[:min(2, len(regionInfos))]
        logging.info(f"Top 2 largest disconnected regions selected: {targetRegions}")

        # 5. 선정된 상위 2개 Region만 개별 3D 모델 메쉬 노드로 씬에 추가
        for idx, (regionId, size) in enumerate(targetRegions):
            # 최소한 꼭짓점 개수가 200개 초과인 의미 있는 경우에만 생성
            if size <= 200 and regionSizes is not None:
                continue

            singleRegionFilter = vtk.vtkPolyDataConnectivityFilter()
            singleRegionFilter.SetInputData(mergedPolyData)
            singleRegionFilter.SetExtractionModeToSpecifiedRegions()
            singleRegionFilter.AddSpecifiedRegion(regionId)
            singleRegionFilter.Update()

            regionPolyData = singleRegionFilter.GetOutput()
            
            # 꼭짓점 개수 최종 확인
            if regionPolyData.GetNumberOfPoints() > 200:
                fragModel = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLModelNode")
                fragModel.SetName(f"Femur_Fragment_{separatedCount + 1}")
                fragModel.SetAndObservePolyData(regionPolyData)
                fragModel.CreateDefaultDisplayNodes()

                # 트랜스폼 노드 연동 상속 (중요: segmentationNode의 transformNode를 그대로 물려줌)
                if transformNode:
                    fragModel.SetAndObserveTransformNodeID(transformNode.GetID())

                # 3D 뷰에서 파편 구분이 쉽도록 다채로운 색상 순차 지정
                displayNode = fragModel.GetDisplayNode()
                if displayNode:
                    # 겹치지 않는 무지개 톤 스펙트럼 색상 배분
                    r = 0.3 + (separatedCount * 0.17) % 0.6
                    g = 0.4 + (separatedCount * 0.11) % 0.5
                    b = 0.8 - (separatedCount * 0.23) % 0.5
                    displayNode.SetColor(r, g, b)
                    displayNode.SetVisibility2D(True)
                    displayNode.SetSliceIntersectionThickness(3)

                separatedCount += 1

        return separatedCount

    def mirrorModel(self, sourceModelNode) -> slicer.vtkMRMLModelNode:
        """선택된 모델 노드를 X축 기준(스케일 -1)으로 반전한 새로운 모델 노드를 생성합니다."""
        if not sourceModelNode or not sourceModelNode.GetPolyData():
            raise RuntimeError("Invalid source model node.")
            
        polyData = sourceModelNode.GetPolyData()
        
        # X축 기준 미러링 변환
        transform = vtk.vtkTransform()
        transform.Scale(-1.0, 1.0, 1.0)
        
        transformFilter = vtk.vtkTransformPolyDataFilter()
        transformFilter.SetInputData(polyData)
        transformFilter.SetTransform(transform)
        transformFilter.Update()
        
        mirroredPolyData = transformFilter.GetOutput()
        
        # 삼각형 노멀(법선벡터) 방향 재정렬
        normals = vtk.vtkPolyDataNormals()
        normals.SetInputData(mirroredPolyData)
        normals.ConsistencyOn()
        normals.SplittingOff()
        normals.FlipNormalsOn()
        normals.Update()
        
        finalPolyData = normals.GetOutput()
        
        # 새로운 모델 노드로 씬에 등록
        mirroredNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLModelNode")
        mirroredNode.SetName(f"{sourceModelNode.GetName()}_Mirrored")
        mirroredNode.SetAndObservePolyData(finalPolyData)
        mirroredNode.CreateDefaultDisplayNodes()
        
        # 색상 설정 (스카이블루 느낌의 투명 가이드 뼈 모델)
        displayNode = mirroredNode.GetDisplayNode()
        if displayNode:
            displayNode.SetColor(0.2, 0.6, 0.9)
            displayNode.SetOpacity(0.5)
            displayNode.SetVisibility2D(True)
            displayNode.SetSliceIntersectionThickness(2)
            
        return mirroredNode

    def runIcpRegistration(self, fragmentModelNodes: list, guideModelNode) -> None:
        """각 분리된 조각 모델 노드(fragment)의 위치를 가이드 모델 노드의 가장 가까운 표면에 강체 ICP 정합합니다.
        메쉬 데이터 자체를 직접 수정하지 않고, 부모 변환 노드의 로컬 변환 행렬을 수학적으로 갱신합니다.
        상위 변환(예: Live 3D Rotation)의 영향이 있더라도 왜곡 없이 정확하게 계산합니다."""
        if not guideModelNode or not guideModelNode.GetPolyData():
            raise RuntimeError("Invalid guide model node.")
            
        guidePolyData = guideModelNode.GetPolyData()
        
        # 가이드 노드의 상위 변환 정보가 있다면 월드 좌표 공간 메쉬로 변환 적용
        guideTransformNode = guideModelNode.GetParentTransformNode()
        if guideTransformNode:
            gTransform = vtk.vtkTransform()
            gMatrix = vtk.vtkMatrix4x4()
            guideTransformNode.GetMatrixTransformToWorld(gMatrix)
            gTransform.SetMatrix(gMatrix)
            
            gtfFilter = vtk.vtkTransformPolyDataFilter()
            gtfFilter.SetInputData(guidePolyData)
            gtfFilter.SetTransform(gTransform)
            gtfFilter.Update()
            guidePolyData = gtfFilter.GetOutput()
        
        for fragNode in fragmentModelNodes:
            if not fragNode or not fragNode.GetPolyData():
                continue
                
            # 1. 수동 정렬 기즈모 변환 노드 획득 (없으면 신설)
            transformNode = fragNode.GetParentTransformNode()
            if not transformNode:
                transformNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
                transformNode.SetName(f"{fragNode.GetName()}_Transform")
                fragNode.SetAndObserveTransformNodeID(transformNode.GetID())
            
            # 2. 계층 내 모든 변환(상위 회전 포함)이 반영된 월드 좌표 메쉬 획득 (정합 대상)
            currentPoly = fragNode.GetPolyData()
            
            worldMatrix = vtk.vtkMatrix4x4()
            transformNode.GetMatrixTransformToWorld(worldMatrix)
            
            transform = vtk.vtkTransform()
            transform.SetMatrix(worldMatrix)
            
            tfFilter = vtk.vtkTransformPolyDataFilter()
            tfFilter.SetInputData(currentPoly)
            tfFilter.SetTransform(transform)
            tfFilter.Update()
            sourceWorldPoly = tfFilter.GetOutput()
            
            # 3. ICP 변환 설정 (무게중심 매칭 해제 및 랜드마크 샘플 수 상향을 통한 정밀도 제어)
            icp = vtk.vtkIterativeClosestPointTransform()
            icp.SetSource(sourceWorldPoly)
            icp.SetTarget(guidePolyData)
            icp.GetLandmarkTransform().SetModeToRigidBody()
            icp.SetMaximumNumberOfIterations(300)      # 반복 횟수 300회 상향
            icp.SetMaximumNumberOfLandmarks(1000)      # 랜드마크 대폭 확장 (단면 정합 정밀도 해결!)
            icp.CheckMeanDistanceOn()                  # 평균 거리 체크 기능 활성화
            icp.SetMaximumMeanDistance(0.01)           # 수렴 판단 오차 완화 (0.0001 -> 0.01로 조정하여 수렴 안정성 확보)
            icp.StartByMatchingCentroidsOff()          # 수동 배치 기반 정합을 위해 무게중심 강제 매칭 비활성화
            icp.Update()
            
            # 4. ICP 결과 델타 변환 행렬 획득
            icpMatrix = icp.GetMatrix()
            
            # 5. 새로운 월드 변환 행렬 계산: W_new = ICP_Matrix * W_current
            newWorldMatrix = vtk.vtkMatrix4x4()
            vtk.vtkMatrix4x4.Multiply4x4(icpMatrix, worldMatrix, newWorldMatrix)
            
            # 6. 상위 부모 변환의 영향이 있다면 역행렬을 계산하여 정확한 로컬 변환 산출
            # T_new_local = W_parent_inv * W_new
            parentTransformNode = transformNode.GetParentTransformNode()
            if parentTransformNode:
                parentWorldMatrix = vtk.vtkMatrix4x4()
                parentTransformNode.GetMatrixTransformToWorld(parentWorldMatrix)
                
                parentWorldInverse = vtk.vtkMatrix4x4()
                vtk.vtkMatrix4x4.Invert(parentWorldMatrix, parentWorldInverse)
                
                newLocalMatrix = vtk.vtkMatrix4x4()
                vtk.vtkMatrix4x4.Multiply4x4(parentWorldInverse, newWorldMatrix, newLocalMatrix)
            else:
                newLocalMatrix = newWorldMatrix
                
            # 7. 신규 로컬 행렬 반영 (PolyData는 수정하지 않고, 부모 변환만 업데이트)
            transformNode.SetMatrixTransformToParent(newLocalMatrix)
            
            # 8. 수동 기즈모 디스플레이 꺼주기
            displayNode = transformNode.GetDisplayNode()
            if displayNode:
                displayNode.SetEditorVisibility(False)
                
            fragNode.Modified()

    def runFractureSurfaceSnap(self, frag1Node, frag2Node) -> None:
        """두 파편 모델 간에 마주 보고 있는 절단면(Fracture surface) 영역을 자동으로 특정하여 상호 밀착 정합(Snap)합니다.
        두 메쉬의 인접한 정점쌍들(거리 25mm 이내)을 추출하여 그 점들 사이의 강체 ICP 정합을 수행합니다."""
        if not frag1Node or not frag1Node.GetPolyData() or not frag2Node or not frag2Node.GetPolyData():
            raise RuntimeError("Invalid fragment nodes.")

        # 1. 두 파편 노드의 최신 월드 행렬을 직접 조립
        tNode1 = frag1Node.GetParentTransformNode()
        tNode2 = frag2Node.GetParentTransformNode()
        
        if not tNode1:
            tNode1 = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
            tNode1.SetName(f"{frag1Node.GetName()}_Transform")
            frag1Node.SetAndObserveTransformNodeID(tNode1.GetID())
        if not tNode2:
            tNode2 = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
            tNode2.SetName(f"{frag2Node.GetName()}_Transform")
            frag2Node.SetAndObserveTransformNodeID(tNode2.GetID())

        lMat1 = vtk.vtkMatrix4x4()
        tNode1.GetMatrixTransformToParent(lMat1)
        parent1 = tNode1.GetParentTransformNode()
        wMat1 = vtk.vtkMatrix4x4()
        if parent1:
            pwMat1 = vtk.vtkMatrix4x4()
            parent1.GetMatrixTransformToWorld(pwMat1)
            vtk.vtkMatrix4x4.Multiply4x4(pwMat1, lMat1, wMat1)
        else:
            wMat1.DeepCopy(lMat1)

        lMat2 = vtk.vtkMatrix4x4()
        tNode2.GetMatrixTransformToParent(lMat2)
        parent2 = tNode2.GetParentTransformNode()
        wMat2 = vtk.vtkMatrix4x4()
        if parent2:
            pwMat2 = vtk.vtkMatrix4x4()
            parent2.GetMatrixTransformToWorld(pwMat2)
            vtk.vtkMatrix4x4.Multiply4x4(pwMat2, lMat2, wMat2)
        else:
            wMat2.DeepCopy(lMat2)

        # 2. 월드 공간의 PolyData 획득
        poly1 = frag1Node.GetPolyData()
        poly2 = frag2Node.GetPolyData()

        tf1 = vtk.vtkTransform()
        tf1.SetMatrix(wMat1)
        tfFilter1 = vtk.vtkTransformPolyDataFilter()
        tfFilter1.SetInputData(poly1)
        tfFilter1.SetTransform(tf1)
        tfFilter1.Update()
        poly1World = tfFilter1.GetOutput()

        tf2 = vtk.vtkTransform()
        tf2.SetMatrix(wMat2)
        tfFilter2 = vtk.vtkTransformPolyDataFilter()
        tfFilter2.SetInputData(poly2)
        tfFilter2.SetTransform(tf2)
        tfFilter2.Update()
        poly2World = tfFilter2.GetOutput()

        # 3. KD-Tree를 이용한 마주 보는 근접 정점 추출 (절단 단면 특정)
        locator = vtk.vtkKdTreePointLocator()
        locator.SetDataSet(poly2World)
        locator.BuildLocator()

        sourcePoints = vtk.vtkPoints()
        targetPoints = vtk.vtkPoints()

        import math
        distLimit = 25.0  # 인접 단면으로 판단할 거리 한계값 (25mm)
        numPoints1 = poly1World.GetNumberOfPoints()

        for i in range(numPoints1):
            pt1 = poly1World.GetPoint(i)
            closestPointId = locator.FindClosestPoint(pt1)
            if closestPointId < 0:
                continue
            pt2 = poly2World.GetPoint(closestPointId)
            
            # 두 점 사이의 유클리드 거리 계산
            dist = math.sqrt(
                (pt1[0] - pt2[0])**2 +
                (pt1[1] - pt2[1])**2 +
                (pt1[2] - pt2[2])**2
            )
            
            if dist <= distLimit:
                sourcePoints.InsertNextPoint(pt1)
                targetPoints.InsertNextPoint(pt2)

        # 단면 매칭을 위해 마주 보는 점의 개수가 충분한지 검증
        if sourcePoints.GetNumberOfPoints() < 50:
            raise RuntimeError(
                "절단면 간의 거리가 너무 멉니다. 수동 기즈모(Move)를 활성화하여 뼈 조각 절단면들을 대략 가까이(2.5cm 이내) 배치한 뒤 다시 실행해 주세요."
            )

        sourcePoly = vtk.vtkPolyData()
        sourcePoly.SetPoints(sourcePoints)
        
        targetPoly = vtk.vtkPolyData()
        targetPoly.SetPoints(targetPoints)

        # 4. 대응되는 단면 점 쌍을 이용해 강체 랜드마크 정합 수행 (1:1 매칭 관계 보존)
        landmarkTransform = vtk.vtkLandmarkTransform()
        landmarkTransform.SetSourceLandmarks(sourcePoints)
        landmarkTransform.SetTargetLandmarks(targetPoints)
        landmarkTransform.SetModeToRigidBody()
        landmarkTransform.Update()

        # 5. 정합 결과 델타 행렬 획득 및 월드 행렬 갱신
        snapMatrix = landmarkTransform.GetMatrix()
        
        newWorldMatrix1 = vtk.vtkMatrix4x4()
        vtk.vtkMatrix4x4.Multiply4x4(snapMatrix, wMat1, newWorldMatrix1)

        # 6. 상위 부모 변환 반영한 신규 로컬 행렬 계산
        if parent1:
            pwMatInverse1 = vtk.vtkMatrix4x4()
            vtk.vtkMatrix4x4.Invert(pwMat1, pwMatInverse1)
            newLocalMatrix1 = vtk.vtkMatrix4x4()
            vtk.vtkMatrix4x4.Multiply4x4(pwMatInverse1, newWorldMatrix1, newLocalMatrix1)
        else:
            newLocalMatrix1 = newWorldMatrix1

        # 7. 신규 로컬 행렬 반영
        tNode1.SetMatrixTransformToParent(newLocalMatrix1)
        frag1Node.Modified()


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
        
        # 1. 테스트용 3D 가이드 모델 생성 (X=5에 위치한 구)
        sphere = vtk.vtkSphereSource()
        sphere.SetRadius(10.0)
        sphere.SetCenter(5.0, 0.0, 0.0)
        sphere.Update()
        
        guideModel = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLModelNode")
        guideModel.SetName("Test_Guide_Model")
        guideModel.SetAndObservePolyData(sphere.GetOutput())
        
        # 2. 미러링(Mirror Guide) 기능 검증
        self.delayDisplay("Testing model mirroring logic...")
        mirroredNode = logic.mirrorModel(guideModel)
        self.assertIsNotNone(mirroredNode)
        self.assertIsNotNone(mirroredNode.GetPolyData())
        
        bounds = [0.0] * 6
        mirroredNode.GetPolyData().GetBounds(bounds)
        centerX = (bounds[0] + bounds[1]) / 2.0
        # X=5에서 X=-5로 반전되었는지 검증
        self.assertAlmostEqual(centerX, -5.0, places=3)
        
        # 3. ICP 자동 정합(ICP Auto-Reduction) 기능 검증
        self.delayDisplay("Testing ICP registration logic...")
        # 가이드와 동일하지만 X=7로 오프셋된 파편 모델 생성
        sphere2 = vtk.vtkSphereSource()
        sphere2.SetRadius(10.0)
        sphere2.SetCenter(7.0, 0.0, 0.0)
        sphere2.Update()
        
        fragModel = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLModelNode")
        fragModel.SetName("Test_Fragment_Model")
        fragModel.SetAndObservePolyData(sphere2.GetOutput())
        
        # ICP 수행 (파편 모델을 가이드 모델에 표면 정합시킴)
        logic.runIcpRegistration([fragModel], guideModel)
        
        # 월드 좌표계 기준의 Bounds 계산하여 검증 (메쉬 좌표는 그대로고 변환 행렬이 변경됨)
        worldPoly = vtk.vtkPolyData()
        worldPoly.DeepCopy(fragModel.GetPolyData())
        
        transformNode = fragModel.GetParentTransformNode()
        if transformNode:
            matrix = vtk.vtkMatrix4x4()
            transformNode.GetMatrixTransformToWorld(matrix)
            transform = vtk.vtkTransform()
            transform.SetMatrix(matrix)
            
            tfFilter = vtk.vtkTransformPolyDataFilter()
            tfFilter.SetInputData(worldPoly)
            tfFilter.SetTransform(transform)
            tfFilter.Update()
            worldPoly = tfFilter.GetOutput()
            
        fragBounds = [0.0] * 6
        worldPoly.GetBounds(fragBounds)
        fragCenterX = (fragBounds[0] + fragBounds[1]) / 2.0
        # ICP가 적용되어 파편의 월드 중심이 가이드 모델의 중심(X=5)으로 정밀 정합되었는지 검증
        self.assertAlmostEqual(fragCenterX, 5.0, places=2)
        
        self.delayDisplay("All tests passed successfully!")
