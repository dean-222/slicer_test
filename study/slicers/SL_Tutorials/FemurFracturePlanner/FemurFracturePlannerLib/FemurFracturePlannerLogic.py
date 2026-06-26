"""
File Name: FemurFracturePlannerLogic.py
Version: v0-2.2.2
Date: 2026-06-25
Description: 대퇴골 골절 계획 모듈의 Volume Rendering, 골편 분리, 모델 정합 계산 로직을 담당한다.

Version History:
- v0-2.2.2 (2026-06-25)
  - 골편 분리 생성 시 컬러 테이블 노드 의존성을 제거하고, 확정적인 고대비(High-contrast) 색상 리스트를 적용하여 골편 간 시각적 구분을 명확히 개선 (Z 증가)
- v0-2.2.1 (2026-06-25)
  - 8단계 골절면 스냅 ICP 연산이 너무 길어지며 뼈가 미끄러지듯 과도하게 겹치는 과적합 현상을 방지하기 위해, 최대 반복 횟수를 50회로 줄이고 평균 오차가 0.5mm 이내로 들어오면 연산을 조기 종료하도록 파라미터 조율 (Z 증가)
- v0-2.2.0 (2026-06-25)
  - 8단계 골절면 스냅(Fracture Surface Snap) 기능에 vtkLandmarkTransform 단일 변환 대신 vtkIterativeClosestPointTransform(ICP) 반복 정복 알고리즘을 이식하여 골절면 굴곡을 정밀하게 맞물리도록 개선 (Y 증가, Z 0 초기화)
- v0-2.1.0 (2026-06-24)
  - 알고리즘에 산재되어 있던 하드코딩 상수들(마스킹 공기값, 렌더링 오프셋, 골편 최소 꼭짓점 수, ICP 반복 수, Snap 기준값 등)을 클래스 최상단 상수로 분리 (Y 증가, Z 0 초기화)
- v0-2.0.0 (2026-06-24)
  - 임계값 미만 영역을 공기(-1000 HU)로 치환(마스킹)한 뒤 3D 엣지를 검출하여 외부 연부조직-공기 경계 노이즈를 필터링하는 detectVolumeEdgesMasked 함수 추가 (X 증가, Y/Z 초기화)
- v0-1.0.3 (2026-06-24)
  - Unsigned 스칼라 형식의 CT 볼륨 강도 반전 시, 음수 HU가 언더플로우되어 소실되는 문제를 방지하기 위해 vtkImageCast를 활용해 signed short 형식으로 변환하는 연산 추가 (Z 증가)
- v0-1.0.2 (2026-06-24)
  - vtkMRMLScalarVolumeDisplayNode에 존재하지 않는 CalculateAutoLevels 호출을 SetAutoWindowLevel(True)로 변경하여 런타임 AttributeError 버그 수정 (Z 증가)
- v0-1.0.1 (2026-06-24)
  - 구문 오류(SyntaxError)를 해결하고 누락된 mirrorModel 기능을 복구하며 전체 주석을 상세화함 (Z 증가)
- v0-1.0.0 (2026-06-23)
  - 영상처리 기능(영상 반전, 3D 엣지 검출) 복원 (X 증가, Y/Z 초기화)
- v0-0.0.1 (2026-06-23)
  - 깨진 주석을 한글로 복구하고 누락된 VTK 객체 초기화 및 표면 스냅 예외 조건을 수정
- v0-0.0.0 (2026-06-23)
  - FemurFracturePlanner.py에서 비즈니스 로직 클래스를 분리하여 최초 작성
"""

import logging
import slicer
import vtk
from slicer import vtkMRMLScalarVolumeNode
from slicer import vtkMRMLSegmentationNode
from slicer.ScriptedLoadableModule import ScriptedLoadableModuleLogic
from slicer.parameterNodeWrapper import parameterNodeWrapper

# Logic 파일은 버튼이나 화면 배치와 분리된 "실제 작업"을 모아 둔다.
# UI에서 호출하더라도, 이 파일의 함수들은 가능한 한 입력 노드와 계산 결과만 다루도록 구성한다.


@parameterNodeWrapper
class FemurFracturePlannerParameterNode:
    """
    모듈 UI 상태를 Slicer MRML parameter node에 저장하기 위한 래퍼 클래스이다.
    
    이 클래스는 Slicer의 Parameter Node 기능을 추상화하여,
    UI의 입력 볼륨 노드 선택 상태 등을 MRML 씬에 영속적으로 바인딩하고 관리할 수 있게 돕니다.
    """
    inputVolume: vtkMRMLScalarVolumeNode


