#include <iostream>
#include <stdio.h>
#include <vector>

using namespace std;

typedef unsigned long long new_addr_type;


new_addr_type PML4_MASK = 0xFF8000000000;
new_addr_type PDP_MASK  = 0x7FC0000000;
new_addr_type PD_MASK   = 0x3FE00000;
new_addr_type PT_MASK   = 0x1FF000;


const int TLB_SIZE = 128;



int main(){

	FILE* f_vtl_trace;
    f_vtl_trace = fopen("backprop.virtual_address.txt","r");

	new_addr_type addr;
	unsigned sid,wid;
    unsigned timestamp;

	vector<new_addr_type> TLB_array[16];
	int TLB_size[16];	
	int TLB_access[16];
	int TLB_hit[16];
	int TLB_miss[16];

	//initialize tlb
	for(int index=0; index<16; index++){
		TLB_size[index] = 0;
		TLB_access[index] = 0;
		TLB_hit[index] = 0;
		TLB_miss[index] = 0;
		TLB_array[index].clear();
	}


	if(f_vtl_trace != NULL){
		while( fscanf(f_vtl_trace,"%llx %d %d %d",&addr , &sid , &wid , &timestamp ) == 4){
			//printf( "%016llx %d %d %d\n",addr , sid , wid , timestamp);

			//search TLB
			int tlb_hit=0;
			for(int index=0; index < TLB_size[sid]; index++){
				if(TLB_array[sid][index] == addr){
					tlb_hit = 1;
					//implement LRU
					if(index != 0){
						new_addr_type tmp = TLB_array[sid][0];
						TLB_array[sid][0] = addr;
						TLB_array[sid][index] = tmp;
					}
					break;
				}
			}


			// TLB HIT
			if(tlb_hit == 1){
				TLB_hit[sid]++;
			}
			// TLB MISS
			else{
				TLB_miss[sid]++;
				vector<new_addr_type>::iterator it;
				it = TLB_array[sid].begin();

	            if(TLB_size[sid] < TLB_SIZE){
					TLB_array[sid].insert(it, addr);
					TLB_size[sid]++;
			    } 
				else{
                    TLB_array[sid].insert(it, addr);
					TLB_array[sid].pop_back();
				}
			}
			TLB_access[sid]++;	
		}
		for(int i=0; i<16;i++){
			printf("SM:%d\n",i);
			printf("TLB_access: %d\n",TLB_access[i]);
            printf("TLB_miss: %d\n",TLB_miss[i]);
            printf("TLB miss rate: %f\n\n",((float)(TLB_miss[i])/(float)TLB_access[i])); 
		}

	}
	return 0;
}
