/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

/*
File Name: vtkSlicerFemurFracturePlannerCppLogic.cxx
Version: v0-3.0.0
Date: 2026-06-30
Description: 대퇴골 골절 수술 계획을 위한 로직 연산 클래스 구현 (볼륨 렌더링 및 변환)

Version History:
- v0-3.0.0 (2026-06-30)
  - 5단계: 골절면 정밀 스냅(RunFractureSurfaceSnap) 및 수동/마스크 정합(RunLandmarkRegistration, RunMaskedIcpRegistration) 구현 (X 증가)
- v0-2.0.0 (2026-06-30)
  - 4단계: 정상 측 가이드 미러링(MirrorModel) 및 병합 ICP 정합(RunIcpRegistration) 구현 (X 증가)
- v0-1.0.1 (2026-06-30)
  - RotateVolume 연산 시 3D 볼륨 렌더링 실시간 리렌더링 갱신 누락 버그 수정 (Z 증가)
- v0-1.0.0 (2026-06-30)
  - 3단계 뼈 분할(SeparateBoneFragments) 기능 알고리즘 이식 (X 증가)
- v0-0.2.0 (2026-06-30)
  - Slice View 내 배경 볼륨 가시성 제어 API 구현 (Y 증가)
- v0-0.1.0 (2026-06-30)
  - 2단계 CT 볼륨 렌더링 및 3D 회전/필터 로직 구현 (Y 증가)
- v0-0.0.0 (2026-06-30)
  - 기존 파일에 버전 관리 최초 등록 (기준 버전)
*/

// FemurFracturePlannerCpp Logic includes
#include "vtkSlicerFemurFracturePlannerCppLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLVolumeRenderingDisplayNode.h>
#include <vtkMRMLVolumePropertyNode.h>
#include <vtkVolumeProperty.h>
#include <vtkPiecewiseFunction.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLSegmentationNode.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLSegmentationDisplayNode.h>
#include <vtkMRMLModelDisplayNode.h>

// VTK includes
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkImageData.h>
#include <vtkPolyDataConnectivityFilter.h>
#include <vtkAppendPolyData.h>
#include <vtkStringArray.h>
#include <vtkIdTypeArray.h>
#include <vtkSegmentation.h>
#include <vtkImageCast.h>
#include <vtkImageGradientMagnitude.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkReverseSense.h>
#include <vtkCleanPolyData.h>
#include <vtkIterativeClosestPointTransform.h>
#include <vtkLandmarkTransform.h>
#include <vtkKdTreePointLocator.h>
#include <vtkMaskPoints.h>
#include <vtkPolyDataNormals.h>
#include <vtkSelectEnclosedPoints.h>
#include <vtkThresholdPoints.h>
#include <vtkPointData.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLTransformableNode.h>

// Slicer Logic includes
#include <vtkSlicerVolumeRenderingLogic.h>
#include <vtkSlicerVolumesLogic.h>

// STD includes
#include <cassert>
#include <string>
#include <vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerFemurFracturePlannerCppLogic);

//----------------------------------------------------------------------------
vtkSlicerFemurFracturePlannerCppLogic::vtkSlicerFemurFracturePlannerCppLogic() {}

//----------------------------------------------------------------------------
vtkSlicerFemurFracturePlannerCppLogic::~vtkSlicerFemurFracturePlannerCppLogic() {}

//----------------------------------------------------------------------------
// [객체 상태 출력]
// VTK object debug 출력에서 호출되는 함수다.
// 현재 Logic 클래스는 별도의 멤버 상태를 저장하지 않으므로
// 상위 클래스 출력만 호출한다.
void vtkSlicerFemurFracturePlannerCppLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
// [Slicer 씬 이벤트 구독 설정]
// Logic이 씬과 연동될 때 호출된다.
// NodeAdded / NodeRemoved / EndBatchProcess 이벤트를 구독하여,
// 씬에서 노드가 추가/삭제되거나 배치 작업이 끝날 때 자동으로 알림을 받는다.
void vtkSlicerFemurFracturePlannerCppLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
// [커스텀 MRML 노드 등록 지점]
// 이 모듈이 자체 MRML node class를 정의했다면 여기에서 scene에 등록한다.
// 현재는 별도 node class가 없으므로 scene 연결 여부만 확인한다.
void vtkSlicerFemurFracturePlannerCppLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
// [MRML scene 전체 갱신 hook]
// scene import, batch process 종료 등으로 전체 scene 상태가 바뀐 뒤 호출될 수 있다.
// 현재는 내부 cache나 자동 재계산 대상이 없으므로 scene 유효성만 확인한다.
void vtkSlicerFemurFracturePlannerCppLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
// [노드 추가 이벤트 hook]
// scene에 새 node가 추가될 때 호출된다.
// 현재는 자동 감지 로직을 두지 않지만, 향후 volume/model/segmentation 자동 선택에 사용할 수 있다.
void vtkSlicerFemurFracturePlannerCppLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node)) {}

//---------------------------------------------------------------------------
// [노드 제거 이벤트 hook]
// scene에서 node가 제거될 때 호출된다.
// 현재는 별도 정리 대상이 없지만, 향후 캐시된 node 포인터를 해제하는 데 사용할 수 있다.
void vtkSlicerFemurFracturePlannerCppLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node)) {}

//---------------------------------------------------------------------------
// [2D 슬라이스 뷰 배경 볼륨 가시성 제어]
// visible=true: 모든 Slice Composite 노드(Red/Yellow/Green 슬라이스)의
//               Background 레이어에 지정된 volumeNode를 배치하여 CT 단면을 표시한다.
// visible=false: Background를 nullptr로 설정하여 배경 볼륨을 숨긴다.
// Slicer의 3종 슬라이스 뷰는 각각 독립된 vtkMRMLSliceCompositeNode를 가지므로
// GetNodesByClass로 전체를 순회하며 일괄 적용한다.
void vtkSlicerFemurFracturePlannerCppLogic::SetVolumeVisibility(vtkMRMLScalarVolumeNode* volumeNode, bool visible)
{
  if (!this->GetMRMLScene())
  {
    return;
  }

  std::vector<vtkMRMLNode*> compositeNodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLSliceCompositeNode", compositeNodes);
  for (vtkMRMLNode* node : compositeNodes)
  {
    vtkMRMLSliceCompositeNode* compositeNode = vtkMRMLSliceCompositeNode::SafeDownCast(node);
    if (compositeNode)
    {
      if (visible && volumeNode)
      {
        // Background 레이어에 볼륨 ID를 지정하면 해당 CT가 2D 단면에 나타난다
        compositeNode->SetBackgroundVolumeID(volumeNode->GetID());
      }
      else
      {
        // nullptr로 설정하면 2D 슬라이스 뷰에서 배경 볼륨이 사라진다
        compositeNode->SetBackgroundVolumeID(nullptr);
      }
    }
  }
}

//---------------------------------------------------------------------------
// [3D 볼륨 렌더링 표시]
// CT 볼륨을 3D 뷰에 렌더링한다.
//
// [처리 흐름]
// 1. VolumeRendering 모듈 Logic을 획득한다.
// 2. 이미 생성된 VolumeRenderingDisplayNode가 있으면 재활용하고,
//    없으면 새로 생성하여 volumeNode에 연결한다.
// 3. CT-Bone 프리셋(관상 뼈 시각화 최적 설정)을 VolumeProperty에 복사한다.
// 4. 현재 UI 슬라이더 임계값(threshold)으로 불투명도 함수를 추가 조정한다.
// 5. 최종적으로 DisplayNode의 Visibility를 true로 설정한다.
void vtkSlicerFemurFracturePlannerCppLogic::ShowVolumeRendering(vtkMRMLScalarVolumeNode* volumeNode, double threshold)
{
  if (!volumeNode || !this->GetMRMLScene())
  {
    return;
  }

  // VolumeRendering 서브모듈 Logic 획득 (없으면 진행 불가)
  vtkSlicerVolumeRenderingLogic* volRenLogic = 
    vtkSlicerVolumeRenderingLogic::SafeDownCast(this->GetModuleLogic("VolumeRendering"));
  if (!volRenLogic)
  {
    vtkErrorMacro("VolumeRendering logic not found!");
    return;
  }

  // 기존 디스플레이 노드를 재활용하거나 새로 생성
  vtkMRMLVolumeRenderingDisplayNode* displayNode = 
    vtkMRMLVolumeRenderingDisplayNode::SafeDownCast(volRenLogic->GetFirstVolumeRenderingDisplayNode(volumeNode));
  if (!displayNode)
  {
    displayNode = volRenLogic->CreateVolumeRenderingDisplayNode();
    if (displayNode)
    {
      this->GetMRMLScene()->AddNode(displayNode);
      volumeNode->AddAndObserveDisplayNodeID(displayNode->GetID());
    }
  }

  if (displayNode)
  {
    // 볼륨 노드 정보를 기반으로 DisplayNode를 최신 상태로 동기화
    volRenLogic->UpdateDisplayNodeFromVolumeNode(displayNode, volumeNode);

    // CT-Bone 프리셋 복사: 뼈 조직의 HU 범위를 잘 부각하는 기본 색상/불투명도 지정
    vtkMRMLVolumePropertyNode* presetNode = volRenLogic->GetPresetByName("CT-Bone");
    if (presetNode && displayNode->GetVolumePropertyNode())
    {
      displayNode->GetVolumePropertyNode()->Copy(presetNode);
    }

    // 사용자 임계값에 맞게 불투명도 함수를 재설정
    this->AdjustVolumeRenderingThreshold(volumeNode, threshold);
    displayNode->SetVisibility(true);
  }
}

