CC = g++
CFLAGS   = -Wall -std=c++11 -I ./include

SRCDIR   = src
HEADDIR  = include
OBJDIR   = build
BINDIR   = bin
# where the executable will be stored
APP := $(BINDIR)/main

# list of all source files
SOURCES  := $(wildcard $(SRCDIR)/*.cpp)
# list of all header files
INCLUDES := $(wildcard $(HEADDIR)/*.h)
# from the list of all source files, create a list of all object files
OBJECTS  := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

print:
	@echo $(SOURCES)
	@echo $(INCLUDES)
	@echo $(OBJECTS)
	
	
# all: clean $(APP)
all: $(APP)

# Link: Generate executable file from object file
$(APP): $(OBJECTS)
	$(CC) -o $@ $^
	
	
# Compile: Generate object files from source files
# $@ := {EXECUTABLE}
# $< := THE first file
# $^ all the dependency
# C++ Sources
$(OBJDIR)/%.o : $(SRCDIR)/%.cpp 
	$(CC) $(CFLAGS) $^ -c -o $@
	

#Delete dependence files, object files, and the EXECUTABLE file
clean:
	@echo CLEANING UP
# check if the directories exist; if so, delete them
	@if exist "$(OBJDIR)" rmdir /s /q $(OBJDIR)
	@if exist "$(BINDIR)" rmdir /s /q $(BINDIR)