class FemurFracturePlannerLogic(ScriptedLoadableModuleLogic):
    """
    대퇴골 골절 계획 모듈의 핵심 연산 및 알고리즘을 수행하는 로직 클래스이다.
    
    이 클래스는 GUI에 의존하지 않고 독립적으로 동작하며, 주요 연산은 다음과 같다:
    1. CT 볼륨 데이터에 대한 3D 볼륨 렌더링(Volume Rendering) 설정 및 실시간 불투명도(Threshold) 조정
    2. 볼륨 데이터 연산 (Numpy 기반 강도 반전 및 VTK 기반 3D 경계면 검출)
    3. 볼륨 및 연관된 세그멘테이션/모델 노드의 3D 공간 상 회전 연산
    4. 세그멘테이션(Segmentation)으로부터 서로 연결되지 않은 독립적인 뼈 조각(Model 메쉬) 고속 분리
    5. 가이드 모델을 X축 기준으로 미러링(Mirroring)하여 정상 측 뼈 가이드 메쉬 생성
    6. ICP(Iterative Closest Point) 강체 정합 알고리즘을 통한 뼈 조각 자동 위치 정렬
    7. 최근접 점 탐색(KD-Tree) 및 Landmark 강체 변환을 통한 골절 단면 정밀 밀착(Snap) 연산
    """

    # =========================================================================
    # 알고리즘 상수 설정 (Parameters)
    # =========================================================================
    # 1. 영상 처리 파라미터
    MASK_AIR_VALUE = -1000          # 뼈 마스킹 에지 검출 시 연부조직/공기를 치환할 공기 HU 값
    OPACITY_OFFSET_MIN = -100       # 볼륨 렌더링 불투명도 전달 함수의 하한 오프셋
    OPACITY_OFFSET_MID = 200        # 볼륨 렌더링 불투명도 전달 함수의 중간 오프셋
    OPACITY_MAX_HU = 1500           # 볼륨 렌더링이 완전 불투명해지는 최대 HU 값

    # 2. 골편 분리 (Connectivity) 파라미터
    MIN_FRAGMENT_POINTS = 200       # 뼈 조각 분리 시 노이즈로 간주하여 필터링할 최소 정점(Point) 수

    # 3. ICP 강체 정합 파라미터
    ICP_MAX_ITERATIONS = 300        # ICP 알고리즘 최대 반복 계산 횟수
    ICP_MAX_LANDMARKS = 1000        # ICP 계산 시 샘플링할 최대 랜드마크 포인트 수
    ICP_CONVERGENCE_DISTANCE = 0.01 # ICP 알고리즘 수렴 허용 오차 거리 (mm)

    # 4. 골절면 정밀 스냅 (Snap) 파라미터
    SNAP_DISTANCE_LIMIT = 25.0      # 골절 단면 인식을 위한 최대 대응점 탐색 거리 (mm)
    SNAP_MIN_POINTS = 3             # Landmark 강체 변환을 기동하기 위해 필요한 최소 대응점 개수

    def getParameterNode(self):
        """
        현재 씬에서 모듈 전용 parameter node를 생성하거나 가져와 래퍼 객체로 감싸서 반환한다.
        """
        # Slicer 모듈은 parameter node를 사용해 UI 선택값을 씬에 저장한다.
        # 씬을 저장했다가 다시 열어도 어떤 볼륨을 선택했는지 복원할 수 있다.
        return FemurFracturePlannerParameterNode(super().getParameterNode())

    def invertVolumeIntensity(self, volumeNode: vtkMRMLScalarVolumeNode) -> vtkMRMLScalarVolumeNode:
        """
        입력된 스칼라 볼륨의 밝기 값(Intensity)을 반전시켜 반사된 흑백 영상을 얻는 함수이다.
        
        Numpy 배열 연산을 활용하여 슬라이스 전체의 명암을 고속으로 반전한 새 볼륨 노드를 생성한다.
        이 기법은 CT 볼륨의 뼈와 주변 연부조직 간의 대조도를 일시적으로 반전하여 시각적 확인을 돕는다.
        
        Args:
            volumeNode: 반전할 대상 원본 스칼라 볼륨 노드
            
        Returns:
            반전된 데이터가 입력된 새로운 스칼라 볼륨 노드
        """
        if not volumeNode:
            return None

        # 1. 볼륨 복제
        # 원본 CT 데이터를 직접 바꾸지 않고 복제본을 만든다.
        # 사용자가 언제든 원본으로 돌아갈 수 있게 하는 안전장치이다.
        volumesLogic = slicer.modules.volumes.logic()
        clonedVolumeName = f"{volumeNode.GetName()}_Inverted"
        clonedNode = volumesLogic.CloneVolume(slicer.mrmlScene, volumeNode, clonedVolumeName)
        if not clonedNode:
            raise RuntimeError("Failed to clone volume for intensity inversion.")

        # 1.5. Unsigned scalar type인 경우 음수 HU 대입 시 overflow 방지를 위해 signed short로 캐스팅
        image = clonedNode.GetImageData()
        if image and "Unsigned" in image.GetScalarTypeAsString():
            castFilter = vtk.vtkImageCast()
            castFilter.SetInputData(image)
            castFilter.SetOutputScalarTypeToShort()
            castFilter.Update()
            clonedNode.SetAndObserveImageData(castFilter.GetOutput())

        # 2. Numpy Array 획득 및 반전 연산
        import slicer.util as su
        import numpy as np

        volumeArray = su.arrayFromVolume(clonedNode)
        minVal = volumeArray.min()
        maxVal = volumeArray.max()

        # 반전 계산: (최대값 + 최소값) - 현재값
        # 예를 들어 값 범위가 0~100이면 20은 80, 80은 20으로 뒤집힌다.
        invertedArray = (maxVal + minVal) - volumeArray

        # 값을 다시 볼륨에 밀어 넣기
        su.updateVolumeFromArray(clonedNode, invertedArray)

        # 디스플레이 최적화
        displayNode = clonedNode.GetScalarVolumeDisplayNode()
        if displayNode:
            displayNode.SetAutoWindowLevel(True)

        return clonedNode

    def detectVolumeEdges(self, volumeNode: vtkMRMLScalarVolumeNode) -> vtkMRMLScalarVolumeNode:
        """
        VTK vtkImageGradientMagnitude 필터를 사용하여 3D 볼륨 데이터의 경계선(Edge)을 검출하는 함수이다.
        
        이 기능은 인접 픽셀 간의 변화율 크기(Gradient Magnitude)를 계산하여 경계면을 밝게 표시한다.
        뼈의 바깥 윤곽선을 3차원 공간에서 뚜렷하게 식별하고자 할 때 유용하다.
        
        Args:
            volumeNode: 경계를 검출할 대상 원본 스칼라 볼륨 노드
            
        Returns:
            3D 엣지가 검출된 데이터가 입력된 새로운 스칼라 볼륨 노드
        """
        if not volumeNode:
            return None

        # 1. 볼륨 복제
        volumesLogic = slicer.modules.volumes.logic()
        clonedVolumeName = f"{volumeNode.GetName()}_Edge"
        clonedNode = volumesLogic.CloneVolume(slicer.mrmlScene, volumeNode, clonedVolumeName)
        if not clonedNode:
            raise RuntimeError("Failed to clone volume for edge detection.")

        # 2. VTK Gradient Magnitude 필터 적용 (3차원 설정)
        # vtkImageGradientMagnitude는 주변 voxel과의 값 차이를 계산한다.
        # 차이가 큰 곳은 조직 경계일 가능성이 높아 밝게 나타난다.
        gradientFilter = vtk.vtkImageGradientMagnitude()
        gradientFilter.SetInputData(clonedNode.GetImageData())
        gradientFilter.SetDimensionality(3)
        gradientFilter.Update()

        # 3. 이미지 데이터 교체
        clonedNode.SetAndObserveImageData(gradientFilter.GetOutput())

        # 4. 디스플레이 범위 최적화
        displayNode = clonedNode.GetScalarVolumeDisplayNode()
        if displayNode:
            displayNode.SetAutoWindowLevel(True)

        return clonedNode

    def detectVolumeEdgesMasked(self, volumeNode: vtkMRMLScalarVolumeNode, threshold: float) -> vtkMRMLScalarVolumeNode:
        """
        입력된 스칼라 볼륨에서 특정 뼈 임계값(threshold) 미만의 값들을 공기(-1000 HU)로 치환(마스킹)한 뒤,
        VTK vtkImageGradientMagnitude 필터를 사용하여 3D 엣지를 검출하는 함수이다.
        
        이 기법은 근육-공기 경계 등 원하지 않는 외부 경계면에서 급격한 그라디언트가 발생하는 것을 차단하고,
        오직 임계값 이상인 뼈 조직(피질골)의 바깥 윤곽만 3D 엣지로 깨끗하게 획득하기 위한 테스트 필터이다.
        
        Args:
            volumeNode: 경계를 검출할 대상 원본 스칼라 볼륨 노드
            threshold: 뼈 영역을 분별하여 마스킹하기 위한 하운스필드 단위(HU) 기준치
            
        Returns:
            마스킹 전처리 후 3D 엣지가 검출된 데이터가 입력된 새로운 스칼라 볼륨 노드
        """
        if not volumeNode:
            return None

        # 1. 볼륨 복제
        volumesLogic = slicer.modules.volumes.logic()
        clonedVolumeName = f"{volumeNode.GetName()}_MaskedEdge"
        clonedNode = volumesLogic.CloneVolume(slicer.mrmlScene, volumeNode, clonedVolumeName)
        if not clonedNode:
            raise RuntimeError("Failed to clone volume for masked edge detection.")

        # 1.5. Unsigned 스칼라 복셀인 경우를 대비하여, -1000 HU 채우기 연산을 위해 signed short 형식으로 캐스팅
        image = clonedNode.GetImageData()
        if image and "Unsigned" in image.GetScalarTypeAsString():
            castFilter = vtk.vtkImageCast()
            castFilter.SetInputData(image)
            castFilter.SetOutputScalarTypeToShort()
            castFilter.Update()
            clonedNode.SetAndObserveImageData(castFilter.GetOutput())

        # 2. Numpy Array 기반 임계치 미만 영역 마스킹 (치환량: 공기 레벨 설정값)
        import slicer.util as su
        import numpy as np

        volumeArray = su.arrayFromVolume(clonedNode)
        
        # 임계값 미만의 연부조직/공기 영역을 클래스 상수인 공기값으로 통일
        maskedArray = np.where(volumeArray < threshold, self.MASK_AIR_VALUE, volumeArray)
        su.updateVolumeFromArray(clonedNode, maskedArray)

        # 3. VTK Gradient Magnitude 필터 적용 (3차원 설정)
        gradientFilter = vtk.vtkImageGradientMagnitude()
        gradientFilter.SetInputData(clonedNode.GetImageData())
        gradientFilter.SetDimensionality(3)
        gradientFilter.Update()

        # 4. 이미지 데이터 교체
        clonedNode.SetAndObserveImageData(gradientFilter.GetOutput())

        # 5. 디스플레이 범위 최적화
        displayNode = clonedNode.GetScalarVolumeDisplayNode()
        if displayNode:
            displayNode.SetAutoWindowLevel(True)

        return clonedNode

    def showVolumeRendering(self, volumeNode: vtkMRMLScalarVolumeNode, threshold: float) -> None:
        """
        선택한 CT 볼륨에 대하여 3D Volume Rendering Display Node를 생성하고 3D 뷰에 가시화한다.
        
        기존에 생성된 Rendering Display Node가 있다면 재사용하고, 없다면 새로 생성하여 연결한다.
        가시화 스타일은 뼈 구조 표현에 특화된 'CT-Bone' 프리셋을 적용하며, 입력된 threshold를 바탕으로
        뼈의 투명도 전달 함수(Opacity Transfer Function)를 최적화하여 표시한다.
        
        Args:
            volumeNode: 3D 가시화를 수행할 스칼라 볼륨 노드
            threshold: 뼈를 분별하기 위한 하운스필드 단위(HU) 임계값
        """
        volRenLogic = slicer.modules.volumerendering.logic()

        # 이미 이 볼륨에 렌더링 표시 노드가 있으면 재사용하고, 없으면 새로 만든다.
        # DisplayNode는 "어떻게 보일지"를 저장하고, VolumeNode는 "원본 데이터"를 저장한다.
        displayNode = volRenLogic.GetFirstVolumeRenderingDisplayNode(volumeNode)
        if not displayNode:
            displayNode = volRenLogic.CreateVolumeRenderingDisplayNode()
            slicer.mrmlScene.AddNode(displayNode)
            volumeNode.AddAndObserveDisplayNodeID(displayNode.GetID())

        volRenLogic.UpdateDisplayNodeFromVolumeNode(displayNode, volumeNode)
        presetNode = volRenLogic.GetPresetByName("CT-Bone")
        if presetNode:
            # CT-Bone preset은 뼈가 잘 보이도록 색상/불투명도 기본값이 맞춰진 Slicer 내장 설정이다.
            displayNode.GetVolumePropertyNode().Copy(presetNode)

        self.adjustVolumeRenderingThreshold(volumeNode, threshold)
        displayNode.SetVisibility(True)

        slicer.util.resetThreeDViews()
        slicer.util.forceRenderAllViews()

    def hideVolumeRendering(self, volumeNode: vtkMRMLScalarVolumeNode) -> None:
        """
        지정된 스칼라 볼륨 노드에 설정된 3D Volume Rendering Display Node를 제거하여 3D 뷰어 화면에서 숨긴다.
        
        Args:
            volumeNode: Volume Rendering 가시화를 비활성화할 스칼라 볼륨 노드
        """
        volRenLogic = slicer.modules.volumerendering.logic()
        displayNode = volRenLogic.GetFirstVolumeRenderingDisplayNode(volumeNode)
        if displayNode:
            slicer.mrmlScene.RemoveNode(displayNode)

        slicer.util.forceRenderAllViews()

    def updateVolumeRendering(self, volumeNode: vtkMRMLScalarVolumeNode, threshold: float) -> None:
        """
        입력 볼륨 변경 또는 임계값 수치 변경이 발생할 때 볼륨 렌더링 표현을 강제로 갱신한다.
        
        Args:
            volumeNode: 볼륨 노드
            threshold: 갱신할 뼈 임계값
        """
        self.showVolumeRendering(volumeNode, threshold)

    def adjustVolumeRenderingThreshold(self, volumeNode: vtkMRMLScalarVolumeNode, threshold: float) -> None:
        """
        사용자가 조절한 Threshold HU 수치를 바탕으로 렌더링 불투명도 전달 함수(Opacity Function)를 재구성한다.
        
        설정된 임계값보다 낮은 밀도의 조직(근육, 지방 등)은 투명하게 만들어 숨기고, 임계값 이상인 고밀도 영역(뼈)만
        선명하고 점진적으로 불투명하게 표현되도록 4개의 제어점을 정의하여 전달 함수를 갱신한다.
        
        Args:
            volumeNode: 대상 스칼라 볼륨 노드
            threshold: 가시화 기준 HU 임계치
        """
        volRenLogic = slicer.modules.volumerendering.logic()
        displayNode = volRenLogic.GetFirstVolumeRenderingDisplayNode(volumeNode)
        if not displayNode:
            return

        volumePropertyNode = displayNode.GetVolumePropertyNode()
        if not volumePropertyNode:
            return

        # Scalar Opacity Transfer Function 획득 및 재정의
        opacityFunction = volumePropertyNode.GetVolumeProperty().GetScalarOpacity()
        opacityFunction.RemoveAllPoints()
        # 전달 함수는 "CT 값(HU)이 어느 정도일 때 얼마나 불투명하게 보일지"를 정하는 그래프이다.
        # 아래 네 점을 연결한 곡선을 통해 낮은 값은 투명하게, 뼈에 가까운 높은 값은 진하게 보이게 한다.
        # 임계치 + 하한 오프셋 이하: 완전히 투명 (0.0)
        opacityFunction.AddPoint(threshold + self.OPACITY_OFFSET_MIN, 0.0)
        # 임계치 부근: 약간 반투명 (0.15)
        opacityFunction.AddPoint(threshold, 0.15)
        # 임계치 + 중간 오프셋 이상: 불투명화 시작 (0.75)
        opacityFunction.AddPoint(threshold + self.OPACITY_OFFSET_MID, 0.75)
        # 완전 불투명 기준 HU 이상: 완전 불투명에 가까움 (0.85)
        opacityFunction.AddPoint(self.OPACITY_MAX_HU, 0.85)

        displayNode.Modified()

    def rotateVolume(self, volumeNode: vtkMRMLScalarVolumeNode, axis: str, angle: float) -> None:
        """
        지정된 스칼라 볼륨 노드와 이 볼륨에 동기화된 세그멘테이션, 추출된 골편 모델들을 동시에 회전시키는 함수이다.
        
        메쉬 정점의 좌표 데이터를 직접 수정하지 않고, 부모 선형 변환 노드(vtkMRMLLinearTransformNode)를 생성하여 
        볼륨, 세그멘테이션 및 분리된 뼈 파편(Model) 노드들을 이 변환 노드에 종속(Parent-Child 구조)시킨 뒤 
        변환 노드의 로컬 4x4 행렬을 제어한다. 이를 통해 하위 노드들의 상대적 위치 관계를 그대로 유지하며 고속 회전한다.
        
        Args:
            volumeNode: 회전의 중심 기준이 될 스칼라 볼륨 노드
            axis: 회전축 ("X", "Y", "Z" 중 하나)
            angle: 회전할 각도 (단위: Degree, 범위: -180 ~ +180)
        """
        if not volumeNode:
            return

        transformNode = volumeNode.GetParentTransformNode()

        # 부모 변환 노드가 존재하지 않거나 선형 변환 노드가 아닌 경우 새로 생성하여 연결
        # Slicer에서는 실제 이미지 voxel을 매번 다시 계산하지 않고,
        # transform node를 부모로 붙여 화면상 위치와 회전을 바꾸는 방식이 빠르고 안전하다.
        if not transformNode or not transformNode.IsA("vtkMRMLLinearTransformNode"):
            transformNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
            transformNode.SetName(f"{volumeNode.GetName()}_RotationTransform")
            volumeNode.SetAndObserveTransformNodeID(transformNode.GetID())
            logging.info(f"Created new linear transform node for '{volumeNode.GetName()}'.")

        # 씬 안의 모든 세그멘테이션 노드도 동일한 변환 노드를 바라보도록 동기화
        # CT 볼륨만 회전하면 segmentation과 위치가 어긋난다.
        # 같은 transform을 공유하게 만들어 볼륨, segmentation, 골편 모델이 함께 움직이게 한다.
        segmentationNodes = slicer.mrmlScene.GetNodesByClass("vtkMRMLSegmentationNode")
        for i in range(segmentationNodes.GetNumberOfItems()):
            segNode = segmentationNodes.GetItemAsObject(i)
            if segNode.GetParentTransformNode() != transformNode:
                segNode.SetAndObserveTransformNodeID(transformNode.GetID())

        # 분리된 골편 모델(이름이 'Femur_Fragment'로 시작)들도 이 변환 노드의 자식으로 종속
        modelNodes = slicer.mrmlScene.GetNodesByClass("vtkMRMLModelNode")
        for i in range(modelNodes.GetNumberOfItems()):
            modelNode = modelNodes.GetItemAsObject(i)
            if modelNode.GetName().startswith("Femur_Fragment") and modelNode.GetParentTransformNode() != transformNode:
                modelNode.SetAndObserveTransformNodeID(transformNode.GetID())

        # 회전 변환 행렬 구성
        # vtkTransform은 사람이 이해하기 쉬운 회전 명령을 4x4 행렬로 바꿔 주는 도구이다.
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
        logging.debug(f"Applied rotation of {angle:.1f} degrees along {axis} axis to volume and child nodes.")

    def separateBoneFragments(
        self,
        segmentationNode: vtkMRMLSegmentationNode,
        referenceVolumeNode: vtkMRMLScalarVolumeNode = None,
    ) -> int:
        """
        [세그멘테이션 데이터를 분석하여 독립된 3D 뼈 파편(Model) 메쉬를 고속 분리하는 알고리즘]
        
        1. Slicer 내장의 Closed Surface 표현형을 재생성하여 세그멘테이션의 메쉬 구조를 얻어낸다.
        2. 화면에 보이는(가시성이 켜진) 세그먼트들의 모든 메쉬 데이터를 수집하여 하나의 메쉬로 병합(vtkAppendPolyData)한다.
        3. VTK Connectivity Filter를 구동하여 공간상으로 물리적으로 끊겨 있는 분리 영역(Region)들을 탐지한다.
        4. 탐지된 독립 영역들을 꼭짓점(Point) 개수 기준으로 정렬하여 가장 큰 조각 2개(예: 골절된 대퇴골 근위부/원위부)를 뼈로 추출한다.
        5. 꼭짓점 개수가 200개 이하인 자잘한 조각(세그멘테이션 노이즈 등)은 필터링하여 불필요한 메쉬 생성을 억제한다.
        6. 분리된 조각들을 각각 'Femur_Fragment_1', 'Femur_Fragment_2'와 같은 독립적인 3D 모델 노드로 신설하고 
           사용자가 쉽게 구분하도록 무지개 테이블에서 색상을 추출하여 다르게 배정한다.
        
        Args:
            segmentationNode: 분리 작업을 수행할 소스 세그멘테이션 노드
            referenceVolumeNode: 참조 스칼라 볼륨 노드 (옵션)
            
        Returns:
            성공적으로 생성된 독립 뼈 파편(Model) 노드의 개수
        """
        if not segmentationNode:
            raise RuntimeError("세그멘테이션 노드가 선택되지 않았습니다.")

        segmentation = segmentationNode.GetSegmentation()
        displayNode = segmentationNode.GetDisplayNode()

        # 세그먼트 생성 확인 사전 체크
        if not segmentation or segmentation.GetNumberOfSegments() == 0:
            raise RuntimeError("세그멘테이션에 segment가 없습니다. 먼저 segment를 만들고 뼈 영역을 칠하세요.")

        if not displayNode:
            raise RuntimeError("세그멘테이션 표시 노드가 없습니다.")

        # 가시성이 확보된(체크된) 세그먼트의 ID 리스트 수집
        # 사용자가 눈 아이콘으로 숨긴 segment는 분리 대상에서 제외한다.
        # 화면에 보이는 뼈 영역만 처리하면 의도하지 않은 잡음 segment를 줄일 수 있다.
        visibleSegmentIds = vtk.vtkStringArray()
        displayNode.GetVisibleSegmentIDs(visibleSegmentIds)
        if visibleSegmentIds.GetNumberOfValues() == 0:
            raise RuntimeError("표시 중인 segment가 없습니다. 분리하려면 segment의 가시성(눈 아이콘)을 켜세요.")

        # 3D Closed Surface 표현형이 없으면 강제로 계산 및 생성
        segmentationNode.CreateClosedSurfaceRepresentation()

        appendFilter = vtk.vtkAppendPolyData()
        hasInput = False

        # 가시적인 모든 세그먼트의 표면 메쉬를 하나로 합침
        # 여러 segment에 나뉘어 칠해진 뼈도 한 덩어리 데이터로 합쳐야
        # Connectivity Filter가 실제로 분리된 3D 조각을 찾을 수 있다.
        segmentationsLogic = slicer.modules.segmentations.logic()
        for i in range(visibleSegmentIds.GetNumberOfValues()):
            segId = visibleSegmentIds.GetValue(i)
            segmentPolyData = vtk.vtkPolyData()
            segmentationsLogic.GetSegmentClosedSurfaceRepresentation(segmentationNode, segId, segmentPolyData)
            if segmentPolyData and segmentPolyData.GetNumberOfPoints() > 0:
                appendFilter.AddInputData(segmentPolyData)
                hasInput = True

        if not hasInput:
            raise RuntimeError("표시 중인 segment에서 유효한 형상을 찾지 못했습니다. 비어 있지 않은지 확인하세요.")

        appendFilter.Update()
        mergedPolyData = appendFilter.GetOutput()

        # 연결성 분석 필터를 실행하여 3D 공간 상 분리된 메쉬 영역 추출
        # Connectivity Filter는 삼각형 메쉬가 서로 붙어 있는지 검사해서
        # 공간적으로 떨어진 영역을 region 번호별로 나눠 준다.
        connectivityFilter = vtk.vtkPolyDataConnectivityFilter()
        connectivityFilter.SetInputData(mergedPolyData)
        connectivityFilter.SetExtractionModeToAllRegions()
        connectivityFilter.Update()

        numRegions = connectivityFilter.GetNumberOfExtractedRegions()
        regionSizes = connectivityFilter.GetRegionSizes()
        logging.info(f"Total disconnected regions detected: {numRegions}")

        transformNode = segmentationNode.GetParentTransformNode()
        separatedCount = 0

        # 각 영역의 인덱스와 정점 수를 쌍으로 묶어 리스트 생성
        regionInfos = []
        if regionSizes:
            for i in range(numRegions):
                if i < regionSizes.GetNumberOfValues():
                    size = regionSizes.GetValue(i)
                    regionInfos.append((i, size))
        else:
            for i in range(numRegions):
                regionInfos.append((i, 0))

        # 정점 개수를 기준으로 내림차순 정렬하여 가장 유의미한 상위 2개 조각 식별
        # 작은 region은 노이즈나 페인팅 찌꺼기일 가능성이 높으므로 큰 조각부터 사용한다.
        regionInfos.sort(key=lambda x: x[1], reverse=True)
        targetRegions = regionInfos[: min(2, len(regionInfos))]
        logging.info(f"Top 2 largest disconnected regions selected: {targetRegions}")

        for idx, (regionId, size) in enumerate(targetRegions):
            # 미세 잔해나 노이즈는 무시 (최소 정점 기준 필터링)
            if size <= self.MIN_FRAGMENT_POINTS:
                continue

            singleRegionFilter = vtk.vtkPolyDataConnectivityFilter()
            singleRegionFilter.SetInputData(mergedPolyData)
            singleRegionFilter.SetExtractionModeToSpecifiedRegions()
            singleRegionFilter.AddSpecifiedRegion(regionId)
            singleRegionFilter.Update()

            regionPolyData = singleRegionFilter.GetOutput()

            if regionPolyData.GetNumberOfPoints() > self.MIN_FRAGMENT_POINTS:
                fragModel = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLModelNode")
                fragModel.SetName(f"Femur_Fragment_{idx+1}")
                fragModel.SetAndObservePolyData(regionPolyData)

                # 부모 변환 노드가 존재하면 자식 관계를 맺어 동기화
                if transformNode:
                    fragModel.SetAndObserveTransformNodeID(transformNode.GetID())

                # 서로 뚜렷하게 구분되는 고대비 색상 (R, G, B) 사전 정의
                distinctColors = [
                    (0.3, 0.9, 0.5),  # 연녹색 (Fragment 1)
                    (0.9, 0.6, 0.2),  # 주황색 (Fragment 2)
                    (0.4, 0.6, 0.9),  # 파란색 (Fragment 3)
                    (0.9, 0.4, 0.7),  # 핑크색
                    (0.8, 0.8, 0.2)   # 노란색
                ]
                color = distinctColors[idx % len(distinctColors)]

                displayNode = fragModel.GetDisplayNode()
                if not displayNode:
                    fragModel.CreateDefaultDisplayNodes()
                    displayNode = fragModel.GetDisplayNode()
                if displayNode:
                    displayNode.SetColor(color[0], color[1], color[2])

                separatedCount += 1

        return separatedCount

    def mirrorModel(self, modelNode) -> slicer.vtkMRMLModelNode:
        """
        선택한 가이드 모델을 X축 기준으로 좌우 대칭(Scaling -1, 1, 1) 처리한 미러링 모델을 생성한다.
        
        대퇴골 수술 계획 시, 건강한 측(Contralateral) 대퇴골 3D 모델을 불러와 미러링하여
        골절된 부위의 복원 가이드 템플릿 모델로 활용한다.
        반전 연산 시 폴리곤의 면 방향(Normals & Cell wind direction)이 뒤집히므로 vtkReverseSense 필터로 보정한다.
        
        Args:
            modelNode: 원본 뼈 모델 노드 (예: 건강한 측 대퇴골 모델)
            
        Returns:
            대칭 미러링 처리가 완료되어 씬에 추가된 새로운 3D 모델 노드
        """
        if not modelNode or not modelNode.GetPolyData():
            return None

        polyData = modelNode.GetPolyData()

        # 1. X축 반전 변환 적용 (-1.0, 1.0, 1.0)
        # Scale의 X 값만 -1로 주면 좌우가 거울처럼 뒤집힌다.
        # 좌우 대칭 뼈를 수술 가이드로 쓸 때 자주 사용하는 방식이다.
        transform = vtk.vtkTransform()
        transform.Scale(-1.0, 1.0, 1.0)

        transformFilter = vtk.vtkTransformPolyDataFilter()
        transformFilter.SetInputData(polyData)
        transformFilter.SetTransform(transform)
        transformFilter.Update()

        # 2. 미러링으로 뒤집힌 메쉬 법선(Normal) 및 셀 감김(Winding) 방향 교정
        # 좌우 반전 후에는 삼각형 면의 앞/뒤 방향이 뒤집힐 수 있다.
        # vtkReverseSense로 법선과 셀 방향을 되돌려 조명과 표면 표시가 자연스럽게 보이게 한다.
        reverseFilter = vtk.vtkReverseSense()
        reverseFilter.SetInputData(transformFilter.GetOutput())
        reverseFilter.ReverseCellsOn()
        reverseFilter.ReverseNormalsOn()
        reverseFilter.Update()

        # 3. 새로운 모델 노드 생성 및 이름 설정
        mirroredNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLModelNode")
        mirroredNode.SetName(f"{modelNode.GetName()}_Mirrored")
        mirroredNode.SetAndObservePolyData(reverseFilter.GetOutput())

        # 4. 반투명 표시 및 구분하기 쉬운 색상으로 기본 디스플레이 설정
        mirroredNode.CreateDefaultDisplayNodes()
        displayNode = mirroredNode.GetDisplayNode()
        if displayNode:
            displayNode.SetOpacity(0.6)
            displayNode.SetColor(0.2, 0.6, 1.0)  # 산뜻한 하늘색 톤

        return mirroredNode

    def runIcpRegistration(self, fragmentModelNodes: list, guideModelNode) -> None:
        """
        [가이드 모델 기반 뼈 조각 자동 ICP(Iterative Closest Point) 정합 알고리즘]
        
        각 뼈 파편(Model) 메쉬를 참조 수술 가이드 뼈 표면에 정렬하기 위해, 
        메쉬 고유 포인트들을 왜곡하지 않고 각 파편의 부모 변환 노드(vtkMRMLLinearTransformNode)의 
        4x4 로컬 아핀 행렬을 찾아 갱신한다.
        
        수학적 연산 흐름:
        1. 가이드 모델 및 대상 뼈 파편에 적용된 누적 월드 변환 행렬을 각각 적용하여 두 메쉬의 포인트들을 월드 좌표계 공간으로 도출한다.
        2. VTK Iterative Closest Point Transform(vtkIterativeClosestPointTransform)에 두 월드 좌표 메쉬를 로드한다.
        3. 회전 및 평행이동만 허용하는 RigidBody(강체) 모드로 설정하고, 최대 반복 횟수(300회) 및 샘플 랜드마크 수(1000개)를 넓혀 정밀도를 고도화한다.
        4. 사용자가 UI 상에서 사전에 맞춰둔 수동 배치 의도를 지키기 위해 Centroid 매칭(StartByMatchingCentroids) 기능은 강제 비활성화한다.
        5. 결과로 획득된 델타 ICP 변환 행렬(M_delta)과 기존 월드 행렬(W_current)을 행렬곱 연산(W_new = M_delta * W_current)한다.
        6. 만약 파편에 상위 부모 변환 노드가 존재한다면, 역행렬 곱 연산(L_new = W_parent_inv * W_new)을 거쳐 
           부모 좌표계 기준의 안전한 로컬 행렬을 도출 및 적용함으로써, Slicer 계층 구조 왜곡 현상을 방지한다.
        
        Args:
            fragmentModelNodes: 정합을 수행할 Femur_Fragment_* 모델 노드 목록
            guideModelNode: 표면 밀착의 기준이 되는 미러링 또는 정상 대퇴골 가이드 모델 노드
        """
        if not guideModelNode or not guideModelNode.GetPolyData():
            raise RuntimeError("가이드 모델 노드가 올바르지 않습니다.")

        guidePolyData = guideModelNode.GetPolyData()

        # 가이드 모델의 월드 좌표계 변환을 계산하여 가이드 메쉬 준비
        # ICP는 두 메쉬가 같은 좌표계에 있다고 가정한다.
        # 따라서 부모 transform이 있으면 먼저 월드 좌표로 변환한 복사본을 만든다.
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

            transformNode = fragNode.GetParentTransformNode()
            # 로컬 이동/회전을 관리할 전용 변환 노드가 없으면 새로 신설
            if not transformNode:
                transformNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
                transformNode.SetName(f"{fragNode.GetName()}_Transform")
                fragNode.SetAndObserveTransformNodeID(transformNode.GetID())

            currentPoly = fragNode.GetPolyData()

            # 현재 파편의 누적 월드 변환 행렬 획득
            # transform node에 저장된 행렬은 화면상 이동/회전 값을 담고 있다.
            # 이 값을 메쉬에 적용해야 ICP가 사용자가 보는 위치 기준으로 계산된다.
            worldMatrix = vtk.vtkMatrix4x4()
            transformNode.GetMatrixTransformToWorld(worldMatrix)

            transform = vtk.vtkTransform()
            transform.SetMatrix(worldMatrix)

            # 월드 좌표 공간의 파편 메쉬 포인트들 생성
            tfFilter = vtk.vtkTransformPolyDataFilter()
            tfFilter.SetInputData(currentPoly)
            tfFilter.SetTransform(transform)
            tfFilter.Update()
            sourceWorldPoly = tfFilter.GetOutput()

            # ICP 정합 설정 및 구동
            # ICP는 source의 각 점이 target 표면에 가까워지도록 회전/이동을 반복해서 찾는 알고리즘이다.
            # 여기서는 뼈 크기를 바꾸지 않도록 RigidBody 모드, 즉 회전과 평행이동만 허용한다.
            icp = vtk.vtkIterativeClosestPointTransform()
            icp.SetSource(sourceWorldPoly)
            icp.SetTarget(guidePolyData)
            icp.GetLandmarkTransform().SetModeToRigidBody()  # 회전/이동만 허용
            icp.SetMaximumNumberOfIterations(self.ICP_MAX_ITERATIONS)
            icp.SetMaximumNumberOfLandmarks(self.ICP_MAX_LANDMARKS)
            icp.CheckMeanDistanceOn()
            icp.SetMaximumMeanDistance(self.ICP_CONVERGENCE_DISTANCE)  # 수렴 오차 거리 설정
            icp.StartByMatchingCentroidsOff()  # 임의 중심 정렬 끄기
            icp.Update()

            # 계산된 델타 행렬
            icpMatrix = icp.GetMatrix()

            # 신규 월드 변환 행렬 계산
            newWorldMatrix = vtk.vtkMatrix4x4()
            vtk.vtkMatrix4x4.Multiply4x4(icpMatrix, worldMatrix, newWorldMatrix)

            # 부모 변환 노드가 있다면 역행렬 처리를 하여 로컬 변환 계산
            # Slicer의 transform은 부모-자식 계층을 가질 수 있다.
            # 부모 transform이 있으면 월드 행렬을 그대로 넣지 말고 부모 기준 로컬 행렬로 바꿔야 한다.
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

            # 새로운 선형 변환 대입
            transformNode.SetMatrixTransformToParent(newLocalMatrix)

            # ICP가 정상 정비되면 수동 조절 핸들(3D Gizmo)은 시야 확보를 위해 자동으로 숨김
            displayNode = transformNode.GetDisplayNode()
            if displayNode:
                displayNode.SetEditorVisibility(False)

            fragNode.Modified()

    def runFractureSurfaceSnap(self, frag1Node, frag2Node) -> None:
        """
        [두 뼈 조각의 절단면 간 최근접 점 매칭 기반 정밀 맞물림(Fracture Surface Snap) 알고리즘]
        
        골절된 두 뼈 파편(Femur_Fragment_1, Femur_Fragment_2)의 끊어진 경계 단면(Fracture Surface)을 인지하여 
        단면과 단면끼리 빈틈없이 강체 정렬해 결합하도록 유도한다.
        
        작동 연산 과정:
        1. 1번 파편(움직일 대상)과 2번 파편(고정 기준)의 포인트 클라우드를 월드 좌표계로 일차 변환하여 기하학적 정렬 위치를 일치시킨다.
        2. 공간 포인트 인덱싱 가속 수단인 KD-Tree(vtkKdTreePointLocator) 객체를 생성하고 2번 파편의 월드 메쉬 데이터를 입력한다.
        3. 1번 파편의 모든 정점(Point)에 대해 KD-Tree 상에서 2번 파편에 수렴하는 최근접 점(Closest Point)을 초고속으로 색인(O(log N))한다.
        4. 두 대응점 사이의 유클리드 거리를 연산하여 25.0mm 이내에 속한 쌍들만 '골절 단면 인근 영역'으로 정의하고 Landmark 후보군으로 선별한다.
        5. 필터링된 대량의 대응점 집합(sourcePoints, targetPoints)을 기반으로 최소제곱법 기반 강체 변환인 vtkLandmarkTransform을 기동한다.
        6. 계산된 4x4 변환 행렬을 기반으로 1번 파편의 Parent Transform Matrix를 갱신한다. (부모 상속 구조 보정 포함)
        
        Args:
            frag1Node: 움직여서 밀착시킬 대상 뼈 모델 노드 (주로 Femur_Fragment_1)
            frag2Node: 위치가 고정되어 기준이 되는 뼈 모델 노드 (주로 Femur_Fragment_2)
        """
        if not frag1Node or not frag1Node.GetPolyData() or not frag2Node or not frag2Node.GetPolyData():
            raise RuntimeError("유효하지 않은 골편 모델 노드입니다.")

        tNode1 = frag1Node.GetParentTransformNode()
        tNode2 = frag2Node.GetParentTransformNode()

        # 각 변환 노드가 존재하지 않는 경우 자동으로 신설
        # 골편 모델은 transform node를 통해 위치가 바뀌므로,
        # 없을 때는 새로 만들어 이후 계산 결과를 저장할 곳을 마련한다.
        if not tNode1:
            tNode1 = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
            tNode1.SetName(f"{frag1Node.GetName()}_Transform")
            frag1Node.SetAndObserveTransformNodeID(tNode1.GetID())
        if not tNode2:
            tNode2 = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
            tNode2.SetName(f"{frag2Node.GetName()}_Transform")
            frag2Node.SetAndObserveTransformNodeID(tNode2.GetID())

        # 1번 파편의 현재 월드 변환 행렬 구동
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

        # 2번 파편의 현재 월드 변환 행렬 구동
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

        poly1 = frag1Node.GetPolyData()
        poly2 = frag2Node.GetPolyData()

        # 1번 파편 메쉬를 월드 좌표 공간으로 강제 변환
        tf1 = vtk.vtkTransform()
        tf1.SetMatrix(wMat1)
        tfFilter1 = vtk.vtkTransformPolyDataFilter()
        tfFilter1.SetInputData(poly1)
        tfFilter1.SetTransform(tf1)
        tfFilter1.Update()
        poly1World = tfFilter1.GetOutput()

        # 2번 파편 메쉬를 월드 좌표 공간으로 강제 변환
        tf2 = vtk.vtkTransform()
        tf2.SetMatrix(wMat2)
        tfFilter2 = vtk.vtkTransformPolyDataFilter()
        tfFilter2.SetInputData(poly2)
        tfFilter2.SetTransform(tf2)
        tfFilter2.Update()
        poly2World = tfFilter2.GetOutput()

        # 2번 파편 월드 포인트를 KD-Tree에 적재하여 고속 검색 인덱스 준비
        # KD-Tree는 "이 점과 가장 가까운 점이 무엇인가"를 빠르게 찾기 위한 공간 인덱스이다.
        # 모든 점을 매번 전부 비교하는 것보다 큰 메쉬에서 훨씬 효율적이다.
        locator = vtk.vtkKdTreePointLocator()
        locator.SetDataSet(poly2World)
        locator.BuildLocator()

        # 최근접 거리 임계값에 포함되는 대응 랜드마크 점 저장소
        sourcePoints = vtk.vtkPoints()
        targetPoints = vtk.vtkPoints()

        import math
        numPoints1 = poly1World.GetNumberOfPoints()

        # 1번 파편의 모든 포인트에 대응되는 2번 파편 점과의 거리 조사
        for i in range(numPoints1):
            pt1 = poly1World.GetPoint(i)
            closestPointId = locator.FindClosestPoint(pt1)
            if closestPointId < 0:
                continue
            pt2 = poly2World.GetPoint(closestPointId)

            dist = math.sqrt(
                (pt1[0] - pt2[0]) ** 2
                + (pt1[1] - pt2[1]) ** 2
                + (pt1[2] - pt2[2]) ** 2
            )

            # 임계 거리 이내의 점들만 골절면 결합 대상 랜드마크로 등록
            if dist <= self.SNAP_DISTANCE_LIMIT:
                sourcePoints.InsertNextPoint(pt1)
                targetPoints.InsertNextPoint(pt2)

        # 랜드마크 매칭을 수행하기 위해서는 기하학적 최소 자유도가 충족되어야 함
        if sourcePoints.GetNumberOfPoints() < self.SNAP_MIN_POINTS:
            raise RuntimeError(
                f"가까운 골절면 점이 충분하지 않습니다. Move 기능으로 두 골편의 골절면을 {self.SNAP_DISTANCE_LIMIT}mm 이내로 더 가깝게 배치한 뒤 다시 실행하세요."
            )

        # 수집된 대량 점쌍 집합을 vtkPolyData 3D 객체 구조로 래핑
        # vtkIterativeClosestPointTransform(ICP)는 점 구름 기반 반복 수렴을 위해 PolyData 입력을 요구한다.
        sourcePoly = vtk.vtkPolyData()
        sourcePoly.SetPoints(sourcePoints)

        targetPoly = vtk.vtkPolyData()
        targetPoly.SetPoints(targetPoints)

        # 반복적 최근접점(ICP) 정복 알고리즘 구동
        # 단 1회 계산이 아닌 수백 번의 점 매칭 반복(Iteration)을 통해 골절 단면의 오목/볼록 굴곡을 정밀하게 맞춘다.
        icpTransform = vtk.vtkIterativeClosestPointTransform()
        icpTransform.SetSource(sourcePoly)
        icpTransform.SetTarget(targetPoly)
        icpTransform.GetLandmarkTransform().SetModeToRigidBody()  # 오직 회전 및 평행이동만 수행
        icpTransform.SetMaximumNumberOfIterations(50)  # 과도한 슬라이딩 방지를 위해 최대 반복 횟수 축소
        icpTransform.CheckMeanDistanceOn()  # 일정 수치 이하로 붙으면 조기 종료 활성화
        icpTransform.SetMaximumMeanDistance(0.5)  # 평균 오차가 0.5mm에 도달하면 즉시 연산 멈춤
        icpTransform.StartByMatchingCentroidsOn()  # 무게중심부터 맞춰서 로컬 미니멈(최적화 함정) 회피 및 수렴 가속
        icpTransform.Update()

        snapMatrix = icpTransform.GetMatrix()

        # 새로운 1번 파편 월드 행렬 도출 (W1_new = Snap_Matrix * W1)
        newWorldMatrix1 = vtk.vtkMatrix4x4()
        vtk.vtkMatrix4x4.Multiply4x4(snapMatrix, wMat1, newWorldMatrix1)

        # 상위 계층 회전 변환이 있는 경우 로컬 좌표 행렬로 전환 처리
        if parent1:
            pwMatInverse1 = vtk.vtkMatrix4x4()
            vtk.vtkMatrix4x4.Invert(pwMat1, pwMatInverse1)
            newLocalMatrix1 = vtk.vtkMatrix4x4()
            vtk.vtkMatrix4x4.Multiply4x4(pwMatInverse1, newWorldMatrix1, newLocalMatrix1)
        else:
            newLocalMatrix1 = newWorldMatrix1

        # 1번 파편의 변환 노드 갱신
        tNode1.SetMatrixTransformToParent(newLocalMatrix1)
        frag1Node.Modified()

    def runLandmarkRegistration(self, sourceNode, targetNode, sourceMarkersNode, targetMarkersNode) -> None:
        """
        [수동 타겟팅: 마커 기반 정밀 매칭 (점 찍기)]
        사용자가 지정한 Fiducial 포인트들을 바탕으로 vtkLandmarkTransform을 기동하여 골편을 변환한다.
        """
        if not sourceNode or not targetNode or not sourceMarkersNode or not targetMarkersNode:
            raise RuntimeError("마커 기반 매칭에 필요한 노드가 부족합니다.")

        numPoints = min(sourceMarkersNode.GetNumberOfControlPoints(), targetMarkersNode.GetNumberOfControlPoints())
        
        sourcePoints = vtk.vtkPoints()
        targetPoints = vtk.vtkPoints()
        
        for i in range(numPoints):
            p1 = [0.0, 0.0, 0.0]
            p2 = [0.0, 0.0, 0.0]
            sourceMarkersNode.GetNthControlPointPositionWorld(i, p1)
            targetMarkersNode.GetNthControlPointPositionWorld(i, p2)
            sourcePoints.InsertNextPoint(p1)
            targetPoints.InsertNextPoint(p2)
            
        landmarkTransform = vtk.vtkLandmarkTransform()
        landmarkTransform.SetSourceLandmarks(sourcePoints)
        landmarkTransform.SetTargetLandmarks(targetPoints)
        landmarkTransform.SetModeToRigidBody()
        landmarkTransform.Update()
        
        snapMatrix = landmarkTransform.GetMatrix()
        
        tNodeSource = sourceNode.GetParentTransformNode()
        if not tNodeSource:
            tNodeSource = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
            tNodeSource.SetName(f"{sourceNode.GetName()}_Transform")
            sourceNode.SetAndObserveTransformNodeID(tNodeSource.GetID())
            
        lMatSource = vtk.vtkMatrix4x4()
        tNodeSource.GetMatrixTransformToParent(lMatSource)
        parentSource = tNodeSource.GetParentTransformNode()
        wMatSource = vtk.vtkMatrix4x4()
        
        if parentSource:
            pwMatSource = vtk.vtkMatrix4x4()
            parentSource.GetMatrixTransformToWorld(pwMatSource)
            vtk.vtkMatrix4x4.Multiply4x4(pwMatSource, lMatSource, wMatSource)
        else:
            wMatSource.DeepCopy(lMatSource)
            
        newWorldMatrixSource = vtk.vtkMatrix4x4()
        vtk.vtkMatrix4x4.Multiply4x4(snapMatrix, wMatSource, newWorldMatrixSource)
        
        if parentSource:
            pwMatInverseSource = vtk.vtkMatrix4x4()
            vtk.vtkMatrix4x4.Invert(pwMatSource, pwMatInverseSource)
            newLocalMatrixSource = vtk.vtkMatrix4x4()
            vtk.vtkMatrix4x4.Multiply4x4(pwMatInverseSource, newWorldMatrixSource, newLocalMatrixSource)
        else:
            newLocalMatrixSource = newWorldMatrixSource
            
        tNodeSource.SetMatrixTransformToParent(newLocalMatrixSource)
        sourceNode.Modified()

    def runMaskedIcpRegistration(self, sourceNode, targetNode, segNode, sourceSegId, targetSegId) -> None:
        """
        [수동 타겟팅: 색칠 영역 기반 매칭 (마스킹)]
        사용자가 칠한 세그멘테이션 마스크 영역을 추출하여, 두 추출된 폴리데이터(Point Cloud)끼리 ICP 정렬을 수행한다.
        """
        if not sourceNode or not targetNode or not segNode:
            raise RuntimeError("색칠 영역 기반 매칭에 필요한 노드가 부족합니다.")
            
        # 세그멘테이션 데이터에서 지정된 Segment를 PolyData로 추출
        segmentation = segNode.GetSegmentation()
        
        segmentation.CreateRepresentation("Closed surface")
        
        sourcePolyData = segmentation.GetSegmentRepresentation(sourceSegId, "Closed surface")
        targetPolyData = segmentation.GetSegmentRepresentation(targetSegId, "Closed surface")
        
        if not sourcePolyData or not targetPolyData:
            raise RuntimeError("색칠 영역 표면 데이터를 추출할 수 없습니다. 색칠을 했는지 확인하세요.")
            
        sourcePoly = vtk.vtkPolyData.SafeDownCast(sourcePolyData)
        targetPoly = vtk.vtkPolyData.SafeDownCast(targetPolyData)
        
        if not sourcePoly or not targetPoly or sourcePoly.GetNumberOfPoints() == 0 or targetPoly.GetNumberOfPoints() == 0:
            raise RuntimeError("색칠된 영역(Targeting_Source_ROI, Targeting_Target_ROI)에 유효한 데이터가 없습니다. 브러시로 칠했는지 확인하세요.")
            
        icpTransform = vtk.vtkIterativeClosestPointTransform()
        icpTransform.SetSource(sourcePoly)
        icpTransform.SetTarget(targetPoly)
        icpTransform.GetLandmarkTransform().SetModeToRigidBody()
        icpTransform.SetMaximumNumberOfIterations(100)
        icpTransform.CheckMeanDistanceOn()
        icpTransform.SetMaximumMeanDistance(0.5)
        icpTransform.StartByMatchingCentroidsOn()
        icpTransform.Update()
        
        snapMatrix = icpTransform.GetMatrix()
        
        tNodeSource = sourceNode.GetParentTransformNode()
        if not tNodeSource:
            tNodeSource = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLinearTransformNode")
            tNodeSource.SetName(f"{sourceNode.GetName()}_Transform")
            sourceNode.SetAndObserveTransformNodeID(tNodeSource.GetID())
            
        lMatSource = vtk.vtkMatrix4x4()
        tNodeSource.GetMatrixTransformToParent(lMatSource)
        parentSource = tNodeSource.GetParentTransformNode()
        wMatSource = vtk.vtkMatrix4x4()
        
        if parentSource:
            pwMatSource = vtk.vtkMatrix4x4()
            parentSource.GetMatrixTransformToWorld(pwMatSource)
            vtk.vtkMatrix4x4.Multiply4x4(pwMatSource, lMatSource, wMatSource)
        else:
            wMatSource.DeepCopy(lMatSource)
            
        newWorldMatrixSource = vtk.vtkMatrix4x4()
        vtk.vtkMatrix4x4.Multiply4x4(snapMatrix, wMatSource, newWorldMatrixSource)
        
        if parentSource:
            pwMatInverseSource = vtk.vtkMatrix4x4()
            vtk.vtkMatrix4x4.Invert(pwMatSource, pwMatInverseSource)
            newLocalMatrixSource = vtk.vtkMatrix4x4()
            vtk.vtkMatrix4x4.Multiply4x4(pwMatInverseSource, newWorldMatrixSource, newLocalMatrixSource)
        else:
            newLocalMatrixSource = newWorldMatrixSource
            
        tNodeSource.SetMatrixTransformToParent(newLocalMatrixSource)
        sourceNode.Modified()
