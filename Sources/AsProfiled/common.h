#ifndef COMMON_H_
#define COMMON_H_

#include <iostream>
#include <cstdio>
#include <vector>

#define ATTRIBUTE_CONTRACT_NAME "AsContractAttribute"
#define NAME_BUFFER_SIZE 2048
#define DEBUG_ALL 0
#define FAILURE(msg, errNo) std::cout << "ERROR AT: " << __FILE__ << ":" << __LINE__ << ":" << msg << std::endl; exit(errNo);
	

void PrintCharArray(WCHAR*);
char* ToBinary(void*, int , int);
std::vector<std::wstring> Split(const std::wstring& text, WCHAR delimiter); 

#endif // end of COMMON