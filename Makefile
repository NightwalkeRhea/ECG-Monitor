.PHONY: default venv install lint test clean help

default: install

venv:
	python -m venv .venv
	.\.venv\Scripts\activate && python -m pip install --upgrade pip

install: venv
	.\.venv\Scripts\activate && pip install -r requirements.txt

lint:
	.\.venv\Scripts\activate && flake8 src/fpga
	clang-format --dry-run --Werror src/mcu/*.c src/mcu/*.h

test:
	.\.venv\Scripts\activate && pytest

clean:
	rm -rf __pycache__ */__pycache__ .pytest_cache

help:
	@echo "Targets: default install venv lint test clean"
