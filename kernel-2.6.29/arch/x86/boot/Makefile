#
# arch/x86/boot/Makefile
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1994 by Linus Torvalds
#

# ROOT_DEV specifies the default root-device when making the image.
# This can be either FLOPPY, CURRENT, /dev/xxxx or empty, in which case
# the default of FLOPPY is used by 'build'.

ROOT_DEV := CURRENT

# If you want to preset the SVGA mode, uncomment the next line and
# set SVGA_MODE to whatever number you want.
# Set it to -DSVGA_MODE=NORMAL_VGA if you just want the EGA/VGA mode.
# The number is the same as you would ordinarily press at bootup.

SVGA_MODE := -DSVGA_MODE=NORMAL_VGA

# If you want the RAM disk device, define this to be the size in blocks.

#RAMDISK := -DRAMDISK=512

targets		:= vmlinux.bin setup.bin setup.elf zImage bzImage
subdir-		:= compressed

setup-y		+= a20.o cmdline.o copy.o cpu.o cpucheck.o edd.o
setup-y		+= header.o main.o mca.o memory.o pm.o pmjump.o
setup-y		+= printf.o string.o tty.o video.o video-mode.o version.o
setup-$(CONFIG_X86_APM_BOOT) += apm.o
setup-$(CONFIG_X86_VOYAGER) += voyager.o

# The link order of the video-*.o modules can matter.  In particular,
# video-vga.o *must* be listed first, followed by video-vesa.o.
# Hardware-specific drivers should follow in the order they should be
# probed, and video-bios.o should typically be last.
setup-y		+= video-vga.o
setup-y		+= video-vesa.o
setup-y		+= video-bios.o

targets		+= $(setup-y)
hostprogs-y	:= mkcpustr tools/build

HOST_EXTRACFLAGS += $(LINUXINCLUDE)

$(obj)/cpu.o: $(obj)/cpustr.h

quiet_cmd_cpustr = CPUSTR  $@
      cmd_cpustr = $(obj)/mkcpustr > $@
targets		+= cpustr.h
$(obj)/cpustr.h: $(obj)/mkcpustr FORCE
	$(call if_changed,cpustr)

# ---------------------------------------------------------------------------

# How to compile the 16-bit code.  Note we always compile for -march=i386,
# that way we can complain to the user if the CPU is insufficient.
KBUILD_CFLAGS	:= $(LINUXINCLUDE) -g -Os -D_SETUP -D__KERNEL__ \
		   -Wall -Wstrict-prototypes \
		   -march=i386 -mregparm=3 \
		   -include $(srctree)/$(src)/code16gcc.h \
		   -fno-strict-aliasing -fomit-frame-pointer \
		   $(call cc-option, -ffreestanding) \
		   $(call cc-option, -fno-toplevel-reorder,\
			$(call cc-option, -fno-unit-at-a-time)) \
		   $(call cc-option, -fno-stack-protector) \
		   $(call cc-option, -mpreferred-stack-boundary=2)
KBUILD_CFLAGS +=   $(call cc-option,-m32)
KBUILD_AFLAGS	:= $(KBUILD_CFLAGS) -D__ASSEMBLY__

$(obj)/zImage:  asflags-y := $(SVGA_MODE) $(RAMDISK)
$(obj)/bzImage: ccflags-y := -D__BIG_KERNEL__
$(obj)/bzImage: asflags-y := $(SVGA_MODE) $(RAMDISK) -D__BIG_KERNEL__
$(obj)/bzImage: BUILDFLAGS   := -b

quiet_cmd_image = BUILD   $@
cmd_image = $(obj)/tools/build $(BUILDFLAGS) $(obj)/setup.bin \
	    $(obj)/vmlinux.bin $(ROOT_DEV) > $@

$(obj)/zImage $(obj)/bzImage: $(obj)/setup.bin \
			      $(obj)/vmlinux.bin $(obj)/tools/build FORCE
	$(call if_changed,image)
	@echo 'Kernel: $@ is ready' ' (#'`cat .version`')'

OBJCOPYFLAGS_vmlinux.bin := -O binary -R .note -R .comment -S
$(obj)/vmlinux.bin: $(obj)/compressed/vmlinux FORCE
	$(call if_changed,objcopy)

SETUP_OBJS = $(addprefix $(obj)/,$(setup-y))

sed-offsets := -e 's/^00*/0/' \
        -e 's/^\([0-9a-fA-F]*\) . \(input_data\|input_data_end\)$$/\#define \2 0x\1/p'

quiet_cmd_offsets = OFFSETS $@
      cmd_offsets = $(NM) $< | sed -n $(sed-offsets) > $@

$(obj)/offsets.h: $(obj)/compressed/vmlinux FORCE
	$(call if_changed,offsets)

targets += offsets.h

AFLAGS_header.o += -I$(obj)
$(obj)/header.o: $(obj)/offsets.h