//---------------------------------------------------------------------------
// [3D 볼륨 렌더링 숨기기]
// volumeNode에 연결된 VolumeRenderingDisplayNode를 씬에서 제거하여
// 3D 뷰의 볼륨 렌더링을 완전히 비활성화한다.
// (Visibility만 false로 하면 노드가 남아 있으나, RemoveNode는 완전 제거한다.)
void vtkSlicerFemurFracturePlannerCppLogic::HideVolumeRendering(vtkMRMLScalarVolumeNode* volumeNode)
{
  if (!volumeNode || !this->GetMRMLScene())
  {
    return;
  }

  vtkSlicerVolumeRenderingLogic* volRenLogic = 
    vtkSlicerVolumeRenderingLogic::SafeDownCast(this->GetModuleLogic("VolumeRendering"));
  if (!volRenLogic)
  {
    return;
  }

  vtkMRMLNode* displayNode = volRenLogic->GetFirstVolumeRenderingDisplayNode(volumeNode);
  if (displayNode)
  {
    this->GetMRMLScene()->RemoveNode(displayNode);
  }
}

//---------------------------------------------------------------------------
// [3D 볼륨 렌더링 HU 임계값 실시간 조정]
// 사용자가 슬라이더를 드래그할 때 호출된다.
//
// [불투명도 함수(Piecewise Function) 설계]
// - threshold + OPACITY_OFFSET_MIN (약 threshold-200): 완전 투명 (공기/배경 제거)
// - threshold 지점            : 15% 불투명 (피부/연부 조직 반투명 처리)
// - threshold + OPACITY_OFFSET_MID (약 threshold+300): 75% 불투명 (피질골 진입)
// - OPACITY_MAX_HU (약 3000)  : 85% 불투명 (치밀한 피질골/금속 임플란트)
// 이 4-포인트 Ramp 구조가 CT 뼈 시각화의 핵심 불투명도 전이를 만든다.
void vtkSlicerFemurFracturePlannerCppLogic::AdjustVolumeRenderingThreshold(vtkMRMLScalarVolumeNode* volumeNode, double threshold)
{
  if (!volumeNode)
  {
    return;
  }

  vtkSlicerVolumeRenderingLogic* volRenLogic = 
    vtkSlicerVolumeRenderingLogic::SafeDownCast(this->GetModuleLogic("VolumeRendering"));
  if (!volRenLogic)
  {
    return;
  }

  vtkMRMLVolumeRenderingDisplayNode* displayNode = 
    vtkMRMLVolumeRenderingDisplayNode::SafeDownCast(volRenLogic->GetFirstVolumeRenderingDisplayNode(volumeNode));
  if (!displayNode)
  {
    return;
  }

  vtkMRMLVolumePropertyNode* volumePropertyNode = displayNode->GetVolumePropertyNode();
  if (!volumePropertyNode || !volumePropertyNode->GetVolumeProperty())
  {
    return;
  }

  // 불투명도 함수에서 모든 기존 제어점을 제거하고 새 임계값 기준으로 재설정
  vtkPiecewiseFunction* opacityFunction = volumePropertyNode->GetVolumeProperty()->GetScalarOpacity();
  if (opacityFunction)
  {
    opacityFunction->RemoveAllPoints();
    opacityFunction->AddPoint(threshold + OPACITY_OFFSET_MIN, 0.0);   // 임계값 아래: 완전 투명
    opacityFunction->AddPoint(threshold,                       0.15);  // 임계값 경계: 반투명 시작
    opacityFunction->AddPoint(threshold + OPACITY_OFFSET_MID,  0.75);  // 중간 밀도: 75% 불투명
    opacityFunction->AddPoint(OPACITY_MAX_HU,                  0.85);  // 최고 HU:   85% 불투명
    
    displayNode->Modified(); // 변경 사항을 3D 뷰에 즉시 반영
  }
}

//---------------------------------------------------------------------------
// [CT 볼륨 및 연결된 모델/세그멘테이션 3D 회전]
// 3D 수술 계획 시 가장 보기 편한 시점으로 전체 CT 씬을 회전한다.
//
// [핵심 설계 - 계층 구조(Transform Hierarchy)]
// 회전 변환 노드(RotationTransform)를 최상위 부모로 두고,
// CT 볼륨 / 세그멘테이션 / 골편 모델 / 가이드 모델 모두를
// 이 변환 아래에 묶어 하나처럼 통째로 회전한다.
//
// [골편 모델 계층 구조]
// RotationTransform (최상위 공통 회전)
//   └─ Fragment_Transform (골편 개별 이동/정복 변환)
//        └─ Femur_Fragment_N (골편 메쉬 노드)
//
// [대상 모델 판별 기준]
// - 이름이 "Femur_Fragment"로 시작하는 골편 모델
// - 이름이 "_Mirrored"로 끝나는 미러링된 가이드 모델
// - guideNode 매개변수로 명시적으로 전달된 외부 로드 가이드 모델
void vtkSlicerFemurFracturePlannerCppLogic::RotateVolume(vtkMRMLScalarVolumeNode* volumeNode, const char* axis, double angle, vtkMRMLModelNode* guideNode)
{
  if (!volumeNode || !this->GetMRMLScene())
  {
    return;
  }

  // 볼륨 노드에 연결된 회전 변환 노드를 획득하거나 신설
  vtkMRMLLinearTransformNode* transformNode = 
    vtkMRMLLinearTransformNode::SafeDownCast(volumeNode->GetParentTransformNode());

  if (!transformNode)
  {
    transformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLLinearTransformNode")
    );
    if (transformNode)
    {
      std::string transformName = std::string(volumeNode->GetName()) + "_RotationTransform";
      transformNode->SetName(transformName.c_str());
      volumeNode->SetAndObserveTransformNodeID(transformNode->GetID());
    }
  }

  if (!transformNode)
  {
    return;
  }

  // 세그멘테이션 노드도 동일 회전 변환 노드에 연결하여 함께 회전
  std::vector<vtkMRMLNode*> segmentationNodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLSegmentationNode", segmentationNodes);
  for (vtkMRMLNode* node : segmentationNodes)
  {
    vtkMRMLSegmentationNode* segNode = vtkMRMLSegmentationNode::SafeDownCast(node);
    if (segNode && segNode->GetParentTransformNode() != transformNode)
    {
      segNode->SetAndObserveTransformNodeID(transformNode->GetID());
    }
  }

  // 골편 모델 및 가이드 모델: 중간에 개별 변환 노드를 끼워 계층 구조 형성
  // 개별 변환(Fragment_Transform)이 최상위 회전 변환의 자식이 되어야
  // "공통 회전 + 개별 정복 이동"이 동시에 적용된다.
  std::vector<vtkMRMLNode*> modelNodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLModelNode", modelNodes);
  for (vtkMRMLNode* node : modelNodes)
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);
    if (!modelNode)
    {
      continue;
    }

    std::string name = modelNode->GetName() ? modelNode->GetName() : "";
    // 회전 대상 모델 여부 판별
    bool isTarget = (name.rfind("Femur_Fragment", 0) == 0) || 
                    (name.size() >= 9 && name.compare(name.size() - 9, 9, "_Mirrored") == 0) ||
                    (guideNode && modelNode->GetID() == guideNode->GetID());

    if (isTarget)
    {
      // 개별 변환 노드가 없으면 생성하고, 해당 변환의 부모를 회전 노드로 설정
      vtkMRMLLinearTransformNode* parentTransform = 
        vtkMRMLLinearTransformNode::SafeDownCast(modelNode->GetParentTransformNode());
      if (!parentTransform)
      {
        parentTransform = vtkMRMLLinearTransformNode::SafeDownCast(
          this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLLinearTransformNode")
        );
        if (parentTransform)
        {
          std::string parentTransformName = name + "_Transform";
          parentTransform->SetName(parentTransformName.c_str());
          modelNode->SetAndObserveTransformNodeID(parentTransform->GetID());
        }
      }

      // 개별 변환의 부모를 회전 변환으로 연결 (이중 계층 구조 완성)
      if (parentTransform && parentTransform->GetParentTransformNode() != transformNode)
      {
        parentTransform->SetAndObserveTransformNodeID(transformNode->GetID());
      }
    }
  }

  // 지정된 축으로 angle 도만큼 회전 행렬을 생성하고 변환 노드에 적용
  vtkNew<vtkTransform> transform;
  if (strcmp(axis, "X") == 0)
  {
    transform->RotateX(angle);
  }
  else if (strcmp(axis, "Y") == 0)
  {
    transform->RotateY(angle);
  }
  else
  {
    transform->RotateZ(angle);
  }

  vtkNew<vtkMatrix4x4> matrix;
  transform->GetMatrix(matrix);
  transformNode->SetMatrixTransformToParent(matrix);

  // 볼륨 렌더링 DisplayNode의 Modified()를 강제 호출하여 3D 뷰를 즉시 갱신
  // (변환만 바꾸면 볼륨 렌더링이 자동 갱신되지 않는 Slicer 버그 방지)
  vtkSlicerVolumeRenderingLogic* volRenLogic = 
    vtkSlicerVolumeRenderingLogic::SafeDownCast(this->GetModuleLogic("VolumeRendering"));
  if (volRenLogic && volumeNode)
  {
    vtkMRMLVolumeRenderingDisplayNode* displayNode = 
      vtkMRMLVolumeRenderingDisplayNode::SafeDownCast(volRenLogic->GetFirstVolumeRenderingDisplayNode(volumeNode));
    if (displayNode)
    {
      displayNode->Modified();
    }
  }
}

