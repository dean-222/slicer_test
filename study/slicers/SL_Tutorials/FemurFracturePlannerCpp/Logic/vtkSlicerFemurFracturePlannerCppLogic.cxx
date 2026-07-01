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
Version: v0-5.0.0
Date: 2026-07-01
Description: 대퇴골 골절 수술 계획을 위한 로직 연산 클래스 구현 (볼륨 렌더링 및 변환)

Version History:
- v0-5.0.0 (2026-07-01)
  - RunIcpRegistration 및 RunFractureSurfaceSnap에 std::function 실시간 로그 콜백 파라미터 구현 (X 증가)
- v0-4.0.1 (2026-06-30)
  - 비선형 부모 Transform에서 계층을 변경하지 않고 실패 처리하며 Surface Snap 월드 메쉬 변환 검증을 보강 (Z 증가)
- v0-4.0.0 (2026-06-30)
  - 완전 이식 보완: 공통 World Delta/Transform 헬퍼, 범용 영상 스칼라 처리, 랜드마크 검증, 골편 역할 속성 추가 (X 증가)
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
#include <vtkGeneralTransform.h>
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
#include <vtkDataArray.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkImageThreshold.h>
#include <vtkMath.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLTransformableNode.h>

// Slicer Logic includes
#include <vtkSlicerVolumeRenderingLogic.h>
#include <vtkSlicerVolumesLogic.h>

// STD includes
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace
{
constexpr const char* FRAGMENT_ROLE_ATTRIBUTE = "FemurFracturePlannerCpp.Role";
constexpr const char* FRAGMENT_ROLE_VALUE = "Fragment";
constexpr const char* FRAGMENT_INDEX_ATTRIBUTE = "FemurFracturePlannerCpp.FragmentIndex";

vtkMRMLLinearTransformNode* EnsureLinearTransformNode(
  vtkMRMLScene* scene,
  vtkMRMLTransformableNode* node)
{
  if (!scene || !node)
  {
    return nullptr;
  }

  vtkMRMLTransformNode* currentParent = node->GetParentTransformNode();
  if (vtkMRMLLinearTransformNode* linearParent =
        vtkMRMLLinearTransformNode::SafeDownCast(currentParent))
  {
    return linearParent;
  }

  vtkMRMLLinearTransformNode* linearTransform = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->AddNewNodeByClass("vtkMRMLLinearTransformNode"));
  if (!linearTransform)
  {
    return nullptr;
  }

  const std::string nodeName = node->GetName() ? node->GetName() : "Transformable";
  linearTransform->SetName((nodeName + "_Transform").c_str());

  // 기존 부모가 비선형 Transform이어도 계층을 잃지 않도록 새 선형 Transform을 그 아래에 삽입한다.
  if (currentParent)
  {
    linearTransform->SetAndObserveTransformNodeID(currentParent->GetID());
  }
  node->SetAndObserveTransformNodeID(linearTransform->GetID());
  return linearTransform;
}

bool ApplyWorldDeltaToNode(
  vtkMRMLScene* scene,
  vtkMRMLTransformableNode* node,
  vtkMatrix4x4* deltaWorld)
{
  if (!scene || !node || !deltaWorld)
  {
    return false;
  }

  // 비선형 부모 아래에서는 하나의 4x4 World delta를 Local 행렬로 정확히 환산할 수 없으므로
  // 노드 계층을 변경하기 전에 명확히 실패시킨다.
  vtkMRMLTransformNode* originalParent = node->GetParentTransformNode();
  if (originalParent && !originalParent->IsTransformToWorldLinear())
  {
    return false;
  }

  vtkMRMLLinearTransformNode* localTransform = EnsureLinearTransformNode(scene, node);
  if (!localTransform)
  {
    return false;
  }

  if (!localTransform->IsTransformToWorldLinear())
  {
    return false;
  }

  vtkNew<vtkMatrix4x4> currentWorld;
  if (!localTransform->GetMatrixTransformToWorld(currentWorld))
  {
    return false;
  }

  vtkNew<vtkMatrix4x4> newWorld;
  vtkMatrix4x4::Multiply4x4(deltaWorld, currentWorld, newWorld);

  vtkNew<vtkMatrix4x4> newLocal;
  vtkMRMLTransformNode* parentTransform = localTransform->GetParentTransformNode();
  if (parentTransform)
  {
    if (!parentTransform->IsTransformToWorldLinear())
    {
      return false;
    }
    vtkNew<vtkMatrix4x4> parentWorld;
    vtkNew<vtkMatrix4x4> parentWorldInverse;
    if (!parentTransform->GetMatrixTransformToWorld(parentWorld))
    {
      return false;
    }
    vtkMatrix4x4::Invert(parentWorld, parentWorldInverse);
    vtkMatrix4x4::Multiply4x4(parentWorldInverse, newWorld, newLocal);
  }
  else
  {
    newLocal->DeepCopy(newWorld);
  }

  localTransform->SetMatrixTransformToParent(newLocal);
  node->Modified();
  return true;
}

