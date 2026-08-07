#if you want to build exe
#TEMPLATE = app
#CONFIG += console

lessThan(QT_MAJOR_VERSION, 6) {
    message("Cannot use Qt $${QT_VERSION}")
    error("Use Qt 6.8 or newer")
}
equals(QT_MAJOR_VERSION, 6):lessThan(QT_MINOR_VERSION, 8) {
    message("Cannot use Qt $${QT_VERSION}")
    error("Use Qt 6.8 or newer")
}

TEMPLATE = lib
QT += network
TARGET = uCNC
DEFINES += MCU=MCU_VIRTUAL_WIN
CONFIG -= debug_and_release
LIBS += -pthread
#DEFINES += WIN_INTERFACE=0

QMAKE_CFLAGS += -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-missing-field-initializers
QMAKE_CXXFLAGS += -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-missing-field-initializers

win32 {
    DEFINES += WINDOWS=1
}

unix:!macx {
    DEFINES += LINUX=1
}

HEADERS += \
    makefiles/virtual/WindowsSerial.h \
    uCNC/boardmap_overrides.h \
    uCNC/build_opt.h \
    uCNC/cnc_config.h \
    uCNC/cnc_hal_config.h \
    uCNC/cnc_hal_overrides.h \
    uCNC/src/buffer.h \
    uCNC/src/cnc.h \
    uCNC/src/cnc_build.h \
    uCNC/src/cnc_hal_config_helper.h \
    uCNC/src/core/interpolator.h \
    uCNC/src/core/io_control.h \
    uCNC/src/core/motion_control.h \
    uCNC/src/core/parser.h \
    uCNC/src/core/planner.h \
    uCNC/src/hal/boards/boarddefs.h \
    uCNC/src/hal/boards/boards_helper.h \
    uCNC/src/hal/boards/pin_mapping_helper.h \
    uCNC/src/hal/io_hal.h \
    uCNC/src/hal/kinematics/kinematic.h \
    uCNC/src/hal/kinematics/kinematic_cartesian.h \
    uCNC/src/hal/kinematics/kinematic_corexy.h \
    uCNC/src/hal/kinematics/kinematic_delta.h \
    uCNC/src/hal/kinematics/kinematic_linear_delta.h \
    uCNC/src/hal/kinematics/kinematic_scara.h \
    uCNC/src/hal/kinematics/kinematicdefs.h \
    uCNC/src/hal/kinematics/kinematics.h \
    uCNC/src/hal/mcus/mcu.h \
    uCNC/src/hal/mcus/mcudefs.h \
    uCNC/src/hal/mcus/mcus.h \
    uCNC/src/hal/mcus/virtual/mcumap_virtual.h \
    uCNC/src/hal/tools/tool.h \
    uCNC/src/hal/tools/tool_helper.h \
    uCNC/src/interface/defaults.h \
    uCNC/src/interface/grbl_interface.h \
    uCNC/src/interface/grbl_print.h \
    uCNC/src/interface/grbl_protocol.h \
    uCNC/src/interface/grbl_settings.h \
    uCNC/src/interface/grbl_stream.h \
    uCNC/src/interface/serial_compatibility.h \
    uCNC/src/module.h \
    uCNC/src/modules/astrocore_sim.h \
    uCNC/src/modules/digimstep.h \
    uCNC/src/modules/digipot.h \
    uCNC/src/modules/encoder.h \
    uCNC/src/modules/endpoint.h \
    uCNC/src/modules/file_system.h \
    uCNC/src/modules/ic74hc165.h \
    uCNC/src/modules/ic74hc595.h \
    uCNC/src/modules/language/language_en.h \
    uCNC/src/modules/modbus.h \
    uCNC/src/modules/pid.h \
    uCNC/src/modules/shift_register.h \
    uCNC/src/modules/softi2c.h \
    uCNC/src/modules/softspi.h \
    uCNC/src/modules/softuart.h \
    uCNC/src/modules/system_languages.h \
    uCNC/src/modules/system_menu.h \
    uCNC/src/modules/websocket.h \
    uCNC/src/utils.h \
    uCNC/uCNC.ino

SOURCES += \
    makefiles/virtual/WindowsSerial.cpp \
    makefiles/virtual/mcu_virtual.cpp \
    uCNC/src/buffer.c \
    uCNC/src/cnc.c \
    uCNC/src/core/interpolator.c \
    uCNC/src/core/io_control.c \
    uCNC/src/core/motion_control.c \
    uCNC/src/core/parser.c \
    uCNC/src/core/planner.c \
    uCNC/src/hal/kinematics/kinematic.c \
    uCNC/src/hal/kinematics/kinematic_cartesian.c \
    uCNC/src/hal/kinematics/kinematic_corexy.c \
    uCNC/src/hal/kinematics/kinematic_delta.c \
    uCNC/src/hal/kinematics/kinematic_linear_delta.c \
    uCNC/src/hal/kinematics/kinematic_scara.c \
    uCNC/src/hal/mcus/mcu.c \
    uCNC/src/hal/tools/tool.c \
    uCNC/src/hal/tools/tools/laser_ppi.c \
    uCNC/src/hal/tools/tools/laser_pwm.c \
    uCNC/src/hal/tools/tools/pen_servo.c \
    uCNC/src/hal/tools/tools/plasma_thc.c \
    uCNC/src/hal/tools/tools/spindle_besc.c \
    uCNC/src/hal/tools/tools/spindle_pwm.c \
    uCNC/src/hal/tools/tools/spindle_relay.c \
    uCNC/src/hal/tools/tools/vfd_modbus.c \
    uCNC/src/hal/tools/tools/vfd_pwm.c \
    uCNC/src/interface/grbl_print.c \
    uCNC/src/interface/grbl_protocol.c \
    uCNC/src/interface/grbl_settings.c \
    uCNC/src/interface/grbl_stream.c \
    uCNC/src/module.c \
    uCNC/src/modules/astrocore_sim.c \
    uCNC/src/modules/digimstep.c \
    uCNC/src/modules/digipot.c \
    uCNC/src/modules/encoder.c \
    uCNC/src/modules/file_system.c \
    uCNC/src/modules/modbus.c \
    uCNC/src/modules/pid.c \
    uCNC/src/modules/shift_register.c \
    uCNC/src/modules/softi2c.c \
    uCNC/src/modules/softspi.c \
    uCNC/src/modules/softuart.c \
    uCNC/src/modules/system_menu.c

DISTFILES += \
    uCNC/README.md \
    uCNC/src/hal/boards/.gitignore \
    uCNC/src/hal/kinematics/README.md \
    uCNC/src/hal/tools/README.md \
    uCNC/src/hal/tools/tools/README.md \
    uCNC/src/modules/system_menu.md

DESTDIR = $$OUT_PWD/../../astrocore
