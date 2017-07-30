#include <iostream>
#include <cstring>

class SmallAllocator {
public:
	SmallAllocator()
		: mem_(NULL)
		, mem_size(0)
	{}	
	
	void Init(void *buf, size_t size) {
		mem_= static_cast<char*>(buf);
		mem_size = size;
		
		Tag *head = (Tag *) (mem_);
		Tag *tail = (Tag *) (mem_ + mem_size - sizeof(Tag));
		
		*head = Tag(mem_size, 0);
		*tail = Tag(mem_size, 0);	
	}
	
	void *Alloc(size_t size);
	void Free(void *ptr);
		
private:
	struct Tag {
		size_t raw_size : 31;
		size_t occupied :  1;
	
		Tag(size_t raw_size, size_t occupied)
			: raw_size(raw_size)
			, occupied(occupied)
		{}
		
		int size() {
			return (int) raw_size 
			     - (int) (2 * sizeof(Tag));
		}
	};

	char *mem_;
	size_t mem_size;
};

void *SmallAllocator::Alloc(size_t size) 
{
	char *begin = mem_;
	char *end   = mem_ + mem_size;
	
	char *ptr = begin;
	
	do {
		Tag *head = (Tag *) ptr;
		Tag *tail = (Tag *) (ptr + sizeof(Tag) + head->size());
		
		if (head->size() < size || head->occupied == 1) {
			ptr += head->raw_size;
			continue;
		}
		
		head->occupied = 1;
		
		size_t new_raw_size = size + 2 * sizeof(Tag); 
		
		if (head->size() > new_raw_size) {
			Tag *new_tail = (Tag *) (ptr + sizeof(Tag) + size);
			Tag *new_head = new_tail + 1;
			
			head->raw_size = new_raw_size;
			*new_tail = Tag(new_raw_size, 1);
			
			*new_head = Tag(tail->raw_size - new_raw_size, 0);
			tail->raw_size = tail->raw_size - new_raw_size;
		} else {
			tail->occupied = 1;
		}
		
		return ptr + sizeof(Tag);
		
	} while (ptr < end); 
	
	return NULL;
}

void SmallAllocator::Free(void *ptr)
{
	Tag *head = (Tag *) ptr - 1; 
	Tag *tail = (Tag *) ((char *) ptr + head->size());
	
	size_t raw_size = head->raw_size;
	
	char *begin = mem_;
	char *end   = mem_ + mem_size;
	
	if ((char *) head > begin && (head - 1)->occupied == 0) {
		head = (Tag *) ((char *) head - (head - 1)->raw_size);
		raw_size += head->raw_size;	
	}
	
	if ((char *) (tail + 1) < end && (tail + 1)->occupied == 0) {
		tail = (Tag *) ((char *) tail + (tail + 1)->raw_size);
		raw_size += tail->raw_size;
	}
	
	head->occupied = 0;
	tail->occupied = 0;
	
	head->raw_size = raw_size;
	tail->raw_size = raw_size;
}

SmallAllocator salloc;

void mysetup(void *buf, std::size_t size)
{
	salloc.Init(buf, size);
}


void *myalloc(std::size_t size)
{
	return salloc.Alloc(size);
}


void myfree(void *p)
{
	salloc.Free(p);
}