vtkSmartPointer<vtkPolyData> GetWorldPolyData(
  vtkMRMLTransformableNode* node,
  vtkPolyData* inputPolyData)
{
  if (!node || !inputPolyData)
  {
    return nullptr;
  }

  vtkSmartPointer<vtkPolyData> worldPolyData = vtkSmartPointer<vtkPolyData>::New();
  vtkMRMLTransformNode* parentTransform = node->GetParentTransformNode();
  if (!parentTransform)
  {
    worldPolyData->DeepCopy(inputPolyData);
    return worldPolyData;
  }

  vtkNew<vtkGeneralTransform> transformToWorld;
  parentTransform->GetTransformToWorld(transformToWorld);
  vtkNew<vtkTransformPolyDataFilter> transformFilter;
  transformFilter->SetInputData(inputPolyData);
  transformFilter->SetTransform(transformToWorld);
  transformFilter->Update();
  worldPolyData->DeepCopy(transformFilter->GetOutput());
  return worldPolyData;
}

bool HasNonCollinearPoints(vtkPoints* points)
{
  if (!points || points->GetNumberOfPoints() < 3)
  {
    return false;
  }

  constexpr double distanceToleranceSquared = 1e-8;
  constexpr double crossTolerance = 1e-6;
  const vtkIdType count = points->GetNumberOfPoints();

  for (vtkIdType i = 0; i < count - 2; ++i)
  {
    double p0[3];
    points->GetPoint(i, p0);
    for (vtkIdType j = i + 1; j < count - 1; ++j)
    {
      double p1[3];
      points->GetPoint(j, p1);
      if (vtkMath::Distance2BetweenPoints(p0, p1) <= distanceToleranceSquared)
      {
        continue;
      }

      double v01[3];
      vtkMath::Subtract(p1, p0, v01);
      for (vtkIdType k = j + 1; k < count; ++k)
      {
        double p2[3];
        points->GetPoint(k, p2);
        double v02[3];
        double cross[3];
        vtkMath::Subtract(p2, p0, v02);
        vtkMath::Cross(v01, v02, cross);
        if (vtkMath::Norm(cross) > crossTolerance)
        {
          return true;
        }
      }
    }
  }
  return false;
}
} // namespace

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerFemurFracturePlannerCppLogic);

//----------------------------------------------------------------------------
vtkSlicerFemurFracturePlannerCppLogic::vtkSlicerFemurFracturePlannerCppLogic() {}

//----------------------------------------------------------------------------
vtkSlicerFemurFracturePlannerCppLogic::~vtkSlicerFemurFracturePlannerCppLogic() {}

//----------------------------------------------------------------------------
void vtkSlicerFemurFracturePlannerCppLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerFemurFracturePlannerCppLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::StartCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerFemurFracturePlannerCppLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerFemurFracturePlannerCppLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerFemurFracturePlannerCppLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerFemurFracturePlannerCppLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
  this->Modified();
}

//---------------------------------------------------------------------------
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
        compositeNode->SetBackgroundVolumeID(volumeNode->GetID());
      }
      else
      {
        compositeNode->SetBackgroundVolumeID(nullptr);
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerFemurFracturePlannerCppLogic::ShowVolumeRendering(vtkMRMLScalarVolumeNode* volumeNode, double threshold)
{
  if (!volumeNode || !this->GetMRMLScene())
  {
    return;
  }

  vtkSlicerVolumeRenderingLogic* volRenLogic = 
    vtkSlicerVolumeRenderingLogic::SafeDownCast(this->GetModuleLogic("VolumeRendering"));
  if (!volRenLogic)
  {
    vtkErrorMacro("VolumeRendering logic not found!");
    return;
  }

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
    volRenLogic->UpdateDisplayNodeFromVolumeNode(displayNode, volumeNode);
    vtkMRMLVolumePropertyNode* presetNode = volRenLogic->GetPresetByName("CT-Bone");
    if (presetNode && displayNode->GetVolumePropertyNode())
    {
      displayNode->GetVolumePropertyNode()->Copy(presetNode);
    }

    this->AdjustVolumeRenderingThreshold(volumeNode, threshold);
    displayNode->SetVisibility(true);
  }
}