//---------------------------------------------------------------------------
// [CT 볼륨 밝기 반전 (Intensity Inversion)]
// 원본 CT 볼륨을 복제한 뒤 각 복셀의 HU 값을 (min + max - 현재값)으로 반전한다.
// 반전된 볼륨에서는 밝은 뼈가 어둡고 어두운 배경/공기가 밝아지므로,
// 에지 검출이나 특정 구조 분리에 도움이 된다.
//
// [주의사항]
// - Unsigned 타입 볼륨은 Short(int16)로 먼저 형변환하여 음수 값 지원을 활성화한다.
// - 복제본에 작업하므로 원본 CT는 보존된다.
vtkMRMLScalarVolumeNode* vtkSlicerFemurFracturePlannerCppLogic::InvertVolumeIntensity(vtkMRMLScalarVolumeNode* volumeNode)
{
  if (!volumeNode || !this->GetMRMLScene())
  {
    return nullptr;
  }

  // Volumes 서브모듈 Logic으로 원본 볼륨 복제
  vtkSlicerVolumesLogic* volumesLogic = 
    vtkSlicerVolumesLogic::SafeDownCast(this->GetModuleLogic("Volumes"));
  if (!volumesLogic)
  {
    return nullptr;
  }

  std::string clonedVolumeName = std::string(volumeNode->GetName()) + "_Inverted";
  vtkMRMLScalarVolumeNode* clonedNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    volumesLogic->CloneVolume(this->GetMRMLScene(), volumeNode, clonedVolumeName.c_str())
  );
  if (!clonedNode)
  {
    return nullptr;
  }

  vtkImageData* image = clonedNode->GetImageData();
  if (!image)
  {
    return clonedNode;
  }

  // Unsigned 타입(DICOM에서 가끔 발생)은 Short로 변환하여 반전 연산 준비
  std::string scalarTypeStr = image->GetScalarTypeAsString() ? image->GetScalarTypeAsString() : "";
  if (scalarTypeStr.find("Unsigned") != std::string::npos)
  {
    vtkNew<vtkImageCast> castFilter;
    castFilter->SetInputData(image);
    castFilter->SetOutputScalarTypeToShort();
    castFilter->Update();
    clonedNode->SetAndObserveImageData(castFilter->GetOutput());
    image = clonedNode->GetImageData();
  }

  // 전체 복셀을 순회하여 (min + max - ptr[i]) 반전 공식 적용
  if (image && image->GetScalarType() == VTK_SHORT)
  {
    short* ptr = static_cast<short*>(image->GetScalarPointer());
    int numScalars = image->GetNumberOfPoints();
    if (numScalars > 0)
    {
      short minVal = ptr[0];
      short maxVal = ptr[0];
      for (int i = 0; i < numScalars; ++i)
      {
        if (ptr[i] < minVal) minVal = ptr[i];
        if (ptr[i] > maxVal) maxVal = ptr[i];
      }
      for (int i = 0; i < numScalars; ++i)
      {
        ptr[i] = (maxVal + minVal) - ptr[i]; // 밝기 반전 공식
      }
      image->Modified();
    }
  }

  // 반전 후 자동 Window/Level 조정으로 최적 명암 표시
  vtkMRMLScalarVolumeDisplayNode* displayNode = 
    vtkMRMLScalarVolumeDisplayNode::SafeDownCast(clonedNode->GetScalarVolumeDisplayNode());
  if (displayNode)
  {
    displayNode->SetAutoWindowLevel(true);
  }

  return clonedNode;
}

//---------------------------------------------------------------------------
// [CT 볼륨 에지(윤곽) 검출 - 비마스크 버전]
// vtkImageGradientMagnitude 필터로 3D 볼륨 전체의 기울기(Gradient) 크기를 계산한다.
// 기울기 크기가 큰 복셀 = 인접 복셀과 HU 차이가 큰 경계면 = 뼈 윤곽선이다.
// 결과 볼륨은 원본의 복제본에 저장되며, 3D 볼륨 렌더링에 활용하면
// 뼈의 겉면 윤곽을 돋보이게 표시할 수 있다.
vtkMRMLScalarVolumeNode* vtkSlicerFemurFracturePlannerCppLogic::DetectVolumeEdges(vtkMRMLScalarVolumeNode* volumeNode)
{
  if (!volumeNode || !this->GetMRMLScene())
  {
    return nullptr;
  }

  vtkSlicerVolumesLogic* volumesLogic = 
    vtkSlicerVolumesLogic::SafeDownCast(this->GetModuleLogic("Volumes"));
  if (!volumesLogic)
  {
    return nullptr;
  }

  std::string clonedVolumeName = std::string(volumeNode->GetName()) + "_Edge";
  vtkMRMLScalarVolumeNode* clonedNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    volumesLogic->CloneVolume(this->GetMRMLScene(), volumeNode, clonedVolumeName.c_str())
  );
  if (!clonedNode)
  {
    return nullptr;
  }

  vtkImageData* image = clonedNode->GetImageData();
  if (!image)
  {
    return clonedNode;
  }

  // 3D Gradient Magnitude 필터 실행 (X/Y/Z 방향 편미분의 크기 합)
  vtkNew<vtkImageGradientMagnitude> gradientFilter;
  gradientFilter->SetInputData(image);
  gradientFilter->SetDimensionality(3);
  gradientFilter->Update();

  clonedNode->SetAndObserveImageData(gradientFilter->GetOutput());

  vtkMRMLScalarVolumeDisplayNode* displayNode = 
    vtkMRMLScalarVolumeDisplayNode::SafeDownCast(clonedNode->GetScalarVolumeDisplayNode());
  if (displayNode)
  {
    displayNode->SetAutoWindowLevel(true);
  }

  return clonedNode;
}

//---------------------------------------------------------------------------
// [CT 볼륨 에지 검출 - 임계값 마스킹(Masked) 버전]
// HU 임계값(threshold) 미만의 복셀을 공기(MASK_AIR_VALUE)로 먼저 초기화하여
// 배경 노이즈를 제거한 뒤 에지를 검출한다.
// 결과적으로 뼈/관심 조직 경계만 선명하게 남아 세그멘테이션 보조 시각화에 유용하다.
//
// [처리 흐름]
// 1. 볼륨 복제
// 2. Unsigned 타입이면 Short 변환 (음수 공간 확보)
// 3. threshold 미만 복셀을 MASK_AIR_VALUE로 강제 설정 (배경 마스킹)
// 4. Gradient Magnitude 필터 적용
vtkMRMLScalarVolumeNode* vtkSlicerFemurFracturePlannerCppLogic::DetectVolumeEdgesMasked(vtkMRMLScalarVolumeNode* volumeNode, double threshold)
{
  if (!volumeNode || !this->GetMRMLScene())
  {
    return nullptr;
  }

  vtkSlicerVolumesLogic* volumesLogic = 
    vtkSlicerVolumesLogic::SafeDownCast(this->GetModuleLogic("Volumes"));
  if (!volumesLogic)
  {
    return nullptr;
  }

  std::string clonedVolumeName = std::string(volumeNode->GetName()) + "_MaskedEdge";
  vtkMRMLScalarVolumeNode* clonedNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    volumesLogic->CloneVolume(this->GetMRMLScene(), volumeNode, clonedVolumeName.c_str())
  );
  if (!clonedNode)
  {
    return nullptr;
  }

  vtkImageData* image = clonedNode->GetImageData();
  if (!image)
  {
    return clonedNode;
  }

  // Unsigned 타입 처리: Short로 변환하여 음수 HU 지원
  std::string scalarTypeStr = image->GetScalarTypeAsString() ? image->GetScalarTypeAsString() : "";
  if (scalarTypeStr.find("Unsigned") != std::string::npos)
  {
    vtkNew<vtkImageCast> castFilter;
    castFilter->SetInputData(image);
    castFilter->SetOutputScalarTypeToShort();
    castFilter->Update();
    clonedNode->SetAndObserveImageData(castFilter->GetOutput());
    image = clonedNode->GetImageData();
  }

  // 임계값 미만 복셀을 공기값(MASK_AIR_VALUE)으로 초기화하여 배경 제거
  if (image && image->GetScalarType() == VTK_SHORT)
  {
    short* ptr = static_cast<short*>(image->GetScalarPointer());
    int numScalars = image->GetNumberOfPoints();
    for (int i = 0; i < numScalars; ++i)
    {
      if (ptr[i] < threshold)
      {
        ptr[i] = MASK_AIR_VALUE; // 공기/배경 HU(-1000)로 설정하여 에지 검출 오탐 방지
      }
    }
    image->Modified();
  }

  // 마스킹된 볼륨에 Gradient Magnitude 에지 검출 적용
  vtkNew<vtkImageGradientMagnitude> gradientFilter;
  gradientFilter->SetInputData(clonedNode->GetImageData());
  gradientFilter->SetDimensionality(3);
  gradientFilter->Update();

  clonedNode->SetAndObserveImageData(gradientFilter->GetOutput());

  vtkMRMLScalarVolumeDisplayNode* displayNode = 
    vtkMRMLScalarVolumeDisplayNode::SafeDownCast(clonedNode->GetScalarVolumeDisplayNode());
  if (displayNode)
  {
    displayNode->SetAutoWindowLevel(true);
  }

  return clonedNode;
}

