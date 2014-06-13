
CXXFLAGS+=-Wall -pedantic -std=c++11 -g -O3
LDFLAGS=

BIN_NAMES=funtup_test
OBJECTS=$((filter-out $(BIN_NAMES:%=%.cpp),$(wildcard *.cpp)):%.cpp=%.o)


# Clear default suffix rules
.SUFFIXES :
# Keep STAMPs and dependencies between calls
.PRECIOUS : %/.STAMP build/dep/%.d build/obj/%.o

all : binaries

binaries : $(BIN_NAMES:%=build/bin/%)

build/bin/% : build/obj/%.o build/dep/%.d $(OBJECTS) build/bin/.STAMP makefile
	$(CXX) $(LDFLAGS) $< $(OBJECTS) -o $@

build/obj/%.o : %.cpp build/dep/%.d build/obj/.STAMP
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(BIN_NAMES:%=build/dep/%.d)

build/dep/%.d : %.cpp build/dep/.STAMP
	@$(CXX) $(CXXFLAGS) -MM -MT '$@' $< > $@

%/.STAMP :
	@mkdir -p $(@D)
	@touch $@

clean :
	@rm -rf build/bin build/dep build/obj

