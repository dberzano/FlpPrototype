# Set default installation path for O2
set(O2_INSTALL_PREFIX "/usr/local")

# Overwrite CMAKE_INSTALL_PREFIX
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX
            ${O2_INSTALL_PREFIX} CACHE PATH "O2 install prefix" FORCE
    )
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

set(CMAKE_CXX_STANDARD 11)
