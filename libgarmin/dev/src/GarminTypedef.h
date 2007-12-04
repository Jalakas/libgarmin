/**********************************************************************************************
    Copyright (C) 2006, 2007 Oliver Eichler oliver.eichler@gmx.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA

  Garmin and MapSource are registered trademarks or trademarks of Garmin Ltd.
  or one of its subsidiaries.

**********************************************************************************************/
#ifndef GARMINTYPEDEF_H
#define GARMINTYPEDEF_H

struct FATblock_t
{
	u_int8_t  flag;		///< 0x00000000
	char    name[8];		///< 0x00000001 .. 0x00000008
	char    type[3];		///< 0x00000009 .. 0x0000000B
	u_int32_t size;		///< 0x0000000C .. 0x0000000F
	u_int16_t part;		///< 0x00000010 .. 0x00000011
	u_int8_t  byte0x00000012_0x0000001F[14];
	u_int16_t blocks[240];	///< 0x00000020 .. 0x000001FF
} __attribute((packed));

struct hdr_img_t
{
	u_int8_t  xorByte;            ///< 0x00000000
	u_int8_t  byte0x00000001_0x0000000F[15];
	char    signature[7];       ///< 0x00000010 .. 0x00000016
	u_int8_t  byte0x00000017_0x00000040[42];
	char    identifier[7];      ///< 0x00000041 .. 0x00000047
	u_int8_t  byte0x00000048;
	char    desc1[20];          ///< 0x00000049 .. 0x0000005C
	u_int8_t  byte0x0000005D_0x00000060[4];
	u_int8_t  e1;                 ///< 0x00000061
	u_int8_t  e2;                 ///< 0x00000062
	u_int8_t  byte0x00000063_0x00000064[2];
	char    desc2[31];          ///< 0x00000065 .. 0x00000083
	u_int8_t  byte0x00000084_0x0000040B[904];
	u_int32_t dataoffset;         ///< 0x0000040C .. 0x0000040F
	u_int8_t  byte0x00000410_0x0000041F[16];
	u_int16_t blocks[240];        ///< 0x00000420 .. 0x000005FF
} __attribute((packed));

static int inline get_blocksize(struct hdr_img_t *i) {
	return (1<<(i->e1+i->e2));
}

struct hdr_subfile_part_t
{
	u_int16_t length;             ///< 0x00000000 .. 0x00000001
	char    type[10];           ///< 0x00000002 .. 0x0000000B
	u_int8_t  byte0x0000000C;	// = 0x01
	u_int8_t  flag;               ///< 0x0000000D
	u_int16_t year;
	u_int8_t month;
	u_int8_t day;
	u_int8_t hour;
	u_int8_t min;
	u_int8_t sec;
} __attribute((packed));

struct hdr_rgn_t
{
	struct hdr_subfile_part_t hsub;
	u_int32_t offset;             ///< 0x00000015 .. 0x00000018
	u_int32_t length;             ///< 0x00000019 .. 0x0000000C
} __attribute((packed));

typedef u_int8_t u_int24_t[3];

struct hdr_tre_t
{
	struct hdr_subfile_part_t hsub;
	u_int8_t northbound[3];         ///< 0x00000015 .. 0x00000017
	u_int8_t eastbound[3];          ///< 0x00000018 .. 0x0000001A
	u_int8_t southbound[3];         ///< 0x0000001B .. 0x0000001D
	u_int8_t westbound[3];          ///< 0x0000001E .. 0x00000020
	u_int32_t tre1_offset;        ///< 0x00000021 .. 0x00000024
	u_int32_t tre1_size;          ///< 0x00000025 .. 0x00000028
	u_int32_t tre2_offset;        ///< 0x00000029 .. 0x0000002C
	u_int32_t tre2_size;          ///< 0x0000002D .. 0x00000030
	u_int32_t tre3_offset;        ///< 0x00000031 .. 0x00000034
	u_int32_t tre3_size;          ///< 0x00000035 .. 0x00000038
	u_int16_t tre3_rec_size;      ///< 0x00000039 .. 0x0000003A
	u_int8_t byte0x0000003B_0x0000003E[4];
	u_int8_t POI_flags;          ///< 0x0000003F
	u_int8_t drawprio;		// map draw priority
	u_int8_t  byte0x00000041_0x00000049[9];
	u_int32_t tre4_offset;        ///< 0x0000004A .. 0x0000004D
	u_int32_t tre4_size;          ///< 0x0000004E .. 0x00000051
	u_int16_t tre4_rec_size;      ///< 0x00000052 .. 0x00000053
	u_int8_t  byte0x00000054_0x00000057[4];
	u_int32_t tre5_offset;        ///< 0x00000058 .. 0x0000005B
	u_int32_t tre5_size;          ///< 0x0000005C .. 0x0000005F
	u_int16_t tre5_rec_size;      ///< 0x00000060 .. 0x00000061
	u_int8_t  byte0x00000062_0x00000065[4];
	u_int32_t tre6_offset;        ///< 0x00000066 .. 0x00000069
	u_int32_t tre6_size;          ///< 0x0000006A .. 0x0000006D
	u_int16_t tre6_rec_size;      ///< 0x0000006E .. 0x0000006F
	u_int8_t  byte0x00000070_0x00000073[4];
	/*-----------------------------------------------------*/
	u_int32_t mapID;
	u_int8_t  byte0x00000078_0x0000007B[4];
	u_int32_t tre7_offset;        ///< 0x0000007C .. 0x0000007F
	u_int32_t tre7_size;          ///< 0x00000080 .. 0x00000083
	u_int32_t tre7_rec_size;      ///< 0x00000084 .. 0x00000085
	u_int8_t  byte0x00000086_0x00000089[4];
	u_int32_t tre8_offset;        ///< 0x0000008A .. 0x0000008D
	u_int32_t tre8_size;          ///< 0x0000008E .. 0x00000091
	u_int8_t  byte0x00000092_0x00000099[8];
	/*-----------------------------------------------------*/
	u_int8_t  key[20];            ///< 0x0000009A .. 0x000000AD
	u_int32_t tre9_offset;        ///< 0x000000AE .. 0x000000B1
	u_int32_t tre9_size;          ///< 0x000000B2 .. 0x000000B5
	u_int32_t tre9_rec_size;      ///< 0x000000B6 .. 0x000000B7
} __attribute((packed));

