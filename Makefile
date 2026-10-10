PIO_ENV ?= m5stack-sticks3

.PHONY: help build flash monitor test lint species clean

help:
	@echo "build    compile firmware for $(PIO_ENV)"
	@echo "flash    compile and upload over USB"
	@echo "monitor  serial log (battery mV + awake minutes every 10s)"
	@echo "test     species art checks"
	@echo "lint     ruff on tools/, then a build with warnings as errors"
	@echo "species  re-render docs/species/*.png"
	@echo "clean    remove build output"
	@echo "PIO_ENV=m5stickc-plus selects the older board"

build:
	pio run -e $(PIO_ENV)

flash:
	pio run -e $(PIO_ENV) -t upload

monitor:
	pio device monitor -e $(PIO_ENV)

test:
	uv run --no-project python -m unittest discover -s tools -v

lint:
	uvx ruff check tools
	PLATFORMIO_BUILD_SRC_FLAGS="-Wall -Wextra -Werror -Wno-error=unused-variable" pio run -e $(PIO_ENV)

species:
	uv run tools/render_species.py

clean:
	pio run -t clean
