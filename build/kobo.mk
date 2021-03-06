KOBO_MENU_SOURCES = \
	$(SRC)/Version.cpp \
	$(SRC)/Formatter/HexColor.cpp \
	$(SRC)/Hardware/CPU.cpp \
	$(SRC)/Hardware/DisplayDPI.cpp \
	$(SRC)/Hardware/RotateDisplay.cpp \
	$(SRC)/Screen/Layout.cpp \
	$(SRC)/Screen/TerminalWindow.cpp \
	$(SRC)/Look/TerminalLook.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/IconLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Gauge/LogoView.cpp \
	$(SRC)/Dialogs/DialogSettings.cpp \
	$(SRC)/Dialogs/WidgetDialog.cpp \
	$(SRC)/Dialogs/HelpDialog.cpp \
	$(SRC)/Dialogs/Message.cpp \
	$(SRC)/Dialogs/TextEntry.cpp \
	$(SRC)/Dialogs/KnobTextEntry.cpp \
	$(SRC)/Dialogs/TouchTextEntry.cpp \
	$(SRC)/Dialogs/SimulatorPromptWindow.cpp \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(TEST_SRC_DIR)/FakeLanguage.cpp \
	$(SRC)/Kobo/WPASupplicant.cpp \
	$(SRC)/Kobo/System.cpp \
	$(SRC)/Kobo/NetworkDialog.cpp \
	$(SRC)/Kobo/SDCardSync.cpp \
	$(SRC)/Kobo/WPASupplicant.cpp \
	$(SRC)/Kobo/WifiDialog.cpp \
	$(SRC)/Event/Shared/Timer.cpp \
	$(SRC)/Net/IpAddress.cpp \
	$(SRC)/Kobo/FakeSymbols.cpp \
	$(SRC)/Kobo/KoboMenu.cpp
KOBO_MENU_LDADD = $(FAKE_LIBS)
KOBO_MENU_DEPENDS = WIDGET FORM SCREEN EVENT RESOURCE ASYNC OS THREAD MATH UTIL
KOBO_MENU_STRIP = y

$(eval $(call link-program,KoboMenu,KOBO_MENU))

ifeq ($(TARGET),UNIX)
OPTIONAL_OUTPUTS += $(KOBO_MENU_BIN)
endif

ifeq ($(TARGET_IS_KOBO),y)

KOBO_POWER_OFF_SOURCES = \
	$(TEST_SRC_DIR)/Fonts.cpp \
	$(SRC)/Hardware/RotateDisplay.cpp \
	$(SRC)/Logger/FlightParser.cpp \
	$(SRC)/Renderer/FlightListRenderer.cpp \
	$(SRC)/FlightInfo.cpp \
	$(SRC)/Thread/Mutex.cpp \
	$(SRC)/Version.cpp \
	$(SRC)/Kobo/PowerOff.cpp
KOBO_POWER_OFF_LDADD = $(FAKE_LIBS)
KOBO_POWER_OFF_DEPENDS = SCREEN RESOURCE IO OS UTIL TIME
KOBO_POWER_OFF_STRIP = y
$(eval $(call link-program,PowerOff,KOBO_POWER_OFF))
OPTIONAL_OUTPUTS += $(KOBO_POWER_OFF_BIN)

BITSTREAM_VERA_DIR ?= $(shell \
if [ -d /usr/share/fonts/truetype/ttf-bitstream-vera ]; then \
	echo /usr/share/fonts/truetype/ttf-bitstream-vera; \
elif [ -d /usr/share/fonts/bitstream-vera ]; then \
	echo /usr/share/fonts/bitstream-vera; \
fi)
BITSTREAM_VERA_NAMES = Vera VeraBd VeraIt VeraBI VeraMono
BITSTREAM_VERA_FILES = $(patsubst %,$(BITSTREAM_VERA_DIR)/%.ttf,$(BITSTREAM_VERA_NAMES))

SYSROOT = $(shell $(CC) -print-sysroot)

# install our version of the system libraries in /opt/tophat/lib; this
# is necessary because:
# - we cannot link statically because we need NSS (name service
#   switch) modules
# - Kobo's stock glibc may be incompatible on some older firmware
#   versions
KOBO_SYS_LIB_NAMES = libc.so.6 libm.so.6 libpthread.so.0 librt.so.1 \
	libdl.so.2 \
	libresolv.so.2 libnss_dns.so.2 libnss_files.so.2 \
	ld-linux-armhf.so.3
