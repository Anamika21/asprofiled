/***************************************************************************
                          Token.cpp  -  A simple token class
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
#include "Token.h"

 Token::Token () {

   symbol = NULL;//L"EOF";
   image = NULL;//L"EOF";

   symbolIndex = 0;

   state = 0;
   kind = 0;
 }

 Token::~Token () {
   delete [] symbol;
   delete [] image;
 }

 Token *Token::newInstance() {
	Token *newToken = new Token ();

	if (symbol != NULL) {
		newToken->symbol = new wchar_t [wcslen(symbol)+1];
		wcscpy (newToken->symbol, symbol);
	}

	if (image != NULL) {
		newToken->image = new wchar_t [wcslen(image)+1];
		wcscpy (newToken->image, image);
	}

	newToken->col = col;
	newToken->kind = kind;
	newToken->line = line;
	newToken->state = state;
	newToken->symbolIndex = symbolIndex;

	return newToken;
 }