//---------------------------------------------------------------------------
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

  vtkPiecewiseFunction* opacityFunction = volumePropertyNode->GetVolumeProperty()->GetScalarOpacity();
  if (opacityFunction)
  {
    opacityFunction->RemoveAllPoints();
    opacityFunction->AddPoint(threshold + OPACITY_OFFSET_MIN, 0.0);
    opacityFunction->AddPoint(threshold, 0.15);
    opacityFunction->AddPoint(threshold + OPACITY_OFFSET_MID, 0.75);
    opacityFunction->AddPoint(OPACITY_MAX_HU, 0.85);
    
    displayNode->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerFemurFracturePlannerCppLogic::RotateVolume(vtkMRMLScalarVolumeNode* volumeNode, const char* axis, double angle, vtkMRMLModelNode* guideNode)
{
  if (!volumeNode || !this->GetMRMLScene())
  {
    return;
  }

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

  // SegmentationNode 동기화
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

  // 골편 모델 및 가이드 모델 동기화
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
    bool isTarget = (name.rfind("Femur_Fragment", 0) == 0) || 
                    (name.size() >= 9 && name.compare(name.size() - 9, 9, "_Mirrored") == 0) ||
                    (guideNode && modelNode->GetID() == guideNode->GetID());

    if (isTarget)
    {
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

      if (parentTransform && parentTransform->GetParentTransformNode() != transformNode)
      {
        parentTransform->SetAndObserveTransformNodeID(transformNode->GetID());
      }
    }
  }

  // 회전 행렬 계산 및 적용
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

  // 볼륨 렌더링 디스플레이 노드가 존재할 경우 3D 뷰 실시간 갱신 강제 트리거
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
vtkMRMLScalarVolumeNode* vtkSlicerFemurFracturePlannerCppLogic::InvertVolumeIntensity(vtkMRMLScalarVolumeNode* volumeNode)
{
  if (!volumeNode || !volumeNode->GetImageData() || !this->GetMRMLScene())
  {
    return nullptr;
  }

  vtkSlicerVolumesLogic* volumesLogic =
    vtkSlicerVolumesLogic::SafeDownCast(this->GetModuleLogic("Volumes"));
  if (!volumesLogic)
  {
    return nullptr;
  }

  const std::string clonedVolumeName =
    std::string(volumeNode->GetName() ? volumeNode->GetName() : "Volume") + "_Inverted";
  vtkMRMLScalarVolumeNode* clonedNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    volumesLogic->CloneVolume(this->GetMRMLScene(), volumeNode, clonedVolumeName.c_str()));
  if (!clonedNode || !clonedNode->GetImageData())
  {
    return nullptr;
  }

  vtkImageData* image = clonedNode->GetImageData();
  vtkDataArray* scalars = image->GetPointData() ? image->GetPointData()->GetScalars() : nullptr;
  if (!scalars || scalars->GetNumberOfTuples() == 0)
  {
    return clonedNode;
  }

  double scalarRange[2] = {0.0, 0.0};
  scalars->GetRange(scalarRange);
  const double inversionSum = scalarRange[0] + scalarRange[1];
  const vtkIdType tupleCount = scalars->GetNumberOfTuples();
  const int componentCount = scalars->GetNumberOfComponents();

  for (vtkIdType tupleIndex = 0; tupleIndex < tupleCount; ++tupleIndex)
  {
    for (int componentIndex = 0; componentIndex < componentCount; ++componentIndex)
    {
      const double value = scalars->GetComponent(tupleIndex, componentIndex);
      scalars->SetComponent(tupleIndex, componentIndex, inversionSum - value);
    }
  }
  scalars->Modified();
  image->Modified();

  vtkMRMLScalarVolumeDisplayNode* displayNode =
    vtkMRMLScalarVolumeDisplayNode::SafeDownCast(clonedNode->GetScalarVolumeDisplayNode());
  if (displayNode)
  {
    displayNode->SetAutoWindowLevel(true);
  }

  return clonedNode;
}

