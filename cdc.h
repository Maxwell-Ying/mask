#include <stdio.h>
#include <inttypes.h>


/* define chunk file header and block entry */  
typedef struct _chunk_file_header {  
        uint32_t block_sz;  
        uint32_t block_nr;  
} chunk_file_header;  
#define CHUNK_FILE_HEADER_SZ    (sizeof(chunk_file_header))  
typedef struct _chunk_block_entry {  
        uint64_t offset;  
        uint32_t len;  
        uint8_t  md5[16 + 1];  
        uint8_t  csum[10 + 1];  
} chunk_block_entry;  
#define CHUNK_BLOCK_ENTRY_SZ    (sizeof(chunk_block_entry))  

/* define delta file header and block entry */  
typedef struct _delta_file_header {  
        uint32_t block_nr;  
        uint32_t last_block_sz;  
        uint64_t last_block_offset;  /* offset in delta file */  
} delta_file_header;  
#define DELTA_FILE_HEADER_SZ    (sizeof(delta_file_header))  
typedef struct _delta_block_entry {  
        uint64_t offset;  
        uint32_t len;  
        uint8_t  embeded; /* 1, block in delta file; 0, block in source file. */  
} delta_block_entry;  
#define DELTA_BLOCK_ENTRY_SZ    (sizeof(delta_block_entry))  


static int file_chunk_cdc(int fd_src, int fd_chunk, chunk_file_header *chunk_file_hdr);