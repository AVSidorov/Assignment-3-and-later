#include <stdio.h>
#include <stdlib.h>
#include "aesd-circular-buffer.h"

int main(int argc, char *argv[]){

	buffer_t *buf=malloc(sizeof(buffer_t));
	aesd_circular_buffer_init(buf);

	const entry_t entry_set[6] = {{"a", 1}, {"bc", 2}, {"def", 3}, {"Sid ",4}, {"is here!", 8}, {"",0}};

	uint8_t i,j;

	printf("Test buffer\n");

	int write_count;
	if (argc<2)
		write_count = 1;
	else
		write_count = atoi(argv[1]);

	int entry_count;
	if (argc<3)
		entry_count = 3;
	else
		entry_count = atoi(argv[2]) % 7;



	for (i=0; i < write_count; i++)
		for (j=0; j < entry_count; j++ )
			aesd_circular_buffer_add_entry(buf,&entry_set[j]);


	entry_t *entry;

	printf("Print full buffer\n");
	AESD_CIRCULAR_BUFFER_FOREACH(entry,buf,i){
		if (buf->in_offs == i)
			printf("\033[31m %s\033[0m|", entry->buffptr);
		else if (buf->out_offs == i)
				printf("\033[32m %s\033[0m|", entry->buffptr);
			else
				printf("%s|", entry->buffptr);

	}
	printf("\n");

	printf("Test Macro GET_ENTRY\n");
	for (uint8_t cur=0; cur < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED * 2; cur++){
		entry = AESD_CIRCULAR_BUFFER_GET_ENTRY(buf,cur);
		if(entry) printf("%s|",entry->buffptr);
	}
	printf("\n");

	printf("Test offset. For exit input non number\n");
	size_t offs_byte = 0;
	size_t offs_pos = 0;

	entry = NULL;
	int res=0;
	while (1){
		if (scanf("%lu", &offs_pos) == 0)
			break;
		//printf("Pos is %lu\n", offs_pos);
		entry = aesd_circular_buffer_find_entry_offset_for_fpos(buf, offs_pos, &offs_byte);
	if (entry != NULL)
		printf("Found %s. Position is %lu\n", entry->buffptr, offs_byte);
	}
}