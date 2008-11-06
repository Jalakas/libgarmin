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
#include <inttypes.h>
#include "libgarmin.h"
#include "libgarmin_priv.h"

/* http://ati.land.cz/gps/typdecomp/ */
struct hdr_typ_t
{
	struct hdr_subfile_part_t hsub;
	u_int32_t tre1_offset;
	u_int32_t tre1_size;
	u_int32_t tre2_offset;
	u_int32_t tre2_size;
	u_int32_t tre3_offset;
	u_int32_t tre3_size;
};

