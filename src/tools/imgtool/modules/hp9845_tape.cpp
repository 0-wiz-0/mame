// license:BSD-3-Clause
// copyright-holders:F. Ulivi
/*********************************************************************

    hp9845_tape.cpp

    HP-9845 tape format 

*********************************************************************/
#include <bitset>

#include "imgtool.h"
#include "formats/imageutl.h"

// Constants
#define SECTOR_LEN			256	// Bytes in a sector
#define WORDS_PER_SECTOR	(SECTOR_LEN / 2)	// 16-bit words in a sector payload
#define SECTORS_PER_TRACK	426	// Sectors in a track
#define TRACKS_NO			2	// Number of tracks
#define TOT_SECTORS			(SECTORS_PER_TRACK * TRACKS_NO)	// Total number of sectors
#define DIR_WORD_0			0x0500	// First word of directories
#define DIR_WORD_1			0xffff	// Second word of directories
#define DIR_LAST_WORD		0xffff	// Last word of directories
#define FIRST_DIR_SECTOR	1	// First directory sector
#define SECTORS_PER_DIR		2	// Sectors per copy of directory
#define MAX_DIR_ENTRIES		42	// And the answer is.... the maximum number of entries in the directory!
#define DIR_COPIES			2	// Count of directory copies
#define PAD_WORD			0xffff	// Word value for padding
#define FIRST_FILE_SECTOR	(FIRST_DIR_SECTOR + SECTORS_PER_DIR * DIR_COPIES)	// First file sector
#define MAGIC				0x5441434f	// Magic value at start of image file: "TACO"
#define ONE_INCH_POS		(968 * 1024)	// 1 inch of tape in tape_pos_t units
#define START_POS			((tape_pos_t)(72.25 * ONE_INCH_POS))	// Start position on each track
#define DZ_WORDS			350	// Words in deadzone
#define IRG_SIZE			ONE_INCH_POS	// Size of inter-record-gap: 1"
#define IFG_SIZE			((tape_pos_t)(2.5 * ONE_INCH_POS))	// Size of inter-file-gap: 2.5"
#define ZERO_BIT_LEN		619     // Length of "0" bits when encoded on tape
#define ONE_BIT_LEN			1083    // Length of "1" bits when encoded on tape
#define HDR_W0_ZERO_MASK	0x4000	// Mask of zero bits in word 0 of header
#define RES_FREE_FIELD		0x2000	// Mask of "reserved free field" bit
#define FILE_ID_BIT			0x8000	// Mask of "file identifier" bit
#define SECTOR_IN_USE		0x1800	// Mask of "empty record indicator" (== !sector in use indicator)
#define SIF_FILE_NO			1	// SIF file #
#define SIF_FILE_NO_MASK	0x07ff	// Mask of SIF file #
#define SIF_FREE_FIELD		0	// SIF free field
#define SIF_FREE_FIELD_MASK	0xf000	// Mask of SIF free field
#define BYTES_AVAILABLE		0xff00	// "bytes available" field = 256
#define BYTES_AVAILABLE_MASK	0xff00	// Mask of "bytes available" field
#define BYTES_USED			0x00ff	// "bytes used" field = 256
#define BYTES_USED_MASK		0x00ff	// Mask of "bytes used" field
#define FORMAT_SECT_SIZE	((tape_pos_t)(2.85 * ONE_INCH_POS))	// Size of sectors including padding: 2.85"
#define PAD_WORD_LENGTH		(17 * ONE_BIT_LEN)	// Size of PAD_WORD on tape
#define PREAMBLE_WORD		0	// Value of preamble word
#define WORDS_PER_SECTOR_W_MARGIN	256	// Maximum number of words in a sector with a lot of margin (there are actually never more than about 160 words)
#define MIN_IRG_SIZE		((tape_pos_t)(0.066 * ONE_INCH_POS))	// Minimum size of IRG gaps: 0.066"

// File types
#define BKUP_FILETYPE		0
#define BKUP_ATTR_STR		"BKUP"
#define DATA_FILETYPE		1
#define DATA_ATTR_STR		"DATA"
#define PROG_FILETYPE		2
#define PROG_ATTR_STR		"PROG"
#define KEYS_FILETYPE		3
#define KEYS_ATTR_STR		"KEYS"
#define BDAT_FILETYPE		4
#define BDAT_ATTR_STR		"BDAT"
#define ALL_FILETYPE		5
#define ALL_ATTR_STR		"ALL "
#define BPRG_FILETYPE		6
#define BPRG_ATTR_STR		"BPRG"
#define OPRM_FILETYPE		7
#define OPRM_ATTR_STR		"OPRM"

