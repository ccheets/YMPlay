#include "ymddrum.h"

YMDDrum::YMDDrum()
{
        // Initialise variables
        size = 0;
        data = 0;
}

YMDDrum::~YMDDrum()
{
        delete  data;
}

