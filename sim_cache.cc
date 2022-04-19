/* Author: Munawira Kotyad 


*  ECE 786 : Project : Prefetcher Implementation         */


#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <iomanip>
using namespace std;

#include "prefetcher.h"
#include "prefetcher.cc"

#define PREFETCH_ENABLE 1 

unsigned long BLOCKSIZE =0;
unsigned long  L1_SIZE=0;
unsigned long L1_ASSOC=0;
unsigned long  L1_PREF_N=0;
unsigned long L1_PREF_M=0;
unsigned long  L2_SIZE=0;
unsigned long  L2_ASSOC=0;
unsigned long  L2_PREF_N=0;
unsigned long  L2_PREF_M=0;

unsigned long counter = 0;// Counts the number of read write operations
int written = 0;//checks for read or write
int L2_presence =0;//checks for L2_SIZE

int SBhit;//checks for Stream Buffer Hit
int victimDirty =0;//Checks if the L1 victim is dirty

int L2miss =0;
int prefetch =0;
double L1reads =0;
double L1readMiss=0;
double L1writes=0;
double L1writeMiss=0;
double L1missrate =0;
double L1writeback =0;
double L1prefetch =0;
double L2reads =0;
double L2readMiss =0;
double L2reads_prefetch =0;
double L2readmiss_prefetch =0;// L2 read misses that originated from prefetches
double L2writes=0;
double L2writeMiss=0;
double L2missrate =0;
double L2writeback =0;
double L2prefetches =0;
double memtraffic =0;

/* Prefetch metrics */

double unused_prefetches =0;
double useful_prefetches =0;
double demand_misses =0;

/* To create sets, arrays of blocks will be created */

/*-----------------------------------------------BLOCK BEGINS HERE----------------------------------------*/
class block
{
	char tag[20]; 
	unsigned long address;
	bool dirty;
	bool valid;
	double cycle_fetched;

	public:
	block()
	{
		dirty = false;
		valid = false;
		address = 0;
		cycle_fetched = -1;
	}
	int setAddress(unsigned long addr)
	{
		address = addr;
		return 1;
	}
	unsigned long getAddress()
	{
		return address;
	}
	int setDirty()
	{
		dirty = true;
		
		return 0;
	}
	int setClean()
	{
		dirty = false;
		return 0;
	}

	int setValid()
	{
		valid = true;
		return 0;
	}
	int setInvalid()
	{
		valid = false;
	}

	bool getDirty()
	{
		return dirty;
	}

	bool getValid()
	{
		return valid;
	}

	int setTag(char* Tag)
	{
		strcpy(tag,Tag);
		return 1;
	}

	char* getTag()
	{
		return tag;
	}

/* Other Functions for Block class*/

};

/*--------------------------------------------------------BLOCK ENDS HERE---------------------------------------*/

/*--------------------------------------------------------CACHE BEGINS HERE-------------------------------------*/
/* L1 and L2 cache to be implemented using this class */
class cache
{
public:
int ASSOC;
int numSets;
int **LRU;
int SIZE;
block **set;


public:
 void cacheinitialize(int size, int assoc)
{
	
	ASSOC = assoc;
	SIZE = size;

	if (size != 0 && assoc != 0)
	numSets = SIZE/(ASSOC * BLOCKSIZE);
	else
	numSets =0;
	if(assoc == 0)
	numSets = 1;
	


	LRU = new int* [numSets];
  	if(LRU == NULL)
      	{
        	cout<<"error in LRU malloc \n";
      	}
   	for (int i=0;i<numSets; i++)
      	{
        	LRU[i] =new int[ASSOC];
        	if (LRU[i] == NULL)
        	{
          	cout<<"error in LRU * malloc \n";
        	}
      	}
	for(int j=0; j<numSets; j++)
	for(int i=0; i < ASSOC; i++)
	{
	LRU[j][i] = ASSOC;
	}

	set = new block* [numSets];
		
      if(set == NULL)
      {
        	cout<<"error in set malloc \n";
      }
      for (int i=0;i<numSets; i++)
      {
        	set[i] =new block[ASSOC];
        	if (set[i] == NULL)
        	{
          	cout<<"error in set * malloc \n";
        	}
      }
}

int getNumSets()
{
	return numSets;
}

int updateLRU(unsigned long index, int blockid)
{
	
	
	for(int i=0; i<ASSOC ; i++)
	{
		if(LRU[index][i] < LRU[index][blockid])
		LRU[index][i]++;
		
	}
	LRU[index][blockid] = 1;
	
	return 0;
}

int cacheSearch( char* tag, unsigned long index)
{
	for(int i=0; i<ASSOC; i++)

	{		
		if(set[index][i].getValid())
		{
			
			if(strcmp(set[index][i].getTag(),tag) == 0)
			{
			updateLRU(index,i);
			
			return i;
			}
		
		}
	}
		return (-1);
}

int setVal(char* tag,unsigned long address, unsigned long index, int block)
{	
	set[index][block].setTag(tag);
	set[index][block].setAddress(address);
	set[index][block].setValid();
	if(written == 1)
	set[index][block].setDirty();
	
	return 1;

}

int checkEmpty(unsigned long index)
{
	for(int i=0; i< ASSOC; i++)
	{
		if(!set[index][i].getValid())
		return i;
	}
	return -1;

}

int findVictim(unsigned long index)
{
	int victim  = 0;
	for(int i=0; i<ASSOC; i++)
	{
		if(LRU[index][i] > LRU[index][victim] /*|| LRU[index][i] == ASSOC*/)
		{
			victim =i;
		
		}
	}
	
	return victim;
}


};
/* ---------------------------------------------CACHE ENDS HERE---------------------------------------------------------*/