// Words stored on tape
typedef UINT16 tape_word_t;

// Tape position, 1 unit = 1 inch / (968 * 1024)
typedef INT32 tape_pos_t;

/********************************************************************************
 * Directory entries
 ********************************************************************************/
typedef struct {
	UINT8 filename[ 6 ];// Filename (left justified, 0 padded on the right)
	bool protection;	// File protection
	UINT8 filetype;		// File type (00-1f)
	UINT16 filepos;		// File position (# of 1st sector)
	UINT16 n_recs;		// Number of records
	UINT16 wpr;			// Word-per-record
	unsigned n_sects;	// Count of sectors
} dir_entry_t;
	
/********************************************************************************
 * Tape image
 ********************************************************************************/
class tape_image_t {
public:
	tape_image_t(void);

	bool is_dirty(void) const { return dirty; }
	
	void format_img(void);

	imgtoolerr_t load_from_file(imgtool_stream *stream);
	imgtoolerr_t save_to_file(imgtool_stream *stream);

	unsigned free_sectors(void) const;

	void set_sector(unsigned s_no , const tape_word_t *s_data);
	void unset_sector(unsigned s_no);

	bool get_dir_entry(unsigned idx , dir_entry_t& entry);

private:
	bool dirty;
	// Tape image
	tape_word_t img[ TOT_SECTORS ][ WORDS_PER_SECTOR ];
	// Map of sectors in use
	std::bitset<TOT_SECTORS> alloc_map;
	// Directory
	std::vector<dir_entry_t> dir;

	static void wipe_sector(tape_word_t *s);
	static void tape_word_to_bytes(tape_word_t w , UINT8& bh , UINT8& bl);
	static void bytes_to_tape_word(UINT8 bh , UINT8 bl , tape_word_t& w);
	void dump_dir_sect(const tape_word_t *dir_sect , unsigned dir_sect_idx);
	void fill_and_dump_dir_sect(tape_word_t *dir_sect , unsigned& idx , unsigned& dir_sect_idx , tape_word_t w)	;
	void encode_dir(void);
	bool read_sector_words(unsigned& sect_no , unsigned& sect_idx , size_t word_no , tape_word_t *out) const;
	static bool filename_char_check(UINT8 c);
	static bool filename_check(const UINT8 *filename);
	bool decode_dir(void);
	static tape_pos_t word_length(tape_word_t w);
	static tape_pos_t block_end_pos(tape_pos_t pos , const tape_word_t *block , unsigned block_len);
	static bool save_word(imgtool_stream *stream , tape_pos_t& pos , tape_word_t w);
	static bool save_words(imgtool_stream *stream , tape_pos_t& pos , const tape_word_t *block , unsigned block_len);
	static tape_word_t checksum(const tape_word_t *block , unsigned block_len);
};

/********************************************************************************
 * Image state
 ********************************************************************************/
typedef struct {
	imgtool_stream *stream;
	tape_image_t *img;
} tape_state_t;

/********************************************************************************
 * Directory enumeration
 ********************************************************************************/
typedef struct {
	unsigned dir_idx;
} dir_state_t;

/********************************************************************************
 * Internal functions
 ********************************************************************************/
tape_image_t::tape_image_t(void)
	: dirty(false)
{
}

void tape_image_t::format_img(void)
{
	// Deallocate all sectors
	alloc_map.reset();

	// Create an empty directory
	dir.clear();

	dirty = true;
}