//---------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkSlicerFemurFracturePlannerCppLogic::DetectVolumeEdges(vtkMRMLScalarVolumeNode* volumeNode)
{
  if (!volumeNode || !volumeNode->GetImageData() || !this->GetMRMLScene())
  {
    return nullptr;
  }

  vtkSlicerVolumesLogic* volumesLogic =
    vtkSlicerVolumesLogic::SafeDownCast(this->GetModuleLogic("Volumes"));
  if (!volumesLogic)
  {
    return nullptr;
  }

  const std::string clonedVolumeName =
    std::string(volumeNode->GetName() ? volumeNode->GetName() : "Volume") + "_Edge";
  vtkMRMLScalarVolumeNode* clonedNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    volumesLogic->CloneVolume(this->GetMRMLScene(), volumeNode, clonedVolumeName.c_str()));
  if (!clonedNode)
  {
    return nullptr;
  }

  vtkNew<vtkImageGradientMagnitude> gradientFilter;
  gradientFilter->SetInputData(volumeNode->GetImageData());
  gradientFilter->SetDimensionality(3);
  gradientFilter->Update();

  vtkNew<vtkImageData> edgeImage;
  edgeImage->DeepCopy(gradientFilter->GetOutput());
  clonedNode->SetAndObserveImageData(edgeImage);

  vtkMRMLScalarVolumeDisplayNode* displayNode =
    vtkMRMLScalarVolumeDisplayNode::SafeDownCast(clonedNode->GetScalarVolumeDisplayNode());
  if (displayNode)
  {
    displayNode->SetAutoWindowLevel(true);
  }

  return clonedNode;
}

//---------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkSlicerFemurFracturePlannerCppLogic::DetectVolumeEdgesMasked(
  vtkMRMLScalarVolumeNode* volumeNode,
  double threshold)
{
  if (!volumeNode || !volumeNode->GetImageData() || !this->GetMRMLScene())
  {
    return nullptr;
  }

  vtkSlicerVolumesLogic* volumesLogic =
    vtkSlicerVolumesLogic::SafeDownCast(this->GetModuleLogic("Volumes"));
  if (!volumesLogic)
  {
    return nullptr;
  }

  const std::string clonedVolumeName =
    std::string(volumeNode->GetName() ? volumeNode->GetName() : "Volume") + "_MaskedEdge";
  vtkMRMLScalarVolumeNode* clonedNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    volumesLogic->CloneVolume(this->GetMRMLScene(), volumeNode, clonedVolumeName.c_str()));
  if (!clonedNode)
  {
    return nullptr;
  }

  // -1000 HU를 안전하게 표현하도록 먼저 signed short로 변환한다.
  vtkNew<vtkImageCast> castFilter;
  castFilter->SetInputData(volumeNode->GetImageData());
  castFilter->SetOutputScalarTypeToShort();
  castFilter->ClampOverflowOn();

  vtkNew<vtkImageThreshold> maskFilter;
  maskFilter->SetInputConnection(castFilter->GetOutputPort());
  maskFilter->ThresholdByLower(threshold);
  maskFilter->SetInValue(MASK_AIR_VALUE);
  maskFilter->ReplaceInOn();
  maskFilter->ReplaceOutOff();
  maskFilter->SetOutputScalarType(VTK_SHORT);

  vtkNew<vtkImageGradientMagnitude> gradientFilter;
  gradientFilter->SetInputConnection(maskFilter->GetOutputPort());
  gradientFilter->SetDimensionality(3);
  gradientFilter->Update();

  vtkNew<vtkImageData> edgeImage;
  edgeImage->DeepCopy(gradientFilter->GetOutput());
  clonedNode->SetAndObserveImageData(edgeImage);

  vtkMRMLScalarVolumeDisplayNode* displayNode =
    vtkMRMLScalarVolumeDisplayNode::SafeDownCast(clonedNode->GetScalarVolumeDisplayNode());
  if (displayNode)
  {
    displayNode->SetAutoWindowLevel(true);
  }

  return clonedNode;
}

