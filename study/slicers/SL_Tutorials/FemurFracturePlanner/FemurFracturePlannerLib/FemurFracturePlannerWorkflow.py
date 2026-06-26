"""
File Name: FemurFracturePlannerWorkflow.py
Version: v0-2.1.2
Date: 2026-06-26
Description: 대퇴골 골절 계획 모듈의 입력, 렌더링, 회전, 세그멘테이션, 가이드 모델 작업 흐름을 담당한다.

Version History:
- v0-2.1.2 (2026-06-26)
  - runFractureSurfaceSnap 실행 시 메모리에 로드된 로직 버전 및 각 골편의 부모 변환 노드 ID를 상세 보고하는 디버그 로그 보강 (Z 증가)
- v0-2.1.1 (2026-06-26)
  - onRunSurfaceSnapClicked 내에서 이름으로 노드 획득 시 동일 노드가 이중 할당되어 오동작하던 버그를 ID 중복 검증 로직으로 해결 (Z 증가)
- v0-2.1.0 (2026-06-26)
  - 실시간 정합 진행 상태 출력을 위한 addLogMessage 메서드 추가
  - onRunIcpReductionClicked 및 onRunSurfaceSnapClicked 핸들러에 로그 연동(logCallback) 구현 적용 (Y 증가, Z 0 초기화)
- v0-2.0.1 (2026-06-25)
  - 골편 분리 성공 시 volumeRendering 토글 여부와 상관없이 무조건 목록 테이블이 갱신되도록 분기문 논리 오류 수정 (Z 증가)
- v0-2.0.0 (2026-06-24)
  - 뼈 임계치(Threshold) 미만을 마스킹한 후 3D 에지를 검출하는 onMaskedEdgeDetectionClicked 핸들러 추가 (X 증가, Y/Z 초기화)
- v0-1.1.0 (2026-06-24)
  - 입력 볼륨 이벤트 핸들러, 3D 렌더링 토글, 3D 회전 제어 및 DICOM 로딩 액션, 팝업 창 리사이징 타이머 모니터링 이벤트 등에 세부 흐름 주석 보강 (Y 증가)
- v0-1.0.1 (2026-06-24)
  - Threshold 적용 시 R/A/S 범위가 좁아지는 버그 수정: 세그멘테이션-볼륨 연결 시 SetReferenceImageGeometryParameterFromVolumeNode 호출 추가
- v0-1.0.0 (2026-06-23)
  - 영상처리 이벤트 핸들러(onInvertIntensityClicked, onEdgeDetectionClicked) 복원 (X 증가, Y/Z 초기화)
- v0-0.0.0 (2026-06-23)
  - FemurFracturePlannerWidget.py에서 입력/렌더링/회전/가이드 모델 관련 메서드를 분리하여 최초 작성
"""

import logging

import qt
import slicer
import vtk

from slicer.i18n import tr as _

# Workflow mixin은 사용자의 버튼 클릭과 선택 변경을 처리한다.
# 여기서는 "UI에서 무엇을 눌렀는지"를 해석하고, 실제 계산은 self.logic에 위임한다.


