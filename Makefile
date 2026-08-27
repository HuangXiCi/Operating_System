# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = source
BUILDDIR      = build

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

copy:
ifeq ($(OS),Windows_NT)
	@powershell -Command "Remove-Item ./docs/* -Recurse -Force -ErrorAction SilentlyContinue"
	@powershell -Command "Copy-Item -Recurse -Path "./build/html/*" -Destination "./docs/""
	@powershell -Command "New-Item -Path "./docs/.nojekyll" -ItemType File"
else
    @rm -rf docs/*
	@cp -r build/html/* docs/
	@touch docs/.nojekyll
endif
