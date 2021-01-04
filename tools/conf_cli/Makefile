SRCDIR = src
OBJDIR = build

GPP=g++
CFLAGS=

SRCS = main.cpp

DEPS = 
OBJS = $(patsubst %, $(OBJDIR)/%, $(patsubst %.cpp, %.o, $(SRCS)))

all: $(OBJDIR)/mfmconf

$(OBJDIR)/mfmconf: $(OBJS)
	$(GPP) -o $@ $^ $(CFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p `dirname $@`
	$(GPP) -c -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -rf $(OBJDIR)