/* -----------------------------------------STREAM BUFFER STARTS HERE-----------------------------------------------------*/
class streamBuffer
{
public:
int numBlocks;
block *SB;
int LRU;
unsigned long cacheIndex;
int blockid;

public:
streamBuffer(){}

void initialize(int numblocks,int numStreambuffers)
{
numBlocks = numblocks;
SB = new block[numBlocks];

LRU = numStreambuffers;

}

int setIndex(unsigned long index)
{
	cacheIndex = index;
} 

unsigned long getIndex()
{
	return cacheIndex;
} 

int setBlockid(int id)
{
	blockid = id;
} 

unsigned long getBlockid()
{
	return blockid;
} 

int getLRU()
{
	return LRU;
}

int setLRU(int lru)
{

	LRU = lru;
}

int incrementLRU()
{
	LRU++;

}

	

int setVal(char* tag,unsigned long address, unsigned long index, int block)
{	
	SB[block].setTag(tag);
	SB[block].setAddress(address);
	SB[block].setValid();	
	return 1;
}


/*To check the first positions in the stream buffer*/
int streamBufferSearch(unsigned long address)
{	
	if(SB[0].getValid())
	{
	
	
	if(SB[0].getAddress() == address)
	return 1;

	}
	
	return(-1);
}

/*To check the entire stream buffer*/
int fullstreamBufferSearch(unsigned long address) 

{	
	unsigned long blockaddress;
	blockaddress = address - (address % BLOCKSIZE);
	for(int i =0;i < numBlocks; i++)
	{
			if(SB[i].getAddress() == blockaddress)
			{
			return i;
			}
	}
	return (-1);
}






};


/*------------------------------------STREAM BUFFER ENDS HERE-------------------------------------------*/

/*CACHE INITIALIZATION*/
cache L1;
cache L2;


/*STREAM INITIALIZATION*/

streamBuffer *SB_L1;
streamBuffer *SB_L2;

/*FORWARD DECLARATIONS*/
int readwrite( unsigned long full_address, char* L1_tag, char* L2_tag,unsigned long L1_index,unsigned long L2_index, bool prefetch);/* Implements writing into L1, L2 and stream buffers*/
int evictL1(unsigned long L1_index,int L1_blockid,unsigned long L2_index, char* L2_tag,  unsigned long address,bool prefetch );
int L1readmiss(char* L1_tag,unsigned long L1_index, unsigned long L2_index,  char* L2_tag, unsigned long address,bool prefetch );
int L2readmiss(unsigned long L2_index, char* tag, unsigned long address, bool prefetch);


int L1preFetch(int bufferId, char* tag,unsigned long L2_index,unsigned long full_address, unsigned long intAddress);

int L2preFetch(int bufferId, char* tag,unsigned long L2_index,unsigned long intAddress);

int indexcalc(unsigned long address,int numSets, unsigned long *index, unsigned long *tag);
int L2SBgetLRU();
int L1SBgetLRU();

int L1SBUpdateLRU(int sbnumber);
int L2SBUpdateLRU(int sbnumber);
int L1shiftUp(int bufferId);

int L2shiftUp(int bufferId);





/* ----------------------------------MAIN BEGINS HERE ---------------------------------------------------------------------*/



