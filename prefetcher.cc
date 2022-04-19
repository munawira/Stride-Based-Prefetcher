
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


strideTrainer::strideTrainer()
{
	//SC = new strideCounter[3];
	for(int i=0; i<3 ;i++)
	{
		full_address[i] = 0;
		stride[i] = 0;
		tid[i] = -1;

	}
	
	PC =0;
	trained_stride = 0;
	current_index =0;
	trained = false;
	last_addr =0;
	last_tid =0;
	LRU = NUM_TABLE_ENTRIES - 1;


}



/*creates entry in the strideCounter table*/
void strideTrainer:: createStrideEntry(unsigned long stride)
{

	int minStrideCount =SC[0].strideCount;
	int minStrideIndex = 0;

	for(int i=0; i<3; i++)
	{
		if(SC[i].stride == stride)
		{
			SC[i].strideCount++;
			return;
		}
	}

	for(int i=0; i<3 ;i++)
	{
		if(SC[i].strideCount == 0)
		{
			SC[i].stride =stride;
			SC[i].strideCount = 1;
			return;

		}
	}

	//strideCounter is full
 	//Replace the stride with the minimum stridecount
	for(int i=0; i< 3 ; i++)
	{
		if(SC[i].strideCount < minStrideCount)
		{
				
			minStrideCount = SC[i].strideCount;
			minStrideIndex = i;
		}

	}

	SC[minStrideIndex].stride = stride;
	SC[minStrideIndex].strideCount =1;

	return;



}


strideCounter::strideCounter()
{
	stride =0;
	strideCount = 0;
	taken = false;

}


globalStride::globalStride()
{
	stride = 0;
	trained = false;

}


/* Function calls for accessing GS, PWS and IP */


unsigned long GS_access(unsigned long pc, unsigned long mem_addr, int warp_id, int tid)
{
	unsigned long stride;
	int i;
	for( i=0; i< NUM_TABLE_ENTRIES; i++)
	{
		if(GS[i].PC == pc)
			break;

	}
	if(i < NUM_TABLE_ENTRIES)
	{
		GS_promote(pc);
		GS_found = true;

		if(GS[i].stride >0)
		return GS[i].stride;

	}
	else
	{
		//cout << "Before PWS_access \n";
		stride = PWS_access(pc,mem_addr,warp_id,tid);

		//cout << "After PWS_access \n";
		return stride;
	}

	return 0;

}
//GS_promote called whenever a new trained entry in PWS is created and whenever a GS_access is made
unsigned long GS_promote(unsigned long pc)
{
	strideCounter *SC = new strideCounter[NUM_WARPS];
	for(int w=0; w<NUM_WARPS; w++)
	{
		SC[w].stride =0;
		SC[w].strideCount =0;
	}

	int maxStrideCount =0;
	unsigned long maxStride;

	//Iterates through PWSs to look for repeating strides
	for(int i=0; i<NUM_WARPS;i++)//Across all PWS
	{
		for(int j=0;j<NUM_TABLE_ENTRIES;j++)//All entries of this PWS
		{
			if(PWS[i][j].PC == pc)
			{
				int k=0;
				while(k < NUM_WARPS && SC[k].stride !=0)//All entries of stridetracker
				{
					if(SC[k].stride == PWS[i][j].trained_stride)
					{
						SC[k].strideCount++;
						if(SC[k].strideCount > maxStrideCount)
						{
							maxStrideCount = SC[k].strideCount;
							maxStride = SC[k].stride;
							k = NUM_WARPS;
							break;
						}

					}
					k++;
				}
				if(k<NUM_WARPS)
				{
					SC[k].stride = PWS[i][j].trained_stride;
						SC[k].strideCount++;
				}
				break;
			}
		}
	}
	if(maxStrideCount > 2)
	{
		int i=0;
		while(i<NUM_TABLE_ENTRIES && GS[i].stride !=0)
		{
			if(GS[i].PC == pc)
			{
				GS[i].stride = maxStride;
				i=NUM_TABLE_ENTRIES;
				break;
			}
			i++;

		}

		if(i<NUM_TABLE_ENTRIES)
		{
			GS[i].PC = pc;
			GS[i].stride = maxStride;

		}
	}


}

