#pragma once
#include <string>
#include <cstdlib>
namespace DonorSystems {
inline bool ParseDestination(const std::string &digits,int &date,int &time) {
    if(digits.size()!=4 && digits.size()!=8 && digits.size()!=12)return false;
    for(char c:digits)if(c<'0' || c>'9')return false;
    if(digits.size()>=8){
        const int month=std::atoi(digits.substr(0,2).c_str());
        const int day=std::atoi(digits.substr(2,2).c_str());
        const int year=std::atoi(digits.substr(4,4).c_str());
        date=year*10000+month*100+day;
    }
    if(digits.size()==4)time=std::atoi(digits.c_str());
    else if(digits.size()==12)time=std::atoi(digits.substr(8,4).c_str());
    return true;
}
}
