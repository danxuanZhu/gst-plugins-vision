# - Try to find Balluff Impact Acquire SDK
# Once done this will define
#
#  IMPACT_ACQUIRE_FOUND - system has Impact Acquire SDK

# @copyright: Copyright [2023] Balluff GmbH, all rights reserved
# @description: CMake module to find the Balluff Impact Acquire SDK if it has been installed on the system.
# @authors: Danxuan Zhu <danxuan.zhu@balluff.de>
# @initial data: 2023-11-15
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

set (IMPACT_ACQUIRE_DIR $ENV{MVIMPACT_ACQUIRE_DIR} CACHE PATH "Directory containing Balluff Impact Acquire SDK includes and libraries")

set (IMPACT_ACQUIRE_INCLUDE_DIR ${IMPACT_ACQUIRE_DIR})

find_library (IMPACT_ACQUIRE_LIBRARIES NAMES mvDeviceManager
    PATHS
    ${IMPACT_ACQUIRE_DIR}/lib/*
    $ENV{MVIMPACT_ACQUIRE_BUILD_DIR}/lib
    )

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (IMPACT_ACQUIRE  DEFAULT_MSG  IMPACT_ACQUIRE_INCLUDE_DIR IMPACT_ACQUIRE_LIBRARIES)