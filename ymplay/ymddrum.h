#ifndef _YMDDRUM_H_
#define _YMDDRUM_H_


#include "inttypes.h"


class YMDDrum {         
public: 
        YMDDrum();
        ~YMDDrum();
        ULONG32          size;
        char            *data;
};

#endif // _YMDDRUM_H_
