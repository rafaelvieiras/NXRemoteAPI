#---------------------------------------------------------------------------------
.SUFFIXES:

TARGET		:=	nxremoteapi_orchestrator
BUILD		:=	build
SOURCES		:=	.
INCLUDES	:=	include

#---------------------------------------------------------------------------------
DEVKITPRO	?=	/opt/devkitpro
PREFIX		:=	$(DEVKITPRO)/devkitA64/bin/aarch64-none-elf-
ARCH		:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=el0 -fPIE

CC		:=	$(PREFIX)gcc
CXX		:=	$(PREFIX)g++
LD		:=	$(PREFIX)gcc
OBJCOPY	:=	$(PREFIX)objcopy
STRIP	:=	$(PREFIX)strip
NM		:=	$(PREFIX)nm

#---------------------------------------------------------------------------------
DEFINES	:=	-D__SWITCH__
CFLAGS	:=	-g -Wall -Wextra -O2 -ffunction-sections -fPIE $(ARCH) $(DEFINES) -I$(CURDIR)/../common/include -isystem $(DEVKITPRO)/libnx/include -isystem $(DEVKITPRO)/portlibs/switch/include
CXXFLAGS:=	$(CFLAGS) -std=c++17 -fno-rtti -fno-exceptions
LDFLAGS	:=	-specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -pie -Wl,-Map,$(notdir $*.map) -L$(DEVKITPRO)/libnx/lib -L$(DEVKITPRO)/portlibs/switch/lib

# No curl/mbedtls/z here on purpose - the orchestrator never talks to the network
# beyond loopback health checks and its own tiny control port, unlike core.
LIBS	:= -lnx

#---------------------------------------------------------------------------------
all: $(TARGET).nso

$(TARGET).nso: $(TARGET).elf
	$(DEVKITPRO)/tools/bin/elf2nso $< $@
	@echo built ... $@


$(TARGET).elf: main.cpp ../common/ConfigManager.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

# main.npdm is committed (see AGENTS.md Rule #3) but reproducible from main.json via
# this rule - regenerate it if main.json ever changes (program ID, permissions, etc).
main.npdm: main.json
	$(DEVKITPRO)/tools/bin/npdmtool $< $@
	@echo built ... $@

# Same packaging requirement as firmware/core - see the comment there and ADR-0009.
# boot2.flag (in flags/, not packed here) is what actually makes Atmosphère launch
# this from boot2, not the exefs.nsp itself.
exefs.nsp: $(TARGET).nso main.npdm
	@rm -rf pfs0_staging && mkdir -p pfs0_staging
	@cp $(TARGET).nso pfs0_staging/main
	@cp main.npdm pfs0_staging/main.npdm
	$(DEVKITPRO)/tools/bin/build_pfs0 pfs0_staging exefs.nsp
	@rm -rf pfs0_staging
	@echo built ... exefs.nsp

pack: exefs.nsp

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nro $(TARGET).nso $(TARGET).nsp exefs.nsp pfs0_staging $(TARGET).map
