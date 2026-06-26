# -*- coding: utf-8 -*-
"""
File Name: FemurFracturePlanner.py
Version: v0-22.0.0
Date: 2026-06-26
Description: 대퇴골 골절 계획 플래너 모듈의 메인 진입점 및 자가 테스트 코드를 포함하는 파일이다.

Version History:
- v0-22.0.0 (2026-06-26)
  - 마스킹 영역 전체를 덩어리로 추출하지 않고 원본 뼈 모델의 표면 정보 중 마스크 내부에 속한 '얇은 표면 점군(Point Cloud)'만 지능적으로 필터링하여 ICP 겹침(Interpenetration) 현상을 차단 (X 증가)
- v0-21.0.0 (2026-06-26)
  - 가상 뼈 정복 단계에 사용자가 원하는 점이나 색칠 영역을 바탕으로 뼈를 맞물리게 하는 수동 타겟팅 정밀 매칭(마커/마스킹) 기능 추가 (X 증가)
- v0-20.0.0 (2026-06-26)
  - 가상 뼈 정복 단계에서 가이드 모델을 3D 뷰어상에서 직접 이동 및 회전 조작할 수 있는 기즈모 UI 기능 추가 (X 증가)
- v0-19.3.1 (2026-06-25)
  - 3단계 골편 분리 시 컬러 테이블 의존성을 제거하고 서로 뚜렷하게 구분되는 고대비 기본 색상(연녹색, 주황색 등)을 확정 할당하여 시각적 구분을 개선함 (Z 증가)
- v0-19.3.0 (2026-06-25)
  - 8단계 골절면 스냅 연산에 vtkLandmarkTransform 대신 고정밀 vtkIterativeClosestPointTransform(ICP) 정복 알고리즘을 이식하여 맞물림을 개선함 (Y 증가, Z 0 초기화)
- v0-19.2.4 (2026-06-25)
  - 골편 생성 시 골편 관리자 표의 색상이 실제 3D 뷰어의 색상과 불일치하는 버그 수정, 그리고 미러링 가이드 삭제 시 원본 가이드 골편까지 함께 씬에서 제거하도록 연동 추가 (Z 증가)
- v0-19.2.3 (2026-06-25)
  - 가이드 모델 보기/숨기기 버튼(guideVisibilityButton)이 QPushButton checkable 속성 누락으로 toggled 시그널을 방출하지 않던 버그 수정 (Z 증가)
- v0-19.2.2 (2026-06-25)
  - 골편 분리 성공 시 3D 볼륨 렌더링 활성 여부와 무조건 상관없이 테이블 목록 갱신을 강제화하도록 분기 오류 수정 (Z 증가)
- v0-19.2.1 (2026-06-25)
  - 모듈 메인 파일 내의 모든 영어 주석 및 독스트링(Docstring)을 한글로 번역 및 보강 (Z 증가)
- v0-19.2.0 (2026-06-24)
  - sys.modules를 참조하여 개별 서브모듈이 이미 로드되어 있는 경우에만 순차 리로드하게 하여 리로드 과정 중 AttributeError 예외 누수 및 스킵 방지 (Y 증가)
- v0-19.1.0 (2026-06-24)
  - Slicer Reload 버튼 기동 시 하위 패키지 라이브러리(FemurFracturePlannerLib) 모듈 캐시도 강제 reload하도록 importlib.reload 로직 이식 (Y 증가)
- v0-19.0.0 (2026-06-24)
  - 진입점 클래스 및 테스트용 가상의 데이터 생성, 미러링 및 ICP 검증 과정에 대한 설명 주석 추가 (Y 증가, Z 초기화)
- v0-18.0.0 (2026-06-23)
  - 영상처리 로직 및 바인딩 기능 복원 (X 증가, Y/Z 초기화)
- v0-17.1.0 (2026-06-23)
  - PlannerDialog와 FemurFracturePlannerWidget을 별도 파일로 분리
- v0-17.0.0 (2026-06-23)
  - 고정 높이 팝업 확장 모드 복원
- v0-15.2.1 (2026-06-23)
  - 깨진 주석 및 구문 오류 수정
- v0-15.2.0 (2026-06-23)
  - FemurFracturePlannerLogic을 별도 모듈로 분리
- v0-15.1.0 (2026-06-23)
  - 사용하지 않는 임포트 정리
- v0-15.0.0 (2026-06-23)
  - 영상처리 UI 영역 및 버튼 추가
"""

import importlib
import logging

import slicer
import vtk
from slicer.i18n import tr as _
from slicer.i18n import translate
from slicer.ScriptedLoadableModule import ScriptedLoadableModule, ScriptedLoadableModuleTest