int main(int argc, char* argv[])
{
	FILE *trace;
	int warp_id;
	unsigned long PC;
	int thread_id;
	int GS_stride =0;
	int IP_stride =0;
	int funal_stride =0;
	int prefetch_width;
	char command = 'w';
	char address[20];
	unsigned long full_address;
	unsigned long stride_addr;
	unsigned long addr;
	unsigned long L1_index, L2_index;
	char L1_tag[20],L2_tag[20];
	unsigned long L1_inttag, L2_inttag;
	int L1_numsets, L2_numsets;
	int i =0;

	if(argc < 3)
	{
		printf("\n Usage: ./sim_cache prefetch_width <trace_file>");
		exit(1);
	}

	prefetch_width = atoi(argv[1]);

	for(i=0; i<NUM_WARPS;i++)
		PWS_LRU[i]=0;





	//TODO: Initialize all elements

	 
	/*Read command line arguments*/
	
	BLOCKSIZE = 4;
	L1_SIZE = 1024;
	L1_ASSOC = 1;
	L1_PREF_N = 0;
	L1_PREF_M = 0;
	L2_SIZE = 8192;
	L2_ASSOC = 4;
	L2_PREF_N = 0;
	L2_PREF_M = 0;
	
	


	cout<<"===== Simulator  Cache configuration ===== \n";
	cout<<"BLOCKSIZE:             "<< BLOCKSIZE << "\n";
	cout<<"L1_SIZE:               "<< L1_SIZE<< "\n";
	cout<<"L1_ASSOC:               "<< L1_ASSOC<< "\n";
	//cout<<"		L1_PREF_N:               "<< L1_PREF_N<< "\n";
	//cout<<"		L1_PREF_M:               "<< L1_PREF_M<< "\n";
	cout<<"L2_SIZE:		"<<L2_SIZE<<"\n";
	cout<<"L2_ASSOC:               "<< L2_ASSOC<< "\n";
	//cout<<"		L2_PREF_N:               "<< L2_PREF_N<< "\n";
	//cout<<"		L2_PREF_M:               "<< L2_PREF_M<< "\n";
	cout<<"trace_file:		"<<argv[2]<<"\n";

	cout<<"Prefetch Width :        "<< prefetch_width<< "\n";

	
	trace = fopen(argv[2], "r");
	/* check to see if trace is a null pointer */
	if(trace == NULL)			
	{
		perror("\n Input file cannot be opened!");
		exit(1);
	}

	/*Initializing L1 Cache*/
	L1.cacheinitialize(L1_SIZE,L1_ASSOC);
	/*Initializing L2 Cache*/
	L2.cacheinitialize(L2_SIZE,L2_ASSOC);
	
	/*Initializing L1 Stream Buffer*/	
	SB_L1 = new streamBuffer[L1_PREF_N];
	for(int s=0; s <L1_PREF_N ;s++)
	{
		SB_L1[s].initialize(L1_PREF_M,L1_PREF_N);
	}
	
	/*Initializing L2 Stream Buffer*/
	SB_L2 = new streamBuffer[L2_PREF_N];
	for(int s=0; s<L2_PREF_N ;s++)
	{
		SB_L2[s].initialize(L2_PREF_M,L2_PREF_N);
	}

	
	L1_numsets = L1.getNumSets();
	L2_numsets = L2.getNumSets();
	
	 if(L2_SIZE > 0)
	 L2_presence = 1;

	/* Reading the file for the access requests and the addresses */
	
	while(fscanf(trace, "%ld %lx %d %d",&PC,&full_address,&warp_id,&thread_id)>0)
	{
	
	
	indexcalc(full_address,L1_numsets, &L1_index,  &L1_inttag);
	if(L2_presence == 1)
	indexcalc(full_address,L2_numsets, &L2_index,  &L2_inttag);

	sprintf(L1_tag, "%lx", L1_inttag); 	/* Tag conversion to char*/
	sprintf(L2_tag, "%lx", L2_inttag);

	sprintf(address, "%lx", full_address); /* Address conversion to char*/


	if (command == 'w')
	{	written =1;
		L1writes++;


		readwrite(full_address,L1_tag,L2_tag,L1_index,L2_index,false);
		
	}else
	{
		if (command == 'r')
		{
		written =0;
		L1reads++;


		readwrite(full_address,L1_tag,L2_tag,L1_index,L2_index,false);
		
		}
	}

#ifdef PREFETCH_ENABLE
	GS_found=false;
	PWS_found = false;
	IP_found = false;
	
//	cout << "Info"<< PC << " " << full_address << " " << warp_id << " " << thread_id << endl;
/*	unsigned long PWS_stride = PWS_access(PC,full_address,warp_id,thread_id);
	if(PWS_stride > 0)
	{
		cout <<"PWS Stride > 0 \n";
		for(int i =0; i< prefetch_width; i++)
		{
			stride_addr = full_address + PWS_stride*i;

			indexcalc(stride_addr,L1_numsets, &L1_index,  &L1_inttag);
			if(L2_presence == 1)
			indexcalc(stride_addr,L2_numsets, &L2_index,  &L2_inttag);

			sprintf(L1_tag, "%lx", L1_inttag); //	Tag conversion to char
			sprintf(L2_tag, "%lx", L2_inttag);

			sprintf(address, "%lx", stride_addr); // Address conversion to char
			readwrite(stride_addr,L1_tag,L2_tag,L1_index,L2_index,true);
		}

	}
*/

	GS_stride = GS_access(PC, full_address,warp_id%32,thread_id);
	if(GS_found && GS_stride !=0)
	{
		//cout << "GS_found \n";

		for(int p=0; p<prefetch_width; p++)
		{
			stride_addr = full_address + GS_stride*p;

			indexcalc(stride_addr,L1_numsets, &L1_index,  &L1_inttag);
			if(L2_presence == 1)
			indexcalc(stride_addr,L2_numsets, &L2_index,  &L2_inttag);

			sprintf(L1_tag, "%lx", L1_inttag); //	Tag conversion to char
			sprintf(L2_tag, "%lx", L2_inttag);

			sprintf(address, "%lx", stride_addr); // Address conversion to char
			readwrite(stride_addr,L1_tag,L2_tag,L1_index,L2_index,true);
		}
	}
	else
	{
		
	//	cout << "GS not found \n";
		IP_stride = IP_access(PC, full_address,warp_id,thread_id);
		if(IP_found && IP_stride !=0)
		{
	//		cout << "IP_found \n";
			for(int p=0;p<prefetch_width;p++)
			{
			stride_addr = full_address + IP_stride*p;

			indexcalc(stride_addr,L1_numsets, &L1_index,  &L1_inttag);
			if(L2_presence == 1)
			indexcalc(stride_addr,L2_numsets, &L2_index,  &L2_inttag);

			sprintf(L1_tag, "%lx", L1_inttag); //	Tag conversion to char
			sprintf(L2_tag, "%lx", L2_inttag);

			sprintf(address, "%lx", stride_addr); // Address conversion to char
			readwrite(stride_addr,L1_tag,L2_tag,L1_index,L2_index,true);
			}
		}
		else
		{
			if(PWS_found && GS_stride > 0)
			{

	//			cout << "PWS_found \n";
			for(int p=0;p<prefetch_width;p++)
			{
			stride_addr = full_address + GS_stride*p;

			indexcalc(stride_addr,L1_numsets, &L1_index,  &L1_inttag);
			if(L2_presence == 1)
			indexcalc(stride_addr,L2_numsets, &L2_index,  &L2_inttag);

			sprintf(L1_tag, "%lx", L1_inttag); //	Tag conversion to char
			sprintf(L2_tag, "%lx", L2_inttag);

			sprintf(address, "%lx", stride_addr); // Address conversion to char
			readwrite(stride_addr,L1_tag,L2_tag,L1_index,L2_index,true);
			}
			}
		}
	}

#endif
	
	}

	int lru = L1_ASSOC;
	char printtag[20];
	char dirty = ' ' ;



	if(L1_SIZE > 0)
	{
	L1missrate = (L1readMiss+L1writeMiss)/(L1reads+L1writes);
	}
	if(L2_SIZE > 0)
	{
	L2missrate = (L2readMiss)/(L2reads);
	}
	if(L2_SIZE > 0)
	memtraffic = L2writeback+L2writeMiss+L2readmiss_prefetch+L2readMiss+L2prefetches;
	else
	memtraffic = L1writeback+L1writeMiss+L1readMiss+L1prefetch;

	/*Simulation results Print out*/
	cout<<"===== Simulation results (raw) =====\n";
	cout<<"c. number of L1 writes:         "<<L1writes<<"\n";
	cout<<"d. number of L1 write misses:   "<<L1writeMiss<<"\n";
	
	cout<< "h. number of L2 reads		"<<L2reads<<"\n";
	cout<<"i. number of L2 read misses 	"<<L2readMiss<<"\n";
	cout<<"l. number of L2 writes:	       "<<L2writes<<"\n";
	cout<<"m. number of L2 write misses:   "<<L2writeMiss<<"\n";
	

	return 0;	
}
	
