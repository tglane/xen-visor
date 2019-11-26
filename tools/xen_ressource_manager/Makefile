#
# tools/xen_ressource_manager
#

XEN_ROOT = $(CURDIR)/../..
include $(XEN_ROOT)/tools/Rules.mk

CFLAGS += -Werror
CFLAGS += $(CFLAGS_libxenstore)

LDLIBS += $(LDLIBS_libxenstore)

RM_OBJS = xen_ressource_manager.o

#genpath-target = $(call buildmakevars2header,_paths.h)
#$(eval $(genpath-target))

#$(RM_OBJS): _paths.h

.PHONY: all
all: xen_ressource_manager

xen_ressource_manager: $(RM_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(RM_OBJS) $(LDLIBS) $(APPEND_LDFLAGS)

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
	$(RM) -f *.o xen_ressource_manager $(DEPS_RM)
	#$(RM) -f *.o xen_ressource_manager _paths.h $(DEPS_RM)

.PHONY: distclean
distclean: clean

-include $(DEPS_INCLUDE)

