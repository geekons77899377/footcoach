# Écrit OUT (en-tête C++) avec le SHA-1 du commit compilé. Lancé à CHAQUE compilation (cible footcoach_git_revision),
# pas seulement à la configuration : un nouveau commit sans reconfiguration est donc pris en compte.
# Le fichier n'est réécrit que s'il change, pour ne pas recompiler inutilement.
# Sans git ou hors dépôt : "inconnu".
set(sha "inconnu")
set(dirty 0)
find_program(GIT_EXECUTABLE git)
if(GIT_EXECUTABLE)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD WORKING_DIRECTORY "${SRC_DIR}"
                    OUTPUT_VARIABLE out RESULT_VARIABLE rc OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(rc EQUAL 0 AND out MATCHES "^[0-9a-f]+$")
        set(sha "${out}")
        # Fichiers suivis modifiés ou non commités : le SHA ne décrit alors pas exactement ce qui est compilé.
        execute_process(COMMAND "${GIT_EXECUTABLE}" status --porcelain --untracked-files=no WORKING_DIRECTORY "${SRC_DIR}"
                        OUTPUT_VARIABLE st RESULT_VARIABLE rc2 OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        if(rc2 EQUAL 0 AND NOT st STREQUAL "")
            set(dirty 1)
        endif()
    endif()
endif()
set(content "// Généré par cmake/GitRevision.cmake : ne pas modifier.\n#pragma once\n#define FOOTCOACH_GIT_SHA \"${sha}\"\n#define FOOTCOACH_GIT_DIRTY ${dirty}\n")
set(old "")
if(EXISTS "${OUT}")
    file(READ "${OUT}" old)
endif()
if(NOT old STREQUAL content)
    file(WRITE "${OUT}" "${content}")
endif()
