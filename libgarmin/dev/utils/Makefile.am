AM_CPPFLAGS=-I../src -Wall
bin_PROGRAMS = garxtract garxor
noinst_PROGRAMS = gartest bsptest
gartest_LDADD = ../src/libgarmin.a
garxtract_LDADD = ../src/libgarmin.a

clean-local:
	rm -rf *~ *.bak core
