CONFIGFILE_PROPER = config.mk
include $(CONFIGFILE_PROPER)

CC   = $(CC_PREFIX)gcc -std=c99
GCOV = gcov

CFLAGS  = -g -O0 -pedantic -fprofile-arcs -ftest-coverage
LDFLAGS = -lgcov -fprofile-arcs

coverage: check
	$(GCOV) -pr $(SRC) 2>&1
