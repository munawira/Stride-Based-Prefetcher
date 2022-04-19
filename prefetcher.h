/* Authors : 
	     Munawira Kotyad
*/


#ifndef _PREDICTORS_H
#define _PREDICTORS_H


#define NUM_TABLE_ENTRIES 10
#define NUM_WARPS 1000

int GS_size;
int PWS_size;
int IP_size;
int PWS_LRU[NUM_WARPS];
int GS_LRU =0;
int IP_LRU =0;

bool GS_found;
bool PWS_found;
bool IP_found;


/*LRU update for GS,IP, and PWS*/

void GS_LRU_update(int current_entry);
void PWS_LRU_update(int current_entry, int warp_id);
void IP_LRU_update(int current_entry);


class strideCounter{
	public:
	unsigned long stride;
	unsigned long strideCount;
	bool taken;
	strideCounter();
};


/* STRIDE TRAINER ===> PWS and GS */

class strideTrainer{
	public:

		unsigned long PC;
		unsigned long full_address[3];
		int tid[3];
		unsigned long stride[3];
		unsigned long trained_stride;
		bool trained;
		strideCounter SC[3];
		int current_index;
		int LRU;
		unsigned long last_addr;
		int last_tid;
		
		// Implementation related functioned
		strideTrainer();
		unsigned long strideCalculator();
		void createStrideEntry(unsigned long stride);

};

strideTrainer PWS[NUM_WARPS][NUM_TABLE_ENTRIES];
strideTrainer IP[NUM_TABLE_ENTRIES];

class globalStride{
	public:
		unsigned long PC;
		unsigned long stride;
		int LRU;
		bool trained;
		//strideCounter *SC;

		globalStride();

};

globalStride GS[NUM_TABLE_ENTRIES];

unsigned long GS_access(unsigned long pc, unsigned long mem_addr, int warp_id, int tid);
unsigned long GS_promote(unsigned long pc);
unsigned long PWS_access(unsigned long pc, unsigned long mem_addr, int warp_id, int tid);
void PWS_update(unsigned long pc, unsigned long mem_addr, int warp_id, int tid);
unsigned long IP_access(unsigned long pc, unsigned long mem_addr, int warp_id, int tid);
void IP_update(unsigned long pc, unsigned long mem_addr, int warp_id, int tid);


//void prefetch(unsigned long mem_addr, char *pc);



#endif
