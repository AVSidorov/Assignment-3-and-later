#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aesd-circular-buffer.h"
#include "queue.h" // for data structures like single linked list, single linked queue etc.

/* Data structures and queue */

typedef struct qentry{
    char* buffptr;
    size_t size;
} qentry_t;

typedef struct qentry_node{
    aesd_buffer_entry_t *entry;
    STAILQ_ENTRY(qentry_node) next;
} qentry_node_t;


typedef STAILQ_HEAD(aesd_buffer_entry_queue, qentry_node) queue_t;

queue_t queue = STAILQ_HEAD_INITIALIZER(queue);
size_t queue_size=0;

aesd_circular_buffer_t *circ_buf;

/***************************************************************
* Test prototype for write function in driver
****************************************************************/


ssize_t aesd_write(const char *ubuf, size_t count, size_t *f_pos)
{

	// TODO change error codes to constants
	ssize_t retval = -1;
	uint8_t packet = 0;

	char *pos = NULL;

	qentry_node_t *node=NULL;
	aesd_buffer_entry_t *full_cmd;

	char *full_buf=NULL;
	void *del_buf=NULL; // to save pointer to free memory

	size_t buf_size = count*sizeof(char);

	//TODO kmalloc
	char *buf = malloc(buf_size);

	if (!buf){
		perror("Error allocate buffer for write data");
		goto clean_buf;
	}
	memset(buf, 0, buf_size);

	// TODO retval = copy_from_user()
	// TODO PDEBUG size of copied
	if (memcpy(buf, ubuf, buf_size) == NULL){
		perror("Error copy from user buf");
		goto clean_buf;
	}


	// if neccessary trim buf. Then check last symbol in entry buf will work
	if((pos = strchr(buf,'\n'))){
		if (((pos-buf)+1)*sizeof(char) < buf_size){
			memset(pos+1, 0, buf_size - (pos-buf)*sizeof(char));
			buf_size = (pos-buf+1)*sizeof(char);
		}
		pos = NULL;
	}
	retval = buf_size;

	// TODO change go kmalloc
	node = malloc(sizeof(qentry_node_t));
	if (!node){
		perror("Error allocate node");
		goto clean_buf;
	}
	memset(node, 0, sizeof(qentry_node_t));

	// TODO change go kmalloc
	node->entry = malloc(sizeof(aesd_buffer_entry_t));
	if (!node->entry){
		perror("Allocate entry");
		goto clean_node;
	}
	memset(node->entry, 0, sizeof(aesd_buffer_entry_t));

	node->entry->buffptr = buf;
	node->entry->size = buf_size;
	// Store buf to clear memory in case of fail


	// add full command to circular buffer
	// check can pass. Buffer should be cut at beginning of function
	if (node->entry->buffptr[node->entry->size - 1] == '\n'){
		// TODO change go kmalloc

		full_cmd = malloc(sizeof(aesd_buffer_entry_t));
		if (!full_cmd){
			perror("Error allocation entry for full command");
			goto clean_entry;
		}


		// TODO change go kmalloc
		full_buf = malloc(queue_size + node->entry->size);
		if (!full_buf){
			perror("Error allocate full command buffer");
			goto clean_full_cmd;
		}
		full_cmd->size = queue_size + node->entry->size;
		memset(full_buf, 0, full_cmd->size);

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
		pos = full_buf;
		while(!STAILQ_EMPTY(&queue)){
			node = STAILQ_FIRST(&queue);
			STAILQ_REMOVE_HEAD(&queue, next);

			if(memcpy(pos, node->entry->buffptr, node->entry->size) == NULL){
				perror("Error copy to full command buffer");
				retval= -2; //
			}
			pos += node->entry->size;
			free((void *)(node->entry->buffptr));
			free(node->entry);
			free(node);
			node = NULL;
		}

		queue_size = 0; //queue_size is part of packet(queue). Access must be locked
		pos = NULL;
		// TODO unlock packet


		// TODO lock circular buffer

		// We need free memory from first command in buffer
		if (circ_buf->full)
			del_buf = (void *)(aesd_circular_buffer_find_entry_offset_for_fpos(circ_buf, 0, NULL) ->buffptr);

		full_cmd->buffptr = full_buf;
		aesd_circular_buffer_add_entry(circ_buf, full_cmd);

		// data from full_cmd copied to circular buf
		// here free full_cmd
		free(full_cmd);
		full_cmd=NULL;

		// here full_cmd data already replaced in circular buffer entry. But we stored in del_buf pointer to old command
		if(del_buf){ //here del_cmd NULL or saved entry
			free(del_buf);
			del_buf=NULL;
		}
		// TODO unlock circular buffer
		packet = 0;
	}

	return retval;

	// Cleanup in case of errors
	clean_full_buffptr: free(full_buf);
	clean_full_cmd: free(full_cmd);
	clean_entry: free(node->entry);
	clean_node:	free(node);
	clean_buf: free(buf);

	ret: return retval;
}

/***************************************************************
* Test prototype for read function in driver
****************************************************************/