imgtoolerr_t tape_image_t::load_from_file(imgtool_stream *stream)
{
	stream_seek(stream , 0 , SEEK_SET);

	UINT8 tmp[ 4 ];

	if (stream_read(stream , tmp , 4) != 4) {
		return IMGTOOLERR_READERROR;
	}

	if (pick_integer_be(tmp , 0 , 4) != MAGIC) {
		return IMGTOOLERR_CORRUPTIMAGE;
	}

	unsigned exp_sector = 0;
	// Loader state:
	// 0	Wait for DZ
	// 1	Wait for sector data
	// 2	Wait for gap
	unsigned state;
	tape_pos_t end_pos = 0;
	for (unsigned track = 0; track < TRACKS_NO; track++) {
		state = 0;

		while (1) {
			if (stream_read(stream , tmp , 4) != 4) {
				return IMGTOOLERR_READERROR;
			}
			UINT32 words_no = pick_integer_le(tmp , 0 , 4);
			if (words_no == (UINT32)-1) {
				// Track ended
				break;
			}
			if (stream_read(stream , tmp , 4) != 4) {
				return IMGTOOLERR_READERROR;
			}
			tape_pos_t pos = pick_integer_le(tmp , 0 , 4);
			tape_word_t buffer[ WORDS_PER_SECTOR_W_MARGIN ];
			for (unsigned i = 0; i < words_no; i++) {
				if (stream_read(stream , tmp , 2) != 2) {
					return IMGTOOLERR_READERROR;
				}
				if (i < WORDS_PER_SECTOR_W_MARGIN) {
					buffer[ i ] = pick_integer_le(tmp , 0 , 2);
				}
			}
			switch (state) {
			case 0:
				// Skip DZ
				state = 1;
				break;

			case 2:
				if ((pos - end_pos) < MIN_IRG_SIZE) {
					// Gap too short, discard data block
					break;
				}
				// Intentional fall-through
				
			case 1:
				if (words_no < 6 + WORDS_PER_SECTOR || buffer[ 0 ] != PREAMBLE_WORD ||
					buffer[ 4 ] != checksum(&buffer[ 1 ], 3) ||
					buffer[ 5 + WORDS_PER_SECTOR ] != checksum(&buffer[ 5 ], WORDS_PER_SECTOR)) {
					return IMGTOOLERR_CORRUPTIMAGE;
				}
				// Check sector content
				if (exp_sector != (buffer[ 2 ] & 0xfff)) {
					return IMGTOOLERR_CORRUPTIMAGE;
				}
				if (((buffer[ 1 ] & FILE_ID_BIT) != 0 && exp_sector != 0) ||
					((buffer[ 1 ] & FILE_ID_BIT) == 0 && exp_sector == 0)) {
					return IMGTOOLERR_CORRUPTIMAGE;
				}
				if ((buffer[ 1 ] & (HDR_W0_ZERO_MASK | RES_FREE_FIELD | SIF_FILE_NO_MASK)) != (RES_FREE_FIELD | SIF_FILE_NO)) {
					return IMGTOOLERR_CORRUPTIMAGE;
				}
				if ((buffer[ 2 ] & SIF_FREE_FIELD_MASK) != SIF_FREE_FIELD) {
					return IMGTOOLERR_CORRUPTIMAGE;
				}
				bool in_use = (buffer[ 1 ] & SECTOR_IN_USE) != 0;
				if ((buffer[ 3 ] & BYTES_AVAILABLE_MASK) != BYTES_AVAILABLE ||
					(in_use && (buffer[ 3 ] & BYTES_USED_MASK) != BYTES_USED) ||
					(!in_use && (buffer[ 3 ] & BYTES_USED_MASK) != 0)) {
					return IMGTOOLERR_CORRUPTIMAGE;
				}
				if (in_use) {
					set_sector(exp_sector, &buffer[ 5 ]);
				} else {
					unset_sector(exp_sector);
				}
				exp_sector++;
				state = 2;
				break;
			}
			end_pos = block_end_pos(pos, &buffer[ 0 ], words_no);
		}
	}

	if (exp_sector != TOT_SECTORS) {
		return IMGTOOLERR_CORRUPTIMAGE;
	}

	if (!decode_dir()) {
		return IMGTOOLERR_CORRUPTDIR;
	}
	
	dirty = false;
	
	return IMGTOOLERR_SUCCESS;
}

tape_pos_t tape_image_t::word_length(tape_word_t w)
{
		unsigned zeros , ones;

		// pop count of w
		ones = (w & 0x5555) + ((w >> 1) & 0x5555);
		ones = (ones & 0x3333) + ((ones >> 2) & 0x3333);
		ones = (ones & 0x0f0f) + ((ones >> 4) & 0x0f0f);
		ones = (ones & 0x00ff) + ((ones >> 8) & 0x00ff);

		zeros = 16 - ones;

		return zeros * ZERO_BIT_LEN + (ones + 1) * ONE_BIT_LEN;
}

