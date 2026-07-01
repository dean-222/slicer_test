/*
File Name: vtkSlicerFemurFracturePlannerCppLogicTest.cxx
Version: v0-0.0.0
Date: 2026-06-30
Description: FemurFracturePlannerCpp Logic의 기본 MRML Scene 연결 및 잘못된 입력 방어 회귀 테스트

Version History:
- v0-0.0.0 (2026-06-30)
  - 최초 작성
*/

#include "vtkSlicerFemurFracturePlannerCppLogic.h"

#include <vtkMRMLScene.h>
#include <vtkNew.h>

#include <cstdlib>
#include <vector>

int vtkSlicerFemurFracturePlannerCppLogicTest(int, char*[])
{
  vtkNew<vtkMRMLScene> scene;
  vtkNew<vtkSlicerFemurFracturePlannerCppLogic> logic;
  logic->SetMRMLScene(scene);

  if (logic->GetMRMLScene() != scene.GetPointer())
  {
    return EXIT_FAILURE;
  }

  double meanDistance = 0.0;
  int iterations = 0;
  const std::vector<vtkMRMLModelNode*> noFragments;
  if (logic->RunIcpRegistration(noFragments, nullptr, meanDistance, iterations))
  {
    return EXIT_FAILURE;
  }

  if (logic->RunFractureSurfaceSnap(nullptr, nullptr, meanDistance))
  {
    return EXIT_FAILURE;
  }

  if (logic->RunLandmarkRegistration(nullptr, nullptr, nullptr, nullptr))
  {
    return EXIT_FAILURE;
  }

  if (logic->RunMaskedIcpRegistration(nullptr, nullptr, nullptr, "", ""))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
