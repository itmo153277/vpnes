#!/bin/sh

LC_ALL=C TZ=UTC0 diff -Naurx .svn svncommit ../vpnes-0.4 > ./vpnes-0.4.patch
