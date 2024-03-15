#if you want to build exe
#TEMPLATE = app
#CONFIG += console

TEMPLATE = lib
QT += network
TARGET = uCNC
DEFINES += BOARD=BOARD_VIRTUAL
DEFINES += MCU=MCU_VIRTUAL_WIN
CONFIG -= debug_and_release
#DEFINES += WIN_INTERFACE=0

HEADERS += \
    makefiles/virtual/WindowsSerial.h \
    uCNC/boardmap_overrides.h \
    uCNC/build_opt.h \
    uCNC/cnc_config.h \
    uCNC/cnc_hal_config.h \
    uCNC/cnc_hal_overrides.h \
    uCNC/src/cnc.h \
    uCNC/src/cnc_build.h \
    uCNC/src/cnc_hal_config_helper.h \
    uCNC/src/core/interpolator.h \
    uCNC/src/core/io_control.h \
    uCNC/src/core/motion_control.h \
    uCNC/src/core/parser.h \
    uCNC/src/core/planner.h \
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
    uCNC/src/interface/protocol.h \
    uCNC/src/interface/serial.h \
    uCNC/src/interface/settings.h \
    uCNC/src/module.h \
    uCNC/src/modules/digimstep.h \
    uCNC/src/modules/digipot.h \
    uCNC/src/modules/encoder.h \
    uCNC/src/modules/endpoint.h \
    uCNC/src/modules/ic74hc595.h \
    uCNC/src/modules/language/language_en.h \
    uCNC/src/modules/modbus.h \
    uCNC/src/modules/pid.h \
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
    uCNC/src/interface/protocol.c \
    uCNC/src/interface/serial.c \
    uCNC/src/interface/settings.c \
    uCNC/src/module.c \
    uCNC/src/modules/digimstep.c \
    uCNC/src/modules/digipot.c \
    uCNC/src/modules/encoder.c \
    uCNC/src/modules/ic74hc595.c \
    uCNC/src/modules/modbus.c \
    uCNC/src/modules/pid.c \
    uCNC/src/modules/softi2c.c \
    uCNC/src/modules/softspi.c \
    uCNC/src/modules/softuart.c \
    uCNC/src/modules/system_menu.c

DISTFILES += \
    uCNC/README.md \
    uCNC/src/hal/kinematics/README.md \
    uCNC/src/hal/tools/README.md \
    uCNC/src/hal/tools/tools/README.md \
    uCNC/src/modules/system_menu.md

