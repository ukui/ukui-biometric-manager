# The default options are compiling with debugging information
mode = debug

# QDBus XML file and the installation directory
XML = /usr/share/dbus-1/interfaces/cn.kylinos.Biometric.xml
INSTALL_DIR = /usr/local/biometric-manager

# Translation file directory
I18N_SRC = i18n_ts/

ifeq ($(mode), debug)
	QMAKE_OPTIONS = CONFIG+=debug
else
	QMAKE_OPTIONS =
endif


# Target
all: biometric-manager i18n

#
# Compilation
#

# Compile Biometric Manager
biometric-manager:
	cp -f $(XML) ./
	qmake $(QMAKE_OPTIONS) -o QtMakefile
	make -f QtMakefile

# Generate Qt translation file
i18n:
	$(MAKE) -C $(I18N_SRC)

#
# Installation
#

install: install-manager install-i18n

install-manager:
	# Install Biometric Manager
	install -D biometric-manager $(DESTDIR)$(INSTALL_DIR)/biometric-manager

install-i18n:
	# Install i18n
	$(MAKE) -C $(I18N_SRC) install

#
# Uninstallation
#

uninstall: uninstall-manager uninstall-i18n

uninstall-manager:
	# Uninstall Biometric Manager
	rm -rf $(DESTDIR)$(INSTALL_DIR)

uninstall-i18n:
	# Uninstall i18n
	$(MAKE) -C $(I18N_SRC) uninstall

#
# Clean intermediate file
#

clean:
	rm -f *.o biometric-manager QtMakefile biometric_interface.cpp \
		biometric_interface.h moc_biometric_interface.cpp \
		moc_mainwindow.cpp ui_mainwindow.h \
		moc_promptdialog.cpp ui_promptdialog.h \
		qrc_assets.cpp cn.kylinos.Biometric.xml
	# Clean i18n intermediate files
	$(MAKE) -C $(I18N_SRC) clean
