TARGET:= checkStackUsage.py
PYTHON:=python3
VENV:=.venv

all:	.setup .analysed test ## Check everything

.analysed:	${TARGET}
	$(MAKE) flake8
	$(MAKE) pylint
	$(MAKE) mypy
	@touch $@

flake8: dev-install ## PEP8 compliance checks with Flake8
	@echo "============================================"
	@echo " Running flake8..."
	@echo "============================================"
	${VENV}/bin/flake8 ${TARGET}

pylint: dev-install ## Static analysis with Pylint
	@echo "============================================"
	@echo " Running pylint..."
	@echo "============================================"
	${VENV}/bin/pylint --disable=I --rcfile=pylint.cfg ${TARGET}

mypy: dev-install ## Type checking with mypy
	@echo "============================================"
	@echo " Running mypy..."
	@echo "============================================"
	${VENV}/bin/mypy --ignore-missing-imports ${TARGET}

dev-install:	.setup | prereq

prereq:
	@${PYTHON} -c 'import sys; sys.exit(1 if (sys.version_info.major<3 or sys.version_info.minor<5) else 0)' || { \
	    echo "=============================================" ; \
	    echo "[x] You need at least Python 3.5 to run this." ; \
	    echo "=============================================" ; \
	    exit 1 ; \
	}

.setup:	requirements.txt
	@if [ ! -d ${VENV} ] ; then                            \
	    echo "[-] Installing VirtualEnv environment..." ;  \
	    ${PYTHON} -m venv ${VENV} || exit 1 ;              \
	fi
	echo "[-] Installing packages inside environment..." ; \
	. ${VENV}/bin/activate || exit 1 ;                     \
	${PYTHON} -m pip install -r requirements.txt || exit 1
	touch $@

test: ## Test proper functioning
	$(MAKE) -C tests/

clean: ## Cleanup artifacts
	rm -rf .cache/ .mypy_cache/ .analysed .setup __pycache__ \
	       tests/__pycache__ .pytest_cache/ .processed .coverage
	$(MAKE) -C tests/ clean

help: ## Display this help section
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z0-9_-]+:.*?## / {printf "\033[36m%-18s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)
.DEFAULT_GOAL := help

define newline # a literal \n


endef

# Makefile debugging trick:
# call print-VARIABLE to see the runtime value of any variable
# (hardened a bit against some special characters appearing in the output)
print-%:
	@echo '$*=$(subst ','\'',$(subst $(newline),\n,$($*)))'
.PHONY:	flake8 pylint mypy clean dev-install prereq