unsigned long PWS_access(unsigned long pc, unsigned long mem_addr, int warp_id, int tid)
{
	int maxStrideCount =0;
	int maxStrideIndex =0;
	int i;
	for(i=0; i<NUM_TABLE_ENTRIES;i++)
	{
		if(PWS[warp_id][i].PC == pc)
			break;
	}
	
	if(i < NUM_TABLE_ENTRIES)  // IF PC found in the table
	{	
	//cout << "Here 2\n";
		if(PWS[warp_id][i].last_tid != tid)
		{
			//cout << "Stride :" << (mem_addr - PWS[warp_id][i].last_addr)/(tid - PWS[warp_id][i].last_tid) << endl;
			PWS[warp_id][i].createStrideEntry((mem_addr - PWS[warp_id][i].last_addr)/(tid - PWS[warp_id][i].last_tid));
			GS_promote(pc);
		
		for(int j=0;j<3;j++)
		{
			if(PWS[warp_id][i].SC[j].strideCount > maxStrideCount)
			{
				maxStrideCount = PWS[warp_id][i].SC[j].strideCount;
				maxStrideIndex= j;

			}

		}

			PWS[warp_id][i].trained_stride = PWS[warp_id][i].SC[maxStrideIndex].stride;
			if(maxStrideCount > 2)
			{
				PWS[warp_id][i].trained = true;
			}
			else
				PWS[warp_id][i].trained = false;

			

			/************************************************/

		}
		
		PWS[warp_id][i].last_tid = tid;
		PWS[warp_id][i].last_addr = mem_addr;
		PWS_found = true;
		return PWS[warp_id][i].trained_stride;
	}
	else //If PC not found in the table
	{
//	cout << "Here 5 \n";
		int k=0;
		while(PWS[warp_id][k].PC > 0)
			k++;
		if (k< NUM_TABLE_ENTRIES)
		{
			PWS[warp_id][k].PC =pc;
			PWS[warp_id][k].full_address[0] = mem_addr;
			PWS[warp_id][k].current_index++;
			PWS[warp_id][k].last_addr = mem_addr;
			PWS[warp_id][k].last_tid - tid;

		}
		else
		{
			int entry = PWS_LRU[warp_id];
			PWS[warp_id][entry].PC =pc;
			PWS[warp_id][entry].full_address[0] = mem_addr;
			PWS[warp_id][entry].current_index++;
			PWS_LRU_update(entry,warp_id);
			PWS[warp_id][k].last_addr = mem_addr;
			PWS[warp_id][k].last_tid = tid;

		}

		return 0;
	}
}

/*IP Access*/
unsigned long IP_access(unsigned long pc, unsigned long mem_addr, int warp_id, int tid)
{
	int maxStrideCount =0;
	int maxStrideIndex =0;
	int i;
	for( i=0; i<NUM_TABLE_ENTRIES;i++)
	{
		if(IP[i].PC == pc)
			break;
	}

	//If PC found
	if(i < NUM_TABLE_ENTRIES)
	{
		if(IP[i].last_tid != tid)
		{
			IP[i].createStrideEntry((mem_addr - IP[i].last_addr)/(tid - IP[i].last_tid));

		for(int j=0;j<3;j++)
		{
			if(IP[i].SC[j].strideCount > maxStrideCount)
			{
				maxStrideCount = IP[i].SC[j].strideCount;
				maxStrideIndex= j;

			}

		}

			IP[i].trained_stride = IP[i].SC[maxStrideIndex].stride;
			if(maxStrideCount > 2)
			{
				IP[i].trained = true;
			}
			else
				IP[i].trained = false;
			
		}

		IP[i].last_tid = tid;
		IP[i].last_addr = mem_addr;
		IP_found = true;
		return IP[i].trained_stride;
			
	}
	else //If PC is not found
	{

		int k=0;
		while(IP[k].PC > 0)
			k++;
		if (k< NUM_TABLE_ENTRIES)
		{
			IP[k].PC =pc;
			IP[k].full_address[0] = mem_addr;
			IP[k].current_index++;
			IP[k].last_addr = mem_addr;
			IP[k].last_tid = tid;

		}
		else
		{
			int entry = IP_LRU;
			IP[entry].PC =pc;
			IP[entry].full_address[0] = mem_addr;
			IP[entry].current_index++;
			IP_LRU_update(entry);
			IP[k].last_addr = mem_addr;
			IP[k].last_tid = tid;

		}

		return 0;
	}

	
}



/**/


void GS_LRU_update(int current_entry)
{
	int current_LRU = GS[current_entry].LRU;
	for(int i =0; i<NUM_TABLE_ENTRIES;i++)
	{
		if(GS[i].LRU < current_LRU)
			GS[i].LRU++;
		if(GS[i].LRU == NUM_TABLE_ENTRIES -1)
			GS_LRU = i;
	}

	GS[current_entry].LRU =0;

}


void PWS_LRU_update(int current_entry, int warp_id)
{
	int current_LRU = PWS[warp_id][current_entry].LRU;
	for(int i =0; i<NUM_TABLE_ENTRIES;i++)
	{
		if(PWS[warp_id][i].LRU < current_LRU)
			PWS[warp_id][i].LRU++;
		if(PWS[warp_id][i].LRU == NUM_TABLE_ENTRIES -1)
			PWS_LRU[warp_id] = i;
	}

	PWS[warp_id][current_entry].LRU =0;



}
void IP_LRU_update(int current_entry)
{
	int current_LRU = IP[current_entry].LRU;
	for(int i =0; i<NUM_TABLE_ENTRIES;i++)
	{
		if(IP[i].LRU < current_LRU)
			IP[i].LRU++;
		if(IP[i].LRU == NUM_TABLE_ENTRIES -1)
			IP_LRU = i;
	//TODO: update all LRUs at the beginning to NUM_TABLE_ENTRIES -1
	}

	IP[current_entry].LRU =0;

}


