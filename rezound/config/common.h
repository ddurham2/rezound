/* 
 * 
 * Copyright (C) 2002 - Anthony Ventimiglia
 * 
 * This file is part of ReZound, an audio editing application.
 * 
 * ReZound is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * ReZound is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */
#ifndef COMMON_H
#define COMMON_H

/* common.h -- This file will deal with low-level portability problems. It
 * should be includede at the top of every package file. */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Redefine PACKAGE TO REZOUND_PACKAGE to clear up a conflict with
 * gnucc's config.h which has PACKAGE and VERSION defined. I really think
 * the Common C++ people are going to have to fix that, because it will cause
 * problems with and automake generated package. */
#ifdef PACKAGE
# define REZOUND_PACKAGE	PACKAGE
# undef PACKAGE
#else
# define REZOUND_PACKAGE "ReZound"	
/* Just in case it wasn't defined, the only time this should happen if for some reason someone would decide to compile without an automake built Makefile*/
#endif

/* do the same thing for VERSION */
#ifdef VERSION
# define REZOUND_VERSION 	VERSION
# undef VERSION
#else
# define REZOUND_VERSION	"vx.xxx" /* just in case it wasn't defined. */
#endif

/* I'm trying to move this here instead if as CXXFLAGS from am_include.mk (I think ccgnu wants this). For some reason It's not happy with this here, but until I figure it out it will remain as a CXXFLAG  */
/*
#define _GNU_SOURCE (1)
*/

#endif /* COMMON_H */