//---------------------------------------------------------------------------
// [세그멘테이션 기반 골편 자동 분리]
// 사용자가 Segment Editor에서 만든 visible segment의 Closed Surface를 모아
// 연결 성분 분석(vtkPolyDataConnectivityFilter)으로 독립된 골편 표면을 분리한다.
//
// 입력:
// - segmentationNode: 골절 부위가 칠해진 세그멘테이션 노드
// - referenceVolumeNode: 향후 geometry 검증/동기화를 위해 남겨둔 입력 볼륨 노드
//
// 출력:
// - scene에 Femur_Fragment_1, Femur_Fragment_2 형태의 vtkMRMLModelNode를 생성한다.
// - 반환값은 실제 생성된 fragment model 개수다.
//
// 주의:
// - 현재는 크기 기준 상위 2개 연결 성분만 모델로 만든다.
// - MIN_FRAGMENT_POINTS 이하의 작은 조각은 노이즈로 보고 제외한다.
// - 생성되는 모델 이름은 이후 테이블 표시, 삭제, 정합 대상 검색에 사용되므로 이름 규칙이 중요하다.
int vtkSlicerFemurFracturePlannerCppLogic::SeparateBoneFragments(
  vtkMRMLSegmentationNode* segmentationNode,
  vtkMRMLScalarVolumeNode* referenceVolumeNode)
{
  if (!segmentationNode || !this->GetMRMLScene())
  {
    return 0;
  }

  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  vtkMRMLSegmentationDisplayNode* displayNode = 
    vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());

  if (!segmentation || segmentation->GetNumberOfSegments() == 0)
  {
    return 0;
  }

  if (!displayNode)
  {
    return 0;
  }

  // 가시성이 확보된(체크된) 세그먼트의 ID 리스트 수집
  vtkNew<vtkStringArray> visibleSegmentIds;
  displayNode->GetVisibleSegmentIDs(visibleSegmentIds);
  if (visibleSegmentIds->GetNumberOfValues() == 0)
  {
    return 0;
  }

  // 3D Closed Surface 표현형이 없으면 강제로 계산 및 생성
  segmentationNode->CreateClosedSurfaceRepresentation();

  vtkNew<vtkAppendPolyData> appendFilter;
  bool hasInput = false;

  // 가시적인 모든 세그먼트의 표면 메쉬를 하나로 합침
  for (int i = 0; i < visibleSegmentIds->GetNumberOfValues(); ++i)
  {
    std::string segId = visibleSegmentIds->GetValue(i);
    vtkNew<vtkPolyData> segmentPolyData;
    segmentationNode->GetClosedSurfaceRepresentation(segId, segmentPolyData);
    if (segmentPolyData && segmentPolyData->GetNumberOfPoints() > 0)
    {
      appendFilter->AddInputData(segmentPolyData);
      hasInput = true;
    }
  }

  if (!hasInput)
  {
    return 0;
  }

  appendFilter->Update();
  vtkPolyData* mergedPolyData = appendFilter->GetOutput();

  // 연결성 분석 필터를 실행하여 3D 공간 상 분리된 메쉬 영역 추출
  vtkNew<vtkPolyDataConnectivityFilter> connectivityFilter;
  connectivityFilter->SetInputData(mergedPolyData);
  connectivityFilter->SetExtractionModeToAllRegions();
  connectivityFilter->Update();

  int numRegions = connectivityFilter->GetNumberOfExtractedRegions();
  vtkIdTypeArray* regionSizes = connectivityFilter->GetRegionSizes();
  if (numRegions == 0 || !regionSizes)
  {
    return 0;
  }

  // 각 영역의 인덱스와 정점 수를 쌍으로 묶어 리스트 생성
  struct RegionInfo {
    int id;
    vtkIdType size;
  };
  std::vector<RegionInfo> regionInfos;
  for (int i = 0; i < numRegions; ++i)
  {
    if (i < regionSizes->GetNumberOfValues())
    {
      regionInfos.push_back({i, regionSizes->GetValue(i)});
    }
    else
    {
      regionInfos.push_back({i, 0});
    }
  }

  // 정점 개수를 기준으로 내림차순 정렬하여 가장 유의미한 상위 2개 조각 식별
  std::sort(regionInfos.begin(), regionInfos.end(), [](const RegionInfo& a, const RegionInfo& b) {
    return a.size > b.size;
  });

  int maxTargets = std::min(2, static_cast<int>(regionInfos.size()));
  int separatedCount = 0;

  double distinctColors[5][3] = {
    {0.3, 0.9, 0.5},  // 연녹색 (Fragment 1)
    {0.9, 0.6, 0.2},  // 주황색 (Fragment 2)
    {0.4, 0.6, 0.9},  // 파란색 (Fragment 3)
    {0.9, 0.4, 0.7},  // 핑크색
    {0.8, 0.8, 0.2}   // 노란색
  };

  vtkMRMLTransformNode* transformNode = segmentationNode->GetParentTransformNode();

  for (int idx = 0; idx < maxTargets; ++idx)
  {
    int regionId = regionInfos[idx].id;
    vtkIdType size = regionInfos[idx].size;

    // 미세 잔해나 노이즈는 무시 (최소 정점 기준 필터링)
    if (size <= MIN_FRAGMENT_POINTS)
    {
      continue;
    }

    vtkNew<vtkPolyDataConnectivityFilter> singleRegionFilter;
    singleRegionFilter->SetInputData(mergedPolyData);
    singleRegionFilter->SetExtractionModeToSpecifiedRegions();
    singleRegionFilter->AddSpecifiedRegion(regionId);
    singleRegionFilter->Update();

    vtkPolyData* regionPolyData = singleRegionFilter->GetOutput();

    if (regionPolyData->GetNumberOfPoints() > MIN_FRAGMENT_POINTS)
    {
      vtkMRMLModelNode* fragModel = vtkMRMLModelNode::SafeDownCast(
        this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLModelNode")
      );
      if (!fragModel)
      {
        continue;
      }

      std::string name = "Femur_Fragment_" + std::to_string(idx + 1);
      fragModel->SetName(name.c_str());
      fragModel->SetAndObservePolyData(regionPolyData);

      // 부모 변환 노드가 존재하면 자식 관계를 맺어 동기화
      if (transformNode)
      {
        fragModel->SetAndObserveTransformNodeID(transformNode->GetID());
      }

      // 색상 지정
      fragModel->CreateDefaultDisplayNodes();
      vtkMRMLModelDisplayNode* dispNode = vtkMRMLModelDisplayNode::SafeDownCast(fragModel->GetDisplayNode());
      if (dispNode)
      {
        double* color = distinctColors[idx % 5];
        dispNode->SetColor(color[0], color[1], color[2]);
      }

      separatedCount++;
    }
  }

  return separatedCount;
}

