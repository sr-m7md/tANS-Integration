CONTIKI_PROJECT = receiver
all: $(CONTIKI_PROJECT)

CONTIKI = /home/andezion/contiki-ng

PROJECT_SOURCEFILES += tans_embedded.c

PROJECT_CONF_PATH = ./project-conf.h

MODULES += os/net/app-layer/coap

TARGET ?= native

include $(CONTIKI)/Makefile.include
