#! /bin/bash
aclocal && autoheader && automake --add-missing && autoreconf && ./configure