tape_pos_t tape_image_t::block_end_pos(tape_pos_t pos , const tape_word_t *block , unsigned block_len)
{
	while (block_len--) {
		pos += word_length(*block++);
	}
	return pos;
}

bool tape_image_t::save_word(imgtool_stream *stream , tape_pos_t& pos , tape_word_t w)
{
	UINT8 tmp[ 2 ];

	place_integer_le(tmp , 0 , 2 , w);
	if (stream_write(stream , tmp , 2) != 2) {
		return false;
	}

	pos += word_length(w);
		
	return true;
}

bool tape_image_t::save_words(imgtool_stream *stream , tape_pos_t& pos , const tape_word_t *block , unsigned block_len)
{
	UINT8 tmp[ 4 ];

	// Number of words (including preamble)
	place_integer_le(tmp , 0 , 4 , block_len + 1);
	if (stream_write(stream , tmp , 4) != 4) {
		return false;
	}
	// Start position
	place_integer_le(tmp , 0 , 4 , pos);
	if (stream_write(stream , tmp , 4) != 4) {
		return false;
	}
	// Preamble
	if (!save_word(stream , pos , PREAMBLE_WORD)) {
		return false;
	}
	// Words
	for (unsigned i = 0; i < block_len; i++) {
		if (!save_word(stream, pos, block[ i ])) {
			return false;
		}
	}

	return true;
}

tape_word_t tape_image_t::checksum(const tape_word_t *block , unsigned block_len)
{
	tape_word_t csum = 0;
	for (unsigned i = 0; i < block_len; i++) {
		csum += *block++;
	}
	return csum & 0xffff;
}

imgtoolerr_t tape_image_t::save_to_file(imgtool_stream *stream)
{
	// Encode copies of directory into sectors
	encode_dir();
	
	// Store sectors
	stream_seek(stream , 0 , SEEK_SET);

	UINT8 tmp[ 4 ];

	place_integer_be(tmp , 0 , 4 , MAGIC);
	if (stream_write(stream , tmp , 4) != 4) {
		return IMGTOOLERR_WRITEERROR;
	}

	tape_pos_t pos = START_POS;
	
	for (unsigned i = 0; i < TOT_SECTORS; i++) {
		if (i == TOT_SECTORS / 2) {
			// Track 0 -> 1
			place_integer_le(tmp , 0 , 4 , (UINT32)-1);
			if (stream_write(stream , tmp , 4) != 4) {
				return IMGTOOLERR_WRITEERROR;
			}
		}
		if (i == 0 || i == TOT_SECTORS / 2) {
			// Start of either track
			pos = START_POS;
			// Deadzone + 1" of gap
			tape_word_t deadzone[ DZ_WORDS ];
			for (unsigned j = 0; j < DZ_WORDS; j++) {
				deadzone[ j ] = PAD_WORD;
			}
			if (!save_words(stream, pos, deadzone, DZ_WORDS)) {
				return IMGTOOLERR_WRITEERROR;
			}

			pos += IRG_SIZE;
		}
		bool in_use = alloc_map[ i ];
		// Sector header
		tape_word_t sector[ WORDS_PER_SECTOR_W_MARGIN ];

		// Header word 0: file identifier bit, reserved free-field bit, empty record indicator & file #
		sector[ 0 ] = RES_FREE_FIELD | SIF_FILE_NO;
		if (i == 0) {
			sector[ 0 ] |= FILE_ID_BIT;
		}
		if (in_use) {
			sector[ 0 ] |= SECTOR_IN_USE;
		}
		// Header word 1: free-field & sector #
		sector[ 1 ] = SIF_FREE_FIELD | i;
		// Header word 2: bytes available & bytes used
		sector[ 2 ] = BYTES_AVAILABLE;
		if (in_use) {
			sector[ 2 ] |= BYTES_USED;
		}
		// Checksum of header
		sector[ 3 ] = checksum(&sector[ 0 ] , 3);
		// Sector payload
		if (in_use) {
			memcpy(&sector[ 4 ] , &img[ i ][ 0 ] , SECTOR_LEN);
		} else {
			for (unsigned j = 4; j < (4 + WORDS_PER_SECTOR); j++) {
				sector[ j ] = PAD_WORD;
			}
		}
		// Checksum of payload
		sector[ 4 + WORDS_PER_SECTOR ] = checksum(&sector[ 4 ] , WORDS_PER_SECTOR);
		// Pad sector up to FORMAT_SECT_SIZE
		tape_pos_t sect_size = 0;
		for (unsigned j = 0; j < 5 + WORDS_PER_SECTOR; j++) {
			sect_size += word_length(sector[ j ]);
		}
		unsigned padding_words = (FORMAT_SECT_SIZE - sect_size) / PAD_WORD_LENGTH;
		for (unsigned j = 5 + WORDS_PER_SECTOR; j < 5 + WORDS_PER_SECTOR + padding_words; j++) {
			sector[ j ] = PAD_WORD;
		}
		if (!save_words(stream, pos, sector, 5 + WORDS_PER_SECTOR + padding_words)) {
			return IMGTOOLERR_WRITEERROR;
		}
		// Gap between sectors
		if (i == 0) {
			pos += IFG_SIZE;
		} else {
			pos += IRG_SIZE;
		}
	}
	
	place_integer_le(tmp , 0 , 4 , (UINT32)-1);
	if (stream_write(stream , tmp , 4) != 4) {
		return IMGTOOLERR_WRITEERROR;
	}

	return IMGTOOLERR_SUCCESS;
}

