#include "dst.h"

const int BEGIN_DST_MONTH = 3;
const int END_DST_MONTH = 10;

const int SUMMER_TIME = 7200;

int adjustDstEurope(int year, int month, int day)
{
  // last sunday of march
  int beginDSTDate = (31 - (5 * year / 4 + 4) % 7);
  //last sunday of october
  int endDSTDate = (31 - (5 * year / 4 + 1) % 7);
  //Serial.println(endDSTDate);
  // DST is valid as:
  if (((month > BEGIN_DST_MONTH) && (month < END_DST_MONTH)) || 
      ((month == BEGIN_DST_MONTH) && (day >= beginDSTDate)) || 
      ((month == END_DST_MONTH) && (day <= endDSTDate)))
  {
    return SUMMER_TIME; // DST europe = utc +2 hour
  }

  return 3600; // nonDST europe = utc +1 hour
}