KOBO_SYS_LIB_PATHS = $(addprefix $(SYSROOT)/lib/,$(KOBO_SYS_LIB_NAMES))

# Optionally, uImage files which supports USB HOST mode are created
# The installation is managed by the KoboMenu and rcS.tophat
UIMAGE_BASE_DIR=$(topdir)/kobo/uimage/hw/imx507/linux-2.6.35.3-USBHOST
UIMAGE_USB=$(UIMAGE_BASE_DIR)/arch/arm/boot/uImage
ifeq ($(KOBO_UIMAGE),y)
	UIMAGE_PREREQUISITES=UIMAGE FORCE_UIMAGE
	UIMAGE_CMD=$(Q)install -m 0644 $(UIMAGE_USB) $(@D)/KoboRoot/mnt/onboard/.kobo/uImage-USB-hot-plug
endif

$(UIMAGE_BASE_DIR)/COPYING:
	git submodule update --init kobo/uimage

UIMAGE: $(UIMAGE_BASE_DIR)/COPYING
	make -C $(UIMAGE_BASE_DIR) CROSS_COMPILE=arm-none-linux-gnueabi- ARCH=arm uImage

FORCE_UIMAGE:
	touch $(TARGET_OUTPUT_DIR)/force_uimage

# /mnt/onboard/.kobo/KoboRoot.tgz is a file that is picked up by
# /etc/init.d/rcS, extracted to / on each boot; we can use it to
# install XCSoar/TopHat
$(TARGET_OUTPUT_DIR)/KoboRoot.tgz: $(XCSOAR_BIN) \
	$(UIMAGE_PREREQUISITES) \
	$(KOBO_MENU_BIN) $(KOBO_POWER_OFF_BIN) \
	$(BITSTREAM_VERA_FILES) \
	$(topdir)/kobo/rcS.tophat \
	$(topdir)/kobo/50-tophat-usb.rules \
	$(topdir)/kobo/10-media-automount.rules \
	$(topdir)/kobo/inittab $(topdir)/kobo/inetd.conf
	@$(NQ)echo "  TAR     $@"
	$(Q)rm -rf $(@D)/KoboRoot
	$(Q)install -m 0755 -d $(@D)/KoboRoot/etc $(@D)/KoboRoot/opt/tophat/bin $(@D)/KoboRoot/opt/tophat/share/fonts $(@D)/KoboRoot/opt/tophat/lib
	$(Q)install -m 0755 -d $(@D)/KoboRoot/mnt/onboard/.kobo
	$(Q)install -m 0755 $(XCSOAR_BIN) $(KOBO_MENU_BIN) $(KOBO_POWER_OFF_BIN) $(@D)/KoboRoot/opt/tophat/bin
	$(Q)$(UIMAGE_CMD)
	$(Q)install -m 0644 $(TARGET_OUTPUT_DIR)/force_uimage $(@D)/KoboRoot/mnt/onboard/.kobo/force_uimage
	$(Q)install -m 0755 $(KOBO_SYS_LIB_PATHS) $(@D)/KoboRoot/opt/tophat/lib
	$(Q)install -m 0644 $(topdir)/kobo/inittab $(@D)/KoboRoot/etc
	$(Q)install -m 0644 $(topdir)/kobo/inetd.conf $(@D)/KoboRoot/etc
	$(Q)install -m 0755 -d $(@D)/KoboRoot/etc/init.d
	$(Q)install -m 0755 -d $(@D)/KoboRoot/etc/udev
	$(Q)install -m 0755 -d $(@D)/KoboRoot/etc/udev/rules.d
	$(Q)install -m 0755 $(topdir)/kobo/rcS.tophat $(@D)/KoboRoot/etc/init.d
	$(Q)install -m 0755 $(topdir)/kobo/50-tophat-usb.rules $(@D)/KoboRoot/etc/udev/rules.d
	$(Q)install -m 0755 -d $(@D)/KoboRoot/media
	$(Q)install -m 0755 $(topdir)/kobo/10-media-automount.rules $(@D)/KoboRoot/etc/udev/rules.d
	$(Q)install -m 0644 $(BITSTREAM_VERA_FILES) $(@D)/KoboRoot/opt/tophat/share/fonts
	$(Q)fakeroot tar czfC $@ $(@D)/KoboRoot .

endif
