#pragma once
namespace DonorSystems {
inline int DaysInMonth(int year,int month){
    const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(month<1 || month>12)return 0;
    return days[month-1]+(month==2 && year%4==0 && (year%100!=0 || year%400==0));
}
inline int NextDate(int date){
    int year=date/10000,month=date/100%100,day=date%100;
    if(date>=99991231)return 99991231;
    if(++day>DaysInMonth(year,month)){day=1;if(++month>12){month=1;++year;}}
    return year*10000+month*100+day;
}
struct TravelCalendar {
    int previousHour=-1;
    void Rebase(int hour){previousHour=hour;}
    int Update(int date,int hour){
        // TimeMod.txt advances the date when the game-clock hour wraps.
        if(previousHour>=0 && hour<previousHour)date=NextDate(date);
        previousHour=hour;
        return date;
    }
};
}