/* ------------------------------------------MAIN ENDS HERE------------------------------------------------------*/
/*Index and Tag calculation function*/


int indexcalc(unsigned long address,int numSets, unsigned long *index, unsigned long *inttag)
{
	unsigned long addr;
	int block_offset = log2(BLOCKSIZE);
	int indexoffset = log2(numSets);
	unsigned long index_mask =0;
	for(int i=0;i<indexoffset;i++)
  	{
		index_mask <<= 1;
        	index_mask |= 1;
  	}
	
	addr = address >> block_offset;
	*inttag = addr >>indexoffset;	/*Tag calculation for L1 and L2*/
	*index = addr & index_mask;

	return 1;
	
}	


/*Index and Tag calculation function*/



/*---------------------------------L1 READ AND WRITE------------------------------------------------------------*/

int readwrite( unsigned long full_address, char* L1_tag, char* L2_tag,unsigned long L1_index,unsigned long L2_index, bool prefetch)
{
	//int miss =0;
	char index[20];
	int streamId = 0;
	SBhit =0;
	char address[20];
	sprintf(index, "%lx", L1_index);
	sprintf(address, "%lx", full_address);
	unsigned long blockaddress;
	blockaddress = full_address - (full_address%BLOCKSIZE);
#ifdef debug
	if(written == 1)
	cout<<"L1 write : " << address << "(tag " << L1_tag << "," << "index " << L1_index <<")\n" ;
	else
	cout<<"L1 read : " << address << "(tag " << L1_tag << "," << "index " << L1_index <<")\n" ;
#endif	
	/* Search in L1 cache and return 1 in case of a hit */
	int blockid = L1.cacheSearch(L1_tag,L1_index);
	
	
	if(blockid == -1)
	{

		/*Increment write and read miss counter*/		
		if(!prefetch)
		{
		if(written == 1)
		L1writeMiss++;
		else
		L1readMiss++;
		}
		/*Check First entries of all Stream Buffers*/
		for(int i=0; i<L1_PREF_N;i++)
		{
		if(L1_PREF_N > 0)
		{
		streamId = SB_L1[i].streamBufferSearch(blockaddress);
		if(streamId == 1)
		{
			streamId = i; 
			L1SBUpdateLRU(i);
			SBhit = 1;
			/*Shift contents of stream buffer by one position*/
			L1shiftUp(streamId);
			if(written == 1)
			L1writeMiss--;
			else
			L1readMiss--;	
			break;
		}
		}
		}

	
		
		L1readmiss(L1_tag, L1_index, L2_index, L2_tag,full_address,prefetch);

		if(SBhit != 1 )
		{
			if(L1_PREF_N > 0)
			{
				
			int lrubuffer =L1SBgetLRU();
			L1preFetch(lrubuffer, L2_tag, L2_index,full_address, blockaddress+BLOCKSIZE);
			
			}	
		}
			
		
	}

	else
	{
#ifdef debug	
		cout << "L1 hit \n";		/*No Stream Buffer Functionality is required for this case*/
		cout << "L1 Update LRU \n";
		
#endif
		if(written == 1)
		{
		L1.set[L1_index][blockid].setDirty();
		}

	
	}
		
	return 0;
}



