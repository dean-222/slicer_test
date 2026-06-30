# 3단계 C++ 이식 컴파일 및 링킹 트러블슈팅

## 1. Mismatched Else Block Error (`C2181`)
- **현상**: `onInputVolumeChanged` 슬롯 내부에서 `else` 문이 꼬여서 중복으로 컴파일 오류 발생.
- **원인**: 치환 패치 시 블록 범위가 오정렬되어 `else { ... }` 블록이 연속 두 번 연달아 배치됨.
- **해결**: 중복된 `else` 블록을 깨끗하게 정리하여 단일 if-else 구조로 원복.

## 2. 'GetParentTransformNode' is not a member of 'vtkMRMLNode' (`C2039`)
- **현상**: `vtkMRMLNode` 객체에 대고 `GetParentTransformNode` 를 직접 호출할 때 선언 멤버가 아니라는 에러 발생.
- **원인**: `GetParentTransformNode` 는 `vtkMRMLNode` 의 직속 멤버가 아닌, 하위 상속 클래스인 `vtkMRMLTransformableNode` 의 멤버임.
- **해결**: 대상 노드를 **`vtkMRMLTransformableNode::SafeDownCast`** 로 캐스팅한 후 `GetParentTransformNode` 를 호출하도록 수정하고, `#include <vtkMRMLTransformableNode.h>` 헤더를 인클루드 추가.

## 3. Incomplete Type Errors (`C2027` / `C3861`)
- **현상**: `vtkMRMLModelDisplayNode` 및 `vtkPolyData` 에 대해 정의되지 않은 형식이라고 에러 발생.
- **원인**: 구현부에서 해당 클래스의 메서드(`SafeDownCast` 등)와 필드를 조작하지만 전방 선언만 있고 헤더 파일이 인클루드되지 않음.
- **해결**: 다이얼로그 구현부(`.cxx`) 상단에 **`#include <vtkMRMLModelDisplayNode.h>`** 및 **`#include <vtkPolyData.h>`** 를 명시적으로 기입.

## 4. 'qvtkConnect' is not a member of Dialog (`C2039`)
- **현상**: `qvtkConnect` 및 `qvtkDisconnect` 가 다이얼로그 클래스의 멤버가 아니라고 에러 발생.
- **원인**: 플래너 다이얼로그는 `qSlicerAbstractModuleWidget` 이 아닌 `qMRMLWidget` 을 상속하고 있으며, `qMRMLWidget` 이 상속하는 Qt `QWidget` 레이어에는 `qvtkConnect` 편의 API가 존재하지 않음.
- **해결**: 다이얼로그 헤더에 **`ctkVTKObject.h`** 및 **`QVTK_OBJECT`** 매크로를 명시적으로 주입하여 `qvtkConnect` 와 `qvtkDisconnect` 멤버 인터페이스를 정상 활성화해 해결 완료.

## 5. Cannot open include file 'vtkEventQtConnector.h' (`C1083`)
- **현상**: `#include <vtkEventQtConnector.h>` 선언 시 파일을 찾을 수 없다는 컴파일 에러 발생.
- **원인**: 해당 헤더는 VTK의 Qt 연동 전용 모듈(`GUISupportQt`)에 들어있으나, 현재 모듈의 CMake 종속성 인클루드 경로에 해당 라이브러리가 명시적으로 바인딩되어 있지 않아 컴파일 에러 발생.
- **해결**: 외부 VTK 모듈을 추가 링킹 종속성으로 땡겨오는 대신, Slicer가 내부 빌드 패스에 기본 인클루드 경로를 전파해주는 내장 매커니즘(`QVTK_OBJECT`)을 온전히 활성화하여 `this->qvtkConnect` 로 이벤트를 처리해 해결 완료.

## 6. Slicer 종료 시 VTK 메모리 누수 경고 (`vtkDebugLeaks has detected LEAKS!`)
- **현상**: 3D Slicer 어플리케이션 종료 시 `vtkTimerLog`, `vtkObserverManager`, `vtkMRMLNodeReference`, `vtkCommand` 등의 VTK 메모리 잔여 누수 경고 창 팝업.
- **원인**: 
  1. 다이얼로그 소멸 시 세그멘테이션 에디터 위젯(`d->SegmentEditorWidget`)을 비동기식 `deleteLater()` 로 소멸시키도록 되어 있었으나, Slicer 종료 시에는 Qt 이벤트 루프가 먼저 중단되므로 파괴자가 아예 실행되지 않고 누수 발생.
  2. 다이얼로그 내의 테이블 행 조작 시 등록되었던 다양한 VTK 관찰자(Observer)들이 소멸자 시점에서 개별 해제되지 않고 남아 있었음.
  3. 다이얼로그가 `qMRMLWidget` 부모 클래스로서 Slicer 씬(`vtkMRMLScene`)에 바인딩될 때 등록되는 수많은 내장 옵저버들이 소멸 시점에 해제되지 않았음. 다이얼로그의 `setMRMLScene(scene)` 구현에 `if (!scene) return;` 방어 코드가 맨 상단에 선언되어 있어, `setMRMLScene(nullptr)`이 들어왔을 때 하위 위젯으로의 해제 처리가 조기에 차단되었고, 부모 클래스의 `setMRMLScene(scene)` 호출도 누락되어 있었음.
- **해결**: 
  1. 소멸자 내 세그멘테이션 에디터 위젯 해제 구문을 **`delete d->SegmentEditorWidget;`** 으로 동기식 강제 소멸하도록 교정.
  2. 다이얼로그 소멸자 진입 시 즉각 **`this->setMRMLScene(nullptr);`** 을 명시적으로 호출하고, `setMRMLScene` 구현에 **`this->qMRMLWidget::setMRMLScene(scene);`** 부모 클래스 호출을 복구함과 동시에 `nullptr` 이 들어왔을 때 하위 자식 위젯들에 전파되도록 파이프라인 방어 코드를 교정함.
  3. 다이얼로그 소멸자 맨 하단에 **`this->qvtkDisconnectAll();`** 을 호출하여 다이얼로그 생명주기 동안 연결된 모든 관찰자들을 최종 정리.