//---------------------------------------------------------------------------
// [정상 측 가이드 모델 X축 대칭 미러링]
// 건강한 쪽의 뼈 모델(guideNode)을 가져와 X축 기준으로 거울 대칭하여
// 골절된 쪽의 "정상 형태 가이드"를 자동 생성한다.
//
// [처리 흐름]
// 1. vtkTransform에 Scale(-1, 1, 1)을 적용하여 X축 반전 메쉬 생성
// 2. X축 반전 시 삼각형의 Winding Order(감김 방향)가 뒤집혀 뼈가 어둡게
//    보이는 문제를 vtkReverseSense로 셀/법선 방향을 함께 교정
// 3. 새 모델 노드를 씬에 추가, 이름에 "_Mirrored" 접미사 부여
// 4. 기본 디스플레이를 반투명 하늘색으로 설정하여 원본과 시각적으로 구분
vtkMRMLModelNode* vtkSlicerFemurFracturePlannerCppLogic::MirrorModel(vtkMRMLModelNode* modelNode)
{
  if (!modelNode || !modelNode->GetPolyData() || !this->GetMRMLScene())
  {
    return nullptr;
  }

  vtkPolyData* polyData = modelNode->GetPolyData();

  // 1. X축 반전 변환 적용 (-1.0, 1.0, 1.0)
  vtkNew<vtkTransform> transform;
  transform->Scale(-1.0, 1.0, 1.0);

  vtkNew<vtkTransformPolyDataFilter> transformFilter;
  transformFilter->SetInputData(polyData);
  transformFilter->SetTransform(transform);
  transformFilter->Update();

  // 2. 미러링으로 뒤집힌 메쉬 법선(Normal) 및 셀 감김(Winding) 방향 교정
  // X축 반전은 행렬 행렬식(Determinant)을 음수로 만들어 삼각형 법선이 안쪽을 향하므로
  // ReverseCells + ReverseNormals로 겉면이 다시 바깥을 향하도록 복원한다.
  vtkNew<vtkReverseSense> reverseFilter;
  reverseFilter->SetInputData(transformFilter->GetOutput());
  reverseFilter->ReverseCellsOn();
  reverseFilter->ReverseNormalsOn();
  reverseFilter->Update();

  // 3. 새로운 모델 노드 생성 및 이름 설정
  vtkMRMLModelNode* mirroredNode = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLModelNode")
  );
  if (!mirroredNode)
  {
    return nullptr;
  }

  std::string mirrorName = std::string(modelNode->GetName() ? modelNode->GetName() : "Guide") + "_Mirrored";
  mirroredNode->SetName(mirrorName.c_str());
  mirroredNode->SetAndObservePolyData(reverseFilter->GetOutput());

  // 4. 반투명 표시 및 구분하기 쉬운 하늘색으로 디스플레이 기본 설정
  mirroredNode->CreateDefaultDisplayNodes();
  vtkMRMLModelDisplayNode* displayNode = vtkMRMLModelDisplayNode::SafeDownCast(mirroredNode->GetDisplayNode());
  if (displayNode)
  {
    displayNode->SetOpacity(0.6);           // 60% 불투명도: 뒤에 있는 CT/골편이 비쳐 보이게
    displayNode->SetColor(0.2, 0.6, 1.0);  // 산뜻한 하늘색 톤으로 원본과 시각적 구분
  }

  return mirroredNode;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// [병합 ICP 자동 정합 (Combined ICP Registration)]
// 분리된 여러 골편을 가이드 모델(정상 뼈 템플릿)에 맞춰 자동으로 정렬한다.
//
// [핵심 알고리즘 설계]
// 개별 ICP 방식의 문제: 각 골편이 독립적으로 가이드에 맞춰지면 서로 겹치거나 벌어진다.
// 병합 ICP 방식: 모든 골편을 vtkAppendPolyData로 하나로 합친 뒤 단일 ICP를 수행하여
// 공통 델타 변환 행렬(icpMatrix)을 획득하고, 이 행렬을 각 골편에 동일하게 적용한다.
//
// [처리 흐름]
// A. 가이드 모델 월드 좌표 변환 + 고립 정점 클린업
// B. 골편들의 월드 좌표 메쉬를 모두 하나로 병합 (vtkAppendPolyData)
// C. 병합 메쉬와 가이드 메쉬 간 ICP 정합 → 공통 델타 행렬 획득
// D. 각 골편의 현재 월드 행렬에 델타 행렬을 곱하여 갱신
//    (newWorldMatrix = icpMatrix * currentWorldMatrix)
// E. 부모 변환이 있는 경우 월드 행렬을 로컬 행렬로 역변환하여 적용
//    (newLocalMatrix = parentWorldInverse * newWorldMatrix)
bool vtkSlicerFemurFracturePlannerCppLogic::RunIcpRegistration(
  const std::vector<vtkMRMLModelNode*>& fragmentModelNodes,
  vtkMRMLModelNode* guideModelNode,
  double& meanDistance,
  int& iterations)
{
  meanDistance = -1.0;
  iterations = 0;

  if (!guideModelNode || !guideModelNode->GetPolyData() || !this->GetMRMLScene())
  {
    return false;
  }

  vtkPolyData* guidePolyData = guideModelNode->GetPolyData();

  // A. 가이드 모델의 월드 좌표계 변환을 계산하여 가이드 메쉬 준비
  vtkMRMLTransformNode* guideTransformNode = guideModelNode->GetParentTransformNode();
  vtkNew<vtkTransformPolyDataFilter> gtfFilter;
  vtkNew<vtkCleanPolyData> cleanGuide;
  if (guideTransformNode)
  {
    vtkNew<vtkTransform> gTransform;
    vtkNew<vtkMatrix4x4> gMatrix;
    guideTransformNode->GetMatrixTransformToWorld(gMatrix);
    gTransform->SetMatrix(gMatrix);

    gtfFilter->SetInputData(guidePolyData);
    gtfFilter->SetTransform(gTransform);
    gtfFilter->Update();
    guidePolyData = gtfFilter->GetOutput(); // 월드 좌표 가이드 메쉬로 교체
  }

  // 가이드 메쉬 고립 정점 정리
  cleanGuide->SetInputData(guidePolyData);
  cleanGuide->Update();
  guidePolyData = cleanGuide->GetOutput();

  // B. 모든 골편의 월드 좌표 메쉬를 수집하여 하나의 묶음으로 병합
  vtkNew<vtkAppendPolyData> appendFilter;
  int validFragmentsCount = 0;

  for (vtkMRMLModelNode* fragNode : fragmentModelNodes)
  {
    if (!fragNode || !fragNode->GetPolyData())
    {
      continue;
    }

    vtkMRMLTransformNode* transformNode = fragNode->GetParentTransformNode();
    if (!transformNode)
    {
      // 변환 노드가 없으면 새로 생성하여 연결
      transformNode = vtkMRMLTransformNode::SafeDownCast(
        this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLLinearTransformNode")
      );
      if (transformNode)
      {
        std::string tName = std::string(fragNode->GetName() ? fragNode->GetName() : "Fragment") + "_Transform";
        transformNode->SetName(tName.c_str());
        fragNode->SetAndObserveTransformNodeID(transformNode->GetID());
      }
    }

    if (!transformNode)
    {
      continue;
    }

    // 골편 메쉬 고립 정점 제거 전처리
    vtkNew<vtkCleanPolyData> cleanFilter;
    cleanFilter->SetInputData(fragNode->GetPolyData());
    cleanFilter->Update();

    // 현재의 누적 월드 변환 행렬 획득 및 적용
    vtkNew<vtkMatrix4x4> worldMatrix;
    transformNode->GetMatrixTransformToWorld(worldMatrix);

    vtkNew<vtkTransform> transform;
    transform->SetMatrix(worldMatrix);

    vtkNew<vtkTransformPolyDataFilter> tfFilter;
    tfFilter->SetInputData(cleanFilter->GetOutput());
    tfFilter->SetTransform(transform);
    tfFilter->Update();

    appendFilter->AddInputData(tfFilter->GetOutput()); // 병합 필터에 추가
    validFragmentsCount++;
  }

  if (validFragmentsCount == 0)
  {
    return false;
  }

  appendFilter->Update();
  vtkPolyData* combinedSourcePoly = appendFilter->GetOutput(); // 병합 완료된 골편 메쉬

  // C. 병합된 단일 소스 메쉬와 가이드 표면 간 ICP 정합 구동
  vtkNew<vtkIterativeClosestPointTransform> icp;
  icp->SetSource(combinedSourcePoly);
  icp->SetTarget(guidePolyData);
  icp->GetLandmarkTransform()->SetModeToRigidBody(); // 강체 회전/평행이동만 허용
  icp->SetMaximumNumberOfIterations(ICP_MAX_ITERATIONS);
  icp->SetMaximumNumberOfLandmarks(ICP_MAX_LANDMARKS);
  icp->CheckMeanDistanceOn();
  icp->SetMaximumMeanDistance(ICP_CONVERGENCE_DISTANCE);
  icp->StartByMatchingCentroidsOff(); // 사용자 초기 대략적 배치 의도 보존
  icp->Update();

  meanDistance = icp->GetMeanDistance();
  iterations = icp->GetNumberOfIterations();

  // D. 획득된 공통 델타 ICP 변환 행렬
  vtkNew<vtkMatrix4x4> icpMatrix;
  icp->GetMatrix(icpMatrix);

  // E. 각 골편에 공통 델타 행렬을 누적 곱셈하여 개별 변환 노드 갱신
  for (vtkMRMLModelNode* fragNode : fragmentModelNodes)
  {
    if (!fragNode || !fragNode->GetPolyData())
    {
      continue;
    }

    vtkMRMLTransformNode* transformNode = fragNode->GetParentTransformNode();
    if (!transformNode)
    {
      continue;
    }

    // newWorldMatrix = icpMatrix * currentWorldMatrix (델타를 왼쪽에 곱함)
    vtkNew<vtkMatrix4x4> worldMatrix;
    transformNode->GetMatrixTransformToWorld(worldMatrix);

    vtkNew<vtkMatrix4x4> newWorldMatrix;
    vtkMatrix4x4::Multiply4x4(icpMatrix, worldMatrix, newWorldMatrix);

    // 부모 변환이 있으면 로컬 행렬로 역변환: newLocal = parentWorldInverse * newWorld
    vtkMRMLTransformNode* parentTransformNode = transformNode->GetParentTransformNode();
    vtkNew<vtkMatrix4x4> newLocalMatrix;
    if (parentTransformNode)
    {
      vtkNew<vtkMatrix4x4> parentWorldMatrix;
      parentTransformNode->GetMatrixTransformToWorld(parentWorldMatrix);

      vtkNew<vtkMatrix4x4> parentWorldInverse;
      vtkMatrix4x4::Invert(parentWorldMatrix, parentWorldInverse);

      vtkMatrix4x4::Multiply4x4(parentWorldInverse, newWorldMatrix, newLocalMatrix);
    }
    else
    {
      newLocalMatrix->DeepCopy(newWorldMatrix);
    }

    transformNode->SetMatrixTransformToParent(newLocalMatrix);
  }

  return true;
}

