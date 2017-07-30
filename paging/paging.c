#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct Memory;
struct Proc;

struct Memory *init_mem();
void free_mem(struct Memory *mem);
void fill_mem(struct Memory *mem, FILE *data, int m);
uint64_t movq(struct Memory *mem, uint64_t addr);

struct Proc *init_proc(uint64_t pt_addr); 

uint64_t translate(struct Proc   *proc, 
		   struct Memory *mem, 
		   uint64_t      logic_addr);

#define FAULT 0xFFFFFFFFFFFFFFFFLU

int main(int argc, char **argv)
{
	if (argc < 2) {
		return -1;
	}

	FILE *f = fopen(argv[1], "r");
	
	if (f == NULL) {
		return -1;
	}
	
	uint64_t m, q, r;
	fscanf(f, "%lu %lu %lu", &m, &q, &r);

	struct Memory *mem = init_mem();
	
	if (mem == NULL) {
		return -1;
	}
	
	fill_mem(mem, f, m);

	struct Proc *proc = init_proc(r);

	if (proc == NULL) {
		return -1;
	}

	FILE *out = fopen("translations.txt", "w");
	
	if (out == NULL) {
		return -1;
	}

	int i = 0;
	uint64_t phys_addr = 0, logic_addr;
	
	for ( ; i < q; i++) {
		fscanf(f, "%lu", &logic_addr);
		
		phys_addr = translate(proc, mem, logic_addr);
	
		if (phys_addr != FAULT) {
			fprintf(out, "%lu\n", phys_addr);
		} else {
			fprintf(out, "%s\n", "fault");
		}	
	}

	free(proc);
	free_mem(mem);
	fclose(f);
	fclose(out);
	return 0;
}

#define TABLE_ID_MASK  0x00000000000001FFUL
#define OFFSET_MASK    0x0000000000000FFFUL
#define PHYS_ADDR_MASK 0x000FFFFFFFFFF000UL

#define BUCKETS_NUM    (1 << 20)

struct QWord {
	uint64_t addr;
	uint64_t val;
	struct QWord *next;
};

struct Memory {
	struct QWord *qwords[BUCKETS_NUM];
};

struct Proc {
	uint64_t pt_addr;
};

uint64_t translate(struct Proc   *proc, 
	           struct Memory *mem, 
	           uint64_t      logic_addr)
{	
	int offset  = logic_addr & OFFSET_MASK;
	
	int table   = (logic_addr >> 12) & TABLE_ID_MASK;
	int dir     = (logic_addr >> 21) & TABLE_ID_MASK;
	int dir_ptr = (logic_addr >> 30) & TABLE_ID_MASK;
	int pml4    = (logic_addr >> 39) & TABLE_ID_MASK;
	
	uint64_t pml4e = movq(mem, proc->pt_addr + 8 * pml4);
	
	if (!(pml4e & 0x1UL)) {
		return FAULT;
	}
	
	uint64_t pdpte = movq(mem, (pml4e & PHYS_ADDR_MASK) + 8 * dir_ptr);
	
	if (!(pdpte & 0x1UL)) {
		return FAULT;
	}
	
	uint64_t pde = movq(mem, (pdpte & PHYS_ADDR_MASK) + 8 * dir);
	
	if (!(pde & 0x1UL)) {
		return FAULT;
	}
	
	uint64_t pte = movq(mem, (pde & PHYS_ADDR_MASK) + 8 * table);
	
	if (!(pte & 0x1UL)) {
		return FAULT;
	}
	
	return (pte & PHYS_ADDR_MASK) + offset;
}

uint64_t movq(struct Memory *mem, uint64_t addr)
{	
	struct QWord *curr = mem->qwords[addr % BUCKETS_NUM];
	
	do {
		if (curr == NULL) {
			return 0x0UL;
		}
		
		if (curr->addr == addr) {
			break;
		}
		
		curr = curr->next;
	} while (1);
	
	return curr->val;
}

struct Proc *init_proc(uint64_t pt_addr)
{
	struct Proc *proc = NULL;
	proc = malloc(sizeof(struct Proc));
	
	if (proc == NULL) {
		return NULL;
	}
	
	proc->pt_addr = pt_addr;
	
	return proc;
}

struct Memory *init_mem()
{
	struct Memory *mem = NULL;
	mem = malloc(sizeof(struct Memory));
	
	if (mem == NULL) {
		return NULL;
	}
	
	int i = 0;
	
	for ( ; i < BUCKETS_NUM; i++) {
		mem->qwords[i] = NULL;
	}

	return mem;
}

void free_mem(struct Memory *mem)
{
	int i = 0;
	
	for ( ; i < BUCKETS_NUM; i++) {
		struct QWord *curr = mem->qwords[i];
	
		while (curr != NULL) {
			struct QWord *tmp = curr->next;
			
			free(curr);	
			
			curr = tmp;
		}
	
		mem->qwords[i] = NULL;
	}
	
	free(mem);	
}

void fill_mem(struct Memory *mem, FILE *data, int m)
{
	int i = 0;
	
	for ( ; i < m; i++) {
		struct QWord *qw = malloc(sizeof(struct QWord));
	
		fscanf(data, "%lu %lu", &qw->addr, &qw->val);
		
		qw->next = mem->qwords[qw->addr % BUCKETS_NUM];
		
		mem->qwords[qw->addr % BUCKETS_NUM] = qw;
	}
}
