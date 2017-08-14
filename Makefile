XML="/usr/share/dbus-1/interfaces/cn.kylinos.Biometric.xml"
QMAKE = qmake5
QMAKE_DEBUG =
QT_MAKEFILE = QtMakefile
INSTALL_DIR = /usr/local/BiometricManager/

all: executable

debug: QMAKE_DEBUG += CONFIG+=debug
debug: executable

executable: BiometricManager

BiometricManager:
	cp -f $(XML) ./
	$(QMAKE) $(QMAKE_DEBUG) -o $(QT_MAKEFILE)
	make -f $(QT_MAKEFILE)

install:
	mkdir -p $(INSTALL_DIR)
	cp BiometricManager $(INSTALL_DIR)

uninstall:
	rm -rf $(INSTALL_DIR)

clean:
	rm -f *.o BiometricManager QtMakefile biometric_interface.cpp \
		biometric_interface.h moc_biometric_interface.cpp \
		moc_mainwindow.cpp ui_mainwindow.h \
		moc_promptdialog.cpp ui_promptdialog.h \
		qrc_assets.cpp cn.kylinos.Biometric.xml
