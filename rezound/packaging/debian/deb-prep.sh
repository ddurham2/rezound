# $Id deb-prep.sh,v 1.1 2003/02/10 14:11:35 lergnom Exp $
# Anthony Ventimiglia
#
# This script is used to prepare the Rezound cvs repository for debianization.
# It should be run from the top level directory of the cleanly checked out 
# CVS repository.
#
# It's quick and dirty.
#
# For more info read packaging/debian/README.cvs

./bootstrap
./configure
make dist