int L1readmiss( char* L1_tag,unsigned long L1_index, unsigned long L2_index, char* L2_tag,  unsigned long address,bool prefetch)
{	
	
	unsigned long victimAddress;
	char victimTag[20];
	char victimaddress[20];
	int success = 0;	
	int blockid;
	int emptyBlock =0;
	victimDirty =0;
	
	emptyBlock = L1.checkEmpty(L1_index); /* Calls the function checkEmpty of cache */
	
	/* After checking L2 */
	if(emptyBlock == -1) 
	{
		blockid = L1.findVictim(L1_index);	

		if(L1.set[L1_index][blockid].getDirty())
		victimDirty = 1;
		
	
		victimAddress = L1.set[L1_index][blockid].getAddress();
		strcpy(victimTag,L1.set[L1_index][blockid].getTag());
		
		int bufferblock =0;
		
		for(int bufferId = 0; bufferId < L1_PREF_N; bufferId++)
		{
			int bufferblock = SB_L1[bufferId].fullstreamBufferSearch(victimAddress);
			if(bufferblock != -1)
			{
				if(victimDirty == 1)
				SB_L1[bufferId].SB[bufferblock].setInvalid();
				break;
			}
			
		}

			
		sprintf(victimaddress, "%lx", victimAddress);
#ifdef debug
		if(victimDirty == 0)							
		cout<<"L1 victim : " << victimaddress << " (tag "<< victimTag<< " , index "<< L1_index << ", clean) \n";
		else	
		cout<<"L1 victim : " << victimaddress << " (tag "<< victimTag<< " , index "<< L1_index << ", dirty) \n";/* Finds the Least 			recently used block in the set*/
#endif		
	
		if(victimDirty == 1)
		evictL1(L1_index, blockid, L2_index,L2_tag, address,prefetch); /* Evicts the least recently used block in the set and also does a write back if the block is dirty*/
		
		victimDirty =0;
		L1.setVal(L1_tag,address,L1_index,blockid);

		if(written ==1)
		{
			L1.set[L1_index][blockid].setDirty();
		}
		else
			L1.set[L1_index][blockid].setClean();




		if(L2_presence == 1 && SBhit != 1)
		{
#ifdef debug
		cout<<"L2 read : " << address << "(tag " << L2_tag << "," << "index " << L2_index <<")\n" ;
#endif
		success = L2.cacheSearch(L2_tag, L2_index);
			L2reads++;

			if(success == -1)
			{
				if(!prefetch)
				L2readMiss++;
			L2readmiss(L2_index,L2_tag,address,prefetch);
			}
			else
			{
#ifdef debuf
			cout << "L2 HIT \n";
#endif			
			}
		}
	}

	else
	{
		if(L2_presence == 1 && SBhit != 1)
		{
#ifdef debug	
		cout<<"read : " << address << "(tag " << L2_tag << "," << "index " << L2_index <<")\n" ;
#endif
		success = L2.cacheSearch(L2_tag, L2_index);
		L2reads++;
			if(success == -1)
			{
				if(!prefetch)
				L2readMiss++;
			L2readmiss(L2_index,L2_tag,address,prefetch);
			
			}
			else
			{
#ifdef debug
			cout << "L2 HIT \n";
#endif		
			}
		}
	
		L1.setVal(L1_tag,address,L1_index,emptyBlock); /* Calls the function setVal of Cache*/
	
		if(written == 1)
		{
		L1.set[L1_index][emptyBlock].setDirty();
		}
	}

	blockid = L1.cacheSearch(L1_tag,L1_index);
	
	return 0;		
}

