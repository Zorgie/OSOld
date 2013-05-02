#include <unistd.h>
#include <stdio.h>
#include "docmd.h"

int main(int argc, char* argv[], char* env[])
{
	return doCmd(argc, argv);
}