//---------------------------------------------------------------------------
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
      fragModel->SetAttribute(FRAGMENT_ROLE_ATTRIBUTE, FRAGMENT_ROLE_VALUE);
      fragModel->SetAttribute(FRAGMENT_INDEX_ATTRIBUTE, std::to_string(idx + 1).c_str());

      // Connectivity 출력의 수명과 중복 정점을 정리한 독립 메쉬를 노드가 소유하도록 한다.
      vtkNew<vtkCleanPolyData> cleanRegion;
      cleanRegion->SetInputData(regionPolyData);
      cleanRegion->Update();
      vtkNew<vtkPolyData> fragmentPolyData;
      fragmentPolyData->DeepCopy(cleanRegion->GetOutput());
      fragModel->SetAndObservePolyData(fragmentPolyData);

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
  mirroredNode->SetAttribute("FemurFracturePlannerCpp.Role", "MirroredGuide");
  vtkNew<vtkPolyData> mirroredPolyData;
  mirroredPolyData->DeepCopy(reverseFilter->GetOutput());
  mirroredNode->SetAndObservePolyData(mirroredPolyData);
  if (modelNode->GetParentTransformNode())
  {
    mirroredNode->SetAndObserveTransformNodeID(modelNode->GetParentTransformNode()->GetID());
  }

  // 4. 반투명 표시 및 구분하기 쉬운 하늘색으로 디스플레이 기본 설정
  mirroredNode->CreateDefaultDisplayNodes();
  vtkMRMLModelDisplayNode* displayNode = vtkMRMLModelDisplayNode::SafeDownCast(mirroredNode->GetDisplayNode());
  if (displayNode)
  {
    displayNode->SetOpacity(0.6);
    displayNode->SetColor(0.2, 0.6, 1.0); // 산뜻한 하늘색 톤
  }

  return mirroredNode;
}

//---------------------------------------------------------------------------
bool vtkSlicerFemurFracturePlannerCppLogic::RunIcpRegistration(
  const std::vector<vtkMRMLModelNode*>& fragmentModelNodes,
  vtkMRMLModelNode* guideModelNode,
  double& meanDistance,
  int& iterations,
  std::function<void(const std::string&)> logCallback)
{
  meanDistance = -1.0;
  iterations = 0;

  if (!guideModelNode || !guideModelNode->GetPolyData() || !this->GetMRMLScene())
  {
    if (logCallback) logCallback("정합 입력 정보 또는 씬 상태가 유효하지 않습니다.");
    return false;
  }

  vtkSmartPointer<vtkPolyData> guideWorld =
    GetWorldPolyData(guideModelNode, guideModelNode->GetPolyData());
  if (!guideWorld || guideWorld->GetNumberOfPoints() == 0)
  {
    return false;
  }

  vtkNew<vtkCleanPolyData> cleanGuide;
  cleanGuide->SetInputData(guideWorld);
  cleanGuide->Update();

  vtkNew<vtkAppendPolyData> appendFilter;
  std::vector<vtkMRMLModelNode*> validFragments;
  for (vtkMRMLModelNode* fragmentNode : fragmentModelNodes)
  {
    if (!fragmentNode || fragmentNode == guideModelNode || !fragmentNode->GetPolyData())
    {
      continue;
    }

    vtkNew<vtkCleanPolyData> cleanFragment;
    cleanFragment->SetInputData(fragmentNode->GetPolyData());
    cleanFragment->Update();

    vtkSmartPointer<vtkPolyData> fragmentWorld =
      GetWorldPolyData(fragmentNode, cleanFragment->GetOutput());
    if (!fragmentWorld || fragmentWorld->GetNumberOfPoints() == 0)
    {
      continue;
    }

    appendFilter->AddInputData(fragmentWorld);
    validFragments.push_back(fragmentNode);
  }

  if (validFragments.empty())
  {
    if (logCallback) logCallback("정합을 실행할 유효한 골편 모델이 없습니다.");
    return false;
  }

  if (logCallback)
  {
    logCallback("병합 ICP 정합(Auto-Reduction) 연산 진행 중...");
    logCallback("정합 대상 골편 수: " + std::to_string(validFragments.size()) + "개");
    logCallback("기준 가이드 모델: " + std::string(guideModelNode->GetName() ? guideModelNode->GetName() : "None"));
  }

  appendFilter->Update();
  vtkPolyData* combinedSourcePoly = appendFilter->GetOutput();
  if (!combinedSourcePoly || combinedSourcePoly->GetNumberOfPoints() == 0)
  {
    return false;
  }

  vtkNew<vtkIterativeClosestPointTransform> icp;
  icp->SetSource(combinedSourcePoly);
  icp->SetTarget(cleanGuide->GetOutput());
  icp->GetLandmarkTransform()->SetModeToRigidBody();
  icp->SetMaximumNumberOfIterations(ICP_MAX_ITERATIONS);
  icp->SetMaximumNumberOfLandmarks(ICP_MAX_LANDMARKS);
  icp->CheckMeanDistanceOn();
  icp->SetMaximumMeanDistance(ICP_CONVERGENCE_DISTANCE);
  icp->StartByMatchingCentroidsOff();
  icp->Update();

  meanDistance = icp->GetMeanDistance();
  iterations = icp->GetNumberOfIterations();
  if (!std::isfinite(meanDistance))
  {
    if (logCallback) logCallback("오류: ICP 정합 오차 연산 결과가 유한하지 않습니다.");
    return false;
  }

  if (logCallback)
  {
    char logBuf[256];
    sprintf(logBuf, "병합 ICP 정합 완료: 평균 오차 %.4f mm, 반복 횟수 %d회", meanDistance, iterations);
    logCallback(logBuf);
  }

  vtkNew<vtkMatrix4x4> icpMatrix;
  icp->GetMatrix(icpMatrix);

  bool allApplied = true;
  for (vtkMRMLModelNode* fragmentNode : validFragments)
  {
    allApplied = ApplyWorldDeltaToNode(this->GetMRMLScene(), fragmentNode, icpMatrix) && allApplied;
  }
  return allApplied;
}

