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
File Name: vtkSlicerFemurFracturePlannerCppLogic.h
Version: v0-3.0.0
Date: 2026-06-30
Description: 대퇴골 골절 수술 계획을 위한 로직 연산 클래스 정의 (볼륨 렌더링 및 변환)

Version History:
- v0-3.0.0 (2026-06-30)
  - 5단계: 골절면 정밀 스냅(RunFractureSurfaceSnap) 및 수동/마스크 정합(RunLandmarkRegistration, RunMaskedIcpRegistration) API 추가 (X 증가)
- v0-2.0.0 (2026-06-30)
  - 4단계: 정상 측 가이드 미러링(MirrorModel) 및 병합 ICP 정합(RunIcpRegistration) API 추가 (X 증가)
- v0-1.0.1 (2026-06-30)
  - RotateVolume 연산 시 3D 볼륨 렌더링 실시간 리렌더링 갱신 누락 버그 수정 (Z 증가)
- v0-1.0.0 (2026-06-30)
  - 3단계 뼈 분할(SeparateBoneFragments) 기능 API 추가 (X 증가)
- v0-0.2.0 (2026-06-30)
  - Slice View 내 배경 볼륨 가시성 제어 API 추가 (Y 증가)
- v0-0.1.0 (2026-06-30)
  - 2단계 CT 볼륨 렌더링 및 3D 회전/필터 로직 API 선언 (Y 증가)
- v0-0.0.0 (2026-06-30)
  - 기존 파일에 버전 관리 최초 등록 (기준 버전)
*/

#ifndef __vtkSlicerFemurFracturePlannerCppLogic_h
#define __vtkSlicerFemurFracturePlannerCppLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes
#include "vtkMRML.h"
#include "vtkMRMLScene.h"

// STD includes
#include <cstdlib>

#include "vtkSlicerFemurFracturePlannerCppModuleLogicExport.h"

class vtkMRMLScalarVolumeNode;
class vtkMRMLModelNode;
class vtkMRMLSegmentationNode;

class VTK_SLICER_FEMURFRACTUREPLANNERCPP_MODULE_LOGIC_EXPORT vtkSlicerFemurFracturePlannerCppLogic : public vtkSlicerModuleLogic
{
public:
  static vtkSlicerFemurFracturePlannerCppLogic* New();
  vtkTypeMacro(vtkSlicerFemurFracturePlannerCppLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // 2단계: 볼륨 렌더링 및 3D 회전/필터 로직 API
  void SetVolumeVisibility(vtkMRMLScalarVolumeNode* volumeNode, bool visible);
  void ShowVolumeRendering(vtkMRMLScalarVolumeNode* volumeNode, double threshold);
  void HideVolumeRendering(vtkMRMLScalarVolumeNode* volumeNode);
  void AdjustVolumeRenderingThreshold(vtkMRMLScalarVolumeNode* volumeNode, double threshold);
  void RotateVolume(vtkMRMLScalarVolumeNode* volumeNode, const char* axis, double angle, vtkMRMLModelNode* guideNode = nullptr);
  vtkMRMLScalarVolumeNode* InvertVolumeIntensity(vtkMRMLScalarVolumeNode* volumeNode);
  vtkMRMLScalarVolumeNode* DetectVolumeEdges(vtkMRMLScalarVolumeNode* volumeNode);
  vtkMRMLScalarVolumeNode* DetectVolumeEdgesMasked(vtkMRMLScalarVolumeNode* volumeNode, double threshold);

  // 3단계: 뼈 분할(Connectivity) 및 골편 연동 API
  int SeparateBoneFragments(vtkMRMLSegmentationNode* segmentationNode, vtkMRMLScalarVolumeNode* referenceVolumeNode = nullptr);

  // 4단계: 정상 측 가이드 미러링 및 Combined ICP 정합 API
  vtkMRMLModelNode* MirrorModel(vtkMRMLModelNode* modelNode);
  bool RunIcpRegistration(const std::vector<vtkMRMLModelNode*>& fragmentModelNodes, 
                           vtkMRMLModelNode* guideModelNode, 
                           double& meanDistance, 
                           int& iterations);

  // 5단계: 골절면 정밀 스냅 및 수동/마스크 정합 API
  bool RunFractureSurfaceSnap(vtkMRMLModelNode* frag1Node, 
                              vtkMRMLModelNode* frag2Node, 
                              double& meanDistance);
  
  bool RunLandmarkRegistration(vtkMRMLModelNode* sourceNode, 
                               vtkMRMLModelNode* targetNode, 
                               vtkMRMLNode* sourceMarkersNode, 
                               vtkMRMLNode* targetMarkersNode);
  
  bool RunMaskedIcpRegistration(vtkMRMLModelNode* sourceNode, 
                                vtkMRMLModelNode* targetNode, 
                                vtkMRMLNode* segNode, 
                                const std::string& sourceSegId, 
                                const std::string& targetSegId);

  // 2단계 제어용 상수
  static constexpr double OPACITY_OFFSET_MIN = -100.0;
  static constexpr double OPACITY_OFFSET_MID = 200.0;
  static constexpr double OPACITY_MAX_HU = 1500.0;
  static constexpr short MASK_AIR_VALUE = -1000;

  // 3단계 제어용 상수
  static constexpr int MIN_FRAGMENT_POINTS = 200;

  // 4단계 ICP 파라미터 상수
  static constexpr int ICP_MAX_ITERATIONS = 100;
  static constexpr int ICP_MAX_LANDMARKS = 200;
  static constexpr double ICP_CONVERGENCE_DISTANCE = 0.01;

  // 5단계 제어용 상수
  static constexpr double SNAP_DISTANCE_LIMIT = 25.0;
  static constexpr int SNAP_MIN_POINTS = 5;

protected:
  vtkSlicerFemurFracturePlannerCppLogic();
  ~vtkSlicerFemurFracturePlannerCppLogic() override;

  void SetMRMLSceneInternal(vtkMRMLScene* newScene) override;
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  void RegisterNodes() override;
  void UpdateFromMRMLScene() override;
  void OnMRMLSceneNodeAdded(vtkMRMLNode* node) override;
  void OnMRMLSceneNodeRemoved(vtkMRMLNode* node) override;

private:
  vtkSlicerFemurFracturePlannerCppLogic(const vtkSlicerFemurFracturePlannerCppLogic&); // Not implemented
  void operator=(const vtkSlicerFemurFracturePlannerCppLogic&);            // Not implemented
};

#endif
