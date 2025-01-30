cpmaddpackage(
        NAME
        freetype
        GIT_REPOSITORY
        https://github.com/aseprite/freetype2.git
        GIT_TAG
        VER-2-10-0
        VERSION
        2.10.0)

if (freetype_ADDED)
    add_library(Freetype::Freetype ALIAS freetype)
endif ()