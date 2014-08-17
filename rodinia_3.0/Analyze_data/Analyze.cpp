#include <iostream>
#include <stdio.h>
#include <vector>
#include <map>
#include <set>
#include <cassert>

#include "Analyze.h"

using namespace std;

//Definition


new_addr_type PML4_MASK = 0xFF8000000000;
new_addr_type PDT_MASK  = 0x7FC0000000;
new_addr_type PD_MASK   = 0x3FE00000;
new_addr_type PT_MASK   = 0x1FF000;

new_addr_type PDT_WHOLE_MASK  = 0xFFFFC0000000;
new_addr_type PD_WHOLE_MASK   = 0xFFFFFFE00000;
new_addr_type PT_WHOLE_MASK   = 0xFFFFFFFFF000;


//configuration
#define cluster_size 15
#define N_LOOP  20


unsigned int Distance(shader_addr_mapping* work_group_A,shader_addr_mapping* work_group_B);
unsigned int min(const unsigned int& ,const unsigned int&);
//Main code
int main(){

	FILE* f_vtl_trace;
    f_vtl_trace = fopen("funcsim_vtl_dump.txt","r");

	new_addr_type addr;
	char rw;
	thread_info t_info;


	new_addr_type addr_pml4;
	new_addr_type addr_pdt;
	new_addr_type addr_pd;
	new_addr_type addr_pt;

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

			addr_pml4 = addr & PML4_MASK;
			addr_pdt  = addr & PDT_WHOLE_MASK;
			addr_pd   = addr & PD_WHOLE_MASK;
			addr_pt   = addr & PT_WHOLE_MASK;

			g_pml4_addr[ addr_pml4 ]++;
			g_pdt_addr[ addr_pdt ]++;
			g_pd_addr[ addr_pd ]++;
			g_pt_addr[ addr_pt ]++;

			// check weather it is same block
			if(g_thread_info.find(t_info) == g_thread_info.end()  ){
				g_thread_info[ t_info ] = block_idx++;

                shader_addr_mapping tmp_map;
                tmp_map.t_info = t_info;
				g_shader_addr_map.push_back(tmp_map);
			}

			int index_map = g_thread_info[ t_info ];		
			shader_addr_mapping* shader_mapping = &g_shader_addr_map[index_map];

            shader_mapping->PML4_addr[ addr_pml4 ]++;
			shader_mapping->PDT_addr [ addr_pdt  ]++;
			shader_mapping->PD_addr  [ addr_pd   ]++;
			shader_mapping->PT_addr  [ addr_pt   ]++;
		}

		//print
		printf("pml4 page size: %d\n",g_pml4_addr.size());
		printf("pdt  page size: %d\n",g_pdt_addr.size());
		printf("pd   page size: %d\n",g_pd_addr.size());
		printf("pt   page size: %d\n",g_pt_addr.size());
		printf("\n");
		printf("block_idx: %d\n",block_idx);


        // write the correlation code below
		std::map< new_addr_type, unsigned int > g_pt_addr_index;
        int addr_count=0;
        std::map< new_addr_type, unsigned int >::iterator it_map;
        for(it_map = g_pt_addr.begin(); it_map != g_pt_addr.end(); it_map++,addr_count++){
			g_pt_addr_index[ it_map->first ] = addr_count;
        }		
		//generate the  vector of each shader core
		for(unsigned int index=0; index < g_shader_addr_map.size(); index++){
			g_shader_addr_map[index].access_cnt.resize( addr_count ,0);

            for(it_map = g_shader_addr_map[index].PT_addr.begin(); it_map != g_shader_addr_map[index].PT_addr.end(); it_map++){
				unsigned int bit_index = g_pt_addr_index[ it_map->first ];
				g_shader_addr_map[index].access_cnt[ bit_index ] = it_map->second;
            }
        }

        // setup kmeans initialization value
        shader_addr_mapping* u_center[cluster_size];
        for(int i=0; i<cluster_size; i++){
            u_center[i] = & g_shader_addr_map[ ((float)i+0.5) * (g_shader_addr_map.size()/cluster_size) ];
        }
        for(int i=0; i < g_shader_addr_map.size(); i++){
            g_shader_addr_map[i].cluster_id = -1;
        }
        std::vector<unsigned int> cluster_avg_vector[cluster_size];
        std::vector<unsigned int> cluster_access_num;

        // Calculate cluster center
        for(int loop = 0; loop < N_LOOP; loop++){
            for(int i=0; i < cluster_size; i++){
                cluster_avg_vector[i].resize( g_shader_addr_map[0].access_cnt.size(), 0);
            }
            cluster_access_num.resize(cluster_size, 0);

            // go through each group mapping
            std::vector< shader_addr_mapping >::iterator it_shader;
            for(it_shader = g_shader_addr_map.begin(); it_shader != g_shader_addr_map.end(); it_shader++ ){
                unsigned int max_distance = Distance( &(*it_shader), u_center[0] );
                int max_index = 0;

                for(int cluster_id=1; cluster_id < cluster_size; cluster_id++){
                    unsigned int dis_tmp = Distance( &(*it_shader), u_center[ cluster_id ] );
                    // hope to find highly overlap
                    if( max_distance <  dis_tmp ){
                        max_distance =  dis_tmp;
                        max_index = cluster_id;
                    }
                }
                it_shader->cluster_id = max_index;
                assert(it_shader->access_cnt.size() ==  cluster_avg_vector[max_index].size());
                for(int index_acc=0; index_acc < it_shader->access_cnt.size(); index_acc++){
                    cluster_avg_vector[max_index][index_acc] += it_shader->access_cnt[index_acc];
                }
                cluster_access_num[ max_index ]++;
            }

            //average the vector
            for(int idx_cluster=0; idx_cluster < cluster_size; idx_cluster++){
                if(cluster_access_num[idx_cluster] == 0){
                    continue;
                }
                for(int idx_acc=0; idx_acc < cluster_avg_vector[idx_cluster].size(); idx_acc++){
                    cluster_avg_vector[idx_cluster][idx_acc] /= cluster_access_num[idx_cluster] ;
                }
            }


            // find the new center of each cluster
            for(int idx_cluster=0; idx_cluster < cluster_size; idx_cluster++){
                int flag =0;
                int min_error=0;
                int min_index=-1;

                for(int idx_map = 0; idx_map < g_shader_addr_map.size(); idx_map++ ){
                    //check vector in the same cluster
                    if(g_shader_addr_map[idx_map].cluster_id != idx_cluster){
                        continue;
                    }
                    // find error between avg vector and this vector
                    int accumulated_error =0;
                    int error_tmp;

                    for(int idx_acc=0; idx_acc < g_shader_addr_map[idx_map].access_cnt.size(); idx_acc++){
                        error_tmp = g_shader_addr_map[idx_map].access_cnt[idx_acc] - cluster_avg_vector[idx_cluster][idx_acc];
                        if(error_tmp < 0){
                            error_tmp *= -1;
                        }
                        accumulated_error += error_tmp;
                    }

                    if(flag ==0){
                        min_error =  accumulated_error;
                        min_index =  idx_map;
                        flag = 1;
                    }
                    else{
                        if(accumulated_error < min_error){
                            min_error =  accumulated_error;
                            min_index =  idx_map;
                        }
                    }
                }
                if(min_index != -1){
                    u_center[idx_cluster] = &g_shader_addr_map[ min_index ];
                }
            }
        }


       printf("finish clustering\n");
       for(int idx_cluster=0; idx_cluster < cluster_size; idx_cluster++){
            thread_info* thread = &u_center[idx_cluster]->t_info;
            printf("cluster id: %2d  b.x: %5d b.y: %5d b.z: %5d t.x: %5d t.y: %5d t.z: %5d  cluster_access_num: %d\n",idx_cluster,
                    thread->b_x, thread->b_y, thread->b_z, thread->t_x, thread->t_y, thread->t_z, cluster_access_num[idx_cluster]);
       }


	}
	return 0;
}


unsigned int Distance(shader_addr_mapping* mapping_A,shader_addr_mapping* mapping_B){

    assert(mapping_A->access_cnt.size() ==  mapping_B->access_cnt.size());
    unsigned int distance=0;
    unsigned int abs_diff;
    for(int index=0; index < mapping_A->access_cnt.size(); index++){
        abs_diff = min( mapping_A->access_cnt[index], mapping_B->access_cnt[index]);
        distance += abs_diff;
    }
    return distance;
}
unsigned int min(const unsigned int& A, const unsigned int& B){

    if(A > B){
        return B;
    }
    else {
        return A;
    }
}