unsigned tape_image_t::free_sectors(void) const
{
	return TOT_SECTORS - alloc_map.count();
}

void tape_image_t::set_sector(unsigned s_no , const tape_word_t *s_data)
{
	if (s_no < TOT_SECTORS) {
		memcpy(&img[ s_no ][ 0 ] , s_data , SECTOR_LEN);
		alloc_map.set(s_no);
		dirty = true;
	}
}

void tape_image_t::unset_sector(unsigned s_no)
{
	if (s_no < TOT_SECTORS) {
		alloc_map.reset(s_no);
		dirty = true;
	}
}

bool tape_image_t::get_dir_entry(unsigned idx , dir_entry_t& entry)
{
	if (idx >= dir.size()) {
		return false;
	} else {
		entry = dir[ idx ];
		return true;
	}
}

void tape_image_t::wipe_sector(tape_word_t *s)
{
	for (unsigned i = 0; i < WORDS_PER_SECTOR; i++) {
		s[ i ] = PAD_WORD;
	}
}

void tape_image_t::tape_word_to_bytes(tape_word_t w , UINT8& bh , UINT8& bl)
{
	bh = (UINT8)(w >> 8);
	bl = (UINT8)(w & 0xff);
}

void tape_image_t::bytes_to_tape_word(UINT8 bh , UINT8 bl , tape_word_t& w)
{
	w = ((tape_word_t)bh << 8) | ((tape_word_t)bl);
}

void tape_image_t::dump_dir_sect(const tape_word_t *dir_sect , unsigned dir_sect_idx)
{
	for (unsigned i = 0; i < DIR_COPIES; i++) {
		set_sector(FIRST_DIR_SECTOR + i * SECTORS_PER_DIR + dir_sect_idx, dir_sect);
	}
}

void tape_image_t::fill_and_dump_dir_sect(tape_word_t *dir_sect , unsigned& idx , unsigned& dir_sect_idx , tape_word_t w)
{
	// Dump sector once it's full
	if (idx >= WORDS_PER_SECTOR) {
		dump_dir_sect(dir_sect, dir_sect_idx);
		wipe_sector(dir_sect);
		idx = 0;
		dir_sect_idx++;
	}
	dir_sect[ idx++ ] = w;
}

