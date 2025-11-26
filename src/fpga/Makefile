.PHONY: install run-pan coef lint format

install:
	python -m venv ../../.venv
	..\..\.\.venv\Scripts\activate && pip install --upgrade pip -r requirements.txt

run-pan:
	..\..\.\.venv\Scripts\activate && python pan_tompkins.py

coef:
	..\..\.\.venv\Scripts\activate && python fir_coef.py

lint:
	..\..\.\.venv\Scripts\activate && ruff check .

format:
	..\..\.\.venv\Scripts\activate && black .
