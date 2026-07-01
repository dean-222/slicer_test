# 최소 필수 CMake 버전 설정
cmake_minimum_required(VERSION 3.20.6...3.22.6 FATAL_ERROR)

# 익스텐션(Extension) 프로젝트 이름 설정
project(SL_Tutorials)

if(MSVC)
  add_compile_options(/utf-8)
endif()

#-----------------------------------------------------------------------------
# 익스텐션 메타 정보 설정 (Slicer Extension Manager 등에 표시되는 정보)
#-----------------------------------------------------------------------------
# 익스텐션 홈페이지 주소
set(EXTENSION_HOMEPAGE "https://www.slicer.org/wiki/Documentation/Nightly/Extensions/SL_Tutorials")
# 개발자/기여자 정보
set(EXTENSION_CONTRIBUTORS "Dean (ETS)")
# 익스텐션 간단 설명
set(EXTENSION_DESCRIPTION "This is an example of a simple extension")
# 익스텐션 아이콘 URL
set(EXTENSION_ICONURL "https://www.example.com/Slicer/Extensions/SL_Tutorials.png")
# 익스텐션 스크린샷 이미지 URL
set(EXTENSION_SCREENSHOTURLS "https://www.example.com/Slicer/Extensions/SL_Tutorials/Screenshots/1.png")
# 의존하는 타 익스텐션 목록 (의존성 없을 경우 "NA")
set(EXTENSION_DEPENDS "NA") 

#-----------------------------------------------------------------------------
# 익스텐션 의존성 설정
#-----------------------------------------------------------------------------
# 3D Slicer 패키지를 필수(REQUIRED)로 검색
find_package(Slicer REQUIRED)
# Slicer의 컴파일 및 빌드 옵션(CMake 설정 등)을 로드
include(${Slicer_USE_FILE})

#-----------------------------------------------------------------------------
# 익스텐션 내에 포함시킬 개별 모듈 추가
#-----------------------------------------------------------------------------
# t_ApplyThreshold 디렉토리를 하위 빌드 디렉토리로 추가 (모듈 빌드 대상 포함)
add_subdirectory(t_ApplyThreshold)
add_subdirectory(FemurFracturePlanner)
add_subdirectory(FemurFracturePlannerCpp)
## NEXT_MODULE

#-----------------------------------------------------------------------------
# Slicer 빌드 설정 파일 생성 및 CPack 패키징 마크로 포함
include(${Slicer_EXTENSION_GENERATE_CONFIG})
include(${Slicer_EXTENSION_CPACK})
