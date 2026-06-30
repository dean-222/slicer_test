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
File Name: qSlicerFemurFracturePlannerCppModuleWidget.h
Version: v0-2.1.1
Date: 2026-06-30
Description: 메인 모듈 위젯 클래스 정의 (다이얼로그 트리거 기능 이식)

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
  - Hello World 출력 테스트용 버튼 기능 추가
- v0-0.0.0 (2026-06-30)
  - 최초 작성 (기본 템플릿 코드 파일 생성)
*/

#ifndef __qSlicerFemurFracturePlannerCppModuleWidget_h
#define __qSlicerFemurFracturePlannerCppModuleWidget_h

// Slicer includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerFemurFracturePlannerCppModuleExport.h"

class qSlicerFemurFracturePlannerCppModuleWidgetPrivate;
class vtkMRMLNode;

class Q_SLICER_QTMODULES_FEMURFRACTUREPLANNERCPP_EXPORT qSlicerFemurFracturePlannerCppModuleWidget : public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerFemurFracturePlannerCppModuleWidget(QWidget* parent = 0);
  virtual ~qSlicerFemurFracturePlannerCppModuleWidget();

public slots:
  void onOpenPlannerWindowClicked();
protected:
  QScopedPointer<qSlicerFemurFracturePlannerCppModuleWidgetPrivate> d_ptr;

  void setup() override;
  void enter() override;
  void exit() override;
  void setMRMLScene(vtkMRMLScene* scene) override;

private:
  Q_DECLARE_PRIVATE(qSlicerFemurFracturePlannerCppModuleWidget);
  Q_DISABLE_COPY(qSlicerFemurFracturePlannerCppModuleWidget);
};

#endif
