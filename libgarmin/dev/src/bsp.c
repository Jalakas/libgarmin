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

/*
 * Simple bitstream reading functions
 */

#include <unistd.h>
#include <sys/types.h>
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "bsp.h"

void bsp_init(struct bsp *bp, u_int8_t *data, u_int32_t len)
{
	bp->cbit = 0;
	bp->data = data;
	bp->ep = data+len;
	bp->cb = bp->data; 
}
// LSB to MSB
int bsp_get_bits(struct bsp *bp, int bits)
{
	u_int32_t ret = 0;
	int i;
	
	for (i=0; i < bits; i++) {
		if (bp->cbit == 8) {
			bp->cbit = 0;
			bp->cb++;
		}
		if (bp->cb >= bp->ep)
			return -1;
		ret |= (!!(*bp->cb & (1<<bp->cbit))) << i;
		bp->cbit ++;
	}
	return ret;
}

void bsp_fd_init(struct bspfd *bp, int fd)
{
	bp->cbit = 7;
	bp->fd = fd;
	bp->datalen = 0;
	bp->ep = bp->cb = bp->buf; 
}

static int bsp_fd_read(struct bspfd *bp)
{
	int rc;
	rc = read(bp->fd, bp->buf, sizeof(bp->buf));
	if (rc < 0)
		return -1;
//	hexdump(bp->buf, rc);
	bp->cb = bp->buf;
	bp->ep = bp->cb + rc;
	bp->datalen = rc;
	return rc;
}

// MSB to LSB
int bsp_fd_get_bits(struct bspfd *bp, int bits)
{
	u_int32_t ret = 0;
	int i;

	if (!bp->datalen) {
		if (bsp_fd_read(bp) < 0)
			return -1;
	}
	if (bits > 32) {
		log(1, "BSP: Error can not handle more than 32bits\n");
		return -1;
	}
	if (!bp->datalen)
		return -1;

	if (bp->cb == bp->ep)
		if (bsp_fd_read(bp) < 0)
			return -1;
	if (!bp->datalen)
		return -1;

	for (i=0; i < bits; i++) {
		if (bp->cbit < 0) {
			bp->cbit = 7;
			bp->cb++;
		}
		if (bp->cb == bp->ep) {
			if (bsp_fd_read(bp) < 0)
				return -1;
			if (!bp->datalen)
				return -1;
		}
		ret |= (!!(*bp->cb & (1<<bp->cbit))) << (bits - i - 1);
		bp->cbit --;
	}
	return ret;
}