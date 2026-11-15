# Vérifie que chaque fichier de qml/ est déclaré dans QML_FILES (CMakeLists.txt). Un fichier oublié n'est pas embarqué :
# les tests QML (qui lisent qml/ directement) passent, mais l'application s'arrête au démarrage (« X is not a type »).
# Appel : cmake -DSRC=<dossier source> -DDECLARED_PIPE="a.qml|b.qml" -P CheckQmlFiles.cmake
string(REPLACE "|" ";" DECLARED "${DECLARED_PIPE}")
file(GLOB found RELATIVE "${SRC}/qml" "${SRC}/qml/*.qml" "${SRC}/qml/*.js")
set(missing)
foreach(f ${found})
    list(FIND DECLARED "${f}" idx)
    if(idx EQUAL -1)
        list(APPEND missing "${f}")
    endif()
endforeach()
if(missing)
    message(FATAL_ERROR "Fichiers QML absents de QML_FILES dans CMakeLists.txt : ${missing}")
endif()
message(STATUS "QML_FILES complet (${found})")
