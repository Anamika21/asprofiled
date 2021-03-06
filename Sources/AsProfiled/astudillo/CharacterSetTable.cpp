/***************************************************************************
                          CharacterSetTable.cpp  -  description
                             -------------------
    begin                : Sun Jun 2 2002
    copyright            : (C) 2002 by Manuel Astudillo
    email                : d00mas@efd.lth.se
 ***************************************************************************/

 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 ***************************************************************************/
#include "stdafx.h"
 #include "CharacterSetTable.h"


 CharacterSetTable::CharacterSetTable (integer nbrEntries) {
   this->nbrEntries = nbrEntries;
   characters = new wchar_t * [nbrEntries];
   for (int i=0; i < nbrEntries; i++) {
     characters[i] = NULL;
   }

 }

 CharacterSetTable::~CharacterSetTable () {
    for (int i = 0; i < nbrEntries; i++) {
        delete [] characters[i];
    }
	delete [] characters;
 }

