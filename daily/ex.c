#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int main(){
	char c;
	while(read(0,&c,1) != 0)
		write(1,&c,1);
	exit(0);
}
