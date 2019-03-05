#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#ifndef PROFILE_DATA_H
#define PROFILE_DATA_H

class profile_data{
   std::unordered_map<std::string, std::unordered_map<std::string, double>> profile;//{"funcition_name": {"arg0_arg_1" : msec} }
public:
   profile_data(){
   }
   profile_data(std::string profile){
      load_profile(profile);
   }
   //load_profile("profile/1_core/5x5_grid_16_layers_1_core.prof", profile);
   long long get_latency_in_psec(std::string function_name, std::string input){
      long long psec;
      if(profile.find(function_name) == profile.end()) return 0;
      if(profile[function_name].find(input) == profile[function_name].end()) return 0;
      psec = profile[function_name][input]*1000000;
      return psec;
   }
   double get_latency_in_sec(std::string function_name, std::string input){
      double sec;
      sec = profile[function_name][input]/1000000;
      return sec;
   }
   void load_profile(std::string profile_file){
      std::string function_name;	
      int frame;
      int partition;
      int reuse;
      int calling_times;
      double avg_time;
      std::ifstream infile(profile_file);
      std::string line;
      getline(infile, line);//skip the first line
      while (getline(infile, line)){
         std::istringstream iss(line);
         if (!(iss >> function_name >> frame >> partition >> reuse >> calling_times >> avg_time)) break;
         std::cout << function_name << "	" << frame << "	" << partition << "	" << reuse << "	" << calling_times << "	" << avg_time << std::endl;
	 std::string function_input = std::to_string(frame) + "_" + std::to_string(partition) + "_" + std::to_string(reuse);
         profile[function_name][function_input] = avg_time;
         if(frame==0 && partition==0 && reuse==0) profile[function_name]["null"] = avg_time;
      }
   }
};
#ifdef __cplusplus
extern "C" {
#endif
void sys_time_wait(char* function_name, char* input);
#ifdef __cplusplus
}
#endif
#endif