int evictL1(unsigned long L1_index,int L1_blockid,unsigned long L2_index, char* L2_tag,  unsigned long address ,bool prefetch)
{
	char victimaddress[20];
	int L2_blockid;
	unsigned long victimAddress;
	unsigned long L2_victimtag;
	char victimTag[20];
	unsigned long victim_index;
	unsigned long index_mask2 =0;
	L1writeback++;
	if(L2_presence == 1)
	{
		
		L2writes++;
		victimAddress = L1.set[L1_index][L1_blockid].getAddress();
		int L2_numsets = L2.getNumSets();

		indexcalc(victimAddress,L2_numsets, &victim_index, &L2_victimtag);
		
		sprintf(victimaddress, "%lx", victimAddress);
		sprintf(victimTag, "%lx", L2_victimtag);	
		
		L2_blockid = L2.cacheSearch(victimTag,victim_index);
#ifdef debug		
		cout<<"L2 read : " << victimaddress << "(tag " << L2_victimtag << "," << "index " << victim_index <<")\n" ;
#endif		
		
		if(L2_blockid != -1)
		{
			if( victimDirty == 1)
			L2.set[victim_index][L2_blockid].setDirty();
	
		}		
		else
		{
		L2readmiss(victim_index,victimTag,victimAddress,prefetch);
		
		if(!prefetch)
		L2writeMiss++;
		
		
		}
		L2.set[victim_index][L2_blockid].setDirty();
	}

	
	
		
	return 0;
}	


