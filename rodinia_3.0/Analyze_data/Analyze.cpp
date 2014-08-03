#include <iostream>
#include <stdio.h>
#include <vector>
#include <map>
#include <set>

using namespace std;


//Definition
typedef unsigned long long new_addr_type;

new_addr_type PML4_MASK = 0xFF8000000000;
new_addr_type PDT_MASK  = 0x7FC0000000;
new_addr_type PD_MASK   = 0x3FE00000;
new_addr_type PT_MASK   = 0x1FF000;

new_addr_type PDT_WHOLE_MASK  = 0xFFFFC0000000;
new_addr_type PD_WHOLE_MASK   = 0xFFFFFFE00000;
new_addr_type PT_WHOLE_MASK   = 0xFFFFFFFFF000;

class thread_info {
public:
   thread_info(){}
   ~thread_info(){}
   bool operator<(const thread_info& b)const{
      if(this->b_x < b.b_x){
		return true;
      }
	  else if(this->b_x > b.b_x){
        return false;
	  }
	  else{
        if(this->b_y < b.b_y){
          return true;
        }
		else if(this->b_y > b.b_y){
		  return false;
		}
        else{
	      if(this->b_z < b.b_z){
			return true;
		  }
		  else{
			return false;
		  }
		}
      }
   }
   int b_x, b_y, b_z;
   int t_x, t_y, t_z;
};

struct shader_addr_mapping{
    std::set< new_addr_type > pml4_addr;
    std::set< new_addr_type > pdt_addr;
    std::set< new_addr_type > pd_addr;
    std::set< new_addr_type > pt_addr;
};


//configuration
unsigned int cluster_size = 15;

//Main code
int main(){

	FILE* f_vtl_trace;
    f_vtl_trace = fopen("funcsim_vtl_dump.txt","r");

	new_addr_type addr;
	char rw;
	thread_info t_info;

	// Total page
	std::map< new_addr_type, unsigned int > g_pml4_addr;
	std::map< new_addr_type, unsigned int > g_pdt_addr;
	std::map< new_addr_type, unsigned int > g_pd_addr;
	std::map< new_addr_type, unsigned int > g_pt_addr;

	
    std::vector< shader_addr_mapping > g_shader_addr_map;
	std::map< thread_info, int > g_thread_info;
	int block_idx = 0;

	if(f_vtl_trace != NULL){
		while( fscanf(f_vtl_trace,"%llx %d %d %d %d %d %d %c ",&addr,&t_info.b_x, &t_info.b_y, &t_info.b_z, &t_info.t_x, &t_info.t_y, &t_info.t_z, &rw ) == 8){
			//add into global data

			if(g_pml4_addr.find( addr & PML4_MASK ) == g_pml4_addr.end()){
				g_pml4_addr[ addr & PML4_MASK ] = 0;
			}
			if(g_pdt_addr.find( addr & PDT_WHOLE_MASK ) == g_pdt_addr.end()){
				g_pdt_addr[ addr & PDT_WHOLE_MASK ] = 0;
			}
			if(g_pd_addr.find( addr & PD_WHOLE_MASK ) == g_pd_addr.end()){
				g_pd_addr[ addr & PD_WHOLE_MASK ] = 0;
			}
			if(g_pt_addr.find( addr & PT_WHOLE_MASK ) == g_pt_addr.end()){
				g_pt_addr[ addr & PT_WHOLE_MASK ] = 0;
			}

			g_pml4_addr[ addr & PML4_MASK ]++;
			g_pdt_addr[ addr & PDT_WHOLE_MASK ]++;
			g_pd_addr[ addr & PD_WHOLE_MASK ]++;
			g_pt_addr[ addr & PT_WHOLE_MASK ]++;

			// check weather it is same block
			if(g_thread_info.find(t_info) == g_thread_info.end()  ){
				g_thread_info[ t_info ] = block_idx++;

				shader_addr_mapping tmp_map;
				g_shader_addr_map.push_back(tmp_map);
			}
			int index_map = g_thread_info[ t_info ];

			g_shader_addr_map[index_map].pml4_addr.insert( addr & PML4_MASK );
			g_shader_addr_map[index_map].pdt_addr.insert ( addr & PDT_MASK );
			g_shader_addr_map[index_map].pd_addr.insert  ( addr & PD_MASK );
			g_shader_addr_map[index_map].pt_addr.insert  ( addr & PT_MASK );
		}

		//print
		printf("pml4 page size: %d\n",g_pml4_addr.size());
		printf("pdt  page size: %d\n",g_pdt_addr.size());
		printf("pd   page size: %d\n",g_pd_addr.size());
		printf("pt   page size: %d\n",g_pt_addr.size());
		printf("\n");
		printf("block_idx: %d\n",block_idx);



		// check how much page in 1st block exist it 2nd block
		std::map<thread_info, int>::iterator it= g_thread_info.begin();
		int index_1stblock = it->second;

        std::map<thread_info, int>::iterator it_next= ++ g_thread_info.begin() ;
        int index_2ndblock = it_next->second;

		int hit=0,miss=0;

		std::set< new_addr_type >::iterator it_set;
        for(it_set = g_shader_addr_map[index_1stblock].pt_addr.begin(); it_set != g_shader_addr_map[index_1stblock].pt_addr.end(); ++it_set ){
			//find if it exist in 2nd block
			if( g_shader_addr_map[index_2ndblock].pt_addr.find(*it_set) != g_shader_addr_map[index_2ndblock].pt_addr.end() ){
				hit++;
			}
			else{
				miss++;
			}
		}
		printf("hit: %d, miss: %d, share rate: %lf\n",hit,miss,(float)hit/(float)(miss+hit));
	}
	return 0;
}
