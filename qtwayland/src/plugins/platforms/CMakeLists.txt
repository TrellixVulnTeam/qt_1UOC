# Generated from platforms.pro.

add_subdirectory(qwayland-generic)
if(QT_FEATURE_wayland_egl)
    add_subdirectory(qwayland-egl)
endif()
if(QT_FEATURE_wayland_brcm)
    add_subdirectory(qwayland-brcm-egl)
endif()
