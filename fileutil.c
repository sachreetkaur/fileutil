#define _XOPEN_SOURCE 500
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/wait.h>
#include<ftw.h>

//global variables
int argcount ; // hold argument count 
const char *search; // hold filename from terminal
const char *sourcepath; // hold source where we need to search for a file
const char *destpath; // hold destination where files are copied or moved
const char *opt; // hold an option for copy and move
char *files[1000]; // hold files array to make zip
int count; // hold value for number of files found inside the storage location and also to check file exit or not inside the root directory

//init function to set values to the global variables
int init(int argc, char *argv[]){
	argcount = argc;
	sourcepath = argv[1];
	switch(argc){
		case 3:
			search =  argv[2];
			break;
		case 4:
			search = argv[3];
			destpath =  argv[2];
			break;
		case 5:
			if(strcmp(argv[3],"-cp")!=0 && strcmp(argv[3],"-mv")!=0){
				printf("\n  Invalid option provided. Only options -cp and -mv are allowed");
				return -1;
			}
			destpath = argv[2];
			opt = argv[3];
			search = argv[4];
			break;
		default:
			printf("\n Invalid argument count %d \n",argc);
			printf("\n Usage ");
			printf("\n 1. fileutil <root_dir> filename ");
			printf("\n 2. fileutil <root_dir> <storage_dir> <option> filename");
			printf("\n 3. fileutil <root_dir> <storage_dir> filename");
			return -1;
	}
	return 0;
}

//  modified pointer to get filename or file extension
const char * getname(const char *path, char delimeter){
	for(int i=strlen(path);i>0;i--){
		if(path[i]==delimeter){
			path = path + i + (delimeter == '.' ? 0 : 1); 
			break;
		}
	}
	return path;
}

//Function is used to check nftw traverse file name is equivalent to user supplied file name
void srchfile(const char *path){
	if(strcmp(getname(path,'/'),search)==0){
		printf("\n %d. %s", ++count,path);
	}
}

// function is used to copy file from path to destpath
int cpy(const char *path, char *destpath){
	// open source file for reading operations
	int rdfd = open(path, O_RDWR);
	if(rdfd==-1){
		printf("\n Ooops !! not able to open file for copying ");
		return -1;
	}else{
		//0644 =  read / write permission for owner and read permission for others.
		int wrfd = open(destpath,O_CREAT | O_WRONLY,0644);
		if(wrfd == -1){
			printf(" Oops !! not able to create file ");
			return -1;
		}else{
			int bytes = 1;
			char buffer;
			// read 1 byte from source file and write that 1 byte to destination file until source file is emptied
			while((bytes=read(rdfd,&buffer,1))>0){
				write(wrfd,&buffer,1);
			}
			close(rdfd);// close source file
			close(wrfd);// close destination file
			return 0;
		}
	}


}
// function for move and copy
int mvcpy(const char *path, char *destpath){
	// option for move 
	if(strcmp(opt,"-mv")==0){
		// use rename system call to  move file from one location to another location
		if(rename(path,destpath)==0){
			printf(" \n file moved successfull from %s to %s ",path,destpath);
			return 0;
		}
	}// option for copy
	else if(strcmp(opt,"-cp")==0){
		// use cpy function to copying a file from one location to another location
		if(cpy(path,destpath)==0){
			printf("\n file copied from %s to %s ", path,destpath);
			return 0;
		}
	}
	return -1;
}

// functio to create zip files
int zip(){
	// create a chile process
	int chldPID = fork();
	if(chldPID==-1){
		printf("\n unable to create child process ");
		return -1;
	}
	else if(chldPID==0){ // child process execution code
		struct stat buffer;
		int check = stat(destpath,&buffer);
		// if directory does not exist make directory with 0755 permission
		if(check==-1 && mkdir(destpath,0755)==-1){
			printf("\n unable to make directory \n");
			return -1;
		}
		// use chdir system call to change the directory 
		if(chdir(destpath)==-1){
			printf("\n unable to change directory \n");
			// unable to change directory 
			return -1;
		}
		// prepare command to 
		// tar -cf a1.tar --absolute-names file1 file2 file3 
		//to create a tar file inside the current working directory of the child process 
		char *args[100];
		args[0] = "tar";
		args[1] = "-cf";
		args[2] = "a1.tar";
		args[3] = "--absolute-names";
		// create argument array to pass execvp system call
		for(int i=0;i<count;i++){
			args[i+4] = files[i];
		}
		args[count+4] = NULL;
		int execute = execvp("tar", args);
		if(execute==-1){
			//execvp system call error
			printf("\n tar file error ");
			exit(0);
		}
	}else{
		int status;
		wait(&status);// parent process is in waiting status until child process complete its excecution.
		if(WIFEXITED(status)){
			if(WEXITSTATUS(status)==0){
				printf("\n Tar file a1.tar created at %s ", destpath);
				return 0;
			}else{
				printf("\n Not able to create ");
			}
		}else{
			printf("\n Signalled error %d ",WTERMSIG(status));
		}
		return -1;
	}

}


// user supplied function that is call for each file that is report by nftw system call.
// path :  file path
// buffer :  file stat buffer containing additional information about the file or directory
// filetype : specifies the file is directory , file or symbolik link
// filebuff : specifies the offset and level of the directory.
int usrfunction(const char *path, const struct stat *buffer, int filetype, struct FTW  *filebuff){
	if(filetype == 0){
		char spath[strlen(path)+1];
		strcpy(spath,path);
		if(argcount==3){
			srchfile(path);
			return 0;
		}else if(argcount==5 && strcmp(getname(spath,'/'),search)==0){
			char path2[strlen(destpath)+strlen(search)+1+1];
			strcpy(path2, destpath);
			strcat(path2,"/");
			strcat(path2,search);// prepare destination path for copy and move 
			int ret = mvcpy(path,path2);
			if(ret==-1){
				printf("\n command execution failed ");
			}
			count++;
			return 1;
		}else if(argcount==4 && strcmp(getname(spath,'.'),search)==0){
			printf("\n %d. %s", count+1,path);
			// prepare array of pointer to string and allocate  dynamic memory using malloc
			files[count] = malloc((strlen(path)+1)*sizeof(char));
			// copy content to the array
			strcpy(files[count++],path);
			return 0;
		}
	}
	// for other than file, return 0 to nftw to continue traverse the directory
	return 0;
}

// cleanup function that will free the memory allocate by the malloc
void cleanup(){
	for(int i=0;i<=count;i++){
		free(files[i]);
	}
}

int main(int argc, char *argv[]){
	// call to init function with argc and argv to intialize the global variable
	if(init(argc,argv)==-1)
		exit(EXIT_FAILURE);


	// nftw system call 
	// 1. sourcepath :  where nftw starts  traversing the directory strcture
	// 2. usrfunction :  user supplied function this call by nftw for each object during traversal
	// 3. fd_limit : maximum number of file descriptor used at each level 
	// 4. flag : FTW_PHYS physically deep down the directory instead of following the symbolink links
	int ret = nftw(sourcepath, usrfunction, 200, FTW_PHYS);

	if(ret==-1){
		printf("\n nftw system call error %d \n",ret);
		exit(EXIT_FAILURE);
	}

	if(count==0){
		printf("Search unsucessful");
		exit(EXIT_FAILURE);
	}

	if(count>0){
		printf("\n Search successfull");
	}

	if(argc==4){
		//call zip function to create a zip file 
		if(zip()==0){
			printf("\n Command executed successfully ");
		}else{
			printf("\n Command execution failed ");
		}
	}
	printf("\n");
	return 0;
}
