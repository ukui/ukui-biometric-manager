INSTALL_DIR = /usr/local/biometric-manager

all: zh_CN.qm

zh_CN.qm:
	lrelease zh_CN.ts -qm zh_CN.qm

clean:
	rm -f zh_CN.qm

install:
	install -D zh_CN.qm $(DESTDIR)$(INSTALL_DIR)/i18n_qm/zh_CN.qm

uninstall:
	rm -f $(DESTDIR)$(INSTALL_DIR)/i18n_qm/zh_CN.qm