struct tre_map_level_t
{
	u_int8_t  level       :4;
	u_int8_t  bit4        :1;
	u_int8_t  bit5        :1;
	u_int8_t  bit6        :1;
	u_int8_t  inherited   :1;
	u_int8_t  bits;
	u_int16_t nsubdiv;
} __attribute((packed));

struct tre_subdiv_t
{
	u_int24_t rgn_offset;
	u_int8_t  elements;
	u_int24_t center_lng;
	u_int24_t center_lat;
	u_int16_t width       :15;
	u_int16_t terminate   :1;
	u_int16_t height;
} __attribute((packed));

struct tre_subdiv_next_t
{
	struct tre_subdiv_t tresub;
	u_int16_t next;
} __attribute((packed));

struct hdr_lbl_t
{
	struct hdr_subfile_part_t hsub;

	u_int32_t lbl1_offset;        ///< 0x00000015 .. 0x00000018
	u_int32_t lbl1_length;        ///< 0x00000019 .. 0x0000001C
	u_int8_t  addr_shift;         ///< 0x0000001D
	u_int8_t  coding;             ///< 0x0000001E
	u_int32_t lbl2_offset;        ///< 0x0000001F .. 0x00000022
	u_int32_t lbl2_length;        ///< 0x00000023 .. 0x00000026
	u_int16_t lbl2_rec_size;      ///< 0x00000027 .. 0x00000028
	u_int8_t  byte0x00000029_0x0000002C[4];
	u_int32_t lbl3_offset;        ///< 0x0000002D .. 0x00000030
	u_int32_t lbl3_length;        ///< 0x00000031 .. 0x00000034
	u_int16_t lbl3_rec_size;      ///< 0x00000035 .. 0x00000036
	u_int8_t  byte0x00000037_0x0000003A[4];
	u_int32_t lbl4_offset;        ///< 0x0000003B .. 0x0000003E
	u_int32_t lbl4_length;        ///< 0x0000003F .. 0x00000042
	u_int16_t lbl4_rec_size;      ///< 0x00000043 .. 0x00000044
	u_int8_t  byte0x00000045_0x00000048[4];
	u_int32_t lbl5_offset;        ///< 0x00000049 .. 0x0000004C
	u_int32_t lbl5_length;        ///< 0x0000004D .. 0x00000050
	u_int16_t lbl5_rec_size;      ///< 0x00000051 .. 0x00000052
	u_int8_t  byte0x00000053_0x00000056[4];
	u_int32_t lbl6_offset;        ///< 0x00000057 .. 0x0000005A
	u_int32_t lbl6_length;        ///< 0x0000005B .. 0x0000005E
	u_int8_t  lbl6_addr_shift;    ///< 0x0000005F
	u_int8_t  lbl6_glob_mask;     ///< 0x00000060
	u_int8_t  byte0x00000061_0x00000063[3];
	u_int32_t lbl7_offset;        ///< 0x00000064 .. 0x00000067
	u_int32_t lbl7_length;        ///< 0x00000068 .. 0x0000006B
	u_int16_t lbl7_rec_size;      ///< 0x0000006C .. 0x0000006D
	u_int8_t  byte0x0000006E_0x00000071[4];
	u_int32_t lbl8_offset;        ///< 0x00000072 .. 0x00000075
	u_int32_t lbl8_length;        ///< 0x00000076 .. 0x00000079
	u_int16_t lbl8_rec_size;      ///< 0x0000007A .. 0x0000007B
	u_int8_t  byte0x0000007C_0x0000007F[4];
	u_int32_t lbl9_offset;        ///< 0x00000080 .. 0x00000083
	u_int32_t lbl9_length;        ///< 0x00000084 .. 0x00000087
	u_int16_t lbl9_rec_size;      ///< 0x00000088 .. 0x00000089
	u_int8_t  byte0x0000008A_0x0000008D[4];
	u_int32_t lbl10_offset;       ///< 0x0000008E .. 0x00000091
	u_int32_t lbl10_length;       ///< 0x00000092 .. 0x00000095
	u_int16_t lbl10_rec_size;     ///< 0x00000096 .. 0x00000097
	u_int8_t  byte0x00000098_0x0000009B[4];
	u_int32_t lbl11_offset;       ///< 0x0000009C .. 0x0000009F
	u_int32_t lbl11_length;       ///< 0x000000A0 .. 0x000000A3
	u_int16_t lbl11_rec_size;     ///< 0x000000A4 .. 0x000000A5
	u_int8_t  byte0x000000A6_0x000000AB[4];
	u_int16_t codepage;           ///< 0x000000AA .. 0x000000AB  optional check length

} __attribute((packed));

