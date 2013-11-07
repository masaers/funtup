SRCDIR=.
BUILDDIR=build
OBJDIR=$(BUILDDIR)/obj
BINDIR=$(BUILDDIR)/bin
DEPDIR=$(BUILDDIR)/deps

CXX=/usr/local/GNU/gcc-4.8.2/bin/g++

CXXFLAGS+=-Wall -pedantic -std=c++11 -g -O3
LDFLAGS=

BIN_NAMES=funtup_test
OBJECTS=$((filter-out $(BIN_NAMES:%=$(SRCDIR)/%.cpp),$(wildcard $(SRCDIR)/*.cpp)):%.cpp=%.o)


# Clear default suffix rules
.SUFFIXES:
# Keep dirstamps and dependencies between calls
.PRECIOUS: %/.dirstamp $(DEPDIR)/%.d $(OBJDIR)/%.o

all: $(BIN_NAMES:%=$(BINDIR)/%)

$(BINDIR)/%: $(OBJDIR)/%.o $(DEPDIR)/%.d $(OBJECTS) $(BINDIR)/.dirstamp makefile
	$(CXX) $(LDFLAGS) $< $(OBJECTS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(DEPDIR)/%.d $(OBJDIR)/.dirstamp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(BIN_NAMES:%=$(DEPDIR)/%.d)

$(DEPDIR)/%.d: $(SRCDIR)/%.cpp $(DEPDIR)/.dirstamp
	@$(CXX) $(CXXFLAGS) -MM -MT '$@' $< > $@

%/.dirstamp:
	@mkdir -p $(@D)
	@touch $@

clean:
	@rm -rf $(BINDIR) $(DEPDIR) $(OBJDIR)

