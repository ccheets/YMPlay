#ifndef _YFRAME_H_
#define _YFRAME_H_

#include <memory.h>

class YMFrame {
public: 
        unsigned char            reg[16];

		YMFrame() { memset(reg, 0, sizeof(reg)); };
};


#endif // _YFRAME_H_
