cmake_minimum_required(VERSION 3.21)
project(FootCoach VERSION 0.8.0 LANGUAGES CXX)

# L'application cible Windows et Android. L'option ci-dessous est réservée aux vérifications de compilation
# sur d'autres systèmes (CI, Linux).
option(FOOTCOACH_ALLOW_NON_WINDOWS "Autoriser la configuration hors Windows et Android (compilation de contrôle)" OFF)
option(FOOTCOACH_BUILD_TESTS "Compiler les tests Qt Test" ON)

if(NOT WIN32 AND NOT ANDROID AND NOT FOOTCOACH_ALLOW_NON_WINDOWS)
    message(FATAL_ERROR "FootCoach cible Windows et Android.")
endif()

# Pas de tests Qt Test dans l'APK : ils se lancent sur le PC.
if(ANDROID)
    set(FOOTCOACH_BUILD_TESTS OFF)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

# Application QML (cible principale) : Qt 6.9 minimum (SafeArea). Les tests et l'ancienne version Widgets
# restent compilables avec Qt 6.4 si Qt Quick n'est pas installé.
set(QT_COMPONENTS Widgets Sql)
if(FOOTCOACH_BUILD_TESTS)
    list(APPEND QT_COMPONENTS Test)
endif()
find_package(Qt6 6.4 REQUIRED COMPONENTS ${QT_COMPONENTS})
find_package(Qt6 6.9 QUIET COMPONENTS Quick QuickControls2)
if(Qt6Quick_FOUND AND Qt6QuickControls2_FOUND)
    set(FOOTCOACH_HAVE_QML ON)
else()
    set(FOOTCOACH_HAVE_QML OFF)
    message(WARNING "Qt Quick / Quick Controls 2 (Qt 6.9+) introuvables : seule la version Widgets est compilée.")
endif()

# Sources partagées entre l'application et les tests.
set(CORE_SOURCES
    src/core/Models.h
    src/core/Database.h
    src/core/Database.cpp
    src/core/TeamRepository.h
    src/core/TeamRepository.cpp
    src/core/PlayerRepository.h
    src/core/PlayerRepository.cpp
    src/core/Recurrence.h
    src/core/Recurrence.cpp
    src/core/SessionRepository.h
    src/core/SessionRepository.cpp
    src/core/AttendanceStats.h
    src/core/AttendanceStats.cpp
    src/core/AttendanceRepository.h
    src/core/AttendanceRepository.cpp
    src/core/ExerciseRepository.h
    src/core/ExerciseRepository.cpp
    src/core/PlanRepository.h
    src/core/PlanRepository.cpp
    src/core/PlanSchedule.h
    src/core/PlanSchedule.cpp
    src/core/SessionSheet.h
    src/core/SessionSheet.cpp
    src/core/TacticalBoard.h
    src/core/TacticalBoard.cpp
    src/core/BoardRepository.h
    src/core/BoardRepository.cpp
)

# Migrations SQL embarquées (accessibles via ":/sql/...").
set(SQL_MIGRATIONS
    sql/001_init.sql
    sql/002_player_jersey_unique.sql
    sql/003_exercise_archived.sql
    sql/004_seed_exercises.sql
)

qt_add_executable(FootCoachWidgets WIN32
    src/main.cpp
    ${CORE_SOURCES}
    src/ui/Labels.h
    src/ui/Platform.h
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/PlayerTableModel.h
    src/ui/PlayerTableModel.cpp
    src/ui/PlayerDialog.h
    src/ui/PlayerDialog.cpp
    src/ui/TeamDialog.h
    src/ui/TeamDialog.cpp
    src/ui/SquadPage.h
    src/ui/SquadPage.cpp
    src/ui/SessionDialog.h
    src/ui/SessionDialog.cpp
    src/ui/RecurrenceDialog.h
    src/ui/RecurrenceDialog.cpp
    src/ui/CalendarPage.h
    src/ui/CalendarPage.cpp
    src/ui/AttendanceDialog.h
    src/ui/AttendanceDialog.cpp
    src/ui/AttendancePage.h
    src/ui/AttendancePage.cpp
    src/ui/ExerciseDialog.h
    src/ui/ExerciseDialog.cpp
    src/ui/ExercisePicker.h
    src/ui/ExercisePicker.cpp
    src/ui/ExercisesPage.h
    src/ui/ExercisesPage.cpp
    src/ui/PlanItemDialog.h
    src/ui/PlanItemDialog.cpp
    src/ui/PdfExport.h
    src/ui/PdfExport.cpp
    src/ui/SessionsPage.h
    src/ui/SessionsPage.cpp
    src/ui/BoardScene.h
    src/ui/BoardScene.cpp
    src/ui/BoardPropertiesDialog.h
    src/ui/BoardPropertiesDialog.cpp
    src/ui/TacticsPage.h
    src/ui/TacticsPage.cpp
)

