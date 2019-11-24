#
# tools/xen_ressource_manager
#

XEN_ROOT = $(CURDIR)/../..
include $(XEN_ROOT)/tools/Rules.mk

CFLAGS += -Werror -Wno-format-zero-length -Wmissing-declarations -Wno-declaration-after-statement -Wformat-nonliteral
CFLAGS += -I. -fPIC

CFLAGS_RM += $(CFLAGS_libxenlight)
CFLAGS_RM += -Wshadow

RM_OBJS = xen_ressource_manager.o

$(RM_OBJS): CFLAGS += $(libxentoollog)
$(RM_OBJS): CFLAGS += $(CFLAGS_RM)

genpath-target = $(call buildmakevars2header,_paths.h)
$(eval $(genpath-target))

$(RM_OBJS): _paths.h

.PHONY: all
all: xen_ressource_manager

xen_ressource_manager: $(RM_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(RM_OBJS)

.PHONY: install
install: all 
	$(INSTALL_DIR) $(DESTDIR)$(sbindir)
	$(INSTALL_DIR) $(DESTDIR)$(BASH_COMPLETION_DIR)
	$(INSTALL_PROG) xen_ressource_manager $(DESTDIR)$(sbindir)
	$(INSTALL_DATA) bash-completion $(DESTDIR)$(BASH_COMPLETION_DIR)/xen_ressource_manager.sh

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(BASH_COMPLETION_DIR)/xen_ressource_manager.sh
	rm -f $(DESTDIR)$(sbindir)/xen_ressource_manager

.PHONY: clean
clean:
	$(RM) -f *.o xen_ressource_manager _paths.h $(DEPS_RM)

distclean: clean

-include $(DEPS_INCLUDE)

