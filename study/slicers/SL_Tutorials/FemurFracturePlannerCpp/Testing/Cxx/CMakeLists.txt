set(KIT qSlicer${MODULE_NAME}Module)

#-----------------------------------------------------------------------------
set(KIT_TEST_SRCS
  vtkSlicer${MODULE_NAME}LogicTest.cxx
  )

#-----------------------------------------------------------------------------
slicerMacroConfigureModuleCxxTestDriver(
  NAME ${KIT}
  SOURCES ${KIT_TEST_SRCS}
  TARGET_LIBRARIES vtkSlicer${MODULE_NAME}ModuleLogic
  WITH_VTK_DEBUG_LEAKS_CHECK
  WITH_VTK_ERROR_OUTPUT_CHECK
  )

#-----------------------------------------------------------------------------
simple_test(vtkSlicer${MODULE_NAME}LogicTest)