qt_add_resources(FootCoachWidgets "sql"
    PREFIX "/sql"
    BASE "sql"
    FILES ${SQL_MIGRATIONS}
)

target_include_directories(FootCoachWidgets PRIVATE src)
target_compile_definitions(FootCoachWidgets PRIVATE FOOTCOACH_VERSION=\"${PROJECT_VERSION}\")
target_link_libraries(FootCoachWidgets PRIVATE Qt6::Widgets Qt6::Sql)
if(MSVC)
    target_compile_options(FootCoachWidgets PRIVATE /W4 /permissive-)
endif()
# Ancienne interface Widgets : inutile sur Android, retirée quand toutes les pages seront portées en QML.
if(ANDROID)
    set_target_properties(FootCoachWidgets PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

if(FOOTCOACH_HAVE_QML)
    qt_policy(SET QTP0001 NEW)   # préfixe de ressources /qt/qml/
    qt_add_executable(FootCoach WIN32
        src/main_qml.cpp
        ${CORE_SOURCES}
        src/app/Backend.h
        src/app/Backend.cpp
        src/app/BackendSessions.cpp
        src/app/BackendPlan.cpp
        src/app/BackendData.cpp
    )
    qt_add_resources(FootCoach "sql"
        PREFIX "/sql"
        BASE "sql"
        FILES ${SQL_MIGRATIONS}
    )

    set(QML_FILES
        ActionSheet.qml AppState.qml AppButton.qml BoardEditor.qml BoardModel.qml BoardThumb.qml BoardToken.qml
        BoardView.qml Card.qml ColorSheet.qml FormationSheet.qml HomePage.qml Icon.qml IconButton.qml
        Main.qml PageHeader.qml PlaceholderPage.qml PromptDialog.qml TacticsPage.qml Theme.qml
        CalendarPage.qml AttendancePage.qml SessionForm.qml RecurrenceForm.qml AttendanceForm.qml CalendarMonth.qml Status.js DatePicker.qml TimePicker.qml TeamSwitcher.qml DateUtil.js SquadPage.qml PlayerForm.qml TeamDialog.qml FormField.qml ChipRow.qml
        ExerciseForm.qml ExercisesPage.qml ExercisePicker.qml PlanItemDialog.qml PlanEditor.qml SessionsPage.qml Toast.qml DataPage.qml
        BoardGeometry.js Icons.js
    )
    set(QML_FILES_PATHS)
    foreach(f ${QML_FILES})
        list(APPEND QML_FILES_PATHS qml/${f})
        set_source_files_properties(qml/${f} PROPERTIES QT_RESOURCE_ALIAS ${f})
    endforeach()
    set_source_files_properties(qml/Theme.qml qml/AppState.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

    qt_add_qml_module(FootCoach
        URI FootCoachUi
        VERSION 1.0
        QML_FILES ${QML_FILES_PATHS}
    )

    target_include_directories(FootCoach PRIVATE src)
    target_compile_definitions(FootCoach PRIVATE FOOTCOACH_VERSION=\"${PROJECT_VERSION}\")
    target_link_libraries(FootCoach PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Sql)
    if(MSVC)
        target_compile_options(FootCoach PRIVATE /W4 /permissive-)
    endif()

    if(ANDROID)
        # Identité du paquet : à ne plus changer une fois l'application installée ; le code de version doit
        # augmenter à chaque nouvelle version à installer par-dessus.
        set_target_properties(FootCoach PROPERTIES
            QT_ANDROID_PACKAGE_NAME "org.footcoach.app"
            QT_ANDROID_VERSION_NAME "${PROJECT_VERSION}"
            QT_ANDROID_VERSION_CODE 14
            # Google Play exige API 36 pour les nouvelles applications depuis le 31/08/2026.
            QT_ANDROID_MIN_SDK_VERSION 23
            QT_ANDROID_TARGET_SDK_VERSION 36
            # Icône de l'application (adaptative + repli) : voir tools/make_icons.py.
            QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        )
    endif()
endif()

if(FOOTCOACH_BUILD_TESTS)
    enable_testing()

    # Un exécutable par fichier tests/tst_<nom>.cpp, avec les sources métier et les migrations.
    foreach(test_name repositories sessions attendance plan board backend)
        qt_add_executable(tst_${test_name} tests/tst_${test_name}.cpp ${CORE_SOURCES} src/app/Backend.h src/app/Backend.cpp src/app/BackendSessions.cpp src/app/BackendPlan.cpp src/app/BackendData.cpp)
        qt_add_resources(tst_${test_name} "sql_${test_name}"
            PREFIX "/sql"
            BASE "sql"
            FILES ${SQL_MIGRATIONS}
        )
        target_include_directories(tst_${test_name} PRIVATE src)
        target_link_libraries(tst_${test_name} PRIVATE Qt6::Sql Qt6::Test)
        add_test(NAME ${test_name} COMMAND tst_${test_name})
    endforeach()
endif()
