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
File Name: qSlicerFemurFracturePlannerCppModuleWidget.cxx
Version: v0-2.1.1
Date: 2026-06-30
Description: 메인 모듈 위젯 클래스 구현 (독립 다이얼로그 관리 기능 이식)

Version History:
- v0-2.1.1 (2026-06-30)
  - 소멸자에서 deleteLater() 대신 즉시 동기 delete를 수행해 VTK 메모리 누수 해결 (Z 증가)
- v0-2.1.0 (2026-06-30)
  - 다이얼로그에 Logic 주입 인터페이스 연동 (Y 증가)
- v0-2.0.3 (2026-06-30)
  - setModal() 함수 제거로 qMRMLWidget 전환 컴파일 오류 해결 (Z 증가)
- v0-2.0.2 (2026-06-30)
  - 널 포인터 런타임 크래시 방어용 openPlannerButton 검증 로직 추가 (Z 증가)
- v0-2.0.1 (2026-06-30)
  - CMakeLists.txt 종속성 추가 및 MSVC 인코딩 오류 대응 (Z 증가)
- v0-2.0.0 (2026-06-30)
  - Hello World 대신 독립 계획 창 열기 기능 이식 (X 증가)
- v0-1.0.0 (2026-06-30)
  - Hello World 출력 테스트용 버튼 기능 구현
- v0-0.0.0 (2026-06-30)
  - 최초 작성 (기본 템플릿 코드 파일 생성)
*/

// Qt includes
#include <QDebug>

// Slicer includes
#include "qSlicerFemurFracturePlannerCppModuleWidget.h"
#include "ui_qSlicerFemurFracturePlannerCppModuleWidget.h"
#include "qSlicerFemurFracturePlannerCppModulePlannerDialog.h"
#include "vtkSlicerFemurFracturePlannerCppLogic.h"

//-----------------------------------------------------------------------------
class qSlicerFemurFracturePlannerCppModuleWidgetPrivate : public Ui_qSlicerFemurFracturePlannerCppModuleWidget
{
public:
  qSlicerFemurFracturePlannerCppModuleWidgetPrivate();
  qSlicerFemurFracturePlannerCppModulePlannerDialog* PlannerDialog;
};

//-----------------------------------------------------------------------------
// qSlicerFemurFracturePlannerCppModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerFemurFracturePlannerCppModuleWidgetPrivate::qSlicerFemurFracturePlannerCppModuleWidgetPrivate()
  : PlannerDialog(nullptr)
{
}

//-----------------------------------------------------------------------------
// qSlicerFemurFracturePlannerCppModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerFemurFracturePlannerCppModuleWidget::qSlicerFemurFracturePlannerCppModuleWidget(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerFemurFracturePlannerCppModuleWidgetPrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerFemurFracturePlannerCppModuleWidget::~qSlicerFemurFracturePlannerCppModuleWidget()
{
  Q_D(qSlicerFemurFracturePlannerCppModuleWidget);
  if (d->PlannerDialog)
  {
    d->PlannerDialog->close();
    delete d->PlannerDialog;
    d->PlannerDialog = nullptr;
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModuleWidget::setup()
{
  Q_D(qSlicerFemurFracturePlannerCppModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // openPlannerButton 클릭 시그널을 onOpenPlannerWindowClicked 슬롯에 연결 (널 포인터 방어 코드 적용)
  if (d->openPlannerButton)
  {
    connect(d->openPlannerButton, SIGNAL(clicked()), this, SLOT(onOpenPlannerWindowClicked()));
  }

  // 독립 팝업 다이얼로그 생성
  d->PlannerDialog = new qSlicerFemurFracturePlannerCppModulePlannerDialog(this);
  d->PlannerDialog->setLogic(vtkSlicerFemurFracturePlannerCppLogic::SafeDownCast(this->logic()));
  d->PlannerDialog->setWindowTitle("Femur Fracture Planner 창");
  d->PlannerDialog->setObjectName("FemurFracturePlannerDialog");
  d->PlannerDialog->setWindowFlags(
    Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint
  );
  d->PlannerDialog->resize(850, 830);
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModuleWidget::enter()
{
  Q_D(qSlicerFemurFracturePlannerCppModuleWidget);
  this->Superclass::enter();

  if (d->PlannerDialog)
  {
    d->PlannerDialog->show();
    d->PlannerDialog->raise();
    d->PlannerDialog->activateWindow();
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModuleWidget::exit()
{
  Q_D(qSlicerFemurFracturePlannerCppModuleWidget);
  if (d->PlannerDialog)
  {
    d->PlannerDialog->hide();
  }
  this->Superclass::exit();
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerFemurFracturePlannerCppModuleWidget);
  this->Superclass::setMRMLScene(scene);

  if (d->PlannerDialog)
  {
    d->PlannerDialog->setMRMLScene(scene);
  }
}

//-----------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppModuleWidget::onOpenPlannerWindowClicked()
{
  Q_D(qSlicerFemurFracturePlannerCppModuleWidget);
  if (d->PlannerDialog)
  {
    d->PlannerDialog->show();
    d->PlannerDialog->raise();
    d->PlannerDialog->activateWindow();
  }
}