LDFLAGS_setup.elf	:= -T
$(obj)/setup.elf: $(src)/setup.ld $(SETUP_OBJS) FORCE
	$(call if_changed,ld)

OBJCOPYFLAGS_setup.bin	:= -O binary
$(obj)/setup.bin: $(obj)/setup.elf FORCE
	$(call if_changed,objcopy)

$(obj)/compressed/vmlinux: FORCE
	$(Q)$(MAKE) $(build)=$(obj)/compressed $@

# Set this if you want to pass append arguments to the zdisk/fdimage/isoimage kernel
FDARGS =
# Set this if you want an initrd included with the zdisk/fdimage/isoimage kernel
FDINITRD =

image_cmdline = default linux $(FDARGS) $(if $(FDINITRD),initrd=initrd.img,)

$(obj)/mtools.conf: $(src)/mtools.conf.in
	sed -e 's|@OBJ@|$(obj)|g' < $< > $@

# This requires write access to /dev/fd0
zdisk: $(BOOTIMAGE) $(obj)/mtools.conf
	MTOOLSRC=$(obj)/mtools.conf mformat a:			; sync
	syslinux /dev/fd0					; sync
	echo '$(image_cmdline)' | \
		MTOOLSRC=$(src)/mtools.conf mcopy - a:syslinux.cfg
	if [ -f '$(FDINITRD)' ] ; then \
		MTOOLSRC=$(obj)/mtools.conf mcopy '$(FDINITRD)' a:initrd.img ; \
	fi
	MTOOLSRC=$(obj)/mtools.conf mcopy $(BOOTIMAGE) a:linux	; sync

# These require being root or having syslinux 2.02 or higher installed
fdimage fdimage144: $(BOOTIMAGE) $(obj)/mtools.conf
	dd if=/dev/zero of=$(obj)/fdimage bs=1024 count=1440
	MTOOLSRC=$(obj)/mtools.conf mformat v:			; sync
	syslinux $(obj)/fdimage					; sync
	echo '$(image_cmdline)' | \
		MTOOLSRC=$(obj)/mtools.conf mcopy - v:syslinux.cfg
	if [ -f '$(FDINITRD)' ] ; then \
		MTOOLSRC=$(obj)/mtools.conf mcopy '$(FDINITRD)' v:initrd.img ; \
	fi
	MTOOLSRC=$(obj)/mtools.conf mcopy $(BOOTIMAGE) v:linux	; sync

fdimage288: $(BOOTIMAGE) $(obj)/mtools.conf
	dd if=/dev/zero of=$(obj)/fdimage bs=1024 count=2880
	MTOOLSRC=$(obj)/mtools.conf mformat w:			; sync
	syslinux $(obj)/fdimage					; sync
	echo '$(image_cmdline)' | \
		MTOOLSRC=$(obj)/mtools.conf mcopy - w:syslinux.cfg
	if [ -f '$(FDINITRD)' ] ; then \
		MTOOLSRC=$(obj)/mtools.conf mcopy '$(FDINITRD)' w:initrd.img ; \
	fi
	MTOOLSRC=$(obj)/mtools.conf mcopy $(BOOTIMAGE) w:linux	; sync

isoimage: $(BOOTIMAGE)
	-rm -rf $(obj)/isoimage
	mkdir $(obj)/isoimage
	for i in lib lib64 share end ; do \
		if [ -f /usr/$$i/syslinux/isolinux.bin ] ; then \
			cp /usr/$$i/syslinux/isolinux.bin $(obj)/isoimage ; \
			break ; \
		fi ; \
		if [ $$i = end ] ; then exit 1 ; fi ; \
	done
	cp $(BOOTIMAGE) $(obj)/isoimage/linux
	echo '$(image_cmdline)' > $(obj)/isoimage/isolinux.cfg
	if [ -f '$(FDINITRD)' ] ; then \
		cp '$(FDINITRD)' $(obj)/isoimage/initrd.img ; \
	fi
	mkisofs -J -r -o $(obj)/image.iso -b isolinux.bin -c boot.cat \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		$(obj)/isoimage
	isohybrid $(obj)/image.iso 2>/dev/null || true
	rm -rf $(obj)/isoimage

zlilo: $(BOOTIMAGE)
	if [ -f $(INSTALL_PATH)/vmlinuz ]; then mv $(INSTALL_PATH)/vmlinuz $(INSTALL_PATH)/vmlinuz.old; fi
	if [ -f $(INSTALL_PATH)/System.map ]; then mv $(INSTALL_PATH)/System.map $(INSTALL_PATH)/System.old; fi
	cat $(BOOTIMAGE) > $(INSTALL_PATH)/vmlinuz
	cp System.map $(INSTALL_PATH)/
	if [ -x /sbin/lilo ]; then /sbin/lilo; else /etc/lilo/install; fi

install:
	sh $(srctree)/$(src)/install.sh $(KERNELRELEASE) $(BOOTIMAGE) System.map "$(INSTALL_PATH)"