//---------------------------------------------------------------------------
// [골절면 스냅 정합]
// 두 골편의 절단면 후보점을 자동으로 찾아 frag1Node를 frag2Node 쪽으로 맞춘다.
//
// 기본 아이디어:
// - 두 골편을 world 좌표계로 변환한다.
// - 위/아래 배치 관계와 normal 방향을 이용해 골절면 후보 영역만 남긴다.
// - KD-tree로 가까운 대응점을 찾고, normal이 서로 마주 보는 점쌍만 대응점으로 채택한다.
// - 대응점이 충분하면 vtkLandmarkTransform으로 rigid transform을 계산한다.
//
// 반환:
// - 성공하면 frag1Node의 transform node가 갱신되고 true를 반환한다.
// - 대응점이 SNAP_MIN_POINTS보다 적거나 입력이 유효하지 않으면 false를 반환한다.
bool vtkSlicerFemurFracturePlannerCppLogic::RunFractureSurfaceSnap(
  vtkMRMLModelNode* frag1Node,
  vtkMRMLModelNode* frag2Node,
  double& meanDistance)
{
  meanDistance = -1.0;
  if (!frag1Node || !frag1Node->GetPolyData() || !frag2Node || !frag2Node->GetPolyData() || !this->GetMRMLScene())
  {
    return false;
  }

  vtkMRMLTransformNode* tNode1 = frag1Node->GetParentTransformNode();
  vtkMRMLTransformNode* tNode2 = frag2Node->GetParentTransformNode();

  if (!tNode1)
  {
    tNode1 = vtkMRMLTransformNode::SafeDownCast(
      this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLLinearTransformNode")
    );
    if (tNode1)
    {
      std::string tName = std::string(frag1Node->GetName() ? frag1Node->GetName() : "Fragment1") + "_Transform";
      tNode1->SetName(tName.c_str());
      frag1Node->SetAndObserveTransformNodeID(tNode1->GetID());
    }
  }
  if (!tNode2)
  {
    tNode2 = vtkMRMLTransformNode::SafeDownCast(
      this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLLinearTransformNode")
    );
    if (tNode2)
    {
      std::string tName = std::string(frag2Node->GetName() ? frag2Node->GetName() : "Fragment2") + "_Transform";
      tNode2->SetName(tName.c_str());
      frag2Node->SetAndObserveTransformNodeID(tNode2->GetID());
    }
  }

  if (!tNode1 || !tNode2)
  {
    return false;
  }

  vtkNew<vtkMatrix4x4> wMat1;
  tNode1->GetMatrixTransformToWorld(wMat1);

  vtkNew<vtkMatrix4x4> wMat2;
  tNode2->GetMatrixTransformToWorld(wMat2);

  // A. 고립 정점 정리 및 월드 변환 적용
  vtkNew<vtkCleanPolyData> clean1;
  clean1->SetInputData(frag1Node->GetPolyData());
  clean1->Update();

  vtkNew<vtkCleanPolyData> clean2;
  clean2->SetInputData(frag2Node->GetPolyData());
  clean2->Update();

  vtkNew<vtkTransform> tf1;
  tf1->SetMatrix(wMat1);
  vtkNew<vtkTransformPolyDataFilter> tfFilter1;
  tfFilter1->SetInputData(clean1->GetOutput());
  tfFilter1->SetTransform(tf1);
  tfFilter1->Update();
  vtkPolyData* poly1World = tfFilter1->GetOutput();

  vtkNew<vtkTransform> tf2;
  tf2->SetMatrix(wMat2);
  vtkNew<vtkTransformPolyDataFilter> tfFilter2;
  tfFilter2->SetInputData(clean2->GetOutput());
  tfFilter2->SetTransform(tf2);
  tfFilter2->Update();
  vtkPolyData* poly2World = tfFilter2->GetOutput();

  // B. 2번 고정 골편 월드 좌표를 KD-Tree에 빌드
  vtkNew<vtkKdTreePointLocator> locator;
  locator->SetDataSet(poly2World);
  locator->BuildLocator();

  // C. 1번 골편 다운샘플링 (vtkMaskPoints 로 10개 중 1개 추출)
  vtkNew<vtkMaskPoints> maskFilter;
  maskFilter->SetInputData(poly1World);
  maskFilter->SetOnRatio(10);
  maskFilter->GenerateVerticesOn();
  maskFilter->SingleVertexPerCellOn();
  maskFilter->Update();
  vtkPolyData* poly1Sampled = maskFilter->GetOutput();

  // D. 두 메쉬의 포인트 법선(Normals) 연산
  vtkNew<vtkPolyDataNormals> normalsFilter1;
  normalsFilter1->SetInputData(poly1Sampled);
  normalsFilter1->ComputePointNormalsOn();
  normalsFilter1->ComputeCellNormalsOff();
  normalsFilter1->SplittingOff();
  normalsFilter1->Update();
  vtkDataArray* poly1Normals = normalsFilter1->GetOutput()->GetPointData()->GetNormals();

  vtkNew<vtkPolyDataNormals> normalsFilter2;
  normalsFilter2->SetInputData(poly2World);
  normalsFilter2->ComputePointNormalsOn();
  normalsFilter2->ComputeCellNormalsOff();
  normalsFilter2->SplittingOff();
  normalsFilter2->Update();
  vtkDataArray* poly2Normals = normalsFilter2->GetOutput()->GetPointData()->GetNormals();

  if (!poly1Normals || !poly2Normals)
  {
    return false;
  }

  // E. 상하 배치 관계 및 바운딩 박스 오프셋 획득
  double bounds1[6];
  poly1World->GetBounds(bounds1);
  double z1_min = bounds1[4], z1_max = bounds1[5];

  double bounds2[6];
  poly2World->GetBounds(bounds2);
  double z2_min = bounds2[4], z2_max = bounds2[5];

  double center1[3];
  poly1World->GetCenter(center1);
  double center2[3];
  poly2World->GetCenter(center2);

  bool isFrag1Below = center1[2] < center2[2];
  double MARGIN = 20.0;
  double z1_threshold = isFrag1Below ? (z1_max - MARGIN) : (z1_min + MARGIN);
  double z2_threshold = isFrag1Below ? (z2_min + MARGIN) : (z2_max - MARGIN);

  // F. 무게중심 오프셋 사전 정렬 계산
  double FRACTURE_AXIS_LIMIT = 0.4;
  double sumPt1[3] = {0.0, 0.0, 0.0};
  int cnt1 = 0;
  vtkIdType numPoints1 = poly1Sampled->GetNumberOfPoints();
  for (vtkIdType i = 0; i < numPoints1; ++i)
  {
    double pt[3];
    poly1Sampled->GetPoint(i, pt);
    if (isFrag1Below ? (pt[2] < z1_threshold) : (pt[2] > z1_threshold))
    {
      continue;
    }
    double n[3];
    poly1Normals->GetTuple(i, n);
    if (std::abs(n[2]) >= FRACTURE_AXIS_LIMIT)
    {
      sumPt1[0] += pt[0];
      sumPt1[1] += pt[1];
      sumPt1[2] += pt[2];
      cnt1++;
    }
  }

  double sumPt2[3] = {0.0, 0.0, 0.0};
  int cnt2 = 0;
  vtkIdType numPoints2_all = poly2World->GetNumberOfPoints();
  for (vtkIdType i = 0; i < numPoints2_all; ++i)
  {
    double pt[3];
    poly2World->GetPoint(i, pt);
    if (isFrag1Below ? (pt[2] > z2_threshold) : (pt[2] < z2_threshold))
    {
      continue;
    }
    double n[3];
    poly2Normals->GetTuple(i, n);
    if (std::abs(n[2]) >= FRACTURE_AXIS_LIMIT)
    {
      sumPt2[0] += pt[0];
      sumPt2[1] += pt[1];
      sumPt2[2] += pt[2];
      cnt2++;
    }
  }

  double preTranslation[3] = {0.0, 0.0, 0.0};
  if (cnt1 > 0 && cnt2 > 0)
  {
    double c1[3] = {sumPt1[0]/cnt1, sumPt1[1]/cnt1, sumPt1[2]/cnt1};
    double c2[3] = {sumPt2[0]/cnt2, sumPt2[1]/cnt2, sumPt2[2]/cnt2};
    preTranslation[0] = c2[0] - c1[0];
    preTranslation[1] = c2[1] - c1[1];
    preTranslation[2] = c2[2] - c1[2];
  }

  // G. 최근접 대응점 수집
  vtkNew<vtkPoints> sourcePoints;
  vtkNew<vtkPoints> targetPoints;
  double NORMAL_DOT_LIMIT = -0.6;

  for (vtkIdType i = 0; i < numPoints1; ++i)
  {
    double pt1[3];
    poly1Sampled->GetPoint(i, pt1);

    if (isFrag1Below ? (pt1[2] < z1_threshold) : (pt1[2] > z1_threshold))
    {
      continue;
    }

    double pt1_temp[3] = {
      pt1[0] + preTranslation[0],
      pt1[1] + preTranslation[1],
      pt1[2] + preTranslation[2]
    };

    vtkIdType closestId = locator->FindClosestPoint(pt1_temp);
    if (closestId < 0)
    {
      continue;
    }

    double pt2[3];
    poly2World->GetPoint(closestId, pt2);

    if (isFrag1Below ? (pt2[2] > z2_threshold) : (pt2[2] < z2_threshold))
    {
      continue;
    }

    double dist = std::sqrt(
      (pt1_temp[0] - pt2[0]) * (pt1_temp[0] - pt2[0]) +
      (pt1_temp[1] - pt2[1]) * (pt1_temp[1] - pt2[1]) +
      (pt1_temp[2] - pt2[2]) * (pt1_temp[2] - pt2[2])
    );

    if (dist <= SNAP_DISTANCE_LIMIT)
    {
      double n1[3];
      poly1Normals->GetTuple(i, n1);
      double n2[3];
      poly2Normals->GetTuple(closestId, n2);

      if (std::abs(n1[2]) >= FRACTURE_AXIS_LIMIT && std::abs(n2[2]) >= FRACTURE_AXIS_LIMIT)
      {
        double dotVal = n1[0]*n2[0] + n1[1]*n2[1] + n1[2]*n2[2];
        if (dotVal <= NORMAL_DOT_LIMIT)
        {
          sourcePoints->InsertNextPoint(pt1);
          targetPoints->InsertNextPoint(pt2);
        }
      }
    }
  }

  if (sourcePoints->GetNumberOfPoints() < SNAP_MIN_POINTS)
  {
    return false;
  }

  // H. Landmark 강체 정합 수행
  vtkNew<vtkLandmarkTransform> landmarkTransform;
  landmarkTransform->SetSourceLandmarks(sourcePoints);
  landmarkTransform->SetTargetLandmarks(targetPoints);
  landmarkTransform->SetModeToRigidBody();
  landmarkTransform->Update();

  // 오차 계산
  double totalDist = 0.0;
  vtkIdType numPts = sourcePoints->GetNumberOfPoints();
  for (vtkIdType i = 0; i < numPts; ++i)
  {
    double p1[3];
    sourcePoints->GetPoint(i, p1);
    double p2[3];
    targetPoints->GetPoint(i, p2);

    double p1Trans[3];
    landmarkTransform->TransformPoint(p1, p1Trans);

    double d = std::sqrt(
      (p1Trans[0] - p2[0])*(p1Trans[0] - p2[0]) +
      (p1Trans[1] - p2[1])*(p1Trans[1] - p2[1]) +
      (p1Trans[2] - p2[2])*(p1Trans[2] - p2[2])
    );
    totalDist += d;
  }
  meanDistance = totalDist / numPts;

  // I. 골편의 로컬/월드 변환 갱신
  vtkNew<vtkMatrix4x4> snapMatrix;
  landmarkTransform->GetMatrix(snapMatrix);

  vtkNew<vtkMatrix4x4> newWorldMatrix1;
  vtkMatrix4x4::Multiply4x4(snapMatrix, wMat1, newWorldMatrix1);

  vtkMRMLTransformNode* parent1 = tNode1->GetParentTransformNode();
  vtkNew<vtkMatrix4x4> newLocalMatrix1;
  if (parent1)
  {
    vtkNew<vtkMatrix4x4> pwMat1;
    parent1->GetMatrixTransformToWorld(pwMat1);

    vtkNew<vtkMatrix4x4> pwMatInverse1;
    vtkMatrix4x4::Invert(pwMat1, pwMatInverse1);

    vtkMatrix4x4::Multiply4x4(pwMatInverse1, newWorldMatrix1, newLocalMatrix1);
  }
  else
  {
    newLocalMatrix1->DeepCopy(newWorldMatrix1);
  }

  tNode1->SetMatrixTransformToParent(newLocalMatrix1);
  frag1Node->Modified();

  return true;
}

