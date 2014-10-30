#include <libc.h>
#include <crt.h>
#include <types.h>
#include <kfcntl.h>

int main(int argc,const char **argv[])
{
      int i;	
  

     

for( i = 0 ; i <= argc - 1; i++){
_console_write(argv[i]);
_console_write("\n");
}
	_exit() ;
	
                
	return 0;	

}
