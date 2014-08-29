/*
 *   XFrisk - The classic board game for X
 *   Copyright (C) 1993-1999 Elan Feingold (elan@aetherworks.com)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *   $Id: snprintf.c,v 1.3 1999/11/13 21:58:32 morphy Exp $
 */

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

/*
 * This is a not very good, but hopefully adequate, substitute for
 * snprintf.
 */

int snprintf(char *buf, size_t len, const char *fmt, ...) {
  int xlen;
  va_list ap;
  va_start(ap, fmt);
  xlen = vsprintf(buf, fmt, ap);
  assert(xlen==strlen(buf));
  assert(xlen<len);
  return xlen;
}