//---------------------------------------------------------------------------
// [수동 마커 기반 랜드마크 정합]
// 사용자가 source/target에 찍은 fiducial control point 쌍을 이용해
// sourceNode를 targetNode에 맞추는 rigid transform을 계산한다.
//
// 조건:
// - sourceMarkersNode와 targetMarkersNode는 vtkMRMLMarkupsFiducialNode여야 한다.
// - rigid landmark transform을 안정적으로 계산하기 위해 최소 3개의 대응점이 필요하다.
//
// 적용 방식:
// - 마커 좌표는 world 좌표로 읽는다.
// - 계산된 world transform을 sourceNode의 현재 transform 계층에 맞춰 local matrix로 변환한 뒤 적용한다.
bool vtkSlicerFemurFracturePlannerCppLogic::RunLandmarkRegistration(
  vtkMRMLModelNode* sourceNode,
  vtkMRMLModelNode* targetNode,
  vtkMRMLNode* sourceMarkersNode,
  vtkMRMLNode* targetMarkersNode)
{
  vtkMRMLMarkupsFiducialNode* srcFid = vtkMRMLMarkupsFiducialNode::SafeDownCast(sourceMarkersNode);
  vtkMRMLMarkupsFiducialNode* tgtFid = vtkMRMLMarkupsFiducialNode::SafeDownCast(targetMarkersNode);

  if (!sourceNode || !targetNode || !srcFid || !tgtFid || !this->GetMRMLScene())
  {
    return false;
  }

  int numPoints = std::min(srcFid->GetNumberOfControlPoints(), tgtFid->GetNumberOfControlPoints());
  if (numPoints < 3)
  {
    return false;
  }

  vtkNew<vtkPoints> sourcePoints;
  vtkNew<vtkPoints> targetPoints;

  for (int i = 0; i < numPoints; ++i)
  {
    double p1[3] = {0.0, 0.0, 0.0};
    double p2[3] = {0.0, 0.0, 0.0};
    srcFid->GetNthControlPointPositionWorld(i, p1);
    tgtFid->GetNthControlPointPositionWorld(i, p2);
    sourcePoints->InsertNextPoint(p1);
    targetPoints->InsertNextPoint(p2);
  }

  vtkNew<vtkLandmarkTransform> landmarkTransform;
  landmarkTransform->SetSourceLandmarks(sourcePoints);
  landmarkTransform->SetTargetLandmarks(targetPoints);
  landmarkTransform->SetModeToRigidBody();
  landmarkTransform->Update();

  vtkNew<vtkMatrix4x4> snapMatrix;
  landmarkTransform->GetMatrix(snapMatrix);

  // source 모델을 움직일 transform node를 확보한다.
  // 모델에 transform이 없으면 새 linear transform node를 만들고 sourceNode에 연결한다.
  vtkMRMLTransformNode* tNodeSource = sourceNode->GetParentTransformNode();
  if (!tNodeSource)
  {
    tNodeSource = vtkMRMLTransformNode::SafeDownCast(
      this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLLinearTransformNode")
    );
    if (tNodeSource)
    {
      std::string tName = std::string(sourceNode->GetName() ? sourceNode->GetName() : "Source") + "_Transform";
      tNodeSource->SetName(tName.c_str());
      sourceNode->SetAndObserveTransformNodeID(tNodeSource->GetID());
    }
  }

  if (!tNodeSource)
  {
    return false;
  }

  vtkNew<vtkMatrix4x4> lMatSource;
  tNodeSource->GetMatrixTransformToParent(lMatSource);
  vtkMRMLTransformNode* parentSource = tNodeSource->GetParentTransformNode();
  vtkNew<vtkMatrix4x4> wMatSource;

  // 현재 source transform을 world matrix로 변환한다.
  // parent transform이 있으면 parentWorld * local 순서로 world matrix를 만든다.
  if (parentSource)
  {
    vtkNew<vtkMatrix4x4> pwMatSource;
    parentSource->GetMatrixTransformToWorld(pwMatSource);
    vtkMatrix4x4::Multiply4x4(pwMatSource, lMatSource, wMatSource);
  }
  else
  {
    wMatSource->DeepCopy(lMatSource);
  }

  vtkNew<vtkMatrix4x4> newWorldMatrixSource;
  vtkMatrix4x4::Multiply4x4(snapMatrix, wMatSource, newWorldMatrixSource);

  // 계산된 새 world matrix를 다시 source transform node의 local matrix로 되돌린다.
  // MRML transform 계층에서는 node에 저장되는 값이 parent 기준 local matrix이기 때문이다.
  vtkNew<vtkMatrix4x4> newLocalMatrixSource;
  if (parentSource)
  {
    vtkNew<vtkMatrix4x4> pwMatInverseSource;
    vtkNew<vtkMatrix4x4> pwMatSource;
    parentSource->GetMatrixTransformToWorld(pwMatSource);
    vtkMatrix4x4::Invert(pwMatSource, pwMatInverseSource);

    vtkMatrix4x4::Multiply4x4(pwMatInverseSource, newWorldMatrixSource, newLocalMatrixSource);
  }
  else
  {
    newLocalMatrixSource->DeepCopy(newWorldMatrixSource);
  }

  tNodeSource->SetMatrixTransformToParent(newLocalMatrixSource);
  sourceNode->Modified();

  return true;
}

