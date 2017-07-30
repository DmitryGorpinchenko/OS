#include <stdio.h>

#define MAX16BIT      (1 << 16)
#define BOOTLOAD_ADDR 0x7c00

int main()
{
	int count = 0;
	int seg   = 0;
	int off   = 0;
	
	for ( ; seg < MAX16BIT; seg++) {
		off = 0;
	
		for ( ; off < MAX16BIT; off += 16) {
			int phys_addr = (seg * 16 + off)
					     % (1 << 20); 
			
			if (phys_addr == BOOTLOAD_ADDR) {
				count++;
			}
		}
	}
	
	printf("%d\n", count);
	
	return 0;
}
