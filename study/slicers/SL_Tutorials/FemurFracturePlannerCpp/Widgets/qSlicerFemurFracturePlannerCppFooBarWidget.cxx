/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// FooBar Widgets includes
#include "qSlicerFemurFracturePlannerCppFooBarWidget.h"
#include "ui_qSlicerFemurFracturePlannerCppFooBarWidget.h"

//-----------------------------------------------------------------------------
class qSlicerFemurFracturePlannerCppFooBarWidgetPrivate : public Ui_qSlicerFemurFracturePlannerCppFooBarWidget
{
  Q_DECLARE_PUBLIC(qSlicerFemurFracturePlannerCppFooBarWidget);

protected:
  qSlicerFemurFracturePlannerCppFooBarWidget* const q_ptr;

public:
  qSlicerFemurFracturePlannerCppFooBarWidgetPrivate(qSlicerFemurFracturePlannerCppFooBarWidget& object);
  virtual void setupUi(qSlicerFemurFracturePlannerCppFooBarWidget*);
};

// --------------------------------------------------------------------------
qSlicerFemurFracturePlannerCppFooBarWidgetPrivate::qSlicerFemurFracturePlannerCppFooBarWidgetPrivate(qSlicerFemurFracturePlannerCppFooBarWidget& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerFemurFracturePlannerCppFooBarWidgetPrivate::setupUi(qSlicerFemurFracturePlannerCppFooBarWidget* widget)
{
  this->Ui_qSlicerFemurFracturePlannerCppFooBarWidget::setupUi(widget);
}

//-----------------------------------------------------------------------------
// qSlicerFemurFracturePlannerCppFooBarWidget methods

//-----------------------------------------------------------------------------
qSlicerFemurFracturePlannerCppFooBarWidget::qSlicerFemurFracturePlannerCppFooBarWidget(QWidget* parentWidget)
  : Superclass(parentWidget)
  , d_ptr(new qSlicerFemurFracturePlannerCppFooBarWidgetPrivate(*this))
{
  Q_D(qSlicerFemurFracturePlannerCppFooBarWidget);
  d->setupUi(this);
}

//-----------------------------------------------------------------------------
qSlicerFemurFracturePlannerCppFooBarWidget ::~qSlicerFemurFracturePlannerCppFooBarWidget() {}
