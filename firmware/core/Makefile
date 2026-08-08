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


$(TARGET).elf: main.cpp ../common/ConfigManager.cpp ../common/HttpFraming.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nro $(TARGET).nso $(TARGET).nsp $(TARGET).map
