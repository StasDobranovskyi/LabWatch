CC ?= cc
PLATFORM := $(shell uname -s)-$(shell uname -m)
BUILD ?= build/$(PLATFORM)
CFLAGS ?= -std=c11 -O0 -g -Wall -Wextra -Werror
CPPFLAGS += -I../firmware/Devices -I../firmware/App -I../firmware/Platform -Ifakes
APP = ../firmware/App/instrument.c ../firmware/App/journal.c ../firmware/Platform/perf.c ../firmware/Platform/clock16.c ../firmware/Platform/rx_rearm.c
DEVICE = ../firmware/Devices/mpu6050.c
RUNTIME = ../firmware/Platform/runtime.c
SUITES = test_mpu test_calibration test_events test_acceptance test_runtime test_logfmt test_clock test_storage test_rx_rearm
HEADERS = $(wildcard ../firmware/App/*.h ../firmware/Devices/*.h ../firmware/Platform/*.h fakes/*.h fixtures/*.h support.h flash_fake.h)

.PHONY: test clean sanitize
test: $(SUITES:%=$(BUILD)/%)
	@set -e; for suite in $(SUITES); do ./$(BUILD)/$$suite; done

$(BUILD):
	mkdir -p "$@"
$(BUILD)/test_storage: test_storage.c ../firmware/Platform/storage.c $(HEADERS) flash_fakes/main.h | $(BUILD)
	$(CC) -Iflash_fakes $(CPPFLAGS) $(CFLAGS) -DLW_STORAGE_HOST_TEST $(filter %.c,$^) -o "$@"
$(BUILD)/test_logfmt: test_logfmt.c ../firmware/Platform/logfmt.c $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) -o "$@"
$(BUILD)/test_mpu: test_mpu.c $(DEVICE) $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) -lm -o "$@"
$(BUILD)/test_runtime: test_runtime.c $(APP) $(DEVICE) $(RUNTIME) $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRUNTIME_TEST_PRINTF $(filter %.c,$^) -lm -o "$@"
$(BUILD)/%: %.c $(APP) $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) -lm -o "$@"
sanitize:
	$(MAKE) test BUILD=build/$(PLATFORM)/sanitize CFLAGS='-std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer'
clean:
	rm -rf build
