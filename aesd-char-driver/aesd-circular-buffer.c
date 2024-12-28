/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
   int current_offset = 0;
   int current_index = 0;   

	for (int i = buffer->out_offs; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED + buffer->out_offs; i++){
		current_index = i % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

		if ((char_offset - current_offset) < buffer->entry[current_index].size){
			*entry_offset_byte_rtn = char_offset - current_offset;
            PDEBUG("AESDREAD: found at %d \n",current_index);
            //PDEBUG("AESDREAD: found  %s \n",buffer->entry[current_index].buffptr);
            PDEBUG("AESDREAD: size found  %d \n",buffer->entry[current_index].size);
            //PDEBUG("AESDREAD: entry_offset_byte_rtn  %d \n",*entry_offset_byte_rtn);
			return &buffer->entry[current_index];
		}

		current_offset += buffer->entry[current_index].size;
	}
    PDEBUG("AESDREAD: not found\n");
    return NULL;
}

/*
void copy_circular_buffer(struct aesd_circular_buffer *aesd_buffer,  int *output_buffer) {
    const char *arrayBuffer = aesd_buffer->entry;
    int buffer_size = aesd_buffer->entry->size;
    int head = aesd_buffer->in_offs;
    int tail = aesd_buffer->out_offs;        
    int num_elements = (head - tail + buffer_size) % buffer_size;


    
    // Copia gli elementi in ordine di inserimento
  for (int i = 0; i < num_elements; i++) {
        int current_index = (tail + i) % buffer_size;
        output_buffer[i] = arrayBuffer[current_index]; // Copia i puntatori
    }
}*/

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    const char* entryToDelete = NULL;

     if(buffer->full)  {             
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;       
        entryToDelete = buffer->entry[buffer->in_offs].buffptr;  
        kfree(entryToDelete);
     }

     buffer->entry[buffer->in_offs].buffptr = add_entry->buffptr;

     buffer->entry[buffer->in_offs].size = add_entry->size;

     if((buffer->in_offs +1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED == buffer->out_offs)     
        buffer->full = true;     

     buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
     PDEBUG("aesd_circular_buffer_add_entry in out %d %d ",buffer->in_offs, buffer->out_offs); 
     
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
