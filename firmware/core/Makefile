#---------------------------------------------------------------------------------
.SUFFIXES:

TARGET		:=	nxremoteapi_core
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

# core never calls into curl/mbedtls/zlib at runtime (verified: no CURL*/curl_*
# symbol is referenced anywhere in main.cpp - unlike firmware/companion-app, which
# actually uses libcurl for GitHub release checks/downloads). The static linker
# already only pulls in .o files a build actually references, so this LIBS line
# was link-time inert either way (confirmed: identical .nso size with or without
# these) - dropped for clarity, matching the orchestrator's `-lnx`-only precedent
# in AGENTS.md Rule #6, not because it was bloating the shipped binary.
LIBS	:= -lnx

#---------------------------------------------------------------------------------
all: $(TARGET).nso

$(TARGET).nso: $(TARGET).elf
	$(DEVKITPRO)/tools/bin/elf2nso $< $@
	@echo built ... $@


$(TARGET).elf: main.cpp ../common/ConfigManager.cpp ../common/HttpFraming.cpp ../common/ByteSearch.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

# main.npdm is committed (see AGENTS.md Rule #3) but reproducible from main.json via
# this rule - regenerate it if main.json ever changes (program ID, permissions, etc).
main.npdm: main.json
	$(DEVKITPRO)/tools/bin/npdmtool $< $@
	@echo built ... $@

# Atmosphère only loads this sysmodule from a packed exefs.nsp (PFS0) at
# atmosphere/contents/010000000000CAFE/exefs.nsp - the loose exefs/main+main.npdm
# directory layout (also valid Atmosphère syntax in general) was tested on real
# hardware 2026-08-08/09 and silently failed to load (see ADR-0009). Always deploy
# this file, never the loose layout, for this project's install.
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