ssize_t aesd_read(char *ubuf, size_t count, size_t *f_pos)
{
	ssize_t retval = 0;
	//PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    //struct aesd_divice *dev = filp->private_data;

    aesd_buffer_entry_t *entry;
    size_t offs_entry; //for avoiding "transfer" pointer and calculations
    size_t offs_full=*f_pos;

//	if (mutex_lock_interruptible(&dev->circ_buf_lock))
//		return -ERESTARTSYS;
	entry = aesd_circular_buffer_find_entry_offset_for_fpos(circ_buf, offs_full, &offs_entry);
	if (!entry){ //Entry not found
		retval=0;
		goto out;
	}

	/* Calculate size to return. Return no more than one command (till end entry buffer)*/
	if ((entry->size - offs_entry) < count)
		count = entry->size - offs_entry;


	// TODO copy_to_user
	if (memcpy(ubuf,  entry->buffptr + offs_entry, count) == NULL){
		perror("Error copy data to user");
		retval = -1;
	}
	*f_pos += count;
	retval = count;

	//out: mutex_unlock(&dev->cicr_buf_lock);
	out: return retval;
}


/***************************************************************
*	Circular Buffer Test Functions (aesd-circular-buffer.h)
****************************************************************/



void print_buf(const aesd_circular_buffer_t *buf){
	const aesd_buffer_entry_t *entry = NULL;
	int i=0;

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

void read_buf(size_t count, size_t pos_from){
	char buf_read[1024];
	memset((char *)&buf_read, 0, 1024);
	count = count % 1025;
	size_t size_read;
	size_t pos=pos_from;

	printf("\n===Read full buffer size %lu bytes from pos %lu ===\n", count, pos);
	while ((size_read = aesd_read((char *)&buf_read, count, &pos))){
		printf("%s", buf_read);
		count -= size_read;
		memset(&buf_read, 0, 1024);
		printf("\t:%lu bytes read %lu new count, %lu new pos\n", size_read, count, pos);
	}
	printf("\n===End Test===\n");
}

void test_get_entry(aesd_circular_buffer_t *buf){
	aesd_buffer_entry_t *entry = NULL;
	aesd_buffer_entry_t *entry_print = NULL;

	size_t offs_byte = 0;
	size_t offs_pos = 0;

//	printf("Test offset. For exit input non number\n");
//			if (scanf("%lu", &offs_pos) == 0)
//			break;

	char buf1[100], buf2[100];
	int i=0;

	printf("\n===Test find_entry_offset_for_fpos===\n\n");

	while ((entry = aesd_circular_buffer_find_entry_offset_for_fpos(buf, offs_pos, &offs_byte))){
		AESD_CIRCULAR_BUFFER_FOREACH(entry_print,buf,i){
		if (buf->in_offs == i)
			printf("\033[31m%s\033[0m", entry_print->buffptr);
		else if ( entry == entry_print)
			printf("\033[32m%s\033[0m", entry_print->buffptr);
		else
			printf("%s", entry_print->buffptr);
		}
		printf("\n");

		memset(buf1, 0, 100);
		memset(buf2, 0, 100);
		memcpy(buf1, entry->buffptr, offs_byte);
		memcpy(buf2, entry->buffptr+offs_byte+1, entry->size-offs_byte-1);

		printf("%lu is position in %s\033[33m%c\033[0m%s\n\n", offs_byte, buf1, entry->buffptr[offs_byte], buf2);
		offs_pos++;
	}

	printf("\n===End Test===\n");

	entry = NULL;

}

/******************************************************
*		main
*******************************************************/

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

	printf("\n");
	printf("======================================\n");
	printf("||Test circular buffer base functions||\n");
	printf("======================================\n");
	printf("\n");


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

	test_get_entry(buf);


	printf("\n");
	printf("=====================================\n");
	printf("||Test full dynamic circular buffer||\n");
	printf("=====================================\n");
	printf("\n");



	qentry_node_t *node;

	circ_buf=malloc(sizeof(aesd_circular_buffer_t));
	aesd_circular_buffer_init(circ_buf);

	if (circ_buf->full)
	 	printf("!!!!!!!!!!!Full Just after init\n");

	printf("Test write function\n");

	size_t wr_size=0;
	aesd_write("write1\n", 7, &wr_size);
	aesd_write("wri", 3, &wr_size);
	aesd_write("te2",3, &wr_size);
	aesd_write("\n",1, &wr_size);
	print_buf(circ_buf);
	aesd_write("write3\ndads", 11, &wr_size);
	aesd_write("write4\n", 7, &wr_size);
	print_buf(circ_buf);
	aesd_write("write5\n", 7, &wr_size);
	aesd_write("write6\n", 7, &wr_size);
	aesd_write("write7\n", 7, &wr_size);
	aesd_write("write8\n", 7, &wr_size);
	aesd_write("write9\n", 7, &wr_size);
	aesd_write("write10\n", 8, &wr_size);
	print_buf(circ_buf);
	aesd_write("write11", 7, &wr_size);
	aesd_write("\n ", 2, &wr_size);
	aesd_write("write12\n", 8, &wr_size);
	print_buf(circ_buf);

	printf("\n\n==================================\n");
	printf("||Test read from circular buffer||\n");
	printf("==================================\n");

	read_buf(1024,0);
	read_buf(1,0);
	read_buf(3,0);
	read_buf(7,0);
	read_buf(9,0);
	read_buf(15,0);
	read_buf(15,5);
	read_buf(9,12);
	read_buf(10,12);
	read_buf(10,10);

	printf("\n===Clean Circular Buffer===\n");

	AESD_CIRCULAR_BUFFER_FOREACH(entry, circ_buf,i){
		free((void *)(entry->buffptr));
		//free(entry);
		entry = NULL;
	}
	// clean all pointers to avoid access
	aesd_circular_buffer_init(circ_buf);

	print_buf(circ_buf);

	read_buf(1024,0);

	return 0;
}