PIO_ENV ?= m5stack-sticks3

.PHONY: help build flash flash-debug log log-clear monitor test test-art test-host test-host-update lint lint-tools lint-firmware lint-firmware-debug secrets-scan install-hooks species clean

help:
	@echo "build    compile firmware for $(PIO_ENV)"
	@echo "flash    compile and upload over USB"
	@echo "flash-debug  the same with the on-device log compiled in (src/petlog.h)"
	@echo "log      copy the on-device log to logs/ (debug build only)"
	@echo "log-clear  empty the on-device log"
	@echo "monitor  serial log (battery mV + awake minutes every 10s)"
	@echo "test     species art checks, then the firmware tests on this computer"
	@echo "test-host-update  rewrite the stored traces in test/golden/"
	@echo "lint     ruff on tools/, then a build with warnings as errors"
	@echo "secrets-scan   gitleaks over the git history"
	@echo "install-hooks  pre-commit hook (ruff + art checks + secret scan)"
	@echo "species  re-render docs/species/*.png"
	@echo "clean    remove build output"
	@echo "PIO_ENV=m5stickc-plus selects the older board"

build:
	pio run -e $(PIO_ENV)

flash:
	pio run -e $(PIO_ENV) -t upload

# The flag changes, so PlatformIO rebuilds src/ when switching between the two.
flash-debug:
	PLATFORMIO_BUILD_SRC_FLAGS="-DPET_LOG" pio run -e $(PIO_ENV) -t upload

log:
	uv run tools/pull_log.py

log-clear:
	uv run tools/pull_log.py --clear

monitor:
	pio device monitor -e $(PIO_ENV)

test: test-art test-host

test-art:
	uv run --no-project python -m unittest discover -s tools -v

# The firmware in src/ built for this computer against the fake hardware in
# test/fakes/ and driven by the tests in test/host/. HOST_TEST=text runs only
# the tests with text in their name; SIM_DUMP=file writes every drawing call.
HOST_CXX ?= c++
HOST_BIN = .pio/host/pet_tests
HOST_FLAGS = -std=gnu++17 -g -O1 -funsigned-char -ffp-contract=off \
	-fsanitize=address,undefined -fno-sanitize-recover=undefined \
	-DBOARD_STICKS3 -Itest/fakes -Itest/host

$(HOST_BIN): $(wildcard test/host/* test/fakes/* test/fakes/driver/* src/* src/buddies/*)
	@mkdir -p $(dir $@)
	$(HOST_CXX) $(HOST_FLAGS) test/host/*.cpp src/buddy.cpp src/buddies/*.cpp -o $@

test-host: $(HOST_BIN)
	$(HOST_BIN) $(HOST_TEST)

test-host-update: $(HOST_BIN)
	UPDATE_GOLDEN=1 $(HOST_BIN) $(HOST_TEST)

lint: lint-tools lint-firmware

lint-tools:
	uvx ruff check tools

LINT_FLAGS = -Wall -Wextra -Werror -Wno-error=unused-variable

lint-firmware:
	PLATFORMIO_BUILD_SRC_FLAGS="$(LINT_FLAGS)" pio run -e $(PIO_ENV)

# The debug build (src/petlog.h) is not what `make lint` or the tests compile,
# so it gets a build of its own to keep it from rotting.
lint-firmware-debug:
	PLATFORMIO_BUILD_SRC_FLAGS="$(LINT_FLAGS) -DPET_LOG" pio run -e $(PIO_ENV)

secrets-scan:
	gitleaks detect --config .gitleaks.toml --redact --no-banner -v

install-hooks:
	git config core.hooksPath .githooks

species:
	uv run tools/render_species.py

clean:
	pio run -t clean
