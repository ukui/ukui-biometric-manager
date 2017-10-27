XML="/usr/share/dbus-1/interfaces/cn.kylinos.Biometric.xml"
QMAKE_OPTIONS =
PAM_MAKE_OPTIONS =
QMAKE = qmake
QT_MAKEFILE = QtMakefile
INSTALL_DIR = /usr/local/BiometricManager


all: debug

debug: QMAKE_OPTIONS += CONFIG+=debug
debug: BiometricManager

release: QMAKE_OPTIONS +=
release: BiometricManager

BiometricManager:
	cp -f $(XML) ./
	lrelease i18n/zh_CN.ts -qm i18n/zh_CN.qm
	$(QMAKE) $(QMAKE_OPTIONS) -o $(QT_MAKEFILE)
	make -f $(QT_MAKEFILE)

install:
	install -D BiometricManager $(DESTDIR)$(INSTALL_DIR)/BiometricManager
	install -D i18n/zh_CN.qm $(DESTDIR)$(INSTALL_DIR)/i18n/zh_CN.qm

uninstall:
	rm -rf $(DESTDIR)$(INSTALL_DIR)

clean:
	rm -f *.o BiometricManager QtMakefile biometric_interface.cpp \
		biometric_interface.h moc_biometric_interface.cpp \
		moc_mainwindow.cpp ui_mainwindow.h \
		moc_promptdialog.cpp ui_promptdialog.h \
		qrc_assets.cpp cn.kylinos.Biometric.xml i18n/zh_CN.qm
