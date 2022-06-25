/*
 * ChipsUtil.cpp
 *
 *  Created on: 8 juin 2022
 *      Author: larbi
 */
#include "ChipsUtil.h"

int findSubString(char* string, const char* substring, int index)
{
	char* _string = &string[index];

	char *result = strstr(_string, substring);

	if (result != NULL)
		return (result - _string);
	else
		return (-1);
}

int findSubString(std::string string, std::string substring, bool reverse, int index)
{
	if (reverse)
	{
	    size_t found = string.rfind(substring);

	    if (found != std::string::npos)
	    	return (found);
	    else
	    	return (-1);
	}
	else
	{
	    size_t found = string.find(substring, index);

	    if (found != std::string::npos)
	    	return (found);
	    else
	    	return (-1);
	}
}

std::string char_arr_to_string(char* string)
{
	std::string s(string);

	return (s);
}

uint32_t str_to_hex(char* str)
{
	uint32_t x;

	std::stringstream ss;
    ss << std::hex << str;
    ss >> x;

    return (x);
}


// Function to calculate index
// of element in a Set

//template<typename T>
//int std_set_find_index(std::set<T> S, T K)
//{
//    // To store the index of K
//    int Index = 1;
//
//    // Traverse the Set
//    for (auto u : S) {
//
//        if (u == K)
//            return Index-1;
//
//        Index++;
//    }
//
//    // If K is not present
//    // in the set
//    return -1;
//}
