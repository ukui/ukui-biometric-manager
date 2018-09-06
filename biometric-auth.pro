TEMPLATE = subdirs

SUBDIRS += \
        bioAuthentication \
        pam-biometric \
        pam-biometric-gui \
        polkit-agent

CONFIG = release
CONFIG += ordered
