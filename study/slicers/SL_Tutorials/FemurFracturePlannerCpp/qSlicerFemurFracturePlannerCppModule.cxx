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
File Name: qSlicerFemurFracturePlannerCppModule.cxx
Version: v0-2.0.2
Date: 2026-06-30
Description: 메인 모듈 로딩 및 생명주기 관리 클래스 구현 (의존성 로드 순서 수정)

Version History:
- v0-2.0.2 (2026-06-30)
  - dependencies()에 Segmentations 의존성을 명시하여 런타임 플러그인 로드 오류 해결 (Z 증가)
- v0-2.0.0 (2026-06-30)
  - 최초 작성 및 기본 뼈대 코드 생성
*/

// FemurFracturePlannerCpp Logic includes
#include <vtkSlicerFemurFracturePlannerCppLogic.h>

// FemurFracturePlannerCpp includes
#include "qSlicerFemurFracturePlannerCppModule.h"
#include "qSlicerFemurFracturePlannerCppModuleWidget.h"

//-----------------------------------------------------------------------------
class qSlicerFemurFracturePlannerCppModulePrivate
{
public:
  qSlicerFemurFracturePlannerCppModulePrivate();
};

//-----------------------------------------------------------------------------
qSlicerFemurFracturePlannerCppModulePrivate::qSlicerFemurFracturePlannerCppModulePrivate() {}

//-----------------------------------------------------------------------------
qSlicerFemurFracturePlannerCppModule::qSlicerFemurFracturePlannerCppModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerFemurFracturePlannerCppModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerFemurFracturePlannerCppModule::~qSlicerFemurFracturePlannerCppModule() {}

//-----------------------------------------------------------------------------
QString qSlicerFemurFracturePlannerCppModule::helpText() const
{
  return "This is a loadable module that can be bundled in an extension";
}

//-----------------------------------------------------------------------------
QString qSlicerFemurFracturePlannerCppModule::acknowledgementText() const
{
  return "This work was partially funded by NIH grant NXNNXXNNNNNN-NNXN";
}

//-----------------------------------------------------------------------------
QStringList qSlicerFemurFracturePlannerCppModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("John Doe (AnyWare Corp.)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerFemurFracturePlannerCppModule::icon() const
{
  return QIcon(":/Icons/FemurFracturePlannerCpp.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerFemurFracturePlannerCppModule::categories() const
{
  return QStringList() << "SL_Tutorials";
}

//-----------------------------------------------------------------------------
QStringList qSlicerFemurFracturePlannerCppModule::dependencies() const
{
  // Segmentations 모듈이 먼저 정상 메모리 로드되어 있어야 플러그인 로더가 에러 없이 구동됩니다.
  return QStringList() << "Segmentations";
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerFemurFracturePlannerCppModule::createWidgetRepresentation()
{
  return new qSlicerFemurFracturePlannerCppModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerFemurFracturePlannerCppModule::createLogic()
{
  return vtkSlicerFemurFracturePlannerCppLogic::New();
}
