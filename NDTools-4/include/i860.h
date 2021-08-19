/* i860.h -- Header file for the I860
   Copyright (C) 1989 Free Software Foundation, Inc.

This file is part of GAS, the GNU Assembler.

GAS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GAS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GAS; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */
#ifndef __I860_H__
#define __I860_H__
/*
 * Relocation types used in the I860 implementation.  Symbols in the 
 * data section are easy to relocate, being either a fixed value from
 * the .data origin or some relative value.  The instruction segment is
 * a little different.  The data can be wedged in to all sorts of odd 
 * locations, including multiple disjoint bit fields.  Bletch...
 *
 */

enum reloc_type
{
    RELOC_VANILLA,	/* Use the r_length and do byte, word, or longword fixup */

    RELOC_PAIR,     RELOC_HIGH,      RELOC_LOW0,     RELOC_LOW1,
    RELOC_LOW2,     RELOC_LOW3,      RELOC_LOW4,     RELOC_SPLIT0,
    RELOC_SPLIT1,   RELOC_SPLIT2,    RELOC_HIGHADJ,  RELOC_BRADDR,
    
    NO_RELOC
};

struct reloc_info_i860
{
    unsigned long int r_address;
/*
 * Using bit fields here is a bad idea because the order is not portable. :-(
 */
    unsigned int r_symbolnum    : 24;
    unsigned int r_pcrel    : 1;
    unsigned int r_length   : 2;
    unsigned int r_extern   : 1;
    enum reloc_type r_type  : 4;	/* Hints on what bits to use in relocation. */
};

#define relocation_info reloc_info_i860
#endif /* __I860_H__ */
