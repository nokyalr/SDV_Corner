include("/home/norriee/Documents/Kuliah/pa/stm32/SDV_Corner/build/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/LEDControl-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "/home/norriee/Documents/Kuliah/pa/stm32/SDV_Corner/build/LEDControl"
    GENERATE_QT_CONF
)