class FemurFracturePlannerWorkflowMixin:
    """
    사용자 작업 흐름을 담당하는 mixin이다.

    이 파일은 입력 볼륨 선택부터 3D 렌더링, 회전, 골편 분리,
    가이드 모델 로드와 자동 정렬까지의 동작을 한곳에 모아 둔다.
    """

    def onInputVolumeChanged(self, node) -> None:
        """
        입력 CT 볼륨이 바뀌면 Segment Editor와 뷰어 상태를 함께 갱신한다.

        [입력 볼륨 변경 시 동기화 파이프라인]
        1. 내장 Segment Editor 위젯이 유효한지 확인한다.
        2. 볼륨 노드가 새롭게 선택된 경우:
           - Slicer 씬에 Segment Editor용 MRML 노드가 없으면 새로 만들어 위젯에 바인딩한다.
           - 활성화된 세그멘테이션 노드가 없으면 첫 번째 노드를 재사용하거나 신설한다.
           - 세그멘테이션 노드의 geometry(원점, 복셀 크기, 영역 범위)를 입력 볼륨 노드의 물리적 좌표 정보와 
             동기화(SetReferenceImageGeometryParameterFromVolumeNode)한다. (RAS 범위 누락 버그 방지 핵심 로직)
        3. 2D 슬라이스 뷰어(배경 레이어) 및 3D Volume Rendering 가시성을 사용자가 켜 둔 상태라면,
           변경된 볼륨 노드에 맞춰 화면 렌더링을 즉시 동기화 업데이트한다.
        """
        if not self.segmentEditorWidget:
            return

        if node:
            if not self.segmentEditorNode:
                # Segment Editor는 편집 상태를 저장할 전용 MRML 노드가 필요하다.
                # 없으면 새로 만들어 현재 위젯에 연결한다.
                self.segmentEditorNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLSegmentEditorNode")
                self.segmentEditorNode.SetName("FemurFracturePlanner_SegmentEditorNode")
            self.segmentEditorWidget.setMRMLSegmentEditorNode(self.segmentEditorNode)

            segNode = self.ui.activeSegmentationSelector.currentNode()
            if not segNode:
                # segmentation이 아직 없으면 씬에서 첫 번째 것을 찾고,
                # 그것도 없으면 사용자가 바로 칠할 수 있도록 새 segmentation을 만든다.
                segNode = slicer.mrmlScene.GetFirstNodeByClass("vtkMRMLSegmentationNode")
                if not segNode:
                    segNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLSegmentationNode")
                    segNode.SetName("Femur_Segmentation")
                    segNode.CreateDefaultDisplayNodes()
                self.ui.activeSegmentationSelector.setCurrentNode(segNode)
            self.segmentEditorWidget.setSegmentationNode(segNode)
            self.segmentEditorWidget.setSourceVolumeNode(node)

            # 세그멘테이션의 내부 labelmap 기하학(해상도, 원점, 범위)을 소스 볼륨과 일치시킨다.
            # 이 호출이 없으면 Threshold 효과가 볼륨 전체가 아닌 극히 좁은 기본 범위에만 적용된다.
            segNode.SetReferenceImageGeometryParameterFromVolumeNode(node)
        else:
            self.segmentEditorWidget.setSourceVolumeNode(None)

        if self.ui.volumeVisibilityButton.checked:
            # slice viewer의 background layer에 CT 볼륨을 넣으면 2D 단면 뷰에 표시된다.
            slicer.util.setSliceViewerLayers(background=node)
        else:
            slicer.util.setSliceViewerLayers(background=None)

        if self.ui.volumeRenderingVisibilityButton.checked and node:
            # 입력 볼륨이 바뀌어도 3D 렌더링 토글 상태는 유지하고 새 볼륨으로 갱신한다.
            self.logic.updateVolumeRendering(node, self.ui.thresholdSlider.value)

        # 입력 볼륨이 바뀌면 회전 기준도 초기 상태부터 다시 시작하는 편이 안전하다.
        self.ui.rotationAngleSlider.value = 0.0

    def onVolumeVisibilityToggled(self, checked: bool) -> None:
        """선택한 CT 볼륨을 2D Slice Viewer(Red/Yellow/Green) 배경 레이어에 노출하거나 숨긴다."""
        inputNode = self.ui.inputVolumeSelector.currentNode()
        if not inputNode:
            if checked:
                self.ui.volumeVisibilityButton.checked = False
                slicer.util.errorDisplay(_("먼저 입력 CT 볼륨을 불러오거나 선택하세요."))
            return

        if checked:
            # background를 지정하면 Red/Yellow/Green slice viewer에 CT가 보인다.
            slicer.util.setSliceViewerLayers(background=inputNode)
            self.ui.volumeVisibilityButton.text = _("숨기기")
            self.ui.volumeVisibilityButton.toolTip = _("뷰어에서 볼륨을 숨깁니다.")
            logging.info(f"Showing volume '{inputNode.GetName()}' in slice viewers.")
        else:
            slicer.util.setSliceViewerLayers(background=None)
            self.ui.volumeVisibilityButton.text = _("보기")
            self.ui.volumeVisibilityButton.toolTip = _("뷰어에서 볼륨을 표시합니다.")
            logging.info("Hiding volume in slice viewers.")

    def onVolumeRenderingToggled(self, checked: bool) -> None:
        """
        선택한 CT 볼륨의 실시간 3D Volume Rendering 가시성을 토글한다.

        [3D 렌더링 가시성 제어 흐름]
        1. 3D 렌더링 대상이 될 볼륨 노드 선택 상태를 체크한다.
        2. checked가 True이면, Logic 계층의 showVolumeRendering을 호출해
           CT-Bone 프리셋과 현재 thresholdSlider의 HU 기반 불투명도 맵을 3D 뷰어에 렌더링한다.
        3. checked가 False이면, Logic 계층의 hideVolumeRendering을 호출해 볼륨 렌더링 노드를 정리하고 화면에서 숨긴다.
        """
        inputNode = self.ui.inputVolumeSelector.currentNode()
        if not inputNode:
            if checked:
                self.ui.volumeRenderingVisibilityButton.checked = False
                slicer.util.errorDisplay(_("먼저 입력 CT 볼륨을 불러오거나 선택하세요."))
            return

        if checked:
            logging.info(f"Enabling real-time 3D volume rendering for: {inputNode.GetName()}")
            # thresholdSlider 값은 HU 기준 불투명도 조절에 사용된다.
            self.logic.showVolumeRendering(inputNode, self.ui.thresholdSlider.value)
            self.ui.volumeRenderingVisibilityButton.text = _("숨기기")
            self.ui.volumeRenderingVisibilityButton.toolTip = _("3D 볼륨 렌더링을 숨깁니다.")
        else:
            logging.info("Disabling volume rendering.")
            self.logic.hideVolumeRendering(inputNode)
            self.ui.volumeRenderingVisibilityButton.text = _("보기")
            self.ui.volumeRenderingVisibilityButton.toolTip = _("3D 볼륨 렌더링을 표시합니다.")

    def onThresholdSliderChanged(self, value: float) -> None:
        """
        사용자가 임계값 슬라이더를 변경하면, 실시간으로 3D 볼륨 렌더링의 불투명도를 조정한다.
        
        불투명도 조절 곡선이 갱신되어, 설정된 HU 미만의 살/연부 조직은 투명해지고 뼈 구조만 선명해진다.
        """
        inputNode = self.ui.inputVolumeSelector.currentNode()
        if not inputNode or not self.ui.volumeRenderingVisibilityButton.checked:
            return
        # 렌더링이 켜져 있을 때만 전달 함수를 갱신한다.
        # 꺼진 상태에서 계산하면 사용자가 보지 않는 렌더링 노드를 불필요하게 만들 수 있다.
        self.logic.adjustVolumeRenderingThreshold(inputNode, value)

    def onLoadVolumeClicked(self) -> None:
        """외부 파일 탐색기를 팝업 형태로 열어 CT 볼륨 파일(*.nrrd, *.nii 등)을 씬으로 다이렉트 로드한다."""
        filePath = qt.QFileDialog.getOpenFileName(
            slicer.util.mainWindow(),
            _("CT 볼륨 파일 선택"),
            "",
            "볼륨 파일 (*.nrrd *.nii *.nii.gz *.mha *.mhd *.dcm)",
        )

        if not filePath:
            # 사용자가 파일 선택 창에서 취소를 누르면 아무 작업도 하지 않는다.
            return

        logging.info(f"Direct volume loading triggered for: '{filePath}'")
        with slicer.util.tryWithErrorDisplay(_("CT 볼륨 파일을 불러오지 못했습니다."), waitCursor=True):
            loadedNode = slicer.util.loadVolume(filePath)
            if loadedNode:
                self.ui.inputVolumeSelector.setCurrentNode(loadedNode)
                self.ui.volumeVisibilityButton.checked = True
                slicer.util.infoDisplay(_(f"CT 볼륨을 불러왔습니다: '{loadedNode.GetName()}'"))

    def onLoadDicomClicked(self) -> None:
        """
        [DICOM 데이터베이스 브라우저 팝업 로딩 액션]
        1. Slicer 내장의 DICOM 브라우저 창 유형을 'Popup' 형태로 기본 설정한다.
        2. 메인 윈도우에 등록된 기본 'ActionOpenDICOMBrowser' 액션 포인터를 직접 검색하여 트리거한다.
        3. 만약 Slicer 버전 차이나 커스텀 빌드 문제로 액션을 찾을 수 없는 경우,
           DICOM 관리 모듈("DICOM")로 다이렉트 전환시키는 방식으로 안정적인 예외 복구를 수행한다.
        """
        logging.info("Opening Slicer's DICOM database browser in popup...")
        try:
            slicer.app.settings().setValue("DICOM/BrowserWindowType", "Popup")

            mainWindow = slicer.util.mainWindow()
            if mainWindow:
                # Slicer 메인 윈도우에 이미 등록된 DICOM 열기 액션을 찾아 직접 실행한다.
                dicomAction = mainWindow.findChild("QAction", "ActionOpenDICOMBrowser")
                if dicomAction:
                    dicomAction.trigger()
                    return

            slicer.util.selectModule("DICOM")
        except Exception as error:
            slicer.util.errorDisplay(_(f"DICOM 브라우저를 열지 못했습니다: {error}"))

    def onInvertIntensityClicked(self) -> None:
        """1.5단계: Numpy 기반 강도 반전 필터를 실행하여 흑백 밝기가 뒤집힌 새 볼륨을 생성하고 선택을 자동 변경한다."""
        inputNode = self.ui.inputVolumeSelector.currentNode()
        if not inputNode:
            slicer.util.errorDisplay(_("먼저 입력 CT 볼륨을 불러오거나 선택하세요."))
            return

        logging.info(f"Inverting volume intensity for: '{inputNode.GetName()}'")
        with slicer.util.tryWithErrorDisplay(_("볼륨 강도 반전에 실패했습니다."), waitCursor=True):
            invertedNode = self.logic.invertVolumeIntensity(inputNode)
            if invertedNode:
                # 새로 만든 반전 볼륨을 입력 selector에 넣어 이후 작업이 그 볼륨을 기준으로 이어지게 한다.
                self.ui.inputVolumeSelector.setCurrentNode(invertedNode)
                slicer.util.infoDisplay(_(f"볼륨을 성공적으로 반전했습니다: '{invertedNode.GetName()}'"))

    def onEdgeDetectionClicked(self) -> None:
        """1.5단계: VTK Gradient Magnitude 필터를 실행하여 3D 외곽 테두리가 검출된 새 볼륨을 생성하고 자동 선택한다."""
        inputNode = self.ui.inputVolumeSelector.currentNode()
        if not inputNode:
            slicer.util.errorDisplay(_("먼저 입력 CT 볼륨을 불러오거나 선택하세요."))
            return

        logging.info(f"Detecting 3D volume edges for: '{inputNode.GetName()}'")
        with slicer.util.tryWithErrorDisplay(_("볼륨 엣지 검출에 실패했습니다."), waitCursor=True):
            edgeNode = self.logic.detectVolumeEdges(inputNode)
            if edgeNode:
                # 엣지 검출 결과도 새로운 볼륨 노드이므로 현재 입력으로 교체한다.
                self.ui.inputVolumeSelector.setCurrentNode(edgeNode)
                slicer.util.infoDisplay(_(f"엣지 볼륨을 성공적으로 생성했습니다: '{edgeNode.GetName()}'"))

    def onMaskedEdgeDetectionClicked(self) -> None:
        """1.5단계: 사용자가 지정한 뼈 임계치를 기준으로 마스킹을 선행한 뒤 3D 에지를 검출하는 테스트 기능을 실행한다."""
        inputNode = self.ui.inputVolumeSelector.currentNode()
        if not inputNode:
            slicer.util.errorDisplay(_("먼저 입력 CT 볼륨을 불러오거나 선택하세요."))
            return

        # 현재 뼈 임계값 슬라이더(ui.thresholdSlider)의 값을 가져옵니다. 
        # 슬라이더가 없으면 기본값으로 200.0 HU를 사용합니다.
        threshold = self.ui.thresholdSlider.value if hasattr(self.ui, "thresholdSlider") else 200.0
        
        logging.info(f"Detecting masked 3D volume edges for: '{inputNode.GetName()}' with threshold {threshold}")
        with slicer.util.tryWithErrorDisplay(_("마스킹 볼륨 엣지 검출에 실패했습니다."), waitCursor=True):
            edgeNode = self.logic.detectVolumeEdgesMasked(inputNode, threshold)
            if edgeNode:
                # 엣지 검출 결과도 새로운 볼륨 노드이므로 현재 입력으로 교체한다.
                self.ui.inputVolumeSelector.setCurrentNode(edgeNode)
                slicer.util.infoDisplay(_(f"마스킹 엣지 볼륨을 성공적으로 생성했습니다: '{edgeNode.GetName()}'"))

    def onNodeAdded(self, caller, event) -> None:
        """
        Scene에 새 노드가 추가되면 필요한 UI 반응을 수행한다.

        - 새 scalar volume은 자동 입력으로 선택
        - 새 fragment 모델은 Fragment Manager 표에 반영
        """
        node = event
        if isinstance(node, str):
            node = slicer.mrmlScene.GetNodeByID(node)

        if not node:
            return

        if node.IsA("vtkMRMLScalarVolumeNode"):
            # 외부에서 새 CT 볼륨을 불러온 경우에도 사용자가 따로 선택하지 않아도 바로 반영한다.
            logging.info(f"New volume node detected in scene: '{node.GetName()}'. Auto-selecting in module...")
            self.ui.inputVolumeSelector.setCurrentNode(node)

        if node.IsA("vtkMRMLModelNode") and node.GetName().startswith("Femur_Fragment"):
            # 모델 생성 직후에는 display node 등이 늦게 준비될 수 있어 짧게 지연 후 표를 갱신한다.
            qt.QTimer.singleShot(100, self.updateFragmentTable)

    def onOpenPlannerWindowClicked(self) -> None:
        """모듈 패널이 닫혀 있거나 숨겨져 있을 때, 독립 실행 플래너 팝업 다이얼로그를 화면에 표출한다."""
        if self.plannerDialog:
            self.plannerDialog.show()
            self.plannerDialog.raise_()
            self.plannerDialog.activateWindow()

    def getSelectedRotationAxis(self) -> str:
        """현재 UI 라디오 버튼 그룹에서 활성화 선택된 3차원 회전 기준 축(X, Y, Z)을 문자열로 반환한다."""
        if self.ui.rotationYRadio.checked:
            return "Y"
        if self.ui.rotationZRadio.checked:
            return "Z"
        return "X"

    def onRotationAxisChanged(self, toggled: bool) -> None:
        """회전 축 라디오 버튼 선택이 변경되면, 기존 회전 각도 수치를 새 축을 기준으로 재연산 투사한다."""
        if toggled:
            # 축만 바꿔도 현재 슬라이더 각도를 새 축 기준으로 다시 적용한다.
            self.onRotationAngleChanged(self.ui.rotationAngleSlider.value)

    def onRotationAngleChanged(self, value: float) -> None:
        """
        [실시간 3D 회전 제어 흐름]
        슬라이더의 각도 변경 시, 부모 선형 변환 행렬의 로컬 회전 요소를 변경하여
        볼륨, 세그멘테이션 및 종속된 골편 모델들을 동시에 3D 공간 상에서 정교하게 회전 정렬한다.
        """
        inputNode = self.ui.inputVolumeSelector.currentNode()
        if not inputNode:
            return

        axis = self.getSelectedRotationAxis()
        # 회전 계산은 Logic에 맡기고, UI 핸들러는 현재 입력값만 전달한다.
        self.logic.rotateVolume(inputNode, axis, value)
        slicer.util.forceRenderAllViews()

    def onResetRotationClicked(self) -> None:
        """회전 제어 슬라이더 값을 0.0도로 리셋하여, 부모 변환 행렬을 단위 행렬(Identity) 상태로 초기화한다."""
        self.ui.rotationAngleSlider.value = 0.0
        logging.info("Interactive 3D rotation reset to 0 degrees.")

    def onRotate90Clicked(self) -> None:
        """현재 3D 회전 각도에 추가적으로 90도 회전을 더해 빠르게 직각 배치를 맞출 수 있게 돕는다."""
        currentAngle = self.ui.rotationAngleSlider.value
        newAngle = currentAngle + 90.0
        if newAngle > 180.0:
            # 슬라이더 범위가 -180~180이므로 270도 대신 -90도로 표현한다.
            newAngle -= 360.0
        self.ui.rotationAngleSlider.value = newAngle
        logging.info(f"Rapid rotation +90 degrees applied. New angle: {newAngle:.1f} degrees")

    def onSegmentationNodeChanged(self, node) -> None:
        """외부 activeSegmentationSelector 콤보박스의 변경을 내장 Segment Editor의 세그멘테이션 데이터로 연동한다."""
        if not self.segmentEditorWidget:
            return

        self.segmentEditorWidget.blockSignals(True)
        # selector에서 바뀐 segmentation을 Segment Editor에도 반영한다.
        # blockSignals는 서로가 서로를 다시 호출하는 순환 이벤트를 막기 위한 장치이다.
        self.segmentEditorWidget.setSegmentationNode(node)
        self.segmentEditorWidget.blockSignals(False)

        inputNode = self.ui.inputVolumeSelector.currentNode()
        if inputNode:
            self.segmentEditorWidget.setSourceVolumeNode(inputNode)
            # 세그멘테이션 노드가 바뀌었으므로 새 노드의 labelmap 범위를 현재 볼륨에 맞춘다.
            if node:
                node.SetReferenceImageGeometryParameterFromVolumeNode(inputNode)

    def onEditorSegmentationNodeChanged(self, node) -> None:
        """내장 Segment Editor 내부에서 활성 노드가 변경되면 역으로 상단 activeSegmentationSelector와 동기화한다."""
        if not self.segmentEditorWidget:
            return

        self.ui.activeSegmentationSelector.blockSignals(True)
        # Segment Editor 내부 선택이 바뀐 경우 상단 selector에도 같은 노드를 보여 준다.
        self.ui.activeSegmentationSelector.setCurrentNode(node)
        self.ui.activeSegmentationSelector.blockSignals(False)

        inputNode = self.ui.inputVolumeSelector.currentNode()
        if inputNode:
            self.segmentEditorWidget.setSourceVolumeNode(inputNode)
            # Editor 내부에서 세그멘테이션이 변경된 경우에도 geometry 동기화를 유지한다.
            if node:
                node.SetReferenceImageGeometryParameterFromVolumeNode(inputNode)

    def monitorActiveEffectAndResize(self) -> None:
        """
        [콜랩서블 패널 및 Threshold 옵션 감시 리사이징 타이머 모니터링 흐름]
        1. 플래너 팝업 창이 활성화되어 있고 표출 중인지 여부를 실시간 검사한다.
        2. 내장 Segment Editor 위젯의 현재 활성 이펙트(Active Effect) 객체의 이름을 파악한다.
        3. 만약 사용자가 'Threshold' 이펙트를 켜서 복잡한 임계값 옵션 패널이 하단으로 길게 전개되었다면,
           창의 크기를 세로 960px로 확장하여 UI 하단 옵션들이 팝업창 범위 밖으로 짤리지 않도록 감시 제어한다.
        4. 사용자가 임계값 설정을 마치고 다른 이펙트(예: Paint, Draw)로 넘어가면,
           다시 원래 기본 크기(세로 830px)로 복원하고 adjustSize를 호출해 레이아웃을 조밀하게 맞춘다.
        """
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
                # activeEffect()가 비어 있을 때는 MRML SegmentEditorNode의 이름 값을 보조로 확인한다.
                node = self.segmentEditorWidget.mrmlSegmentEditorNode()
                if node:
                    effectName = node.GetActiveEffectName()
            except Exception:
                pass

        isActive = effectName == "Threshold"
        if isActive != self.isThresholdActive:
            # Threshold 패널은 옵션이 많아 기본 창 높이에서는 일부가 잘릴 수   있다.
            self.isThresholdActive = isActive
            if isActive:
                self.plannerDialog.setMinimumHeight(960)
                self.plannerDialog.resize(850, 960)
            else:
                self.plannerDialog.setMinimumHeight(830)
                self.plannerDialog.resize(850, 830)
                qt.QTimer.singleShot(100, self.plannerDialog.adjustSize)

    def onSeparateFragmentsClicked(self) -> None:
        """5단계: VTK Connectivity 필터를 기동하여 세그멘테이션 메쉬에서 독립 뼈 파편(Model)들을 분리/신설한다."""
        segNode = self.ui.activeSegmentationSelector.currentNode()
        if not segNode:
            slicer.util.errorDisplay(_("먼저 입력 세그멘테이션 노드를 선택하세요."))
            return

        inputNode = self.ui.inputVolumeSelector.currentNode()
        logging.info(f"Separating disjoint bone fragments in segmentation: '{segNode.GetName()}'")

        with slicer.util.tryWithErrorDisplay(_("골편 분리에 실패했습니다."), waitCursor=True):
            separatedCount = self.logic.separateBoneFragments(segNode, inputNode)
        if separatedCount > 0:
            slicer.util.infoDisplay(_(f"{separatedCount}개의 독립 골편으로 분리했습니다."))

            # 3D 볼륨 렌더링이 활성화되어 있다면, 뼈 조각 모델을 뚜렷하게 관찰할 수 있도록 가시성을 자동으로 꺼둠
            if self.ui.volumeRenderingVisibilityButton.checked:
                self.ui.volumeRenderingVisibilityButton.checked = False

            # 골편 분리 성공 시 테이블 목록을 무조건 강제 갱신
            self.updateFragmentTable()
        else:
            slicer.util.errorDisplay(_("유효한 골편을 추출하지 못했습니다. 세그멘테이션에 실제 영역이 있는지 확인하세요."))

    def onMirrorGuideClicked(self) -> None:
        """6단계: 로드된 정상측 가이드를 X축 대칭 거울상 연산하고 법선을 보정한 가이드 뼈 모델을 신설한다."""
        guideNode = self.ui.guideModelSelector.currentNode()
        if not guideNode:
            slicer.util.errorDisplay(_("먼저 가이드 모델을 선택하세요."))
            return

        logging.info(f"Mirroring guide model: '{guideNode.GetName()}'")
        with slicer.util.tryWithErrorDisplay(_("가이드 모델 미러링에 실패했습니다."), waitCursor=True):
            mirroredNode = self.logic.mirrorModel(guideNode)
            if mirroredNode:
                self.ui.guideModelSelector.setCurrentNode(mirroredNode)
                slicer.util.infoDisplay(_(f"가이드 모델을 미러링했습니다: '{mirroredNode.GetName()}'"))

    def addLogMessage(self, message: str) -> None:
        """가상 뼈 정복 진행 로그 창에 시간 정보와 함께 메시지를 추가한다."""
        if hasattr(self.ui, "reductionLogWidget") and self.ui.reductionLogWidget:
            import datetime
            timeStr = datetime.datetime.now().strftime("%H:%M:%S")
            self.ui.reductionLogWidget.append(f"[{timeStr}] {message}")
            self.ui.reductionLogWidget.ensureCursorVisible()
            # UI 이벤트를 즉시 강제 처리하여 실시간 출력 갱신
            slicer.app.processEvents()

    def onRunIcpReductionClicked(self) -> None:
        """7단계: 분리된 골편들의 정점을 가이드 뼈 표면에 매칭하여 최적 정합 4x4 부모 변환 행렬을 자동 산출하고 정렬한다."""
        guideNode = self.ui.guideModelSelector.currentNode()
        if not guideNode:
            slicer.util.errorDisplay(_("먼저 가이드 모델을 선택하세요."))
            return

        fragmentNodes = []
        nodes = slicer.mrmlScene.GetNodesByClass("vtkMRMLModelNode")
        for i in range(nodes.GetNumberOfItems()):
            node = nodes.GetItemAsObject(i)
            if node.GetName().startswith("Femur_Fragment"):
                # 골편 분리 로직이 만든 모델만 정합 대상으로 모은다.
                fragmentNodes.append(node)

        if not fragmentNodes:
            slicer.util.errorDisplay(_("분리된 골편(Femur_Fragment_*)이 없습니다. 먼저 5단계를 실행하세요."))
            return

        if hasattr(self.ui, "reductionLogWidget") and self.ui.reductionLogWidget:
            self.ui.reductionLogWidget.clear()
            self.addLogMessage("========================================")
            self.addLogMessage("ICP 자동 정복(Auto-Reduction) 연산 시작...")
            self.addLogMessage(f"정합 대상 골편 수: {len(fragmentNodes)}개")
            self.addLogMessage(f"기준 가이드 모델: '{guideNode.GetName()}'")
            self.addLogMessage("========================================")

        logging.info(f"Running ICP auto-reduction for {len(fragmentNodes)} fragments against guide: '{guideNode.GetName()}'")
        with slicer.util.tryWithErrorDisplay(_("ICP 자동 정복 실행에 실패했습니다."), waitCursor=True):
            self.logic.runIcpRegistration(fragmentNodes, guideNode, logCallback=self.addLogMessage)
            self.updateFragmentTable()
            self.addLogMessage("▶ ICP 자동 정복이 최종 성공적으로 완료되었습니다.")
            slicer.util.infoDisplay(_("ICP 자동 정복이 완료되었습니다."))

    def onRunSurfaceSnapClicked(self) -> None:
        """8단계: 골편 1과 골편 2의 절단 랜드마크 점군을 추출해 최근접 거리 25mm 영역 간 강체 스냅 결합을 실행한다."""
        fragmentNodes = []
        nodes = slicer.mrmlScene.GetNodesByClass("vtkMRMLModelNode")
        for i in range(nodes.GetNumberOfItems()):
            node = nodes.GetItemAsObject(i)
            if node.GetName().startswith("Femur_Fragment"):
                fragmentNodes.append(node)

        if len(fragmentNodes) < 2:
            slicer.util.errorDisplay(
                _("Fracture Surface Snap을 실행하려면 최소 2개의 분리된 골편(Femur_Fragment_1, Femur_Fragment_2)이 필요합니다.")
            )
            return

        # 씬에 등록된 Femur_Fragment_* 접두사를 가진 노드 중 고유한 1번과 2번 노드를 추출
        frag1 = None
        frag2 = None
        for node in fragmentNodes:
            if "Fragment_1" in node.GetName():
                frag1 = node
            elif "Fragment_2" in node.GetName():
                frag2 = node

        # 이름 불일치 혹은 중복 시 순차 리스트 적용
        if not frag1 or not frag2 or frag1.GetID() == frag2.GetID():
            frag1 = fragmentNodes[0]
            frag2 = fragmentNodes[1]

        if frag1.GetID() == frag2.GetID():
            slicer.util.errorDisplay(_("오류: 정합 대상 두 골편 노드가 동일한 객체입니다. 씬 노드를 확인하세요."))
            return

        if hasattr(self.ui, "reductionLogWidget") and self.ui.reductionLogWidget:
            self.ui.reductionLogWidget.clear()
            self.addLogMessage("========================================")
            self.addLogMessage("Fracture Surface Snap 정합 연산 시작...")
            self.addLogMessage(f"실행 로직 버전: {self.logic.VERSION if hasattr(self.logic, 'VERSION') else 'Unknown (캐시꼬임)'}")
            self.addLogMessage(f"움직일 대상: '{frag1.GetName()}' (ID: {frag1.GetID()})")
            self.addLogMessage(f"└─ Parent Transform ID: {frag1.GetParentTransformNode().GetID() if frag1.GetParentTransformNode() else 'None'}")
            self.addLogMessage(f"고정된 기준: '{frag2.GetName()}' (ID: {frag2.GetID()})")
            self.addLogMessage(f"└─ Parent Transform ID: {frag2.GetParentTransformNode().GetID() if frag2.GetParentTransformNode() else 'None'}")
            self.addLogMessage("========================================")

        logging.info(f"Running Fracture Surface Snap between: '{frag1.GetName()}' and '{frag2.GetName()}'")
        with slicer.util.tryWithErrorDisplay(_("Fracture Surface Snap 실행에 실패했습니다. 골절면이 충분히 가까운지 확인하세요."), waitCursor=True):
            self.logic.runFractureSurfaceSnap(frag1, frag2, logCallback=self.addLogMessage)
            self.updateFragmentTable()
            self.onFragmentTableSelectionChanged()
            self.addLogMessage("▶ Fracture Surface Snap이 최종 성공적으로 완료되었습니다.")
            slicer.util.infoDisplay(_("Fracture Surface Snap이 완료되었습니다."))

    def onRunLandmarkMatchClicked(self) -> None:
        """수동 타겟팅: 마커 기반 정밀 매칭 (점 찍기)"""
        guideNode = self.ui.guideModelSelector.currentNode()
        if not guideNode:
            slicer.util.errorDisplay(_("먼저 타겟 뼈를 위 '가이드 모델(또는 기준 골편)' 창에서 선택하세요."))
            return
            
        selectedItems = self.ui.fragmentTableWidget.selectedItems()
        if not selectedItems:
            slicer.util.errorDisplay(_("표에서 움직일 대상 골편을 1개 선택하세요."))
            return
            
        fragmentNodeId = selectedItems[0].data(qt.Qt.UserRole)
        fragmentNode = slicer.mrmlScene.GetNodeByID(fragmentNodeId)
        
        sourceMarkersNode = slicer.mrmlScene.GetFirstNodeByName("Targeting_Source_Markers")
        targetMarkersNode = slicer.mrmlScene.GetFirstNodeByName("Targeting_Target_Markers")
        
        if not sourceMarkersNode or not targetMarkersNode:
            if not sourceMarkersNode:
                sourceMarkersNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLMarkupsFiducialNode", "Targeting_Source_Markers")
                sourceMarkersNode.GetDisplayNode().SetSelectedColor(1, 1, 0)
            if not targetMarkersNode:
                targetMarkersNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLMarkupsFiducialNode", "Targeting_Target_Markers")
                targetMarkersNode.GetDisplayNode().SetSelectedColor(0, 1, 1)
                
            selectionNode = slicer.mrmlScene.GetNodeByID("vtkMRMLSelectionNodeSingleton")
            selectionNode.SetReferenceActivePlaceNodeClassName("vtkMRMLMarkupsFiducialNode")
            selectionNode.SetActivePlaceNodeID(sourceMarkersNode.GetID())
            interactionNode = slicer.mrmlScene.GetNodeByID("vtkMRMLInteractionNodeSingleton")
            interactionNode.SetCurrentInteractionMode(slicer.vtkMRMLInteractionNode.Place)
                
            slicer.util.infoDisplay(_("마커 노드가 생성되어 '마우스 점 찍기 모드'가 자동으로 켜졌습니다.\n\n"
                                      "1. 3D 뷰어에서 움직일 대상 골편 표면에 점 3개를 클릭해 찍습니다.\n"
                                      "2. 이어서 타겟 뼈(가이드 또는 기준 골편) 표면에 똑같이 점 3개를 찍습니다. (총 6개)\n"
                                      "3. 완료 후 이 버튼을 다시 눌러주세요."))
            return
            
        # 사용자가 대상을 바꾸지 않고 Source 노드(노란색)에만 6개(또는 짝수개)를 연달아 찍은 경우 자동 분할 편의성 제공
        if sourceMarkersNode.GetNumberOfControlPoints() >= 6 and targetMarkersNode.GetNumberOfControlPoints() == 0:
            numTotal = sourceMarkersNode.GetNumberOfControlPoints()
            # 짝수 개수만 허용하여 앞 절반은 Source, 뒤 절반은 Target으로 분할
            if numTotal % 2 == 0:
                numHalf = numTotal // 2
                for i in range(numHalf, numTotal):
                    pos = [0.0, 0.0, 0.0]
                    sourceMarkersNode.GetNthControlPointPositionWorld(i, pos)
                    targetMarkersNode.AddControlPointWorld(vtk.vtkVector3d(pos[0], pos[1], pos[2]))
                # Source에서는 복사된 뒤쪽 점들을 제거 (인덱스 오류 방지를 위해 역순 삭제)
                for i in range(numTotal - 1, numHalf - 1, -1):
                    sourceMarkersNode.RemoveNthControlPoint(i)
            
        if sourceMarkersNode.GetNumberOfControlPoints() < 3 or targetMarkersNode.GetNumberOfControlPoints() < 3:
            slicer.util.errorDisplay(_("점(마커) 개수가 부족합니다. 각각 최소 3개의 점이 필요합니다."))
            return
            
        if sourceMarkersNode.GetNumberOfControlPoints() != targetMarkersNode.GetNumberOfControlPoints():
            slicer.util.errorDisplay(_("두 마커 노드의 점 개수가 일치하지 않습니다."))
            return
            
        logging.info("Running Landmark Registration")
        with slicer.util.tryWithErrorDisplay(_("마커 기반 매칭 실행에 실패했습니다."), waitCursor=True):
            self.logic.runLandmarkRegistration(fragmentNode, guideNode, sourceMarkersNode, targetMarkersNode)
            self.updateFragmentTable()
            slicer.util.infoDisplay(_("마커 기반 정밀 매칭이 완료되었습니다."))

    def onRunMaskMatchClicked(self) -> None:
        """수동 타겟팅: 색칠 영역 기반 매칭 (마스킹)"""
        guideNode = self.ui.guideModelSelector.currentNode()
        if not guideNode:
            slicer.util.errorDisplay(_("먼저 타겟 뼈를 위 '가이드 모델(또는 기준 골편)' 창에서 선택하세요."))
            return
            
        selectedItems = self.ui.fragmentTableWidget.selectedItems()
        if not selectedItems:
            slicer.util.errorDisplay(_("표에서 움직일 대상 골편을 1개 선택하세요."))
            return
            
        fragmentNodeId = selectedItems[0].data(qt.Qt.UserRole)
        fragmentNode = slicer.mrmlScene.GetNodeByID(fragmentNodeId)
        
        segNode = self.ui.activeSegmentationSelector.currentNode()
        if not segNode:
            slicer.util.errorDisplay(_("먼저 입력 세그멘테이션 노드를 5번 영역에서 지정하세요 (4번 에디터와 연동됨)."))
            return
            
        segmentation = segNode.GetSegmentation()
        sourceSegId = segmentation.GetSegmentIdBySegmentName("Targeting_Source_ROI")
        targetSegId = segmentation.GetSegmentIdBySegmentName("Targeting_Target_ROI")
        
        if not sourceSegId or not targetSegId:
            if not sourceSegId:
                segNode.GetSegmentation().AddEmptySegment("Targeting_Source_ROI", "Targeting_Source_ROI", [1.0, 1.0, 0.0])
            if not targetSegId:
                segNode.GetSegmentation().AddEmptySegment("Targeting_Target_ROI", "Targeting_Target_ROI", [0.0, 1.0, 1.0])
            
            slicer.util.infoDisplay(_("세그멘테이션 라벨(Targeting_Source_ROI, Targeting_Target_ROI)이 추가되었습니다.\n\n"
                                      "4번 Segment Editor 탭에서 Paint 브러시를 이용해\n"
                                      "움직일 골편 단면과 타겟 뼈(가이드 또는 기준 골편) 단면을 각각 색칠한 뒤\n"
                                      "이 버튼을 다시 눌러주세요."))
            return
            
        logging.info("Running Mask-based ICP Registration")
        with slicer.util.tryWithErrorDisplay(_("색칠 영역 기반 매칭 실행에 실패했습니다."), waitCursor=True):
            self.logic.runMaskedIcpRegistration(fragmentNode, guideNode, segNode, sourceSegId, targetSegId)
            self.updateFragmentTable()
            slicer.util.infoDisplay(_("색칠 영역 기반 매칭이 완료되었습니다."))

    def onClearMarkersClicked(self) -> None:
        """수동 타겟팅용 마커(점) 노드들을 모두 지우고 초기화한다."""
        sourceMarkersNode = slicer.mrmlScene.GetFirstNodeByName("Targeting_Source_Markers")
        targetMarkersNode = slicer.mrmlScene.GetFirstNodeByName("Targeting_Target_Markers")
        
        cleared = False
        if sourceMarkersNode:
            sourceMarkersNode.RemoveAllControlPoints()
            cleared = True
        if targetMarkersNode:
            targetMarkersNode.RemoveAllControlPoints()
            cleared = True
            
        if cleared:
            slicer.util.infoDisplay(_("마커(점)가 모두 지워졌습니다. 이제 다시 정확한 위치에 순서대로 점을 찍어주세요."))
        else:
            slicer.util.infoDisplay(_("지울 마커가 없습니다."))

    def onClearMasksClicked(self) -> None:
        """수동 타겟팅용 마스킹(색칠 영역) 라벨들을 깨끗하게 비운다."""
        segNode = self.ui.activeSegmentationSelector.currentNode()
        if not segNode:
            slicer.util.infoDisplay(_("선택된 세그멘테이션 노드가 없습니다."))
            return
            
        segmentation = segNode.GetSegmentation()
        sourceSegId = segmentation.GetSegmentIdBySegmentName("Targeting_Source_ROI")
        targetSegId = segmentation.GetSegmentIdBySegmentName("Targeting_Target_ROI")
        
        cleared = False
        if sourceSegId:
            segmentation.RemoveSegment(sourceSegId)
            cleared = True
        if targetSegId:
            segmentation.RemoveSegment(targetSegId)
            cleared = True
            
        if cleared:
            slicer.util.infoDisplay(_("색칠된 마스킹 영역이 모두 지워졌습니다."))
        else:
            slicer.util.infoDisplay(_("지울 마스킹 영역이 없습니다."))

    def onLoadGuideClicked(self) -> None:
        """외부 3D 뼈 파일(*.stl, *.obj 등)을 다이렉트 선택 로드하여 가이드 모델 노드로 배치한다."""
        filePath = qt.QFileDialog.getOpenFileName(
            slicer.util.mainWindow(),
            _("가이드 뼈 모델 파일 선택"),
            "",
            "모델 파일 (*.stl *.obj *.ply *.vtk)",
        )

        if not filePath:
            return

        logging.info(f"Loading guide model file: '{filePath}'")
        with slicer.util.tryWithErrorDisplay(_("가이드 모델 파일을 불러오지 못했습니다."), waitCursor=True):
            loadedNode = slicer.util.loadModel(filePath)
            if loadedNode:
                self.ui.guideModelSelector.setCurrentNode(loadedNode)
                slicer.util.infoDisplay(_(f"가이드 모델을 불러왔습니다: '{loadedNode.GetName()}'"))

    def onGuideVisibilityToggled(self, checked: bool) -> None:
        """3D 뷰 및 단면 뷰에서 가이드 모델의 가시성 상태(Visibility)를 켜거나 끈다."""
        guideNode = self.ui.guideModelSelector.currentNode()
        if not guideNode:
            if checked:
                self.ui.guideVisibilityButton.blockSignals(True)
                self.ui.guideVisibilityButton.checked = False
                self.ui.guideVisibilityButton.blockSignals(False)
                slicer.util.errorDisplay(_("먼저 가이드 모델을 선택하세요."))
            return

        displayNode = guideNode.GetDisplayNode()
        if displayNode:
            # display node의 visibility는 3D 뷰에서 모델을 보일지 숨길지 결정한다.
            displayNode.SetVisibility(checked)
            slicer.util.forceRenderAllViews()

        if checked:
            self.ui.guideVisibilityButton.text = _("숨기기")
            self.ui.guideVisibilityButton.toolTip = _("뷰어에서 가이드 모델을 숨깁니다.")
        else:
            self.ui.guideVisibilityButton.text = _("보기")
            self.ui.guideVisibilityButton.toolTip = _("뷰어에서 가이드 모델을 표시합니다.")

    def onGuideInteractionToggled(self, checked: bool) -> None:
        """가이드 모델의 3D 뷰 수동 조작 Gizmo 표시를 토글한다."""
        guideNode = self.ui.guideModelSelector.currentNode()
        if not guideNode:
            if checked:
                self.ui.guideInteractionButton.blockSignals(True)
                self.ui.guideInteractionButton.checked = False
                self.ui.guideInteractionButton.blockSignals(False)
                slicer.util.errorDisplay(_("먼저 가이드 모델을 선택하세요."))
            return

        transformNode = guideNode.GetParentTransformNode()
        originalParentTransform = None

        if transformNode and not transformNode.GetName().endswith("_Transform"):
            originalParentTransform = transformNode
            transformNode = None

        if not transformNode or not transformNode.IsA("vtkMRMLLinearTransformNode"):
            transformNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
            transformNode.SetName(f"{guideNode.GetName()}_Transform")

            if originalParentTransform:
                transformNode.SetAndObserveTransformNodeID(originalParentTransform.GetID())

            guideNode.SetAndObserveTransformNodeID(transformNode.GetID())

        tDisplayNode = transformNode.GetDisplayNode()
        if not tDisplayNode:
            transformNode.CreateDefaultDisplayNodes()
            tDisplayNode = transformNode.GetDisplayNode()

        if tDisplayNode:
            tDisplayNode.SetEditorVisibility(checked)

        slicer.util.forceRenderAllViews()

    def onGuideModelChanged(self, node) -> None:
        """선택 가이드 모델이 변경되면, 해당 신규 가이드 모델의 표시 visibility 값을 획득해 UI 버튼 체크 상태와 연동한다."""
        if not node:
            self.ui.guideVisibilityButton.blockSignals(True)
            self.ui.guideVisibilityButton.checked = False
            self.ui.guideVisibilityButton.text = _("보기")
            self.ui.guideVisibilityButton.blockSignals(False)
            
            self.ui.guideInteractionButton.blockSignals(True)
            self.ui.guideInteractionButton.checked = False
            self.ui.guideInteractionButton.blockSignals(False)
            return

        displayNode = node.GetDisplayNode()
        isVisible = displayNode.GetVisibility() if displayNode else False

        transformNode = node.GetParentTransformNode()
        isInteractable = False
        if transformNode and transformNode.GetName().endswith("_Transform"):
            tDisplayNode = transformNode.GetDisplayNode()
            if tDisplayNode:
                isInteractable = tDisplayNode.GetEditorVisibility()

        # 실제 모델 표시 상태를 버튼 체크 상태와 텍스트에 반영한다.
        self.ui.guideVisibilityButton.checked = isVisible
        if isVisible:
            self.ui.guideVisibilityButton.text = _("숨기기")
        else:
            self.ui.guideVisibilityButton.text = _("보기")
        self.ui.guideVisibilityButton.blockSignals(False)

        self.ui.guideInteractionButton.blockSignals(True)
        self.ui.guideInteractionButton.checked = isInteractable
        self.ui.guideInteractionButton.blockSignals(False)

    def onNodeRemoved(self, caller, event) -> None:
        """장면(Scene)에서 골편 모델이 제거(Remove)되면 QTableWidget 목록에서 해당 행을 정리한다."""
        node = event
        if isinstance(node, str):
            node = slicer.mrmlScene.GetNodeByID(node)

        if node and node.IsA("vtkMRMLModelNode") and node.GetName().startswith("Femur_Fragment"):
            qt.QTimer.singleShot(100, self.updateFragmentTable)
