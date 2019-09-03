#include "cdc.h"

#define BUF_MAX_SZ 1000
#define BLOCK_MAX_SZ 1000
#define BLOCK_WIN_SZ 1000
#define BLOCK_MIN_SZ 100
#define CHAR_OFFSET 8


/*
 *   a simple 32 bit checksum that can be upadted from either end 
 *   (inspired by Mark Adler's Adler-32 checksum) 
 */
unsigned int adler32_checksum(char *buf, int len)  
{
    int i;
    unsigned int s1, s2;  
    s1 = s2 = 0;  
    for (i = 0; i < (len - 4); i += 4) {  
        s2 += 4 * (s1 + buf[i]) + 3 * buf[i+1] + 2 * buf[i+2] + buf[i+3] +  
          10 * CHAR_OFFSET; 
        s1 += (buf[i+0] + buf[i+1] + buf[i+2] + buf[i+3] + 4 * CHAR_OFFSET);  
    }  
    for (; i < len; i++) {  
        s1 += (buf[i]+CHAR_OFFSET);  
        s2 += s1;  
    }  
    return (s1 & 0xffff) + (s2 << 16);  
}  
/* 
 * adler32_checksum(X0, ..., Xn), X0, Xn+1 ----> adler32_checksum(X1, ..., Xn+1) 
 * where csum is adler32_checksum(X0, ..., Xn), c1 is X0, c2 is Xn+1 
 */  
unsigned int adler32_rolling_checksum(unsigned int csum, int len, char c1, char c2)  
{  
        unsigned int s1, s2, s11, s22;  
        s1 = csum & 0xffff;  
        s2 = csum >> 16;  
        s1 -= (c1 - c2);  
        s2 -= (len * c1 - s1);  
        return (s1 & 0xffff) + (s2 << 16);  
}  



