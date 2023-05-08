#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aesd-circular-buffer.h"
#include "queue.h" // for data structures like single linked list, single linked queue etc.

/* Data structures and queue */


typedef struct qentry_node{
    aesd_buffer_entry_t *entry;
    STAILQ_ENTRY(qentry_node) next;
} qentry_node_t;

typedef STAILQ_HEAD(aesd_buffer_entry_queue, qentry_node) queue_t;

queue_t queue = STAILQ_HEAD_INITIALIZER(queue);
size_t queue_size=0;

aesd_circular_buffer_t *circ_buf;

/* Test prototype for write function in driver*/

ssize_t aesd_write(const char *buf, size_t count, size_t *f_pos)
{
	// TODO using f_pos

	// TODO change error codes to constants
	ssize_t retval = -1;
	qentry_node_t *node;
	aesd_buffer_entry_t *full_cmd;

	size_t buf_size = count*sizeof(char);
	uint8_t packet = 0;

	// TODO change go kmalloc
	node = malloc(sizeof(qentry_node_t));
	if (!node){
		perror("Error allocate node");
		goto ret;
	}

	// TODO change go kmalloc
	node->entry = malloc(sizeof(aesd_buffer_entry_t));
	if (!node->entry){
		perror("Allocate entry");
		goto clean_node;
	}

	// TODO change go kmalloc
	node->entry->buffptr = malloc(buf_size);
	node->entry->size = buf_size;
	if(!node->entry->buffptr){
		perror("Error allocate buf");
		goto clean_entry;
	}

	// TODO retval = copy_from_user()
	// TODO PDEBUG size of copied
	if (memcpy(node->entry->buffptr, buf, buf_size) == NULL){
		perror("Error copy from buf");
		goto clean_buffptr;
	}
	// TODO real copied size
	// node->entry->size = retval;



	// add full command to circular buffer
	if (node->entry->buffptr[node->entry->size - 1] == '\n'){
		// TODO change go kmalloc
		full_cmd = malloc(sizeof(aesd_buffer_entry_t));
		if (!full_cmd){
			perror("Error allocation entry for full command");
			goto clean_buffptr;
		}

		// TODO change go kmalloc
		full_cmd->buffptr = malloc(queue_size + node->entry->size);
		if (!full_cmd->buffptr){
			perror("Error allocate full command buffer");
			goto clean_full_cmd;
		}
		packet = 1;
	}

	// in case of full packet last part will be added only if all allocation succeeded
	// caller gets the error and has ability resend (retry) last part and fulfill command
	// TODO lock packet
	if (STAILQ_EMPTY(&queue)){
		STAILQ_NEXT(node, next) = NULL;
		STAILQ_INSERT_HEAD(&queue,node, next);
	}
	else
		STAILQ_INSERT_TAIL(&queue, node, next);

	// collect full size of packet
	queue_size += node->entry->size;

	// TODO unlock packet

	// here should be all allocation success. Add command to circular buffer
	if (packet){
		// TODO lock packet
		char *pos = full_cmd->buffptr;
		while(!STAILQ_EMPTY(&queue)){
			node = STAILQ_FIRST(&queue);
			STAILQ_REMOVE_HEAD(&queue, next);

			if(memcpy(pos, node->entry->buffptr, node->entry->size) == NULL){
				perror("Error copy to full command buffer");
				retval= -2; //
			}
			pos += node->entry->size;
			free(node->entry->buffptr);
			free(node->entry);
			free(node);
			node = NULL;
		}

		full_cmd->size = queue_size;
		queue_size = 0; //queue_size is part of packet(queue). Access must be locked
		// TODO unlock packet

		// TODO lock circular buffer
		aesd_circular_buffer_add_entry(circ_buf, full_cmd);
		// TODO unlock circular buffer
		packet = 0;
		goto ret;
	}

	return buf_size;

	// Cleanup in case of errors
	clean_full_buffptr: free(full_cmd->buffptr);
	clean_full_cmd: free(full_cmd);
	clean_buffptr: free(node->entry->buffptr);
	clean_entry: free(node->entry);
	clean_node:	free(node);

	ret: return retval;
}

void print_buf(const aesd_circular_buffer_t *buf){
	aesd_buffer_entry_t *entry;
	int i;

	printf("Print full buffer\n");
	AESD_CIRCULAR_BUFFER_FOREACH(entry,buf,i){
		if (buf->in_offs == i)
			printf("\033[31m%s\033[0m|", entry->buffptr);
		else if (buf->out_offs == i)
				printf("\033[32m%s\033[0m|", entry->buffptr);
			else
				printf("%s|", entry->buffptr);

	}
	printf("\n");
}

int main(int argc, char *argv[]){

	/* Initialization*/

	//Parse input arguments

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

	// test Set
	aesd_buffer_entry_t entry_set[6] = {{"a", 1}, {"bc", 2}, {"def", 3}, {"Sid ",4}, {"is here!", 8}, {"",0}};

	// Counters for loops
	uint8_t i,j;

	/* Tests*/

	printf("Test queue\n");

	qentry_node_t *node;

	for (i=0; i < entry_count; i++ ){
		node = malloc(sizeof(qentry_node_t));
		node->entry = &entry_set[i];
		if (STAILQ_EMPTY(&queue)){
			STAILQ_NEXT(node, next) = NULL;
			STAILQ_INSERT_HEAD(&queue,node, next);
		}
		else
			STAILQ_INSERT_TAIL(&queue, node, next);
	}
	STAILQ_FOREACH(node, &queue, next){
		printf("%s|", node->entry->buffptr);
	}
	printf("\n");

	printf("Test buffer\n");

	aesd_circular_buffer_t *buf=malloc(sizeof(aesd_circular_buffer_t));
	aesd_circular_buffer_init(buf);

	for (i=0; i < write_count; i++)
		for (j=0; j < entry_count; j++ )
			aesd_circular_buffer_add_entry(buf,&entry_set[j]);


	print_buf(buf);
	aesd_buffer_entry_t *entry;


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

	printf("Test write function\n");

	//clean queue
	while(!STAILQ_EMPTY(&queue)){
		node = STAILQ_FIRST(&queue);
		STAILQ_REMOVE_HEAD(&queue, next);
		// here entries are not dynamic
		// These are array elements on stack
//		free(node->entry->buffptr);
//		free(node->entry);
		free(node);
		node = NULL;
	}

	circ_buf=malloc(sizeof(aesd_circular_buffer_t));
	aesd_circular_buffer_init(circ_buf);
	size_t wr_size=0;
	aesd_write("write1\n", 7, &wr_size);
	aesd_write("wri", 3, &wr_size);
	aesd_write("te2",3, &wr_size);
	aesd_write("\n",1, &wr_size);
	aesd_write("write3\n", 7, &wr_size);
	aesd_write("write4\n", 7, &wr_size);
	aesd_write("write5\n", 7, &wr_size);
	aesd_write("write6\n", 7, &wr_size);
//	aesd_write("write7\n", 7, &wr_size);
//	aesd_write("write8\n", 7, &wr_size);
//	aesd_write("write9\n", 7, &wr_size);
//	aesd_write("write10\n", 8, &wr_size);
//	aesd_write("write11", 7, &wr_size);
//	aesd_write("\n", 1, &wr_size);
//	aesd_write("write12\n", 8, &wr_size);
	print_buf(circ_buf);
}