//---------------------------------------------------------------------------
// [ROI 마스크 기반 ICP 정합]
// 세그멘테이션에 칠해진 source/target ROI segment를 사용해
// 두 모델의 관심 표면만 잘라낸 뒤 ICP 정합을 수행한다.
//
// 입력:
// - sourceNode: 이동 대상 모델
// - targetNode: 기준 모델
// - segNode: Targeting_Source_ROI / Targeting_Target_ROI segment를 포함한 segmentation node
// - sourceSegId, targetSegId: 실제 segment ID 문자열
//
// 처리:
// - ROI segment의 closed surface를 world 좌표계로 변환한다.
// - vtkSelectEnclosedPoints로 bone model 중 ROI 내부에 포함된 점만 추출한다.
// - 추출된 부분 표면끼리 ICP를 수행하고 sourceNode transform에 결과를 누적 적용한다.
bool vtkSlicerFemurFracturePlannerCppLogic::RunMaskedIcpRegistration(
  vtkMRMLModelNode* sourceNode,
  vtkMRMLModelNode* targetNode,
  vtkMRMLNode* segNode,
  const std::string& sourceSegId,
  const std::string& targetSegId)
{
  vtkMRMLSegmentationNode* segmentNode = vtkMRMLSegmentationNode::SafeDownCast(segNode);
  if (!sourceNode || !targetNode || !segmentNode || !this->GetMRMLScene())
  {
    return false;
  }

  vtkSegmentation* segmentation = segmentNode->GetSegmentation();
  if (!segmentation)
  {
    return false;
  }

  segmentation->CreateRepresentation("Closed surface");

  vtkDataObject* sourceMaskData = segmentation->GetSegmentRepresentation(sourceSegId, "Closed surface");
  vtkDataObject* targetMaskData = segmentation->GetSegmentRepresentation(targetSegId, "Closed surface");

  if (!sourceMaskData || !targetMaskData)
  {
    return false;
  }

  vtkPolyData* sourceMaskPoly = vtkPolyData::SafeDownCast(sourceMaskData);
  vtkPolyData* targetMaskPoly = vtkPolyData::SafeDownCast(targetMaskData);

  if (!sourceMaskPoly || !targetMaskPoly)
  {
    return false;
  }

  // 월드 변환 람다
  // 주어진 polydata를 node의 현재 world 좌표계로 변환한다.
  // basePoly가 있으면 해당 polydata를 사용하고, 없으면 model node의 polydata를 사용한다.
  // segmentation ROI surface와 bone model surface를 같은 좌표계에서 비교하기 위한 helper다.
  auto GetWorldPolyData = [&](vtkMRMLTransformableNode* mrmlNode, vtkPolyData* basePoly) -> vtkSmartPointer<vtkPolyData> {
    vtkPolyData* poly = basePoly ? basePoly : vtkMRMLModelNode::SafeDownCast(mrmlNode)->GetPolyData();
    vtkMRMLTransformNode* tNode = mrmlNode->GetParentTransformNode();
    if (tNode)
    {
      vtkNew<vtkMatrix4x4> mat;
      tNode->GetMatrixTransformToWorld(mat);
      vtkNew<vtkTransform> trans;
      trans->SetMatrix(mat);
      vtkNew<vtkTransformPolyDataFilter> tf;
      tf->SetInputData(poly);
      tf->SetTransform(trans);
      tf->Update();
      return tf->GetOutput();
    }
    else
    {
      vtkNew<vtkPolyData> clone;
      clone->DeepCopy(poly);
      return clone;
    }
  };

  vtkSmartPointer<vtkPolyData> sourceMaskWorld = GetWorldPolyData(segmentNode, sourceMaskPoly);
  vtkSmartPointer<vtkPolyData> targetMaskWorld = GetWorldPolyData(segmentNode, targetMaskPoly);

  // 뼈 겉껍질 교차 헬퍼
  // ROI mask closed surface 내부에 포함되는 bone surface point만 추출한다.
  // vtkSelectEnclosedPoints가 각 point에 SelectedPoints 배열을 만들고,
  // vtkThresholdPoints가 선택된 point만 남긴 polydata를 만든다.
  auto GetEnclosedSurface = [&](vtkMRMLModelNode* boneNode, vtkPolyData* maskWorldPoly) -> vtkSmartPointer<vtkPolyData> {
    vtkSmartPointer<vtkPolyData> boneWorldPoly = GetWorldPolyData(boneNode, nullptr);
    vtkNew<vtkSelectEnclosedPoints> enclosedFilter;
    enclosedFilter->SetInputData(boneWorldPoly);
    enclosedFilter->SetSurfaceData(maskWorldPoly);
    enclosedFilter->SetTolerance(0.001);
    enclosedFilter->Update();

    vtkNew<vtkThresholdPoints> threshold;
    threshold->SetInputConnection(enclosedFilter->GetOutputPort());
    threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "SelectedPoints");
    threshold->ThresholdByUpper(1.0);
    threshold->Update();
    return threshold->GetOutput();
  };

  vtkSmartPointer<vtkPolyData> sourcePoly = GetEnclosedSurface(sourceNode, sourceMaskWorld);
  vtkSmartPointer<vtkPolyData> targetPoly = GetEnclosedSurface(targetNode, targetMaskWorld);

  if (!sourcePoly || !targetPoly || sourcePoly->GetNumberOfPoints() == 0 || targetPoly->GetNumberOfPoints() == 0)
  {
    return false;
  }

  vtkNew<vtkIterativeClosestPointTransform> icpTransform;
  icpTransform->SetSource(sourcePoly);
  icpTransform->SetTarget(targetPoly);
  icpTransform->GetLandmarkTransform()->SetModeToRigidBody();
  icpTransform->SetMaximumNumberOfIterations(100);
  icpTransform->CheckMeanDistanceOn();
  icpTransform->SetMaximumMeanDistance(0.5);
  icpTransform->StartByMatchingCentroidsOn();
  icpTransform->Update();

  vtkNew<vtkMatrix4x4> snapMatrix;
  icpTransform->GetMatrix(snapMatrix);

  vtkMRMLTransformNode* tNodeSource = sourceNode->GetParentTransformNode();
  if (!tNodeSource)
  {
    tNodeSource = vtkMRMLTransformNode::SafeDownCast(
      this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLLinearTransformNode")
    );
    if (tNodeSource)
    {
      std::string tName = std::string(sourceNode->GetName() ? sourceNode->GetName() : "Source") + "_Transform";
      tNodeSource->SetName(tName.c_str());
      sourceNode->SetAndObserveTransformNodeID(tNodeSource->GetID());
    }
  }

  if (!tNodeSource)
  {
    return false;
  }

  vtkNew<vtkMatrix4x4> lMatSource;
  tNodeSource->GetMatrixTransformToParent(lMatSource);
  vtkMRMLTransformNode* parentSource = tNodeSource->GetParentTransformNode();
  vtkNew<vtkMatrix4x4> wMatSource;

  // ROI 기반 ICP도 최종 적용 방식은 landmark/ICP와 동일하다.
  // 먼저 현재 source transform의 local matrix를 world matrix로 변환한다.
  if (parentSource)
  {
    vtkNew<vtkMatrix4x4> pwMatSource;
    parentSource->GetMatrixTransformToWorld(pwMatSource);
    vtkMatrix4x4::Multiply4x4(pwMatSource, lMatSource, wMatSource);
  }
  else
  {
    wMatSource->DeepCopy(lMatSource);
  }

  vtkNew<vtkMatrix4x4> newWorldMatrixSource;
  vtkMatrix4x4::Multiply4x4(snapMatrix, wMatSource, newWorldMatrixSource);

  // 새 world matrix를 parent transform 기준 local matrix로 되돌려 저장한다.
  // 이 과정을 거치지 않으면 parent transform이 있는 모델에서 이동량이 중복 적용될 수 있다.
  vtkNew<vtkMatrix4x4> newLocalMatrixSource;
  if (parentSource)
  {
    vtkNew<vtkMatrix4x4> pwMatInverseSource;
    vtkNew<vtkMatrix4x4> pwMatSource;
    parentSource->GetMatrixTransformToWorld(pwMatSource);
    vtkMatrix4x4::Invert(pwMatSource, pwMatInverseSource);

    vtkMatrix4x4::Multiply4x4(pwMatInverseSource, newWorldMatrixSource, newLocalMatrixSource);
  }
  else
  {
    newLocalMatrixSource->DeepCopy(newWorldMatrixSource);
  }

  tNodeSource->SetMatrixTransformToParent(newLocalMatrixSource);
  sourceNode->Modified();

  return true;
}
