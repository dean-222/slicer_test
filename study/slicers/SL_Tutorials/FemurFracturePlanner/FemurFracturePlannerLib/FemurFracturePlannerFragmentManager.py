"""
File Name: FemurFracturePlannerFragmentManager.py
Version: v0-0.1.4
Date: 2026-06-26
Description: 대퇴골 골절 계획 모듈의 Fragment Manager UI, 골편 표시 제어, 선택 추적 로직을 담당한다.

Version History:
- v0-0.1.4 (2026-06-26)
  - onActiveTransformNodeModified 함수 내에서 골편의 로컬 Bounds 중심 좌표에 worldMatrix를 곱해주어, 3D 수동 조작 시 UI 수치 패널의 Position 정보가 실시간으로 갱신되도록 버그 수정 (Z 증가)
- v0-0.1.3 (2026-06-25)
  - 골편 생성 시 골편 관리자 표의 색상이 실제 3D 뷰어의 색상과 불일치하는 버그 수정, 그리고 미러링 가이드 삭제 시 원본 가이드 골편까지 함께 씬에서 제거하도록 연동 추가 (Z 증가)
- v0-0.1.2 (2026-06-25)
  - 전체 삭제 시 guideModelSelector에 로드된 원래 가이드 모델 및 부모 변환 노드도 함께 씬에서 제거하고 선택을 해제하도록 수정 (Z 증가)
- v0-0.1.1 (2026-06-24)
  - 골편 및 가이드 모델 삭제 시, 모델에 할당된 3D 수동 조작용 변환 노드(vtkMRMLLinearTransformNode)가 삭제되지 않고 씬에 남아 3D Gizmo 잔상이 생기던 버그 수정 (Z 증가)
- v0-0.1.0 (2026-06-24)
  - QTableWidget을 활용한 골편 목록 표 구성, 가시성/색상/삭제 바인딩 로직, 실시간 변환 옵저버(vtkCommand.ModifiedEvent)를 활용한 3D 월드 좌표 및 오리엔테이션 각도(Roll/Pitch/Yaw) 동기화 파이프라인 상세 설명 추가 (Y 증가)
- v0-0.0.0 (2026-06-23)
  - FemurFracturePlannerWidget.py에서 Fragment Manager 관련 메서드를 분리하여 최초 작성
"""

import logging

import qt
import slicer
import vtk

# Fragment Manager는 "분리된 골편 모델을 목록으로 보여 주고 조작하는" UI 보조 계층이다.
# 계산 알고리즘은 Logic에 있고, 여기서는 표 갱신과 표시/색상/삭제 같은 사용자 조작을 맡는다.


