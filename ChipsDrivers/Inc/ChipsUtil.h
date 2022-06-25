/*
 * ChipsUtil.h
 *
 *  Created on: Mar 9, 2022
 *      Author: larbi
 */

#ifndef INC_CHIPSUTIL_H_
#define INC_CHIPSUTIL_H_

//#define __STDC_LIMIT_MACROS

#include "main.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "string.h"
#include <stdarg.h>
#include <vector>
#include <set>



typedef enum
{
	STATUS_ERROR = 0,
	STATUS_OK = !STATUS_ERROR
}
DriversExecStatus; // execution status

typedef enum
{
	LOW = 0,
	HIGH = !LOW
}
DigitalStateTypeDef;


int findSubString(char* string, const char* substring, int index = 0);
int findSubString(std::string string, std::string substring, bool reverse = false, int index = 0);

std::string char_arr_to_string(char* string);

uint32_t str_to_hex(char* str);



template<typename T>
inline int std_set_find_index(std::set<T> S, T K)
{
    // To store the index of K
    int Index = 1;

    // Traverse the Set
    for (auto u : S) {

        if (u == K)
            return Index-1;

        Index++;
    }

    // If K is not present
    // in the set
    return -1;
}


#endif /* INC_CHIPSUTIL_H_ */