int L2readmiss(unsigned long L2_index,  char* tag, unsigned long address, bool prefetch)
{	
	
	int blockid;
	int L2_blockid;
	int L2victimDirty =0;
	unsigned long L2_victimAddress;
	unsigned long blockaddress;
	char L2_victimTag[20],L2_victimaddress[20];
	int emptyBlock = L2.checkEmpty(L2_index); /* Calls the function checkEmpty of cache */
	blockaddress = address - (address % BLOCKSIZE);

	int SB2hit =0;
	int streamId =0;
	/* After checking L2 */
	if(emptyBlock == -1) 
	{
	blockid = L2.findVictim(L2_index); /* Finds the Least recently used block in the set*/

	if(L2.set[L2_index][blockid].getDirty())
	L2victimDirty = 1;
		
	
	L2_victimAddress = L2.set[L2_index][blockid].getAddress();
	strcpy(L2_victimTag,L2.set[L2_index][blockid].getTag());
	sprintf(L2_victimaddress, "%lx", L2_victimAddress);

	/*Invalidating Entries in the Stream Buffer*/
		int bufferblock =0;
		
		for(int bufferId = 0; bufferId < L2_PREF_N; bufferId++)
		{
			int bufferblock = SB_L2[bufferId].fullstreamBufferSearch(L2_victimAddress);
			if(bufferblock != -1)
			{
				if(L2victimDirty == 1)
				SB_L2[bufferId].SB[bufferblock].setInvalid();
				break;
			}
			
		}


	
	if(L2victimDirty != 0)
	L2writeback++;							\
	
	L2.setVal(tag,address,L2_index,blockid);
	if( victimDirty == 1)
	L2.set[L2_index][blockid].setDirty();
	else
	L2.set[L2_index][blockid].setClean();

	}
	else
	{
		L2.setVal(tag,address,L2_index,emptyBlock); /* Calls the function setVal of Cache*/
		if(victimDirty == 0)
		L2.set[L2_index][emptyBlock].setClean();
	}
	SB2hit =0;
	/*Check First entries of all Stream Buffers*/
		if(L2_PREF_N > 0)
		{
		for(int i=0; i<L2_PREF_N;i++)
		{
		
		streamId = SB_L2[i].streamBufferSearch(blockaddress);
		if(streamId == 1)
		{
						
			streamId = i; 
			L2SBUpdateLRU(i);
			SB2hit = 1;
			/*Shift contents of stream buffer by one position*/
			L2shiftUp(streamId);
			if(victimDirty == 1)
			L2writeMiss--;
			else
			{
			if(prefetch == 0)
			L2readMiss--;
			else				
			L2readmiss_prefetch--;
			}	
			
			break;
		}
		}
		}
		

		if(SB2hit != 1 )
		{
			if(L2_PREF_N > 0)
			{

			int lrubuffer =L2SBgetLRU();
			L2preFetch(lrubuffer,tag, L2_index,blockaddress+BLOCKSIZE);
			
			}	
		}

	L2_blockid = L2.cacheSearch(tag,L2_index);

	return 0;					
}

/*Prefetching for SB_L1*/
int L1preFetch(int bufferId, char* tag,unsigned long L2_index,unsigned long full_address, unsigned long intAddress)
{
	
	
	char addr[20];
	char L2_tag[20];
	unsigned long index =0;
	unsigned long inttag =0;
	int L2_numSets = L2.getNumSets(); 	
	
	memset (L2_tag, 0, 20);
	L1SBUpdateLRU(bufferId);
	indexcalc(intAddress,L2_numSets,&index,&inttag);
	sprintf(L2_tag, "%lx", inttag);
	
	int blockid = L2.cacheSearch(L2_tag,index);
	L2reads_prefetch++;
	L1prefetch++;
	
	full_address += BLOCKSIZE;
	SB_L1[bufferId].setVal(L2_tag,intAddress,index,0);
	
	if(blockid == -1)
	{
	L2readmiss_prefetch++;

	prefetch =1;
	L2readmiss(index,L2_tag,full_address,prefetch);
	prefetch =0;

	}
	

	
	for(int i=1; i<L1_PREF_M; i++)
	{
	index =0;
	inttag =0;
	 memset ( L2_tag, 0, 20);
	intAddress += BLOCKSIZE;
	full_address += BLOCKSIZE;
	indexcalc(intAddress,L2_numSets,&index, &inttag);
	sprintf(L2_tag, "%lx", inttag);
	L2reads_prefetch++;
	L1prefetch++;
	
	SB_L1[bufferId].setVal(L2_tag,intAddress,index,i);
	blockid = L2.cacheSearch(L2_tag,index);

	
	if(blockid == -1)
	{

	L2readmiss_prefetch++;

	prefetch =1;
	L2readmiss(index,L2_tag,full_address,prefetch);
	prefetch =0;
	
	}
	


	}
	return 0;
}
/*L2 Prefetch Functionality*/

int L2preFetch(int bufferId, char* tag,unsigned long L2_index, unsigned long intAddress)
{
	L2prefetches++;	
	char addr[20];
	char L2_tag[20];
	unsigned long index =0;
	unsigned long inttag =0;
	int L2_numSets = L2.getNumSets(); 	
	
	memset (L2_tag, 0, 20);
	L2SBUpdateLRU(bufferId);
	indexcalc(intAddress,L2_numSets,&index,&inttag);
	sprintf(L2_tag, "%lx", inttag);
	
	
	SB_L2[bufferId].setVal(L2_tag,intAddress,index,0);

	
	
	for(int i=1; i<L2_PREF_M; i++)
	{

	index =0;
	inttag =0;
	
	L2prefetches++;
	intAddress += BLOCKSIZE;
	sprintf(addr, "%lx", intAddress);
	indexcalc(intAddress,L2_numSets,&index, &inttag);
	sprintf(L2_tag, "%lx", inttag);
	SB_L2[bufferId].setVal(L2_tag,intAddress,index,i);
	
	}
	
	return 0;
}
/*L2 Shift up functionality*/