//---------------------------------------------------------------------------
bool vtkSlicerFemurFracturePlannerCppLogic::RunFractureSurfaceSnap(
  vtkMRMLModelNode* frag1Node,
  vtkMRMLModelNode* frag2Node,
  double& meanDistance,
  std::function<void(const std::string&)> logCallback)
{
  meanDistance = -1.0;
  if (!frag1Node || !frag1Node->GetPolyData() || !frag2Node || !frag2Node->GetPolyData()
      || frag1Node == frag2Node || !this->GetMRMLScene())
  {
    if (logCallback) logCallback("스냅 입력 골편 정보 또는 씬 상태가 유효하지 않습니다.");
    return false;
  }

  if (logCallback)
  {
    logCallback("골절면 정밀 스냅(Fracture Surface Snap) 실행 중...");
    logCallback("대상 골편 1: " + std::string(frag1Node->GetName() ? frag1Node->GetName() : "None"));
    logCallback("대상 골편 2: " + std::string(frag2Node->GetName() ? frag2Node->GetName() : "None"));
  }

  // A. 고립 정점 정리 및 월드 변환 적용
  vtkNew<vtkCleanPolyData> clean1;
  clean1->SetInputData(frag1Node->GetPolyData());
  clean1->Update();

  vtkNew<vtkCleanPolyData> clean2;
  clean2->SetInputData(frag2Node->GetPolyData());
  clean2->Update();

  vtkSmartPointer<vtkPolyData> poly1World = GetWorldPolyData(frag1Node, clean1->GetOutput());
  vtkSmartPointer<vtkPolyData> poly2World = GetWorldPolyData(frag2Node, clean2->GetOutput());
  if (!poly1World || !poly2World
      || poly1World->GetNumberOfPoints() < SNAP_MIN_POINTS
      || poly2World->GetNumberOfPoints() < SNAP_MIN_POINTS)
  {
    return false;
  }

  // B. 삼각형 메쉬 전체에서 먼저 법선을 계산한 뒤, 법선 배열을 보존한 채 소스만 다운샘플링한다.
  vtkNew<vtkPolyDataNormals> normalsFilter1;
  normalsFilter1->SetInputData(poly1World);
  normalsFilter1->ComputePointNormalsOn();
  normalsFilter1->ComputeCellNormalsOff();
  normalsFilter1->SplittingOff();
  normalsFilter1->ConsistencyOn();
  normalsFilter1->Update();

  vtkNew<vtkPolyDataNormals> normalsFilter2;
  normalsFilter2->SetInputData(poly2World);
  normalsFilter2->ComputePointNormalsOn();
  normalsFilter2->ComputeCellNormalsOff();
  normalsFilter2->SplittingOff();
  normalsFilter2->ConsistencyOn();
  normalsFilter2->Update();
  vtkPolyData* poly2WithNormals = normalsFilter2->GetOutput();

  vtkNew<vtkMaskPoints> maskFilter;
  maskFilter->SetInputData(normalsFilter1->GetOutput());
  maskFilter->SetOnRatio(10);
  maskFilter->GenerateVerticesOn();
  maskFilter->SingleVertexPerCellOn();
  maskFilter->Update();
  vtkPolyData* poly1Sampled = maskFilter->GetOutput();

  vtkDataArray* poly1Normals = poly1Sampled->GetPointData()->GetNormals();
  vtkDataArray* poly2Normals = poly2WithNormals->GetPointData()->GetNormals();
  if (!poly1Normals || !poly2Normals)
  {
    return false;
  }

  vtkNew<vtkKdTreePointLocator> locator;
  locator->SetDataSet(poly2WithNormals);
  locator->BuildLocator();

  // E. 상하 배치 관계 및 바운딩 박스 오프셋 획득
  double bounds1[6];
  poly1World->GetBounds(bounds1);
  double z1_min = bounds1[4], z1_max = bounds1[5];

  double bounds2[6];
  poly2WithNormals->GetBounds(bounds2);
  double z2_min = bounds2[4], z2_max = bounds2[5];

  double center1[3];
  poly1World->GetCenter(center1);
  double center2[3];
  poly2WithNormals->GetCenter(center2);

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
  vtkIdType numPoints2_all = poly2WithNormals->GetNumberOfPoints();
  for (vtkIdType i = 0; i < numPoints2_all; ++i)
  {
    double pt[3];
    poly2WithNormals->GetPoint(i, pt);
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
    poly2WithNormals->GetPoint(closestId, pt2);

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
    if (logCallback) logCallback("오류: 추출된 스냅 표면 점의 개수가 부족합니다.");
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

  if (logCallback)
  {
    char logBuf[256];
    sprintf(logBuf, "골절면 스냅 완료: 평균 오차 %.4f mm (매칭 포인트: %d개)", meanDistance, static_cast<int>(numPts));
    logCallback(logBuf);
  }

  // I. 공통 World delta 헬퍼로 부모 Transform 계층을 보존하며 이동 골편에 적용한다.
  vtkNew<vtkMatrix4x4> snapMatrix;
  landmarkTransform->GetMatrix(snapMatrix);
  return ApplyWorldDeltaToNode(this->GetMRMLScene(), frag1Node, snapMatrix);
}

//---------------------------------------------------------------------------
bool vtkSlicerFemurFracturePlannerCppLogic::RunLandmarkRegistration(
  vtkMRMLModelNode* sourceNode,
  vtkMRMLModelNode* targetNode,
  vtkMRMLNode* sourceMarkersNode,
  vtkMRMLNode* targetMarkersNode)
{
  vtkMRMLMarkupsFiducialNode* sourceFiducials =
    vtkMRMLMarkupsFiducialNode::SafeDownCast(sourceMarkersNode);
  vtkMRMLMarkupsFiducialNode* targetFiducials =
    vtkMRMLMarkupsFiducialNode::SafeDownCast(targetMarkersNode);

  if (!sourceNode || !targetNode || sourceNode == targetNode ||
      !sourceFiducials || !targetFiducials || !this->GetMRMLScene())
  {
    return false;
  }

  const int sourceCount = sourceFiducials->GetNumberOfControlPoints();
  const int targetCount = targetFiducials->GetNumberOfControlPoints();
  if (sourceCount < 3 || sourceCount != targetCount)
  {
    return false;
  }

  vtkNew<vtkPoints> sourcePoints;
  vtkNew<vtkPoints> targetPoints;
  for (int pointIndex = 0; pointIndex < sourceCount; ++pointIndex)
  {
    double sourcePoint[3] = {0.0, 0.0, 0.0};
    double targetPoint[3] = {0.0, 0.0, 0.0};
    sourceFiducials->GetNthControlPointPositionWorld(pointIndex, sourcePoint);
    targetFiducials->GetNthControlPointPositionWorld(pointIndex, targetPoint);
    sourcePoints->InsertNextPoint(sourcePoint);
    targetPoints->InsertNextPoint(targetPoint);
  }

  // 3점 이상이어도 거의 일직선이면 강체 회전이 불안정하므로 거부한다.
  if (!HasNonCollinearPoints(sourcePoints) || !HasNonCollinearPoints(targetPoints))
  {
    return false;
  }

  vtkNew<vtkLandmarkTransform> landmarkTransform;
  landmarkTransform->SetSourceLandmarks(sourcePoints);
  landmarkTransform->SetTargetLandmarks(targetPoints);
  landmarkTransform->SetModeToRigidBody();
  landmarkTransform->Update();

  vtkNew<vtkMatrix4x4> deltaWorld;
  landmarkTransform->GetMatrix(deltaWorld);
  for (int row = 0; row < 4; ++row)
  {
    for (int column = 0; column < 4; ++column)
    {
      if (!std::isfinite(deltaWorld->GetElement(row, column)))
      {
        return false;
      }
    }
  }

  return ApplyWorldDeltaToNode(this->GetMRMLScene(), sourceNode, deltaWorld);
}

//---------------------------------------------------------------------------
bool vtkSlicerFemurFracturePlannerCppLogic::RunMaskedIcpRegistration(
  vtkMRMLModelNode* sourceNode,
  vtkMRMLModelNode* targetNode,
  vtkMRMLNode* segNode,
  const std::string& sourceSegId,
  const std::string& targetSegId)
{
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(segNode);
  if (!sourceNode || !targetNode || sourceNode == targetNode ||
      !sourceNode->GetPolyData() || !targetNode->GetPolyData() ||
      !segmentationNode || sourceSegId.empty() || targetSegId.empty() ||
      !this->GetMRMLScene())
  {
    return false;
  }

  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  if (!segmentation)
  {
    return false;
  }

  segmentation->CreateRepresentation("Closed surface");
  vtkPolyData* sourceMaskPoly = vtkPolyData::SafeDownCast(
    segmentation->GetSegmentRepresentation(sourceSegId, "Closed surface"));
  vtkPolyData* targetMaskPoly = vtkPolyData::SafeDownCast(
    segmentation->GetSegmentRepresentation(targetSegId, "Closed surface"));
  if (!sourceMaskPoly || !targetMaskPoly ||
      sourceMaskPoly->GetNumberOfPoints() < 3 || targetMaskPoly->GetNumberOfPoints() < 3)
  {
    return false;
  }

  vtkSmartPointer<vtkPolyData> sourceMaskWorld =
    GetWorldPolyData(segmentationNode, sourceMaskPoly);
  vtkSmartPointer<vtkPolyData> targetMaskWorld =
    GetWorldPolyData(segmentationNode, targetMaskPoly);
  vtkSmartPointer<vtkPolyData> sourceBoneWorld =
    GetWorldPolyData(sourceNode, sourceNode->GetPolyData());
  vtkSmartPointer<vtkPolyData> targetBoneWorld =
    GetWorldPolyData(targetNode, targetNode->GetPolyData());
  if (!sourceMaskWorld || !targetMaskWorld || !sourceBoneWorld || !targetBoneWorld)
  {
    return false;
  }

  auto extractEnclosedSurface = [](vtkPolyData* boneWorld, vtkPolyData* maskWorld)
    -> vtkSmartPointer<vtkPolyData>
  {
    vtkNew<vtkSelectEnclosedPoints> enclosedFilter;
    enclosedFilter->SetInputData(boneWorld);
    enclosedFilter->SetSurfaceData(maskWorld);
    enclosedFilter->SetTolerance(0.001);
    enclosedFilter->Update();

    vtkNew<vtkThresholdPoints> thresholdFilter;
    thresholdFilter->SetInputConnection(enclosedFilter->GetOutputPort());
    thresholdFilter->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "SelectedPoints");
    thresholdFilter->ThresholdByUpper(1.0);
    thresholdFilter->Update();

    vtkSmartPointer<vtkPolyData> selectedPoints = vtkSmartPointer<vtkPolyData>::New();
    selectedPoints->DeepCopy(thresholdFilter->GetOutput());
    return selectedPoints;
  };

  vtkSmartPointer<vtkPolyData> sourceSurface =
    extractEnclosedSurface(sourceBoneWorld, sourceMaskWorld);
  vtkSmartPointer<vtkPolyData> targetSurface =
    extractEnclosedSurface(targetBoneWorld, targetMaskWorld);
  if (!sourceSurface || !targetSurface ||
      sourceSurface->GetNumberOfPoints() < 3 || targetSurface->GetNumberOfPoints() < 3)
  {
    return false;
  }

  vtkNew<vtkIterativeClosestPointTransform> icpTransform;
  icpTransform->SetSource(sourceSurface);
  icpTransform->SetTarget(targetSurface);
  icpTransform->GetLandmarkTransform()->SetModeToRigidBody();
  icpTransform->SetMaximumNumberOfIterations(100);
  icpTransform->SetMaximumNumberOfLandmarks(ICP_MAX_LANDMARKS);
  icpTransform->CheckMeanDistanceOn();
  icpTransform->SetMaximumMeanDistance(0.5);
  icpTransform->StartByMatchingCentroidsOn();
  icpTransform->Update();

  if (!std::isfinite(icpTransform->GetMeanDistance()))
  {
    return false;
  }

  vtkNew<vtkMatrix4x4> deltaWorld;
  icpTransform->GetMatrix(deltaWorld);
  return ApplyWorldDeltaToNode(this->GetMRMLScene(), sourceNode, deltaWorld);
}
