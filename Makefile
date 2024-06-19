.PHONY: docs

docs:
	@docker run --rm -v $(CURDIR):/project -w /project/src ghcr.io/doxygen/doxygen:latest /project/src/P22296-10-SW.doxyfile