struct hdr_net_t
{
	struct hdr_subfile_part_t hsub;
	// NET1 Road definitions
	u_int32_t net1_offset;        ///< 0x00000015 .. 0x00000018
	u_int32_t net1_length;        ///< 0x00000019 .. 0x0000001C
	u_int8_t  net1_addr_shift;    ///< 0x0000001D
	// Segmented roads
	u_int32_t net2_offset;        ///< 0x0000001E .. 0x00000021
	u_int32_t net2_length;        ///< 0x00000022 .. 0x00000025
	u_int8_t net2_addr_shift;    ///< 0x00000026
	// Sorted Roads
	u_int32_t net3_offset;        ///< 0x00000027 .. 0x0000002A
	u_int32_t net3_length;        ///< 0x0000002B .. 0x0000002E
} __attribute((packed));



struct garmin_bmp_t{
	u_int16_t bfType;
	u_int32_t bfSize;
	u_int32_t bfReserved;
	u_int32_t bfOffBits;

	u_int32_t biSize;
	int32_t biWidth;
	int32_t biHeight;
	u_int16_t biPlanes;
	u_int16_t biBitCount;
	u_int32_t biCompression;
	u_int32_t biSizeImage;
	u_int32_t biXPelsPerMeter;
	u_int32_t biYPelsPerMeter;
	u_int32_t biClrUsed;
	u_int32_t biClrImportant;
	u_int32_t clrtbl[0x100];
	u_int8_t  data[];
} __attribute((packed));

struct hdr_nod_t
{
	struct hdr_subfile_part_t hsub;
	// Unknown
	u_int32_t	nod1offset;	// 0x15    Offset for section NOD1      4
	u_int32_t	nod1length;	// 0x19    Length of section NOD1       4
//	u_int32_t	nod1recsize;
	u_int8_t	b1;
	u_int8_t	b2;
	u_int8_t	b3;
	u_int8_t	b4;
	u_int8_t	blockalign;	// 0x24 (offset << blockalign + idx) >> blockalign
	u_int8_t	unknown3;	// 0x25 0x21    Unknown                      2
	u_int16_t	unknown4;	// 0x26 0x23    Unknown                      2
	// Road Data
	u_int32_t	nod2offset;	// 0x25    Road data offset, NOD2       4
	u_int32_t	nod2length;	// 0x29    Road data length             4
	u_int32_t	unknown5;	// addres shift? 0x2d    0x00000000 ???               4
	// Boundary Nodes
	u_int32_t	bondoffset;	//  0x31    Boundary nodes offset, NOD3  4
	u_int32_t	bondlength;	//  0x35    Boundary nodes length        4
	u_int8_t	bondrecsize;	//  0x39    Boundary nodes record length 1
	u_int16_t	zeroterm1;	// 0x21    Unknown                      2
	u_int16_t	zeroterm2;	// 0x23    Unknown                      2
	// if header len > 63
	u_int8_t	zero5;
	u_int16_t	bond2offset;
	u_int16_t	zero3;
	u_int16_t	bond2lenght;
	u_int16_t	zero4;
	u_int32_t	u1offset;
	u_int32_t	u1lenght;
	u_int32_t	u2offset;
	u_int32_t	u2lenght;
} __attribute((packed));

#define SPEEDCLASS(x)	((x>>1)&0x07)
#define ROADTYPE(x)	(((x)>>4)&0x07)
#define NODTYPE(x)	((x) & 1)
#define CHARINFO(x)	((x) & (1<<7))


struct nod_road_data {
	u_int8_t	flags;	// 0
	u_int24_t	nod1off;	// 1
	u_int16_t	nodes;		// if bit !NODTYPE(flags)
	u_int8_t	b3;		// 6
} __attribute((packed));

struct nod_bond {
	u_int24_t	east;	//  coord_east    3
	u_int24_t	north;	//    coord_north   3
	u_int24_t	offset;	// offset        3
};


#define TDB_HEADER	0x50
#define TDB_TRADEMARK	0x52
#define TDB_REGIONS	0x53
#define TDB_TAIL	0x54
#define TDB_COPYRIGHT	0x44
#define TDB_BASEMAP	0x42
#define TDB_DETAILMAP	0x4C

struct tdb_block {
	u_int8_t id;
	u_int16_t size;
} __attribute((packed));

#endif //GARMINTYPEDEF_H

