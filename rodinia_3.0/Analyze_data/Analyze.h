#include <iostream>

typedef unsigned long long new_addr_type;

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
    std::map< new_addr_type,unsigned int > PML4_addr;
    std::map< new_addr_type,unsigned int > PDT_addr;
    std::map< new_addr_type,unsigned int > PD_addr;
    std::map< new_addr_type,unsigned int > PT_addr;

    std::vector<unsigned int> access_cnt;

    int cluster_id;
    thread_info t_info;
};