class FemurFracturePlannerFragmentManagerMixin:
    """
    Fragment Manager 영역을 담당하는 mixin이다.

    이 파일은 골편 목록 표(QTableWidget)를 만들고, 각 골편의
    - 표시/숨김 (display node visibility)
    - 수동 이동 gizmo (transform node editor visibility)
    - 색상 변경 (QColorDialog 연동)
    - 삭제 (mrmlScene node remove)
    - 실시간 위치/회전 표시 (vtkCommand.ModifiedEvent 옵저버 동기화)
    를 한곳에서 관리한다.
    """

    def updateFragmentTable(self) -> None:
        """
        Scene의 `Femur_Fragment*` 모델 목록을 표 형태로 다시 그린다.

        [QTableWidget을 활용한 골편 목록 동적 표 구성 및 바인딩 로직]
        1. 씬(Scene)에서 `Femur_Fragment`로 시작하는 모든 vtkMRMLModelNode를 수집한다.
        2. 수집된 모델 수에 맞추어 QTableWidget의 행(Row) 개수를 조절한다.
        3. 각 행(Row)에 대해 다음 5개 열(Column)을 구성하고 동적 바인딩을 수행한다:
           - 0열: 골편 이름 (QTableWidgetItem, UserRole에 Node ID 보관)
           - 1열: 가시성 토글 (QCheckBox, toggled 이벤트를 onFragmentVisibilityToggled와 연결)
           - 2열: 3D 수동 조작 Gizmo 표시 여부 (QCheckBox, toggled 이벤트를 onFragmentInteractionToggled와 연결)
           - 3열: 골편 색상 변경 버튼 (QPushButton, 클릭 시 QColorDialog 호출 및 색상 프리뷰 업데이트)
           - 4열: 삭제 버튼 (QPushButton, 클릭 시 씬에서 모델 노드를 제거하고 테이블 자동 갱신)
        """
        if not self.plannerDialog:
            return

        table = self.ui.fragmentTableWidget
        # 표를 새로 그리는 동안 itemChanged 같은 이벤트가 연쇄 호출되지 않도록 잠시 막는다.
        table.blockSignals(True)
        table.clear()

        table.setColumnCount(5)
        table.setHorizontalHeaderLabels(["이름", "표시", "이동", "색상", "삭제"])
        header = table.horizontalHeader()
        header.setSectionResizeMode(0, qt.QHeaderView.Stretch)
        header.setSectionResizeMode(1, qt.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(2, qt.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(3, qt.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(4, qt.QHeaderView.ResizeToContents)

        fragmentNodes = []
        nodes = slicer.mrmlScene.GetNodesByClass("vtkMRMLModelNode")
        for i in range(nodes.GetNumberOfItems()):
            node = nodes.GetItemAsObject(i)
            if node.GetName().startswith("Femur_Fragment"):
                # separateBoneFragments가 만든 골편 모델은 Femur_Fragment 접두어로 구분한다.
                fragmentNodes.append(node)

        table.setRowCount(len(fragmentNodes))

        for row, node in enumerate(fragmentNodes):
            nameItem = qt.QTableWidgetItem(node.GetName())
            # 화면에 보이는 이름 셀에 MRML node ID도 함께 숨겨 저장한다.
            # 나중에 사용자가 이름을 수정하거나 행을 선택했을 때 원래 모델 노드를 찾기 위해서이다.
            nameItem.setData(qt.Qt.UserRole, node.GetID())
            table.setItem(row, 0, nameItem)

            showWidget = qt.QCheckBox()
            displayNode = node.GetDisplayNode()
            showWidget.checked = displayNode.GetVisibility() if displayNode else False
            # lambda의 n=node는 반복문 변수 node가 나중에 바뀌어도 현재 행의 노드를 기억하게 한다.
            showWidget.connect("toggled(bool)", lambda checked, n=node: self.onFragmentVisibilityToggled(n, checked))

            showContainer = qt.QWidget()
            showLayout = qt.QHBoxLayout(showContainer)
            showLayout.addWidget(showWidget)
            showLayout.setAlignment(qt.Qt.AlignCenter)
            showLayout.setContentsMargins(0, 0, 0, 0)
            table.setCellWidget(row, 1, showContainer)

            moveWidget = qt.QCheckBox()
            transformNode = node.GetParentTransformNode()
            if not transformNode or not transformNode.IsA("vtkMRMLLinearTransformNode"):
                # 수동 이동 gizmo는 transform node의 editor visibility를 통해 표시된다.
                # transform이 없으면 먼저 만들어 모델에 연결해야 3D 조작 핸들이 나타난다.
                transformNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
                transformNode.SetName(f"{node.GetName()}_Transform")
                node.SetAndObserveTransformNodeID(transformNode.GetID())

            if not transformNode.GetDisplayNode():
                # transform display node가 있어야 3D 뷰에서 이동/회전 핸들을 표시할 수 있다.
                transformNode.CreateDefaultDisplayNodes()
            transformDisplayNode = transformNode.GetDisplayNode()

            moveWidget.checked = transformDisplayNode.GetEditorVisibility() if transformDisplayNode else False
            moveWidget.connect("toggled(bool)", lambda checked, n=node: self.onFragmentInteractionToggled(n, checked))

            moveContainer = qt.QWidget()
            moveLayout = qt.QHBoxLayout(moveContainer)
            moveLayout.addWidget(moveWidget)
            moveLayout.setAlignment(qt.Qt.AlignCenter)
            moveLayout.setContentsMargins(0, 0, 0, 0)
            table.setCellWidget(row, 2, moveContainer)

            colorButton = qt.QPushButton()
            colorButton.setFixedWidth(40)
            colorButton.setFixedHeight(20)
            if displayNode:
                rgb = displayNode.GetColor()
            elif transformDisplayNode:
                rgb = transformDisplayNode.GetColor()
            else:
                rgb = None

            if rgb:
                # Slicer 색상 값은 0.0~1.0 범위이고, Qt stylesheet은 0~255 정수 RGB를 사용한다.
                r, g, b = int(rgb[0] * 255), int(rgb[1] * 255), int(rgb[2] * 255)
                colorButton.setStyleSheet(
                    f"background-color: rgb({r}, {g}, {b}); border: 1px solid gray; border-radius: 3px;"
                )

            colorButton.connect("clicked()", lambda n=node, btn=colorButton: self.onFragmentColorClicked(n, btn))

            colorContainer = qt.QWidget()
            colorLayout = qt.QHBoxLayout(colorContainer)
            colorLayout.addWidget(colorButton)
            colorLayout.setAlignment(qt.Qt.AlignCenter)
            colorLayout.setContentsMargins(0, 0, 0, 0)
            table.setCellWidget(row, 3, colorContainer)

            deleteButton = qt.QPushButton("X")
            deleteButton.setFixedWidth(30)
            deleteButton.setFixedHeight(22)
            # 삭제 버튼도 현재 행의 node를 기억하도록 기본 인자 n=node를 사용한다.
            deleteButton.connect("clicked()", lambda n=node: self.onFragmentDeleteClicked(n))

            deleteContainer = qt.QWidget()
            deleteLayout = qt.QHBoxLayout(deleteContainer)
            deleteLayout.addWidget(deleteButton)
            deleteLayout.setAlignment(qt.Qt.AlignCenter)
            deleteLayout.setContentsMargins(0, 0, 0, 0)
            table.setCellWidget(row, 4, deleteContainer)

        table.blockSignals(False)

    def onFragmentTableItemChanged(self, item: qt.QTableWidgetItem) -> None:
        """표에서 골편 이름을 수정하면 MRML 모델 노드 이름에도 반영한다."""
        if item.column() != 0:
            return

        nodeId = item.data(qt.Qt.UserRole)
        if not nodeId:
            return

        node = slicer.mrmlScene.GetNodeByID(nodeId)
        if node and node.GetName() != item.text():
            # 표의 이름과 MRML 노드 이름을 맞춰 두면 Slicer subject hierarchy에서도 같은 이름으로 보인다.
            logging.info(f"Renaming fragment node from '{node.GetName()}' to '{item.text()}'")
            node.SetName(item.text())
            slicer.util.forceRenderAllViews()

    def onFragmentVisibilityToggled(self, node, checked: bool) -> None:
        """골편 모델의 표시 여부를 3D 뷰어 및 슬라이스 뷰어에서 켜거나 숨긴다."""
        displayNode = node.GetDisplayNode()
        if displayNode:
            # 모델 자체를 삭제하지 않고 display node의 visibility만 바꾼다.
            displayNode.SetVisibility(checked)
            slicer.util.forceRenderAllViews()

    def onFragmentColorClicked(self, node, button) -> None:
        """색상 선택기를 열어 골편 모델의 표시 색상을 변경한다."""
        displayNode = node.GetDisplayNode()
        if not displayNode:
            return

        rgb = displayNode.GetColor()
        initialColor = qt.QColor.fromRgbF(rgb[0], rgb[1], rgb[2])

        # QColorDialog는 사용자가 색을 직접 고를 수 있는 Qt 기본 색상 선택 창이다.
        color = qt.QColorDialog.getColor(initialColor, slicer.util.mainWindow(), "골편 색상 선택")
        if color.isValid():
            r, g, b = color.redF(), color.greenF(), color.blueF()
            displayNode.SetColor(r, g, b)
            btnR, btnG, btnB = color.red(), color.green(), color.blue()
            button.setStyleSheet(
                f"background-color: rgb({btnR}, {btnG}, {btnB}); border: 1px solid gray; border-radius: 3px;"
            )
            slicer.util.forceRenderAllViews()

    def onFragmentDeleteClicked(self, node) -> None:
        """선택한 골편 모델 및 연관 변환 노드를 Scene에서 삭제하고 테이블을 자동 갱신한다."""
        logging.info(f"Removing fragment node: '{node.GetName()}'")
        
        # 골편에 연결되어 수동 조작에 쓰이던 변환 노드도 찾아 삭제 목록에 추가
        transformNode = node.GetParentTransformNode()
        slicer.mrmlScene.RemoveNode(node)

        if transformNode and transformNode.IsA("vtkMRMLLinearTransformNode"):
            tName = transformNode.GetName()
            if tName.startswith("Femur_Fragment") and (tName.endswith("_Transform") or tName.endswith("_RotationTransform")):
                logging.info(f"Removing associated transform node: '{tName}'")
                slicer.mrmlScene.RemoveNode(transformNode)

        slicer.util.forceRenderAllViews()

    def onFragmentInteractionToggled(self, node, checked: bool) -> None:
        """
        골편 모델의 3D 뷰 3차원 수동 조작 Gizmo 표시를 토글한다.

        - Gizmo가 활성화되면 사용자가 3D 뷰에서 마우스로 직접 골편을 이동/회전시킬 수 있다.
        - 수동 이동용 선형 변환 노드가 존재하지 않는 경우 자동으로 신설하여 바인딩한다.
        - EditorVisibility가 True이면 3D 뷰 상에 조작 핸들(Translate/Rotate)이 표출된다.
        """
        transformNode = node.GetParentTransformNode()
        originalParentTransform = None

        if transformNode and not transformNode.GetName().endswith("_Transform"):
            # 이미 다른 상위 transform이 붙어 있다면 그것을 보존하고,
            # 골편 수동 이동용 transform을 그 아래 새로 끼워 넣는다.
            originalParentTransform = transformNode
            transformNode = None

        if not transformNode or not transformNode.IsA("vtkMRMLLinearTransformNode"):
            transformNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
            transformNode.SetName(f"{node.GetName()}_Transform")

            if originalParentTransform:
                transformNode.SetAndObserveTransformNodeID(originalParentTransform.GetID())

            node.SetAndObserveTransformNodeID(transformNode.GetID())

        if not transformNode.GetDisplayNode():
            transformNode.CreateDefaultDisplayNodes()
        displayNode = transformNode.GetDisplayNode()
        if displayNode:
            # EditorVisibility가 True이면 3D 뷰에 이동/회전 gizmo가 나타난다.
            displayNode.SetEditorVisibility(checked)

        self.onFragmentTableSelectionChanged()
        slicer.util.forceRenderAllViews()

    def onClearModelsClicked(self) -> None:
        """분리된 모든 골편 모델, 미러링 가이드 모델 및 연관 변환 노드를 Scene에서 일괄 삭제한다."""
        logging.info("Clearing all fragmented and mirrored model nodes from the scene...")

        nodesToDelete = []
        transformsToDelete = []

        # 1. guideModelSelector에 현재 선택된 가이드 모델도 명시적으로 수집 대상에 포함
        guideNode = self.ui.guideModelSelector.currentNode()
        if guideNode:
            nodesToDelete.append(guideNode)
            transformNode = guideNode.GetParentTransformNode()
            if transformNode and transformNode.IsA("vtkMRMLLinearTransformNode"):
                transformsToDelete.append(transformNode)

            # 만약 현재 선택된 가이드 노드가 미러링된 노드라면, 원본 가이드 노드도 찾아 삭제 목록에 추가
            guideName = guideNode.GetName()
            if guideName.endswith("_Mirrored"):
                originalName = guideName[:-9]  # "_Mirrored" 제거
                originalNode = slicer.mrmlScene.GetFirstNodeByName(originalName)
                if originalNode and originalNode.IsA("vtkMRMLModelNode") and originalNode not in nodesToDelete:
                    nodesToDelete.append(originalNode)
                    origTransform = originalNode.GetParentTransformNode()
                    if origTransform and origTransform.IsA("vtkMRMLLinearTransformNode"):
                        transformsToDelete.append(origTransform)

        # 2. 대상 모델 노드 수집 및 연관 변환 노드 추적
        nodes = slicer.mrmlScene.GetNodesByClass("vtkMRMLModelNode")
        for i in range(nodes.GetNumberOfItems()):
            node = nodes.GetItemAsObject(i)
            nodeName = node.GetName()
            # 분리 골편 또는 미러링된 대칭 가이드 모델인 경우
            if nodeName.startswith("Femur_Fragment") or nodeName.endswith("_Mirrored"):
                if node not in nodesToDelete:
                    nodesToDelete.append(node)
                
                # 모델에 직접 연결된 부모 변환 노드도 삭제 대상으로 수집
                transformNode = node.GetParentTransformNode()
                if transformNode and transformNode.IsA("vtkMRMLLinearTransformNode"):
                    tName = transformNode.GetName()
                    if tName.startswith("Femur_Fragment") and (tName.endswith("_Transform") or tName.endswith("_RotationTransform")):
                        if transformNode not in transformsToDelete:
                            transformsToDelete.append(transformNode)

        # 3. 씬에 고아로 남아 있을 수 있는 이름이 유사한 뼈 조각 전용 변환 노드들도 수집
        transformNodes = slicer.mrmlScene.GetNodesByClass("vtkMRMLLinearTransformNode")
        for i in range(transformNodes.GetNumberOfItems()):
            tNode = transformNodes.GetItemAsObject(i)
            tName = tNode.GetName()
            if tName.startswith("Femur_Fragment") and (tName.endswith("_Transform") or tName.endswith("_RotationTransform")):
                if tNode not in transformsToDelete:
                    transformsToDelete.append(tNode)

        if not nodesToDelete and not transformsToDelete:
            slicer.util.infoDisplay("삭제할 골편, 미러링 모델 또는 변환 노드가 없습니다.")
            return

        confirm = slicer.util.confirmOkCancelDisplay(
            f"장면에서 {len(nodesToDelete)}개의 골편/가이드 모델 및 {len(transformsToDelete)}개의 연관 변환 노드를 정말 삭제하시겠습니까?\n(가이드 모델 선택도 초기화됩니다.)",
            windowTitle="전체 삭제 확인",
        )
        if not confirm:
            # 확인 창에서 취소하면 장면 데이터는 그대로 둔다.
            return

        with slicer.util.tryWithErrorDisplay("모델 노드를 삭제하지 못했습니다.", waitCursor=True):
            for node in nodesToDelete:
                slicer.mrmlScene.RemoveNode(node)
            for tNode in transformsToDelete:
                slicer.mrmlScene.RemoveNode(tNode)

            # 가이드 선택 상태를 None으로 완전 해제
            self.ui.guideModelSelector.setCurrentNode(None)

            slicer.util.forceRenderAllViews()
            self.updateFragmentTable()
            slicer.util.infoDisplay(
                f"모델 노드 {len(nodesToDelete)}개와 연관 변환 노드 {len(transformsToDelete)}개를 삭제했습니다."
            )

    def removeActiveTransformObserver(self) -> None:
        """
        현재 추적 중인 활성 골편의 transform event 옵저버를 안전하게 해제한다.

        선택된 골편이 해제되거나 변경될 때 호출되며, 우측 하단의 수치 표시 패널(Position/Rotation)도
        기본 초기화 상태("없음", "N/A")로 되돌린다.
        """
        if self._activeTransformObserverTag is not None and self._activeTransformNode:
            try:
                # 이전에 선택했던 골편의 transform 이벤트 감시를 해제한다.
                self._activeTransformNode.RemoveObserver(self._activeTransformObserverTag)
            except Exception:
                pass
        self._activeTransformObserverTag = None
        self._activeTransformNode = None
        self._activeModelNode = None

        if hasattr(self, "ui") and hasattr(self.ui, "selectedNameValue"):
            self.ui.selectedNameValue.setText("없음 (표에서 행을 선택하세요)")
            self.ui.positionValue.setText("N/A")
            self.ui.rotationValue.setText("N/A")

    def onFragmentTableSelectionChanged(self) -> None:
        """
        사용자가 표(QTableWidget)에서 골편 행을 선택하면, 해당 골편의 위치/회전 추적 파이프라인을 설정한다.

        - 선택된 모델의 선형 변환 노드(vtkMRMLLinearTransformNode)를 감지하고,
          ModifiedEvent 발생 시 실시간으로 위치와 회전 각도를 계산해주는 옵저버를 신설한다.
        - 만약 변환 노드가 존재하지 않는 경우, 고정된 모델 메쉬 자체의 기하학적 중심 좌표(Bounds Center)만 우선 산출해 표기한다.
        """
        self.removeActiveTransformObserver()

        table = self.ui.fragmentTableWidget
        selectedRanges = table.selectedRanges()
        if not selectedRanges:
            return

        # 단일 셀을 선택해도 해당 셀이 속한 행의 첫 번째 열에서 node ID를 읽는다.
        row = selectedRanges[0].topRow()
        nameItem = table.item(row, 0)
        if not nameItem:
            return

        nodeId = nameItem.data(qt.Qt.UserRole)
        if not nodeId:
            return

        node = slicer.mrmlScene.GetNodeByID(nodeId)
        if not node:
            return

        self._activeModelNode = node
        self.ui.selectedNameValue.setText(node.GetName())

        transformNode = node.GetParentTransformNode()
        if not transformNode:
            poly = node.GetPolyData()
            if poly:
                # transform이 없는 모델은 메쉬 bounds의 중심을 현재 위치처럼 표시한다.
                bounds = [0.0] * 6
                poly.GetBounds(bounds)
                cx = (bounds[0] + bounds[1]) / 2.0
                cy = (bounds[2] + bounds[3]) / 2.0
                cz = (bounds[4] + bounds[5]) / 2.0
                self.ui.positionValue.setText(f"X: {cx:.2f}, Y: {cy:.2f}, Z: {cz:.2f}")
                self.ui.rotationValue.setText("R: 0.00, P: 0.00, Y: 0.00")
            return

        self._activeTransformNode = transformNode
        # 선택된 골편의 transform이 바뀔 때마다 위치/회전 표시를 자동 갱신한다.
        self._activeTransformObserverTag = transformNode.AddObserver(
            vtk.vtkCommand.ModifiedEvent, self.onActiveTransformNodeModified
        )
        self.onActiveTransformNodeModified(transformNode, None)

    def onActiveTransformNodeModified(self, caller, event) -> None:
        """
        선택된 활성 골편의 transform 노드가 변경될 때 호출되는 콜백 핸들러이다.

        [3D 월드 좌표 및 오리엔테이션 각도(Roll/Pitch/Yaw) 동기화 파이프라인]
        1. 변환 노드로부터 최신 3D 월드 변환 행렬(vtkMatrix4x4)을 획득한다.
        2. vtkTransform 객체를 통해 행렬로부터 평행이동(Position) 및 오리엔테이션 각도(Roll/Pitch/Yaw)를 실시간으로 추출한다.
        3. 모델의 기하학적 형상(PolyData)이 유효하다면, 메쉬 자체의 로컬 중심 좌표에 월드 행렬을 적용하여 3차원 월드 공간 상의 실제 물리적 무게 중심(Bounds Center)을 계산한다.
        4. 산출된 X/Y/Z 좌표와 R(Roll)/P(Pitch)/Y(Yaw) 각도를 UI 레이블(`positionValue`, `rotationValue`)에 실시간으로 반영한다.
        """
        if not self._activeTransformNode:
            return

        worldMatrix = vtk.vtkMatrix4x4()
        # World matrix는 부모 transform까지 모두 반영한 최종 화면상 변환이다.
        self._activeTransformNode.GetMatrixTransformToWorld(worldMatrix)

        transform = vtk.vtkTransform()
        transform.SetMatrix(worldMatrix)
        orientation = transform.GetOrientation()

        if self._activeModelNode and self._activeModelNode.GetPolyData():
            poly = self._activeModelNode.GetPolyData()
            # 위치 표시는 모델 자체의 중심을 기준으로 계산한다.
            # 회전은 transform에서, 중심점은 polydata bounds에서 가져온다.
            bounds = [0.0] * 6
            poly.GetBounds(bounds)
            cx = (bounds[0] + bounds[1]) / 2.0
            cy = (bounds[2] + bounds[3]) / 2.0
            cz = (bounds[4] + bounds[5]) / 2.0
            
            # [수정] 로컬 공간 중심점 좌표에 월드 변환 행렬을 적용하여 3차원 월드 공간 상의 실제 물리적 무게 중심 산출
            point_local = [cx, cy, cz, 1.0]
            point_world = [0.0, 0.0, 0.0, 1.0]
            worldMatrix.MultiplyPoint(point_local, point_world)
            position = (point_world[0], point_world[1], point_world[2])
        else:
            position = transform.GetPosition()

        self.ui.positionValue.setText(f"X: {position[0]:.2f}, Y: {position[1]:.2f}, Z: {position[2]:.2f}")
        self.ui.rotationValue.setText(
            f"R: {orientation[0]:.2f}, P: {orientation[1]:.2f}, Y: {orientation[2]:.2f}"
        )