int L2shiftUp(int bufferId)
{
	
	L2prefetches++;
	int i;
	for(i = 0; i<(L2_PREF_M-1); i++)
	{
		SB_L2[bufferId].SB[i].setTag(SB_L2[bufferId].SB[i+1].getTag());
		SB_L2[bufferId].SB[i].setAddress(SB_L2[bufferId].SB[i+1].getAddress());
		if(SB_L2[bufferId].SB[i+1].getValid())
		SB_L2[bufferId].SB[i].setValid();
		else
		SB_L2[bufferId].SB[i].setInvalid();
	}
	
	
	unsigned long index=0;
	unsigned long inttag=0;
	char ptag[20];
	int L2_numSets = L2.getNumSets();
	unsigned long address = SB_L2[bufferId].SB[i].getAddress() + BLOCKSIZE;
	indexcalc(address,L2_numSets,&index,&inttag);
	sprintf(ptag, "%lx", inttag);
	
	SB_L2[bufferId].setVal(ptag,address,index,i);
	
	SB_L2[bufferId].SB[i].setValid();

	return 1;

}


/*Shift Up function for L1*/
int L1shiftUp(int bufferId)
{
	int i;
	for(i = 0; i<(L1_PREF_M - 1); i++)
	{
		SB_L1[bufferId].SB[i].setTag(SB_L1[bufferId].SB[i+1].getTag());
		SB_L1[bufferId].SB[i].setAddress(SB_L1[bufferId].SB[i+1].getAddress());
		if(SB_L1[bufferId].SB[i+1].getValid())
		SB_L1[bufferId].SB[i].setValid();
		else
		SB_L1[bufferId].SB[i].setInvalid();
	}
	
	
	unsigned long index=0;
	unsigned long inttag=0;
	char ptag[20];
	int L2_numSets = L2.getNumSets();
	unsigned long address = SB_L1[bufferId].SB[i].getAddress() + BLOCKSIZE;
	indexcalc(address,L2_numSets,&index,&inttag);
	sprintf(ptag, "%lx", inttag);
	int blockid = L2.cacheSearch(ptag,index);
	L1prefetch++;
	L2reads_prefetch++;
	SB_L1[bufferId].setVal(ptag,address,index,i);
	
	SB_L1[bufferId].SB[i].setValid();
	
	if(blockid == -1)
	{
		
	L2readmiss_prefetch++;
	prefetch =1;
	L2readmiss(index,ptag,address,prefetch);
	prefetch =0;
	}
	
	
	return 1;
	

}

/*Updating Stream Buffer LRU*/

int L1SBUpdateLRU(int sbnumber)
{
	int currLRU = SB_L1[sbnumber].getLRU();
	

	for(int i= 0 ; i<L1_PREF_N ; i++)
	{
		if(SB_L1[i].getLRU() < currLRU)
		SB_L1[i].incrementLRU();
	}
	SB_L1[sbnumber].setLRU(1);
	return 0;
}

int L2SBUpdateLRU(int sbnumber)
{
	int currLRU = SB_L2[sbnumber].getLRU();
	

	for(int i= 0 ; i<L2_PREF_N ; i++)
	{
		if(SB_L2[i].getLRU() < currLRU)
		SB_L2[i].incrementLRU();
	}
	SB_L2[sbnumber].setLRU(1);

	return 0;
}


/*Getting the Least Recently Used Buffer from all the stream buffers*/

int L1SBgetLRU()
{
	int victim =0;
	for(int i=0; i< L1_PREF_N; i++)
	{

		if(SB_L1[i].getLRU() > SB_L1[victim].getLRU() || SB_L1[i].getLRU() == L1_PREF_N )
		victim =i;
	}
	
	return victim;

}


int L2SBgetLRU()
{
	int victim =0;

	for(int i=0; i< L2_PREF_N; i++)
	{

		if(SB_L2[i].getLRU() > SB_L2[victim].getLRU()|| SB_L2[i].getLRU() == L2_PREF_N )
		victim =i;
	}
	
	return victim;

}

 /*End of Program*/