/* content-defined chunking */
/*fd_src为分块的源文件*/
/*fd_chunk为分块后的文件*/
/*分块文件头chunk file header*/
static int file_chunk_cdc(int fd_src, int fd_chunk, chunk_file_header *chunk_file_hdr)
{
	char buf[BUF_MAX_SZ] = {0};  //缓冲区最大值
	char block_buf[BLOCK_MAX_SZ] = {0}; // 块的最大值
	char win_buf[BLOCK_WIN_SZ + 1] = {0}; //块的窗口大小
	unsigned char md5_checksum[16 + 1] = {0};
	unsigned char csum[10 + 1] = {0};
	unsigned int bpos = 0;
	unsigned int rwsize = 0;
	unsigned int exp_rwsize = BUF_MAX_SZ;
	unsigned int head, tail;
	unsigned int block_sz = 0, old_block_sz = 0;
	unsigned int hkey = 0;
	chunk_block_entry chunk_bentry; //分块实体
	uint64_t offset = 0;

	while(rwsize = read(fd_src, buf + bpos, exp_rwsize)) {
		/* last chunk */
		if ((rwsize + bpos + block_sz) < BLOCK_MIN_SZ)
			break;

		head = 0;
		tail = bpos + rwsize;
		/* avoid unnecessary computation and comparsion */
		if (block_sz < (BLOCK_MIN_SZ - BLOCK_WIN_SZ)) {
			old_block_sz = block_sz;
			block_sz = ((block_sz + tail - head) > (BLOCK_MIN_SZ - BLOCK_WIN_SZ)) ?
				BLOCK_MIN_SZ - BLOCK_WIN_SZ : block_sz + tail -head;
			memcpy(block_buf + old_block_sz, buf + head, block_sz - old_block_sz);
			head += (block_sz - old_block_sz);
		}

		while ((head + BLOCK_WIN_SZ) <= tail) {
			memcpy(win_buf, buf + head, BLOCK_WIN_SZ);
			hkey = (block_sz == (BLOCK_MIN_SZ - BLOCK_WIN_SZ)) ? adler32_checksum(win_buf, BLOCK_WIN_SZ) :
				adler32_rolling_checksum(hkey, BLOCK_WIN_SZ, buf[head-1], buf[head+BLOCK_WIN_SZ-1]);

			/* get a normal chunk, write block info to chunk file */
			if ((hkey % BLOCK_SZ) == CHUNK_CDC_R) {
				memcpy(block_buf + block_sz, buf + head, BLOCK_WIN_SZ);
				head += BLOCK_WIN_SZ;
				block_sz += BLOCK_WIN_SZ;
				if (block_sz >= BLOCK_MIN_SZ) {
					md5(block_buf, block_sz, md5_checksum);
					uint_2_str(adler32_checksum(block_buf, block_sz), csum);
					chunk_file_hdr->block_nr++;
					chunk_bentry.len = block_sz;
					chunk_bentry.offset = offset;
					memcpy(chunk_bentry.md5, md5_checksum, 16 + 1);
					memcpy(chunk_bentry.csum, csum, 10 + 1);
					rwsize = write(fd_chunk, &chunk_bentry, CHUNK_BLOCK_ENTRY_SZ);
					if (rwsize == -1 || rwsize != CHUNK_BLOCK_ENTRY_SZ)
						return -1;
					offset += block_sz;
					block_sz = 0;
				}
			} else {
				block_buf[block_sz++] = buf[head++];
				/* get an abnormal chunk, write block info to chunk file */
				if (block_sz >= BLOCK_MAX_SZ) {
					md5(block_buf, block_sz, md5_checksum);
					uint_2_str(adler32_checksum(block_buf, block_sz), csum);
					chunk_file_hdr->block_nr++;
					chunk_bentry.len = block_sz;
					chunk_bentry.offset = offset;
					memcpy(chunk_bentry.md5, md5_checksum, 16+1);
					memcpy(chunk_bentry.csum, csum, 10 + 1);
					rwsize = write(fd_chunk, &chunk_bentry, CHUNK_BLOCK_ENTRY_SZ);
					if (rwsize == -1 || rwsize != CHUNK_BLOCK_ENTRY_SZ)
						return -1;
					offset += block_sz;
					block_sz = 0;
				}
			}

			/* avoid unnecessary computation and comparsion */
			if (block_sz == 0) {
				block_sz = ((tail - head) > (BLOCK_MIN_SZ - BLOCK_WIN_SZ)) ?
					BLOCK_MIN_SZ - BLOCK_WIN_SZ : tail - head;
				memcpy(block_buf, buf + head, block_sz);
				head = ((tail - head) > (BLOCK_MIN_SZ - BLOCK_WIN_SZ)) ?
					head + (BLOCK_MIN_SZ - BLOCK_WIN_SZ) : tail;
			}
		}

		/* read expected data from file to full up buf */
		bpos = tail - head;
		exp_rwsize = BUF_MAX_SZ - bpos;
		memmove(buf, buf + head, bpos);
	}

	if (rwsize == -1)
		return -1;
	/* process last block */
	uint32_t last_block_sz = ((rwsize + bpos + block_sz) >= 0) ? rwsize + bpos + block_sz : 0;
	char last_block_buf[BLOCK_MAX_SZ] = {0};
	if (last_block_sz > 0) {
		memcpy(last_block_buf, block_buf, block_sz);
		memcpy(last_block_buf + block_sz, buf, rwsize + bpos);
		md5(last_block_buf, last_block_sz, md5_checksum);
		uint_2_str(adler32_checksum(last_block_buf, last_block_sz), csum);
		chunk_file_hdr->block_nr++;
		chunk_bentry.len = last_block_sz;
		chunk_bentry.offset = offset;
		memcpy(chunk_bentry.md5, md5_checksum, 16+1);
		memcpy(chunk_bentry.csum, csum, 10 + 1);
		rwsize = write(fd_chunk, &chunk_bentry, CHUNK_BLOCK_ENTRY_SZ);
		if (rwsize == -1 || rwsize != CHUNK_BLOCK_ENTRY_SZ)
			return -1;
	}

	return 0;
}