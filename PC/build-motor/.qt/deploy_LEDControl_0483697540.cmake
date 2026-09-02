include("C:/Users/pb123/Documents/Kuliah/pa/stm32/SDV_Corner/PC/build-motor/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/LEDControl-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "C:/Users/pb123/Documents/Kuliah/pa/stm32/SDV_Corner/PC/build-motor/LEDControl.exe"
    GENERATE_QT_CONF
)