# Slicer의 Reload 버튼은 메인 모듈 파일만 다시 로드합니다.
# 임포트 캐시를 지우기 위해 sys.modules에 로드된 서브모듈들을 강제로 다시 로드해야 합니다.
import sys
try:
    # sys.modules에서 직접 관련 서브모듈을 찾아서 리로드
    for name in [
        "FemurFracturePlannerLib.FemurFracturePlannerLogic",
        "FemurFracturePlannerLib.FemurFracturePlannerWorkflow",
        "FemurFracturePlannerLib.FemurFracturePlannerFragmentManager",
        "FemurFracturePlannerLib.FemurFracturePlannerWidget"
    ]:
        if name in sys.modules:
            importlib.reload(sys.modules[name])
            logging.info(f"성공적으로 서브모듈을 리로드했습니다: {name}")
    
    # 패키지 자체도 리로드
    if "FemurFracturePlannerLib" in sys.modules:
        importlib.reload(sys.modules["FemurFracturePlannerLib"])
        logging.info("성공적으로 패키지를 리로드했습니다: FemurFracturePlannerLib")
except Exception as e:
    logging.debug(f"서브모듈 리로드 스킵 또는 실패: {e}")

from FemurFracturePlannerLib.FemurFracturePlannerLogic import FemurFracturePlannerLogic
from FemurFracturePlannerLib.FemurFracturePlannerWidget import FemurFracturePlannerWidget


class FemurFracturePlanner(ScriptedLoadableModule):
    """
    3D Slicer 모듈 관리자에 등록되는 메인 스크립트 로드 가능 모듈 클래스이다.
    모듈 메타데이터(기여자, 카테고리, 도움말 텍스트 등)를 Slicer에 전달한다.
    """

    def __init__(self, parent):
        ScriptedLoadableModule.__init__(self, parent)
        self.parent.title = _("Femur Fracture Planner")
        self.parent.categories = [translate("qSlicerAbstractCoreModule", "SL_Tutorials")]
        self.parent.dependencies = []
        self.parent.contributors = ["Antigravity (DeepMind)"]
        self.parent.helpText = _(
            """
이 모듈은 대퇴골 골절 수술 계획을 지원하기 위해 실시간 3D 볼륨 렌더링, 3D 회전 제어 및 내장 세그먼트 에디터(Segment Editor) 기능을 제공합니다.
"""
        )
        self.parent.acknowledgementText = _(
            """
3D Slicer의 확장 위젯들을 사용하여 개발되었습니다.
"""
        )


class FemurFracturePlannerTest(ScriptedLoadableModuleTest):
    """
    미러링 연산 및 ICP 자동 정렬 알고리즘의 유효성을 검증하기 위한 자가 테스트(Self-test) 클래스이다.
    모듈 패널의 Self Test 버튼을 누르거나 파이썬 콘솔에서 실행할 수 있다.
    """

    def setUp(self):
        slicer.mrmlScene.Clear()

    def runTest(self):
        self.setUp()
        self.test_FemurFracturePlannerEmbedded()

    def test_FemurFracturePlannerEmbedded(self):
        self.delayDisplay("내장 볼륨 렌더링 및 에디터 연산 점검 중...")
        
        logic = FemurFracturePlannerLogic()
        self.assertIsNotNone(logic)

        # 1. 구형의 가이드 템플릿 모델 생성 (중심점: X=5.0)
        sphere = vtk.vtkSphereSource()
        sphere.SetRadius(10.0)
        sphere.SetCenter(5.0, 0.0, 0.0)
        sphere.Update()

        guideModel = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLModelNode")
        guideModel.SetName("Test_Guide_Model")
        guideModel.SetAndObservePolyData(sphere.GetOutput())

        # 2. 미러링 알고리즘 테스트 (X축 기준 반전 대칭)
        mirroredNode = logic.mirrorModel(guideModel)
        self.assertIsNotNone(mirroredNode)
        self.assertIsNotNone(mirroredNode.GetPolyData())

        # 3. 골절된 파편을 모사한 구형 모델 생성 (중심점: X=7.0)
        sphere2 = vtk.vtkSphereSource()
        sphere2.SetRadius(10.0)
        sphere2.SetCenter(7.0, 0.0, 0.0)
        sphere2.Update()

        fragModel = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLModelNode")
        fragModel.SetName("Test_Fragment_Model")
        fragModel.SetAndObservePolyData(sphere2.GetOutput())

        # 4. ICP 정합을 구동하여 뼈 파편을 가이드 모델에 밀착 정렬
        logic.runIcpRegistration([fragModel], guideModel)

        # 5. 정렬된 뼈 파편의 월드 좌표 획득
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

        # 6. 정렬 후 중심 좌표가 가이드 중심(X=5.0)으로 수렴하는지 확인
        fragBounds = [0.0] * 6
        worldPoly.GetBounds(fragBounds)
        fragCenterX = (fragBounds[0] + fragBounds[1]) / 2.0
        self.assertAlmostEqual(fragCenterX, 5.0, places=2)

        self.delayDisplay("모든 테스트가 성공적으로 통과되었습니다!")
