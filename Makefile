XML="/usr/share/dbus-1/interfaces/cn.kylinos.Biometric.xml"
QMAKE_OPTIONS =
PAM_MAKE_OPTIONS =
QMAKE = qmake5
QT_MAKEFILE = QtMakefile
INSTALL_DIR = /usr/local/BiometricManager


all: debug

debug: QMAKE_OPTIONS += CONFIG+=debug
debug: BiometricManager

release: QMAKE_OPTIONS +=
release: BiometricManager

BiometricManager:
	cp -f $(XML) ./
	$(QMAKE) $(QMAKE_OPTIONS) -o $(QT_MAKEFILE)
	make -f $(QT_MAKEFILE)

install:
	install -D BiometricManager $(DESTDIR)$(INSTALL_DIR)/BiometricManager

uninstall:
	rm -rf $(DESTDIR)$(INSTALL_DIR)

clean:
	rm -f *.o BiometricManager QtMakefile biometric_interface.cpp \
		biometric_interface.h moc_biometric_interface.cpp \
		moc_mainwindow.cpp ui_mainwindow.h \
		moc_promptdialog.cpp ui_promptdialog.h \
		qrc_assets.cpp cn.kylinos.Biometric.xml
