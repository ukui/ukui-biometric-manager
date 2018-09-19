TEMPLATE = subdirs

SUBDIRS += \
        bioAuthentication \
#        pam-biometric \
        pam-biometric-dialog \
        polkit-agent

CONFIG += ordered debug
