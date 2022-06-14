#include <iostream>
#include<stdio.h>
#include<sched.h>
#include<unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/wait.h>
#include<sys/mount.h>
#include <fstream>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>


#define STACK 8192
#define SYS_ERROR "system error: "
#define MEM_ERROR "memory allocation failed"
#define CLONE_ERROR "clone failure"
#define UMOUNT_ERROR "umount failure"
#define REMOVE_ERROR "remove failure"
#define WAIT_ERROR "wait failure"
#define HOST_ERROR "hostname failure"
#define CHROOT_ERROR "chroot failure"
#define MKDIR_ERROR "mkdir failure"
#define CHDIR_ERROR "chdir failure "
#define MOUNT_ERROR "mount failure"
#define EXECVP_ERROR "execvp failure"
#define FILE_ERROR "file failure"
#define ARG_NUM_ERROR "not enough arguments given"
#define RMDIR_ERROR "rmdir failed"
#define SUCCESS 0
#define EXIT_FAIL 1
#define FAILURE_CODE -1

int container(void* arg) {
	char** argv = (char **)arg;
	char* new_hostname = argv[1];
	char* new_filesystem_dir = argv[2];
	char* num_processes = argv[3];
	char* path_to_program_to_run_within_container = argv[4];
	char** args_for_program = argv + 5;

	// change hostname
	if (sethostname(new_hostname, strlen(new_hostname)) == FAILURE_CODE) {
	    std::cerr << SYS_ERROR << HOST_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
//	char myname[40+1];
//	gethostname(myname, 40);
//	std::cerr << "HOSTNAME: " << myname << std::endl;

    //change working directory
    if (chdir(new_filesystem_dir) == FAILURE_CODE) {
        std::cerr << SYS_ERROR << CHDIR_ERROR << strerror(errno) << std::endl;
        exit(EXIT_FAIL);
    }

	// change root
	if (chroot(new_filesystem_dir) == FAILURE_CODE) {
	    std::cerr << SYS_ERROR << CHROOT_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}

//    std::cerr << new_filesystem_dir << std::endl;
//    char tmp[256];
//    getcwd(tmp, 256);
//    std::cerr << "Current working directory: " << tmp << std::endl;




	//TODO ERASE - Only for debugging
//	std::cerr << new_filesystem_dir << std::endl;
//	char tmp[256];
//	getcwd(tmp, 256);
//	std::cerr << "Current working directory: " << tmp << std::endl;


//	std::string dirs[] = {"sys", "fs", "cgroups", "pids"};
//    for (std::string s : dirs) {
//        if (s.compare("sys")) { // when equal we do not go into condition
//            if (mkdir(s.c_str(), 0755) == FAILURE_CODE) {
//                std::cerr << s <<std::endl;
//                std::cerr << SYS_ERROR << MKDIR_ERROR << strerror(errno) << std::endl;
//                exit(EXIT_FAIL);
//            }
//        }
//        if (chdir(s.c_str()) == FAILURE_CODE) {
//            std::cerr << s << std::endl;
//            std::cerr << SYS_ERROR << CHDIR_ERROR << std::endl;
//            exit(EXIT_FAIL);
//        }
//    }
    if (mkdir("sys/fs", 0755) == FAILURE_CODE) {
        //std::cerr << s <<std::endl;
        std::cerr << SYS_ERROR << MKDIR_ERROR << strerror(errno) << std::endl;
        exit(EXIT_FAIL);
    }
    if (mkdir("sys/fs/cgroups", 0755) == FAILURE_CODE) {
        //std::cerr << s <<std::endl;
        std::cerr << SYS_ERROR << MKDIR_ERROR << strerror(errno) << std::endl;
        exit(EXIT_FAIL);
    }
    if (mkdir("sys/fs/cgroups/pids", 0755) == FAILURE_CODE) {
        //std::cerr << s <<std::endl;
        std::cerr << SYS_ERROR << MKDIR_ERROR << strerror(errno) << std::endl;
        exit(EXIT_FAIL);
    }

	// To attach the container process into this new cgroup, you need to write the process’s pid into the file “cgroup.procs” under the new directory we created
	std::ofstream cgroupProcs("sys/fs/cgroups/pids/cgroup.procs");
	if (cgroupProcs.bad()) {
	    std::cerr << SYS_ERROR << FILE_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
	cgroupProcs << "1";
	cgroupProcs.close();

	// Next, we need to write into the file “pids.max” the number of processes which is allowed
	std::ofstream pidsMax("sys/fs/cgroups/pids/pids.max");
	if (pidsMax.bad()) {
	    std::cerr << SYS_ERROR << FILE_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
	pidsMax << num_processes;
	pidsMax.close();


	// Finally, to release the resources when the container is finished, we will write into the file “notify_on_release” the value 1
	std::ofstream notifyOnRelease("sys/fs/cgroups/pids/notify_on_release");
	if (notifyOnRelease.bad()) {
	    std::cerr << SYS_ERROR << FILE_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
	notifyOnRelease << "1";
	notifyOnRelease.close();

//    if (mkdir("/sys/fs/cgroups/pids/", S_IRUSR | S_IWUSR) == FAILURE_CODE) {
//        std::cerr << SYS_ERROR << MKDIR_ERROR << std::endl;
//        exit(EXIT_FAIL);
//    }
//
//
//    int fp1 = open("/sys/fs/cgroups/pids/cgroup.procs", O_WRONLY | O_APPEND );
//    write(fp1, "1", strlen("1"));
//    close(fp1);
//
//    int fp2 = open("/sys/fs/cgroups/pids/pids.max", O_WRONLY | O_APPEND );
//    write(fp2, num_processes, strlen(num_processes));
//    close(fp2);
//
//    int fp3 = open("/sys/fs/cgroups/pids/notify_on_release", O_WRONLY | O_APPEND );
//    write(fp3, "1", strlen("1"));
//    close(fp3);

    //return to file directory
//	if (chdir(new_filesystem_dir) == FAILURE_CODE) {
//	    std::cerr << new_filesystem_dir << std::endl;
//	    std::cerr << SYS_ERROR << CHDIR_ERROR << std::endl;
//	    exit(EXIT_FAIL);
//	}


	//Mount
	int c = mount("proc", "/proc", "proc", 0, 0);
	if (c != SUCCESS) {
	    std::cerr << c << std::endl;
	    std::cerr << strerror(errno) << std::endl;
	    std::cerr << SYS_ERROR << MOUNT_ERROR << std::endl;
	    //exit(EXIT_FAIL);
	}

    char tmp[256];
    getcwd(tmp, 256);
    std::cerr << "Current working directory: " << tmp << std::endl;

	//  run the terminal/new program
	int ret = execvp(path_to_program_to_run_within_container, args_for_program);
    if (ret == FAILURE_CODE) {
        std::cerr << path_to_program_to_run_within_container << std::endl;
        std::cerr << SYS_ERROR << EXECVP_ERROR << strerror(errno) << std::endl;
        exit(EXIT_FAIL);
    }
	return 0;
}


int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << SYS_ERROR << ARG_NUM_ERROR << std::endl;
        exit(EXIT_FAIL);
    }
	char* stack = (char*) malloc(STACK);
	if (stack == nullptr) {
	    std::cerr << SYS_ERROR << MEM_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
	std::string new_filesystem_dir = argv[2];
	int container_pid = clone(container, stack + STACK, CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD, argv);
	if (container_pid == FAILURE_CODE) {
	    std::cerr << SYS_ERROR << CLONE_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
	if (wait(nullptr) == FAILURE_CODE) {
	    std::cerr << SYS_ERROR << WAIT_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
	if (umount((new_filesystem_dir + "/proc").c_str()) == FAILURE_CODE) {
	    std::cerr << SYS_ERROR << UMOUNT_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
	if (remove((new_filesystem_dir + "/sys/fs/cgroups/pids/cgroup.procs").c_str()) != SUCCESS) {
	    //std::cerr << (new_filesystem_dir + "/sys/fs/cgroups/pids/cgroup.procs").c_str() << std::endl;
	    std::cerr << SYS_ERROR << REMOVE_ERROR << strerror(errno) << std::endl;
	    exit(EXIT_FAIL);
	}
	if (remove((new_filesystem_dir + "/sys/fs/cgroups/pids/pids.max").c_str()) != SUCCESS) {
	    std::cerr << SYS_ERROR << REMOVE_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
	if (remove((new_filesystem_dir + "/sys/fs/cgroups/pids/notify_on_release").c_str()) != SUCCESS) {
	    std::cerr << SYS_ERROR << REMOVE_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}

	if (rmdir((new_filesystem_dir + "/sys/fs/cgroups/pids").c_str()) == FAILURE_CODE) {
        std::cerr << SYS_ERROR << RMDIR_ERROR << std::endl;
        exit(EXIT_FAIL);
    }
	if (rmdir((new_filesystem_dir + "/sys/fs/cgroups").c_str()) == FAILURE_CODE) {
	    std::cerr << SYS_ERROR << RMDIR_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}
	if (rmdir((new_filesystem_dir + "/sys/fs").c_str()) == FAILURE_CODE) {
	    std::cerr << SYS_ERROR << RMDIR_ERROR << std::endl;
	    exit(EXIT_FAIL);
	}


//	if (chdir("sys/fs/cgroups") == FAILURE_CODE) {
//	    std::cerr << SYS_ERROR << CHDIR_ERROR << std::endl;
//	    exit(EXIT_FAIL);
//	}
//	std::string dirs[] = {"pids", "cgroups", "fs"};
//	for (std::string s : dirs) {
//	    if (rmdir(s.c_str()) == FAILURE_CODE) {
//	        std::cerr << SYS_ERROR << RMDIR_ERROR << std::endl;
//	        exit(EXIT_FAIL);
//	    }
//	    if (s.compare("sys")) { // when equal we do not go into condition
//	        if (chdir("..") == FAILURE_CODE) {
//	            std::cerr << SYS_ERROR << CHDIR_ERROR << std::endl;
//	            exit(EXIT_FAIL);
//	        }
//	    }
//
//	}
	free(stack);
}