void tape_image_t::encode_dir(void)
{
	tape_word_t dir_sect[ WORDS_PER_SECTOR ];

	wipe_sector(dir_sect);

	unsigned idx = 0;
	unsigned dir_sect_idx = 0;

	fill_and_dump_dir_sect(dir_sect, idx, dir_sect_idx, DIR_WORD_0);
	fill_and_dump_dir_sect(dir_sect, idx, dir_sect_idx, DIR_WORD_1);

	for (const dir_entry_t& ent : dir) {
		tape_word_t tmp;

		// Filename
		bytes_to_tape_word(ent.filename[ 0 ], ent.filename[ 1 ], tmp);
		fill_and_dump_dir_sect(dir_sect, idx, dir_sect_idx, tmp);
		bytes_to_tape_word(ent.filename[ 2 ], ent.filename[ 3 ], tmp);
		fill_and_dump_dir_sect(dir_sect, idx, dir_sect_idx, tmp);
		bytes_to_tape_word(ent.filename[ 4 ], ent.filename[ 5 ], tmp);
		fill_and_dump_dir_sect(dir_sect, idx, dir_sect_idx, tmp);
		// Protection, file type & file position
		tmp = ((tape_word_t)ent.filetype << 10) | (tape_word_t)ent.filepos;
		if (ent.protection) {
			tmp |= 0x8000;
		}
		fill_and_dump_dir_sect(dir_sect, idx, dir_sect_idx, tmp);
		// File size (# of records)
		fill_and_dump_dir_sect(dir_sect, idx, dir_sect_idx, ent.n_recs);
		// Words per record
		fill_and_dump_dir_sect(dir_sect, idx, dir_sect_idx, ent.wpr);
	}

	// Terminator
	fill_and_dump_dir_sect(dir_sect, idx, dir_sect_idx, DIR_LAST_WORD);

	// Dump last partial sector
	dump_dir_sect(dir_sect, dir_sect_idx);

	// Unset unused sectors
	for (unsigned i = dir_sect_idx + 1; i < SECTORS_PER_DIR; i++) {
		for (unsigned j = 0; j < DIR_COPIES; j++) {
			unset_sector(FIRST_DIR_SECTOR + i + j * SECTORS_PER_DIR);
		}
	}
}

bool tape_image_t::read_sector_words(unsigned& sect_no , unsigned& sect_idx , size_t word_no , tape_word_t *out) const
{
	while (word_no > 0) {
		if (sect_idx >= WORDS_PER_SECTOR) {
			sect_idx = 0;
			sect_no++;
			if (sect_no >= TOT_SECTORS || !alloc_map[ sect_no ]) {
				return false;
			}
		}
		*out++ = img[ sect_no ][ sect_idx ];
		sect_idx++;
		word_no--;
	}

	return true;
}

bool tape_image_t::filename_char_check(UINT8 c)
{
	// Colons and quotation marks are forbidden in file names
	return 0x20 < c && c < 0x7f && c != ':' && c != '"';
}

bool tape_image_t::filename_check(const UINT8 *filename)
{
	bool ended = false;

	for (unsigned i = 0; i < 6; i++) {
		UINT8 c = *filename++;
		
		if (ended) {
			if (c != 0) {
				return false;
			}
		} else if (c == 0) {
			ended = true;
		} else if (!filename_char_check(c)) {
			return false;
		}
	}

	return true;
}

