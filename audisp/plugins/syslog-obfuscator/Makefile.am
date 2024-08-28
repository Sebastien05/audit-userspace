# Makefile.am --
#
# Authors:
#   Sebastien LEFEVRE
# 

CONFIG_CLEAN_FILES = *.loT *.rej *.orig
EXTRA_DIST = syslog-obfuscator.conf $(man_MANS)
AM_CPPFLAGS = -I${top_srcdir} -I${top_srcdir}/lib -I${top_srcdir}/common -I${top_srcdir}/auparse
prog_confdir = $(sysconfdir)/audit
plugin_confdir=$(prog_confdir)/plugins.d
plugin_conf = syslog-obfuscator.conf
sbin_PROGRAMS = audisp-syslog-obfuscator

audisp_syslog_obfuscator_DEPENDENCIES = ${top_builddir}/common/libaucommon.la
audisp_syslog_obfuscator_SOURCES = audisp-syslog-obfuscator.c obfuscator_config.c
audisp_syslog_obfuscator_CFLAGS = -fPIE -DPIE -g3 -D_GNU_SOURCE -Wundef ${WFLAGS}
audisp_syslog_obfuscator_LDFLAGS = -pie -Wl,-z,relro -Wl,-z,now
audisp_syslog_obfuscator_LDADD = $(CAPNG_LDADD) ${top_builddir}/common/libaucommon.la ${top_builddir}/auparse/libauparse.la

install-data-hook:
	mkdir -p -m 0750 ${DESTDIR}${plugin_confdir}
	$(INSTALL_DATA) -D -m 640 ${srcdir}/$(plugin_conf) ${DESTDIR}${plugin_confdir}

uninstall-hook:
	rm ${DESTDIR}${plugin_confdir}/$(plugin_conf)

