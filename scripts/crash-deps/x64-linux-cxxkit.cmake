# Overlay triplet for crash-deps export (internal to scripts/export_crash_deps.sh).
# -fPIC is a hard requirement: libbreakpad_client.a gets linked into shared
# libraries (cxxkit shared build), so all static code must be PIC.
# Functionally a Linux guest triplet — vcpkg would otherwise treat the unknown '-cxxkit' suffix as Windows
# ('Use of Visual Studio's Developer Prompt is unsupported on non-Windows hosts').
set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_C_FLAGS "${VCPKG_CMAKE_C_FLAGS} -fPIC")
set(VCPKG_CMAKE_CXX_FLAGS "${VCPKG_CMAKE_CXX_FLAGS} -fPIC")
