#!/bin/sh
#
# $Id$
# Anthony Ventimiglia
#
# This script is used to prepare the Rezound cvs repository for debianization.
# It should be run from the top level directory of the cleanly checked out 
# CVS repository.
#
# For more info read packaging/debian/README.cvs

./bootstrap
./configure