bool tape_image_t::decode_dir(void)
{
	unsigned sect_no = FIRST_DIR_SECTOR - 1;
	unsigned sect_idx = SECTOR_LEN;

	dir.clear();
	
	tape_word_t tmp;
	
	if (!read_sector_words(sect_no, sect_idx, 1, &tmp)) {
		return false;
	}
	if (tmp != DIR_WORD_0) {
		return false;
	}
	if (!read_sector_words(sect_no, sect_idx, 1, &tmp)) {
		return false;
	}
	if (tmp != DIR_WORD_1) {
		return false;
	}

	// This is to check for overlapping files
	std::bitset<TOT_SECTORS> sect_in_use;
	
	while (1) {
		if (!read_sector_words(sect_no, sect_idx, 1, &tmp)) {
			return false;
		}
		if (tmp == DIR_LAST_WORD) {
			// End of directory
			break;
		}

		if (dir.size() >= MAX_DIR_ENTRIES) {
			// Too many entries
			return false;
		}

		dir_entry_t new_entry;

		// Filename
		tape_word_to_bytes(tmp, new_entry.filename[ 0 ], new_entry.filename[ 1 ]);
		if (!read_sector_words(sect_no, sect_idx, 1, &tmp)) {
			return false;
		}
		tape_word_to_bytes(tmp, new_entry.filename[ 2 ], new_entry.filename[ 3 ]);
		if (!read_sector_words(sect_no, sect_idx, 1, &tmp)) {
			return false;
		}
		tape_word_to_bytes(tmp, new_entry.filename[ 4 ], new_entry.filename[ 5 ]);
		if (!filename_check(new_entry.filename)) {
			return false;
		}

		// Protection, file type & file position
		if (!read_sector_words(sect_no, sect_idx, 1, &tmp)) {
			return false;
		}
		new_entry.protection = (tmp & 0x8000) != 0;
		new_entry.filetype = ((tmp >> 10) & 0x1f);
		new_entry.filepos = tmp & 0x3ff;
		if (new_entry.filepos < FIRST_FILE_SECTOR || new_entry.filepos >= TOT_SECTORS) {
			return false;
		}
		
		// File size (# of records)
		if (!read_sector_words(sect_no, sect_idx, 1, &tmp)) {
			return false;
		}
		new_entry.n_recs = tmp;

		// Words per record
		if (!read_sector_words(sect_no, sect_idx, 1, &tmp)) {
			return false;
		}
		new_entry.wpr = tmp;
		if (new_entry.wpr < 1) {
			return false;
		}

		new_entry.n_sects = ((unsigned)new_entry.wpr * new_entry.n_recs * 2 + SECTOR_LEN - 1) / SECTOR_LEN;
		if (new_entry.n_sects < 1 || (new_entry.n_sects + new_entry.filepos) > TOT_SECTORS) {
			return false;
		}

		for (unsigned i = new_entry.filepos; i < new_entry.n_sects + new_entry.filepos; i++) {
			if (sect_in_use[ i ]) {
				return false;
			}
			sect_in_use[ i ] = true;
		}

		dir.push_back(new_entry);
	}

	// Check for inconsistency between alloc_map & sect_in_use
	for (unsigned i = 0; i < FIRST_FILE_SECTOR; i++) {
		sect_in_use[ i ] = alloc_map[ i ];
	}
	
	std::bitset<TOT_SECTORS> tmp_map(~alloc_map & sect_in_use);
	if (tmp_map.any()) {
		// There is at least 1 sector that is in use by a file but it's empty/unallocated
		return false;
	}

	alloc_map = sect_in_use;
	
	return true;
}

static tape_state_t& get_tape_state(imgtool_image *img)
{
	tape_state_t *ts = (tape_state_t*)imgtool_image_extra_bytes(img);

	return *ts;
}

static tape_image_t& get_tape_image(tape_state_t& ts)
{
	if (ts.img == nullptr) {
		ts.img = global_alloc(tape_image_t);
	}

	return *(ts.img);
}

/********************************************************************************
 * Imgtool functions
 ********************************************************************************/
static imgtoolerr_t hp9845_tape_open(imgtool_image *image, imgtool_stream *stream)
{
	tape_state_t& state = get_tape_state(image);

	state.stream = stream;
	
	tape_image_t& tape_image = get_tape_image(state);

	return tape_image.load_from_file(stream);
}

static imgtoolerr_t hp9845_tape_create(imgtool_image *image, imgtool_stream *stream, util::option_resolution *opts)
{
	tape_state_t& state = get_tape_state(image);

	state.stream = stream;
	
	tape_image_t& tape_image = get_tape_image(state);

	tape_image.format_img();

	return IMGTOOLERR_SUCCESS;
}

static void hp9845_tape_close(imgtool_image *image)
{
	tape_state_t& state = get_tape_state(image);
	tape_image_t& tape_image = get_tape_image(state);
	
	if (tape_image.is_dirty()) {
		(void)tape_image.save_to_file(state.stream);
	}

	stream_close(state.stream);
	
	// Free tape_image
	global_free(&tape_image);
}

static imgtoolerr_t hp9845_tape_begin_enum (imgtool_directory *enumeration, const char *path)
{
	dir_state_t *ds = (dir_state_t*)imgtool_directory_extrabytes(enumeration);

	ds->dir_idx = 0;

	return IMGTOOLERR_SUCCESS;
}

static const char *const filetype_attrs[] = {
	BKUP_ATTR_STR,	// 0
	DATA_ATTR_STR,	// 1
	PROG_ATTR_STR,	// 2
	KEYS_ATTR_STR,	// 3
	BDAT_ATTR_STR,	// 4
	ALL_ATTR_STR,	// 5
	BPRG_ATTR_STR,	// 6
	OPRM_ATTR_STR	// 7
};

