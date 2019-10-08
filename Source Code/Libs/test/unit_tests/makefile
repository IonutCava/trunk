# compiler and flags.
CC=g++
CFLAGS=-Wall

# folders.
OBJDIR=obj
EXEDIR=bin

# target executable.
EXENAME=test_unit_tests_main

OBJECTS=$(OBJDIR)/test_unit_tests_main.o \
        $(OBJDIR)/TestClass.tests.o \
        $(OBJDIR)/TestCompare.tests.o \
        $(OBJDIR)/TestConverterUtility.tests.o \
        $(OBJDIR)/TestIntegerUtility.tests.o \
        $(OBJDIR)/TestMemoryUtility.tests.o \
        $(OBJDIR)/TestMinMax.tests.o \
        $(OBJDIR)/TestNumericUtility.tests.o \
        $(OBJDIR)/TestString.tests.o \
        $(OBJDIR)/TestStringUtility.tests.o \
        $(OBJDIR)/TestTime.tests.o \
        $(OBJDIR)/TestTypes.tests.o

# object files and executable targets.
all : $(OBJECTS) $(EXEDIR)/$(EXENAME)

# pre-requisite for targets.
$(OBJECTS) : | $(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR) $(EXEDIR)

# build target.
$(EXEDIR)/$(EXENAME) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $(EXEDIR)/$(EXENAME) $(OBJECTS)

# build objects.
$(OBJDIR)/$(EXENAME).o : $(EXENAME).cpp
	$(CC) $(CFLAGS) -c $(EXENAME).cpp -o $(OBJDIR)/$(EXENAME).o
$(OBJDIR)/TestClass.tests.o : TestClass.tests.cpp
	$(CC) $(CFLAGS) -c TestClass.tests.cpp -o $(OBJDIR)/TestClass.tests.o
$(OBJDIR)/TestCompare.tests.o : TestCompare.tests.cpp
	$(CC) $(CFLAGS) -c TestCompare.tests.cpp -o $(OBJDIR)/TestCompare.tests.o
$(OBJDIR)/TestConverterUtility.tests.o : TestConverterUtility.tests.cpp
	$(CC) $(CFLAGS) -c TestConverterUtility.tests.cpp -o $(OBJDIR)/TestConverterUtility.tests.o
$(OBJDIR)/TestIntegerUtility.tests.o : TestIntegerUtility.tests.cpp
	$(CC) $(CFLAGS) -c TestIntegerUtility.tests.cpp -o $(OBJDIR)/TestIntegerUtility.tests.o
$(OBJDIR)/TestMemoryUtility.tests.o : TestMemoryUtility.tests.cpp
	$(CC) $(CFLAGS) -c TestMemoryUtility.tests.cpp -o $(OBJDIR)/TestMemoryUtility.tests.o
$(OBJDIR)/TestMinMax.tests.o : TestMinMax.tests.cpp
	$(CC) $(CFLAGS) -c TestMinMax.tests.cpp -o $(OBJDIR)/TestMinMax.tests.o
$(OBJDIR)/TestNumericUtility.tests.o : TestNumericUtility.tests.cpp
	$(CC) $(CFLAGS) -c TestNumericUtility.tests.cpp -o $(OBJDIR)/TestNumericUtility.tests.o
$(OBJDIR)/TestString.tests.o : TestString.tests.cpp
	$(CC) $(CFLAGS) -c TestString.tests.cpp -o $(OBJDIR)/TestString.tests.o
$(OBJDIR)/TestStringUtility.tests.o : TestStringUtility.tests.cpp
	$(CC) $(CFLAGS) -c TestStringUtility.tests.cpp -o $(OBJDIR)/TestStringUtility.tests.o
$(OBJDIR)/TestTime.tests.o : TestTime.tests.cpp
	$(CC) $(CFLAGS) -c TestTime.tests.cpp -o $(OBJDIR)/TestTime.tests.o
$(OBJDIR)/TestTypes.tests.o : TestTypes.tests.cpp
	$(CC) $(CFLAGS) -c TestTypes.tests.cpp -o $(OBJDIR)/TestTypes.tests.o

# clean files. (delete object files and executable)
.PHONY : clean
clean :
	rm $(EXEDIR)/$(EXENAME) $(OBJECTS)
	rmdir bin
	rmdir obj
