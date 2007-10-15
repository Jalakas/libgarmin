/*
    Copyright (C) 2007  Alexander Atanasov	<aatanasov@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
    
    
*/
#include <sys/types.h>

/* http://ati.land.cz/gps/typdecomp/ */
struct hdr_typ_t {
	u_int16_t	x1;	// 5b 00
	u_int8_t	id[10];
	u_int16_t	x2;	// 10 00
	u_int16_t	year;
	u_int8_t	month;	// 0-11
	u_int8_t	day;	// 1-31
	u_int8_t	hour;	// 0-24
	u_int8_t	minute;	// 1-60
	u_int16_t	x3;	// e4 04
	u_int16_t	fid;	// offset 0x2f
	u_int16_t	product_id;
};

