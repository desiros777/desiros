
#include <libc.h>
#include <crt.h>
#include <types.h>
#include <kfcntl.h>

int main(int argc ,char *argv[])
{
      int i;	
  
_console_write("--------------------------------------------------\n");
	_console_write("Run user prog 1\n");
       

 	int fd = -1;
 	fd = _open ("/core/boot/teste", O_RDWR );

	if(fd != -1 )
	_console_write("vfs open ... OK \n");

	char* bufw ="(USERLAND) Now the kernel can read and write files\n";
	_write( fd, bufw, 1000 );
      
       char buf[1000];
       _read( fd, buf,512);
       _console_write(buf);

       _console_write("Exit prog 1\n");
_console_write("--------------------------------------------------\n");
      
char *args[] = {"/core/boot/myprog2", "-r", "-t", "-l", (char *) 0 };

_exec("/core/boot/myprog2",args,sizeof(args));

	_exit() ;
	
                
	return 0;	

}
