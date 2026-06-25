# File Name: CMakeLists.txt
# Version: v0-0.4.0
# Date: 2026-06-24
# Description: FemurFracturePlanner scripted module build configuration.
#
# Version History:
# - v0-0.4.0 (2026-06-24)
#   - 보조 Python 스크립트를 FemurFracturePlannerLib/ 패키지 하위로 이동하여 Slicer 모듈 디스커버리 충돌 해결
# - v0-0.3.0 (2026-06-23)
#   - FemurFracturePlannerWorkflow.py 스크립트를 추가하여 입력 및 작업 흐름 로직을 별도 파일로 분리
# - v0-0.2.0 (2026-06-23)
#   - FemurFracturePlannerFragmentManager.py 스크립트를 추가하여 Fragment Manager 로직을 별도 파일로 분리
# - v0-0.1.0 (2026-06-23)
#   - FemurFracturePlannerWidget.py 스크립트를 추가하여 위젯 클래스를 별도 파일로 분리
# - v0-0.0.0 (2026-06-23)
#   - 최초 버전 등록 및 FemurFracturePlannerLogic.py 스크립트 패키지 추가

#-----------------------------------------------------------------------------
# 모듈 이름 정의
#-----------------------------------------------------------------------------
set(MODULE_NAME FemurFracturePlanner)

#-----------------------------------------------------------------------------
# 모듈에 포함할 Python 스크립트 목록
#-----------------------------------------------------------------------------
set(MODULE_PYTHON_SCRIPTS
  ${MODULE_NAME}.py
  ${MODULE_NAME}Lib/__init__.py
  ${MODULE_NAME}Lib/${MODULE_NAME}Logic.py
  ${MODULE_NAME}Lib/${MODULE_NAME}Widget.py
  ${MODULE_NAME}Lib/${MODULE_NAME}FragmentManager.py
  ${MODULE_NAME}Lib/${MODULE_NAME}Workflow.py
  )

#-----------------------------------------------------------------------------
# 모듈 리소스 파일 목록
#-----------------------------------------------------------------------------
set(MODULE_PYTHON_RESOURCES
  Resources/Icons/${MODULE_NAME}.png
  Resources/UI/${MODULE_NAME}.ui
  )

#-----------------------------------------------------------------------------
# Slicer scripted module 빌드 등록
#-----------------------------------------------------------------------------
slicerMacroBuildScriptedModule(
  NAME ${MODULE_NAME}
  SCRIPTS ${MODULE_PYTHON_SCRIPTS}
  RESOURCES ${MODULE_PYTHON_RESOURCES}
  WITH_GENERIC_TESTS
  )

#-----------------------------------------------------------------------------
# 테스트 빌드가 켜져 있을 때 unittest 등록
#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  # 메인 스크립트 파일 안의 ScriptedLoadableModuleTest 서브클래스를 CTest에 등록한다.
  slicer_add_python_unittest(SCRIPT ${MODULE_NAME}.py)
endif()