static imgtoolerr_t hp9845_tape_next_enum (imgtool_directory *enumeration, imgtool_dirent *ent)
{
	tape_state_t& state = get_tape_state(imgtool_directory_image(enumeration));
	tape_image_t& tape_image = get_tape_image(state);
	dir_state_t *ds = (dir_state_t*)imgtool_directory_extrabytes(enumeration);

	dir_entry_t entry;

	if (!tape_image.get_dir_entry(ds->dir_idx, entry)) {
		ent->eof = 1;
	} else {
		ds->dir_idx++;

		UINT8 tmp_fn[ 7 ];

		memcpy(&tmp_fn[ 0 ] , &entry.filename[ 0 ] , 6);
		tmp_fn[ 6 ] = '\0';
		
		// Decode filetype
		UINT8 type_low = entry.filetype & 7;
		UINT8 type_hi = (entry.filetype >> 3) & 3;

		const char *filetype_str = filetype_attrs[ type_low ];

		// Same logic used by hp9845b to add a question mark next to filetype
		bool qmark = (type_low == DATA_FILETYPE && type_hi == 3) ||
			(type_low != DATA_FILETYPE && type_hi != 2);

		// "filename" and "attr" fields try to look like the output of the "CAT" command
		snprintf(ent->filename , sizeof(ent->filename) , "%-6s %c %s%c" , tmp_fn , entry.protection ? '*' : ' ' , filetype_str , qmark ? '?' : ' ');
		
		snprintf(ent->attr , sizeof(ent->attr) , "%4u %4u %3u" , entry.n_recs , entry.wpr * 2 , entry.filepos);
		
		ent->filesize = entry.n_sects * SECTOR_LEN;
	}
	return IMGTOOLERR_SUCCESS;
}

static imgtoolerr_t hp9845_tape_free_space(imgtool_partition *partition, UINT64 *size)
{
	tape_state_t& state = get_tape_state(imgtool_partition_image(partition));
	tape_image_t& tape_image = get_tape_image(state);

	*size = tape_image.free_sectors() * SECTOR_LEN;

	return IMGTOOLERR_SUCCESS;
}

static imgtoolerr_t hp9845_tape_read_file(imgtool_partition *partition, const char *filename, const char *fork, imgtool_stream *destf)
{
	tape_state_t& state = get_tape_state(imgtool_partition_image(partition));
	tape_image_t& tape_image = get_tape_image(state);

	// TODO: 
	return IMGTOOLERR_UNIMPLEMENTED;
}

void hp9845_tape_get_info(const imgtool_class *imgclass, UINT32 state, union imgtoolinfo *info)
{
	switch (state) {
	case IMGTOOLINFO_STR_NAME:
		strcpy(info->s = imgtool_temp_str(), "hp9845_tape");
		break;

	case IMGTOOLINFO_STR_DESCRIPTION:
		strcpy(info->s = imgtool_temp_str(), "HP9845 tape image");
		break;

	case IMGTOOLINFO_STR_FILE:
		strcpy(info->s = imgtool_temp_str(), __FILE__);
		break;
		
	case IMGTOOLINFO_STR_FILE_EXTENSIONS:
		strcpy(info->s = imgtool_temp_str(), "hti");
		break;

	case IMGTOOLINFO_INT_IMAGE_EXTRA_BYTES:
		info->i = sizeof(tape_state_t);
		break;

	case IMGTOOLINFO_INT_DIRECTORY_EXTRA_BYTES:
		info->i = sizeof(dir_state_t);
		break;

	case IMGTOOLINFO_PTR_OPEN:
		info->open = hp9845_tape_open;
		break;

	case IMGTOOLINFO_PTR_CREATE:
		info->create = hp9845_tape_create;
		break;

	case IMGTOOLINFO_PTR_CLOSE:
		info->close = hp9845_tape_close;
		break;

	case IMGTOOLINFO_PTR_BEGIN_ENUM:
		info->begin_enum = hp9845_tape_begin_enum;
		break;

	case IMGTOOLINFO_PTR_NEXT_ENUM:
		info->next_enum = hp9845_tape_next_enum;
		break;

	case IMGTOOLINFO_PTR_FREE_SPACE:
		info->free_space = hp9845_tape_free_space;
		break;

	case IMGTOOLINFO_PTR_READ_FILE:
		info->read_file = hp9845_tape_read_file;
		break;
	}
}

