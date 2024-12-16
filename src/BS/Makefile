# name of your application
APPLICATION = Demo_TCC_BS

# If no BOARD is found in the environment, use this default:
BOARD ?= pulga

# default serial port
PORT ?= /dev/ttyUSB0

#
WERROR ?= 0

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../../..

# Include NimBLE
USEPKG += nimble
USEMODULE += nimble_svc_gap
USEMODULE += nimble_svc_gatt
USEMODULE += nimble_scanner
USEMODULE += nimble_svc_gap
USEMODULE += nimble_svc_gatt
USEMODULE += bme280_i2c

# USEPKG += nanopb


# Use automated advertising
USEMODULE += nimble_autoadv
USEMODULE += nimble_scanner
USEMODULE += nimble_scanlist


CFLAGS += -DNIMBLE_AUTOADV_DEVICE_NAME='"MARMOSET 6157"'
CFLAGS += -DNIMBLE_AUTOADV_START_MANUALLY=1
CFLAGS += -DNIMBLE_AUTOADV_FLAG_FIELD=0
# Use autoconn
# USEMODULE += nimble_netif
# USEMODULE += nimble_autoconn
# CFLAGS += -NIMBLE_AUTOCONN_NODE_ID='"MARMOSET 666"'



SENSORS += accelerometer

#BMI160 MODULE
ifeq (pulga,$(BOARD))
FEATURES_REQUIRED += periph_i2c
#USEMODULE += i2c
endif

# Modules to include:
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += printf_float
USEMODULE += ztimer_no_periph_rtt
# USEMODULE += ztimer
USEMODULE += ztimer_msec
USEMODULE += rtt_rtc
USEMODULE += event_timeout_ztimer

# USEMODULE += xtimer


# FEATURES_OPTIONAL += periph_wdt



ifneq (,$(filter accelerometer,$(SENSORS)))
CFLAGS += -DPULGA_ENABLE_ACCELEROMETER=1
endif

CFLAGS += -DPULGA_USE_RINGBUFFER=1

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
